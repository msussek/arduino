#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPUpdateServer.h>
#include <Ticker.h>
#include <WiFiClient.h>
#include <PubSubClient.h>
extern "C" {
#include "gpio.h"
#include "user_interface.h"
}
 
#define INT_PIN1 5     // Zuordnung der GPIOs
#define INT_PIN2 4
#define INT_PIN3 2
#define INT_PIN4 13
#define INT_PIN5 12
#define INT_PIN6 14
 
unsigned long impulse[6], alteZeit[6], entprellZeit[6];
 
ESP8266WebServer server(80);// Serverport  hier einstellen
ESP8266HTTPUpdateServer httpUpdater;
Ticker Timer;
WiFiClient net;
PubSubClient client(net);

// Zugangsdaten zum WLAN:
const char* ssid = "meineWLAN-SSID";
const char* password = "meinWLANPasswort";

// Zugangsdaten zum MQTT-Broker:
const char* mqtt_server = "HostnameMQTT-Broker";
const int   mqtt_port = 1883;
const char* mqtt_user = "meinMQTTUserName";
const char* mqtt_password = "meinMQTTPasswort";

 
void ICACHE_RAM_ATTR interruptRoutine1() {
  handleInterrupt(0);
}
void ICACHE_RAM_ATTR interruptRoutine2() {
  handleInterrupt(1);
}
void ICACHE_RAM_ATTR interruptRoutine3() {
  handleInterrupt(2);
}
void ICACHE_RAM_ATTR interruptRoutine4() {
  handleInterrupt(3);
}
void ICACHE_RAM_ATTR interruptRoutine5() {
  handleInterrupt(4);
}
void ICACHE_RAM_ATTR interruptRoutine6() {
  handleInterrupt(5);
}

void handleInterrupt(int i){
  unsigned long timeInMillis = millis();
  if((timeInMillis - alteZeit[i]) > entprellZeit[i] || timeInMillis < alteZeit[i]) { 
    impulse[i]++;
    Serial.println("Impulse Zähler " + String(i) + ": " + String(impulse[i]));
    alteZeit[i] = timeInMillis;
   }
}
 
void handleHttpRequest(){
  String response = "";
  for (int i = 0; i < 6; i++){
    response += String(impulse[i]) + ",";
  }
  server.sendHeader("Cache-Control", "no-cache");
  server.send(200, "text/plain", response);
}
 
void sendMqttMessage(){ 
  Serial.println("Sende MQTT Nachrichten");
  for (int i = 0; i < 6; i++) {
    char buffer[10];
    sprintf(buffer, "%lu", impulse[i]);
    String topic = "/SmartHome/Sensor/Haustechnikraum/Impulszaehler/Zaehler_" + String(i) + "/Impulse";
    client.publish(const_cast<char*>(topic.c_str()), buffer);
  }
  for (int i = 0; i < 6; i++) {
    char buffer[10];
    sprintf(buffer, "%lu", entprellZeit[i]);
    String topic = "/SmartHome/Sensor/Haustechnikraum/Impulszaehler/Zaehler_" + String(i) + "/Entprellzeit";
    client.publish(const_cast<char*>(topic.c_str()), buffer);
  }
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Verbindungsaufbau zum MQTT-Broker...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str(), mqtt_user, mqtt_password)) {
      Serial.println("connected");
      client.subscribe("/SmartHome/Sensor/Haustechnikraum/Impulszaehler/*/Entprellzeit");
    } else {
      Serial.print("fehlgeschlagen, rc=");
      Serial.print(client.state());
      Serial.println(" erneut versuchen in 5 Sekunden");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  String sTopic = String(topic);

  for (int i = 0; i < 6; i++) {
    char buffer[10];
    String topic = "/SmartHome/Sensor/Haustechnikraum/Impulszaehler/Zaehler_" + String(i) + "/Entprellzeit";
    if (sTopic == topic) {
       // Workaround to get int from payload
      payload[length] = '\0';
      Serial.println("Setze Entprellzeit für Zähler " + String(i) + " auf " + String((char*)payload));
      entprellZeit[i] = String((char*)payload).toInt();
    }
  }
}
 
void setup(){
  pinMode(INT_PIN1, INPUT_PULLUP);
  pinMode(INT_PIN2, INPUT_PULLUP);
  pinMode(INT_PIN3, INPUT_PULLUP);
  pinMode(INT_PIN4, INPUT_PULLUP);
  pinMode(INT_PIN5, INPUT_PULLUP);
  pinMode(INT_PIN6, INPUT_PULLUP);
 
  Serial.begin(115200);
  Serial.println("");
  Serial.println("Start");
  Serial.println("");
  Serial.println("Warte auf Verbindung");
 
  WiFi.mode(WIFI_STA);                             // station  modus verbinde mit dem Router
  WiFi.begin(ssid, password); // WLAN Login daten
  WiFi.hostname("ESP-Zaeler");

  // Warte auf verbindung
  int timout = 0;
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print("O");
    timout++;
    if  (timout > 20) // Wenn Anmeldung nicht möglich
    {
      Serial.println("");
      Serial.println("Wlan verbindung fehlt");
      break;
    }
  }
 
  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.println("");
    Serial.println("Mit Wlan verbunden");
    Serial.println("IP Adresse: ");
    Serial.println(WiFi.localIP());
  }

  for (int i = 0; i < 6; i++){
    alteZeit[i]=0;
    entprellZeit[i]=1000;
  }

  client.setServer(mqtt_server, mqtt_port);
 
  server.on("/", handleHttpRequest);     //  Bechandlung der Ereignissen anschlissen
  httpUpdater.setup(&server);         //  Updater
  server.begin();                     // Starte den Server
  Serial.println("HTTP Server gestartet");
 
  Timer.attach(30, sendMqttMessage);    // Starte den 60s Timer
 
  // ---------------------- Starte Interrupts ---------------------------------------------
  attachInterrupt(INT_PIN1, interruptRoutine1, FALLING);
  attachInterrupt(INT_PIN2, interruptRoutine2, FALLING);
  attachInterrupt(INT_PIN3, interruptRoutine3, FALLING);
  attachInterrupt(INT_PIN4, interruptRoutine4, FALLING);
  attachInterrupt(INT_PIN5, interruptRoutine5, FALLING);
  attachInterrupt(INT_PIN6, interruptRoutine6, FALLING);

  reconnect();

  client.setCallback(callback);
}
 
void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  server.handleClient();
}
