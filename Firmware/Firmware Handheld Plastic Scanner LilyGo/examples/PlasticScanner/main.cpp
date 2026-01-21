#include "tlc59208.h"
#include <LilyGo_RGBPanel.h>
#include <LV_Helper.h>
#include <Wire.h>
#include <Arduino.h>
#include <SparkFun_Qwiic_Scale_NAU7802_Arduino_Library.h>
#include <EloquentTinyML.h>
#include "barbie.h"
#include "ps_logo.h"
#include <Adafruit_AS7341.h>
#include <EEPROM.h> 


#define N_INPUTS 8 // darkreading + 7 leds, no color sensor for now
#define N_RESULTS 6
#define TENSOR_ARENA_SIZE 2 * 1024
#define N_OUTPUTS 7 // number of LEDs
#define RESET 2
#define ADDR 0x20
#define UI_BUTTON_PIN 0   // GPIO0 (BOOT)

// Time thresholds
#define SHORT_PRESS_MS 50      // min duration for a "short press"
#define LONG_PRESS_MS 2000     // min duration for "long press" to enter sleep
#define IDLE_TIMEOUT_MS (5 * 60 * 1000UL)  // 5 minutes idle timeout
#define RESULTS_DISPLAY_MS (1 * 60 * 1000UL)  // Results display timeout (1 minute)

NAU7802 nau;
TLC59208 tlc;
LilyGo_RGBPanel panel;
Adafruit_AS7341 as7341;

Eloquent::TinyML::TfLite<N_INPUTS, N_RESULTS, TENSOR_ARENA_SIZE> tf;
float baseline[N_OUTPUTS]; // store calibration readings
bool calibrated = false;

volatile bool scanRunning = false;
RTC_DATA_ATTR int bootCount = 0;
unsigned long lastActivity = 0;

//plastic scanner logo
lv_obj_t *logo_img = nullptr;
lv_timer_t *splash_timer = nullptr;
lv_timer_t *results_timer = nullptr;  // Timer to auto-return to main UI after results

//storage for results
long darkReading = 0;
lv_obj_t *titleLabel;
long ledReadings[N_OUTPUTS];
long val;
int scanIndex = 0;

// conversion of raw readings
double allReadings[9]; // combined IR + color readings
float input[N_INPUTS];

//same as in training model
const char* PLASTIC_NAMES[] = {
    "PP",
    "PS",
    "PET",
    "HDPE",
    "PVC",
    "LDPE"
};

static const int NUM_PLASTICS = sizeof(PLASTIC_NAMES) / sizeof(PLASTIC_NAMES[0]);
// Confidence thresholds
#define CONFIDENCE_MIN 0.50f      // minimum probability to accept result
#define CONFIDENCE_GAP 0.15f      // gap between best and second-best

// UI Customization
#define MAIN_RESULT_FONT &lv_font_montserrat_40    // Font size for main prediction
#define PROB_FONT &lv_font_montserrat_24           // Font size for class probabilities
#define RESULT_Y_OFFSET -40                         // Y offset for main result from center (negative = up)
#define LINE_Y_OFFSET 20                            // Y offset of divider from main result
#define PROB_START_OFFSET 10                        // Y offset where probabilities start from divider
#define PROB_SPACING 30                             // Vertical spacing between probability labels

float lastResults[N_RESULTS];
int predictedIndex = -1;
float predictedConfidence = 0.0f;
bool predictionInconclusive = true;

void interpretResults(float* results) {
    int best = 0;
    int second = -1;

    for (int i = 1; i < N_RESULTS; i++) {
        if (results[i] > results[best]) {
            second = best;
            best = i;
        } else if (second < 0 || results[i] > results[second]) {
            second = i;
        }
    }

    float bestVal = results[best];
    float secondVal = (second >= 0) ? results[second] : 0.0f;

    predictedIndex = best;
    predictedConfidence = bestVal;

    // Inconclusive logic
    predictionInconclusive =
        (bestVal < CONFIDENCE_MIN) ||
        ((bestVal - secondVal) < CONFIDENCE_GAP);
}


// Beer's law + normalization
void preprocessInput() {
    float ratios[N_INPUTS];

    // --- Beer's law ratios ---
    ratios[0] = (darkReading != 0) ? 
                ((float)ledReadings[0] / (float)darkReading) : 0.0f;

    for (int i = 1; i < N_OUTPUTS; i++) {
        ratios[i] = (ledReadings[i - 1] != 0) ?
                    ((float)ledReadings[i] / (float)ledReadings[i - 1]) : 0.0f;
    }

    // Fill remaining inputs if any
    for (int i = N_OUTPUTS; i < N_INPUTS; i++) {
        ratios[i] = 0.0f;
    }

    // --- Find per-scan min/max ---
    float minVal = ratios[0];
    float maxVal = ratios[0];

    for (int i = 1; i < N_INPUTS; i++) {
        if (ratios[i] < minVal) minVal = ratios[i];
        if (ratios[i] > maxVal) maxVal = ratios[i];
    }

    // Prevent divide-by-zero
    float range = maxVal - minVal;
    if (range < 1e-6f) range = 1.0f;

    // --- Normalize per scan ---
    for (int i = 0; i < N_INPUTS; i++) {
        input[i] = (ratios[i] - minVal) / range;

        // Clamp (safety)
        if (input[i] < 0.0f) input[i] = 0.0f;
        if (input[i] > 1.0f) input[i] = 1.0f;
    }

    // Debug
    Serial.print("min=");
    Serial.print(minVal, 6);
    Serial.print(" max=");
    Serial.println(maxVal, 6);
}

// Enter deep sleep
void goToDeepSleep() {
    Serial.println("Entering deep sleep. Press BOOT button (GPIO0) to wake.");

    // Wait until button released to avoid glitches
    while (digitalRead(UI_BUTTON_PIN) == LOW) {
        delay(10);
        Serial.flush();
    }

    delay(50); // small debounce

    // EXT1 wake: wake when GPIO0 is LOW
    esp_sleep_enable_ext1_wakeup(1ULL << GPIO_NUM_0, ESP_EXT1_WAKEUP_ANY_LOW);

    esp_deep_sleep_start();
}

void calibrate() {
    Serial.println("Place spectralon and press button...");
    delay(500); // let user settle
    for (int i = 0; i < N_OUTPUTS; i++) {
        baseline[i] = nau.getReading();
        Serial.print("Baseline LED "); Serial.print(i); Serial.print(": "); Serial.println(baseline[i]);
    }
    calibrated = true;
}

//plastic scanner logo
void createMainUI() {
    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0xFFFFFF), 0); // black background

    lv_obj_clean(lv_scr_act());

    lv_obj_t *label = lv_label_create(lv_scr_act());
    lv_label_set_text(label, "Hello PlasticScanner");
    lv_obj_center(label);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_32, 0); //32px

    lv_obj_t *btn = lv_btn_create(lv_scr_act());
    lv_obj_set_size(btn, 120, 50);
    lv_obj_align_to(btn, label, LV_ALIGN_OUT_BOTTOM_MID, 0, 0);

    lv_obj_t *btn_label = lv_label_create(btn);
    lv_label_set_text(btn_label, "Start Scan");
    lv_obj_center(btn_label);
}

void splashTimeout(lv_timer_t *timer) {
    if (timer) {
        lv_timer_del(timer);
    }
    splash_timer = nullptr;

    if (logo_img != nullptr) {
        lv_obj_del(logo_img);
    }
    logo_img = nullptr;

    createMainUI();
}

// Return to main UI after results display timeout
void resultsTimeout(lv_timer_t *timer) {
    if (timer) {
        lv_timer_del(timer);
    }
    results_timer = nullptr;

    createMainUI();
}


//result UI
void createResultsUI() {
    lv_obj_clean(lv_scr_act());

    // Background color logic 
    lv_color_t bg;

    Serial.print("DEBUG: predictionInconclusive="); Serial.print(predictionInconclusive);
    Serial.print(" predictedConfidence="); Serial.println(predictedConfidence, 3);

    if (predictionInconclusive) {
        bg = lv_color_hex(0xFF0000); // red (pure red)
        Serial.println("Setting RED");
    } else if (predictedConfidence < 0.60f) {
        bg = lv_color_hex(0xFF8000); // orange
        Serial.println("Setting ORANGE");
    } else {
        bg = lv_color_hex(0x00FF00); // green (pure green)
        Serial.println("Setting GREEN");
    }

    lv_obj_set_style_bg_color(lv_scr_act(), bg, 0);
    lv_obj_set_style_bg_opa(lv_scr_act(), LV_OPA_COVER, 0);

    // Main result label - centered in middle of screen
    lv_obj_t *result = lv_label_create(lv_scr_act());
    lv_obj_set_width(result, LV_PCT(100));
    lv_obj_set_style_text_align(result, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(result, lv_color_black(), 0);
    lv_obj_set_style_text_font(result, MAIN_RESULT_FONT, 0);

    if (predictionInconclusive) {
        lv_label_set_text(result, "INCONCLUSIVE");
    } else {
        lv_label_set_text_fmt(
            result,
            "%s\n%.0f%%",
            PLASTIC_NAMES[predictedIndex],
            predictedConfidence * 100.0f
        );
    }

    lv_obj_align(result, LV_ALIGN_CENTER, 0, RESULT_Y_OFFSET);

    //  Divider 
    lv_obj_t *line = lv_line_create(lv_scr_act());
    static lv_point_t line_points[] = {{10, 0}, {230, 0}};
    lv_line_set_points(line, line_points, 2);
    lv_obj_align_to(line, result, LV_ALIGN_OUT_BOTTOM_MID, 0, LINE_Y_OFFSET);
    lv_obj_set_style_line_color(line, lv_color_black(), 0);
    lv_obj_set_style_line_width(line, 2, 0);

    // Class probabilities 
    for (int i = 0; i < N_RESULTS; i++) {
        lv_obj_t *l = lv_label_create(lv_scr_act());
        lv_obj_set_width(l, LV_PCT(100));
        lv_label_set_text_fmt(
            l,
            "%s: %.1f%%",
            PLASTIC_NAMES[i],
            lastResults[i] * 100.0f
        );
        lv_obj_set_style_text_align(l, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_style_text_color(l, lv_color_black(), 0);
        lv_obj_set_style_text_font(l, PROB_FONT, 0);
        lv_obj_align_to(l, line, LV_ALIGN_OUT_BOTTOM_MID, 0, PROB_START_OFFSET + i * PROB_SPACING);
    }
}



//loading UI
void createLoadingUI() {
    lv_obj_clean(lv_scr_act());

    lv_obj_t *label = lv_label_create(lv_scr_act());
    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0xFFFFFF), 0); 
    lv_label_set_text(label, "Scanning...");
    lv_obj_set_style_text_font(label, &lv_font_montserrat_32, 0); 
    lv_obj_center(label);
}


// Reset TLC59208
static void resetTLC() {
    digitalWrite(RESET, LOW);
    delayMicroseconds(1);
    digitalWrite(RESET, HIGH);
    delayMicroseconds(1);
}

// Softmax function
void softmax(float* x, int n) {
    float maxVal = x[0];
    for (int i = 1; i < n; i++) {
        if (x[i] > maxVal) maxVal = x[i];
    }

    float sum = 0.0f;
    for (int i = 0; i < n; i++) {
        x[i] = expf(x[i] - maxVal);  // numerical stability
        sum += x[i];
    }

    for (int i = 0; i < n; i++) {
        x[i] /= sum;
    }
}


// Run scan
void runScan() {
    scanRunning = true;
    Serial.println("Starting scan.");

    darkReading = nau.getReading();
    Serial.print("Dark reading: "); Serial.println(darkReading);
    delay(500);

    for (int i = 0; i < N_OUTPUTS; i++) {
        tlc.on(i);
        delay(10);
        ledReadings[i] = nau.getReading();
        Serial.print("LED "); Serial.print(i); Serial.print(" reading: "); Serial.println(ledReadings[i]);
        delay(50);
        resetTLC(); //new
        tlc.begin(); // do not call tlc.off(i) makes UI crash, but tlc.begin() works as well for tuning
        // Only call lv_timer_handler every other iteration to avoid stack issues
        if (i % 2 == 0) {
            lv_timer_handler();
        }
    }

    // Final lv_timer_handler call
    lv_timer_handler();

    // Preprocess input 
    preprocessInput();

    float results[N_RESULTS];

    Serial.println("Normalized input:");
    for (int i = 0; i < N_INPUTS; i++) {
        Serial.print(input[i], 6);
        Serial.print(", ");
    }
    Serial.println();


    Serial.println("Predicting material...");
    tf.predict(input, results);
    softmax(results, N_RESULTS);

    //float predicted = tf.predict(input);
    Serial.print("ML output: "); //Serial.println(predicted);
    // Print all probabilities
    for (int i = 0; i < N_RESULTS; i++) {
        Serial.print("Class ");
        Serial.print(i);
        Serial.print(": ");
        Serial.println(results[i], 6);
    }

    // Store results
    for (int i = 0; i < N_RESULTS; i++) {
        lastResults[i] = results[i];
    }

    // Interpret
    interpretResults(results);

    scanRunning = false;
    
}


// Setup
void setup() {
    Serial.begin(115200);
    delay(2000);
    bootCount++;
    Serial.print("Boot number: "); Serial.println(bootCount);

    pinMode(RESET, OUTPUT);
    pinMode(UI_BUTTON_PIN, INPUT_PULLUP);

    lastActivity = millis();

    // Initialize display
    if (!panel.begin()) {
        while (1) {
            Serial.println("Error initializing T-RGB");
            delay(1000);
        }
    }

    beginLvglHelper(panel);

    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(lv_scr_act(), LV_OPA_COVER, 0);

    // Show splash logo
    logo_img = lv_img_create(lv_scr_act());
    lv_img_set_src(logo_img, &ps_logo);
    lv_obj_set_style_img_recolor(logo_img, lv_color_hex(0xFF0000), 0);  // Red color for logo
    lv_obj_center(logo_img);
    lv_img_set_zoom(logo_img, 128);  // 256 = 100%, 192 ≈ 75%, 128 = 50%
    lv_obj_center(logo_img); //re center after scaling


    // Start 5s timer
    splash_timer = lv_timer_create(splashTimeout, 5000, nullptr);

    panel.setBrightness(10);

    Wire.begin();
    Wire.setClock(100000);  // Slow down I2C to 100kHz to prevent bus conflicts
    nau.begin();
    tlc.begin();
    tf.begin(barbie);


    if (nau.begin() == false) {
        Serial.println("NAU7802 not detected.");
    } else {
        Serial.println("NAU7802 detected!");
        nau.setGain(NAU7802_GAIN_4);
    }
}


// Main loop
void loop() {
    lv_timer_handler();

    // Handle GPIO0 press for short vs long press
    static bool lastButtonState = HIGH;
    static unsigned long pressStart = 0;

    bool buttonState = digitalRead(UI_BUTTON_PIN);
    unsigned long now = millis();

    // Button pressed
    if (lastButtonState == HIGH && buttonState == LOW) {
        pressStart = now;
    }

    // Button released
    if (lastButtonState == LOW && buttonState == HIGH) {
        unsigned long pressDuration = now - pressStart;

        if (pressDuration >= LONG_PRESS_MS) {
            // Long press → deep sleep
            goToDeepSleep();
        } else if (pressDuration >= SHORT_PRESS_MS) {
            // Short press → scan
            if (!scanRunning) {
                lastActivity = now;
                createLoadingUI();
                lv_timer_handler();   // force draw
                delay(200);

                scanRunning = true;
                runScan();          // BLOCKING scan
                scanRunning = false;

                createResultsUI();
                
                // Cancel any existing results timer
                if (results_timer) {
                    lv_timer_del(results_timer);
                    results_timer = nullptr;
                }
                
                // Start 1 minute timer to auto-return to main UI
                results_timer = lv_timer_create(resultsTimeout, 60000, nullptr);
                //lv_timer_handler();
            }
        }
    }

    lastButtonState = buttonState;

    // Auto deep sleep after idle timeout
    if (!scanRunning && (millis() - lastActivity > IDLE_TIMEOUT_MS)) {
    goToDeepSleep();
    }

    delay(10);
}
