#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <DHT.h>

/* Definições do sensor de temperatura e umidade */
#define DHTPIN   13      /* GPIO que será ligado o pino 2 do DHT22 */

/* Definições da escolha do modelo de sensor */
#define DHTTYPE DHT11   // DHT 11
//#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321
//#define DHTTYPE DHT21   // DHT 21 (AM2301)

/* Credenciais wi-fi */
#define ssid_wifi "NET_VIRTUA_501_2.4GHz"      /* Coloque aqui o nome da rede wiifi que o ESP32 deve se conectar */
#define password_wifi "25606350"  /* Coloque aqui a senha da rede wiifi que o ESP32 deve se conectar */

/* Definições do Telegram */
#define TEMPO_ENTRE_CHECAGEM_DE_MENSAGENS   250 //ms

/* Token de acesso Telegram */
#define token_acesso_telegram "1497799192:AAEJw2-WH87IjKMQjPoApNndAr4rGr-bSJ0"  /* Coloque aqui o seu token de acesso Telegram (fornecido pelo BotFather) */

/* Definições das mensagens possíveis de serem recebidas */
#define CMD_CLIMA               "CLIMA"

/* Variáveis e objetos globais */
WiFiClientSecure client;
UniversalTelegramBot bot(token_acesso_telegram, client);
DHT dht(DHTPIN, DHTTYPE);
unsigned long timestamp_checagem_msg_telegram = 0;
int num_mensagens_recebidas_telegram = 0;
String resposta_msg_recebida;

/* Prototypes */
void init_wifi(void);
void conecta_wifi(void);
void verifica_conexao_wifi(void);
unsigned long diferenca_tempo(unsigned long timestamp_referencia);
String trata_mensagem_recebida(String msg_recebida);

/*
    Implementações
*/

/* Função: inicializa wi-fi
   Parametros: nenhum
   Retorno: nenhum
*/
void init_wifi(void)
{
  Serial.println("------WI-FI -----");
  Serial.print("Conectando-se a rede: ");
  Serial.println(ssid_wifi);
  Serial.println("Aguarde...");
  conecta_wifi();
}

/* Função: conecta-se a rede wi-fi
   Parametros: nenhum
   Retorno: nenhum
*/
void conecta_wifi(void)
{
  /* Se ja estiver conectado, nada é feito. */
  if (WiFi.status() == WL_CONNECTED)
    return;

  /* refaz a conexão */
  WiFi.begin(ssid_wifi, password_wifi);

  while (WiFi.status() != WL_CONNECTED)
  {
    vTaskDelay( 100 / portTICK_PERIOD_MS );
    Serial.print(".");
  }

  Serial.println();
  Serial.print("Conectado com sucesso a rede wi-fi ");
  Serial.println(ssid_wifi);
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
}

/* Função: verifica se a conexao wi-fi está ativa
           (e, em caso negativo, refaz a conexao)
   Parametros: nenhum
   Retorno: nenhum
*/
void verifica_conexao_wifi(void)
{
  conecta_wifi();
}

/* Função: calcula a diferença de tempo entre o timestamp
           de referência e o timestamp atual
   Parametros: timestamp de referência
   Retorno: diferença de tempo
*/
unsigned long diferenca_tempo(unsigned long timestamp_referencia)
{
  return (millis() - timestamp_referencia);
}

/* Função: trata mensagens recebidas via Telegram
   Parametros: mensagem recebida
   Retorno: resposta da mensagem recebida
*/
String trata_mensagem_recebida(String msg_recebida)
{
  String resposta = "";
  bool comando_valido = false;
  float temperatura_lida = 0.0;
  float umidade_lida = 0.0;

  /* Faz tratamento da mensagem recebida */
  if (msg_recebida.equals(CMD_CLIMA))
  {
    /* Responde com temperatura ambiente e umidade relativa do ar
       obtidas do sensor DHT22 */
    temperatura_lida =  dht.readTemperature();
    umidade_lida = dht.readHumidity();

    if ( isnan(temperatura_lida) || isnan(umidade_lida) ) {
      resposta = "Erro ao ler sensor de temperatura.";
    }
    else {
      resposta = "Informacoes do clima: temperatura = " +
                 String(temperatura_lida) +
                 "C e umidade = " +
                 String(umidade_lida) + "%";
    }
    comando_valido = true;
  }

  if (comando_valido == false)
    resposta = "Comando invalido: " + msg_recebida;

  return resposta;
}

void setup()
{
  Serial.begin(115200);

  /* Inicializa DHT22 */
  dht.begin();

  /* Inicializa a conexão wi-fi */
  init_wifi();

  /* Inicializa timestamp de checagem de mensagens recebidas via Telegram */
  timestamp_checagem_msg_telegram = millis();
}

void loop()
{
  int i;

  /* Garante que o wi-fi está conectado */
  verifica_conexao_wifi();

  /* Verifica se é hora de checar por mensagens enviadas ao bot Telegram */
  if ( diferenca_tempo(timestamp_checagem_msg_telegram) >= TEMPO_ENTRE_CHECAGEM_DE_MENSAGENS)
  {
    /* Verifica se há mensagens a serem recebidas */
    num_mensagens_recebidas_telegram = bot.getUpdates(bot.last_message_received + 1);

    if (num_mensagens_recebidas_telegram > 0)
    {
      Serial.print("[BOT] Mensagens recebidas: ");
      Serial.println(num_mensagens_recebidas_telegram);
    }

    /* Recebe mensagem a mensagem, faz tratamento e envia resposta */
    while (num_mensagens_recebidas_telegram)
    {
      for (i = 0; i < num_mensagens_recebidas_telegram; i++)
      {
        resposta_msg_recebida = "";
        resposta_msg_recebida = trata_mensagem_recebida(bot.messages[i].text);
        bot.sendMessage(bot.messages[i].chat_id, resposta_msg_recebida, "");

        Serial.println(bot.messages[i].chat_id);
      }

      num_mensagens_recebidas_telegram = bot.getUpdates(bot.last_message_received + 1);
    }

    /* Reinicializa timestamp de checagem de mensagens recebidas via Telegram */
    timestamp_checagem_msg_telegram = millis();
  }
}
