#ifndef SERIALHANDLER_H
#define SERIALHANDLER_H

#include <Arduino.h>
#include <stdint.h>

class SerialHandler{
    
public:
    SerialHandler() = default;
    bool read(String &data);
    void fletcherChkSum(const String&  buf, uint8_t& chkA, uint8_t& chkB);
    bool send(String &data);
};

#endif // SERIALHANDLER_H