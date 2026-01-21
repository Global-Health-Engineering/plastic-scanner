
#include "tlc59208.h" // Small homemade definition library for TLC59208
#include <LilyGo_RGBPanel.h>
#include <LV_Helper.h>
#include <Wire.h>     // The I2C library
#include <Arduino.h>  // Some Arduino thing
#include <unity.h>    // Unit testing for C
#include <SparkFun_Qwiic_Scale_NAU7802_Arduino_Library.h>


// ML
#include <EloquentTinyML.h>
//#include <eloquent_tinyml/tensorflow.h>
#include "barbie.h"


#define N_INPUTS 12
#define N_RESULTS 1

// in future projects you may need to tweak this value: it's a trial and error process
#define TENSOR_ARENA_SIZE 2 * 1024
#define NUMPIXELS 1

NAU7802 nau;
TLC59208 tlc;

static void slider_event_cb(lv_event_t *e);
static lv_obj_t *slider_label;


static const int N_OUTPUTS = 8; // Number of LEDs to control
static const int RESET = 2;     // //Define reset pin for TLC59208
static const int ADDR = 0x20;   // The TLC59208 I2C address.

//Button
#define UI_BUTTON_PIN 0   // GPIO0 (BOOT)


Eloquent::TinyML::TfLite<N_INPUTS, N_RESULTS, TENSOR_ARENA_SIZE> tf; 


LilyGo_RGBPanel panel;
bool startScanFlag = false;  // Flag to trigger scan from button

static void btn_event_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    
    if(code == LV_EVENT_CLICKED) {
        Serial.println("Start Scan button pressed!");
        startScanFlag = true;  // Set flag to start scan
    }
}

static void reset()
{
    digitalWrite(RESET, LOW);
    delayMicroseconds(1);
    digitalWrite(RESET, HIGH);
    delayMicroseconds(1);
}

// using highjackt bot to start scan with extrenal
void checkExternalUIButton()
{   
    static bool lastState = HIGH;
    bool now = digitalRead(UI_BUTTON_PIN);

    
    if (lastState == HIGH && now == LOW) {
        // Detect press (HIGH → LOW)
        if (lastState == HIGH && now == LOW) {
            Serial.println("External UI button pressed → start scan");
            startScanFlag = true;
        }
    }

    lastState = now;
}



void runScan()
{
    // pixels.fill(0x02198B); // Set color of NEO Pixel to BLUE.
    // pixels.show();         // Indicate the device is now running a scan.
    // // Function to blink the 8 LEDs and collect the associated data from the ADC.
    // tft.fillScreen(TFT_DARKGREEN);

    // tft.drawCentreString("Scanning", 120, 120, 4);
    Serial.println("Starting scan.");
    // Get a baseline reading without LEDs active.
    long val = nau.getReading();
    Serial.print("Dark reading: ");
    Serial.println(val);

    delay(500);
    // // Flash the LEDs sequentially and read results from the ADC.
    // tft.drawCentreString("Scanning", 120, 120, 4);
    for (int i = 0; i < N_OUTPUTS; i++)
    {
        // Turn on the numerated LED.
        tlc.on(i);
        // Based on comment from Jerry, they tested that 10 ms is fitting for the light to be on, before taking a reading.
        delay(10);
        // Read data from the IR sensor through the ADC.
        long val = nau.getReading();
        // long avgVal = nau.getAverage(5);

        // Serial.print("Read ");
        Serial.println(val);

        // Calculate the progress in % and combine it with a % sign in a string.
        float progress = float(i) / 8 * 100;
        // String progressOutput = String(progress, 2) + "%";
        String progressOutput = String(val);

        // // Make space on the screen.
        // tft.fillRect(0, 140, 240, 180, TFT_DARKGREEN); // clear a region of the display with the background color
        // tft.drawCentreString(progressOutput, 120, 140, 4);
        // delay(500);
        // tlc.off(i);
        // delay(10);
    }

    // tft.fillScreen(TFT_DARKGREEN);
    // tft.drawCentreString("PP", 120, 100, 4);
    // // tft.drawCentreString("...", 120, 140, 4);

    // Waits for button press.
    while (digitalRead(5) == HIGH)
    {
        // Do nothing
    }

    // tft.fillScreen(TFT_DARKGREEN);
    // tft.drawCentreString("Ready to scan", 120, 110, 4);
}

void setup()
{
    Serial.begin(115200);
    delay(5000); //time to open serial
    Serial.println("boot");
    pinMode(5, INPUT_PULLUP);
    //initalize button
    pinMode(UI_BUTTON_PIN, INPUT_PULLUP);
    Serial.println("BOOT button test active (GPIO0)");
    Serial.println("Press external button AFTER boot to test");


    
    Serial.println("=== SETUP STARTED ===");

    //** Four initialization methods */

    // Automatically determine the touch model to determine the initialization screen type. If touch is not available, it may fail.
    bool rslt = panel.begin();

    // Specify 2.1-inch semicircular screen
    // https://www.lilygo.cc/products/t-rgb?variant=42407295877301
    // bool rslt = panel.begin(LILYGO_T_RGB_2_1_INCHES_HALF_CIRCLE);

    // Specified as a 2.1-inch full-circle screen
    // https://www.lilygo.cc/products/t-rgb
    // bool rslt = panel.begin(LILYGO_T_RGB_2_1_INCHES_FULL_CIRCLE);

    // Specified as a 2.8-inch full-circle screen
    // https://www.lilygo.cc/products/t-rgb?variant=42880799441077
    // bool rslt = panel.begin(LILYGO_T_RGB_2_8_INCHES);
    if (!rslt) {
        while (1) {
            Serial.println("Error, failed to initialize T-RGB"); delay(1000);
        }
    }
    // Call lvgl initialization
    beginLvglHelper(panel);


    lv_obj_t *label = lv_label_create(lv_scr_act());        /*Add a label the current screen*/
    lv_label_set_text(label, "Hello PlasticScanner");                 /*Set label text*/
    lv_obj_center(label);                                   /*Set center alignment*/


    lv_obj_t *btn = lv_btn_create(lv_scr_act());            /*Add a button the current screen*/
    lv_obj_set_size(btn, 120, 50);                          /*Set its size*/
    lv_obj_add_event_cb(btn, btn_event_cb, LV_EVENT_ALL, NULL);  /*Assign a callback to the button*/
    lv_obj_align_to(btn, label, LV_ALIGN_OUT_BOTTOM_MID, 0, 0); /*Set the label to it and align it in the center below the label*/

    lv_obj_t *btn_label = lv_label_create(btn);           /*Add a label to the button*/
    lv_label_set_text(btn_label, "Start Scan");               /*Set the labels text*/
    lv_obj_center(btn_label);

    // Turn on the backlight and set it to the highest value, ranging from 0 to 16
    panel.setBrightness(10);

    Serial.println("Starting Initialization");

    //reset();

    Wire.begin(); // Define the I2C library for use.
    Serial.println("wire begin done");
    nau.begin();
    Serial.println("nau begin done");
    tlc.begin();  // Define the TLC library for use.
    Serial.println("tlc begin done");

   // NAU7802 SETUP //
    if (nau.begin() == false)
    {
        Serial.println("NAU7802 not detected.");
    }
    else  
    {
        Serial.println("NAU7802 detected!");
        nau.setGain(NAU7802_GAIN_4);
    }

   

}


void loop()
{
    // pixels.fill(0x26580F); // Set color of NEO Pixel to GREEN.
    // pixels.show();         // Indicate the device is now running loop code.
    lv_timer_handler();


    // Check external button
    checkExternalUIButton();

    // Check UI button OR external button
    if (startScanFlag) {
        startScanFlag = false;

        // Optional UI feedback
        lv_obj_t *btn = lv_obj_get_child(lv_scr_act(), 1);
        if (btn) {
            lv_obj_t *btn_label = lv_obj_get_child(btn, 0);
            if (btn_label) {
                lv_label_set_text(btn_label, "Scanning...");
            }
        }

        runScan();

        // Restore UI
        if (btn) {
            lv_obj_t *btn_label = lv_obj_get_child(btn, 0);
            if (btn_label) {
                lv_label_set_text(btn_label, "Start Scan");
            }
        }
    }

    delay(10); // Small delay for stability



    // Waits for button press.
    //while (digitalRead(5) == LOW)
    //{
        // Do nothing
   // }
    //reset();
    // Execute a scan and display the results.
    //UNITY_BEGIN();
    //RUN_TEST(runScan);
    //UNITY_END();

 

    // Check if button was pressed
    // if (startScanFlag) {
    //     startScanFlag = false;  // Reset flag
        
    //     // Optional: Update button text during scan
    //     lv_obj_t *btn = lv_obj_get_child(lv_scr_act(), 1);  // Get button
    //     if (btn) {
    //         lv_obj_t *btn_label = lv_obj_get_child(btn, 0);  // Get button label
    //         if (btn_label) {
    //             lv_label_set_text(btn_label, "Scanning...");
    //         }
    //     }
        
    //     // Run the scan
    //     runScan();
     
        
    //     // Restore button text
    //     if (btn) {
    //         lv_obj_t *btn_label = lv_obj_get_child(btn, 0);
    //         if (btn_label) {
    //             lv_label_set_text(btn_label, "Start Scan");
    //         }
    //     }
    // }
    
    
}
