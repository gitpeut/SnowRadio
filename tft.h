#ifndef TFT_H
#define TFT_H


#define TFT_OLDREALGOLD       0xC676 //0xE5EC
    
#define TFT_MY_GOLD        0xE68E
#define TFT_MY_SILVER      0xC615

#define TFT_REALGOLD       TFT_MY_GOLD
#define TFT_ROTATION       2

//just to make it resemble the examples
#define GFXFF 1
// symbol font for volume, battery and buttons
#include "fonts.h"
//freefont for stations
#define STATION_FONT FreeMonoBold12pt7b

//clock font, use built in 7segment font
//If a freefont is to be used, showClock code show be changed
// to load the font in the sprite, see showStation for an example 
//on how to do that.
 
#define CLOCK_FONT 7

#ifdef MONTHNAMES_EN
const char *monthnames[] = {"January","February","March","April","May","June","July","August","September","October","November","December"};
const char *daynames[] = {"Su","Mo","Tu","We","Th", "Fr","Sa"};
#else
const char *monthnames[] = {"januari","februari","maart","april","may","juni","juli","augustus","september","october","november","december"};
const char *daynames[] = {"zo","ma","di","wo","do", "vr","za"};
#endif

void IRAM_ATTR grabTft();
void IRAM_ATTR releaseTft();
void showVolume( int percentage );
void drawBmp(const char *filename, int16_t x, int16_t y, TFT_eSprite *sprite=NULL, bool show=true );

typedef struct{
  char     *name;
  uint16_t w;
  uint16_t h;
  uint8_t  *data;   
}bmpFile;

#include <vector>
extern std::vector<bmpFile*> bmpCache;

// to display image in sprite, provide poiter to sprite. 
// to display on screen, omit this argument or fill it with NULL );
// display in sprite:
//  TFT_eSprite sprite;
//  drawBmp( "/tets.bmp", 5,5, &sprite); 
// display on screen:
//  tft ( will use TFT_eSPI tft variable declared globally )
//  drawBmp("/OranjeRadio24.bmp", 55, 15 );

#define TFTINDICH 32                             //height
#define TFTINDICT 61                             //top
#define TFTINDICB (TFTINDICT + TFTINDICH )       //bottom 83

#define TFTDAYH  10                                //height
#define TFTDAYT  51                                //top     
#define TFTDAYB (TFTDAYT + TFTDAYH)                //bottom  94

#define TFTCLOCKH 80                                //height
#define TFTCLOCKT (TFTDAYB + 1)                   //top     
#define TFTCLOCKB (TFTCLOCKT + TFTCLOCKH)           //bottom 175

#define TFTSPECTRUMH 50                             // height
#define TFTSPECTRUMT (TFTCLOCKB + 1 )               // top    
#define TFTSPECTRUMB (TFTSPECTRUMT + TFTSPECTRUMH ) //bottom  226

#define TFTSTATIONH  50                             //height
#define TFTSTATIONT  (TFTSPECTRUMB + 1)             //top      
#define TFTSTATIONB  ( TFTSTATIONT + TFTSTATIONH )  //bottom  277

#define BUTOFFSET ((tft.width() - 4*BUTW)/5)

// neopixel 
#define NEONUMBER   10

#define PIX_BLACKC    0
#define PIX_WAKEUP    1
#define PIX_RIGHT     2
#define PIX_LEFT      3
#define PIX_CONFIRM   4
#define PIX_SCROLLUP  8
#define PIX_STOP      10
#define PIX_MUTE      11
#define PIX_UNMUTE    12
#define PIX_SCROLLDOWN 16
#define PIX_BLACK     911

#define PIX_BLINKRED    21
#define PIX_BLINKGREEN  22
#define PIX_BLINKBLUE   23
#define PIX_BLINKYELLOW 24

#define PIX_RED         41
#define PIX_GREEN       42
#define PIX_BLUE        43
#define PIX_YELLOW      44

#define PIX_DECO        51

void toggleStop( bool nostop=true );

#endif
