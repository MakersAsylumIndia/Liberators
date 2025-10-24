#include <SPI.h>
#include "DEV_Config.h" 
#include "LCD_Driver.h"
#include "GUI_Paint.h"
#include "image.h"

// --- TTS LIBRARIES ---
#include <WiFi.h>
#include <AudioFileSourceHTTPStream.h>
#include <AudioGeneratorMP3.h>
#include <AudioOutputI2S.h>

// --- TTS CONFIGURATION ---
const char* WIFI_SSID = "Makers Asylum";
const char* WIFI_PASS = "Makeithappen";

// Your PC running server.py
const char* HOST = "192.168.68.154";
const uint16_t PORT = 5000;
const char* LANG = "en"; // TTS language

// I2S pins to MAX98357A
const int I2S_BCLK = 26; 
const int I2S_LRC  = 25; 
const int I2S_DOUT = 13; 

// --- BUTTON & DISPLAY CONFIGURATION ---
#define BUTTON_PIN 15 // Define the button pin (INPUT_PULLUP used in setup)
const unsigned long ACTION_DELAY_MS = 500; // Fixed non-blocking delay for synchronization

// --- TTS GLOBAL OBJECTS ---
AudioGeneratorMP3 *mp3 = nullptr;
AudioFileSourceHTTPStream *file = nullptr;
AudioOutputI2S *out = nullptr;

// --- STATE MACHINE VARIABLES ---
unsigned long lastPressTime = 0;
bool showMessage = false;

// Define the states for the non-blocking button logic
enum ButtonState {
    STATE_IDLE,
    STATE_DELAYING,
    STATE_ACTION_COMPLETE
};
ButtonState currentButtonState = STATE_IDLE; 

// ----------------------------------------------------
// FORWARD DECLARATIONS
// ----------------------------------------------------
void tts_setup(const char* ssid, const char* pass);
void tts_loop();
void speak(const String& text);
String urlEncode(const String& s);
bool isPlaying();


void setup() {
    Serial.begin(115200);
    Serial.println("LCD 1.83 inch Test with Button and Smooth TTS");

    // --- BUTTON/LCD SETUP ---
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    System_Init();
    LCD_Init();
    LCD_SetBacklight(1000); 
    Paint_NewImage(LCD_WIDTH, LCD_HEIGHT, 0, WHITE);
    Paint_Clear(WHITE);
    LCD_Display();

    // --- TTS SETUP (Connect WiFi, Init I2S) ---
    tts_setup(WIFI_SSID, WIFI_PASS);
}


void loop() {
    // 1. Run the TTS playback logic (MUST be called on every loop, and should be FIRST)
    tts_loop();

    int buttonState = digitalRead(BUTTON_PIN);

    // --- State Machine Logic ---
    switch (currentButtonState) {
        
        case STATE_IDLE: {
            // Check for button press and ensure no audio is playing
            if (buttonState == LOW && !isPlaying()) {
                // Initial confirmed press: debounce check
                lastPressTime = millis();
                // Non-blocking wait for debounce
                while (digitalRead(BUTTON_PIN) == LOW && (millis() - lastPressTime < 50)) {
                    tts_loop(); // Keep audio running just in case
                }

                if (digitalRead(BUTTON_PIN) == LOW) { // Confirmed press
                    lastPressTime = millis();
                    currentButtonState = STATE_DELAYING;
                    // Serial.println("Button Pressed. Entering 500ms non-blocking delay..."); // REMOVED
                }
            }
            // Add a small idle processing delay to let other ESP32 tasks run if nothing is happening
            if (buttonState == HIGH && !isPlaying()) {
                 delay(1); 
            }
            break;
        }

        case STATE_DELAYING: {
            // Wait for the synchronization delay to expire. tts_loop() is still running.
            if (millis() - lastPressTime >= ACTION_DELAY_MS) {
                
                // --- SYNCHRONIZED ACTION POINT ---
                
                // 1. Draw LCD message - ONLY DRAW ONCE
                showMessage = true;
                Paint_Clear(WHITE);
                Paint_DrawString_EN(30, 60, "Hi I am Boot", &Font24, WHITE, BLACK);
                LCD_Display(); // This is the blocking call. Do it once.

                // 2. Start Speaking - This is a fast, non-blocking call.
                // Serial.println("Delay complete. Starting LCD update and speak()."); // REMOVED
                speak("Hi I am Boot");
                
                lastPressTime = millis(); // Reset timer for message clear logic
                currentButtonState = STATE_ACTION_COMPLETE;
            } 
            // If the button is released during the delay, cancel the action
            else if (buttonState == HIGH) {
                 currentButtonState = STATE_IDLE;
                 // Serial.println("Button released during delay. Action canceled."); // REMOVED
            }
            break;
        }

        case STATE_ACTION_COMPLETE: {
            // Check for conditions to clear the LCD and reset to IDLE
            bool audioJustFinished = isPlaying() == false;
            bool timeElapsed = (millis() - lastPressTime > 2000);

            // Condition 1: Time elapsed OR audio finished
            if (showMessage && (timeElapsed || audioJustFinished)) {
                showMessage = false;
                Paint_Clear(WHITE);
                LCD_Display(); // Blocking call, only executed once
            }
            
            // Condition 2: Reset to IDLE only when both button is released AND audio is done/cleared
            if (buttonState == HIGH && !showMessage) {
                currentButtonState = STATE_IDLE; 
            }
            break;
        }
    }
}


// ----------------------------------------------------
// TTS FUNCTION IMPLEMENTATION
// ----------------------------------------------------

/**
 * @brief Simple URL encoder for Arduino String.
 */
String urlEncode(const String& s) {
    const char* hex = "0123456789ABCDEF";
    String out; out.reserve(s.length() * 3);
    for (size_t i = 0; i < s.length(); i++) {
        char c = s[i];
        bool safe = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                    (c >= '0' && c <= '9') || c == '-' || c == '_' ||
                    c == '.' || c == '~';
        if (safe) {
            out += c;
        } else if (c == ' ') {
            out += "%20";
        } else {
            out += '%';
            out += hex[(c >> 4) & 0xF];
            out += hex[c & 0xF];
        }
    }
    return out;
}

/**
 * @brief Initializes WiFi and I2S audio output.
 */
void tts_setup(const char* ssid, const char* pass) {
    Serial.println("Connecting to WiFi...");

    WiFi.begin(ssid, pass);
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        Serial.print(".");
        attempts++;
    }
    
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("\nWiFi connection failed! TTS will not work.");
        return;
    }

    Serial.println("\nWiFi connected!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());

    out = new AudioOutputI2S();
    out->SetPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
    out->SetChannels(1);       // mono
    out->SetGain(1.0);         // 0.0–1.0
}

/**
 * @brief Keeps the audio playback running (MUST be called often in loop).
 */
void tts_loop() {
    if (mp3 && mp3->isRunning()) {
        if (!mp3->loop()) {
            // Serial.println("Playback finished."); // REMOVED
            mp3->stop();
        }
    }
}

/**
 * @brief Checks if the audio generator is actively playing sound.
 */
bool isPlaying() {
    return mp3 && mp3->isRunning();
}


/**
 * @brief Stops current playback and starts playing the given text via TTS server.
 */
void speak(const String& text) {
    // 1. Stop and clean up previous playback resources
    if (isPlaying()) mp3->stop();
    if (mp3) { delete mp3; mp3 = nullptr; }
    if (file) { delete file; file = nullptr; }

    // 2. Build the TTS request URL
    String url = "http://" + String(HOST) + ":" + String(PORT) +
                 "/tts?lang=" + String(LANG) + "&text=" + urlEncode(text);

    // Serial.print("Requesting URL: "); // REMOVED
    // Serial.println(url); // REMOVED

    // 3. Start new playback
    file = new AudioFileSourceHTTPStream(url.c_str());
    mp3  = new AudioGeneratorMP3();
    
    if (out) {
        mp3->begin(file, out);
    } else {
        // Serial.println("Error: Audio output not initialized."); // REMOVED
    }
}