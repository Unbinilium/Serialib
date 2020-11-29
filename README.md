## Serialib

Initilize Unix\Linux tty serial for high level communicating with serial devices.

### Requirement

- C++ 14 or later
- Unix\Linux system

### Example

```cpp
#include <string>
#include <vector>
#include <iostream>

#include "include/serialib.hpp"

int main(void)
{
    std::string str = "Hello world!";
    std::vector<char> s_str(str.begin(), str.end()), r_str, end;
    {
        sl::serialib serial;
        serial.open("/dev/tty.usbserial-0001", 921600);
        serial.send(s_str);
        serial.read(r_str, end, 12, 10);
    }
    return 0;
}
```
