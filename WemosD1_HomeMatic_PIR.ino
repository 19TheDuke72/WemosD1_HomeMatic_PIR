#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiManager.h>
#include <FS.h>
#include <ArduinoJson.h>

#define TasterPin      D7 //Taster gegen GND, um den Konfigurationsmodus zu aktivieren
#define PIRPin         D5 //Optional: D5 gegen GND, um serielle Ausgabe zu aktivieren (115200,8,N,1)

#define IPSIZE  16
#define VARIABLESIZE  100
char ccuip[IPSIZE];
char Variable[VARIABLESIZE];

//WifiManager - don't touch
byte ConfigPortalTimeout = 180;
bool shouldSaveConfig        = false;
String configJsonFile        = "config.json";
bool wifiManagerDebugOutput = false;
char ip[IPSIZE]      = "0.0.0.0";
char netmask[IPSIZE] = "0.0.0.0";
char gw[IPSIZE]      = "0.0.0.0";
boolean startWifiManager = false;

volatile bool interruptDetected = false;

void handleInterrupt() {
  interruptDetected = true;
}

void setup() {
  pinMode(TasterPin, INPUT_PULLUP);
  pinMode(PIRPin, INPUT);
  attachInterrupt(digitalPinToInterrupt(PIRPin), handleInterrupt, RISING);
  pinMode(LED_BUILTIN, OUTPUT);

  digitalWrite(LED_BUILTIN,  HIGH);
  wifiManagerDebugOutput = true;
  Serial.begin(115200);
  printSerial("Programmstart...");

  if (digitalRead(TasterPin) == LOW) {
    Serial.println("Taster gedrückt - Config Mode wird gestartet!");
    startWifiManager = true;
    bool state = LOW;
    for (int i = 0; i < 7; i++) {
      state = !state;
      digitalWrite(LED_BUILTIN, state);
      delay(100);
    }
  }
  loadSystemConfig();

  if (doWifiConnect()) {
    printSerial("WLAN erfolgreich verbunden!");
  } else ESP.restart();
}

void loop() {
  if (interruptDetected) {
    interruptDetected = false;
    sendMotionDetectedToCCU();
  }
}

void sendMotionDetectedToCCU() {
  if (WiFi.status() == WL_CONNECTED)
  {
    HTTPClient http;
    String tempVar = String(Variable);
    tempVar.replace(" ","%20");
    String url = "http://" + String(ccuip) + ":8181/cuxd.exe?ret=dom.GetObject(%22" + tempVar + "%22).State(true)";
    printSerial("URL = " + url);
    http.begin(url);
    int httpCode = http.GET();
    printSerial("httpcode = " + String(httpCode));

    if (httpCode != 200) {
      printSerial("HTTP fail " + String(httpCode));
    }
    http.end();
  } else ESP.restart();
}

void printSerial(String text) {
  Serial.println(text);
}

bool doWifiConnect() {
  String _ssid = WiFi.SSID();
  String _psk = WiFi.psk();

  const char* ipStr = ip; byte ipBytes[4]; parseBytes(ipStr, '.', ipBytes, 4, 10);
  const char* netmaskStr = netmask; byte netmaskBytes[4]; parseBytes(netmaskStr, '.', netmaskBytes, 4, 10);
  const char* gwStr = gw; byte gwBytes[4]; parseBytes(gwStr, '.', gwBytes, 4, 10);

  WiFiManager wifiManager;
  wifiManager.setDebugOutput(wifiManagerDebugOutput);
  wifiManager.setAPCallback(configModeCallback);
  wifiManager.setSaveConfigCallback(saveConfigCallback);
  WiFiManagerParameter custom_ccuip("ccu", "IP der CCU2", ccuip, IPSIZE);

  WiFiManagerParameter custom_variablename("variable", "Name der Systemvariable", Variable, VARIABLESIZE);

  WiFiManagerParameter custom_ip("custom_ip", "IP-Adresse", "", IPSIZE);
  WiFiManagerParameter custom_netmask("custom_netmask", "Netzmaske", "", IPSIZE);
  WiFiManagerParameter custom_gw("custom_gw", "Gateway", "", IPSIZE);

  wifiManager.addParameter(&custom_ccuip);
  wifiManager.addParameter(&custom_variablename);
  WiFiManagerParameter custom_text("<br/><br>Statische IP (wenn leer, dann DHCP):");
  wifiManager.addParameter(&custom_text);
  wifiManager.addParameter(&custom_ip);
  wifiManager.addParameter(&custom_netmask);
  wifiManager.addParameter(&custom_gw);

  String Hostname = "WemosD1-" + WiFi.macAddress();

  wifiManager.setConfigPortalTimeout(ConfigPortalTimeout);

  if (startWifiManager == true) {
    digitalWrite(LED_BUILTIN, LOW);
    if (_ssid == "" || _psk == "" ) {
      wifiManager.resetSettings();
    }
    else {
      if (!wifiManager.startConfigPortal(Hostname.c_str())) {
        printSerial("failed to connect and hit timeout");
        delay(1000);
        ESP.restart();
      }
    }
  }

  wifiManager.setSTAStaticIPConfig(IPAddress(ipBytes[0], ipBytes[1], ipBytes[2], ipBytes[3]), IPAddress(gwBytes[0], gwBytes[1], gwBytes[2], gwBytes[3]), IPAddress(netmaskBytes[0], netmaskBytes[1], netmaskBytes[2], netmaskBytes[3]));

  wifiManager.autoConnect(Hostname.c_str());

  printSerial("Wifi Connected");
  printSerial("CUSTOM STATIC IP: " + String(ip) + " Netmask: " + String(netmask) + " GW: " + String(gw));
  if (shouldSaveConfig) {
    SPIFFS.begin();
    printSerial("saving config");
    DynamicJsonBuffer jsonBuffer;
    JsonObject& json = jsonBuffer.createObject();
    if (String(custom_ip.getValue()).length() > 5) {
      printSerial("Custom IP Address is set!");
      strcpy(ip, custom_ip.getValue());
      strcpy(netmask, custom_netmask.getValue());
      strcpy(gw, custom_gw.getValue());
    } else {
      strcpy(ip,      "0.0.0.0");
      strcpy(netmask, "0.0.0.0");
      strcpy(gw,      "0.0.0.0");
    }
    strcpy(ccuip, custom_ccuip.getValue());
    strcpy(Variable, custom_variablename.getValue());
    json["ip"] = ip;
    json["netmask"] = netmask;
    json["gw"] = gw;
    json["ccuip"] = ccuip;
    json["variable"] = Variable;

    SPIFFS.remove("/" + configJsonFile);
    File configFile = SPIFFS.open("/" + configJsonFile, "w");
    if (!configFile) {
      printSerial("failed to open config file for writing");
    }

    json.printTo(Serial);
    Serial.println("");
    json.printTo(configFile);
    configFile.close();

    SPIFFS.end();
    delay(100);
    ESP.restart();
  }

  return true;
}


void configModeCallback (WiFiManager *myWiFiManager) {
  printSerial("AP-Modus ist aktiv!");
}

void saveConfigCallback () {
  printSerial("Should save config");
  shouldSaveConfig = true;
}

void parseBytes(const char* str, char sep, byte* bytes, int maxBytes, int base) {
  for (int i = 0; i < maxBytes; i++) {
    bytes[i] = strtoul(str, NULL, base);
    str = strchr(str, sep);
    if (str == NULL || *str == '\0') {
      break;
    }
    str++;
  }
}

bool loadSystemConfig() {
  printSerial("mounting FS...");
  if (SPIFFS.begin()) {
    printSerial("mounted file system");
    if (SPIFFS.exists("/" + configJsonFile)) {
      printSerial("reading config file");
      File configFile = SPIFFS.open("/" + configJsonFile, "r");
      if (configFile) {
        printSerial("opened config file");
        size_t size = configFile.size();
        std::unique_ptr<char[]> buf(new char[size]);
        configFile.readBytes(buf.get(), size);
        DynamicJsonBuffer jsonBuffer;
        JsonObject& json = jsonBuffer.parseObject(buf.get());
        json.printTo(Serial);
        Serial.println("");
        if (json.success()) {
          printSerial("\nparsed json");
          ((json["ip"]).as<String>()).toCharArray(ip, IPSIZE);
          ((json["netmask"]).as<String>()).toCharArray(netmask, IPSIZE);
          ((json["gw"]).as<String>()).toCharArray(gw, IPSIZE);
          ((json["ccuip"]).as<String>()).toCharArray(ccuip, IPSIZE);
          ((json["variable"]).as<String>()).toCharArray(Variable, VARIABLESIZE);

        } else {
          printSerial("failed to load json config");
        }
      }
      return true;
    } else {
      printSerial("/" + configJsonFile + " not found.");
      return false;
    }
    SPIFFS.end();
  } else {
    printSerial("failed to mount FS");
    return false;
  }
}


