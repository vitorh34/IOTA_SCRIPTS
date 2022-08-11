/*
 Basic ESP8266 MQTT example
 This sketch demonstrates the capabilities of the pubsub library in combination
 with the ESP8266 board/library.
 It connects to an MQTT server then:
  - publishes "hello world" to the topic "outTopic" every two seconds
  - subscribes to the topic "inTopic", printing out any messages
    it receives. NB - it assumes the received payloads are strings not binary
  - If the first character of the topic "inTopic" is an 1, switch ON the ESP Led,
    else switch it off
 It will reconnect to the server if the connection is lost using a blocking
 reconnect function. See the 'mqtt_reconnect_nonblocking' example for how to
 achieve the same result without blocking the main loop.
 To install the ESP8266 board, (using Arduino 1.6.4+):
  - Add the following 3rd party board manager under "File -> Preferences -> Additional Boards Manager URLs":
       http://arduino.esp8266.com/stable/package_esp8266com_index.json
  - Open the "Tools -> Board -> Board Manager" and click install for the ESP8266"
  - Select your ESP8266 in "Tools -> Board"
*/

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ESPDateTime.h>


// Update these with values suitable for your network.
const char* ssid = "Vitor";
const char* password = "puppy301112";
const char* mqtt_server = "iota.vimacsolucoes.com.br";
//const char* mqtt_server = "192.168.1.107";  //testar localhost

const char* ntpServer = "pool.ntp.org";
const char* gmtOffset = "GMT+3";     //Replace with your GMT offset
const char* dateTime;
// Update these with values suitable for your clientID.
const char* clientID = "8ec4897f-7f4d-4ff2-84bc-25d815741360";

//define potentiometer anolog input
const int ADC_pin = A0;  
int sensor_reading = 0;  

String resistence, state, datetime, mac, payload;

WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE	(110)
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
    if (client.connect(clientID)) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish("outTopic", "hello world");
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
  setup_wifi();
  client.setServer(mqtt_server, 1883);

  //start and config datetime
  setup_datetime();

}

void loop() {

  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  unsigned long now = millis();
  if (now - lastMsg > 2000) {
    lastMsg = now;

    if (!DateTime.isTimeValid()) {
      Serial.println("Failed to get time from server, retry.");
      DateTime.begin();
    } else {
      Serial.println(DateTime.toISOString());
    }

//    sensor_reading = analogRead(ADC_pin);   
//    datetime = DateTime.toISOString();
//    mac = WiFi.macAddress().c_str();
//    
    //define and publish message to MQTT Broker
    snprintf (msg, MSG_BUFFER_SIZE, "{'ts': '%s', 'device': '%s', 'resistence': %d}", DateTime.toISOString().c_str(), WiFi.macAddress().c_str(), analogRead(ADC_pin));
    client.publish("outTopic", msg);
    Serial.print("Publish message: ");
    Serial.println(msg);

  }
}
