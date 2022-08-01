#ifndef SERIALHANDLER_H
#define SERIALHANDLER_H

#include <Arduino.h>
#include <stdint.h>
#include <lmic.h>
#include <hal/hal.h>

class SerialHandler
{

public:
    SerialHandler() = default;
    bool read(String &data);
    void fletcherChkSum(const String &buf, uint8_t &chkA, uint8_t &chkB);
    bool send(const char *data);
    bool send(const String &data);

private:
    String buf{};
};

#endif // SERIALHANDLER_H