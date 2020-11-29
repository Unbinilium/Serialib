/*
 * @name: serialib.hpp
 * @namespace: sl
 * @class: serialib
 * @brief: Initilize Unix/Linux tty serial for high level communicating with serial devices
 * @author Unbinilium
 * @version 1.1.4
 * @date 2020-10-17
 */

#ifndef SERIAL_LIB
#define SERIAL_LIB

/* SERIALIB DEFINE

 LOG_LEVEL:
 0 - no output
 1 - only status and error
 2 - notify
 3 - send and read details
 */
#define LOG_LEVEL 3

#include <iostream>

#include <vector>
#include <string>

#include <thread>
#include <mutex>

namespace _c_std
{
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
} // namespace _c_std

namespace _c_sys
{
#include <sys/ioctl.h>
} // namespace _c_sys

namespace sl
{
    // Define serialib class
    class serialib
    {
    protected:
        mutable std::mutex send_lk;
        mutable std::mutex read_lk;

        int fd;
        int status;
        mutable size_t avail;

    private:
        mutable char ch;
        mutable size_t char_read;
        mutable bool if_ch_end;
        size_t str_size;
        mutable size_t ch_end_cur;

        mutable std::chrono::time_point<std::chrono::high_resolution_clock> read_previous_time;
        mutable std::chrono::time_point<std::chrono::high_resolution_clock> read_current_time;

    public:
        const char *device;
        const size_t *baudrates;

        serialib();
        ~serialib();

        bool open(const char *dev, const size_t &bauds);
        bool is_open(void);
        bool close(void);
        bool send(const std::vector<char> &str);
        size_t read_avail(void);
        size_t read(std::vector<char> &str, const std::vector<char> &end, const size_t &length, const size_t &timeout);
        bool flush(void);
    };

    // Init serialib
    serialib::serialib()
    {
        send_lk.unlock();
        read_lk.unlock();

        fd = 0;
        status = 0;
        device = NULL;
        baudrates = NULL;
        avail = 0;
        ch = '\n';
        char_read = 0;
        if_ch_end = false;
        str_size = std::numeric_limits<size_t>::max() - 1;
        ch_end_cur = 0;

        read_previous_time = std::chrono::high_resolution_clock::now();
        read_current_time = std::chrono::high_resolution_clock::now();
    }

    // Destruct serialib
    serialib::~serialib()
    {
        serialib::close();

        send_lk.~mutex();
        read_lk.~mutex();
    }

    /*
     @brief: Open serial port on tty devices with specialized baudrates
     @param: dev - tty device path
     @param: bauds - baudrates
     @return: bool - whether serial port is opend
     */
    bool serialib::open(const char *dev, const size_t &bauds)
    {
        if (serialib::is_open() != false)
        {
#if (LOG_LEVEL > 1)
            std::cout << "Serialib -> " << fd << ", serial'" << dev << "' already opened" << std::endl;
#endif
            return true;
        }

        device = dev;
        baudrates = &bauds;

        /* Config open options
         - O_RDWR      open for reading and writing
         - O_NOCTTY    don't assign controlling terminal
         - O_NONBLOCK  no delay
         - O_ASYNC     signal pgrp when data ready
         */
        fd = _c_std::open(device, (O_RDWR | O_NOCTTY | O_NONBLOCK | O_ASYNC));
        if (fd <= 0)
        {
#if (LOG_LEVEL != 0)
            std::cout << "Serialib -> " << fd << ", open '" << dev << "' failed" << std::endl;
#endif
            return false;
        }

        /* Config fcntl options
         - F_SETFL  set file status flags
         - O_RDWR   open for reading and writing
         */
        _c_std::fcntl(fd, F_SETFL, O_RDWR);

        struct _c_std::termios opt;

        // Get current serial port options
        tcgetattr(fd, &opt);
        cfmakeraw(&opt);

        // Set input and output baudrates
        cfsetispeed(&opt, *baudrates);
        cfsetospeed(&opt, *baudrates);

        /* Config serial port options

         Special Control Characters:
         - VTIME
         - VMIN

         Hardware control of terminal:
         - CS8       character size mask as 8 bits
         - CLOCAL    ignore modem status lines
         - CREAD     enable receiver
         - CSIZE     character size mask
         - CSTOPB    send 2 stop bits
         - PARENB    parity enable

         Software input processing:
         - INPCK     enable checking of parity errors
         - ICRNL     map CR to NL (ala CRMOD)
         - IXOFF     enable input flow control
         - IUTF8     maintain state for UTF-8 VERASE

         Software output processing:
         - OPOST     enable following output processing

         Dumping ground for other state:
         - ECHO      enable echoing
         - ECHOE     visually erase chars
         - ISIG      enable signals INTR, QUIT
         - ICANON    canonicalize input lines
         */
        opt.c_cc[VTIME] = 0;
        opt.c_cc[VMIN] = 0;
        opt.c_cflag |= (CS8 | CLOCAL | CREAD);
        opt.c_cflag &= ~(CSIZE | CSTOPB | PARENB);
        opt.c_iflag |= (INPCK | ICRNL | IXOFF | IUTF8);
        opt.c_oflag &= ~(OPOST);
        opt.c_lflag &= ~(ECHO | ECHOE | ISIG | ICANON);

        // Set serial port options
        tcsetattr(fd, TCSANOW, &opt);

        // Get current serial port status
        _c_sys::ioctl(fd, TIOCMGET, &status);

        /* Compatability with old terminal driver:
         - TIOCM_DTR  data terminal ready
         - TIOCM_RTS  request to send
         */
        status |= TIOCM_DTR;
        status |= TIOCM_RTS;

        // Set current serial port status
        _c_sys::ioctl(fd, TIOCMSET, &status);

        serialib::flush();

#if (LOG_LEVEL != 0)
        std::cout << "Serialib -> " << fd << ", open '" << dev << "' success" << std::endl;
#endif
        return true;
    }

    /*
     @brief: Get current serial port status
     @return: bool - whether serial port is opend
     */
    bool serialib::is_open(void) { return (fd <= 0 ? false : true); }

    /*
     @brief: Close serial port
     @return: bool - whether the operation is finished
     */
    bool serialib::close(void)
    {
        if (serialib::is_open() == false)
        {
#if (LOG_LEVEL > 1)
            std::cout << "Serialib -> " << fd << ", serial'" << device << "' already closed" << std::endl;
#endif
            return true;
        }

        fd = _c_std::close(fd);
        if (fd != 0)
        {
#if (LOG_LEVEL != 0)
            std::cout << "Serialib -> " << fd << ", close '" << device << "' failed" << std::endl;
#endif
            return false;
        }
        else
        {
#if (LOG_LEVEL != 0)
            std::cout << "Serialib -> " << fd << ", close '" << device << "' success" << std::endl;
#endif
            return true;
        }
    }

    /*
     @brief: Send string(chars) using serial port
     @param: str - char(s) stored in vector to send
     @return: bool - whether all the date is sent
     */
    bool serialib::send(const std::vector<char> &str)
    {
        const std::lock_guard<std::mutex> send_gd(send_lk);

#if (LOG_LEVEL > 2)
        std::cout << "Serialib -> " << fd << ", send >> ";
#endif
        for (const char &_str : str)
        {
            if (_c_std::write(fd, &_str, 1) != -1)
            {
#if (LOG_LEVEL > 2)
                std::cout << (_str != '\n' ? _str : ' ');
#endif
            }
            else
            {
#if (LOG_LEVEL != 0)
                std::cout << (_str != '\n' ? _str : ' ') << '~' << std::endl;
#endif
                return false;
            }
        }
#if (LOG_LEVEL > 2)
        std::cout << std::endl;
#endif
        return true;
    }

    /*
     @brief: Get how many char(s) can be read in buffer
     @return: long int - char(s) count
     */
    size_t serialib::read_avail(void)
    {
        _c_sys::ioctl(fd, FIONREAD, &avail);
        return avail;
    }

    /*
     @brief: Read string(chars) from buffer
     @param: str - char(s) stored in vector to read
     @param: end - char(s0 stored in vector, set as endpoint to stop reading
     @param: length - the number limit of char(s) to read
     @param: timeout_ms - the time limit while reading
     @return: unsigned long int - the count of readed char(s)
     */
    size_t serialib::read(std::vector<char> &str, const std::vector<char> &end, const size_t &length, const size_t &timeout_ms)
    {
        const std::lock_guard<std::mutex> read_gd(read_lk);

        char_read = 0;
        str_size = (length != 0 ? length : str_size);
        if_ch_end = (end.size() != 0 ? true : false);
        ch_end_cur = 0;

#if (LOG_LEVEL > 2)
        std::cout << "Serialib -> " << fd << ", read << ";
#endif

        // If timeout_ms equals 0, read all char(s) in buffer with time limit, else without the time limit
        if (timeout_ms != 0)
        {
            // Update read_previous_time to current time
            read_previous_time = std::chrono::high_resolution_clock::now();

            // While char(s) in buffer and readed char(s) count less than the number limit of char(s)
            while (serialib::read_avail() > 0 || char_read != str_size)
            {
                // Update read_current_time to current time
                read_current_time = std::chrono::high_resolution_clock::now();

                // If timeout_ms subtract duration time larger than 0, else stop the reading operation
                if (timeout_ms - std::chrono::duration_cast<std::chrono::milliseconds>(read_current_time - read_previous_time).count() > 0)
                {
                    // Read 1 char from buffer
                    if (_c_std::read(fd, &ch, 1) == 1)
                    {
                        // Store readed char to str
                        str.push_back(ch);
#if (LOG_LEVEL > 2)
                        std::cout << (ch != '\n' ? ch : ' ');
#endif
                        // Self-add char_read
                        char_read++;

                        // If has set str stop point
                        if (if_ch_end)
                        {
                            // If current readed char equals to current end char, check next, else the next not, reset ch_end_cur
                            if (ch == end[ch_end_cur])
                            {
                                // Self-add to check next one
                                ch_end_cur++;

                                // If meet all the end char(s), finish read
                                if (ch_end_cur == end.size())
                                {
                                    break;
                                }
                            }
                            else
                            {
                                // Reset ch_end_cur
                                ch_end_cur = 0;
                            }
                        }
                    }
                }
                else
                {
                    break;
                }
            }
        }
        else
        {
            while (serialib::read_avail() > 0 && char_read != str_size)
            {
                if (_c_std::read(fd, &ch, 1) == 1)
                {
                    str.push_back(ch);
#if (LOG_LEVEL > 2)
                    std::cout << (ch != '\n' ? ch : ' ');
#endif
                    char_read++;

                    if (if_ch_end)
                    {
                        if (ch == end[ch_end_cur])
                        {
                            ch_end_cur++;
                            if (ch_end_cur == end.size())
                            {
                                break;
                            }
                        }
                        else
                        {
                            ch_end_cur = 0;
                        }
                    }
                }
            }
        }
#if (LOG_LEVEL > 2)
        std::cout << std::endl;
#endif
        return char_read;
    }

    /*
     @brief: Flush serial buffer
     @return: bool - whether the operation is finished
     */
    bool serialib::flush(void)
    {
        const std::lock_guard<std::mutex> send_gd(send_lk);
        const std::lock_guard<std::mutex> read_gd(read_lk);

        if (_c_std::tcflush(fd, TCIOFLUSH) != 0)
        {
            return true;
        }
        else
        {
            return false;
        }
    }

} // namespace sl

#endif
