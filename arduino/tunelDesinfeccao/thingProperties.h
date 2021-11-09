#include <ArduinoIoTCloud.h>
#include <Arduino_ConnectionHandler.h>
#include "arduino_secrets.h"

const char THING_ID[] = "a73f86b0-9b74-4e9f-9deb-3236b166ee8c";

const char SSID[]     = SECRET_SSID;    // Network SSID (name)
const char PASS[]     = SECRET_PASS;    // Network password (use for WPA, or use as key for WEP)

void onOcupacaoChange();
void onTaxaMaximaOcupacaoChange();
void onStatusTunelChange();

int ocupacao;
float taxaMaximaOcupacao;
bool statusTunel;

void initProperties() {

  ArduinoCloud.setThingId(THING_ID);
  ArduinoCloud.addProperty(ocupacao, READWRITE, ON_CHANGE, onOcupacaoChange);
  ArduinoCloud.addProperty(taxaMaximaOcupacao, READWRITE, ON_CHANGE, onTaxaMaximaOcupacaoChange);
  ArduinoCloud.addProperty(statusTunel, READWRITE, ON_CHANGE, onStatusTunelChange);

}

WiFiConnectionHandler ArduinoIoTPreferredConnection(SSID, PASS);
