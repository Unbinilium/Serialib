## Serialib

Initialize Unix/Linux tty serial for high level communicating with serial devices, basic checksum *CRC8_MAXIM* implemented and multi-threads ready.

### Requirement

- GCC/Clang with `-std=c++17` or later
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
        while (*p_thr_keep)
        {
            std::vector<char> s_d_full(1, 'S');
            std::vector<char> s_d_temp(6);
            p_data->sync_lk.lock();
            std::to_chars(s_d_temp.data()    , s_d_temp.data() + 1, p_data->in_tracking + p_data->is_candidate); // in_tracking +0/+1, is_candidate +2/+3
            std::to_chars(s_d_temp.data() + 1, s_d_temp.data() + 3, p_data->pitch                             ); // +0   ~ +99
            std::to_chars(s_d_temp.data() + 3, s_d_temp.data() + 5, std::abs(p_data->pivot)                   ); // -360 ~ +360, 'L' for negative, 'R' for positive
            s_d_temp[5] = pivot >= 0 ? 'R' : 'L';
            p_data->sync_lk.unlock();
            *p_serial << ((s_d_full << s_d_temp << al::CRC8_MAXIM << s_d_temp) << 'E');
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
std::cout << serial;
```

#### Open Serial

```cpp
// Open while declare
sl::serialib serial(DEVICE, BAUDRATES);
// Open after declaration, returns bool
serial     (DEVICE, BAUDRATES);
serial.open(DEVICE, BAUDRATES);
```

#### Destruct/Close Serial

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
std::vector<char> end;
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
// Implent simple terminal for debugging (congesting current thread), returns bool
serial.terminal();
```

#### Authlib/CRC8_MAXIM

Generate 2 char digits and append to vector, each digit is checksum's hexadecimal number place.

```cpp
using namespace al; // Using namespace authlib
std::vector<char> checksum;
// Print CRC8_MAXIM checksum, 'std::cout << CRC8_MAXIM <<' returns std::ostream
std::cout << CRC8_MAXIM << str << std::endl; // Works with std::ostream
std::cout << CRC8_MAXIM << "Hello World!";
// Generate CRC8_MAXIM checksum and append to std::vector<char>, it 2 digit of the HEX
checksum  =  CRC8_MAXIM(str);   // or 'checksum << CRC8_MAXIM(str)'
checksum  << CRC8_MAXIM << str; // or 'checksum = CRC8_MAXIM << str'
checksum  << CRC8_MAXIM << "Hello World";
```

#### Multi-threading

Serialib (both authlib) is multi-threading ready, all functions are thread safe. For basic parallel use, the builtin functions are listed here.

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
