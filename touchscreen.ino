#ifdef USETOUCH
#include "fonts.h"
#include "tft.h"
#include "touch.h"
#include "owm.h"

RadioButton touchbutton [ TOUCHBUTTONCOUNT  ] ;


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
    //tft.fillScreen(TFT_BLACK);
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
  
  if ( pressed ){
        for ( int i = startbutton ; i < endbutton; ++i ){
          if ( touchbutton[i].contains(touch_x, touch_y) ) return( i ); 
        }
  }
  return( -1 );  
} 
//--------------------------------------------------------------------
void touch_setup(){
int topt  = 3;
int bott  = tft.height() - BUTH - 2;
int itemw = tft.width() - 2*topt;
int itemh = 32;
int listbuttonw = 57, listbuttonh = 24; 
int listbuttonxo = ( tft.width() - 3*listbuttonw ) /4;
  
  tft.setFreeFont(&indicator);
  
  int butoffset = BUTOFFSET; 

  log_i( "ram free before buttons %d",ESP.getFreeHeap()  );

#ifdef USEINPUTSELECT
  touchbutton[BUTTON_AV].init( butoffset,                  topt, (char *)"0",BUTW,BUTH ); 
  touchbutton[BUTTON_BLUETOOTH].init( 1*BUTW +2*butoffset, topt, (char *)"1",BUTW,BUTH );
#else
  touchbutton[BUTTON_UP].init( butoffset,                  topt, (char *)"9",BUTW,BUTH ); 
  touchbutton[BUTTON_DOWN].init( 1*BUTW +2*butoffset, topt, (char *)":",BUTW,BUTH );
#endif

  touchbutton[BUTTON_RADIO].init( 2*BUTW +3*butoffset,     topt, (char *)"2",BUTW,BUTH );   
  touchbutton[BUTTON_STOP].init( 3*BUTW + 4*butoffset,     topt, (char *)"3",BUTW,BUTH );
   
  
  touchbutton[BUTTON_MUTE].init( butoffset,                bott, (char *)"7",BUTW,BUTH, (char *)"8" );

  touchbutton[BUTTON_PREV].init( 1*BUTW + 2*butoffset,     bott, (char *)"4",BUTW,BUTH );
  touchbutton[BUTTON_NEXT].init( 2*BUTW + 3*butoffset,     bott, (char *)"6",BUTW,BUTH );
  touchbutton[BUTTON_LIST].init( 3*BUTW + 4*butoffset,     bott, (char *)"5",BUTW,BUTH );

  
  touchbutton[BUTTON_ITEM0].init( topt, topt, (char *)"", itemw, itemh);
  touchbutton[BUTTON_ITEM1].init( topt, 2*topt + 1*itemh, (char *)"", itemw, itemh);
  touchbutton[BUTTON_ITEM2].init( topt, 3*topt + 2*itemh, (char *)"", itemw, itemh);
  touchbutton[BUTTON_ITEM3].init( topt, 4*topt + 3*itemh, (char *)"", itemw, itemh);
  touchbutton[BUTTON_ITEM4].init( topt, 5*topt + 4*itemh, (char *)"", itemw, itemh);
  touchbutton[BUTTON_ITEM5].init( topt, 6*topt + 5*itemh, (char *)"", itemw, itemh);
  touchbutton[BUTTON_ITEM6].init( topt, 7*topt + 6*itemh, (char *)"", itemw, itemh);
  touchbutton[BUTTON_ITEM7].init( topt, 8*topt + 7*itemh, (char *)"", itemw, itemh);
  
  touchbutton[BUTTON_LEFTLIST].init(  listbuttonxo,                   11*topt + 8*itemh, (char *)"", listbuttonw, listbuttonh, NULL,(char*)"/buttons/leftList.bmp",(char*)"/buttons/leftListPressed.bmp");
  touchbutton[BUTTON_QUITLIST].init(  1*listbuttonw + 2*listbuttonxo, 11*topt + 8*itemh, (char *)"", listbuttonw, listbuttonh, NULL,(char*)"/buttons/backList.bmp");
  touchbutton[BUTTON_RIGHTLIST].init( 2*listbuttonw + 3*listbuttonxo, 11*topt + 8*itemh, (char *)"", listbuttonw, listbuttonh, NULL,(char*)"/buttons/rightList.bmp",(char*)"/buttons/rightListPressed.bmp");
    
  log_i( "ram free after buttons %d",ESP.getFreeHeap());
    
//   touchbutton[BUTTON_DOWN].init( butoffset,                 bott, 'X');
//   touchbutton[BUTTON_UP].init  ( 1*BUTW + 2*butoffset,      bott, 'W');

log_i("touch button setup done"); 
}

//--------------------------------------------------------------------
void draw_buttons( int startidx ){

  int   startbutton, endbutton;
  int   stationidx;  
  int   playing_button = -1;


  if ( currDisplayScreen == RADIO ){
      startbutton = 0;
      endbutton   = BUTTON_ITEM0;
  }

  if ( currDisplayScreen != RADIO && currDisplayScreen != STNSELECT  ){
      startbutton = 0;
      endbutton   = BUTTON_PREV;
      if ( currDisplayScreen == POWEROFF) endbutton = BUTTON_MUTE;
  }

  if ( currDisplayScreen == STNSELECT ){
      log_i("-- draw_buttons %d (max = %d)", startidx, stationCount);
      startbutton = BUTTON_ITEM0;
      endbutton   = BUTTON_DOWN;
      //int playing_station = getStation();
      
      stationidx  = startidx;
      int playing_station = getStation();
      
      for ( int i = 0; i < 8 ; ++i ){
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
      if ( i == BUTTON_MUTE ){
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
  tft.fillScreen(TFT_DARKCYAN);
  releaseTft();
  
  tft.setTextColor( TFT_WHITE );
  tft.setTextFont( bigfont);     
 
  
}
//--------------------------------------------------------------------
void drawRadioScreen(){ 
  

  grabTft();
    tft.fillScreen(TFT_BLACK);
  releaseTft();
    
   
    if ( playingStation >= 0 )tft_showstation( getStation() );

  
}

//--------------------------------------------------------------------

void drawMode(){

  if ( currDisplayScreen == RADIO || currDisplayScreen == STNSELECT )return;

  char  modetext[32];

  switch( currDisplayScreen ){
        case POWEROFF:
            strcpy( modetext, "POWER OFF");
            break;
        case LINEIN:
            strcpy( modetext, "LINE IN");
            break;
        case BLUETOOTH:      
            strcpy( modetext, "BLUETOOTH");
            break;                  
        default: 
            // RADIO screen
            return;
  }
    
    tft.setFreeFont( DATE_FONT );  
    tft.setTextColor( TFT_BLACK, TFT_REALGOLD ); 

    int txtw    = tft.textWidth( modetext, 1 );
    int txtx    = tft.width() - txtw - 20 ; 

    int labelh  = tft.fontHeight(1)-1 ;
    int labelw  = txtw + 20;
    int labelx  = txtx - 10; 
    int labely  = tft.height() -  tft.fontHeight(1) - 4;

    tft.fillRoundRect( labelx, labely, labelw , labelh, 8, TFT_REALGOLD);  
    tft.drawString( modetext, txtx, labely + 1 ); 

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
  
 
    
  currDisplayScreen = newscreen;                  

// make sure screens are displayed after setting currDisplayScreen
// screen functions call functions that test this.

  if ( newscreen != RADIO && newscreen != STNSELECT ){


     drawWeather();
     if ( newscreen != POWEROFF ){
        tft.fillRect( 0,weathert + label2t, labelw+labelo+1, tft.height() - label2t, TFT_BLACK  );
     }
 
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
    
    Serial.println("Touch read...");
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
             break;
        case BUTTON_NEXT:
             log_i("next station");
             change_volstat( 1, gStation );
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
        case BUTTON_ITEM6:
        case BUTTON_ITEM7:
             setStation( touchbutton[button_pressed].stationidx , -1 );                       
             draw_buttons( touchbutton[ BUTTON_ITEM0].stationidx );
             noreset = true;
             break; 
        case BUTTON_QUITLIST:
             newScreen = RADIO; 
             break;  
        case BUTTON_LEFTLIST:
             newidx      = touchbutton[ BUTTON_ITEM0].stationidx - 8;
             if ( newidx <= 0 ){
                 log_d("*newidx < 0 : %d", newidx);
                 newidx = stationCount + newidx; 
                 log_d("*newidx modified to %d", newidx);
             }
             draw_buttons( newidx );
             break; 
        case BUTTON_RIGHTLIST:            
             newidx      = touchbutton[ BUTTON_ITEM7].stationidx + 1;
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
