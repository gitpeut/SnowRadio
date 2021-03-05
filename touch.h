
#define BUTW  50
#define BUTH  40

#ifdef USETOUCH

#ifndef TOUCH_H
#define TOUCH_H

#include "btn_radio.h"




class RadioButton : public TFT_eSPI_Button {
  private:
  
  static const uint16_t outline    = TFT_MY_SILVER;
  static const uint16_t buttonfill = TFT_REALGOLD;
  static const uint16_t textcolor  = TFT_BLACK;
  
  public:
  
    RadioButton(void){};

    void set_symbol_normal( char *newvalue ){
        if ( symbol_normal != NULL )free( symbol_normal );
        symbol_normal = NULL;
        if( newvalue )symbol_normal = ps_strdup( newvalue );
    }

    void set_symbol_invert( char *newvalue ){
        if ( symbol_invert != NULL )free( symbol_invert );
        symbol_invert = NULL;
        if( newvalue )symbol_invert = ps_strdup( newvalue );
    }
          
    void draw( bool invert=false, char *longname = NULL){
   
      if ( longname == NULL ){
        if ( invert && symbol_invert != NULL){
          longname = symbol_invert;
        }else{
          longname = symbol_normal;
        }  
      }

      if ( bmp_normal == NULL && bmp_invert == NULL ){
        xSemaphoreTake( tftSemaphore, portMAX_DELAY);
        if ( strlen( longname ) == 1 ){
            //use Btn_radio font
            tft.setFreeFont(&Btn_radio);
            if ( invert ){
              tft.setTextColor( TFT_BLUE, TFT_BLACK);             
            }else{
              tft.setTextColor( TFT_REALGOLD, TFT_BLACK);
            }
            log_d( "fontbutton - draw %s", longname);
            tft.drawString( longname, x,y );          
        }else{ 
            tft.setFreeFont( STATION_FONT );
             
            drawButton( invert, longname);      
        }
        xSemaphoreGive( tftSemaphore );
      }
      
      if ( bmp_normal != NULL && !invert ){
        log_i ( "drawing %s", bmp_normal); 
        drawBmp( bmp_normal, x, y );  
      }
      if ( bmp_invert != NULL && invert ){
        log_i ( "drawing %s", bmp_invert); 
        drawBmp( bmp_invert, x, y );  
      }
    }

    void init( int argx, int argy, char *argsymbol_normal, int arg_butw = BUTW, int arg_buth = BUTH, char *argsymbol_invert = NULL, char *argbmp_normal=NULL, char *argbmp_invert=NULL ){
      x = argx;
      y = argy;
      butw = arg_butw;
      buth = arg_buth;

      set_symbol_normal( argsymbol_normal );
      set_symbol_invert( argsymbol_invert );
        
      if ( argbmp_normal != NULL) bmp_normal = ps_strdup( argbmp_normal );
      if ( argbmp_invert != NULL) bmp_invert = ps_strdup( argbmp_invert );
      if ( argbmp_normal && argbmp_invert == NULL ) bmp_invert = bmp_normal;
      
      initButtonUL(&tft, x, y, arg_butw, arg_buth, outline, buttonfill, textcolor, symbol_normal, 1);
      setLabelDatum(0, 0, MC_DATUM );  // x-delta, ydelta ( a bit down) , what place in the text ( here: the middle)
                                       // in the middle of the label ("datum"). y offset of 7 for font 4 = bigfont
                                       // y offset 0 for FreeSansBold10pt8b    
    }

   
    int x;
    int y;
    int butw;
    int buth;
    char *symbol_normal;
    char *symbol_invert;
    char *bmp_normal;
    char *bmp_invert;
    int  stationidx;
    
};


#define TOUCHBUTTONCOUNT 30 
#ifdef USEINPUTSELECT
  enum buttontype{
  BUTTON_AV,
  BUTTON_BLUETOOTH,
  BUTTON_RADIO,
  BUTTON_STOP,
  BUTTON_MUTE,
  BUTTON_PREV,
  BUTTON_NEXT,
  BUTTON_LIST,
  BUTTON_ITEM0,
  BUTTON_ITEM1,
  BUTTON_ITEM2,
  BUTTON_ITEM3,
  BUTTON_ITEM4,
  BUTTON_ITEM5,
  BUTTON_ITEM6,
  BUTTON_ITEM7,
  BUTTON_LEFTLIST,
  BUTTON_QUITLIST,
  BUTTON_RIGHTLIST,
  BUTTON_DOWN,
  BUTTON_UP
  };
#else
  enum buttontype{
  BUTTON_UP,
  BUTTON_DOWN,
  BUTTON_RADIO,
  BUTTON_STOP,
  BUTTON_MUTE,
  BUTTON_PREV,
  BUTTON_NEXT,
  BUTTON_LIST,
  BUTTON_ITEM0,
  BUTTON_ITEM1,
  BUTTON_ITEM2,
  BUTTON_ITEM3,
  BUTTON_ITEM4,
  BUTTON_ITEM5,
  BUTTON_ITEM6,
  BUTTON_ITEM7,
  BUTTON_LEFTLIST,
  BUTTON_QUITLIST,
  BUTTON_RIGHTLIST,
  BUTTON_AV,
  BUTTON_BLUETOOTH,
  };
#endif


#define CALIBRATION_FILE "/TouchCal.dat"

extern RadioButton touchbutton [ TOUCHBUTTONCOUNT  ] ;
extern bool MuteActive;

int touch_init(); 

#endif
#endif
