/*
   PROJECT OF SPRAY AGAINST COVID19
   Version: 4.0
   Commit: Add send to internet.
   Client: Pixels Escola de Design e Tecnologia
   Start: 6/17/2020
   Finish:
   Update:
*/

#include "thingProperties.h"

#define SensorA_  1
#define SensorB_ -1

#define DETECTADO 0

#define tempoEspera 1000

#define MAXIMO_PESSOAS_ISOLAMENTO_SOCIAL 100,00

//Definition of the Arduino pins
const byte pinInputSensor = 2; //Digital pin 2 of the Arduino connected to sense 1 (Thread blue)
const byte pinMiddleSensor = 3; //Digital pin 3 of the Arduino connected to sense 2 (Thread yellow)
const byte pinOutputSensor = 4; //Digital pin 4 of the Arduino connected to sense 3 (Thread orange)
const byte pinRelayPump = 5; //Digital pin 5 of the Arduino connected to pin Relay of Pump Water
const byte pinBuzzer = 0; //Digital pin 6 of the Arduino connected to buzzer (Thread green)
const byte pinGreenLed = 8; // Digital pin 8 of the Arduino connected to Led Green
const byte pinYellowLed = 9; // Digital pin 9 of the Arduino connected to Led Yellow
const byte pinRedLed = 10; // Digital pin 10 of the Arduino connected to Led Red
const byte pinMedLevelSensor = 11; // Digital pin 12 of the Arduino connected to Sensor of Medium Level (Thread white)
const byte pinLowLevelSensor = 12; // Digital pin 11 of the Arduino connected to Sensor of Low Level (Thread orange)
const byte pinSensorPeopleEntering = 7; // Digital pin 14 of the Arduino connected to Infrared Sensor of people entering
const byte pinSensorPeopleOut = 6; // Digital pin 15 of the Arduino connected to Infrared Sensor of people comming out.

//Definition of variables of the sensors
bool inputSensor, middleSensor, outputSensor; //Variables that keep the state of the of presence sensors
bool lowLevelSensor, medLevelSensor; //Variables that keep the state of the of water level sensors
bool sensorPeopleEntering, sensorPeopleOut; // Variables that keep the state of the Infrared Sensor that counts people.
int estadoSensores = 0;
int estadoSensoresAnt = 0;
unsigned long controleTempo;

void setup() {
  Serial.begin(9600); //Starting serial communication
  //Sets the Sensors pins
  pinMode(pinInputSensor, INPUT_PULLUP); //Define the pin 2 as input
  pinMode(pinMiddleSensor, INPUT_PULLUP); //Define the pin 3 as input
  pinMode(pinOutputSensor, INPUT_PULLUP); //Define the pin 4 as input
  pinMode(pinLowLevelSensor, INPUT_PULLUP); //Define the pin 11 as input
  pinMode(pinMedLevelSensor, INPUT_PULLUP); //Define the pin 12 as input
  pinMode(pinSensorPeopleEntering, INPUT_PULLUP); //Define the pin 14 as input
  pinMode(pinSensorPeopleOut, INPUT_PULLUP); //Define the pin 15 as input

  //Sets the Actuators pins
  pinMode(pinRelayPump, OUTPUT); //Define the pin 5 as output
  pinMode(pinBuzzer, OUTPUT); //Define the pin 6 as output
  pinMode(pinGreenLed, OUTPUT); //Define the pin 8 as output
  pinMode(pinYellowLed, OUTPUT); //Define the pin 9 as output
  pinMode(pinRedLed, OUTPUT); //Define the pin 10 as output

  // Defined in thingProperties.h
  initProperties();

  // Connect to Arduino IoT Cloud
  ArduinoCloud.begin(ArduinoIoTPreferredConnection);

}//End of the setup

void loop() {

  ArduinoCloud.update();

  //Checks water level sensors
  lowLevelSensor = digitalRead(pinLowLevelSensor);
  medLevelSensor = digitalRead(pinMedLevelSensor);

  //Checks infrared sensor if passed person
  sensorPeopleEntering = digitalRead(pinSensorPeopleEntering);
  sensorPeopleOut = digitalRead(pinSensorPeopleOut);

  taxaMaximaOcupacao = float(ocupacao) / MAXIMO_PESSOAS_ISOLAMENTO_SOCIAL;
  Serial.println(ocupacao);
  Serial.println(taxaMaximaOcupacao);

  if (sensorPeopleEntering == DETECTADO && sensorPeopleOut == DETECTADO) {
    controleTempo = millis();

  } else if (sensorPeopleEntering == DETECTADO || sensorPeopleOut == DETECTADO) {

    if (sensorPeopleEntering == DETECTADO) {

      switch (estadoSensoresAnt) {
        case SensorB_ : {
            estadoSensores = 0;
            ocupacao--;
            controleTempo = millis();
            break;
          }
        case 0 : {
            estadoSensores = SensorA_;
            controleTempo = millis();
            break;
          }
        case SensorA_ : {
            estadoSensores = SensorA_;
            controleTempo = millis();
            break;
          }
      }


    }

    if (sensorPeopleOut == DETECTADO) {

      switch (estadoSensoresAnt) {
        case SensorA_ : {
            estadoSensores = 0;
            ocupacao++;
            controleTempo = millis();
            break;
          }
        case 0 : {
            estadoSensores = SensorB_;
            controleTempo = millis();
            break;
          }
        case SensorB_ : {
            estadoSensores = SensorB_;
            controleTempo = millis();
            break;
          }
      }

    }

  } else {
    if ( millis() - controleTempo > tempoEspera ) {
      estadoSensores = 0;
      estadoSensoresAnt = estadoSensores;
    }
  }

  estadoSensoresAnt = estadoSensores;

  //Logic of water level sensors: 0 --> With water | 1 --> No water

  // If both sensors are active (With water)
  if ((lowLevelSensor == 0) and (medLevelSensor == 0)) {
    digitalWrite(pinGreenLed, HIGH);
    digitalWrite(pinYellowLed, LOW);
    digitalWrite(pinRedLed, LOW);
    enabledTunnel();// Tunnel operating function
  }

  // If the medium level sensor not activated (Low water)
  if ((lowLevelSensor == 0) and (medLevelSensor == 1)) {
    digitalWrite(pinGreenLed, LOW);
    digitalWrite(pinYellowLed, HIGH);
    digitalWrite(pinRedLed, LOW);
    enabledTunnel();// Tunnel operating function
  }

  // If the low level sensor not activated (no water) but medium level sensor are active
  if ((lowLevelSensor == 1) and (medLevelSensor == 0)) {
    //Error in sensors
    digitalWrite(pinGreenLed, HIGH);
    digitalWrite(pinYellowLed, HIGH);
    digitalWrite(pinRedLed, HIGH);
    Serial.println("Level sensor in ERROR!");
    disableTunnel();
  }

  // If both sensors not activated (empty tank)
  if ((lowLevelSensor == 1) and (medLevelSensor == 1)) {
    digitalWrite(pinGreenLed, LOW);
    digitalWrite(pinYellowLed, LOW);
    digitalWrite(pinRedLed, HIGH);
    disableTunnel();// Tunnel no operating function
  }
  Serial.println("======================================================");

} //End of the loop

void enabledTunnel() {
  Serial.println("Tunnel enabled.");

  statusTunel = true;

  //Reading the status of the sensors
  inputSensor = digitalRead(pinInputSensor);
  middleSensor = digitalRead(pinMiddleSensor);
  outputSensor = digitalRead(pinOutputSensor);

  //Prints sensor readings on the serial monitor
  Serial.println("Input Sensor: " + String(inputSensor) + " | Middle Sensor: " + String(middleSensor) + " | Output Sensor: " + String(outputSensor));

  //If any of the 3 motion sensors is trigged
  if ((inputSensor == LOW) || (middleSensor == LOW) || (outputSensor == LOW)) {
    Serial.println("Detected person presence in the tunel.");
    digitalWrite(pinRelayPump, HIGH); //Start the pump
    noTone(pinBuzzer);
    tone(pinBuzzer, 440, 200);
    //delay(300);
  }

  else { //If you don't
    Serial.println("No movement in the tunel.");
    digitalWrite(pinRelayPump, LOW); //Turn off the pump
    noTone(pinBuzzer); //Disable the buzzer
    //delay(300);
  }

}//end enabledTunnel

void disableTunnel() {
  //Serial.println("Tunnel disabled.");

  statusTunel = false;

  digitalWrite(pinRelayPump, LOW); //Turn off the pump
  noTone(pinBuzzer); //Disable the buzzer
  //delay(500);
  tone(pinBuzzer, 3000, 200);
  //delay(500);
}

void onOcupacaoChange() {
  
}

void onTaxaMaximaOcupacaoChange() {

}

void onStatusTunelChange() {

}
