#ifndef NULPINS_H
#define NULPINS_H

#define NEOPIN        2

//gesture sensor
#define GSDAPIN       27
#define GSCLPIN       26
#define GINTPIN       25

//vs1053
#define VS_CS_PIN     5
//SCLK                18
//MISO                19
#define VS_DCS_PIN    15
#define VS_DREQ_PIN   22
//MOSI                23
#define VS1053_RST    21 // you can also connect this to the ESP32 reset

//battery
#define BATPIN        36

//tft
//SDO(MISO) TFT_MISO  19 // defined in TFT_eSPI header 
//SDI(MOSI) TFT_MOSI  23
//SCK       TFT_SCLK  18
//#define TFT_CS         4
//#define TFT_RST       -1  // connects to the ESP32 reset pin
//#define TFT_DC        13
//#define TFT_LED       32
//#define TFT_BL        TFT_LED

//touch 
//TOUCH_IRQ           35 // defined in TFT_eSPI header 
//TOUCH_CS            33 


#endif
