## Serialib

Initialize Unix/Linux tty serial for high level communicating with serial devices, basic checksum *CRC8_MAXIM* implemented and multi-threads ready.

### Requirement

- GCC/Clang with C++17 or later
- Unix/Linux environment

### Example

The minimal **serialib** usage.

```cpp
#include "include/serialib.hpp"
int main()
{
    sl::serialib serial("/dev/tty.usbserial-0001", 921600);
    serial << "Hello World!";
    std::cout << serial;
}
```

And basic **authlib** (a component of serialib) example.

```cpp
#include "include/authlib.hpp"
int main()
{
    using namespace al;
    std::cout << CRC8_MAXIM << "Hello World!";
}
```

What's more? Here's `async_send_data()` function used to sync data with robots using serial port, the function returns immediately after detach newly created **thread**, the thread is running background, destruct by set `thr_keep` *false*.

```cpp
template <typename T_data> static void async_send_data(const T_data& data, sl::serialib& serial, bool& thr_keep)
{
    std::thread thr([p_data = &data, p_serial = &serial, p_thr_keep = &thr_keep]() mutable {
        const static char DEC_DIGIT[10] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9' };
        while (*p_thr_keep)
        {
            std::vector<char> s_d_full(1, 'S');
            std::vector<char> s_d_ass (8);
            p_data->sync_lk.lock();
            s_d_ass[0] = p_data->in_tracking  != true ? '0' : '1';
            s_d_ass[1] = p_data->is_candidate != true ? '0' : '1';
            s_d_ass[2] = p_data->yawn >= 0 ? '+' : '-';
            s_d_ass[3] = DEC_DIGIT[size_t(p_data->yawn / 10)];
            s_d_ass[4] = DEC_DIGIT[size_t(p_data->yawn % 10)];
            s_d_ass[5] = p_data->pitch >= 0 ? '+' : '-';
            s_d_ass[6] = DEC_DIGIT[size_t(p_data->pitch / 10)];
            s_d_ass[7] = DEC_DIGIT[size_t(p_data->pitch % 10)];
            p_data->sync_lk.unlock();
            s_d_ass << al::CRC8_MAXIM << s_d_ass;
            s_d_full.insert(s_d_full.end(), s_d_ass.begin(), s_d_ass.end());
            s_d_full.push_back('E');
            *p_serial << s_d_full;
            _c_std::usleep(p_data->sync_duration_us);
        }
    });
    thr.detach();
}
```

### Documention

Basic function usage implement, for all available functions and detailed description, please view `/include/serialib.hpp` and `/include/authlib.hpp` source.

#### Declaration

```cpp
// Only declare
sl::serialib serial; // or 'sl::serialib* serial = new sl::serialib'
// Declare and open
sl::serialib serial(DEVICE, BAUDRATES);
```

#### Type

```cpp
// Use as type size_t, stands for the buffer size, equals to serial.read_avail()
if (serial == 0) std::cout << "Buffer free";
// Use as ty std::ostream, stands for the buffer contents
std::cout << serialib;
```

#### Open Serial

```cpp
// Open while declare
sl::serialib serial(DEVICE, BAUDRATES);
// Open after declaration, returns bool
serial     (DEVICE, BAUDRATES);
serial.open(DEVICE, BAUDRATES);
```

#### Close Serial/Destruct

```cpp
// Default destructor
~serial(); // or 'delete serial'
// Close manually, returns bool
serial.close();
```

#### Send

```cpp
std::string s = "Hello World!";
std::vector<char> str(s.begin(), s.end());
// Send use operator <<, char* and std::vector<char> allowed, returns bool
serial << str;
serial << "Hello World!";
// Send use function, only std::vector<char> allowed, returns bool
serial.send(str);
```

#### Read

```cpp
// Read buffer use std::ostream operator <<, returns bool
std::cout << serial;
// Read buffer use operator >>, only std::vector<char> allowed, append, returns bool
serial >> str;
/*
Read buffer use read() function, append, returns size_t (bytes read)
  str        - char(s) stored in vector to read
  end        - char(s) stored in vector, finish reading at the end char(s)
  length     - how many char(s) to read in the buffer, 0 stands for no limitation (SIZE_T_MAX)
  timeout_ms - the time wait for serial buffer get filled, 0 stands for read immediately
*/
serial.read(str, end, length, timeout_ms);
```

#### Misc

```cpp
// Flush serial buffer, returns bool
serial.flush();
// Wheather serial is open, returns bool
serial.is_open();
// Get available char(s) to read in buffer, returns size_t
serial.read_avail();
// Implement simple terminal for debugging (congesting current thread), returns bool
serial.terminal();
```

#### Authlib/CRC8_MAXIM

Generate 2 char digit and append to vector, each digit is checksum's hexadecimal number place.

```cpp
using namespace al; // Using namespace authlib
std::vector<char> checksum;
// Print CRC8_MAXIM checksum, 'std::cout << CRC8_MAXIM <<' returns std::ostream
std::cout << CRC8_MAXIM << str << std::endl; // Works with std::ostream
std::cout << CRC8_MAXIM << "Hello World!";
// Generate CRC8_MAXIM checksum and append to std::vector<char>, it 2 digit of the HEX
checksum  =  CRC8_MAXIM(raw);   // or 'checksum << CRC8_MAXIM(str)'
checksum  << CRC8_MAXIM << str; // or 'checksum = CRC8_MAXIM << str'
checksum  << CRC8_MAXIM << "Hello World";
```

#### Multi-threads

Serialib (both authlib) is multi-threading ready, all its functions is thread safe. For basic parallel use,the builtin threaded function listed here.

```cpp
/*
Async send char(s) using serial port
  str         - std::vector<char>, char(s) stored in vector to send
  str_lk      - std::mutex, to prevent str changing while accessing
  duration_us - useconds_t, each send duration, microseconds
  thr_keep    - bool, wheather the while loop is finished
*/
serial.async_send(str, str_lk, duration_us, thr_keep);
/*
Async read char(s) using serial port
  str         - std::vector<char>, char(s) stored in vector to read
  end         - std::vector<char>, char(s) stored in vector, finish reading at the end char(s)
  length      - size_t, char(s) to read in the buffer, 0 stands for no limitation (SIZE_T_MAX)
  str_lk      - std::mutex, to prevent str changing while accessing
  duration_us - useconds_t, each send duration, microseconds
  thr_keep    - bool, wheather the while loop is finished
*/
serial.async_read(str, end, length, str_lk, duration_us, thr_keep);
```

###  License

[MIT License](https://github.com/Unbinilium/Serialib/blob/main/LICENSE) Copyright (c) 2020 Unbinilium.
