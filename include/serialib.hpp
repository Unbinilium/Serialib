/*
 * @name: serialib.hpp
 * @namespace: sl
 * @class: Serialib
 * @brief: Initilize Unix/Linux tty serial for high level communicating with serial devices
 * @author Unbinilium
 * @version 1.3.1
 * @date 2020-10-17
 */

#ifndef SERIALIB_SERIALIB_HPP_
#define SERIALIB_SERIALIB_HPP_

#include <iostream>

#include <mutex>
#include <atomic>
#include <thread>

#include <vector>

#include <cstring>

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
    // Define Serialib class
    class Serialib
    {
    protected:
        mutable std::recursive_mutex send_lk;
        mutable std::mutex           read_lk;
        mutable std::mutex           term_lk;
        
        int                          fd     { 0 };
        struct  _c_std::termios      opt;
        int                          status { 0 };
        
    private:
        const char                   *p_str     { nullptr };
        mutable size_t               avail      { 0 };
        mutable char                 ch         { '\n' };
        mutable size_t               char_read  { 0 };
        mutable bool                 if_ch_end  { false };
        mutable size_t               str_size   { std::numeric_limits<size_t>::max() - 1 };
        mutable size_t               ch_end_idx { 0 };
        
        mutable std::chrono::time_point<std::chrono::steady_clock> read_previous_time;
        mutable std::chrono::time_point<std::chrono::steady_clock> read_current_time;
        
    public:
        const char                   *device    { nullptr };
        const size_t                 *baudrates { nullptr };
        
        Serialib (void);
        Serialib (const char *m_dev, const size_t &m_baud);
        ~Serialib(void);
        
        inline Serialib &operator= (const class Serialib &rhs);
        inline Serialib &operator()(const char *rhs_d, const size_t &rhs_b);
        
        inline bool operator<<(const char              *rhs) const;
        inline bool operator<<(const std::vector<char> &rhs);
        inline bool operator>>(std::vector<char>       &rhs);
        
        inline operator size_t();
        
        bool is_open(void);
        bool open   (const char *dev, const size_t &bauds);
        bool close  (void);
        bool flush  (void);
        
        inline bool   send      (const std::vector<char> &str);
        inline size_t read_avail(void);
        inline size_t read      (std::vector<char>       &str,
                                 const std::vector<char> &end,
                                 const size_t            &length,
                                 const double            &timeout_us
        );
        
        inline void async_send  (std::vector<char>       &str,
                                 double                  &duration_us,
                                 std::mutex              &args_lk,
                                 std::atomic<bool>       &thr_keep
        );
        inline void async_read  (std::vector<char>       &str,
                                 std::vector<char>       &end,
                                 size_t                  &length,
                                 double                  &duration_us,
                                 std::mutex              &args_lk,
                                 std::atomic<bool>       &thr_keep
        );
        
        void terminal           (void);
    };
    
    /*
     @brief: Operator << for std::ostream support
     @param:  lfs          - std::ostream
     @param:  rhs          - Serialib
     @return: std::ostream - lfs
     */
    std::ostream &operator<<(std::ostream &lfs, class Serialib &rhs)
    {
        std::vector<char> rhs_v;
        rhs >> rhs_v;
        for (char &_rhs_v : rhs_v) { lfs << _rhs_v; }
        
        return lfs;
    }
    
    // Init Serialib
    Serialib::Serialib(void) {}
    
    /*
     @brief: Init Serialib with device name and baudrates
     @param:  m_dev          - const char *, device name
     @param:  m_baud         - const size_t &, baudrates
     */
    Serialib::Serialib(const char *m_dev, const size_t &m_baud) { open(m_dev, m_baud); }
    
    // Destruct Serialib
    Serialib::~Serialib(void)
    {
        close();
        
        send_lk.~recursive_mutex();
        read_lk.~mutex();
        term_lk.~mutex();
    }
    
    /*
     @brief: Operator = clones itself without mutex
     @param: rhs - Serialib
     @return: Serialib*
     */
    inline Serialib &Serialib::operator=(const class Serialib &rhs)
    {
        device    = rhs.device;
        baudrates = rhs.baudrates;
        fd        = rhs.fd;
        opt       = rhs.opt;
        status    = rhs.status;
        
        return *this;
    }
    
    /*
     @brief: Operator () opens serial port with device = rhs_d, baudrates = rhs_b
     @param:  rhs_d - char
     @param:  rhs_b - size_t
     @return: Serialib*
     */
    inline Serialib &Serialib::operator()(const char *rhs_d, const size_t &rhs_b)
    {
        open(rhs_d, rhs_b);
        
        return *this;
    }
    
    /*
     @brief: Operator << send data
     @param:  rhs  - char
     @return: bool - whether rhs data is sent
     */
    inline bool Serialib::operator<<(const char *rhs) const
    {
        const std::lock_guard<std::recursive_mutex> send_gd(send_lk);
        
        return (_c_std::write(fd, rhs, std::strlen(rhs)) != -1 ? true : false);
    }
    
    /*
     @brief: Operator << send data
     @param:  rhs  - std::vector<char>
     @return: bool - whether rhs data is sent
     */
    inline bool Serialib::operator<<(const std::vector<char> &rhs)
    {
        const std::lock_guard<std::recursive_mutex> send_gd(send_lk);
        
        p_str = &(*rhs.begin());
        
        return (_c_std::write(fd, p_str, rhs.size()) != -1 ? true : false);
    }
    
    /*
     @brief: Operator >> receive data to rhs
     @param:  rhs  - std::vector<char>
     @return: bool - whether read data is finished
     */
    inline bool Serialib::operator>>(std::vector<char> &rhs)
    {
        const std::lock_guard<std::mutex> read_gd(read_lk);
        
        while (read_avail() != 0) { if (_c_std::read(fd, &ch, 1) != 0) { rhs.emplace_back(ch); } }
        
        return (rhs.size() != 0 ? true : false);
    }
    
    /*
     @brief: Operator size_t initlize Serialib use as size_t
     @return: read_avail
     */
    inline Serialib::operator size_t() { return read_avail(); }
    
    /*
     @brief: Get current serial port status
     @return: bool - whether serial port is opend
     */
    bool Serialib::is_open(void) { return (fd <= 0 ? false : true); }
    
    /*
     @brief: Open serial port on tty devices with specialized baudrates
     @param:  dev   - tty device path
     @param:  bauds - baudrates
     @return: bool  - whether serial port is opend
     */
    bool Serialib::open(const char *dev, const size_t &bauds)
    {
        if (is_open() != false) { std::cout << "Serialib -> " << fd << ", serial'" << dev << "' already opened" << std::endl; return true; }
        
        device    = dev;
        baudrates = &bauds;
        
        /* Config open options
         - O_RDWR      open for reading and writing
         - O_NOCTTY    don't assign controlling terminal
         - O_NONBLOCK  no delay
         - O_ASYNC     signal pgrp when data ready
         */
        fd = _c_std::open(device, (O_RDWR | O_NOCTTY | O_NONBLOCK | O_ASYNC));
        if (fd <= 0) { std::cout << "Serialib -> " << fd << ", open '" << dev << "' failed" << std::endl; return false; }
        
        /* Config fcntl options
         - F_SETFL  set file status flags
         - O_RDWR   open for reading and writing
         */
        _c_std::fcntl(fd, F_SETFL, O_RDWR);
        
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
         - CREAD     enable receiver
         - CLOCAL    ignore modem status lines
         
         Software input processing:
         - INPCK     enable checking of parity errors
         - ICRNL     map CR to NL (ala CRMOD)
         - IXON      enable output flow control
         - IXOFF     enable input flow control
         - IUTF8     maintain state for UTF-8 VERASE
         */
        opt.c_cc[VTIME] = 0;
        opt.c_cc[VMIN]  = 0;
        opt.c_cflag    |= (CS8   | CREAD | CLOCAL);
        opt.c_iflag    |= (INPCK | ICRNL | IXON | IXOFF | IUTF8);
        
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
        
        flush();
        
        std::cout << "Serialib -> " << fd << ", open '" << dev << "' success" << std::endl;
        return true;
    }
    
    /*
     @brief: Close serial port
     @return: bool - whether the operation is finished
     */
    bool Serialib::close(void)
    {
        if (is_open() == false) { std::cout << "Serialib -> " << fd << ", serial '" << device << "' not opened" << std::endl; return true; }
        
        fd = _c_std::close(fd);
        if (fd != 0) { std::cout << "Serialib -> " << fd << ", close '" << device << "' failed" << std::endl; return false; }
        else         { std::cout << "Serialib -> " << fd << ", close '" << device << "' success" << std::endl; return true; }
    }
    
    /*
     @brief: Flush serial buffer
     @return: bool - whether the operation is finished
     */
    bool Serialib::flush(void)
    {
        const std::lock_guard<std::recursive_mutex> send_gd(send_lk);
        const std::lock_guard<std::mutex> read_gd(read_lk);
        
        if (_c_std::tcflush(fd, TCIOFLUSH) != 0) { return true; }
        else                                     { return false; }
    }
    
    /*
     @brief: Send char(s) using serial port
     @param:  str  - char(s) stored in vector to send
     @return: bool - whether all the date is sent
     */
    inline bool Serialib::send(const std::vector<char> &str)
    {
        const std::lock_guard<std::recursive_mutex> send_gd(send_lk);
        
        if (*this << str) { std::cout << "Serialib -> " << fd << ", send >> " << p_str << std::endl; return true; }
        else              { std::cout << "Serialib -> " << fd << ", send >> " << p_str << " failed" << std::endl; return false; }
    }
    
    /*
     @brief: Get how many char(s) can be read in buffer
     @return: size_t - char(s) count
     */
    inline size_t Serialib::read_avail(void)
    {
        _c_sys::ioctl(fd, FIONREAD, &avail);
        return avail;
    }
    
    /*
     @brief: Read char(s) from buffer
     @param:  str        - char(s) stored in vector to read
     @param:  end        - char(s) stored in vector, finish reading at the end char(s)
     @param:  length     - how many char(s) to read in the buffer, 0 stands for no limitation (SIZE_T_MAX)
     @param:  timeout_us - the time wait for serial buffer get filled, 0 stands for read immediately
     @return: size_t     - the number of readed char(s)
     */
    inline size_t Serialib::read(std::vector<char> &str, const std::vector<char> &end, const size_t &length, const double &timeout_us)
    {
        const std::lock_guard<std::mutex> read_gd(read_lk);
        
        char_read  = 0;
        str_size   = (length     != 0 ? length : std::numeric_limits<size_t>::max() - 1);
        if_ch_end  = (end.size() != 0 ? true   : false);
        ch_end_idx = 0;
        
        std::cout << "Serialib -> " << fd << ", read << ";
        
        // If timeout_us equals 0, read all char(s) in buffer with time limit, else without the time limit
        if (int(timeout_us) != 0)
        {
            // Update read_previous_time to current time
            read_previous_time = std::chrono::steady_clock::now();
            
            // While readed char(s) count less than the number limit of char(s)
            while (char_read != str_size)
            {
                // Update read_current_time to current time
                read_current_time = std::chrono::steady_clock::now();
                
                // If timeout_us subtract duration time larger than 0, else stop the reading operation
                if (timeout_us - std::chrono::duration_cast<std::chrono::microseconds>(read_current_time - read_previous_time).count() > 0)
                {
                    // Read 1 char from buffer
                    if (_c_std::read(fd, &ch, 1) != 0)
                    {
                        // Store readed char to str
                        str.emplace_back(ch);
                        
                        std::cout << ch;
                        
                        // Self-add char_read
                        ++char_read;
                        
                        // If has set str stop point
                        if (if_ch_end)
                        {
                            // If current readed char equals to current end char, check next, else the next not, reset ch_end_idx
                            if (ch == end[ch_end_idx])
                            {
                                // Self-add to check next one, if meet all the end char(s), finish read
                                if (++ch_end_idx == end.size()) { break; }
                            } else {
                                // Reset ch_end_idx
                                ch_end_idx = 0;
                            }
                        }
                    }
                } else { break; }
            }
        } else {
            while (read_avail() > 0 && char_read != str_size)
            {
                if (_c_std::read(fd, &ch, 1) != 0)
                {
                    str.emplace_back(ch);
                    std::cout << ch;
                    ++char_read;
                    if (if_ch_end)
                    {
                        if (ch == end[ch_end_idx]) { if (++ch_end_idx == end.size()) { break; } }
                        else                       { ch_end_idx = 0; }
                    }
                }
            }
        }
        std::cout << std::endl;
        
        return char_read;
    }
    
    /*
     @brief: Async read char(s) from buffer
     @param: str         - char(s) stored in vector to read
     @param: duration_us - each send duration, microseconds
     @param: args_lk     - mutex to prevent arguments changing while sending
     @param: thr_keep    - bool wheather the while loop is finished
     */
    inline void Serialib::async_send(std::vector<char> &str, double &duration_us, std::mutex &args_lk, std::atomic<bool> &thr_keep)
    {
        // Create a thread, capture all varibles pointer
        std::thread thr([p_str = &str, p_duration_us = &duration_us, p_args_lk = &args_lk, p_thr_keep = &thr_keep, p_this = this]() mutable -> void {
            // Copy duration_us to l_duration_us, allow sleep_for to use duration_us after args_lk unlocked
            static double l_duration_us { 0 };
            while (p_thr_keep->load())
            {
                p_args_lk->lock();
                *p_this << *p_str;
                l_duration_us = *p_duration_us;
                p_args_lk->unlock();
                
                std::this_thread::sleep_for(std::chrono::duration<double, std::micro>(l_duration_us));
            }
        });
        
        // Detach the created thred, return immediately
        thr.detach();
    }
    
    /*
     @brief: Async send char(s) using serial port
     @param: str         - char(s) stored in vector to send
     @param: end         - char(s) stored in vector, finish reading at the end char(s)
     @param: length      - how many char(s) to read in the buffer, 0 stands for no limitation (SIZE_T_MAX)
     @param: duration_us - each send duration, microseconds
     @param: args_lk     - mutex to prevent arguments changing while sending
     @param: thr_keep    - bool wheather the while loop is finished
     */
    inline void Serialib::async_read(std::vector<char> &str, std::vector<char> &end, size_t &length, double &duration_us, std::mutex &args_lk, std::atomic<bool> &thr_keep)
    {
        // Create a thread, capture all varibles pointer
        std::thread thr([p_str = &str, p_end = &end, p_length = &length, p_duration_us = &duration_us, p_args_lk = &args_lk, p_thr_keep = &thr_keep, p_this = this]() mutable -> void {
            // Copy duration_us to l_duration_us, allow sleep_for to use duration_us after args_lk unlocked
            static double l_duration_us { 0 };
            while (p_thr_keep->load())
            {
                p_args_lk->lock();
                p_this->read(*p_str, *p_end, *p_length, 0);
                l_duration_us = *p_duration_us;
                p_args_lk->unlock();
                
                std::this_thread::sleep_for(std::chrono::duration<double, std::micro>(l_duration_us));
            }
        });
        
        // Detach the created thred, return immediately
        thr.detach();
    }
    
    /*
     @brief: Implement simple terminal by congesting current thread
     */
    void Serialib::terminal(void)
    {
        if (is_open()) { std::cout << "Serialib -> " << fd << ", running terminal on '" << device << "', enter 'exit' to leave" << std::endl; }
        else           { std::cout << "Serialib -> " << fd << ", serial '" << device << "' not opened" << std::endl; return; }
        
        std::atomic<bool> thr_keep{ true };
        std::string       str;
        
        const std::lock_guard<std::mutex> term_gd(term_lk);
        
        // Create read thread, capture all varibles pointer, read for the serial
        std::thread thr([p_thr_keep = &thr_keep, p_this = this]() mutable -> void {
            while (p_thr_keep->load())
            {
                std::cout << *p_this;
                std::this_thread::yield();
            }
        });
        
        // Grab str to send from keyboard input
        while (true)
        {
            std::getline(std::cin, str);
            if (str != "exit")
            {
                std::vector<char> command { str.begin(), str.end() };
                command.emplace_back('\n');
                *this << command;
            } else { break; }
        }
        
        // Stop loop in read thread and join
        thr_keep = false;
        thr.join();
        thr.~thread();
    }
    
} // namespace sl

#endif
