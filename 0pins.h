#ifndef NULPINS_H
#define NULPINS_H

#define NEOPIN        2

//gesture sensor
#define GSDAPIN       27
#define GSCLPIN       26
#define GINTPIN       25

//vs1053
#define VS_CS_PIN     4
//SCLK                18
//MISO                19
#define VS_DCS_PIN    15
#define VS_DREQ_PIN   22
//MOSI                23
#define VS1053_RST    21 // you can also connect this to the ESP32 reset

//battery
#define BATPIN        36

//tft on HSPI bus, in an attempt to reduce possible hardware and 
// software crosstalk all pins defined in the tft_eSPI user setup.h
//HSPI
//    #define TFT_MOSI       13 
//    #define TFT_SCLK       14
//    #define TFT_MISO       12
//    #define TFT_CS          5 
//    #define TFT_DC          2 
    //touch 
//    #define TOUCH_IRQ      35 
//    #define TOUCH_CS       33

//#define USE_HSPI_PORT

//    #define TFT_LED       32
//    #define TFT_BL        TFT_LED
//    #define TFT_RST       -1 

#endif

/*
#ifndef NULPINS_H
#define NULPINS_H

#define NEOPIN        0

//gesture sensor
#define GSDAPIN       21
#define GSCLPIN       22
#define GINTPIN       36

//vs1053
#define VS_DREQ_PIN   35
#define VS_DCS_PIN    33
#define VS1053_RST    13
#define VS_CS_PIN     32

//MCP23017
#define MCP_RST       26      // MCP23017 port expander
#define MCP_SDA       21
#define MCP_SCL       22
#define MCP_PIN0      0
#define MCP_PIN1      1
#define MCP_PIN2      2
#define MCP_PIN3      3
#define MCP_PIN4      4
#define MCP_PIN5      5
#define MCP_PIN6      6
#define MCP_PIN7      7

#define stnChangePin   34    // Sensor btn for chanel "+".
#define PhotoSensPin   39   //Photoresistor, 10K resistor to ground

//battery
#define BATPIN        2

//tft     User_Setup.h
//#define TFT_MISO  19
//#define TFT_MOSI  23
//#define TFT_SCLK  18
//#define TFT_CS     5     // Chip select control pin
//#define TFT_RST    4     // Reset pin (could connect to RST pin)
//#define TFT_DC    27     // Data Command control pin
//#define TFT_LED   14     // TFT Backligh (LED)
//#define TOUCH_CS  12     // Chip select pin (T_CS) of touch screen
//#define TOUCH_IRQ 15

#define TFT_BL        TFT_LED

#endif
*/
