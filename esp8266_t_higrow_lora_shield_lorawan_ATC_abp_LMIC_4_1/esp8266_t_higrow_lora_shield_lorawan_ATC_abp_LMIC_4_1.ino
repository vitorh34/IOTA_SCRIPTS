/*******************************************************************************
 * Copyright (c) 2015 Thomas Telkamp and Matthijs Kooijman
 * Copyright (c) 2018 Terry Moore, MCCI
 *
 * Permission is hereby granted, free of charge, to anyone
 * obtaining a copy of this document and accompanying files,
 * to do whatever they want with them without any restriction,
 * including, but not limited to, copying, modification and redistribution.
 * NO WARRANTY OF ANY KIND IS PROVIDED.
 *
 * This example sends a valid LoRaWAN packet with payload "Hello,
 * world!", using frequency and encryption settings matching those of
 * the The Things Network.
 *
 * This uses ABP (Activation-by-personalisation), where a DevAddr and
 * Session keys are preconfigured (unlike OTAA, where a DevEUI and
 * application key is configured, while the DevAddr and session keys are
 * assigned/generated in the over-the-air-activation procedure).
 *
 * Note: LoRaWAN per sub-band duty-cycle limitation is enforced (1% in
 * g1, 0.1% in g2), but not the TTN fair usage policy (which is probably
 * violated by this sketch when left running for longer)!
 *
 * To use this sketch, first register your application and device with
 * the things network, to set or generate a DevAddr, NwkSKey and
 * AppSKey. Each device should have their own unique values for these
 * fields.
 *
 * Do not forget to define the radio type correctly in
 * arduino-lmic/project_config/lmic_project_config.h or from your BOARDS.txt.
 *
 *******************************************************************************/

 // References:
 // [feather] adafruit-feather-m0-radio-with-lora-module.pdf

#include <lmic.h>
#include <hal/hal.h>
#include <SPI.h>

//
// For normal use, we require that you edit the sketch to replace FILLMEIN
// with values assigned by the TTN console. However, for regression tests,
// we want to be able to compile these scripts. The regression tests define
// COMPILE_REGRESSION_TEST, and in that case we define FILLMEIN to a non-
// working but innocuous value.
//
#ifdef COMPILE_REGRESSION_TEST
# define FILLMEIN 0
#else
# warning "You must replace the values marked FILLMEIN with real values from the TTN control panel!"
# define FILLMEIN (#dont edit this, edit the lines that use FILLMEIN)
#endif

// LoRaWAN NwkSKey, network session key
// This should be in big-endian (aka msb).
static const PROGMEM u1_t NWKSKEY[16] = { 0x42, 0x38, 0xC0, 0xC3, 0x40, 0x1F, 0xF2, 0xC2, 0x5F, 0xE3, 0xA8, 0x10, 0x72, 0x15, 0x74, 0x8E };

// LoRaWAN AppSKey, application session key
// This should also be in big-endian (aka msb).
static const u1_t PROGMEM APPSKEY[16] = { 0x42, 0x38, 0xC0, 0xC3, 0x40, 0x1F, 0xF2, 0xC2, 0x5F, 0xE3, 0xA8, 0x10, 0x72, 0x15, 0x74, 0x8E };

// LoRaWAN end-device address (DevAddr)
// See http://thethingsnetwork.org/wiki/AddressSpace
// The library converts the address to network byte order as needed, so this should be in big-endian (aka msb) too.
static const u4_t DEVADDR = 0x6CE9E160 ; // <-- Change this address for every node!

// These callbacks are only used in over-the-air activation, so they are
// left empty here (we cannot leave them out completely unless
// DISABLE_JOIN is set in arduino-lmic/project_config/lmic_project_config.h,
// otherwise the linker will complain).
void os_getArtEui (u1_t* buf) { }
void os_getDevEui (u1_t* buf) { }
void os_getDevKey (u1_t* buf) { }
// static const u1_t PROGMEM APPEUI[8]={ 0x85, 0xE6, 0x1B, 0x37, 0x54, 0x4B, 0x90, 0x54 };  //colocar valar hexadecimal invertido
// void os_getArtEui (u1_t* buf) { memcpy_P(buf, APPEUI, 8);}
// static const u1_t PROGMEM DEVEUI[8]={ 0xAD, 0x22, 0x6C, 0x4E, 0xB3, 0xF1, 0xAC, 0x41 }; //colocar valar hexadecimal invertido
// void os_getDevEui (u1_t* buf) { memcpy_P(buf, DEVEUI, 8);}
// static const u1_t PROGMEM APPKEY[16] = { 0x42, 0x38, 0xC0, 0xC3, 0x40, 0x1F, 0xF2, 0xC2, 0x5F, 0xE3, 0xA8, 0x10, 0x72, 0x15, 0x74, 0x8E };
// void os_getDevKey (u1_t* buf) {  memcpy_P(buf, APPKEY, 16);}


static uint8_t mydata[] = "T-Higrow LoRaWAN";
static osjob_t sendjob;

// Schedule TX every this many seconds (might become longer due to duty
// cycle limitations).
const unsigned TX_INTERVAL = 30;

/* Definições do rádio LoRa (SX1276) */
#define GANHO_LORA_DB 20 //dB

//Pinos T-Higrow com ESP32
// #define RADIO_RESET         23 
// #define RADIO_DIO_0         14
// #define RADIO_DIO_1         13
// #define RADIO_DIO_2         15
// #define RADIO_CS            18
// #define RADIO_MISO          19
// #define RADIO_MOSI          27
// #define RADIO_CLK           5


//Pinos ESP8266
#define RADIO_RESET         16  // D0  -> 23 shield lora
#define RADIO_DIO_0         5   // D1  -> 14 shield lora
#define RADIO_DIO_1         4   // D2  -> 13 shield lora
// #define RADIO_DIO_2         0   // D3 -> 15 shield lora
#define RADIO_CS            15  // D8 -> 18 shield lora
#define RADIO_MISO          12  // D6 -> 19 shield lora
#define RADIO_MOSI          13  // D7 -> 27 shield lora
#define RADIO_CLK           14  // D5 -> 5 shield lora

// Pin mapping
const lmic_pinmap lmic_pins = {
    .nss = RADIO_CS,
    .rxtx = LMIC_UNUSED_PIN,
    .rst = RADIO_RESET,
    .dio = {RADIO_DIO_0, RADIO_DIO_1, LMIC_UNUSED_PIN},  // Pins for the Heltec ESP32 Lora board/ TTGO Lora32 with 3D metal antenna
    // .rxtx_rx_active = 0,
    // .rssi_cal = 8
};

void onEvent (ev_t ev) {
    Serial.print(os_getTime());
    Serial.print(": ");
    switch(ev) {
        case EV_SCAN_TIMEOUT:
            Serial.println(F("EV_SCAN_TIMEOUT"));
            break;
        case EV_BEACON_FOUND:
            Serial.println(F("EV_BEACON_FOUND"));
            break;
        case EV_BEACON_MISSED:
            Serial.println(F("EV_BEACON_MISSED"));
            break;
        case EV_BEACON_TRACKED:
            Serial.println(F("EV_BEACON_TRACKED"));
            break;
        case EV_JOINING:
            Serial.println(F("EV_JOINING"));
            break;
        case EV_JOINED:
            Serial.println(F("EV_JOINED"));
            break;
        /*
        || This event is defined but not used in the code. No
        || point in wasting codespace on it.
        ||
        || case EV_RFU1:
        ||     Serial.println(F("EV_RFU1"));
        ||     break;
        */
        case EV_JOIN_FAILED:
            Serial.println(F("EV_JOIN_FAILED"));
            break;
        case EV_REJOIN_FAILED:
            Serial.println(F("EV_REJOIN_FAILED"));
            break;
        case EV_TXCOMPLETE:
            Serial.println(F("EV_TXCOMPLETE (includes waiting for RX windows)"));
            if (LMIC.txrxFlags & TXRX_ACK)
              Serial.println(F("Received ack"));
            if (LMIC.dataLen) {
              Serial.println(F("Received "));
              Serial.println(LMIC.dataLen);
              Serial.println(F(" bytes of payload"));
            }
            // Schedule next transmission
            os_setTimedCallback(&sendjob, os_getTime()+sec2osticks(TX_INTERVAL), do_send);
            break;
        case EV_LOST_TSYNC:
            Serial.println(F("EV_LOST_TSYNC"));
            break;
        case EV_RESET:
            Serial.println(F("EV_RESET"));
            break;
        case EV_RXCOMPLETE:
            // data received in ping slot
            Serial.println(F("EV_RXCOMPLETE"));
            break;
        case EV_LINK_DEAD:
            Serial.println(F("EV_LINK_DEAD"));
            break;
        case EV_LINK_ALIVE:
            Serial.println(F("EV_LINK_ALIVE"));
            break;
        /*
        || This event is defined but not used in the code. No
        || point in wasting codespace on it.
        ||
        || case EV_SCAN_FOUND:
        ||    Serial.println(F("EV_SCAN_FOUND"));
        ||    break;
        */
        case EV_TXSTART:
            Serial.println(F("EV_TXSTART"));
            break;
        case EV_TXCANCELED:
            Serial.println(F("EV_TXCANCELED"));
            break;
        case EV_RXSTART:
            /* do not print anything -- it wrecks timing */
            break;
        case EV_JOIN_TXCOMPLETE:
            Serial.println(F("EV_JOIN_TXCOMPLETE: no JoinAccept"));
            break;
        default:
            Serial.print(F("Unknown event: "));
            Serial.println((unsigned) ev);
            break;
    }
}

void do_send(osjob_t* j){
    // Check if there is not a current TX/RX job running
    if (LMIC.opmode & OP_TXRXPEND) {
        Serial.println(F("OP_TXRXPEND, not sending"));
    } else {
        // Prepare upstream data transmission at the next possible time.
        LMIC_setTxData2(1, mydata, sizeof(mydata)-1, 0);
        Serial.println(F("Packet queued"));
    }
    // Next TX is scheduled after TX_COMPLETE event.
}

void setup() {
    Serial.begin(115200);
    Serial.println(F("Starting"));

    #ifdef VCC_ENABLE
    // For Pinoccio Scout boards
    pinMode(VCC_ENABLE, OUTPUT);
    digitalWrite(VCC_ENABLE, HIGH);
    delay(1000);
    #endif

    //SPI.begin(RADIO_CLK, RADIO_MISO, RADIO_MOSI, RADIO_CS);
    SPI.begin();

    // LMIC init
    os_init();
    // Reset the MAC state. Session and pending data transfers will be discarded.
    LMIC_reset();

    // Set static session parameters. Instead of dynamically establishing a session
    // by joining the network, precomputed session parameters are be provided.
    #ifdef PROGMEM
    // On AVR, these values are stored in flash and only copied to RAM
    // once. Copy them to a temporary buffer here, LMIC_setSession will
    // copy them into a buffer of its own again.
    uint8_t appskey[sizeof(APPSKEY)];
    uint8_t nwkskey[sizeof(NWKSKEY)];
    memcpy_P(appskey, APPSKEY, sizeof(APPSKEY));
    memcpy_P(nwkskey, NWKSKEY, sizeof(NWKSKEY));
    LMIC_setSession (0x13, DEVADDR, nwkskey, appskey);
    #else
    // If not running an AVR with PROGMEM, just use the arrays directly
    LMIC_setSession (0x13, DEVADDR, NWKSKEY, APPSKEY);
    #endif

    // Faz inicializações de rádio pertinentes a região do gateway LoRaWAN (ATC / Everynet Brasil) - Opção 1
    // for (u1_t b = 0; b < 8; ++b)	LMIC_disableSubBand(b);

    // LMIC_enableChannel(0); 														// 915.2 MHz
    // LMIC_enableChannel(1); 														// 915.4 MHz
    // LMIC_enableChannel(2); 														// 915.6 MHz
    // LMIC_enableChannel(3); 														// 915.8 MHz
    // LMIC_enableChannel(4); 														// 916.0 MHz
    // LMIC_enableChannel(5); 														// 916.2 MHz
    // LMIC_enableChannel(6); 														// 916.4 MHz
    // LMIC_enableChannel(7); 														// 916.6 MHz

    // Faz inicializações de rádio pertinentes a região do gateway LoRaWAN (ATC / Everynet Brasil) - Opção 2 
    LMIC_selectSubBand(0);

    LMIC_setAdrMode(0);
    LMIC_setLinkCheckMode(0);

    // Data rate para janela de recepção RX2
    LMIC.dn2Dr = DR_SF12CR;

    // Configura data rate de transmissão e ganho do rádio LoRa (dBm) na transmissão 
    LMIC_setDrTxpow(DR_SF12, GANHO_LORA_DB);

    // Inicializa geração de número aleatório
    randomSeed(0);

    // Força primeiro envio de pacote LoRaWAN 
    do_send(&sendjob);	

}

void loop() {
    os_runloop_once();
}
