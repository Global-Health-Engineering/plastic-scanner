## Plastic Scanner
 The goal of this project is to build a minimum viable product of the open-source Plastic Scanner and assess its use for the Global Health Engineering Lab. The design will build on the existing expertise of the project and incorporate the newest hardware version, DB 2.3. The overall goal of the project is to assess the scanner’s adaptability to the use case in Malawi and improve it. This scanner is functional and could be repoduced for testing and development purposes, however it is not adviced to use it in any commercial application. Further development is still needed.

<img src="Images/final_device_on_table.png" alt="Alt text" width="30%" />

## Introduction
Plastic is a widely used material, inexpensive to manufacture, and therefore a common choice for single-use products. However, due to its extensive use, global plastic pollution and its subsequent degradation in landfills into micro- and nanoplastics are threatening ecosystems (Walker & Fequet, 2023). Estimates show that plastic consumption will further increase from 464 Mt/y in 2020 to over 700 Mt/y by 2050 (Dokl et al., 2024). Plastic recycling is established, but it first requires thorough separation of plastic components. Industrial plastic scanners use various spectroscopic methods, with Near-Infrared (NIR) spectroscopy being the main technology (Werner et al., 2023). Light is emitted, and the reflected relative light intensity from the material is measured to identify its composition.

Especially for small-scale recycling facilities and projects with limited financial resources, commercial plastic scanners are too expensive. Sorting is therefore done manually, which is often very labor-intensive and of low accuracy. An example is WASTE Adviser (WASTE Adviser Website, 2025), an NGO in Malawi that is establishing plastic beneficiation pathways for PET and HDPE. For the quality of the upcycled product, it is essential that the plastic is sorted with high accuracy; otherwise, the recycled product suffers from embrittlement.

The plastic scanner is a low cost device, utiliziing cheap infrared LED's and a machine learning model to identify the following six types of plastic:

![plastic types](Images/symbols_plastic.jpg)

## How it works
The PlasticScanner allows you to identify six different plastic types by using an approach called **discreate near-infrared (NIR) spectroscopy **. The IR LED's on the Scanner module emitt light one-by-one for a short period of time. Each time the reflectance of the emitted light is measured by a Photodiode. 

<img src="Images/NIR_spectrometry_illustration.png" alt="Alt text" width="50%" />

This results in a Spectralanalysis of the measured reflectance.The plastic identification works by comparing each measurment with the distinct reflectance spectrum of the six plastic types. 

![normalized reflectance](Images/Reflectance.png)

## Handheld Scanner
![components](Images/components.png)

Components:
- 3d printed enclosure
- scanning PCB (I2C for data transfer)
- Lilygo T-RGB UI interface, ESP32-S3 Microcontroller, USB-C port, Battery recharging, (I2C for data transfer)
- 3.7V 2600mAh Battery
- Button
- Power on/off shifter

## Further improvements
- Firmware User Interface 
- Firmware add Color Sensor
- Tetsing
- Supplychain addaptations (IR LED's)
- Find better Button solution (see Hardware Readme)

There is still a lot to improve! The User interface is still quite basic and could be improved to give the user a clear feedback. Testing was not jet done due to time constraints and would be very important. The color sensor is included on the PCB, but was not implemented in the Firmware. For future designs the supplier of the IR LED's should be changed. Since especially high wavelengths are hard to get and tend to have long delivery times. Changing to through hole LED's multiplies the range of suppliers. 
## References

<!-- 
PCB: Lawrence R Kincheloe III, [handheld scanner](https://github.com/LokiMetaSmith/handheld-scanner.git)

Firmware:

CAD: Markus Glavind, [handheld scanner] (https://github.com/Plastic-Scanner/handheld-scanner.git) 
-->

Walker, T. R., & Fequet, L. (2023). Current trends of unsustainable plastic production and micro(nano)plastic pollution. TrAC Trends in Analytical Chemistry, 160, 116984. https://doi.org/10.1016/j.trac.2023.116984

Dokl, M., Copot, A., Krajnc, D., Fan, Y. V., Vujanović, A., Aviso, K. B., Tan, R. R., Kravanja, Z., & Čuček, L. (2024). Global projections of plastic use, end-of-life fate and potential changes in consumption, reduction, recycling and replacement with bioplastics to 2050. Sustainable Production and Consumption, 51, 498–518. https://doi.org/10.1016/j.spc.2024.09.025 

Werner, T., Taha, I., & Aschenbrenner, D. (2023). An overview of polymer identification techniques in recycling plants with focus on current and future challenges. Procedia CIRP, 120, 1381–1386. https://doi.org/10.1016/j.procir.2023.09.180

WASTE Adviser Website. (2025, Oktober 7). https://www.wasteadvisersmw.org/eu-building-better
