/*
  Project of Generator Supervisory

  Start: 04/17/2020
  Conclusion:
  Update:

  Developer: Mesquita, Sandro
             Tomé de Paula, Eliezer
  Requester: Luiz
*/

#define TINY_GSM_MODEM_SIM800 //Modem type used
//Libraries for GPRS/MQTT communication
#include <SoftwareSerial.h>
#include <TinyGsmClient.h>
#include <PubSubClient.h>

//Libraries for use the Card RFID
#include <SPI.h>
#include <MFRC522.h>

/* Begin communication variables*/

// MQTT Broker authentication information
#define TOPIC_PUBLISH "MQTTGeradorIOT"
#define ID_MQTT "MQTTGeradorIOT_ID"
#define TOKEN_MQTT "eaMweITdlSjfLJWCTfNcAwWiwti62qw7q8rsGauQTDTsaFUTKvT9RDfeWn8NEuKS"

// Pin modem
#define PINTXMODEM 26
#define PINRXMODEM 27

// GPRS connection
#define APN "claro.com.br"
#define USERAPN "claro"
#define PASSAPN "claro"

// MQTT Broker varibles
const char* BROKER_MQTT = "mqtt.flespi.io"; //URL MQTT broker
int BROKER_PORT = 1883; // MQTT broker port

// GPRS connection objects
SoftwareSerial SerialGSM(PINTXMODEM, PINRXMODEM);
TinyGsm modemGSM(SerialGSM);
TinyGsmClient gsmClient(modemGSM);

PubSubClient MQTT(gsmClient); // Instancia o Cliente MQTT passando o objeto gsmClient

/* End communication variables*/

//Pin Reset and SS módule MFRC522
#define SS_PIN 53
#define RST_PIN 12
MFRC522 mfrc522(SS_PIN, RST_PIN);

float inputVoltage = 0.0;
float measureVoltage = 0.0;

float valueR1 = 30000.0;
float valueR2 = 7500.0;

int sensorReading = 0;

char st[20];
bool maintenance = 0;

//SENSORS ON THE PIN ARDUINO*************************
byte extPhase1 = 2, extPhase2 = 3, extPhase3 = 4,
     genPhase1 = 5, genPhase2 = 6, genPhase3 = 7,
     levelCool = 8, levelFuel = 9, temp = 10, door = 11;

//ALARM LEDS ON THE PIN ARDUINO***********************
byte alarmExtPhase1 = 49, alarmExtPhase2 = 48, alarmExtPhase3 = 47,
     alarmGenPhase1 = 46, alarmGenPhase2 = 45, alarmGenPhase3 = 44,
     alarmLevelCool = 43, alarmLevelFuel = 42, alarmTemp = 41, alarmDoor = 40;

//STATUS STORAGE
bool flag1 = 0, flag2 = 0, flag3 = 0, flag4 = 0, flag5 = 0, flag6 = 0, flag7 = 0, flag8 = 0, flag9 = 0, flag10 = 0;
bool statusExtPhase1, statusExtPhase2, statusExtPhase3, statusGenPhase1, statusGenPhase2,
     statusGenPhase3, statusLevelCool, statusLevelFuel, statusTemp, statusDoor;
bool alarmVoltage = 0;

//MACHINE STATUS LED
byte pinRed = 39, pinGreen = 38, pinBlue = 37;

int voltageSense = A2, msn = 0;
unsigned report, tempo;

/* Begin communication functions prototypes */
void setupGSM(void);
void setupMQTT(void);
void reconnectGPRS(void);
void reconnectMQTT(void);
void verify_GPRS_MQTT_connection(void);
void sendMQTTInformation(char *);
/* End communication functions prototypes */

void setup() {
  Serial.begin(9600); // Serial starts
  SPI.begin();      // SPI bus starts
  mfrc522.PCD_Init();   // MFRC522 starts
  Serial.println("Bring your card closer to the read...");
  Serial.println();
  Serial.print("Booting the system");

  pinMode(A2, INPUT);

  for (byte x = 0; x <= 10; x++) {
    Serial.print(".");
    delay(500);
  }
  Serial.println(" ");
  for (byte x = 2; x <= 11; x++) {
    pinMode(x, INPUT_PULLUP); //CONFIGURE THE PIN ARDUINO AS INPUT
    delay(1);
  }

  for (byte x = 36; x <= 49; x++) {
    pinMode(x, OUTPUT); //CONFIGURE THE PIN ARDUNINO AS OUTPUT
    digitalWrite(x, LOW); //TURN ALL OFF
    delay(1);
  }

  //Establish GSM/GPRS and MQTT connection
  setupGSM();
  setupMQTT();

  mensageminicial();
  tempo = millis();
}

void loop() {

  //Verify GPRS and MQTT connection
  verify_GPRS_MQTT_connection();

  // Look for new cards
  if ( ! mfrc522.PICC_IsNewCardPresent())
  {
    report = millis() - tempo;
    sensorReading = analogRead(A2);
    inputVoltage = (sensorReading * 4.8) / 1024.0;
    measureVoltage = inputVoltage / (valueR2 / (valueR1 + valueR2));

    if ((measureVoltage <= 11.00) && (report >= 2500) && (alarmVoltage == 0)) {
      Serial.println("ATTENTION: Undervoltage in the battery system.");
      Serial.print("Measure DC voltage: ");
      Serial.print(measureVoltage, 2);
      Serial.println("V");
      alarmVoltage = 1;

      /*Send message*/
      char underVoltageMessage[100];
      sprintf(underVoltageMessage, "WARN - Undervoltage in the battery system. - %.2f V", measureVoltage);
      sendMQTTInformation(underVoltageMessage);
    }
    if (maintenance == 0) {
      //READ THE SENSORS
      statusExtPhase1 = digitalRead(extPhase1);
      statusExtPhase2 = digitalRead(extPhase2);
      statusExtPhase3 = digitalRead(extPhase3);
      statusGenPhase1 = digitalRead(genPhase1);
      statusGenPhase2 = digitalRead(genPhase2);
      statusGenPhase3 = digitalRead(genPhase3);
      statusLevelCool = digitalRead(levelCool);
      statusLevelFuel = digitalRead(levelFuel);
      statusTemp = digitalRead(temp);
      statusDoor = digitalRead(door);

      if (statusExtPhase1 == 1 && statusExtPhase2 == 1 && statusExtPhase3 == 1 && statusGenPhase1 == 1
          && statusGenPhase2 == 1 && statusGenPhase3 == 1 && statusLevelCool == 1 && statusLevelFuel == 1
          && statusTemp == 1 && statusDoor == 1) {
        if (report >= 5000) {
          Serial.println("Machine in normal operation!");
          Serial.print("Measure DC voltage: ");
          Serial.print(measureVoltage, 2);
          Serial.println("V");
          Serial.println(" ");
          digitalWrite(pinRed, LOW);
          digitalWrite(pinGreen, HIGH);
          digitalWrite(pinBlue, LOW);
          alarmVoltage = 0;
          tempo = millis();

          /*Send message*/
          char normalOperationMessage[100];
          sprintf(normalOperationMessage, "INFO - Machine in normal operation. - %.2f V", measureVoltage);
          sendMQTTInformation(normalOperationMessage);
        }
        msn++;
      }

      if (statusExtPhase1 == 0 || statusExtPhase2 == 0 || statusExtPhase3 == 0 || statusGenPhase1 == 0
          || statusGenPhase2 == 0 || statusGenPhase3 == 0 || statusLevelCool == 0 || statusLevelFuel == 0
          || statusTemp == 0 || statusDoor == 0) {
        sensorsFailed();
        digitalWrite(pinRed, HIGH);
        digitalWrite(pinGreen, LOW);
        digitalWrite(pinBlue, LOW);
        buzzer();
      }

      if (statusExtPhase1 == 1 || statusExtPhase2 == 1 || statusExtPhase3 == 1 || statusGenPhase1 == 1
          || statusGenPhase2 == 1 || statusGenPhase3 == 1 || statusLevelCool == 1 || statusLevelFuel == 1
          || statusTemp == 1 || statusDoor == 1) {
        sensorsNormalized();
      }
    }
    if ((maintenance == 1) && (report >= 5000)) {
      Serial.println("Machine under maintenance: Card 01!");
      Serial.println("Machine in normal operation!");
      Serial.print("Measure DC voltage: ");
      Serial.print(measureVoltage, 2);
      Serial.println("V");
      Serial.println(" ");
      digitalWrite(pinRed, LOW);
      digitalWrite(pinGreen, LOW);
      digitalWrite(pinBlue, HIGH);
      alarmVoltage = 0;
      tone(36, 3000);
      delay(1000);
      noTone(36);
      tempo = millis();

      /*Send message*/
      char maintenanceMessage[100];
      sprintf(maintenanceMessage, "INFO - Machine under maintenance: Card 01. - %.2f V", measureVoltage);
      sendMQTTInformation(maintenanceMessage);
    }
    return;
  }

  if ( ! mfrc522.PICC_ReadCardSerial())
  {
    return;
  }
  //Print UID in the serial
  Serial.print("UID da tag :");
  String conteudo = "";
  byte letra;
  for (byte i = 0; i < mfrc522.uid.size; i++)
  {
    Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
    Serial.print(mfrc522.uid.uidByte[i], HEX);
    conteudo.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
    conteudo.concat(String(mfrc522.uid.uidByte[i], HEX));
  }
  Serial.println();
  Serial.print("Mensagem : ");
  conteudo.toUpperCase();
  if (conteudo.substring(1) == "B0 3A 7A A2") //UID 1 - Card 01
  {
    if (maintenance == 0) {
      Serial.println("Maintenance started: Card 01!");
      Serial.println();
      maintenance = !maintenance;

      /*Send message*/
      char maintenanceStartedMessage[100];
      sprintf(maintenanceStartedMessage, "INFO - Maintenance started: Card 01.");
      sendMQTTInformation(maintenanceStartedMessage);
    }
    else {
      Serial.println("Maintenance completed: Card 01!");
      Serial.println();
      maintenance = !maintenance;

      /*Send message*/
      char maintenanceCompletedMessage[100];
      sprintf(maintenanceCompletedMessage, "INFO - Maintenance completed: Card 01.");
      sendMQTTInformation(maintenanceCompletedMessage);
    }
    delay(3000);
    mensageminicial();
  }

  //MQTT keep-alive
  MQTT.loop();
}

void sensorsFailed() {
  if ((statusExtPhase1 == 0) && (flag1 == 0)) {
    Serial.println("Phase 01 of the external network failed.");
    digitalWrite(alarmExtPhase1, HIGH);
    flag1 = 1;

    /*Send message*/
    char message[100];
    sprintf(message, "ERROR - Phase 01 of the external network failed.");
    sendMQTTInformation(message);
  }

  else if ((statusExtPhase2 == 0) && (flag2 == 0)) {
    Serial.println("Phase 02 of the external network failed.");
    digitalWrite(alarmExtPhase2, HIGH);
    flag2 = 1;

    /*Send message*/
    char message[100];
    sprintf(message, "ERROR - Phase 02 of the external network failed.");
    sendMQTTInformation(message);
  }

  else if ((statusExtPhase3 == 0) && (flag3 == 0)) {
    Serial.println("Phase 03 of the external network failed.");
    digitalWrite(alarmExtPhase3, HIGH);
    flag3 = 1;

    /*Send message*/
    char message[100];
    sprintf(message, "ERROR - Phase 03 of the external network failed.");
    sendMQTTInformation(message);
  }

  else if ((statusGenPhase1 == 0) && (flag4 == 0)) {
    Serial.println("Phase 01 of the generator network failed.");
    digitalWrite(alarmGenPhase1, HIGH);
    flag4 = 1;

    /*Send message*/
    char message[100];
    sprintf(message, "ERROR - Phase 01 of the generator network failed.");
    sendMQTTInformation(message);
  }

  else if ((statusGenPhase2 == 0) && (flag5 == 0)) {
    Serial.println("Phase 02 of the generator network failed.");
    digitalWrite(alarmGenPhase2, HIGH);
    flag5 = 1;

    /*Send message*/
    char message[100];
    sprintf(message, "ERROR - Phase 02 of the generator network failed.");
    sendMQTTInformation(message);
  }

  else if ((statusGenPhase3 == 0) && (flag6 == 0)) {
    Serial.println("Phase 03 of the generator network failed.");
    digitalWrite(alarmGenPhase3, HIGH);
    flag6 = 1;

    /*Send message*/
    char message[100];
    sprintf(message, "ERROR - Phase 03 of the generator network failed.");
    sendMQTTInformation(message);
  }

  else if ((statusLevelCool == 0) && (flag7 == 0)) {
    Serial.println("Low cooling level.");
    digitalWrite(alarmLevelCool, HIGH);
    flag7 = 1;

    /*Send message*/
    char message[100];
    sprintf(message, "WARN - Low cooling level.");
    sendMQTTInformation(message);
  }

  else if ((statusLevelFuel == 0) && (flag8 == 0)) {
    Serial.println("Low fuel level.");
    digitalWrite(alarmLevelFuel, HIGH);
    flag8 = 1;

    /*Send message*/
    char message[100];
    sprintf(message, "WARN - Low fuel level.");
    sendMQTTInformation(message);
  }

  else if ((statusTemp == 0) && (flag9 == 0)) {
    Serial.println("High temperature in the generator.");
    digitalWrite(alarmTemp, HIGH);
    flag9 = 1;

    /*Send message*/
    char message[100];
    sprintf(message, "WARN - High temperature in the generator.");
    sendMQTTInformation(message);
  }

  else if ((statusDoor == 0) && (flag10 == 0)) {
    Serial.println("Generator door open.");
    digitalWrite(alarmDoor, HIGH);
    flag10 = 1;

    /*Send message*/
    char message[100];
    sprintf(message, "WARN - Generator door open.");
    sendMQTTInformation(message);
  }

}

void sensorsNormalized() {
  if ((statusExtPhase1 == 1) && (flag1 == 1)) {
    Serial.println("Normal phase 01 of the external network.");
    digitalWrite(alarmExtPhase1, LOW);
    flag1 = 0;

    /*Send message*/
    char message[100];
    sprintf(message, "INFO - Normal phase 01 of the external network.");
    sendMQTTInformation(message);
  }

  else if ((statusExtPhase2 == 1) && (flag2 == 1)) {
    Serial.println("Normal phase 02 of the external network.");
    digitalWrite(alarmExtPhase2, LOW);
    flag2 = 0;

    /*Send message*/
    char message[100];
    sprintf(message, "INFO - Normal phase 02 of the external network.");
    sendMQTTInformation(message);
  }

  else if ((statusExtPhase3 == 1) && (flag3 == 1)) {
    Serial.println("Normal phase 03 of the external network.");
    digitalWrite(alarmExtPhase3, LOW);
    flag3 = 0;

    /*Send message*/
    char message[100];
    sprintf(message, "INFO - Normal phase 03 of the external network.");
    sendMQTTInformation(message);
  }

  else if ((statusGenPhase1 == 1) && (flag4 == 1)) {
    Serial.println("Normal phase 01 of the generator network.");
    digitalWrite(alarmGenPhase1, LOW);
    flag4 = 0;

    /*Send message*/
    char message[100];
    sprintf(message, "INFO - Normal phase 01 of the generator network.");
    sendMQTTInformation(message);
  }

  else if ((statusGenPhase2 == 1) && (flag5 == 1)) {
    Serial.println("Normal phase 02 of the generator network.");
    digitalWrite(alarmGenPhase2, LOW);
    flag5 = 0;

    /*Send message*/
    char message[100];
    sprintf(message, "INFO - Normal phase 02 of the generator network.");
    sendMQTTInformation(message);
  }

  else if ((statusGenPhase3 == 1) && (flag6 == 1)) {
    Serial.println("Normal phase 03 of the generator network.");
    digitalWrite(alarmGenPhase3, LOW);
    flag6 = 0;

    /*Send message*/
    char message[100];
    sprintf(message, "INFO - Normal phase 03 of the generator network.");
    sendMQTTInformation(message);
  }

  else if ((statusLevelCool == 1) && (flag7 == 1)) {
    Serial.println("Normal cooling level.");
    digitalWrite(alarmLevelCool, LOW);
    flag7 = 0;

    /*Send message*/
    char message[100];
    sprintf(message, "INFO - Normal cooling level.");
    sendMQTTInformation(message);
  }

  else if ((statusLevelFuel == 1) && (flag8 == 1)) {
    Serial.println("Normal fuel level.");
    digitalWrite(alarmLevelFuel, LOW);
    flag8 = 0;

    /*Send message*/
    char message[100];
    sprintf(message, "INFO - Normal fuel level.");
    sendMQTTInformation(message);
  }

  else if ((statusTemp == 1) && (flag9 == 1)) {
    Serial.println("Normal temperature in the generator.");
    digitalWrite(alarmTemp, LOW);
    flag9 = 0;

    /*Send message*/
    char message[100];
    sprintf(message, "INFO - Normal temperature in the generator.");
    sendMQTTInformation(message);
  }

  else if ((statusDoor == 1) && (flag10 == 1)) {
    Serial.println("Generator door closed.");
    digitalWrite(alarmDoor, LOW);
    flag10 = 0;

    /*Send message*/
    char message[100];
    sprintf(message, "INFO - Generator door closed.");
    sendMQTTInformation(message);
  }
}

void buzzer() {
  tone(36, 3000);
  delay(100);
  noTone(36);
  delay(100);
}

void mensageminicial()
{
  Serial.println("Bring your card!");
}

/* Prepare GSM/GPRS connection environment */
void setupGSM(void)
{
  delay(10);
  Serial.println("Setup GSM.");
  //Initialize serial
  SerialGSM.begin(9600);
  if (!modemGSM.restart())
  {
    Serial.println("Initializing GSM Modem failed.");
  }
  else {
    delay(3000);

    //Show modem information on serial
    Serial.println("Modem information: " + modemGSM.getModemInfo());
  }
}

/* Prepare MQTT connection environment */
void setupMQTT(void)
{
  //Inform MQTT server and port
  Serial.println("Setup MQTT.");
  MQTT.setServer(BROKER_MQTT, BROKER_PORT);
}

/* Establish MQTT broker connection */
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

/* Establish GSM/GPRS connection */

void reconnectGPRS(void)
{

  while (!modemGSM.isGprsConnected()) {
    Serial.println("Trying to connect GPRS network.");

    //Initialize modem
    if (!modemGSM.restart())
    {
      Serial.println("Restarting GSM Modem failed.");
    }

    //Wait network connection
    if (!modemGSM.waitForNetwork())
    {
      Serial.println("Failed to connect to network.");
    }

    if (modemGSM.isNetworkConnected()) {

      Serial.println("Network connected.");

      //Establish GPRS connection
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

/* Verify GPRS and MQTT connection */
void verify_GPRS_MQTT_connection(void)
{
  /* If GSM/GPRS connection down, establish again. */
  if (!modemGSM.isGprsConnected()) {
    reconnectGPRS();
  }
  /* If MQTT connection down, establish again.  */
  if (!MQTT.connected()) {
    reconnectMQTT();
  }
}

/* Send MQTT message to broker server */
void sendMQTTInformation(char* message)
{
  //Verify GPRS and MQTT connection
  verify_GPRS_MQTT_connection();

  if (MQTT.connected()) {
    Serial.print("Sended message: ");
    Serial.println(message);
    MQTT.publish(TOPIC_PUBLISH, message);
  }
}
