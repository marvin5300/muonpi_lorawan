/*******************************************************************************
 * AmbaSat-1
 * Filename: AmbaSatLMIC.cpp
 *
 * This library is designed for use with AmbaSat-1 and is a wrapper for
 * IBM LMIC functionality
 *
 * Copyright (c) 2022 AmbaSat Ltd
 * https://ambasat.com
 *
 * licensed under Creative Commons Attribution-ShareAlike 3.0
 *******************************************************************************/

/*******************************************************************************
 *                Leds                GPIO
 *                ----                ----
 *                LED   <――――――――――>  13  (LED_BUILTIN) (SCK) Active-high,
 *                                        Useless, shared with SCK.
 *
 *                I2C [display]       GPIO
 *                ---                 ----
 *                SDA   <――――――――――>   2  (SDA)
 *                SCL   <――――――――――>   3  (SCL)
 *
 *                SPI/LoRa module     GPIO
 *                ----                ----
 *                MOSI  <――――――――――>  11  (MOSI)
 *                MISO  <――――――――――>  12  (MISO)
 *                SCK   <――――――――――>  13  (SCK)
 *                NSS   <――――――――――>  10  (SS)
 *                RST   <――――――――――>   7
 *                DIO0  <――――――――――>   8
 *                DIO1  <――――――――――>   9
 *                DIO2                 -  Not needed for LoRa.
 *******************************************************************************/

#include "muonpi_lmic.h"
#include <Arduino.h>
#include <lmic.h>
#include <hal/hal.h>
// #include <SPI.h>

bool joined = false;
bool sleeping = false;

// static osjob_t sendjob;
static osjob_t sendjob;

uint32_t uplinkSequenceNo = 0; // aka FCnt

uint8_t *data = nullptr;

// arduino lmic pin mapping
const lmic_pinmap lmic_pins = {
    .nss = 10,
    .rxtx = LMIC_UNUSED_PIN,
    .rst = 7,
    .dio = {/*dio0*/ 8, /*dio1*/ 9, /*dio2*/ LMIC_UNUSED_PIN},
    .rxtx_rx_active = 0,
    .rssi_cal = 10,
    .spi_freq = 1000000 /* 1 MHz */
};

// ======================================================================================



void printEvent(ev_t ev){

}
// =========================================================================================================================================
// onEvent
// =========================================================================================================================================

void MuonPiLMIC::onEvent(void *pUserData, ev_t ev)
{
    switch (ev)
    {
    case EV_RXSTART:
        // Do not print anything for this event or it will mess up timing.
        break;

    case EV_TXSTART:
        Serial.print(os_getTime());
        Serial.print(": ");
        Serial.print(F("EV_TXSTART\n"));
        break;
    case EV_JOIN_TXCOMPLETE:
        Serial.print(F("EV_JOIN_TXCOMPLETE\n"));
    case EV_TXCANCELED:
        Serial.print(F("EV_TXCANCELLED\n"));
        break;
    case EV_SCAN_TIMEOUT:
        Serial.print(F("EV_SCAN_TIMEOUT\n"));
        break;
    case EV_BEACON_FOUND:
        Serial.print(F("EV_BEACON_FOUND\n"));
        break;
    case EV_BEACON_MISSED:
        Serial.print(F("EV_BEACON_MISSED\n"));
        break;
    case EV_BEACON_TRACKED:
        Serial.print(F("EV_BEACON_TRACKED\n"));
        break;
    case EV_JOINING:
        Serial.print(F("EV_JOINING\n"));
        break;
    case EV_JOINED:
        Serial.print(F("EV_JOINED\n"));
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
        Serial.print(F("EV_JOIN_FAILED\n"));
        break;
    case EV_REJOIN_FAILED:
        Serial.print(F("EV_REJOIN_FAILED\n"));
        break;
    case EV_TXCOMPLETE:
        Serial.print(F("EV_TXCOMPLETE (includes waiting for RX windows)\n"));
        if (LMIC.txrxFlags & TXRX_ACK)
            Serial.print(F("Received ack\n"));
        if (LMIC.dataLen)
        {
            Serial.println(F("Received "));
            Serial.println(LMIC.dataLen);
            Serial.println(F(" bytes of payload"));
        }
        // Schedule next transmission
        // will be called by main loop
        break;
    case EV_LOST_TSYNC:
        Serial.print(F("EV_LOST_TSYNC\n"));
        break;
    case EV_RESET:
        Serial.print(F("EV_RESET\n"));
        break;
    case EV_RXCOMPLETE:
        // data received in ping slot
        Serial.print(F("EV_RXCOMPLETE\n"));
        break;
    case EV_LINK_DEAD:
        Serial.print(F("EV_LINK_DEAD\n"));
        break;
    case EV_LINK_ALIVE:
        Serial.print(F("EV_LINK_ALIVE\n"));
        break;
    /*
    || This event is defined but not used in the code. No
    || point in wasting codespace on it.
    ||
    || case EV_SCAN_FOUND:
    ||    Serial.println(F("EV_SCAN_FOUND"));
    ||    break;
    */
    default:
        Serial.print(F("Unknown event: "));
        // Serial.println((unsigned)ev);
        break;
    }
}

// ======================================================================================

bool MuonPiLMIC::setup(u4_t netid, devaddr_t devaddr, unsigned char *appskey, unsigned char *nwkskey)
{
    os_init(); // LMIC init

    Serial.println(F("Starting"));
    LMIC_reset(); // Reset the MAC state. Session and pending data transfers will be discarded.

    // network ID 0x01 = Expiremental
    // network ID 0x13 = The Things Network
    LMIC_setSession(0x13, devaddr, nwkskey, appskey);

    LMIC_setupChannel(0, 868100000, DR_RANGE_MAP(DR_SF12, DR_SF7), BAND_CENTI);  // g-band
    LMIC_setupChannel(1, 868300000, DR_RANGE_MAP(DR_SF12, DR_SF7B), BAND_CENTI); // g-band
    LMIC_setupChannel(2, 868500000, DR_RANGE_MAP(DR_SF12, DR_SF7), BAND_CENTI);  // g-band
    LMIC_setupChannel(3, 867100000, DR_RANGE_MAP(DR_SF12, DR_SF7), BAND_CENTI);  // g-band
    LMIC_setupChannel(4, 867300000, DR_RANGE_MAP(DR_SF12, DR_SF7), BAND_CENTI);  // g-band
    LMIC_setupChannel(5, 867500000, DR_RANGE_MAP(DR_SF12, DR_SF7), BAND_CENTI);  // g-band
    LMIC_setupChannel(6, 867700000, DR_RANGE_MAP(DR_SF12, DR_SF7), BAND_CENTI);  // g-band
    LMIC_setupChannel(7, 867900000, DR_RANGE_MAP(DR_SF12, DR_SF7), BAND_CENTI);  // g-band
    LMIC_setupChannel(8, 868800000, DR_RANGE_MAP(DR_FSK, DR_FSK), BAND_MILLI);   // g2-band

    // Disable link check validation
    LMIC_setLinkCheckMode(0);

    // TTN uses SF9 for its RX2 window.
    LMIC.dn2Dr = DR_SF9;

    // Set data rate and transmit power for uplink (note: txpow seems to be ignored by the library)
    LMIC_setDrTxpow(DR_SF12, 20);

    LMIC_setAdrMode(false);

    LMIC_registerEventCb(&onEvent, nullptr);
    return true;
}

// ======================================================================================

bool MuonPiLMIC::do_send(osjob_t *workjob)
{
    // Check if there is not a current TX/RX job running
    if (LMIC.opmode & OP_TXRXPEND)
    {
        Serial.println(F("OP_TXRXPEND, not sending"));
        return false;
    }
    else
    {
        // Prepare upstream data transmission at the next possible time.
        LMIC_setTxData2(1, data, sizeof(data) - 1, 0);
        Serial.println(F("Packet queued"));
    }
    // Next TX is scheduled after TX_COMPLETE event.
    return true;
}

bool MuonPiLMIC::sendLoraPayload(uint8_t port, uint8_t *message)
{
    Serial.flush();
    data = message;
    uplinkSequenceNo = uplinkSequenceNo + 1;
    LMIC.seqnoUp = uplinkSequenceNo;

    return do_send(&sendjob);
}