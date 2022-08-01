#include "../include/main.h"
#include "../include/serial.h"

std::string listen(serial &ser)
{
    std::string rx_data{};
    while (rx_data.empty())
    {
        rx_data = ser.receive();
    }
    return rx_data;
}

int main(){
    constexpr int verbosity{0};
    constexpr int baud_rate{115200};
    serial ser(verbosity);
    auto initialized = ser.init(baud_rate);
    if (initialized)
    {
        // ser.send("reset");
        // ser.send("testmessage");
        while(1){
            auto msg = listen(ser);
            std::cout << msg << "\n" << std::flush;
            if (msg == "Starting" || msg == "EV_TXCOMPLETE"){
                ser.send("testmessage");
            }
        }
    }
    else
    {
        std::cout << "problem at initializing serial" << std::endl;
    }
}