#ifndef TFT_H
#define TFT_H

#include <TFT_eSPI.h>
    
#define TFT_MY_GOLD        0xE68E
#define TFT_MY_SILVER      0xC615
#define TFT_REALGOLD       TFT_MY_GOLD
#define TFT_MY_BLACK        0x31A7  // Темный задний фон
#define TFT_MY_GRAY         0xBDF7  // Контур, шрифты
#define TFT_MY_DARKGRAY     0x424A  // Заливка отсеков
#define TFT_MY_BLUE         0x5C5C  // Часы, спект, активные кнопки.
#define TFT_MY_RED          0xEAA9
#define TFT_MY_GREEN        0x0567
#define TFT_MY_YELLOW       0xF700

#define TFT_ROTATION       0

// --- fonts ---
//just to make it resemble the examples
#define GFXFF 1
// symbol font for volume, battery and buttons
#include "fonts.h"
//freefont for stations
#define STATION_FONT       &FreeSansBold10pt8b //Font with Latin & Cyrillic characters
#define STATION_FONT_LATIN &LatinStation  //Font with extended ASCI characters
#define LIST_FONT          &FreeMonoBold12pt7b //Font with Latin & Cyrillic characters
#define LABEL_FONT         &FreeSansBold6pt8b  //Font with Latin & Cyrillic characters  
#define DATE_FONT          &FreeSansBold10pt8b //Font with Latin & Cyrillic characters
#if defined (METAPOPUP) 
  #define META_FONT        &MetaBold14         //= DejaVu Bold 14 - Font with extended ASCI characters
  #define META_FONTRUS     &FreeSansBold7pt8b  //Font with ASCII and Cyrillic characters (modified y-offset by -4
                                               //to makeit align with the DejaVu Bold 14 font
#else
  #define META_FONT        &MetaBold11         //Font with extended ASCI characters
  #define META_FONTRUS     LABEL_FONT
#endif
//clock font, use built in 7segment font
 
#define CLOCK_FONT 7

// Which page are we on? Home page = normal use, stnslect is list of stations
enum screenPage
{
  RADIO,
  STNSELECT,
  POWEROFF,
  BLUETOOTH,
  LINEIN
};


typedef struct{
  char     *name;
  uint16_t w;
  uint16_t h;
  uint8_t  *data;   
}bmpFile;

#include <vector>
extern std::vector<bmpFile*> bmpCache;
extern SemaphoreHandle_t tftSemaphore;


// to display image in sprite, provide poiter to sprite. 
// to display on screen, omit this argument or fill it with NULL );
// display in sprite:
//  TFT_eSprite sprite;
//  drawBmp( "/tets.bmp", 5,5, &sprite); 
// display on screen:
//  tft ( will use TFT_eSPI tft variable declared globally )
//  drawBmp("/OranjeRadio24.bmp", 55, 15 );

#ifdef USETOUCH
  #define TFTCLOCKT 51                             //top
#else
  #define TFTCLOCKT  0                             //top
#endif

#define TFTCLOCKH 80                                //height
#define TFTCLOCKB (TFTCLOCKT + TFTCLOCKH)           //bottom 175

#define TFTSPECTRUMH 50                             // height
#define TFTSPECTRUMT (TFTCLOCKB + 32 )               // top    
#define TFTSPECTRUMB (TFTSPECTRUMT + TFTSPECTRUMH ) //bottom  226

#define TFTSTATIONH  25                             //height
#define TFTSTATIONT  (TFTSPECTRUMB + 1)             //top      
#define TFTSTATIONB  ( TFTSTATIONT + TFTSTATIONH )  //bottom  277

#define TFTMETAH  36// was 30                                //height
#define TFTMETAT  (TFTSTATIONB)                     //top      
#define TFTMETAB  (TFTMETAT + TFTMETAH)             //bottom  277

#define BUTOFFSET ((tft.width() - 4*BUTW)/5)

// neopixel 
#define NEONUMBER    10

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


struct metaInfo{
  int  metacount = 0;
  int  metalen   = 0;
  char *metar;
  char metadata[1024];
  int  inquote   = 0;
  int  qoffset   = 0;
  int  intervalcount = 0;
  bool ignorequote = false;
  bool intransit = false;
  
};

extern struct metaInfo meta;
extern void          broadcast_meta(bool reset=false);
extern volatile bool screenUpdateInProgress;
extern volatile int  nextprevChannel;

void IRAM_ATTR grabTft();
void IRAM_ATTR releaseTft();
void showVolume( int percentage , bool force = false);
void showBattery( bool force = false);
void drawBmp(const char *filename, int16_t x, int16_t y, TFT_eSprite *sprite=NULL, bool show=true );
void showClock ( bool force = false);
void toggleStop( bool nostop=true );
void tft_message( const char  *message1, const char *message2 );
void tft_create_meta( int spritew = 0);
void tft_fillmeta();
void tft_showmeta(bool resetx=false);

int  txt2fontmap( unsigned char *txt, unsigned char **utf, uint64_t *wordmap );
char *utf8torus(const char *source, char *target);
void latin2utf( unsigned char *latin, unsigned char **utf ); // in asyncwebserver.ino

#endif
