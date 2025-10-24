#ifndef __LCD_DRIVER_H
#define __LCD_DRIVER_H

#include "DEV_Config.h"

// Wrap function declarations with extern "C" to fix linking errors
#ifdef __cplusplus
extern "C" {
#endif

// LCD Dimensions
#define LCD_WIDTH   240 // LCD width
#define LCD_HEIGHT  280 // LCD height

// Core Driver Functions
void LCD_WriteData_Byte(UBYTE da); 
void LCD_WriteData_Word(UWORD da);
void LCD_WriteReg(UBYTE da);

void LCD_SetCursor(UWORD x1, UWORD y1, UWORD x2, UWORD y2);
void LCD_SetUWORD(UWORD x, UWORD y, UWORD Color);

// Initialization and Display Functions
void System_Init(void); 
void LCD_Display(void); 

void LCD_Init(void);
void LCD_SetBacklight(UWORD Value);
void LCD_Clear(UWORD Color);
void LCD_ClearWindow(UWORD Xstart, UWORD Ystart, UWORD Xend, UWORD Yend, UWORD UWORD);

// Close the extern "C" block
#ifdef __cplusplus
}
#endif

#endif