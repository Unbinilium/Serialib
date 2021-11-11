## Serialib

Initialize Unix/Linux tty serial for high level communication with serial devices, basic checksum *crc8_maxim* implemented and async ready.

### Requirement

- GCC/Clang with `-std=c++20` or later
- Unix/Linux environment

### Example

The minimal **serialib** usage.

```cpp
#include "include/serialib.hpp"
int main() {
    ubn::serialib serial("/dev/tty.usbserial-0001", 921600);
    serial << "Hello World!";
    std::cout << serial;
}
```

And **authlib** (a component of serialib) example.

```cpp
#include "include/authlib.hpp"
int main() {
    std::cout << std::hex << ubn::crc8_gen<ubn::crc8_types::maxim>("Hello World!");
}
```

### Documention

Basic functions usage implement, for all available functions and detailed descriptions, please view `/include/serialib.hpp` and `/include/authlib.hpp` source.

#### Declaration

```cpp
// Only declare
ubn::serialib serial;
// Declare and open
ubn::serialib serial(DEVICE, BAUDRATES);
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
ubn::serialib serial(DEVICE, BAUDRATES);
// Open after declaration, returns bool
serial     (DEVICE, BAUDRATES);
serial.open(DEVICE, BAUDRATES);
```

#### Destruct/Close Serial

```cpp
// Default destructor
~serial();
// Close manually, returns bool
serial.close();
```

#### Send

```cpp
std::string s = "Hello World!";
// Send use operator << char *, std::string and std::string_view allowed, returns bool
serial << str;
serial << "Hello World!";
```

#### Read

```cpp
// Read buffer use std::ostream operator <<, returns bool
std::cout << serial;
// Read buffer use operator >>, std::string and std::string_view allowed, overwrite, returns bool
std::string str;
serial >> str;
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

#### Authlib

Call `crc8_gen()` to generate 1 byte hexadecimal CRC8 checksum.

```cpp
// Print crc8 maxim checksum
std::cout << std::hex << ubn::crc8_gen<ubn::crc8_types::maxim>(str);
std::cout << std::hex << ubn::crc8_gen<ubn::crc8_types::maxim>("Hello World!");
```

All available CRC8 checksum types are listed in `crc8_types` enum.

```cpp
enum crc8_types { ccitt, itu, rohc, ebu, i_code, maxim, darc, cdma2000, wcdma, dvb_s2 };
```

Authlib uses template with `crc8_types` and const expression so that the CRC8 table could be generated at compile time for better performance.

#### Async

Serialib (also authlib) is thread-safe and async ready, the builtin methods are listed here.

```cpp
/*
     @brief: Async send data
     @param:  _data_ftr          - const std::future<std::string_view> &, data to send
     @return: std::future<bool>  - send status future
 */
template <typename T, std::enable_if_t<std::is_nothrow_convertible_v<T, std::string_view>, bool> = true>
std::future<bool> async_send(const std::future<T>& _data_ftr);
/*
    @brief: Async read data
    @return: std::future<std::string_view> - received buffer data future
 */
template <typename T, std::enable_if_t<std::is_nothrow_convertible_v<T, std::string_view>, bool> = true>
std::future<T> async_read();
```

### License

[MIT License](https://github.com/Unbinilium/serialib/blob/main/LICENSE) Copyright (c) 2020 Unbinilium.
