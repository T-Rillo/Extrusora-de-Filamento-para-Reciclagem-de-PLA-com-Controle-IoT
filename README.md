# Extrusora de Filamento para Reciclagem de PLA com Controle IoT

![Status do Projeto](https://img.shields.io/badge/Status-Em_Desenvolvimento-yellow)
![Plataforma](https://img.shields.io/badge/Platform-ESP32-blue)
![IoT](https://img.shields.io/badge/Cloud-AWS_IoT_Core-orange)
![Licença](https://img.shields.io/badge/License-MIT-green)

Este projeto consiste em uma extrusora de filamento de baixo custo projetada para reciclar purgas de impressão 3D e estruturas de suporte. O sistema realiza o controle preciso de temperatura da resistência de aquecimento e a velocidade do motor de tração, permitindo o monitoramento remoto em tempo real via AWS IoT Core.

## 📋 Índice
- [Visão Geral](#-visão-geral)
- [Funcionalidades](#-funcionalidades)
- [Arquitetura do Sistema](#-arquitetura-do-sistema)
- [Hardware Necessário](#-hardware-necessário)
- [Esquema de Ligação](#-esquema-de-ligação)
- [Configuração do Software](#-configuração-do-software)
- [Como Replicar](#-como-replicar)
- [Monitoramento e Debug](#-monitoramento-e-debug)
- [Segurança](#-segurança)
- [Licença](#-licença)
- [Referências](#-referências)

---

## 🔍 Visão Geral

O objetivo deste projeto é criar uma solução sustentável para o desperdício de material na impressão 3D. A máquina utiliza um sistema de aquecimento controlado por PWM (Pulse Width Modulation) para manter a temperatura ideal de extrusão e um motor DC para tracionar o filamento.

O cérebro do sistema é um **ESP32**, que gerencia:
1.  Leitura da temperatura via termopar tipo K.
2.  Controle da potência do aquecedor via SSR (Solid State Relay).
3.  Conexão Wi-Fi e comunicação MQTT com a AWS para telemetria.

![Extrusora](imagens/extrusora.jpeg)

## 🚀 Funcionalidades

- **Controle de Temperatura PID/Curva Suave:** Algoritmo de controle que ajusta a potência do aquecedor suavemente para evitar overshoot (ultrapassagem da temperatura alvo).
- **Leitura de Temperatura:** Utiliza o módulo MAX6675 com termopar Tipo-K para leituras precisas até 1000°C.
- **Acionamento via SSR:** Controle de potência por PWM de baixa frequência (janela de 2 segundos) utilizando um Solid State Relay (Fotek SSR-40DA).
- **Conectividade AWS IoT Core:** Publicação periódica da temperatura e status do sistema via protocolo MQTT sobre TLS.
- **Monitoramento Remoto:** Possibilidade de acompanhar a curva de aquecimento em dashboards na nuvem.

## 🏗 Arquitetura do Sistema

O sistema é dividido em três partes principais:

1.  **Controle Térmico:** O ESP32 lê a temperatura e calcula a potência necessária (0-100%). O SSR é acionado proporcionalmente para manter a temperatura estável no *setpoint*.
2.  **Tração (Motor):** Um controlador de velocidade independente gerencia o motor DC "Para-brisa" responsável por puxar o filamento.
3.  **Interface IoT:** O ESP32 envia dados JSON para a AWS IoT Core a cada 5 segundos.

## 🛠 Hardware Necessário

- **Controlador:** ESP32 DevKit V1
- **Sensor de Temperatura:** Módulo MAX6675 + Termopar Tipo-K
- **Atuador de Potência:** SSR (Solid State Relay) - Ex: Fotek SSR-40DA
- **Fonte de Alimentação:**
    - Fonte 110V/220V para 12V (para o motor e ESP32)
    - Rede elétrica (110V/220V) para o aquecedor via SSR
- **Motor:** Motor DC 12V (tipo para-brisa) com controlador de velocidade PWM
- **Aquecedor:** Resistência elétrica de cartucho ou banda acoplada ao cano de extrusão.

### 🔌 Esquema de Ligação

| Componente | Pino no ESP32 | Função |
| :--- | :--- | :--- |
| **MAX6675** | VCC -> 3.3V | Alimentação |
| | GND -> GND | Terra |
| | SCK -> GPIO 18 | Clock SPI |
| | CS -> GPIO 5 | Chip Select |
| | SO -> GPIO 19 | Data Out |
| **SSR (Fotek)** | Terminal (+) -> GPIO 2 | Sinal de Controle PWM |
| | Terminal (-) -> GND | Terra |

![Diagrama Elétrico](imagens/diagrama-eletrico.png)


> **⚠️ Atenção:** O acionamento do aquecedor (110V/220V) deve ser feito exclusivamente através do SSR. Nunca ligue a rede elétrica diretamente ao ESP32.

## 💻 Configuração do Software

### Bibliotecas Necessárias (Arduino IDE)

Instale as seguintes bibliotecas via Gerenciador de Bibliotecas:
1.  **PubSubClient** (by Nick O'Leary)
2.  **ArduinoJson** (by Benoit Blanchon)
3.  **MAX6675** (by Adafruit)
4.  **WiFiClientSecure** (Nativa do ESP32)

### Arquivo `secrets.h`

Você precisará criar um arquivo `secrets.h` na mesma pasta do código `.ino` com suas credenciais. **Não suba este arquivo para o GitHub.**

```cpp
#include <pgmspace.h>

#define SECRET
#define THINGNAME "NomeDoSeuThingAWS"

const char WIFI_SSID[] = "NOME_DA_SUA_REDE";
const char WIFI_PASSWORD[] = "SENHA_DA_SUA_REDE";
const char AWS_IOT_ENDPOINT[] = "xxxxxx-ats.iot.regiao.amazonaws.com";

// Certificados AWS (Copie o conteúdo dos arquivos baixados na AWS)
static const char AWS_CERT_CA[] PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
...
-----END CERTIFICATE-----
)EOF";

static const char AWS_CERT_CRT[] PROGMEM = R"KEY(
-----BEGIN CERTIFICATE-----
...
-----END CERTIFICATE-----
)KEY";

static const char AWS_CERT_PRIVATE[] PROGMEM = R"KEY(
-----BEGIN RSA PRIVATE KEY-----
...
-----END RSA PRIVATE KEY-----
)KEY";
```

### Ajuste de Parâmetros no Código Principal

No arquivo `esp_IoT.ino`, ajuste as constantes conforme o material que será extrudado:

```cpp
// Configurações de Controle
const float TEMP_ALVO = 130.0;        // Temperatura alvo (Ex: 130°C para PLA/Purga)
const float FAIXA_CONTROLE = 15.0;    // Zona onde a potência começa a reduzir
const float POTENCIA_MINIMA = 8.0;    // % mínima para manter o aquecimento
const float POTENCIA_MAXIMA = 100.0;  // % máxima
```

## 🔄 Como Replicar

1.  **Montagem Mecânica:** Construa a estrutura da extrusora (tubo de aquecimento, funil de alimentação e sistema de tração).
2.  **Conexões Elétricas:** Siga o diagrama esquemático para ligar o MAX6675, o SSR e o Motor.
    - *Nota:* O motor "Para-brisa" geralmente é controlado por um módulo PWM separado (conforme visto na foto do projeto), que recebe a alimentação de 12V.
3.  **Configuração da Nuvem (AWS):**
    - Crie um "Thing" no AWS IoT Core.
    - Gere e baixe os certificados.
    - Crie uma política permitindo `iot:Connect`, `iot:Publish`, `iot:Subscribe` e `iot:Receive`.
4.  **Upload do Código:** Compile e faça o upload do código para o ESP32.
5.  **Operação:** Ligue a máquina. O sistema iniciará a curva de aquecimento e conectará à AWS.

## 📊 Monitoramento e Debug

O código possui logs detalhados via **Serial Monitor** (115200 baud).

Exemplo de saída:
```text
=== SISTEMA DE AQUECIMENTO ===
Alvo: 130.00 °C
Connecting to Wi-Fi...
Wi-Fi connected!
Connecting to AWS IOT...
AWS IoT Connected!
T = 25.00 °C | Potência: 100% | Saída: ON
...
T = 120.00 °C | Potência: 45% | Saída: OFF
Temperatura Publicada no IoT Core
```

### Tópicos MQTT

- **Publicação (Telemetria):** `{ "temp": 125.50 }`
- **Assinatura (Comandos):** `{ "message": "comando" }`

## 🔒 Segurança

- **Certificados:** Mantenha os arquivos de certificado e chave privada seguros. Eles permitem o controle total do seu dispositivo.
- **Isolamento:** Recomenda-se utilizar uma fonte de alimentação isolada para o ESP32 e o motor para evitar ruídos elétricos que possam reiniciar o microcontrolador quando o SSR ou o motor forem acionados.
- **Fusíveis:** Instale fusíveis adequados na entrada de energia do aquecedor e do motor para proteção contra curto-circuitos.

## 📄 Licença

Este projeto está licenciado sob a Licença MIT. Isso significa que você pode usar, copiar, modificar, mesclar, publicar, distribuir, sublicenciar e/ou vender cópias do software, desde que inclua o aviso de copyright original e a permissão.

Consulte o arquivo [LICENSE](LICENSE) para obter o texto completo da licença.

## 📚 Referências

Abaixo estão as referências técnicas e bibliográficas utilizadas para o desenvolvimento deste projeto:

- **AWS IoT Core Developer Guide:** [https://docs.aws.amazon.com/iot/latest/developerguide/what-is-aws-iot.html](https://docs.aws.amazon.com/iot/latest/developerguide/what-is-aws-iot.html)
- **ESP32 AWS IoT Core Tutorial (How2Electronics):** [https://how2electronics.com/connecting-esp32-to-amazon-aws-iot-core-using-mqtt/](https://how2electronics.com/connecting-esp32-to-amazon-aws-iot-core-using-mqtt/)
- **MAX6675 Datasheet (Adafruit):** [https://cdn-shop.adafruit.com/datasheets/MAX6675.pdf](https://cdn-shop.adafruit.com/datasheets/MAX6675.pdf)
- **PubSubClient Library:** [https://pubsubclient.knolleary.net/](https://pubsubclient.knolleary.net/)

---
Desenvolvido para fins de pesquisa em reciclagem de polímeros e automação IoT.


