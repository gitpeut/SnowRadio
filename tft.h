#ifndef TFT_H
#define TFT_H

#include <TFT_eSPI.h>
    
#define TFT_MY_GOLD         0xE68E
#define TFT_MY_SILVER       0xC615
#define TFT_MY_BLACK        0x31A7  // Темный задний фон
#define TFT_MY_GRAY         0xBDF7  // Контур, шрифты
#define TFT_MY_DARKGRAY     0x424A  // Заливка отсеков
#define TFT_MY_BLUE         0x5C5C  // Часы, спект, активные кнопки.
#define TFT_MY_RED          0xEAA9
#define TFT_MY_GREEN        TFT_DARKGREEN
#define TFT_MY_YELLOW       TFT_MY_GOLD


#define TFT_ROTATION       3

// --- fonts ---
//just to make it resemble the examples
#define GFXFF 1
// symbol font for volume, battery and buttons

#include "fonts.h"    //freefont for stations
#define BTN_FONT      &radio_button_font    // Кнопки управления радио и режимами // Radio and mode control buttons
#define ARROW_FONT    &arrow                // Шрифт для стрелок переключения станции и mute. // Font for the station switching arrows and the symbol for mute mode.
#define INDICATOR_FONT  &indicator          // Индикаторы аккумулятора и громкости
#define LABEL_FONT    &FreeSansBold6pt8b    // Шрифт для метки погоды, описание погоды, день недели, название трека. // Font for the weather label, weather description, day of the week, track name.
#define DATE_FONT     &FreeSansBold9pt8b    // Шрифт для следующей станции и текущей даты // Font for the next station and the current date
#define STATION_FONT  &FreeSansBold10pt8b   // Шрифт списка станций // Station List Font
#define TIME_FONT     &FreeSansBold44pt7b   // Шрифт для часов. Только цифры. // The font for the clock. Only numbers.
#define LABELW_FONT   &Digital_8            // Шрифт, цифры для даты в прогнозах погоды. // Font, numbers for the date in the weather forecasts.
#define NUM_FONT      &Digital_16pt8b       // Шрифт, цифры для параметров погоды + специальные символы. // Font, numbers for weather parameters + special characters.
#define META_FONT     &MetaBold11           //
#define TRAFFIC_TIME  &traffic_time         // only numbers font for traffic time
#define TRAFFIC_NUM   &traffic_num          // traffic severity

//clock font, use built in 7segment font
//If a freefont is to be used, showClock code show be changed
// to load the font in the sprite, see showStation for an example 
//on how to do that.
 
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
  #define TFTCLOCKT 51
#else
  #define TFTCLOCKT  0
#endif

#define TFTCLOCKH 80
#define TFTCLOCKB (TFTCLOCKT + TFTCLOCKH)

#define TFTSPECTRUMH 80
#define TFTSPECTRUMT (TFTCLOCKB + 32 )    
#define TFTSPECTRUMB (TFTSPECTRUMT + TFTSPECTRUMH )

#define TFTSTATIONH  25
#define TFTSTATIONT  (TFTSPECTRUMB + 1)    
#define TFTSTATIONB  ( TFTSTATIONT + TFTSTATIONH )

#define TFTMETAH  25
#define TFTMETAT  (TFTSTATIONB)     
#define TFTMETAB  (TFTMETAT + TFTMETAH)

#define BUTOFFSET ((230 - 4*BUTW)/5)

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

extern struct        metaInfo meta;
extern void          broadcast_meta(bool reset=false);
extern volatile bool screenUpdateInProgress;
extern volatile int  nextprevChannel;
extern volatile int  trafficCount; // in asyncwebserver
 
void IRAM_ATTR grabTft();
void IRAM_ATTR releaseTft();
//void showVolume( int percentage , bool force = false);
//void showBattery( bool force = false);
void drawBmp(const char *filename, int16_t x, int16_t y, TFT_eSprite *sprite=NULL, bool show=true );
void showClock ( bool force = false);
void toggleStop( bool nostop=true );
void tft_message( const char  *message1, const char *message2 );
void tft_create_meta( int spritew = 0);
void tft_fillmeta();
void tft_showmeta(bool resetx=false);

void latin2utf( unsigned char *latin, unsigned char **utf ); // in asyncwebserver.ino

#endif
