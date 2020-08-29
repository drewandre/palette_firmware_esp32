palette_firmware

Final dipswitch configuration
1: OFF
2: OFF
3: ON (for GPIO_NUM_15 access)
4: ON (for GPIO_NUM_13 access)
5: ON (for GPIO_NUM_12 access)
6: ON (for GPIO_NUM_14 access)
7: ON (for aux input detection)
8: OFF
GPIO_NUM_0  - n/a - Automatic Upload, I2S MCLK (wouldn't be able to use automatic reset, second furthest pin from usb)
GPIO_NUM_2  - n/a - Automatic Upload, MicroSD D0 (wouldn't be able to use automatic reset, fourth furthest pin from usb)
RX          - n/a - UART0 RX (wouldn't be able to use any HOST -> ESP32 UART control)
TX          - n/a - UART0 TX (can't use USB debugging though...)
GPIO_NUM_12 - 5   - JTAG MTDI, MicroSD D2, Aux signal detect
GPIO_NUM_13 - 4   - JTAG MTCK, MicroSD D3, Audio Vol- (TP)
GPIO_NUM_14 - 6   - JTAG MTMS, MicroSD CLK
GPIO_NUM_15 - 3   - JTAG MTDO, MicroSD CMD
Other useful pins:
GPIO_NUM_22 - green led
GPIO_NUM_19 - aux insert detection
GPIO_NUM_12 - aux insert detection
GPIO_NUM_21 - PA enable output

 Core configuration
 1: ARDUINO
 1: FFT
 1: FastLED
 0: BTDM
 0: BLUEDROID
 0: WIFI