## Serialib

Initialize Unix/Linux tty serial for high level communication with serial devices, basic checksum *CRC8_MAXIM* implemented and multi-threading ready.

### Requirement

- GCC/Clang with `-std=c++17` or later
- Unix/Linux environment

### Example

The minimal **Serialib** usage.

```cpp
#include "include/serialib.hpp"
int main()
{
    sl::Serialib serial("/dev/tty.usbserial-0001", 921600);
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
    std::cout << CRC8_MAXIM("Hello World!");
}
```

What's more? Here's `async_send_data()` function used to sync data real-time using serial port, the function returns immediately after detach the newly created **thread**, and the thread is running background, destruct by set `thr_keep` to *false*.

```cpp
template <typename DataT> inline void async_send_data(const DataT &data, class sl::Serialib &serial, std::atomic<bool> &thr_keep)
{
    std::thread thr([p_data = &data, p_serial = &serial, p_thr_keep = &thr_keep]() mutable -> void {
        while (p_thr_keep->load())
        {
            // Lock p_data, get the data to send and update l_sync_duration_us
            p_data->sync_lk.lock();
            std::vector<char> data    { p_data->data.beign(),p_data->data.end() };
            double l_sync_duration_us { p_data->sync_duration_us };
            p_data->sync_lk.unlock();

            // Construct data and send by serial
            using namespace al;
            *p_serial << ("S" | data | CRC8_MAXIM(data) | "E");

            // Sleep until data ready
            std::this_thread::sleep_for(std::chrono::duration<double, std::micro>(l_sync_duration_us));
        }
    });
    thr.detach();
}
```

### Documention

Basic functions usage implement, for all available functions and detailed descriptions, please view `/include/serialib.hpp` and `/include/authlib.hpp` source.

#### Declaration

```cpp
// Only declare
sl::Serialib serial; // or 'sl::Serialib* serial = new sl::Serialib'
// Declare and open
sl::Serialib serial(DEVICE, BAUDRATES);
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
sl::Serialib serial(DEVICE, BAUDRATES);
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
  str        - std::vector<char>, char(s) stored in vector to read
  end        - std::vector<char>, char(s) stored in vector, finish reading at the end char(s)
  length     - size_t, how many char(s) to read in the buffer, 0 stands for no limitation (SIZE_T_MAX)
  timeout_us - double, the time wait for serial buffer get filled, 0 stands for read immediately
*/
serial.read(str, end, length, timeout_us);
```

#### Misc

```cpp
// Flush serial buffer, returns bool
serial.flush();
// Wheather serial is open, returns bool
serial.is_open();
// Get available char(s) to read in buffer, returns size_t
serial.read_avail();
// Implent simple terminal for debugging (congesting current thread)
serial.terminal();
```

#### Authlib/CRC8_MAXIM

Call `CRC8_MAXIM()` to generate 2 char digits CRC8 MAXIM checksum, each digit is checksum's hexadecimal number place.

```cpp
using namespace al;
std::vector<char> checksum;
// Print CRC8_MAXIM checksum
std::cout << CRC8_MAXIM(str) << std::endl;
std::cout << CRC8_MAXIM("Hello World!");
// Generate CRC8_MAXIM checksum and append to std::vector<char>, it 2 digit of the HEX
checksum << CRC8_MAXIM(str); // or 'checksum = CRC8_MAXIM(str)'
checksum << CRC8_MAXIM("Hello World");
// Chain for serial API
serial << ("S" | data | CRC8_MAXIM(data) | "E");
```

#### Multi-threading

Serialib (both authlib) is multi-threading ready, all functions are thread-safe. For basic parallel use, the builtin functions are listed here.

```cpp
/*
Async send char(s) using serial port
  str         - std::vector<char>, char(s) stored in vector to send
  duration_us - double, each send duration, microseconds
  args_lk     - std::mutex, to prevent arguments changing while sending
  thr_keep    - std::atomic<bool>, wheather the while loop is finished
*/
serial.async_send(str, duration_us, args_lk, thr_keep);
/*
Async read char(s) using serial port
  str         - std::vector<char>, char(s) stored in vector to read
  end         - std::vector<char>, char(s) stored in vector, finish reading at the end char(s)
  length      - size_t, char(s) to read in the buffer, 0 stands for no limitation (SIZE_T_MAX)
  duration_us - double, each send duration, microseconds
  args_lk     - std::mutex, to prevent arguments changing while sending
  thr_keep    - std::atomic<bool>, wheather the while loop is finished
*/
serial.async_read(str, end, length, duration_us, args_lk, thr_keep);
```



### License

[MIT License](https://github.com/Unbinilium/Serialib/blob/main/LICENSE) Copyright (c) 2020 Unbinilium.
