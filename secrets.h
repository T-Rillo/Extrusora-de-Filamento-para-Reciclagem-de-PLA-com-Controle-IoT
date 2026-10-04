#include <pgmspace.h>
 
#define SECRET  
#define THINGNAME "*****"                                     //Mudar para thingname configurado no AWS IoT Core
 
const char WIFI_SSID[] = "*****";                             //Nome da rede WI-FI para conectar o ESP32 
const char WIFI_PASSWORD[] = "******";                        //Senha do WI-FI
const char AWS_IOT_ENDPOINT[] = "*******";                    //Endpoint do AWS IoT Core
 

// Abaixo vão os certificados baixados no momento da criação no AWS IoT Core

// Amazon Root CA 1
static const char AWS_CERT_CA[] PROGMEM = R"EOF(               
-----BEGIN CERTIFICATE-----

-----END CERTIFICATE-----

)EOF";
 
// Device Certificate                                              
static const char AWS_CERT_CRT[] PROGMEM = R"KEY(
-----BEGIN CERTIFICATE-----

-----END CERTIFICATE-----
 
)KEY";
 
// Device Private Key                                             
static const char AWS_CERT_PRIVATE[] PROGMEM = R"KEY(
-----BEGIN RSA PRIVATE KEY-----

-----END RSA PRIVATE KEY-----
 
)KEY";