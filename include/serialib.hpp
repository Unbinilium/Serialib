#pragma once

#include <iostream>

#include <mutex>
#include <atomic>
#include <type_traits>

#include <string>
#include <string_view>

#include <future>
#include <thread>

#include <fcntl.h>
#include <unistd.h>
#include <termios.h>

#include <sys/ioctl.h>

namespace ubn {
    class serialib {
    public:
        /*
            @brief: Default constructor of serialib without init
        */
        serialib() = default;

        /*
            @brief: Init serialib with device name and baudrates
            @param:  _dev  - const std::string_view &, device name
            @param:  _baud - const std::size_t &, baudrates
        */
        template <typename T, std::enable_if_t<std::is_nothrow_convertible_v<T, std::string_view>, bool> = true>
        constexpr explicit serialib(const T& _dev, const std::size_t& _baud) noexcept : m_device(_dev), m_baudrates(_baud) {
            open(m_device, m_baudrates);
        }

        /*
            @brief: Default destructor of serialib without init
        */
        ~serialib() noexcept {
            flush();
            close();

            send_lk.~mutex();
            read_lk.~mutex();
            term_lk.~mutex();
        }

        /*
            @brief: Operator = clones serialib
            @param: _rhs        - const serialib &, clone serialib
            @return: serialib & - return cloned serialib
        */
        serialib& operator=(const serialib& _rhs) noexcept {
            const std::lock_guard<std::mutex> send_gd(send_lk);
            const std::lock_guard<std::mutex> read_gd(read_lk);
            const std::lock_guard<std::mutex> term_gd(term_lk);

            m_device    = _rhs.m_device;
            m_baudrates = _rhs.m_baudrates;
            m_fd        = _rhs.m_fd;
            m_opt       = _rhs.m_opt;
            m_sta       = _rhs.m_sta;

            return *this;
        }

        /*
            @brief: Operator << for std::ostream
            @param: _os         - std::ostream &, output stream
            @param: _rhs        - const serialib &, clone serialib
            @return: serialib & - return cloned serialib
        */
        constexpr friend std::ostream& operator<<(std::ostream& _os, const serialib& _rhs) noexcept {
            std::string_view buffer;
            *this >> buffer;
            _os   << buffer;
            return _os;
        }

        /*
            @brief: Operator << send data
            @param:  _rhs  - const std::string_view &, data to send
            @return: bool  - whether rhs data is sent
        */
        template <typename T, std::enable_if_t<std::is_nothrow_convertible_v<T, std::string_view>, bool> = true>
        constexpr bool operator<<(const T& _rhs) const noexcept {
            const std::string_view rhs { _rhs };
            const std::lock_guard<std::mutex> send_gd(send_lk);
            return ::write(m_fd, rhs.data(), rhs.size()) > 0;
        }

        /*
            @brief: Get how many char(s) can be read in buffer
            @return: std::size_t - char(s) count
        */
        std::size_t read_avail() const noexcept {
            std::size_t avail { 0 };
            ::ioctl(m_fd, FIONREAD, &avail);
            return avail;
        }

        /*
            @brief: Operator std::size_t initlize serialib use as std::size_t
            @return: read_avail
        */
        operator std::size_t() const noexcept { return read_avail(); }

        /*
            @brief: Operator >> store received buffer data to rhs_
            @param:  rhs_  - const std::string_view &, the string_view to store the buffer data
            @return: bool  - whether read buffer data is succeeded
        */
        template <typename T, std::enable_if_t<std::is_nothrow_convertible_v<std::string_view, T>, bool> = true>
        bool operator>>(T& rhs_) const noexcept {
            const std::lock_guard<std::mutex> read_gd(read_lk);

            void*             p_buffer    { nullptr };
            const std::size_t buffer_size { read_avail() };
            if (buffer_size > 0 && ::read(m_fd, &p_buffer, buffer_size) > 0) {
                rhs_ = static_cast<T>(std::string_view(static_cast<char *>(p_buffer), buffer_size));
                return true;
            }

            return false;
        }

        /*
            @brief: Get current serial port status
            @return: bool - whether serial port is opend
        */
        constexpr bool is_open() const noexcept { return m_fd > 0; }

        /*
            @brief: Open serial port on tty m_devices with specialized m_baudrates
            @param:  _dev  - const std::string_view&, device name
            @param:  _baud - const std::size_t&, baudrates
            @return: bool  - whether serial port is opend
        */
        template <typename T, std::enable_if_t<std::is_nothrow_convertible_v<T, std::string_view>, bool> = true>
        bool open(const T& _dev, const std::size_t& _baud) noexcept {
            if (is_open() != false) {
                std::cout << "serialib -> " << m_fd << ", serial'" << _dev << "' already opened" << std::endl;
                return true;
            }

            if (m_device.empty() || m_baudrates == 0) {
                m_device    = _dev;
                m_baudrates = _baud;
            }

            /* Config open options
                - O_RDWR      open for reading and writing
                - O_NOCTTY    don't assign controlling terminal
                - O_NONBLOCK  no delay
                - O_ASYNC     signal pgrp when data ready
            */
            m_fd = ::open(m_device.data(), O_RDWR | O_NOCTTY | O_NONBLOCK | O_ASYNC);
            if (m_fd <= 0) {
                std::cout << "serialib -> " << m_fd << ", open '" << m_device << "' failed" << std::endl;
                return false;
            }

            /* Config fcntl options
                - F_SETFL  set file m_sta flags
                - O_RDWR   open for reading and writing
            */
            ::fcntl(m_fd, F_SETFL, O_RDWR);

            // Get current serial port options
            ::tcgetattr(m_fd, &m_opt);
            ::cfmakeraw(&m_opt);

            // Set input and output baudrates
            ::cfsetispeed(&m_opt, m_baudrates);
            ::cfsetospeed(&m_opt, m_baudrates);

            /* Config serial port options
            Special Control Characters:
                - VTIME
                - VMIN
            Hardware control of terminal:
                - CS8       character size mask as 8 bits
                - CREAD     enable receiver
                - CLOCAL    ignore modem m_sta lines
            Software input processing:
                - INPCK     enable checking of parity errors
                - ICRNL     map CR to NL (ala CRMOD)
                - IXON      enable output flow control
                - IXOFF     enable input flow control
                - IUTF8     maintain state for UTF-8 VERASE
            */
            m_opt.c_cc[VTIME] = 0;
            m_opt.c_cc[VMIN]  = 0;
            m_opt.c_cflag    |= CS8   | CREAD | CLOCAL;
            m_opt.c_iflag    |= INPCK | ICRNL | IXON | IXOFF | IUTF8;

            // Set serial port options
            ::tcsetattr(m_fd, TCSANOW, &m_opt);

            // Get current serial port status
            ::ioctl(m_fd, TIOCMGET, &m_sta);

            /* Compatability with old terminal driver:
                - TIOCM_DTR  data terminal ready
                - TIOCM_RTS  request to send
            */
            m_sta |= TIOCM_DTR;
            m_sta |= TIOCM_RTS;

            // Set current serial port status
            ::ioctl(m_fd, TIOCMSET, &m_sta);

            flush();
            std::cout << "serialib -> " << m_fd << ", open '" << m_device << "' success" << std::endl;

            return true;
        }

        /*
            @brief: Close serial port
            @return: bool - whether the operation is finished
        */
        bool close() noexcept {
            if (is_open() == false) {
                std::cout << "serialib -> " << m_fd << ", serial '" << m_device << "' not opened" << std::endl;
                return true;
            }

            m_fd = ::close(m_fd);
            if (m_fd != 0) {
                std::cout << "serialib -> " << m_fd << ", close '" << m_device << "' failed" << std::endl;
                return false;
            } else {
                std::cout << "serialib -> " << m_fd << ", close '" << m_device << "' success" << std::endl;
                return true;
            }
        }

        /*
            @brief: Flush serial buffer
            @return: bool - whether the operation is succeeded
        */
        bool flush() const noexcept {
            const std::lock_guard<std::mutex> send_gd(send_lk);
            const std::lock_guard<std::mutex> read_gd(read_lk);

            return ::tcflush(m_fd, TCIOFLUSH) != -1;
        }

        /*
            @brief: Async send data
            @param:  _data_ftr          - const std::future<std::string_view> &, data to send
            @return: std::future<bool>  - send status future
        */
        template <typename T, std::enable_if_t<std::is_nothrow_convertible_v<T, std::string_view>, bool> = true>
        std::future<bool> async_send(const std::future<T>& _data_ftr) const noexcept {
            std::promise<bool> pms;
            std::future<bool>  ftr { pms.get_future() };

            std::thread thr([_this = this, _pms = std::move(pms), __data_ftr = &_data_ftr]() mutable {
                _pms.set_value(*_this << __data_ftr.get());
            });
            thr.detach();

            return ftr;
        }

        /*
            @brief: Async read data
            @return: std::future<std::string_view> - received buffer data future
        */
        template <typename T, std::enable_if_t<std::is_nothrow_convertible_v<std::string_view, T>, bool> = true>
        std::future<T> async_read() const noexcept {
            std::promise<T> pms;
            std::future<T>  ftr { pms.get_future() };

            std::thread thr([_this = this, _pms = std::move(pms)]() {
                T buffer;
                while (!(*_this >> buffer)) {
                    std::this_thread::yield();
                }
                _pms.set_value(buffer);
            });
            thr.detach();

            return ftr;
        }

        /*
            @brief: Implement simple terminal by congesting current thread
        */
        void terminal() const noexcept {
            if (is_open()) {
                std::cout << "serialib -> " << m_fd << ", running terminal on '" << m_device << "', enter 'exit' to leave" << std::endl;
            } else {
                std::cout << "serialib -> " << m_fd << ", serial '" << m_device << "' not opened" << std::endl;
                return;
            }

            std::string      str;
            std::atomic_flag exit;
            const std::lock_guard<std::mutex> term_gd(term_lk);

            // Create read thread, capture all varibles pointer, read from the serial
            std::jthread thr([_this = this, _exit = &exit]() mutable {
                while (!_exit) {
                    std::cout << *_this;
                    std::this_thread::yield();
                }
            });

            // Grab str to send from keyboard input
            while (true) {
                std::getline(std::cin, str);
                using namespace std::string_literals;
                if (str != "exit"s) { *this << str + "\n"; }
                else                { break; }
            }
            exit.test_and_set();
        }

    protected:
        std::string_view             m_device;
        std::size_t                  m_baudrates;

        int                          m_fd        { -1 };
        int                          m_sta       { -1 };
        struct  termios              m_opt;

    private:
        mutable std::mutex           send_lk;
        mutable std::mutex           read_lk;
        mutable std::mutex           term_lk;
    };
}
