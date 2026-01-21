## Firmware
This is the firmware for the handheld PlasticScanner using a LilyGo board. The Firmware can initiate a scan and send the recived data to the machine learning model. The model gives a result what plastic it thinks is scanned. Since the current model uses different IR LED's the result is not correct. The model would need to be retrained to generate correct results. 

<img src="Images/IR_LED_range.png" alt="Alt text" width="50%" />

 
Additionaly the firmware is still implemented as an example in the complete Lilygo Firmware. This is due the firmware otherwise crashing (ESP32-S3 crashes right after start up) if its outside of the example structure. To run the current firmware the platform.ini needs to be set on: 
src_dir = examples/PlasticScanner

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
