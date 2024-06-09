#include <SoftwareSerial.h>
#include <RoboCore_SMW_SX1262M0.h>

#define LORA_RX 7
#define LORA_TX 6

#define RGB_BUILTIN 8
#define RGB_BRIGHTNESS 255

SoftwareSerial LoRaSerial(LORA_RX, LORA_TX);
SMW_SX1262M0 lorawan(LoRaSerial);

CommandResponse response;

bool joined = false;

void setup() {
  Serial.begin(9600);
  LoRaSerial.begin(9600);

  Serial.println(F("--- DevKit ESP32-C3 LoRa Simple Join ---"));

  // Efetua o reset(reboot) do módulo LoRaWAN
  response = lorawan.reset();
  if (response == CommandResponse::OK) {
  } else {
    Serial.println(F("Error on reset the module"));
  }

  delay(1000);

  // Efetua a leitura do Device EUI do módulo
  char deveui[16];
  response = lorawan.get_DevEUI(deveui);
  if (response == CommandResponse::OK) {
    Serial.print(F("DevEUI: "));
    Serial.write(deveui, 16);
    Serial.println();
  } else {
    Serial.println(F("Error getting the Device EUI"));
  }

  // Efetua a leitura do Device Addr do módulo
  char devaddr[8];
  response = lorawan.get_DevAddr(devaddr);
  if (response == CommandResponse::OK) {
    Serial.print(F("DevAddr: "));
    Serial.write(devaddr, 8);
    Serial.println();
  } else {
    Serial.println(F("Error getting the Device Addr"));
  }

  // Efetua a leitura da AppSKey do módulo
  char appSKey[32];
  response = lorawan.get_AppSKey(appSKey);
  if (response == CommandResponse::OK) {
    Serial.print(F("AppSKey: "));
    Serial.write(appSKey, 32);
    Serial.println();
  } else {
    Serial.println(F("Error getting the AppSKey"));
  }

  // Efetua a leitura da NwkSKey do módulo
  char nwkSKey[32];
  response = lorawan.get_NwkSKey(nwkSKey);
  if (response == CommandResponse::OK) {
    Serial.print(F("NwkSKey: "));
    Serial.write(nwkSKey, 32);
    Serial.println();
  } else {
    Serial.println(F("Error getting the NwkSKey"));
  }

  // Define o modo de autenticação na rede LoRaWAN como ABP
  response = lorawan.set_JoinMode(SMW_SX1262M0_JOIN_MODE_ABP);
  if (response == CommandResponse::OK) {
    Serial.println(F("Mode set to ABP"));
  } else {
    Serial.println(F("Error setting the join mode"));
  }

  // Salva as configurações setadas acima na memória do módulo
  response = lorawan.save();
  if (response == CommandResponse::OK) {
    Serial.println(F("Settings saved"));
  } else {
    Serial.println(F("Error on saving"));
  }

  // Efetua uma tentativa de JOIN na rede LoRaWAN
  Serial.println(F("Joining the network"));

  // Aguarda até o módulo conseguir se conectar na rede
  while (true) {
    if (lorawan.isConnected()) {
      Serial.println(F("Joined"));

      neopixelWrite(RGB_BUILTIN, 0, 0, RGB_BRIGHTNESS);
      delay(100);
      neopixelWrite(RGB_BUILTIN, 0, 0, 0);

      break;
    } else {
      Serial.println(F("Not joined"));

      neopixelWrite(RGB_BUILTIN, RGB_BRIGHTNESS, 0, 0);
      delay(100);
      neopixelWrite(RGB_BUILTIN, 0, 0, 0);
    }

    delay(1000);
  }
}

void loop() {
  // Verifica se a conexão está ok
  if (lorawan.isConnected()) {
    joined = true;
  } else {
    joined = false;
  }

  if (joined) {
    Serial.println(F("Sending Uplink"));

    // Led para feedbak
    neopixelWrite(RGB_BUILTIN, 0, RGB_BRIGHTNESS, 0);

    // // Envia para a rede LoRaWAN uma payload no formato String na Porta 2
    // lorawan.sendT(2, "Hello World");

    //Envia para a rede LoRaWAN uma payload no formato JSON na Porta 2
    lorawan.sendT(2, "{\"TempC\":34}");

    // Envia para a rede LoRaWAN uma payload no formato HEX na Porta 1
    // lorawan.sendX(1, "01020304");

    Serial.println(F("Uplink sended"));

    // Led para feedback
    neopixelWrite(RGB_BUILTIN, 0, 0, 0);
  }

  delay(15000);
}