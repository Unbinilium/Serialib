## Serialib

Initilize Unix/Linux tty serial for high level communicating with serial devices.

### Requirement

- GCC/Clang with c++13 or later
- Unix/Linux environment

### Example

The minimal serialib usage.

```cpp
#include "include/serialib.hpp"
int main()
{
    sl::serialib serial("/dev/tty.usbserial-0001", 921600);
    serial << "Hello World!";
    std::cout << serial;
}
```

### Documention

Basic function usage implement, for all available functions and detailed description, please view `/include/serialib.hpp` source.

#### Declare

```cpp
// Only declare
sl::serialib  serial;
sl::serialib* serial = new sl::serialib;
// Declare and open
sl::serialib serial(DEVICE, BAUDRATES);
```

#### Type

```cpp
// Use as type size_t, size_t(serial) stands for the buffer size, equals to serial.read_avail()
if (serial == 0) std::cout << "Buffer free"
```

#### Open

```cpp
// Open while declare
sl::serialib serial(DEVICE, BAUDRATES);
// Open after declaration, return bool
serial     (DEVICE, BAUDRATES);
serial.open(DEVICE, BAUDRATES);
```

#### Close/Destruct

```cpp
// Default destructor
~serial();
delete serial;
// Close manually, return bool
serial.close();
```

#### Send

```cpp
std::string str = "Hello World!";
std::vector<char> s_str(str.begin(), str.end());
// Use operator <<, return bool
serial << s_str;
serial << "Hello World!";
// Use function, return bool
serial.send(s_str);
```

#### Read

```cpp
std::vector<char> r_str, r_end;
// Use std::ostream operator <<, return bool
std::cout << serialib;
// Use operator >> append, return bool
serial >> r_str;
/*
Use function, return size_t (bytes read)
  str - char(s) stored in vector to read
  end - char(s) stored in vector, set as endpoint to stop reading
  length - the number limit of char(s) to read
  timeout_ms - the time limit while reading
*/
serial.read(r_str, r_end, length, timeout_ms);
```

#### Misc

```cpp
// Flush serial buffer, return bool
serial.flush();
// If serial is open, return bool
serial.is_open();
// Get current buffer size, return size_t
serial.read_avail();
// Implement simple terminal for debugging (congesting current thread), return bool
serial.terminal();
```
