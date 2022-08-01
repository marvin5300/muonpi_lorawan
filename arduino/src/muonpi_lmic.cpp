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
#include "serialhandler.h"
#include <Arduino.h>
#include <lmic.h>
#include <hal/hal.h>
// #include <SPI.h>

bool joined = false;
bool sleeping = false;

// static osjob_t sendjob;
static osjob_t sendjob;

uint32_t uplinkSequenceNo = 0; // aka FCnt

String MuonPiLMIC::data{};

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

SerialHandler *MuonPiLMIC::m_serial_handler{nullptr};

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
        m_serial_handler->send(String(os_getTime() + String(": EV_TXSTART")));
        break;
    case EV_JOIN_TXCOMPLETE:
        m_serial_handler->send(F("EV_JOIN_TXCOMPLETE"));
    case EV_TXCANCELED:
        m_serial_handler->send(F("EV_TXCANCELLED"));
        break;
    case EV_SCAN_TIMEOUT:
        m_serial_handler->send(F("EV_SCAN_TIMEOUT"));
        break;
    case EV_BEACON_FOUND:
        m_serial_handler->send(F("EV_BEACON_FOUND"));
        break;
    case EV_BEACON_MISSED:
        m_serial_handler->send(F("EV_BEACON_MISSED"));
        break;
    case EV_BEACON_TRACKED:
        m_serial_handler->send(F("EV_BEACON_TRACKED"));
        break;
    case EV_JOINING:
        m_serial_handler->send(F("EV_JOINING"));
        break;
    case EV_JOINED:
        m_serial_handler->send(F("EV_JOINED"));
        break;
    /*
    || This event is defined but not used in the code. No
    || point in wasting codespace on it.
    ||
    || case EV_RFU1:
    ||     m_serial_handler->send(F("EV_RFU1"));
    ||     break;
    */
    case EV_JOIN_FAILED:
        m_serial_handler->send(F("EV_JOIN_FAILED"));
        break;
    case EV_REJOIN_FAILED:
        m_serial_handler->send(F("EV_REJOIN_FAILED"));
        break;
    case EV_TXCOMPLETE:
        m_serial_handler->send(F("EV_TXCOMPLETE"));
        if (LMIC.txrxFlags & TXRX_ACK)
            m_serial_handler->send(F("Received ack"));
        if (LMIC.dataLen)
        {
            m_serial_handler->send(F("Received "));
            m_serial_handler->send(LMIC.dataLen);
            m_serial_handler->send(F(" bytes of payload"));
        }
        // Schedule next transmission
        // will be called by main loop
        break;
    case EV_LOST_TSYNC:
        m_serial_handler->send(F("EV_LOST_TSYNC\n"));
        break;
    case EV_RESET:
        m_serial_handler->send(F("EV_RESET\n"));
        break;
    case EV_RXCOMPLETE:
        // data received in ping slot
        m_serial_handler->send(F("EV_RXCOMPLETE\n"));
        break;
    case EV_LINK_DEAD:
        m_serial_handler->send(F("EV_LINK_DEAD\n"));
        break;
    case EV_LINK_ALIVE:
        m_serial_handler->send(F("EV_LINK_ALIVE\n"));
        break;
    /*
    || This event is defined but not used in the code. No
    || point in wasting codespace on it.
    ||
    || case EV_SCAN_FOUND:
    ||    m_serial_handler->send(F("EV_SCAN_FOUND"));
    ||    break;
    */
    default:
        m_serial_handler->send(F("Unknown event: "));
        // m_serial_handler->send((unsigned)ev);
        break;
    }
}

// ======================================================================================

bool MuonPiLMIC::setup(devaddr_t devaddr, unsigned char *appskey, unsigned char *nwkskey, SerialHandler *f_serial_handler)
{
    m_serial_handler = f_serial_handler;
    os_init(); // LMIC init

    m_serial_handler->send(F("Starting"));
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
    LMIC_setDrTxpow(static_cast<dr_t>(DR_SF12), static_cast<s1_t>(20));

    LMIC_setAdrMode(false);

    uint32_t clockError = (LMIC_CLOCK_ERROR_PPM / 100) * (MAX_CLOCK_ERROR / 100) / 100;
    LMIC_setClockError(clockError);

    LMIC_registerEventCb(&onEvent, nullptr);
    return true;
}

// ======================================================================================

void MuonPiLMIC::do_send(osjob_t *workjob)
{
    // Check if there is not a current TX/RX job running
    if (LMIC.opmode & OP_TXRXPEND)
    {
        m_serial_handler->send(F("OP_TXRXPEND, not sending"));
        return;
    }
    else
    {
        // Prepare upstream data transmission at the next possible time.
        LMIC_setTxData2(1, data.c_str(), sizeof(data) - 1, 0);
        m_serial_handler->send("Packed queued: " + data);
    }
    // Next TX is scheduled after TX_COMPLETE event.
}

void MuonPiLMIC::sendLoraPayload(uint8_t port, String &message)
{
    data = message;
    uplinkSequenceNo = uplinkSequenceNo + 1;
    LMIC.seqnoUp = uplinkSequenceNo;

    os_setCallback(&sendjob, do_send);
}