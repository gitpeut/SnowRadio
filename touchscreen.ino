#include "fonts/indicator32x32.h"
#include "touch.h"
#ifdef USETOUCH

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
  }
  log_i("written uint16_t caldata={ %u, %u, %u, %u,%u }", calData[0], calData[1], calData[2], calData[3], calData[4]); 
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
  
  uint16_t  touch_x = 0, touch_y = 0;
  static uint32_t  touch_count=0;
  
  if( xSemaphoreTake( tftSemaphore, 50 ) == pdTRUE){
    
    boolean pressed = tft.getTouch(&touch_x, &touch_y, 50);
    xSemaphoreGive( tftSemaphore);
  
  
    log_d("%d - touch (%s) at %u,%u", touch_count++, pressed?"pressed":"not pressed", touch_x,touch_y);
  
    if ( pressed ){
      for ( int i = 0 ; i < TOUCHBUTTONCOUNT; ++i ){
        if ( touchbutton[i].contains(touch_x, touch_y) ) return( i ); 
      }  
    }
  }
  return( -1 );  
} 


//--------------------------------------------------------------------
void touch_setup(){
int topt  = 2;
int bott  = tft.height() - BUTH;     

   tft.setFreeFont(&indicator);
   
   int butoffset = BUTOFFSET; 
   touchbutton[BUTTON_MUTE].init( butoffset,                  topt, 'Q');
   touchbutton[BUTTON_STOP].init(tft.width() - BUTW - butoffset, topt, 'S');
   
   touchbutton[BUTTON_DOWN].init( butoffset,                 bott, 'X');
   touchbutton[BUTTON_UP].init  ( 1*BUTW + 2*butoffset,      bott, 'W');
   touchbutton[BUTTON_PREV].init( 2*BUTW + 3*butoffset,      bott, 'V');
   touchbutton[BUTTON_NEXT].init( 3*BUTW + 4*butoffset,      bott, 'U');



log_i("touch button setup done"); 
}
//--------------------------------------------------------------------

void touch_process( void *param){
  uint32_t    touch_command;
  bool        irqset = false;
  int         button_pressed= -1;
  bool        noreset= false; 
  
  Serial.printf("Touch running on core %d\n", xPortGetCoreID()); 

  pinMode( TOUCH_IRQ, INPUT_PULLUP);
  
  touch_setup();
  
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
      if ( button_pressed >= 0 ){
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
        case BUTTON_PREV:
             log_i("previous station");
             change_volstat( -1, gStation );
             break;
        case BUTTON_NEXT:
             log_i("next station");
             change_volstat( 1, gStation );
             break;
        case BUTTON_MUTE:
             noreset = true; 
             toggleMute();
             break;
        case BUTTON_STOP:
             log_i("stop"); 
             break;
        default:
            //Unknown or no button pressed
            break;      
      }
      if ( button_pressed >= 0 && noreset == false ){
        log_i("print normal");
        touchbutton[ button_pressed].draw( false );  
      }
      noreset = false;
          
  }

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
