#include <SPI.h>
#include <Ethernet.h>
#include <PubSubClient.h>

/* defines de id mqtt e tópicos para publicação e subscribe */
#define TOPIC_PUBLISH "MQTTMedidorAguaEnergiaIOTPublisher"
#define TOPIC_SUBSCRIBE "MQTTMedidorAguaEnergiaIOTSubscriber"
#define ID_MQTT "MQTTMedidorAguaEnergiaIOT_ID"
#define TOKEN_MQTT "up0AtIRqbEneUDNkkxCzmt9qo5KOGIA0XoqgX5vYzI0hcBskWUK6bHnpCV16trWe"

//Informacoes de endereco IP, gateway, mascara de rede
byte mac[] = { 0xA4, 0x28, 0x72, 0xCA, 0x55, 0x2F };
byte ip[] = { 192, 168, 0, 110 };
byte gateway[] = { 192, 168, 0, 1 };
byte subnet[] = { 255, 255, 255, 0 };

const char* BROKER_MQTT = "mqtt.flespi.io"; //URL do broker MQTT que se deseja utilizar
int BROKER_PORT = 1883; // Porta do Broker MQTT

/* Variáveis e objetos globais*/

EthernetClient client;

PubSubClient MQTT(client); // Instancia o Cliente MQTT passando o objeto gsmClient

/* prototypes */
void setupEthernet(void);
void setupMQTT(void);
void reconectarMQTT(void);
void verificarEthernetMQTTConnection(void);
void enviarInformacaoMQTT(char *);
void receberInformacaoMQTT(char* , byte* , unsigned int );

/*
   Implementações
*/

/* Função: inicializa e conecta-se na rede WI-FI desejada
   Parâmetros: nenhum
  Retorno: nenhum
*/
void setupEthernet(void)
{
  // Iniciar conexão Ethernet:
  Serial.println("Inicializar Ethernet com DHCP:");
  if (Ethernet.begin(mac) == 0) {
    Serial.println("Falha ao configurar Ethernet usando DHCP");
    // Verificar se a shield Ethernet está presente
    if (Ethernet.hardwareStatus() == EthernetNoHardware) {
      Serial.println("Ethernet shield não encontrada.  Desculpe, impossível rodar sem o hardware adequado. :(");
      while (true) {
        delay(1);
      }
    }
    if (Ethernet.linkStatus() == LinkOFF) {
      Serial.println("O cabo de rede parece estar desconectado.");
    }
    // Tentar configurar manualmente o endereço IP
    Ethernet.begin(mac, ip, gateway, subnet);
    Serial.print("Meu endereço IP: ");
    Serial.println(Ethernet.localIP());
  } else {
    Serial.print("  IP obtido via DHCP ");
    Serial.println(Ethernet.localIP());
  }
  // Um segundo para inicializar a Ethernet Shield
  delay(1000);
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
void reconectarMQTT(void)
{
  while (!MQTT.connected())
  {
    Serial.print("Tentando conectar ao Broker MQTT: ");
    Serial.println(BROKER_MQTT);
    if (MQTT.connect(ID_MQTT, TOKEN_MQTT, NULL)) {
      Serial.println("Conexão ao Broker MQTT realizada com sucesso.");
      MQTT.setCallback(receberInformacaoMQTT);
      MQTT.subscribe(TOPIC_SUBSCRIBE);
    }
    else
    {
      Serial.println("Conexão ao Broker MQTT falhou.");
      Serial.println("Tentando conectar ao broker MQTT em 2s.");
      delay(5000);
    }
  }
}

/* Função: verifica o estado da conexão ao broker MQTT.
           Em caso de desconexão, a conexão é refeita.
   Parâmetros: nenhum
   Retorno: nenhum
*/
void verificarEthernetMQTTConnection(void)
{
  /* se não há conexão com o Broker, a conexão é refeita  */
  if (!MQTT.connected()) {
    reconectarMQTT();
  }
}

void enviarInformacaoMQTT(char* message)
{

  verificarEthernetMQTTConnection();

  if (MQTT.connected()) {
    Serial.print("Mensagem enviada: ");
    Serial.println(message);
    MQTT.publish(TOPIC_PUBLISH, message);
  }
}

void receberInformacaoMQTT(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message recebida: ");
  char message[20];
  strncpy(message, (char*)payload, length);
  Serial.println(message);
}

void setup() {
  /* configura comunicação serial (para enviar mensgens com as medições)
    e inicializa comunicação com o sensor.
  */
  Serial.begin(9600);

  /* inicializações do GSM e MQTT */
  setupEthernet();
  setupMQTT();
}

/*
  Programa principal
*/
void loop() {

  verificarEthernetMQTTConnection();

  char message[100];
  sprintf(message, "Sensor funcionando. 0");
  enviarInformacaoMQTT(message);

  /* Faz o keep-alive do MQTT */
  MQTT.loop();

  /* espera cinco segundos até a próxima leitura  */
  delay(5000);
}
