/*
 * @name: serialib.hpp
 * @namespace: sl
 * @class: serialib
 * @brief: Initilize Unix/Linux tty serial for high level communicating with serial devices
 * @author Unbinilium
 * @version 1.1
 * @date 2020-10-17
 */

#ifndef SERIAL_LIB
#define SERIAL_LIB

/* Define std::cout level
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

#include <cmath>

namespace _c_std
{
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
} // namespace _c_std

namespace _c_sys
{
#include <sys/time.h>
#include <sys/ioctl.h>
} // namespace _c_sys

namespace sl
{
    // Define serialib class
    class serialib
    {
    protected:
        std::mutex send_lk;
        std::mutex read_lk;
        
        int fd;
        
        int status;
        const char *device;
        
        int avail;
        
    private:
        char ch;
        
        unsigned long int char_read;
        bool if_ch_end;
        unsigned long int str_size;
        unsigned long int ch_end_cur;
        
        struct timeval previous_time;
        struct timeval current_time;
        unsigned long int sec;
        unsigned long int usec;
        
        unsigned long int time_used(void);
        
    public:
        serialib();
        ~serialib();
        
        bool open(const char *dev, const unsigned long int bauds);
        bool is_open(void);
        bool close(void);
        bool send(const std::vector<char> &str);
        long int read_avail(void);
        unsigned long int read(std::vector<char> &str, const std::vector<char> &end, const unsigned long int length, const unsigned long int timeout);
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
        
        avail = 0;
        
        ch = '\0';
        
        char_read = 0;
        if_ch_end = false;
        str_size = std::pow(2, sizeof(int) * 8) - 1;
        ch_end_cur = 0;
        
        sec = 0;
        usec = 0;
    }
    
    // Destruct serialib
    serialib::~serialib()
    {
        serialib::close();
        
        send_lk.~mutex();
        read_lk.~mutex();
    }
    
    /*
     @brief: Used for determine time used in opreation (private)
     @param: previous_time - previous time in class
     @param: current_time - surrent time in class
     @return: unsigned long int - used time in ms
     */
    unsigned long int serialib::time_used(void)
    {
        sec = current_time.tv_sec - previous_time.tv_sec;
        usec = current_time.tv_usec - previous_time.tv_usec;
        
        if (usec <= 0)
        {
            usec = 1e6 - previous_time.tv_usec + current_time.tv_usec;
            sec--;
        }
        
        return sec * 1e3 + usec / 1e3;
    }
    
    /*
     @brief: Open serial port on tty devices with specialized baudrates
     @param: dev - tty device path
     @param: bauds - baudrates
     @return: bool - whether serial port is opend
     */
    bool serialib::open(const char *dev, const unsigned long int bauds)
    {
        if (serialib::is_open() != false)
        {
#if (LOG_LEVEL > 1)
            std::cout << "Serialib -> " << fd << ", serial'" << dev << "' already opened" << std::endl;
#endif
            return true;
        }
        
        device = dev;
        
        // Open serial port
        fd = _c_std::open(dev, (O_RDWR | O_NOCTTY | O_NDELAY | O_NONBLOCK));
        if (fd <= 0)
        {
#if (LOG_LEVEL != 0)
            std::cout << "Serialib -> " << fd << ", open '" << dev << "' failed" << std::endl;
#endif
            return false;
        }
        
        _c_std::fcntl(fd, F_SETFL, O_RDWR);
        
        struct _c_std::termios opt;
        
        // Get current serial port options
        tcgetattr(fd, &opt);
        cfmakeraw(&opt);
        
        // Set input and output baudrates
        cfsetispeed(&opt, bauds);
        cfsetospeed(&opt, bauds);
        
        /* Config serial port options
         
         Hardware control of terminal:
         - CLOCAL    ignore modem status lines
         - CREAD     enable receiver
         - CS8       character size mask as 8 bits
         
         Software input processing:
         - IGNBRK    ignore BREAK condition
         - IGNPAR    ignore (discard) parity errors
         
         Dumping ground for other state:
         - ECHOE     visually erase chars
         - ECHO      enable echoing
         - ISIG      enable signals INTR, QUIT
         - ICANON    canonicalize input lines
         */
        opt.c_cflag |= (CLOCAL | CREAD | CS8);
        opt.c_iflag |= (IGNBRK | IGNPAR);
        opt.c_lflag |= (ICANON | ECHO | ECHOE | ISIG);
        opt.c_cc[VTIME] = 0;
        opt.c_cc[VMIN] = 0;
        
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
    bool serialib::is_open(void)
    {
        return (fd <= 0 ? false : true);
    }
    
    /*
     @brief: Close serial port
     @return: bool - whether the operation is finished
     */
    bool serialib::close(void)
    {
        const std::lock_guard<std::mutex> send_gd(send_lk);
        const std::lock_guard<std::mutex> read_gd(read_lk);
        
        if (serialib::is_open() != false)
        {
#if (LOG_LEVEL > 1)
            std::cout << "Serialib -> " << fd << ", serial'" << device << "' already closed" << std::endl;
#endif
            return true;
        }
        
        fd = _c_std::close(fd);
#if (LOG_LEVEL != 0)
        std::cout << "Serialib -> " << fd << ", close '" << device << "' success" << std::endl;
#endif
        return true;
    }
    
    /*
     @brief: Send string(chars) using serial port
     @param: str - char(s) stored in vector to send
     @return: bool - whether all the date is sent
     */
    bool serialib::send(const std::vector<char> &str)
    {
        const std::lock_guard<std::mutex> send_gd(send_lk);
        
        if (serialib::is_open() != true)
        {
#if (LOG_LEVEL != 0)
            std::cout << "Serialib -> " << fd << ", serial'" << device << "' not opened" << std::endl;
#endif
            return false;
        }
        
#if (LOG_LEVEL > 2)
        std::cout << "Serialib -> " << fd << ", send >> ";
#endif
        for (const char &_str : str)
        {
            if (_c_std::write(fd, &_str, 1) == -1)
            {
#if (LOG_LEVEL != 0)
                std::cout << "Serialib -> " << fd << ", failed to write '" << _str << "'" << std::endl;
#endif
            }
#if (LOG_LEVEL > 2)
            std::cout << _str;
#endif
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
    long int serialib::read_avail(void)
    {
        const std::lock_guard<std::mutex> read_gd(read_lk);
        
        if (serialib::is_open() != true)
        {
#if (LOG_LEVEL != 0)
            std::cout << "Serialib -> " << fd << ", serial'" << device << "' not opened" << std::endl;
#endif
            return false;
        }
        
        if (_c_sys::ioctl(fd, FIONREAD, &avail) == -1)
        {
#if (LOG_LEVEL != 0)
            std::cout << "Serialib -> " << fd << ", avaliable to read cannot be determined on '" << device << "'" << std::endl;
#endif
            return -1;
        }
        
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
    unsigned long int serialib::read(std::vector<char> &str, const std::vector<char> &end, const unsigned long int length, const unsigned long int timeout_ms)
    {
        const std::lock_guard<std::mutex> read_gd(read_lk);
        
        if (serialib::is_open() != true)
        {
#if (LOG_LEVEL != 0)
            std::cout << "Serialib -> " << fd << ", serial'" << device << "' not opened" << std::endl;
#endif
            return 0;
        }
        
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
            // Update previoust_time to current time
            _c_sys::gettimeofday(&previous_time, NULL);
            
            // While char(s) in buffer and readed char(s) count less than the number limit of char(s)
            while (serialib::read_avail() > 0 && char_read != str_size)
            {
                // Update current_time to current time
                _c_sys::gettimeofday(&current_time, NULL);
                
                // If timeout_ms subtract time_used larger than 0, else stop the reading operation
                if (timeout_ms - serialib::time_used() > 0)
                {
                    // Read 1 char from buffer
                    if (_c_std::read(fd, &ch, 1) == 1)
                    {
                        // Store readed char to str
                        str.push_back(ch);
#if (LOG_LEVEL > 2)
                        std::cout << ch;
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
                                if (ch_end_cur == end.size() - 1)
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
                    std::cout << ch;
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
        if (serialib::is_open() != true)
        {
            const std::lock_guard<std::mutex> read_gd(read_lk);
            
#if (LOG_LEVEL != 0)
            std::cout << "Serialib -> " << fd << ", serial'" << device << "' not opened" << std::endl;
#endif
            return false;
        }
        
        _c_std::tcflush(fd, TCIFLUSH);
        
        return true;
    }
    
} // namespace sl

#endif
