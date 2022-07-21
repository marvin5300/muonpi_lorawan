#ifndef SERIAL_H
#define SERIAL_H

#include <string>
#include <iostream>

class serial{
public:
    serial(int f_verbosity = 0);
    ~serial();
    auto init(const unsigned baud_rate = 9600) -> bool;
    auto send(const std::string &data) const -> bool;
    auto receive() -> std::string;
private:
    static void fletcherChkSum(const std::string& str, uint8_t& chkA, uint8_t& chkB);
    int serial_port{0};
    int m_verbosity;
    std::string buf{};
};

#endif // SERIAL_H