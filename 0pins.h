

















































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
#define PhotoSensPin  39

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
