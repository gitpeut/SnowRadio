#ifdef USETOUCH

#ifndef TOUCH_H
#define TOUCH_H

#include "tft.h"

#define BUTW  50
#define BUTH  31


class RadioButton : public TFT_eSPI_Button {
  private:
  
  static const uint16_t outline = TFT_DARKGREY;
  static const uint16_t buttonfill = TFT_REALGOLD;
  static const uint16_t textcolor = TFT_BLACK;
  
  public:
  
    RadioButton(void){};
              
    void draw( bool invert=false, String longname = ""){
      
      tft.setFreeFont(&indicator);     
     
      xSemaphoreTake( tftSemaphore, portMAX_DELAY);
      drawButton( invert, longname);
      xSemaphoreGive( tftSemaphore);                 
    }

    void init( int argx, int argy, char argsymbol ){
      x = argx;
      y = argy;
      symbol = String(argsymbol);
      initButtonUL(&tft, x, y, BUTW, BUTH, outline, buttonfill, textcolor, (char *)symbol.c_str(), 1);
      setLabelDatum(-1, 3 );
      draw(false, symbol);
    }

    
    int x;
    int y;
    String symbol;
    String inverse_symbol;
    
};


#define TOUCHBUTTONCOUNT 6
enum buttontype{
BUTTON_MUTE,
BUTTON_DOWN,
BUTTON_UP,
BUTTON_PREV,
BUTTON_NEXT,
BUTTON_STOP
};


#define CALIBRATION_FILE "/TouchCal.dat"

extern RadioButton touchbutton [ TOUCHBUTTONCOUNT  ] ;

#endif
#endif
