#include "secrets.h"
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "WiFi.h"
#include "max6675.h"

// ========== CONFIGURAÇÃO DO MAX6675 ==========
int so = 19;
int cs = 5;
int sck = 18;
MAX6675 thermocouple(sck, cs, so);

// ========== CONFIGURAÇÃO AWS IoT ==========
#define AWS_IOT_PUBLISH_TOPIC   "*******"         // Alterar para o nome do tópico presente na política do AWS IoT Core
#define AWS_IOT_SUBSCRIBE_TOPIC "*******"         // Alterar para o nome do tópico presente na política do AWS IoT Core

WiFiClientSecure net = WiFiClientSecure();
PubSubClient client(net);

// ========== VARIÁVEIS GLOBAIS ==========
float temperatura = 0.0;
float potencia_atual = 0.0;  // 0-100%

// ========== CONFIGURAÇÕES DE CONTROLE ==========
const float TEMP_ALVO = ****;         // Insira a temperatura alvo de acordo com a temperatura de fusão do material utilizado
const float FAIXA_CONTROLE = 15.0;    // Inicia redução quando T >= TEMP_ALVO - 15  -- Quanto maior a Faixa mais suave a transição de temperatura
const float POTENCIA_MINIMA = 8.0;    // % mínima para manter
const float POTENCIA_MAXIMA = 100.0;  // % máxima

// ========== PWM PARA SSR ==========
#define SAIDA 2
const int PERIODO_PWM = 2000;  // 2 segundos (em ms)
bool saidaEstado = LOW;

// ========== CONTROLE DE PUBLICAÇÃO ==========
unsigned long ultima_publicacao = 0;
const int INTERVALO_PUBLICACAO = 5000;  // 5 segundos

// ========== PROTÓTIPOS ==========
void connectAWS();
void publishMessage();
void messageHandler(char* topic, byte* payload, unsigned int length);
float calcularPotencia(float temp);
void controlarSSR();

// ========== CONEXÃO AWS ==========
void connectAWS()
{
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.println("Connecting to Wi-Fi");

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWi-Fi connected!");

  net.setCACert(AWS_CERT_CA);
  net.setCertificate(AWS_CERT_CRT);
  net.setPrivateKey(AWS_CERT_PRIVATE);

  client.setServer(AWS_IOT_ENDPOINT, 8883);
  client.setCallback(messageHandler);

  Serial.println("Connecting to AWS IOT");

  while (!client.connect(THINGNAME))
  {
    Serial.print(".");
    delay(100);
  }

  if (!client.connected())
  {
    Serial.println("AWS IoT Timeout!");
    return;
  }

  client.subscribe(AWS_IOT_SUBSCRIBE_TOPIC);
  Serial.println("AWS IoT Connected!");
}

// ========== PUBLICAR MENSAGEM ==========
void publishMessage()
{
  // Documento JSON apenas com temperatura
  StaticJsonDocument<64> doc;
  doc["temp"] = temperatura;
  
  char jsonBuffer[128];
  serializeJson(doc, jsonBuffer);
  
  if (client.publish(AWS_IOT_PUBLISH_TOPIC, jsonBuffer)) {
    Serial.println("Temperatura Publicada no IoT Core");
  } else {
    Serial.println("Erro de Publicação no IoT Core");
  }
}

// ========== HANDLER DE MENSAGENS ==========
void messageHandler(char* topic, byte* payload, unsigned int length)
{
  Serial.print("Incoming: ");
  Serial.println(topic);

  StaticJsonDocument<200> doc;
  deserializeJson(doc, payload);
  const char* message = doc["message"];
  
  if (message) {
    Serial.println(message);
  }
}

// ========== CÁLCULO DE POTÊNCIA COM CURVA SUAVE ==========
float calcularPotencia(float temp) {
  float erro = TEMP_ALVO - temp;
  
  // Caso 1: Muito abaixo do alvo - potência máxima
  if (erro > FAIXA_CONTROLE) {
    return POTENCIA_MAXIMA;
  }
  
  // Caso 2: Atingiu ou passou do alvo - potência mínima
  if (erro <= 0) {
    return POTENCIA_MINIMA;
  }
  
  // Caso 3: Zona de controle suave (curva quadrática)
  float progresso = 1.0 - (erro / FAIXA_CONTROLE);
  float fator = progresso * progresso;
  float potencia = POTENCIA_MAXIMA - (fator * (POTENCIA_MAXIMA - POTENCIA_MINIMA));
  
  return constrain(potencia, POTENCIA_MINIMA, POTENCIA_MAXIMA);
}

// ========== CONTROLE PWM DO SSR ==========
void controlarSSR() {
  unsigned long agora = millis();
  unsigned long tempo_no_ciclo = agora % PERIODO_PWM;
  
  unsigned long tempo_ligado = (potencia_atual * PERIODO_PWM) / 100;
  
  bool novo_estado = (tempo_no_ciclo < tempo_ligado);
  
  if (novo_estado != saidaEstado) {
    saidaEstado = novo_estado;
    digitalWrite(SAIDA, saidaEstado ? HIGH : LOW);
  }
}

// ========== SETUP ==========
void setup()
{
  pinMode(SAIDA, OUTPUT);
  digitalWrite(SAIDA, LOW);
  
  Serial.begin(115200);
  Serial.println("=== SISTEMA DE AQUECIMENTO ===");
  Serial.print("Alvo: ");
  Serial.print(TEMP_ALVO);
  Serial.println(" °C");
  
  connectAWS();
  delay(1000);
}

// ========== LOOP ==========
void loop()
{
  // Verifica conexão AWS
  if (!client.connected()) {
    Serial.println("Restabelecendo conexão ao AWS");
    connectAWS();
  }

  // Lê temperatura do MAX6675
  float t = thermocouple.readCelsius();
  
  if (isnan(t)) {
    Serial.println("Verifique ligação do sensor de temperatura");
  } else {
    temperatura = t;
    
    // Calcula potência com curva suave
    potencia_atual = calcularPotencia(temperatura);
    
    // Controla o SSR via PWM
    controlarSSR();
    
    // Debug no Serial
    Serial.print("T = ");
    Serial.print(temperatura, 2);
    Serial.print(" °C | Potência: ");
    Serial.print(potencia_atual, 0);
    Serial.print("% | Saída:");
    Serial.println(saidaEstado ? " ON" : " OFF");
    
    // Publica APENAS temperatura na AWS a cada 5 segundos
    if (millis() - ultima_publicacao >= INTERVALO_PUBLICACAO) {
      publishMessage();
      ultima_publicacao = millis();
    }
  }
  
  client.loop();
  delay(50);  // 50ms para resposta rápida do PWM
}