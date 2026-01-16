## Firmware
The firmware is still very basic and not fully functioning. All UI's are still quite raw and only show the necessary. The Firmware can initiate a scan and send the recived data to the ML model. The model gives a result what plastic it thinks is scanned. Somthing in that logic is still wrong, since the ML always concludes to not know the material. Additionaly the firmware is still implemented as an example in the complete Lilygo Firmware. This is due the firmware otherwise crashing (ESP32-S3 crashes right after start up) if its outside of the example structure. 

Implemented:
- Deep-sleep after long button press
- Deep-sleep after timeout of 5min
- Waking from Deep-sleep with long button press
- Scan after short button press
- ML Model
- Startup UI
- Inital UI
- Scanning UI
- Results UI


## References
This firmware is based on:
- LilyGO T-RGB firmware
- https://github.com/Xinyuan-LilyGO/LilyGo-T-RGB.git

Original license is preserved.
Significant modifications were made for the Plastic Scanner project.
