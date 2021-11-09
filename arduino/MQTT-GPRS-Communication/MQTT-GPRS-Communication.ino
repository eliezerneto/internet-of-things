#define TINY_GSM_MODEM_SIM800 //Tipo de modem que estamos usando
#include <SoftwareSerial.h>
#include <TinyGsmClient.h>
#include <PubSubClient.h>

/* defines de id mqtt e tópicos para publicação e subscribe */
#define TOPIC_PUBLISH "MQTTGeradorIOT"
#define ID_MQTT "MQTTGeradorIOT_ID"
#define TOKEN_MQTT "eaMweITdlSjfLJWCTfNcAwWiwti62qw7q8rsGauQTDTsaFUTKvT9RDfeWn8NEuKS"

#define PINTXMODEM 3
#define PINRXMODEM 2

#define APN "claro.com.br"
#define USERAPN "claro"
#define PASSAPN "claro"

const char* BROKER_MQTT = "mqtt.flespi.io"; //URL do broker MQTT que se deseja utilizar
int BROKER_PORT = 1883; // Porta do Broker MQTT

/* Variáveis e objetos globais*/

//Canal serial que vamos usar para comunicarmos com o modem. Utilize sempre 1
SoftwareSerial SerialGSM(PINTXMODEM, PINRXMODEM);
TinyGsm modemGSM(SerialGSM);
TinyGsmClient gsmClient(modemGSM);

PubSubClient MQTT(gsmClient); // Instancia o Cliente MQTT passando o objeto gsmClient


/* prototypes */
void setupGSM(void);
void setupMQTT(void);
void reconnectGPRS(void);
void reconnectMQTT(void);
void verify_GPRS_MQTT_connection(void);
void sendMQTTInformation(char *);

/*
   Implementações
*/

/* Função: inicializa e conecta-se na rede WI-FI desejada
   Parâmetros: nenhum
  Retorno: nenhum
*/
void setupGSM(void)
{
  delay(10);
  Serial.println("Setup GSM.");
  //Inicializamos a serial onde está o modem
  SerialGSM.begin(9600);
  if (!modemGSM.restart())
  {
    Serial.println("Initializing GSM Modem failed.");
  }
  else {
    delay(3000);

    //Mostra informação sobre o modem
    Serial.println("Modem information: " + modemGSM.getModemInfo());
  }
}

/* Função: inicializa parâmetros de conexão MQTT(endereço do broker e porta)
   Parâmetros: nenhum
   Retorno: nenhum
*/
void setupMQTT(void)
{
  //informa qual broker e porta deve ser conectado
  Serial.println("Setup MQTT.");
  MQTT.setServer(BROKER_MQTT, BROKER_PORT);
}

/* Função: reconecta-se ao broker MQTT (caso ainda não esteja conectado ou em caso de a conexão cair)
           em caso de sucesso na conexão ou reconexão, o subscribe dos tópicos é refeito.
   Parâmetros: nenhum
   Retorno: nenhum
*/
void reconnectMQTT(void)
{
  while (!MQTT.connected())
  {
    Serial.print("Trying to connect to Broker MQTT: ");
    Serial.println(BROKER_MQTT);
    if (MQTT.connect(ID_MQTT, TOKEN_MQTT, NULL))
      Serial.println("Broker MQTT connection success.");
    else
    {
      Serial.println("Broker MQTT connection failed.");
      Serial.println("Trying broker MQTT connection in 2s.");
      delay(5000);
    }
  }
}

/* Função: reconecta-se ao WiFi
   Parâmetros: nenhum
   Retorno: nenhum
*/
void reconnectGPRS(void)
{

  while (!modemGSM.isGprsConnected()) {
    Serial.println("Trying to connect GPRS network.");

    //Inicializa o modem
    if (!modemGSM.restart())
    {
      Serial.println("Restarting GSM Modem failed.");
    }

    //Espera pela rede
    if (!modemGSM.waitForNetwork())
    {
      Serial.println("Failed to connect to network.");
    }

    if (modemGSM.isNetworkConnected()) {

      Serial.println("Network connected.");

      //Conecta à rede gprs (APN, usuário, senha)
      if (!modemGSM.gprsConnect(APN, USERAPN, PASSAPN)) {
        Serial.println("GPRS Connection Failed.");
        delay(5000);
      }

      if (modemGSM.isGprsConnected()) {
        Serial.println("GPRS Connection Success.");
      }
    }
  }
}

/* Função: verifica o estado das conexões WiFI e ao broker MQTT.
           Em caso de desconexão (qualquer uma das duas), a conexão
          é refeita.
   Parâmetros: nenhum
   Retorno: nenhum
*/
void verify_GPRS_MQTT_connection(void)
{
  /* se não há conexão com a rede GPRS, a conexão é refeita */
  if (!modemGSM.isGprsConnected()) {
    reconnectGPRS();
  }
  /* se não há conexão com o Broker, a conexão é refeita  */
  if (!MQTT.connected()) {
    reconnectMQTT();
  }
}

void sendMQTTInformation(char* message)
{

  verify_GPRS_MQTT_connection();

  if (MQTT.connected()) {
    Serial.print("Sended message: ");
    Serial.println(message);
    MQTT.publish(TOPIC_PUBLISH, message);
  }
}

void setup() {
  /* configura comunicação serial (para enviar mensgens com as medições)
    e inicializa comunicação com o sensor.
  */
  Serial.begin(9600);

  /* inicializações do GSM e MQTT */
  setupGSM();
  setupMQTT();
}

/*
  Programa principal
*/
void loop() {

  verify_GPRS_MQTT_connection();

  char message[100];
  sprintf(message, "Sensor funcionando. 0");
  sendMQTTInformation(message);

  /* Faz o keep-alive do MQTT */
  MQTT.loop();

  /* espera cinco segundos até a próxima leitura  */
  delay(5000);
}
