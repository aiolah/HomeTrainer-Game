#include <ArduinoBLE.h>
#include <PubSubClient.h>
#include <WiFiS3.h>

const char *ssid     = "RouterIotFIA4";
const char *password = "RouterIotFIA4!";
const char *mqtt_broker = "192.168.0.175";
const char *mqtt_topic  = "isisChallenge/borne/B019";
const int   mqtt_port   = 1883;
String client_id = "BorneB019-";

WiFiClient wifiClient;
PubSubClient mqtt_client(wifiClient);
static unsigned long lastScanTime = 0;

void connectToWiFi() {
  WiFi.begin(ssid, password);
  Serial.print("Connexion WiFi");
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  Serial.println("\nWiFi connecté !");
  byte mac[6]; WiFi.macAddress(mac);
  for (int i = 0; i < 6; i++) client_id += String(mac[i], HEX);
}

void connectToMQTTBroker() {
  while (!mqtt_client.connected()) {
    Serial.print("Connexion MQTT...");
    if (mqtt_client.connect(client_id.c_str())) {
      Serial.println("Connecté !");
    } else {
      Serial.print("Échec rc="); Serial.print(mqtt_client.state());
      Serial.println(" retry 5s"); delay(5000);
    }
  }
}

void setup() {
  Serial.begin(9600); while (!Serial);
  connectToWiFi();
  mqtt_client.setServer(mqtt_broker, mqtt_port);
  connectToMQTTBroker();
  if (!BLE.begin()) { Serial.println("Erreur BLE !"); while (1); }
  Serial.println("Borne B019 - Scan BLE actif...");
  BLE.scan();
}

void loop() {
  if (!mqtt_client.connected()) connectToMQTTBroker();
  mqtt_client.loop();

  unsigned long now = millis();
  if (now - lastScanTime >= 2000) {
    lastScanTime = now;
    int bestRssi = -100;

    BLEDevice peripheral = BLE.available();
    while (peripheral) {
      if (peripheral.hasLocalName() && peripheral.localName().endsWith("Beacon")) {
        int rssi = peripheral.rssi();
        if (rssi > bestRssi) bestRssi = rssi;
        Serial.print("Beacon détecté RSSI : "); Serial.print(rssi); Serial.println(" dBm");
      }
      peripheral = BLE.available();
    }

    mqtt_client.publish(mqtt_topic, String(bestRssi).c_str());
    Serial.print("Borne B019 → publié : "); Serial.println(bestRssi);
    BLE.scan();
  }
}
