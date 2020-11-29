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
    std::vector<char> rec;
    {
        sl::serialib serial;
        serial.open("/dev/tty.usbserial-0001", 921600);
        serial << "Hello World!";
        serial >> rec;
    }
    for(auto& _rec : rec) { std::cout << _rec; }
}
```
