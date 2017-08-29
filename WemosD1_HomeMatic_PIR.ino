#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiManager.h>
#include <FS.h>
#include <ArduinoJson.h>

#define TasterPin         D7
#define InputPin1         D5
#define InputPin2         D6
#define InputPin3         D1
#define InputPin4         D2

#define IPSIZE  16
#define VARIABLESIZE  100
char ccuip[IPSIZE];
char Variable1[VARIABLESIZE];
char Variable2[VARIABLESIZE];
char Variable3[VARIABLESIZE];
char Variable4[VARIABLESIZE];

byte BackendType = 0;

//WifiManager - don't touch
byte ConfigPortalTimeout = 180;
bool shouldSaveConfig        = false;
String configJsonFile        = "config.json";
bool wifiManagerDebugOutput = false;
char ip[IPSIZE]      = "0.0.0.0";
char netmask[IPSIZE] = "0.0.0.0";
char gw[IPSIZE]      = "0.0.0.0";
boolean startWifiManager = false;

enum BackendTypes_e {
  BackendType_HomeMatic,
  BackendType_ioBroker
};

volatile byte interrupt1Detected = 0;
volatile byte interrupt2Detected = 0;
volatile byte interrupt3Detected = 0;
volatile byte interrupt4Detected = 0;

void handleInterrupt1() {
  interrupt1Detected = (digitalRead(InputPin1) == HIGH) ? 1 : 2;
}
void handleInterrupt2() {
  interrupt2Detected = (digitalRead(InputPin2) == HIGH) ? 1 : 2;
}
void handleInterrupt3() {
  interrupt3Detected = (digitalRead(InputPin3) == HIGH) ? 1 : 2;
}
void handleInterrupt4() {
  interrupt4Detected = (digitalRead(InputPin4) == HIGH) ? 1 : 2;
}
void setup() {
  pinMode(TasterPin, INPUT_PULLUP);
  pinMode(InputPin1, INPUT);
  pinMode(InputPin2, INPUT);
  pinMode(InputPin3, INPUT);
  pinMode(InputPin4, INPUT);
  attachInterrupt(digitalPinToInterrupt(InputPin1), handleInterrupt1, CHANGE);
  attachInterrupt(digitalPinToInterrupt(InputPin2), handleInterrupt2, CHANGE);
  attachInterrupt(digitalPinToInterrupt(InputPin3), handleInterrupt3, CHANGE);
  attachInterrupt(digitalPinToInterrupt(InputPin4), handleInterrupt4, CHANGE);
  pinMode(LED_BUILTIN, OUTPUT);

  digitalWrite(LED_BUILTIN,  HIGH);
  wifiManagerDebugOutput = true;
  Serial.begin(115200);
  Serial.println("Programmstart...");

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
    Serial.println("WLAN erfolgreich verbunden!");
  } else ESP.restart();
}

void loop() {
  if (interrupt1Detected) {
    sendMotionDetectedToCCU(interrupt1Detected, Variable1);
    interrupt1Detected = 0;
  }
  if (interrupt2Detected) {
    sendMotionDetectedToCCU(interrupt2Detected, Variable2);
    interrupt2Detected = 0;
  }
  if (interrupt3Detected) {
    sendMotionDetectedToCCU(interrupt3Detected, Variable3);
    interrupt3Detected = 0;
  }
  if (interrupt4Detected) {
    sendMotionDetectedToCCU(interrupt4Detected, Variable4);
    interrupt4Detected = 0;
  }
}

void sendMotionDetectedToCCU(byte interruptValue, char *Variable) {
  String tempVar = Variable;
  if (interruptValue > 0 && tempVar != "") {
    if (WiFi.status() == WL_CONNECTED) {
      String val = (interruptValue == 1) ? "true" : "false";
      HTTPClient http;
      tempVar.replace(" ", "%20");
      String url = "";
      if (BackendType == BackendType_HomeMatic)
        url = "http://" + String(ccuip) + ":8181/cuxd.exe?ret=dom.GetObject(%22" + tempVar + "%22).State(" + val + ")";
      if (BackendType == BackendType_ioBroker)
        url = "http://" + String(ccuip) + ":8087/set/" + tempVar + "?value=" + val + "&wait=100&prettyPrint";

      Serial.println("URL = " + url);
      http.begin(url);
      int httpCode = http.GET();
      Serial.println("httpcode = " + String(httpCode));

      if (httpCode != 200) {
        Serial.println("HTTP fail " + String(httpCode));
      }
      http.end();
    } else ESP.restart();
  }
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
  WiFiManagerParameter custom_ccuip("central", "CCU2 / ioBroker IP", ccuip, IPSIZE);

  WiFiManagerParameter custom_variable1name("variable1", "1. Systemvariable / ObjektID (D5)", Variable1, VARIABLESIZE);
  WiFiManagerParameter custom_variable2name("variable2", "2. Systemvariable / ObjektID (D6)", Variable2, VARIABLESIZE);
  WiFiManagerParameter custom_variable3name("variable3", "3. Systemvariable / ObjektID (D1)", Variable3, VARIABLESIZE);
  WiFiManagerParameter custom_variable4name("variable4", "4. Systemvariable / ObjektID (D2)", Variable4, VARIABLESIZE);

  String options = "";
  switch (BackendType) {
    case BackendType_HomeMatic:
      options = F("<option selected value='0'>HomeMatic</option><option value='1'>ioBroker</option>");
      break;
    case BackendType_ioBroker:
      options = F("<option value='0'>HomeMatic</option><option selected value='1'>ioBroker</option>");
      break;
    default:
      options = F("<option value='0'>HomeMatic</option><option value='1'>ioBroker</option>");
      break;
  }
  WiFiManagerParameter custom_backendtype("backendtype", "Backend", "", 8, 2, options.c_str());

  WiFiManagerParameter custom_ip("custom_ip", "IP-Adresse", "", IPSIZE);
  WiFiManagerParameter custom_netmask("custom_netmask", "Netzmaske", "", IPSIZE);
  WiFiManagerParameter custom_gw("custom_gw", "Gateway", "", IPSIZE);

  wifiManager.addParameter(&custom_backendtype);
  wifiManager.addParameter(&custom_ccuip);
  wifiManager.addParameter(&custom_variable1name);
  wifiManager.addParameter(&custom_variable2name);
  wifiManager.addParameter(&custom_variable3name);
  wifiManager.addParameter(&custom_variable4name);
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
        Serial.println("failed to connect and hit timeout");
        delay(1000);
        ESP.restart();
      }
    }
  }

  wifiManager.setSTAStaticIPConfig(IPAddress(ipBytes[0], ipBytes[1], ipBytes[2], ipBytes[3]), IPAddress(gwBytes[0], gwBytes[1], gwBytes[2], gwBytes[3]), IPAddress(netmaskBytes[0], netmaskBytes[1], netmaskBytes[2], netmaskBytes[3]));

  wifiManager.autoConnect(Hostname.c_str());

  Serial.println("Wifi Connected");
  Serial.println("CUSTOM STATIC IP: " + String(ip) + " Netmask: " + String(netmask) + " GW: " + String(gw));
  if (shouldSaveConfig) {
    SPIFFS.begin();
    Serial.println("saving config");
    DynamicJsonBuffer jsonBuffer;
    JsonObject& json = jsonBuffer.createObject();
    if (String(custom_ip.getValue()).length() > 5) {
      Serial.println("Custom IP Address is set!");
      strcpy(ip, custom_ip.getValue());
      strcpy(netmask, custom_netmask.getValue());
      strcpy(gw, custom_gw.getValue());
    } else {
      strcpy(ip,      "0.0.0.0");
      strcpy(netmask, "0.0.0.0");
      strcpy(gw,      "0.0.0.0");
    }
    strcpy(ccuip, custom_ccuip.getValue());
    strcpy(Variable1, custom_variable1name.getValue());
    strcpy(Variable2, custom_variable2name.getValue());
    strcpy(Variable3, custom_variable3name.getValue());
    strcpy(Variable4, custom_variable4name.getValue());
    BackendType = (atoi(custom_backendtype.getValue()));
    json["ip"] = ip;
    json["netmask"] = netmask;
    json["gw"] = gw;
    json["ccuip"] = ccuip;
    json["variable1"] = Variable1;
    json["variable2"] = Variable2;
    json["variable3"] = Variable3;
    json["variable4"] = Variable4;
    json["backendtype"] = BackendType;

    SPIFFS.remove("/" + configJsonFile);
    File configFile = SPIFFS.open("/" + configJsonFile, "w");
    if (!configFile) {
      Serial.println("failed to open config file for writing");
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
  Serial.println("AP-Modus ist aktiv!");
}

void saveConfigCallback () {
  Serial.println("Should save config");
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
  Serial.println("mounting FS...");
  if (SPIFFS.begin()) {
    Serial.println("mounted file system");
    if (SPIFFS.exists("/" + configJsonFile)) {
      Serial.println("reading config file");
      File configFile = SPIFFS.open("/" + configJsonFile, "r");
      if (configFile) {
        Serial.println("opened config file");
        size_t size = configFile.size();
        std::unique_ptr<char[]> buf(new char[size]);
        configFile.readBytes(buf.get(), size);
        DynamicJsonBuffer jsonBuffer;
        JsonObject& json = jsonBuffer.parseObject(buf.get());
        json.printTo(Serial);
        Serial.println("");
        if (json.success()) {
          Serial.println("\nparsed json");
          ((json["ip"]).as<String>()).toCharArray(ip, IPSIZE);
          ((json["netmask"]).as<String>()).toCharArray(netmask, IPSIZE);
          ((json["gw"]).as<String>()).toCharArray(gw, IPSIZE);
          ((json["ccuip"]).as<String>()).toCharArray(ccuip, IPSIZE);
          ((json["variable1"]).as<String>()).toCharArray(Variable1, VARIABLESIZE);
          ((json["variable2"]).as<String>()).toCharArray(Variable2, VARIABLESIZE);
          ((json["variable3"]).as<String>()).toCharArray(Variable3, VARIABLESIZE);
          ((json["variable4"]).as<String>()).toCharArray(Variable4, VARIABLESIZE);
          BackendType = json["backendtype"];
        } else {
          Serial.println("failed to load json config");
        }
      }
      return true;
    } else {
      Serial.println("/" + configJsonFile + " not found.");
      return false;
    }
    SPIFFS.end();
  } else {
    Serial.println("failed to mount FS");
    return false;
  }
}


