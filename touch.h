#ifdef USETOUCH

#ifndef TOUCH_H
#define TOUCH_H



#define BUTW  50
#define BUTH  40


class RadioButton : public TFT_eSPI_Button {
  private:
  
  static const uint16_t outline    = TFT_MY_SILVER;
  static const uint16_t buttonfill = TFT_REALGOLD;
  static const uint16_t textcolor  = TFT_BLACK;
  
  public:
  
    RadioButton(void){};

    void set_symbol( char *newvalue ){
        if ( symbol )free( symbol );
        symbol = NULL;
        if( newvalue )symbol = ps_strdup( newvalue );
    }
              
    void draw( bool invert=false, char *longname = NULL){

      if ( longname == NULL )longname = symbol;
     
      xSemaphoreTake( tftSemaphore, portMAX_DELAY);
      //tft.setFreeFont( STATION_FONT );        
      drawButton( invert, longname);
      xSemaphoreGive( tftSemaphore );
      
      if ( bmp_normal != NULL && !invert ){
        log_i ( "drawing %s", bmp_normal); 
        drawBmp( bmp_normal, x, y );  
      }
      if ( bmp_invert != NULL && invert ){
        log_i ( "drawing %s", bmp_invert); 
        drawBmp( bmp_invert, x, y );  
      }
      
                       
    }

    void init( int argx, int argy, char *argsymbol, int arg_butw = BUTW, int arg_buth = BUTH, char *argbmp_normal=NULL, char *argbmp_invert=NULL ){
      x = argx;
      y = argy;
      butw = arg_butw;
      buth = arg_buth;

      set_symbol( argsymbol );
        
      if ( argbmp_normal != NULL) bmp_normal = ps_strdup( argbmp_normal );
      if ( argbmp_invert != NULL) bmp_invert = ps_strdup( argbmp_invert );
      if ( argbmp_normal && argbmp_invert == NULL ) bmp_invert = bmp_normal;
      
      initButtonUL(&tft, x, y, arg_butw, arg_buth, outline, buttonfill, textcolor, symbol, 1);
      setLabelDatum(-1, 3 );      
      //draw(false, symbol);
    }

    int x;
    int y;
    int butw;
    int buth;
    char *symbol;
    char *bmp_normal;
    char *bmp_invert;
    int  stationidx;
    
};


#define TOUCHBUTTONCOUNT 30 
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


#define CALIBRATION_FILE "/TouchCal.dat"

extern RadioButton touchbutton [ TOUCHBUTTONCOUNT  ] ;
int touch_init(); 

#endif
#endif
