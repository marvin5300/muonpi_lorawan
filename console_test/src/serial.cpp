#include "../include/serial.h"

// C library headers
#include <stdio.h>
// #include <string.h>
#include <string>
#include <cstring>

// Linux headers
#include <fcntl.h> // Contains file controls like O_RDWR
#include <errno.h>   // Error integer and std::strerror() function
// #include <termios.h> // Contains POSIX terminal control definitions

#include <asm/termbits.h>
#include <sys/ioctl.h>
#include <asm/ioctls.h>
#include <unistd.h> // write(), read(), close()
#include <chrono>
#include <thread>

// Just for information about the structure of the termios struct
// struct termios {
// 	tcflag_t c_iflag;		/* input mode flags */
// 	tcflag_t c_oflag;		/* output mode flags */
// 	tcflag_t c_cflag;		/* control mode flags */
// 	tcflag_t c_lflag;		/* local mode flags */
// 	cc_t c_line;			/* line discipline */
// 	cc_t c_cc[NCCS];		/* control characters */
// };
// struct termios2 {
// 	tcflag_t c_iflag;		/* input mode flags */
// 	tcflag_t c_oflag;		/* output mode flags */
// 	tcflag_t c_cflag;		/* control mode flags */
// 	tcflag_t c_lflag;		/* local mode flags */
// 	cc_t c_line;			/* line discipline */
// 	cc_t c_cc[NCCS];		/* control characters */
// 	speed_t c_ispeed;		/* input speed */
// 	speed_t c_ospeed;		/* output speed */
// };
constexpr uint8_t MESSAGE_HEADER = 0xf9u;
constexpr std::size_t buffer_size = 0xff;

serial::serial(int f_verbosity)
    : m_verbosity{f_verbosity} {}

serial::~serial()
{
    if (ioctl(serial_port, TIOCNXCL))
    {
        printf("Error %i from ioctl(TIOCNXCL): %s\n", errno, std::strerror(errno));
    }
    close(serial_port);
}

auto serial::init(const unsigned baud_rate) -> bool
{
    serial_port = open("/dev/ttyACM0", O_RDWR);

    if (serial_port < 0)
    {
        printf("Error %i from open: %s\n", errno, std::strerror(errno));
        return false;
    }

    // exclusive mode
    if (ioctl(serial_port, TIOCEXCL))
    {
        printf("Error %i from ioctl(TIOCEXCL): %s\n", errno, std::strerror(errno));
        return false;
    }

    // Create new termios struct, we call it 'tty' for convention
    // No need for "= {0}" at the end as we'll immediately write the existing
    // config to this struct
    // struct termios tty;
    struct termios2 tty;
    // memset (&tty, 0, sizeof(tty));

    // Read in existing settings, and handle any error
    // NOTE: This is important! POSIX states that the struct passed to tcsetattr()
    // must have been initialized with a call to tcgetattr() overwise behaviour
    // is undefined
    // if (tcgetattr(serial_port, &tty) != 0)
    // {
    //     printf("Error %i from tcgetattr: %s\n", errno, std::strerror(errno));
    //     return false;
    // }

    if (ioctl(serial_port, TCGETS2, &tty) != 0){
        printf("Error %i from tcgetattr: %s\n", errno, std::strerror(errno));
        return false;
    }

    tty.c_cflag &= ~PARENB; // Clear parity bit, disabling parity (most common)
    // tty.c_cflag |= PARENB;  // Set parity bit, enabling parity

    tty.c_cflag &= ~CSTOPB; // Clear stop field, only one stop bit used in communication (most common)
    // tty.c_cflag |= CSTOPB;  // Set stop field, two stop bits used in communication

    tty.c_cflag &= ~CSIZE; // Clear all the size bits, then use one of the statements below
    // tty.c_cflag |= CS5; // 5 bits per byte
    // tty.c_cflag |= CS6; // 6 bits per byte
    // tty.c_cflag |= CS7; // 7 bits per byte
    tty.c_cflag |= CS8; // 8 bits per byte (most common)

    tty.c_cflag &= ~CRTSCTS; // Disable RTS/CTS hardware flow control (most common)
    // tty.c_cflag |= CRTSCTS;  // Enable RTS/CTS hardware flow control

    tty.c_cflag |= CREAD | CLOCAL; // Turn on READ & ignore ctrl lines (CLOCAL = 1)

    // for termios2
    tty.c_cflag &= ~CBAUD;
    tty.c_cflag |= CBAUDEX;

    /*
    UNIX systems provide two basic modes of input, canonical and non-canonical mode.
    In canonical mode, input is processed when a new line character is received.
    The receiving application receives that data line-by-line.
    This is usually undesirable when dealing with a serial port, and so we normally want to disable canonical mode.

    Canonical mode is disabled with:
    */
    tty.c_lflag &= ~ICANON;

    tty.c_lflag &= ~ECHO;   // Disable echo
    tty.c_lflag &= ~ECHOE;  // Disable erasure
    tty.c_lflag &= ~ECHONL; // Disable new-line echo

    tty.c_lflag &= ~ISIG; // Disable interpretation of INTR, QUIT and SUSP

    tty.c_iflag &= ~(IXON | IXOFF | IXANY); // Turn off s/w flow ctrl

    tty.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL); // Disable any special handling of received bytes

    tty.c_oflag = 0;
    tty.c_oflag &= ~OPOST; // Prevent special interpretation of output bytes (e.g. newline chars)
    // tty.c_oflag &= ~ONLCR; // Prevent conversion of newline to carriage return/line feed
    // tty.c_oflag &= ~OXTABS; // Prevent conversion of tabs to spaces (NOT PRESENT IN LINUX)
    // tty.c_oflag &= ~ONOEOT; // Prevent removal of C-d chars (0x004) in output (NOT PRESENT IN LINUX)

    // VMIN: minimum characters received
    // VTIME: timeout
    // 0 means deactivated; read() will block until either VMIN characters received or VTIME deciseconds have passed
    tty.c_cc[VTIME] = 5; // Wait for up to 0.5s (50 deciseconds), returning as soon as any data is received.
    tty.c_cc[VMIN] = 0;

    // UNIX compliant baud rates:
    // B0,  B50,  B75,  B110,  B134,  B150,  B200, B300, B600, B1200, B1800, B2400, B4800, B9600, B19200, B38400, B57600, B115200, B230400, B460800
    // cfsetispeed(&tty, B9600);
    // cfsetospeed(&tty, B9600);

    tty.c_ispeed = baud_rate;
    tty.c_ospeed = baud_rate;

    // Save tty settings, also checking for error
    // if (tcsetattr(serial_port, TCSANOW, &tty) != 0)
    // {
    //     printf("Error %i from tcsetattr: %s\n", errno, std::strerror(errno));
    //     return false;
    // }
    if (ioctl(serial_port, TCSETS2, &tty)) {
        printf("Error %i from tcsetattr: %s\n", errno, std::strerror(errno));
        return false;
    }

    // std::this_thread::sleep_for(std::chrono::milliseconds(2));
    // fcntl(serial_port, F_SETFL, 0);
    // ioctl(serial_port, TCIOFLUSH);
    // std::this_thread::sleep_for(std::chrono::milliseconds(70));
    std::this_thread::sleep_for(std::chrono::milliseconds(1300));

    return true;
}

void serial::fletcherChkSum(const std::string &str, uint8_t &chkA, uint8_t &chkB)
{
    // calc Fletcher checksum, ignore the message header (b5 62)
    chkA = 0;
    chkB = 0;
    for (std::size_t i = 0; i < str.size(); i++)
    {
        chkA += static_cast<uint8_t>(str[i]);
        chkB += chkA;
    }
}

auto serial::send(const std::string &data) const -> bool
{
    if (data.size() > 255)
    {
        return false;
    }
    uint8_t chkA, chkB;
    fletcherChkSum(data, chkA, chkB);
    std::string txBuf{};
    txBuf += static_cast<char>(MESSAGE_HEADER);
    txBuf += static_cast<uint8_t>(data.size());
    txBuf += data;
    txBuf += static_cast<char>(chkA);
    txBuf += static_cast<char>(chkB);
    auto num_bytes = write(serial_port, txBuf.c_str(), txBuf.size());
    if (m_verbosity > 0)
    {
        std::cout << "\nsend " << num_bytes << "bytes of data: '" << data << "' header: " << std::hex << static_cast<unsigned>(MESSAGE_HEADER);
        std::cout << " size: " << data.size() << " chkA: " << static_cast<unsigned>(chkA);
        std::cout << " chkB: " << static_cast<unsigned>(chkB) << std::endl;
    }
    if (num_bytes < 0)
    {
        printf("Error %i from write: %s\n", errno, std::strerror(errno));
        return false;
    }
    return true;
}

auto serial::receive() -> std::string
{
    char rxBuf[buffer_size];
    auto num_bytes = read(serial_port, &rxBuf, buffer_size);
    if (num_bytes < 0){
        printf("Error %i from read: %s\n", errno, std::strerror(errno));
        return "";         
    }
    for (std::size_t i = 0; i < num_bytes; i++)
    {
        buf += rxBuf[i];
    }
    if (m_verbosity > 0)
    {
        std::cout << num_bytes << " bytes read, buf: " << std::endl;
        for (auto c : buf)
        {
            std::cout << std::hex << (static_cast<uint16_t>(c) & 0xff) << "\n";
        }
    }
    std::cout << std::flush;
    if (buf.size() < 4)
    {
        return "";
    }
    for (std::size_t i = 0; i < buf.size(); i++)
    {
        if (static_cast<uint8_t>(buf[i]) == MESSAGE_HEADER && buf.size() >= i + 4)
        { // header, size, data block, chkA, chkB => size >= 5
            uint8_t payload_size = static_cast<uint8_t>(buf[i + 1]);
            if (m_verbosity > 0)
            {
                std::cout << "found header at position " << i << std::endl;
                std::cout << "found payload size to be " << static_cast<int>(payload_size);
                std::cout << " buf size in total: " << buf.size() << ", needed: " << static_cast<int>(payload_size) + 4 << std::endl;
            }
            if (buf.size() >= i + 4 + payload_size)
            {
                std::string str{};
                for (size_t j = i + 2; j < i + 2 + payload_size; j++)
                {
                    str += buf[j];
                }
                uint8_t chkA, chkB;
                fletcherChkSum(str, chkA, chkB);
                if (chkA != static_cast<uint8_t>(buf[i + 2 + payload_size])
                || chkB != static_cast<uint8_t>(buf[i + 3 + payload_size]))
                {
                    return "";
                }
                std::string temp = "";
                for (int k = i + 5 + payload_size; k < buf.size(); k++)
                {
                    temp += buf[k];
                }
                buf = temp;

                // do stuff with the data
                return str;
            }
        }
    }
    return "";
}