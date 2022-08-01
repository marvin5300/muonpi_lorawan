/*******************************************************************************
 * AmbaSat-1
 * Filename: AmbaSatLMIC.h
 *
 * This library is designed for use with AmbaSat-1 and is a wrapper for
 * IBM LMIC functionality
 *
 * Copyright (c) 2021 AmbaSat Ltd
 * https://ambasat.com
 *
 * licensed under Creative Commons Attribution-ShareAlike 3.0
 * ******************************************************************************/

#ifndef __MuonPiLMIC__
#define __MuonPiLMIC__

#include "serialhandler.h"
#include <Arduino.h>
#include <Wire.h>
#include <lmic.h>
#include <hal/hal.h>

#define LMIC_CLOCK_ERROR_PPM 30000

class MuonPiLMIC
{
public:
    bool setup(devaddr_t devaddr, unsigned char *appskey, unsigned char *nwkskey, SerialHandler *f_serial_handler = nullptr);
    void sendLoraPayload(uint8_t port, String &message); // port can be chosen at will
    static void do_send(osjob_t *sendjob);
    static void onEvent(void *pUserData, ev_t ev);

private:
    static SerialHandler *m_serial_handler;
    static String data;
};

#ifdef __cplusplus
extern "C"
{
#endif

#ifdef __cplusplus
} // extern "C"
#endif

#endif // __MuonPiLMIC__