#include <algorithm>
#include <iostream>
#include <WiFi.h>
#include <Button2.h>            //https://github.com/LennartHennigs/Button2
#include <BH1750.h>             //https://github.com/claws/BH1750
#include "DHT.h"

#include <PubSubClient.h>
#include <ESPDateTime.h>
#include <ArduinoJson.h>

#define DHTTYPE           DHT11     // DHT 11

#define I2C_SDA                 (25)
#define I2C_SCL                 (26)
#define I2C1_SDA                (21)
#define I2C1_SCL                (22)
#define DS18B20_PIN             (21)

#define DHT1x_PIN               (16)
#define BAT_ADC                 (33)
#define SALT_PIN                (34)
#define SOIL_PIN                (32)
#define BOOT_PIN                (0)
#define POWER_CTRL              (4)
#define USER_BUTTON             (35)
#define DS18B20_PIN             (21)                  //18b20 data pin

#define MOTOR_PIN               (19)
#define RGB_PIN                 (18)

#define OB_BH1750_ADDRESS       (0x23)
#define OB_BME280_ADDRESS       (0x77)
#define OB_SHT3X_ADDRESS        (0x44)

typedef enum {
    DHTxx_SENSOR_ID,
    BHT1750_SENSOR_ID,
    SOIL_SENSOR_ID,
    SALT_SENSOR_ID,
    VOLTAGE_SENSOR_ID,
} sensor_id_t;

typedef struct {
    uint32_t timestamp;     /**< time is in milliseconds */
    float temperature;      /**< temperature is in degrees centigrade (Celsius) */
    float light;            /**< light in SI lux units */
    float humidity;         /**<  humidity in percent */
    float voltage;           /**< voltage in volts (V) */
    uint8_t soli;           //Percentage of soil
    uint8_t salt;           //Percentage of salt
} higrow_sensors_event_t;

Button2             useButton(USER_BUTTON);
BH1750              lightMeter(OB_BH1750_ADDRESS);  //0x23
DHT                 dht(DHT1x_PIN, DHTTYPE);

bool                    has_lightSensor = false;
bool                    has_dhtSensor   = false;
bool                    has_dht11       = false;

uint64_t                timestamp       = 0;

// Update these with values suitable for your network.
const char* ssid = "nome do seu wi-fi";
const char* password = "senha do seu wi-fi";
const char* mqtt_server = "iota.vimacsolucoes.com.br";
const char* ntpServer = "pool.ntp.org";
const char* gmtOffset = "GMT+3";     //Replace with your GMT offset
const char* dateTime;
const char* clientID = "clientID gerado na plataforma IOTA";
const char* mqttTopic = "e-mail utilizado no cadastro na plataforma IOTA";

WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE  (200)
char msg[MSG_BUFFER_SIZE];

void deviceProbe(TwoWire &t);

// void sleepHandler(Button2 &b){
void sleepHandler(){  
    Serial.println("Enter Deepsleep ...");
    //esp_sleep_enable_ext1_wakeup(GPIO_SEL_35, ESP_EXT1_WAKEUP_ALL_LOW);
    esp_sleep_enable_timer_wakeup(3600000000);
    delay(1000);
    esp_deep_sleep_start();
}

void setup_wifi(){
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

    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
}

bool get_higrow_sensors_event(sensor_id_t id, higrow_sensors_event_t &val){
    switch (id) {
    case DHTxx_SENSOR_ID: {
        val.temperature = dht.readTemperature();
        val.humidity = dht.readHumidity();
        if (isnan(val.temperature)) {
            val.temperature = 0.0;
        }
        if (isnan(val.humidity)) {
            val.humidity = 0.0;
        }
    }
    break;

    case BHT1750_SENSOR_ID: {
        val.light = lightMeter.readLightLevel();
        if (isnan(val.light)) {
            val.light = 0.0;
        }
    }
    break;

    case SOIL_SENSOR_ID: {
        uint16_t soil = analogRead(SOIL_PIN);
        val.soli = map(soil, 0, 4095, 100, 0);
    }
    break;

    case SALT_SENSOR_ID: {
        uint8_t samples = 120;
        uint32_t humi = 0;
        uint16_t array[120];
        for (int i = 0; i < samples; i++) {
            array[i] = analogRead(SALT_PIN);
            delay(2);
        }
        std::sort(array, array + samples);
        for (int i = 1; i < samples - 1; i++) {
            humi += array[i];
        }
        humi /= samples - 2;
        val.salt = humi;
    }
    break;

    case VOLTAGE_SENSOR_ID: {
        int vref = 1100;
        uint16_t volt = analogRead(BAT_ADC);
        val.voltage = ((float)volt / 4095.0) * 6.6 * (vref);
    }
    break;
    default:
        break;
    }
    return true;
}

bool dhtSensorProbe(){
    dht.begin();
    delay(2000);// Wait a few seconds between measurements.
    int i = 5;
    while (i--) {
        // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
        float h = dht.readHumidity();
        // Check if any reads failed and exit early (to try again).
        if (isnan(h)) {
            Serial.println("Failed to read from DHT sensor!");
        } else {
            return true;
        }
        delay(500);
    }
    return false;
}

void reconnect(){
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(clientID)) {
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

void setup(){
    Serial.begin(115200);

    // useButton.setLongClickHandler(sleepHandler);

    /* *
    * Warning:
    *   Higrow sensor power control pin, use external port and onboard sensor, IO4 must be set high
    */

    pinMode(POWER_CTRL, OUTPUT);
    digitalWrite(POWER_CTRL, HIGH);
    delay(100);

    Wire.begin(I2C_SDA, I2C_SCL);
    Wire1.begin(I2C1_SDA, I2C1_SCL);

    Serial.println("-------------Devices probe-------------");
    deviceProbe(Wire);
    deviceProbe(Wire1);

    //Check DHT11 temperature and humidity sensor
    if (!dhtSensorProbe()) {
        has_dht11 = false;
        Serial.println("Warning: Failed to find DHT11 temperature and humidity sensor!");
    } else {
        has_dht11 = true;
        Serial.println("DHT11 temperature and humidity sensor init succeeded, using DHT11");
    }

    //Check BH1750 light sensor
    if (has_lightSensor) {
        if (!lightMeter.begin()) {
            has_lightSensor = false;
            Serial.println("Warning: Failed to find BH1750 light sensor!");
        } else {
            Serial.println("BH1750 light sensor init succeeded, using BH1750");
        }
    }

    // Código de conexão Wi-Fi e conexão ao Broker MQTT
    setup_wifi();
    client.setServer(mqtt_server, 1883);

    //start and config datetime
    setup_datetime();
}



void loop(){
    useButton.loop();

    if (!client.connected()) {
      reconnect();
    }
    client.loop();

    if (millis() - timestamp > 2000) {
        timestamp = millis();

        if (!DateTime.isTimeValid()) {
          Serial.println("Failed to get time from server, retry.");
          DateTime.begin();
        } else {
          Serial.println(DateTime.toISOString());
        }

        higrow_sensors_event_t val = {0};

        get_higrow_sensors_event(SOIL_SENSOR_ID, val);
        get_higrow_sensors_event(SALT_SENSOR_ID, val);
        get_higrow_sensors_event(VOLTAGE_SENSOR_ID, val);
        if (has_dht11) {
            get_higrow_sensors_event(DHTxx_SENSOR_ID, val);
        }
        if (has_lightSensor) {
            get_higrow_sensors_event(BHT1750_SENSOR_ID, val);
        }

        // Allocate the JSON document
        DynamicJsonDocument  doc(MSG_BUFFER_SIZE);

        // Atribui os valores ao documento JSON
        doc["ts"] = DateTime.toISOString();
        doc["device"] = WiFi.macAddress();
        doc["temperature"] = String(val.temperature);
        doc["humidity"] = String(val.humidity);
        doc["soil"] = String(val.soli);
        doc["voltage"] = String(val.voltage);
        doc["light"] = String(val.light);
        serializeJson(doc, msg);

        // Publica os dados no tópico MQTT
        char topic[100];
        sprintf(topic, "%s/%s", mqttTopic, clientID);
        client.publish(topic, msg);
        
        Serial.print("Publish message: ");
        Serial.println(msg);

        sleepHandler();
    }
}



void deviceProbe(TwoWire &t){
    uint8_t err, addr;
    int nDevices = 0;
    for (addr = 1; addr < 127; addr++) {
        t.beginTransmission(addr);
        err = t.endTransmission();
        if (err == 0) {

            switch (addr) {
            case OB_BH1750_ADDRESS:
                has_lightSensor = true;
                Serial.println("BH1750 light sensor found!");
                break;
            default:
                Serial.print("I2C device found at address 0x");
                if (addr < 16)
                    Serial.print("0");
                Serial.print(addr, HEX);
                Serial.println(" !");
                break;
            }
            nDevices++;
        } else if (err == 4) {
            Serial.print("Unknow error at address 0x");
            if (addr < 16)
                Serial.print("0");
            Serial.println(addr, HEX);
        }
    }
}








