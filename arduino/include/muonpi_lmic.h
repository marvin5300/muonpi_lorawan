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

#include <Arduino.h>
#include <Wire.h>
#include <lmic.h>
#include <hal/hal.h>

class MuonPiLMIC
{
public:
    bool setup(u4_t netid, devaddr_t devaddr, unsigned char *appskey, unsigned char *nwkskey);
    bool sendLoraPayload(uint8_t port, uint8_t *message); // port can be chosen at will
    bool do_send(osjob_t *sendjob);
    static void onEvent(void *pUserData, ev_t ev);


private:
};

#ifdef __cplusplus
extern "C"
{
#endif

#ifdef __cplusplus
} // extern "C"
#endif

#endif // __MuonPiLMIC__