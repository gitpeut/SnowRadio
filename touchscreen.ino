#ifdef USETOUCH
#include "fonts.h"
#include "tft.h"
#include "touch.h"
#include "owm.h"

RadioButton touchbutton [ TOUCHBUTTONCOUNT  ] ;
volatile bool screenUpdateInProgress = false;
volatile int  nextprevChannel = 0;
//------------------------------------------------------------------------------------------
// from Bodmer's example.

void touch_calibrate()
{
  uint16_t calData[5];
  bool     calDataOK = false;

  // check if calibration file exists and size is correct
  if (RadioFS.exists(CALIBRATION_FILE)) {

    File f = RadioFS.open(CALIBRATION_FILE, "r");
    if (f) {
      if (f.readBytes((char *)calData, 14) == 14)calDataOK = true;
      f.close();      
    }
  }

  if (calDataOK) {
    // calibration data valid
    tft.setTouch(calData);
  } else {
    // data not valid so recalibrate
    tft.fillScreen(TFT_BLACK);
    tft.setCursor(20, 0);
    tft.setTextFont(2);
    tft.setTextSize(1);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);

    tft.println("Touch corners as indicated");

    tft.setTextFont(1);
    tft.println();

    tft.calibrateTouch(calData, TFT_MAGENTA, TFT_BLACK, 15);
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.println("Calibration complete!");
    tft.println("Delete " CALIBRATION_FILE " to re-calibrate");
    

    // store data
    File f = RadioFS.open(CALIBRATION_FILE, "w");
    if (f) {
      f.write((const unsigned char *)calData, 14);
      f.close();
    }
    delay( 8000 );
  }
  log_i("written/found uint16_t caldata={ %u, %u, %u, %u,%u }", calData[0], calData[1], calData[2], calData[3], calData[4]); 
}

//--------------------------------------------------------------------

void IRAM_ATTR touch_found (){
int started=0;
   xTaskNotifyFromISR( touchTask, 1 ,eSetValueWithOverwrite, &started); 
   if( started ){
      portYIELD_FROM_ISR();
   }
}

//--------------------------------------------------------------------
int what_button(){

  //find out which button has been pressed
  // return the button number 
  
  uint16_t          touch_x = 0, touch_y = 0;
 //static uint32_t   touch_count=0;
  int               startbutton, endbutton;
  boolean           pressed;
    
  if( xSemaphoreTake( tftSemaphore, 50 ) == pdTRUE){   
    pressed = tft.getTouch(&touch_x, &touch_y, 50);
    xSemaphoreGive( tftSemaphore);
    if( !pressed ) return(-1);
  }else{
    return(-1);
  }
  
  //log_d("%d - touch (%s) at %u,%u", touch_count++, pressed?"pressed":"not pressed", touch_x,touch_y);

    if ( currDisplayScreen == RADIO ){
      startbutton = 0; // BUTTON_AV or BUTTON_UP depending on USEINPUTSELECT
      endbutton   = BUTTON_ITEM0;
  }

  if ( currDisplayScreen != RADIO && currDisplayScreen != STNSELECT  ){
      startbutton = 0; //BUTTON_AV or BUTTON_UP depending on USEINPUTSELECT
      endbutton   = BUTTON_PREV;
      if ( currDisplayScreen == POWEROFF) endbutton = BUTTON_MUTE;
  }

  if ( currDisplayScreen == STNSELECT  ){
      startbutton = BUTTON_ITEM0;
      endbutton   = BUTTON_DOWN;
  }
  
  for ( int i = startbutton ; i < endbutton; ++i ){
    if ( touchbutton[i].contains(touch_x, touch_y) ) return( i ); 
  }
  return( -1 );  
} 
//--------------------------------------------------------------------
void touch_setup(){

int itemw = 218; 
int itemh = 35;

draw_left_frames();
  
tft.setFreeFont( BTN_FONT );
  
  log_i( "ram free before buttons %d",ESP.getFreeHeap()  );

  touchbutton[BUTTON_AV].init       ( 250, 11, (char *)"0",BUTW,BUTH ); 
  touchbutton[BUTTON_BLUETOOTH].init( 306, 11, (char *)"1",BUTW,BUTH );
  touchbutton[BUTTON_RADIO].init    ( 362, 11, (char *)"2",BUTW,BUTH );   
  touchbutton[BUTTON_STOP].init     ( 418, 11, (char *)"3",BUTW,BUTH );
   
  touchbutton[BUTTON_MUTE].init( 250, 269, (char *)"7",BUTW,BUTH, (char *)"8" );
  touchbutton[BUTTON_PREV].init( 306, 269, (char *)"4",BUTW,BUTH );
  touchbutton[BUTTON_NEXT].init( 362, 269, (char *)"6",BUTW,BUTH );
  touchbutton[BUTTON_LIST].init( 418, 269, (char *)"5",BUTW,BUTH );

  touchbutton[BUTTON_ITEM0].init( 250, 12,           (char *)"", itemw, itemh);
  touchbutton[BUTTON_ITEM1].init( 250, 18 + 1*itemh, (char *)"", itemw, itemh);
  touchbutton[BUTTON_ITEM2].init( 250, 24 + 2*itemh, (char *)"", itemw, itemh);
  touchbutton[BUTTON_ITEM3].init( 250, 30 + 3*itemh, (char *)"", itemw, itemh);
  touchbutton[BUTTON_ITEM4].init( 250, 36 + 4*itemh, (char *)"", itemw, itemh);
  touchbutton[BUTTON_ITEM5].init( 250, 42 + 5*itemh, (char *)"", itemw, itemh);

  touchbutton[BUTTON_LEFTLIST].init(  260, 269, (char *)"<", 50, 40);
  touchbutton[BUTTON_QUITLIST].init(  334, 269, (char *)"=", 50, 40);
  touchbutton[BUTTON_RIGHTLIST].init( 408, 269, (char *)">", 50, 40);
    
  log_i( "ram free after buttons %d",ESP.getFreeHeap());
    
  log_i("touch button setup done"); 
}

//--------------------------------------------------------------------
void draw_buttons( int startidx ){

  int   startbutton, endbutton;
  int   stationidx;  
  int   playing_button = -1;


  if ( currDisplayScreen == RADIO ){
      startbutton = 0;
      endbutton   = (int)BUTTON_ITEM0;
  }

  if ( currDisplayScreen != RADIO && currDisplayScreen != STNSELECT  ){
      startbutton = 0;
      endbutton   = (int)BUTTON_PREV;
      if ( currDisplayScreen == POWEROFF) endbutton = BUTTON_MUTE;
  }

  if ( currDisplayScreen == STNSELECT ){
      log_i("-- draw_buttons %d (max = %d)", startidx, stationCount);
      startbutton = BUTTON_ITEM0;
      endbutton   = (int)BUTTON_DOWN;
            
      stationidx  = startidx;
      int playing_station = getStation();
      
      for ( int i = 0; i < 6 ; ++i ){
          if ( stationidx >= stationCount )stationidx = 0;
          char *last = stations[ stationidx ].name;
          if ( strlen( last) > 18 ) { //omit the first word of the station name
             while( *last && *last != ' ') ++last;
             if ( *last ) ++last; 
          }
          touchbutton[ startbutton + i ].set_symbol_normal( last );

          if ( stationidx == playing_station ){
             playing_button = startbutton + i;  
             //log_d("playing buttton = %d", playing_button);           
          }
                  
          touchbutton[ startbutton + i ].stationidx = stationidx; 

          ++stationidx;
      }
      
  }
  
  for ( int i = startbutton ; i < endbutton; ++i ){
      //log_i( "draw button %d", i);
      if ( i == (int)BUTTON_MUTE ){
        touchbutton[i].draw( MuteActive ); 
      }else{
          touchbutton[i].draw( ( i == playing_button)?true: false );
      }
      delay(20); // give others a chance and avoid taskwatchdog
      //log_i( "end draw button %d", i);
  }
  
  switch( currDisplayScreen ){
    case RADIO:
        touchbutton[BUTTON_RADIO].draw( true );
        break;
    case LINEIN:
        touchbutton[BUTTON_AV].draw( true );
        break;
    case BLUETOOTH:
        touchbutton[BUTTON_BLUETOOTH].draw( true );
        break;
    case POWEROFF:
        touchbutton[BUTTON_STOP].draw( true );
        break;    
    default:    
        break;
  }
  
}


//--------------------------------------------------------------------
void drawStationScreen(){
   
  grabTft();

  releaseTft();
  drawBmp("/frames/list_frame.bmp", 243, 3 );  
//  tft.setTextColor( TFT_WHITE );
//  tft.setTextFont( bigfont);     

  tft_showmeta( true );
  
}
//--------------------------------------------------------------------
void drawRadioScreen(){ 

  grabTft();
  tft.fillRect (240,   0, 238, 318, TFT_MY_BLACK);
  delay (1);
  releaseTft();
    
  draw_right_frames ();
   
  if ( playingStation >= 0 )tft_showstation( getStation() );
  
}

//--------------------------------------------------------------------

void drawMode( bool traffic_only ){

  if ( currDisplayScreen == RADIO || currDisplayScreen == STNSELECT )return;
  char  modetext[32];
  bool  draw_txt = true;
  uint16_t text_color = TFT_MY_GRAY, background_color=TFT_MY_DARKGRAY;
  int mode_x=0;
         
  #ifdef USEOWM
    if ( ! traffic_only )drawForecastSprite(); 
  #endif

            

#ifdef USETRAFFIC

        sprintf( modetext, "%d", traffic_info.level );
        draw_txt = false;
        
        switch( traffic_info.level ){
            case 0:
            case 1:
            case 2:
            case 3:
                  background_color=TFT_MY_GREEN;
                  break;
            case 4:            
            case 5:            
            case 6:            
                  background_color=TFT_MY_YELLOW;
                  text_color = TFT_MY_DARKGRAY;
                  break;
            case 7:
            case 8:
            case 9:
                  background_color=TFT_RED;
                  break;
            default:
                  background_color=TFT_WHITE;
                  break;            
        }
        if ( traffic_info.stale ) background_color=TFT_WHITE;

#endif  

  if ( ! traffic_only ){
          
            mode_x = 387 ;

            switch ( currDisplayScreen ){
                case LINEIN:
                  if ( draw_txt ) sprintf( modetext, "LINE IN");  
                  drawBmp("/frames/line_frame.bmp", 243,  66 );
                  drawBmp("/frames/btn_close.bmp",  306, 269 );
                  break;
                case BLUETOOTH:
                  if ( draw_txt ) sprintf( modetext, "BLUETOOTH");  
                  drawBmp("/frames/bt_frame.bmp",  243,  66 );
                  drawBmp("/frames/btn_close.bmp",  306, 269 );
                  break;
                case POWEROFF:
                  if ( draw_txt ) sprintf( modetext, "POWER OFF");  
                  drawBmp("/frames/power_frame.bmp", 243,  66 );
                  drawBmp("/frames/power_close.bmp", 250, 269 );  
                  mode_x = 359 ;
                  break;
                default:
                  return;  
            }                   
  } 
  
  delay(5);
  
  if ( !draw_txt ){
    mode_x = mode_x - 7;
  }
  
  grabTft(); 
  
  if ( currDisplayScreen != POWEROFF ){
    tft.fillRect( 300, 268, 170, 42, background_color);  
  }else{
    tft.fillRect( 250, 268, 220, 42, background_color);      
  }
  if ( draw_txt ){
      // no traffic info  
        tft.setTextColor( text_color, background_color ); 
        tft.setFreeFont( STATION_FONT );
        tft.setTextDatum(TC_DATUM);  
        tft.drawString( modetext, mode_x, 279 );
        tft.setTextDatum(TL_DATUM);
  }else{
        tft.setTextColor( text_color, background_color ); 
        tft.setFreeFont( TRAFFIC_NUM );
        tft.setTextDatum(TC_DATUM);  
        tft.drawString( modetext, mode_x, 275 );
        
        tft.setTextDatum(TR_DATUM);   // align to the right side of the string  
        tft.setFreeFont( TRAFFIC_TIME );
        sprintf( modetext, "(%s)", traffic_info.time.c_str() );
        mode_x = tft.width() - 10;
        tft.drawString( modetext, mode_x, 275+7 );
        tft.setTextDatum(TL_DATUM); // rest to default datum 
         
  }
  releaseTft();
    

}



//--------------------------------------------------------------

void drawScreen( screenPage newscreen){ 
  int playing_station;
  int stationidx = 0;
   
  if ( newscreen == RADIO || newscreen == STNSELECT){
    log_i("releasing radio semaphore, if needed");
    if ( xSemaphoreGetMutexHolder( radioSemaphore ) != NULL ){
     xSemaphoreGive( radioSemaphore);     

     log_i("released radio semaphore %s", MuteActive?"mute is active":"mute is NOT active" );
    }
  }else{                   

    if ( xSemaphoreGetMutexHolder( radioSemaphore ) == NULL ){  
     xSemaphoreTake(radioSemaphore, portMAX_DELAY);
    }

  }
  
  screenUpdateInProgress = true;
  currDisplayScreen = newscreen;                  

// make sure screens are displayed after setting currDisplayScreen
// screen functions call functions that test this.

  if ( newscreen != RADIO && newscreen != STNSELECT ){


     drawWeather();

 
  }


  if ( newscreen == STNSELECT ){

    playing_station = getStation();
    stationidx      = playing_station - 3;
    if ( stationidx <=  0 ) stationidx = stationCount  + stationidx;
    
    drawStationScreen();
  }

  if ( newscreen == RADIO ){
    drawRadioScreen();
  }
  
  draw_buttons( stationidx );

  drawMode();  
  if ( currDisplayScreen != POWEROFF )save_last_volstat(2); //save last mode

  if ( newscreen != STNSELECT ){
      if ( MuteActive ) toggleMute();
  }

  screenUpdateInProgress = false;
  
  if ( newscreen == RADIO )tft_showmeta();
  
  toggleStop();

}

//--------------------------------------------------------------------

void touch_process( void *param){
  uint32_t    touch_command;
  bool        irqset = false;
  int         button_pressed= -1;
  bool        noreset= false;
  int         newidx; 
  screenPage  newScreen = currDisplayScreen;
  static screenPage  previousScreen = currDisplayScreen;
  
  Serial.printf("Touch running on core %d\n", xPortGetCoreID()); 
  
  pinMode( TOUCH_IRQ, INPUT_PULLUP);
  
  touch_setup();
  
  // if last mode was not radio init the other mode
  screenPage lastmode = (screenPage)get_last_volstat(2);
  lastmode = RADIO;
  //lastmode = BLUETOOTH;
  log_d ("last mode is %d, forcing RADIO", lastmode);

  while ( !stationCount) delay(50);
  
  log_i("drawing screen");
  drawScreen( lastmode );
  log_i("finished drawing screen");
    
  while(1){
    
    if ( ! irqset ){
      attachInterrupt( TOUCH_IRQ, touch_found, FALLING );
      irqset = true;
    }
    
    //log_d("Touch read...");
    xTaskNotifyWait(0,0,&touch_command,portMAX_DELAY);
      
    //the touch irq also fires when positions are read. To avoid
    //endless loops, disable the interrupt when reading positions. 
    //Found here: https://github.com/PaulStoffregen/XPT2046_Touchscreen
    //
    
      detachInterrupt( TOUCH_IRQ );
      irqset=false;

      button_pressed = what_button();
      if ( button_pressed >= 0 && 
          button_pressed != BUTTON_MUTE && 
          button_pressed != BUTTON_STOP ){
            touchbutton[button_pressed].draw( true );  
      }
        
      switch( button_pressed ){
        case BUTTON_DOWN:
             log_i("T lower volume");
             change_volstat( -1, gVolume );
             break; 
        case BUTTON_UP:
             log_i("T increase volume");
             change_volstat( 1, gVolume );
             break;
        case BUTTON_AV:
             newScreen = LINEIN; 
             noreset = true; 
             break;       
        case BUTTON_BLUETOOTH:
             newScreen = BLUETOOTH;
             noreset = true; 
             break;       
        case BUTTON_RADIO:
             newScreen = RADIO; 
             noreset = true;             
             break;
        case BUTTON_LIST:
             newScreen = STNSELECT; 
             break;      
        case BUTTON_PREV:
             log_i("previous station");
             change_volstat( -1, gStation );
             noreset = true;
             break;
        case BUTTON_NEXT:
             log_i("next station");
             change_volstat( 1, gStation );
             noreset = true;
             break;
        case BUTTON_MUTE:
             log_d("mute - toggle from %d",  MuteActive );
             noreset = true; 
             toggleMute();
             break;
        case BUTTON_STOP:
             log_i("stop");
             if ( currDisplayScreen == POWEROFF ){              
              newScreen = previousScreen; // leave here to prevent compiler whining 'not used'
              newScreen = RADIO; // force to RADIO after poweron
             }else{
              previousScreen = currDisplayScreen; // could be RADIO, it's a matterof taste
              newScreen = POWEROFF;
             } 
             noreset = true; 
             break;
        case BUTTON_ITEM0:
        case BUTTON_ITEM1:
        case BUTTON_ITEM2:
        case BUTTON_ITEM3:
        case BUTTON_ITEM4:
        case BUTTON_ITEM5:

             setStation( touchbutton[button_pressed].stationidx , -1 );                       
             draw_buttons( touchbutton[ BUTTON_ITEM0].stationidx );
             noreset = true;
             break; 
        case BUTTON_QUITLIST:
             newScreen = RADIO; 
             break;  
        case BUTTON_LEFTLIST:
             newidx      = touchbutton[ BUTTON_ITEM0].stationidx - 6;
             if ( newidx <= 0 ){
                 log_d("*newidx < 0 : %d", newidx);
                 newidx = stationCount + newidx; 
                 log_d("*newidx modified to %d", newidx);
             }
             draw_buttons( newidx );
             break; 
        case BUTTON_RIGHTLIST:            
             newidx      = touchbutton[ BUTTON_ITEM5].stationidx + 1;
             draw_buttons( newidx );
             break;                                  
        default:
            //Unknown or no button pressed
            break;      
      }// end switch
      
      if ( button_pressed >= 0 && noreset == false ){
        log_i("print normal");
        touchbutton[ button_pressed].draw( false );  
      }

      if ( newScreen != currDisplayScreen ){
        drawScreen( newScreen );
      }    

      noreset = false;

  }// end while

}
//--------------------------------------------------------------------

int touch_init(){

    touch_calibrate();

    xTaskCreatePinnedToCore( 
         touch_process,                                      // Task to handle special functions.
         "Touch",                                            // name of task.
         4*1024,                                          // Stack size of task
         NULL,                                               // parameter of the task
         TOUCHTASKPRIO,                                      // priority of the task
         &touchTask,                                         // Task handle to keep track of created task 
         TOUCHCORE  );                                       // processor core


log_i("touch task started");                
return(0);  
}

#endif
