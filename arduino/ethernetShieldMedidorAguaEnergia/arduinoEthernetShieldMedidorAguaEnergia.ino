/* Programação para o medidor de vazão */

#include <DS1307.h>
#include <SPI.h>
#include <Ethernet.h>
#include <PubSubClient.h>
#include <Streaming.h>
#include <PString.h>
#include "EmonLib.h"

/*Pino do Arduino onde será conectado o pino de dados do sensor de fluxo ou vazão*/
#define pinMedidorVazao 19
#define pinMedidorEnergia A8

/* Define id mqtt e tópicos para publicação e subscribe */
#define TOPIC_PUBLISH "MQTTMedidorAguaEnergiaIOTPublisher"
#define TOPIC_SUBSCRIBE "MQTTMedidorAguaEnergiaIOTSubscriber"
#define ID_MQTT "MQTTMedidorAguaEnergiaIOT_ID"
#define TOKEN_MQTT "IHI1Od99TOacf92SR5g4E7PganKBipAOW8Gz9DySgwSlwK3bF8385Ie3TOs21SGO"

/* Variáveis globais */

/* Variáveis medidor de vazão */
float vazao; /* Variável para armazenar o valor em L/min */
int contaPulso; /* Variável para a quantidade de pulsos */
float valorTotalAguaConsumida = 0.0; /* Variável para Quantidade de água */
float valorFinanceiroTotalAguaConsumida = 0.0; /* Variável para o valor financeiro total gasto com água */
float litros = 0.0; /* Variável para Conversão */
float valorTarifaAgua = 2.0;

/* Variáveis medidor de energia */

int tensao = 220;
double valorTarifaEnergia = 0.79338; /* Valor da tarifa em kilowatt hora */
double valorTotalEnergiaConsumido = 0; /* Valor total consumido de energia */
double valorFinanceiroTotalEnergiaConsumido = 0; /* Valor financeiro total consumido de energia */

//Informacoes de endereco IP, gateway, mascara de rede
byte mac[] = { 0xA4, 0x28, 0x72, 0xCA, 0x55, 0x2F };
byte ip[] = { 192, 168, 0, 110 };
byte gateway[] = { 192, 168, 0, 1 };
byte subnet[] = { 255, 255, 255, 0 };

const char* BROKER_MQTT = "mqtt.flespi.io"; //URL do broker MQTT que se deseja utilizar
int BROKER_PORT = 1883; // Porta do Broker MQTT

char mensagem[100];
PString buffer(mensagem, sizeof(mensagem));

unsigned long periodoEnvioMensagem = 600000;
unsigned long tempoAnterior = 0;

DS1307 rtc(A4, A5);

EthernetClient client;

PubSubClient MQTT(client); // Instancia o Cliente MQTT passando o objeto gsmClient

EnergyMonitor emon1;

/* Protótipos */
void setupEthernet(void);
void setupMQTT(void);
void reconectarMQTT(void);
void verificarEthernetMQTTConnection(void);
void enviarInformacaoMQTT(char *);
void receberInformacaoMQTT(char* , byte* , unsigned int );
void inicializarRelogio(void);
void calcularMedicaoAgua(void);
void incpulso ();

void inicializarRelogio() {

  /* Inicializar relógio com data e hora atuais */
  /* Comentar após a primeira sincronização */
  /*rtc.setDOW(SUNDAY);*/      /* Define o dia da semana */
  /*rtc.setTime(20, 04, 0);*/    /* Define o horario */
  /*rtc.setDate(22, 11, 2020);*/   /* Define o dia, mes e ano */

}

void calcularMedicaoAgua() {

  vazao = contaPulso / 5.5; /* Converte para L/min */

  litros = vazao / 60; /* Estima a quantidade de litros medidos no período de 1 (um) segundo */

  valorTotalAguaConsumida = valorTotalAguaConsumida + litros;

  valorFinanceiroTotalAguaConsumida = valorTotalAguaConsumida * valorTarifaAgua;

  /*Serial.println();
    Serial.println("Medição de água:");

    Serial.println("Vazão: " + String(vazao) + " L/min ");

    Serial.print("Litros: ");
    Serial.println(valorTotalAguaConsumida);*/

}

void calcularMedicaoEnergia() {

  double valorCorrente = emon1.calcIrms(1480);
  double potencia = 0;
  double valorConsumidoUmSegundo = 0;
  double valorFinanceiroConsumidoUmSegundo = 0;

  valorConsumidoUmSegundo = 0;

  if (valorCorrente <= 0.02) {
    valorCorrente = 0;
  }

  potencia = valorCorrente * tensao; /*Calcular a potencia utilizada naquele instante*/

  valorConsumidoUmSegundo = potencia / 1000.0; /* COnverter a potência para kilowatt */

  valorFinanceiroConsumidoUmSegundo = ((valorConsumidoUmSegundo) * (valorTarifaEnergia / 3600.0)); /* Calculando o valor consumido em 1 segundo, de acordo com a tarifa configurada */

  valorFinanceiroTotalEnergiaConsumido = valorFinanceiroTotalEnergiaConsumido + valorFinanceiroConsumidoUmSegundo; /* Acumulando o valor total consumido de energia */
  valorTotalEnergiaConsumido = valorTotalEnergiaConsumido + valorConsumidoUmSegundo; /* Acumulando o valor total financeiro consumido */

  /* Mostra o valor da corrente no serial monitor */

  /*Serial.println();
    Serial.println("Medição de energia:");

    Serial.print("Corrente: ");
    Serial.println(valorCorrente);

    Serial.print("Valor Consumido em 1 segundo: ");
    Serial.println(valorConsumidoUmSegundo);

    Serial.print("Valor Financeiro Consumido em 1 segundo: ");
    Serial.println(valorFinanceiroConsumidoUmSegundo);

    Serial.print("Valor Total Consumido: ");
    Serial.println(valorTotalEnergiaConsumido);

    Serial.print("Valor Total Financeiro Consumido: ");
    Serial.println(valorFinanceiroTotalEnergiaConsumido);*/
}

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
      delay(2000);
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

void enviarMensagem(float valorTotalAguaConsumidaMensagem, float valorFinanceiroTotalAguaConsumidaMensagem,
                    float valorTotalEnergiaConsumidoMensagem, float valorFinanceiroTotalEnergiaConsumidoMensagem) {
  buffer.begin();

  buffer << "[" << rtc.getDateStr(FORMAT_SHORT) << "," << rtc.getTimeStr() << "," << valorTotalAguaConsumidaMensagem << "," << valorFinanceiroTotalAguaConsumidaMensagem <<
         "," << valorTotalEnergiaConsumidoMensagem << "," << valorFinanceiroTotalEnergiaConsumidoMensagem <<
         "]" << endl;

  enviarInformacaoMQTT(buffer);
}

void incpulso ()  {
  contaPulso++; /* Incrementa a variável de pulsos */
}

void setup()  {
  Serial.begin(9600);

  pinMode(pinMedidorVazao, INPUT);

  /* Pino, calibracao - Cur Const= Ratio/BurdenR. 2000/33 = 60 */
  emon1.current(pinMedidorEnergia, 60);

  /* Configura a interrupção para captura dos pulsos do sensor */
  attachInterrupt(digitalPinToInterrupt(pinMedidorVazao), incpulso, RISING); /* Configura o pino de interrupção */

  inicializarRelogio();

  setupEthernet();
  setupMQTT();
}

void loop ()  {
  contaPulso = 0;/* Zera a variável */

  attachInterrupt(digitalPinToInterrupt(pinMedidorVazao), incpulso, RISING);
  delay (1000); /* Aguarda 1 segundo */
  detachInterrupt(digitalPinToInterrupt(pinMedidorVazao));

  /*Serial.print("Data : ");
    Serial.print(rtc.getDateStr(FORMAT_SHORT));
    Serial.print(" ");
    Serial.print("Hora : ");
    Serial.println(rtc.getTimeStr());*/

  calcularMedicaoAgua();

  calcularMedicaoEnergia();

  /* Envia mensagem ao Broker MQTT a cada 10 minutos */
  if (millis() - tempoAnterior >= periodoEnvioMensagem) {

    verificarEthernetMQTTConnection();

    enviarMensagem(valorTotalAguaConsumida, valorFinanceiroTotalAguaConsumida,
                   valorTotalEnergiaConsumido, valorFinanceiroTotalEnergiaConsumido);

    tempoAnterior += periodoEnvioMensagem;
  }

  /* Faz o keep-alive do MQTT */
  MQTT.loop();
  attachInterrupt(digitalPinToInterrupt(pinMedidorVazao), incpulso, RISING);
}
