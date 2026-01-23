## Hardware
<img src="Images/Hardware Circuit.png" alt="Alt text" width="80%" />

The hardware has the following components:

- Scanning PCB
- Lilygo T-RGB (ESP32-S3, Touch Screen, Battery recharging cycle, USB-C port, SD card)
- Button
- on/off Switch
- 3.7V 2600mAh Battery, rechargable 

Since the Lilygo board does not provide any dedicated external input pins, the onboard BOOT button was repurposed as an external input. Although the Scanning PCB includes additional external inputs, using them would require transmitting an extra button signal over the existing I²C link between the two boards, which would significantly increase implementation complexity.

## Scanning PCB
The scanning PCB was adapted from Lawrence Kincheloe's design, which represents the latest version of the handheld PlasticScanner. The IR LED wavelengths were modified to use available LEDs, and a few resistor values were adjusted accordingly.

Lawrence R Kincheloe III, [handheld scanner](https://github.com/LokiMetaSmith/handheld-scanner.git)

The PCB was soldered by hand using low-temperature solder paste, a soldering iron, a hot-air gun, and a microscope. The 1650nm IR LED is still missing, since its delivery data has been pushed to the 18.08.2026. 

<p align="center">
  <img src="Images/PCB.jpeg" height="300" />
  <img src="Images/pcb_parts_placed.jpeg" height="300" />
</p>


## Further Work
- IR LED sourcing problems -> change to push through LED's instead of SMD
- Separated LED PCB (first design exists)
- Find better solution for Button Signal (currently BOOT Button repurposed as an external input)
- to reduce cost the design could be changerd to only one photodiode or the LED count could be reduced

