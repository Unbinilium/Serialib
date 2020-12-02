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

### Documention

Basic function usage implement, for all available functions and detailed description, please view `/include/serialib.hpp` and `/include/authlib.hpp` source.

#### Declaration

```cpp
// Only declare
sl::serialib  serial; // or 'sl::serialib* serial = new sl::serialib'
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
std::string str = "Hello World!";
std::vector<char> s_str(str.begin(), str.end());
// Send use operator <<, char* and std::vector<char> allowed, returns bool
serial << s_str;
serial << "Hello World!";
// Send use function, only std::vector<char> allowed, returns bool
serial.send(s_str);
```

#### Read

```cpp
std::vector<char> str, end;
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
std::string str = "Hello World!";
std::vector<char> raw(str.begin(), str.end()), checksum;
// Print CRC8_MAXIM checksum, 'std::cout << CRC8_MAXIM <<' returns std::ostream
std::cout << CRC8_MAXIM << raw << std::endl; // Works with std::ostream
std::cout << CRC8_MAXIM << "Hello World!";
// Generate CRC8_MAXIM checksum and append to std::vector<char>, it 2 digit of the HEX
checksum =  CRC8_MAXIM(raw);      // checksum << CRC8_MAXIM(raw)
checksum << CRC8_MAXIM << raw;    // checksum = CRC8_MAXIM << raw
checksum << CRC8_MAXIM << "Hello World";
```

### License

[MIT License](https://github.com/Unbinilium/Serialib/blob/main/LICENSE) Copyright (c) 2020 Unbinilium.
