# OLED_hal硬件I2C

移植的江科大OLED的硬件I2c版本，没有写测试函数，可自行测试，如果写的有测试文件，可提交pr

## 使用方式

先包含头文件

```
#include "OLED.h"
```

初始化

```
OLED_Init();
```

绘制字符串，数字等字符推荐使用该函数OLED_Printf

每次调用绘图函数和显示函数时需要更新

```
OLED_Update();
```
