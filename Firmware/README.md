## Firmware
This is the firmware for the handheld PlasticScanner using a LilyGo board. The Firmware can initiate a scan and send the recived data to the machine learning model. The model gives a result what plastic it thinks is scanned. Since the current model uses different IR LED's the result is not correct. The model would need to be retrained to generate correct results. In red the 1720nm LED, which is currently unavailable and was thus replaced with a (green) 850nm LED. The orange 1650nm LED, was available but got discontinued. 

<img src="Images/IR_LED_range.png" alt="Alt text" width="70%" />

 
Additionaly the firmware is still implemented as an example in the complete Lilygo Firmware. This is due the firmware otherwise crashing (ESP32-S3 crashes right after start up) if its outside of the example structure. To run the current firmware the platform.ini needs to be set on:
 
<span style="color:red;">src_dir = examples/PlasticScanner</span>

Implemented features:
- Deep-sleep after long button press
- Deep-sleep after timeout of 5min
- Waking from Deep-sleep with long button press
- Scan after short button press
- old machine learning Model
- Startup UI
- Inital UI
- Scanning UI
- Results UI

## Future Work
Next steps could be generating training data for the machine learning model, especially for the 850nm LED and going through the available traning data of the PlasticScanner. There are different versions of the model that should be compared before retraining one of them with the specified IR LED wavelength range. Additionally the AS7341 color sensor could be implemented in the Firmware and in the training data. 


## References
This firmware is based on:
- LilyGO T-RGB firmware
- https://github.com/Xinyuan-LilyGO/LilyGo-T-RGB.git

Original license is preserved.
Significant modifications were made for the Plastic Scanner project.
