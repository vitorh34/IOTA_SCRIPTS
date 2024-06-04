#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ESPDateTime.h>
#include <ArduinoJson.h>
#include <Adafruit_BMP085.h>

Adafruit_BMP085 bmp;

// Update these with values suitable for your network.
const char* ssid = "Vitor";
const char* password = "puppy301112";
const char* mqtt_server = "iota.vimacsolucoes.com.br";

const char* ntpServer = "pool.ntp.org";
const char* gmtOffset = "GMT+3";     //Replace with your GMT offset
const char* dateTime;
const char* mqttTopic = "vitorh34@yahoo.com.br/7af69ddd-b059-43b8-9e83-4ddcf4b6481b";
const char* clientID = "7af69ddd-b059-43b8-9e83-4ddcf4b6481b";
const char* username = "vitorh34@yahoo.com.br";
const char* passwd = "7af69ddd-b059-43b8-9e83-4ddcf4b6481b";

String datetime, mac;
float temperature, pressure, altitude;

WiFiClient espClient;
PubSubClient client(espClient);
unsigned long updateInterval = 300000;  //Intervalo de envio de dados p/ plataforma (em milisegundos)
unsigned long lastMsg = 2000 - updateInterval; //Variável de controle do loop

#define MSG_BUFFER_SIZE  (200)
char msg[MSG_BUFFER_SIZE];

void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(clientID, username, passwd)) {
      Serial.println("connected");
      // Once connected, publish an announcement...
//      client.publish("outTopic", "hello world");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup_datetime(){
  //setup datetime config
  DateTime.setServer(ntpServer);
  DateTime.setTimeZone(gmtOffset);
  DateTime.begin();
  if (!DateTime.isTimeValid()) {
    Serial.println("Failed to get time from server.");
  } else {
    Serial.printf("Date Now is %s\n", DateTime.toISOString().c_str());
//    Serial.printf("Timestamp is %ld\n", DateTime.now());
  }
}

void setup() {
  pinMode(BUILTIN_LED, OUTPUT);     // Initialize the BUILTIN_LED pin as an output
  Serial.begin(115200);
  
  // Código de conexão Wi-Fi e conexão ao Broker MQTT
  setup_wifi();
  client.setServer(mqtt_server, 1883);

  //start and config datetime
  setup_datetime();

  if (!bmp.begin()) {
    Serial.println("Could not find a valid BMP085 sensor, check wiring!");
    while (1);
  }
}

void loop() {

  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  unsigned long now = millis();
  if (now - lastMsg > updateInterval) {
    lastMsg = now;

    if (!DateTime.isTimeValid()) {
      Serial.println("Failed to get time from server, retry.");
      DateTime.begin();
    } else {
      Serial.println(DateTime.toISOString());
    }

    // Código de leitura do sensor BMP180
    temperature = bmp.readTemperature();
    pressure = bmp.readPressure();
    altitude = bmp.readAltitude();

    if (isnan(temperature) || isnan(pressure)) {
      Serial.println("Failed to read temperature and pressure from DHT sensor");
      return;
    }

    // Allocate the JSON document
    DynamicJsonDocument  doc(MSG_BUFFER_SIZE);
    // Atribui os valores ao documento JSON
    doc["ts"] = DateTime.toISOString();
    doc["device"] = WiFi.macAddress();
    doc["temperature"] = String(temperature);
    doc["pressure"] = String(pressure); 
    doc["altitude"] = String(altitude);
    serializeJson(doc, msg);

    // Publica os dados no tópico MQTT
    client.publish(mqttTopic, msg);
    
    Serial.print("Publish message: ");
    Serial.println(msg);   

  }
}