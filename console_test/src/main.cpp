#include "../include/main.h"
#include "../include/serial.h"

int main(){
    constexpr int verbosity{1};
    constexpr int baud_rate{115200};
    serial ser(verbosity);
    auto initialized = ser.init(baud_rate);
    if (initialized){
        std::string test{"test"};
        std::string data{};
        // ser.send(""); // does not get read back (who knows why)
        data = ser.receive();
        ser.send(test);
        while (data.empty())
        {
            data = ser.receive();
            if (!data.empty()) {
                std::cout << "received: " << data << "\n" << std::flush;
            }
        }
        data.clear();
        ser.send("another one");
        while (data.empty())
        {
            data = ser.receive();
            if (!data.empty())
            {
                std::cout << "received: " << data << "\n"
                          << std::flush;
            }
        }
    }else{
        std::cout << "problem at initializing serial" << std::endl;
    }
}