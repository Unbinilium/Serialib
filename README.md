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
    std::cout << std::hex << ubn::crc_gen<ubn::crc_types::crc8_maxim>("Hello World!");
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

Call `crc_gen()` to generate hexadecimal CRC checksum.

```cpp
// Print crc8_maxim checksum
std::cout << std::hex << ubn::crc_gen<ubn::crc_types::crc8_maxim>(str);
std::cout << std::hex << ubn::crc_gen<ubn::crc_types::crc8_maxim>("Hello World!");
```

All available CRC checksum types are listed in `crc_types` enum.

```cpp
enum crc_types {
    crc8, crc8_cdma2000, crc8_darc, crc8_dvb_s2, crc8_ebu, crc8_i_code, crc8_itu, crc8_maxim, crc8_rohc, crc8_wcdma,
    crc16_a, crc16_arc, crc16_aug_ccitt, crc16_buypass, crc16_ccitt_false, crc16_cdma2000, crc16_dds_110, crc16_dect_r, crc16_dect_x, crc16_dnp, crc16_en_13757, crc16_genibus, crc16_kermit, crc16_maxim, crc16_mcrf4xx, crc16_modbus, crc16_riello, crc16_t10_dif, crc16_teledisk, crc16_tms37157, crc16_usb, crc16_x_25, crc16_xmodem,
    crc32, crc32_bzip2, crc32_c, crc32_d, crc32_jamcrc, crc32_mpeg_2, crc32_posix, crc32_q, crc32_xfer,
    crc64_ecma, crc64_iso
};
```

Authlib uses template with `crc_types` and const expression so that the CRC table could be generated at compile time for better performance.

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
