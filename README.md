## Serialib

Initilize unix/linux tty serial for high level communicating with serial devices.

### Requirement

- C++ 14 or later
- Unix\Linux system

### Example

```cpp
#include <string>
#include <vector>
#include <iostream>

#include "serialib.hpp"

int main(void)
{
    sl::serialib serial;
    serial.open("/dev/tty.usbserial-0001", 115200);
    
    std::string str = "Hello world!";
    std::vector<char> s_str(str.begin(), str.end()), r_str, end;
    
    serial.send(s_str);
    
    while(1)
    {
        serial.read(r_str, end, 12, 0);
        serial.flush();
    }
    
    return 0;
}
```
