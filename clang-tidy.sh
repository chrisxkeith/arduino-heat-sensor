cp arduino-heat-sensor.ino arduino-heat-sensor.cpp
clang-tidy arduino-heat-sensor.cpp -- \
    -I/home/ck/.arduino15/packages/arduino/hardware/mbed_giga/4.6.0/libraries/Wire \
    -I/home/ck/Arduino/libraries/Arduino_GigaDisplay_GFX/src \
    -I/home/ck/Arduino/libraries/Adafruit_GFX_Library \
    -I/home/ck/Arduino/libraries/SparkFun_GridEYE_AMG88_Library/src \
    -I/home/ck/.arduino15/packages/arduino/hardware/mbed_giga/4.6.0/cores/arduino \
    -I/home/ck/.arduino15/packages/arduino/hardware/mbed_giga/4.6.0/variants/GIGA \
    -I/home/ck/.arduino15/packages/arduino/hardware/mbed_giga/4.6.0/cores/arduino/mbed/targets/TARGET_STM \
    -I/home/ck/.arduino15/packages/arduino/hardware/mbed_giga/4.6.0/cores/arduino/mbed/targets/TARGET_STM/TARGET_STM32H7 \
    -I/home/ck/.arduino15/packages/arduino/hardware/mbed_giga/4.6.0/cores/arduino/mbed/targets/TARGET_STM/TARGET_STM32H7/STM32Cube_FW/CMSIS \
    -I/home/ck/.arduino15/packages/arduino/hardware/mbed_giga/4.6.0/cores/arduino/mbed/targets/TARGET_STM/TARGET_STM32H7/TARGET_STM32H747xI/TARGET_STM32H747xI_CM7 \
    -I/home/ck/.arduino15/packages/arduino/hardware/mbed_giga/4.6.0/cores/arduino/mbed/platform/include

rm arduino-heat-sensor.cpp
