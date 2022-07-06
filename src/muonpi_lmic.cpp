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

bool joined = false;
bool sleeping = false;

//static osjob_t sendjob;
static osjob_t initjob;

uint32_t uplinkSequenceNo = 0; // aka FCnt

// arduino lmic pin mapping
const lmic_pinmap lmic_pins = {
  .nss = 10,
  .rxtx = LMIC_UNUSED_PIN,
  .rst = 7,
  .dio = { /*dio0*/ 8, /*dio1*/ 9, /*dio2*/ LMIC_UNUSED_PIN }
};

// ======================================================================================

void initfunc(osjob_t* j)
{
    // reset MAC state
    LMIC_reset();
    LMIC_setClockError(MAX_CLOCK_ERROR * 5 / 100);

    // init done - onEvent() callback will be invoked...
    Serial.print(F("LMIC initfunc complete\n"));
}

// These callbacks are only used in over-the-air activation, so they are
// left empty here (cannot be left out completely unless
// DISABLE_JOIN is set in config.h, otherwise the linker will complain).
void os_getArtEui (u1_t* buf) { }
void os_getDevEui (u1_t* buf) { }
void os_getDevKey (u1_t* buf) { }

// =========================================================================================================================================
// onEvent
// =========================================================================================================================================
void onEvent (ev_t ev)
{
    int i;

    switch (ev)
    {        
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
            // Disable link check validation (automatically enabled
            // during join, but not supported by TTN at this time).
            LMIC_setLinkCheckMode(0);

            // after Joining a job with the values will be sent.
            joined = true;
        break;
        case EV_RFU1:
            Serial.print(F("EV_RFU1\n"));
        break;
        case EV_JOIN_FAILED:
            Serial.print(F("EV_JOIN_FAILED\n"));
        break;
        case EV_REJOIN_FAILED:
            Serial.print(F("EV_REJOIN_FAILED\n"));
            // Re-init
            os_setCallback(&initjob, initfunc);
        break;
        case EV_TXCOMPLETE:
            sleeping = true;

            if (LMIC.dataLen)
            {
                // data received in rx slot after tx
                // if any data received, a LED will blink
                // this number of times, with a maximum of 10
                Serial.print(F("Data Received: \n"));
                Serial.print(LMIC.frame[LMIC.dataBeg]+"\n");

                i=(LMIC.frame[LMIC.dataBeg]);
                // i (0..255) can be used as data for any other application
                // like controlling a relay, showing a display message etc.
                if (i>10)
                {
                    i=10;     // maximum number of BLINKs
                }

            }

            Serial.print(F("EV_TXCOMPLETE (includes waiting for RX windows)\n"));
            delay(50);  // delay to complete Serial Output before Sleeping

            // Schedule next transmission
            // next transmission will take place after next wake-up cycle in main loop
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
        default:
            Serial.print(F("Unknown event\n"));
        break;
    }
}

// ======================================================================================

bool MuonPiLMIC::setup(u4_t netid, devaddr_t devaddr, unsigned char *appskey, unsigned char *nwkskey)
{    
    os_init();  // LMIC init

    LMIC_reset(); // Reset the MAC state. Session and pending data transfers will be discarded.

    // network ID 0x01 = Expiremental
    // network ID 0x13 = The Things Network    
    LMIC_setSession(0x13, devaddr, nwkskey, appskey);

    #if defined(CFG_eu868)
        // Set up the channels used by the Things Network, which corresponds
        // to the defaults of most gateways. Without this, only three base
        // channels from the LoRaWAN specification are used, which certainly
        // works, so it is good for debugging, but can overload those
        // frequencies, so be sure to configure the full frequency range of
        // your network here (unless your network autoconfigures them).
        // Setting up channels should happen after LMIC_setSession, as that
        // configures the minimal channel set.
        // NA-US channels 0-71 are configured automatically
        LMIC_setupChannel(0, 868100000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
        LMIC_setupChannel(1, 868300000, DR_RANGE_MAP(DR_SF12, DR_SF7B), BAND_CENTI);      // g-band
        LMIC_setupChannel(2, 868500000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
        LMIC_setupChannel(3, 867100000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
        LMIC_setupChannel(4, 867300000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
        LMIC_setupChannel(5, 867500000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
        LMIC_setupChannel(6, 867700000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
        LMIC_setupChannel(7, 867900000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
        LMIC_setupChannel(8, 868800000, DR_RANGE_MAP(DR_FSK,  DR_FSK),  BAND_MILLI);      // g2-band

        // TTN defines an additional channel at 869.525Mhz using SF9 for class B
        // devices' ping slots. LMIC does not have an easy way to set this
        // frequency and support for class B is spotty and untested, so this
        // frequency is not configured here.
    #elif defined(CFG_us915)
        // NA-US channels 0-71 are configured automatically
        // but only one group of 8 should (a subband) should be active
        // TTN recommends the second sub band, 1 in a zero based count.
        // https://github.com/TheThingsNetwork/gateway-conf/blob/master/US-global_conf.json        
        LMIC_selectSubBand(1);
    #elif defined(CFG_other)
        LMIC_setupChannel(0, 923200000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
        LMIC_setupChannel(1, 923400000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
        LMIC_setupChannel(2, 923600000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
        LMIC_setupChannel(3, 923800000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
        LMIC_setupChannel(4, 924000000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
        LMIC_setupChannel(5, 924200000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
        LMIC_setupChannel(6, 924400000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
        LMIC_setupChannel(7, 924600000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
        LMIC_setupChannel(8, 924800000, DR_RANGE_MAP(DR_FSK,  DR_FSK),  BAND_MILLI);      // g2-band
    #endif

    // Disable link check validation
    LMIC_setLinkCheckMode(0);

    // TTN uses SF9 for its RX2 window.
    LMIC.dn2Dr = DR_SF9;

    // Set data rate and transmit power for uplink (note: txpow seems to be ignored by the library)
    LMIC_setDrTxpow(DR_SF12, 20);

    // os_radio(OP_TXDATA);
    // LMIC.opmode = OP_TXDATA;
    return true;
}

// ======================================================================================

bool MuonPiLMIC::sendLoraPayload(uint8_t port, uint8_t* message)
{
    Serial.flush();
    uplinkSequenceNo = uplinkSequenceNo + 1;
    LMIC.seqnoUp = uplinkSequenceNo;

    sleeping = false;

    LMIC_setTxData2(port, message, sizeof(message), 0);

    while(sleeping == false)
    {
        os_runloop_once();
    }
    delay(50);
    return true;
}