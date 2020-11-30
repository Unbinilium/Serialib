## Serialib

Initilize Unix\Linux tty serial for high level communicating with serial devices.

### Requirement

- C++ 14 or later
- Unix\Linux system

### Example

```cpp
#include "include/serialib.hpp"

int main()
{
    sl::serialib serial;
    serial("/dev/tty.usbserial-0001", 921600);
    serial    << "Hello World!";
    std::cout << serial;
}
```
