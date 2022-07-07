/*******************************************************************************
 * AmbaSat-1
 * Filename: main.cpp
 * AmbaSat-1 Flight Code for Sensor 01 - SHT31: Temperature & Humidity
 * 16th January 2022
 * Authors: Martin Platt, James Vonteb
 *
 * Copyright (c) 2022 AmbaSat Ltd
 * https://ambasat.com
 *
 * To use this code, set NWKSKEY, APPSKEY & DEVADDR values as per your Dashboard
 * See the HowTo: https://ambasat.com/howto/kit-2/#/../unique-ids
 *
 * For ISM band configuration: See lmic/config.h eg. #define CFG_us915 1
 * licensed under Creative Commons Attribution-ShareAlike 3.0
 * ******************************************************************************/

#include "main.h"
#include "muonpi_lmic.h"
#include <Arduino.h>

// TTN *****************************
#define DEVICEID "eui-70b3d57ed0052abe"
#define ABP_DEVICEID "eui-70b3d57ed0052abe"

char *test{"Dies ist ein Test"};

// The Network Session Key / DO NOT SHARE
static const PROGMEM u1_t NWKSKEY[16] = {0xCC, 0xB8, 0xF3, 0xD3, 0xFD, 0x39, 0x75, 0xAE, 0xE4, 0x84, 0x35, 0x90, 0xFE, 0x37, 0x1C, 0x88};

// LoRaWAN AppSKey, application session key / DO NOT SHARE
static const u1_t PROGMEM APPSKEY[16] = {0xA8, 0xDF, 0x3A, 0xC7, 0x51, 0xB2, 0xD1, 0x73, 0xAC, 0x58, 0x81, 0x91, 0xD2, 0x58, 0xCB, 0x4E};

// LoRaWAN end-device address (DevAddr) / DO NOT SHARE
static const u4_t DEVADDR = 0x260BC37E;
/********************************/

osjob_t workjob;

// void(* resetFunc) (void) = 0;  //declare reset function at address 0

int sleepcycles = 1; // 130 X 8 seconds = ~17 mins sleep

// ============================================================================

MuonPiLMIC *muonpi_lmic;

// ============================================================================

void process_work(osjob_t *job)
{
    if (muonpi_lmic != nullptr)
    {
        muonpi_lmic->sendLoraPayload(1u, reinterpret_cast<uint8_t *>(test));
    }
    os_setTimedCallback(&workjob, os_getTime() + sec2osticks(TX_INTERVAL), process_work);
}

void setup()
{
#ifndef SERIAL_BAUD
    Serial.begin(9600);
#else
    Serial.begin(SERIAL_BAUD);
#endif

    while (!Serial)
        delay(10);

    // Create the LMIC object
    muonpi_lmic = new MuonPiLMIC();

    // Set static session parameters. Instead of dynamically establishing a session
    // by joining the network, precomputed session parameters are provided.
    uint8_t appskey[sizeof(APPSKEY)];
    uint8_t nwkskey[sizeof(NWKSKEY)];
    memcpy_P(appskey, APPSKEY, sizeof(APPSKEY));
    memcpy_P(nwkskey, NWKSKEY, sizeof(NWKSKEY));

    // Setup LMIC
    muonpi_lmic->setup(0x13, DEVADDR, appskey, nwkskey);
    process_work(&workjob);
}

// ============================================================================

void loop()
{
    os_runloop_once();
}

// ============================================================================
