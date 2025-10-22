#include <SPI.h>
#include "DEV_Config.h" 
#include "LCD_Driver.h"
#include "GUI_Paint.h"
#include "image.h"

// Define the button pin
#define BUTTON_PIN 15

// Variable to track message display time
unsigned long lastPressTime = 0;
bool showMessage = false;

void setup() {
  Serial.begin(115200);
  Serial.println("LCD 1.83 inch Test with Button");

  // Initialize button input with internal pull-up (no external resistor needed)
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  // Initialize the LCD - These functions should now be recognized
  System_Init();
  LCD_Init();
  LCD_SetBacklight(1000);  // Set brightness (adjust as needed)
  
  // Clear the screen to white initially
  Paint_NewImage(LCD_WIDTH, LCD_HEIGHT, 0, WHITE);
  Paint_Clear(WHITE);
  LCD_Display();
}

void loop() {
  int buttonState = digitalRead(BUTTON_PIN);

  // Since we’re using INPUT_PULLUP: LOW = pressed, HIGH = released
  if (buttonState == LOW) {
    // Button pressed → show message and record time
    showMessage = true;
    lastPressTime = millis();
    Paint_Clear(WHITE);
    // Draw the "I am Boot" message
    Paint_DrawString_EN(30, 60, "I am Boot", &Font24, WHITE, BLACK);
    LCD_Display();
  } 
  else if (showMessage && (millis() - lastPressTime > 2000)) {
    // More than 2 seconds since last press → clear screen
    showMessage = false;
    Paint_Clear(WHITE);
    LCD_Display();
  }

  delay(50); // Small debounce delay
}