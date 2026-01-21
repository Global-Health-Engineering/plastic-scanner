#include "tlc59208.h"
#include <LilyGo_RGBPanel.h>
#include <LV_Helper.h>
#include <Wire.h>
#include <Arduino.h>
#include <SparkFun_Qwiic_Scale_NAU7802_Arduino_Library.h>
#include <EloquentTinyML.h>
#include "barbie.h"
#include "ps_logo.h"


#define N_INPUTS 12
#define N_RESULTS 1
#define TENSOR_ARENA_SIZE 2 * 1024
#define N_OUTPUTS 8
#define RESET 2
#define ADDR 0x20
#define UI_BUTTON_PIN 0   // GPIO0 (BOOT)

// Time thresholds
#define SHORT_PRESS_MS 50      // min duration for a "short press"
#define LONG_PRESS_MS 2000     // min duration for "long press" to enter sleep
#define IDLE_TIMEOUT_MS (1 * 60 * 1000UL)  // 1 minutes idle timeout

NAU7802 nau;
TLC59208 tlc;
LilyGo_RGBPanel panel;

Eloquent::TinyML::TfLite<N_INPUTS, N_RESULTS, TENSOR_ARENA_SIZE> tf;

volatile bool scanRunning = false;
RTC_DATA_ATTR int bootCount = 0;
unsigned long lastActivity = 0;

//plastic scanner logo
lv_obj_t *logo_img = nullptr;
lv_timer_t *splash_timer = nullptr;

//storage for results
long darkReading = 0;
long ledReadings[N_OUTPUTS];
long val;
int scanIndex = 0;

// Enter deep sleep
void goToDeepSleep() {
    Serial.println("Entering deep sleep. Press BOOT button (GPIO0) to wake.");

    // Wait until button released to avoid glitches
    while (digitalRead(UI_BUTTON_PIN) == LOW) {
        delay(10);
    }

    delay(50); // small debounce

    // EXT1 wake: wake when GPIO0 is LOW
    esp_sleep_enable_ext1_wakeup(1ULL << GPIO_NUM_0, ESP_EXT1_WAKEUP_ANY_LOW);

    esp_deep_sleep_start();
}

//plastic scanner logo
void createMainUI() {
    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0xFFFFFF), 0); // LilyGo default blue

    lv_obj_clean(lv_scr_act());

    lv_obj_t *label = lv_label_create(lv_scr_act());
    lv_label_set_text(label, "Hello PlasticScanner");
    lv_obj_center(label);

    lv_obj_t *btn = lv_btn_create(lv_scr_act());
    lv_obj_set_size(btn, 120, 50);
    lv_obj_align_to(btn, label, LV_ALIGN_OUT_BOTTOM_MID, 0, 0);

    lv_obj_t *btn_label = lv_label_create(btn);
    lv_label_set_text(btn_label, "Start Scan");
    lv_obj_center(btn_label);
}

void splashTimeout(lv_timer_t *timer) {
    lv_timer_del(timer);
    splash_timer = nullptr;

    if (logo_img) {
        lv_obj_del(logo_img);
        logo_img = nullptr;
    }

    createMainUI();
}

//result UI
void createResultsUI() {
    lv_obj_clean(lv_scr_act());   // <-- first
    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0xFFFFFF), 0); // then set bg
    lv_obj_set_style_bg_opa(lv_scr_act(), LV_OPA_COVER, 0);

    // Dark reading label
    lv_obj_t *label = lv_label_create(lv_scr_act());
    lv_obj_set_width(label, LV_PCT(100));
    lv_label_set_text_fmt(label, "Dark: %ld", darkReading);
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 10);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(label, lv_color_black(), 0);

    // LED readings
    for (int i = 0; i < N_OUTPUTS; i++) {
        lv_obj_t *l = lv_label_create(lv_scr_act());
        lv_obj_set_width(l, LV_PCT(100));
        lv_label_set_text_fmt(l, "LED %d: %ld", i, ledReadings[i]);
        lv_obj_align(l, LV_ALIGN_TOP_MID, 0, 40 + i * 20);
        lv_obj_set_style_text_align(l, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_style_text_color(l, lv_color_black(), 0);
    }
}

//loading UI
void createLoadingUI() {
    lv_obj_clean(lv_scr_act());

    lv_obj_t *label = lv_label_create(lv_scr_act());
    lv_label_set_text(label, "Scanning...");
    lv_obj_center(label);
}


// Reset TLC59208
static void resetTLC() {
    digitalWrite(RESET, LOW);
    delayMicroseconds(1);
    digitalWrite(RESET, HIGH);
    delayMicroseconds(1);
}


// Run scan
void runScan() {

    scanRunning = true;
    Serial.println("Starting scan.");
    long val = nau.getReading();
    Serial.print("Dark reading: "); Serial.println(val);
    delay(500);

    for (int i = 0; i < N_OUTPUTS; i++) {
        tlc.on(i);
        delay(10);
        val = nau.getReading();
        Serial.print("LED "); Serial.print(i); Serial.print(" reading: "); Serial.println(val);
    }

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

    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_black(), 0);
    lv_obj_set_style_bg_opa(lv_scr_act(), LV_OPA_COVER, 0);

    // Show splash logo
    logo_img = lv_img_create(lv_scr_act());
    lv_img_set_src(logo_img, &ps_logo);
    lv_obj_center(logo_img);
    lv_img_set_zoom(logo_img, 128);  // 256 = 100%, 192 ≈ 75%, 128 = 50%
    lv_obj_center(logo_img); //re center after scaling


    // Start 5s timer
    splash_timer = lv_timer_create(splashTimeout, 5000, nullptr);

    panel.setBrightness(10);

    Wire.begin();
    nau.begin();
    tlc.begin();

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

                runScan();

                createResultsUI();
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
