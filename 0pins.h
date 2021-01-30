#ifndef NULPINS_H
#define NULPINS_H

// neopixel 
#define NEOPIN        2

//gesture sensor

#define GSDAPIN       27
#define GSCLPIN       26
#define GINTPIN       25

//vs1053
#define VS_CS_PIN     5
#define VS_DCS_PIN    15
#define VS_DREQ_PIN   22
#define VS1053_RST    21


//battery
#define BATPIN        36

//tft

#define TFT_CS        4
#define TFT_RST       14  // you can also connect this to the ESP32 reset
#define TFT_DC        13
#define TFT_LED       32
//SDO(MISO)           19
//SCK                 18
//SDI(MOSI)           23


#define TOUCH_IRQ     39 //??
#define TOUCH_CS      32 //??

#endif
