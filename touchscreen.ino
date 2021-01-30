#ifdef USETOUCH

#define TOUCHBUTTONCOUNT = 11;
enum buttontype{
BUTTON_MUTE,
BUTTON_LIST,
BUTTON_BACK,
BUTTON_NEXT,
BUTTON_PREV,
BUTTON_STOP,
BUTTON_LIST0,
BUTTON_LIST1,
BUTTON_LIST2,
BUTTON_LIST3,
BUTTON_LIST4
};

TFT_eSPI_Button touchbutton [ TOUCHBUTTONCOUNT  ] ;



//--------------------------------------------------------------------
void process_mute_button(){

}
//--------------------------------------------------------------------
void process_back_button(){



}

//--------------------------------------------------------------------

void IRAM_ATTR touch_found (){
int started=0;
  
  if( ! digitalRead( TOUCH_IRQ ) ){     
     xTaskNotifyFromISR( touchTask, 1 ,eSetValueWithOverwrite, &started); 
     if( started ){
        portYIELD_FROM_ISR();
     }
  }
}

//--------------------------------------------------------------------
int whatbutton(){

  //find out which button has been pressed
  // return the button number 
  
  uint16_t touch_x = 0, touch_y = 0;
  
  boolean pressed = tft.getTouch(&touch_x, &touch_y, 50);

  if ( pressed ){
    for ( int i ; i < TOUCHBUTTONCOUNT; ++i ){
      if ( touchbutton[i].contains(touch_x, touch_y) ) return( i ); 
    }  
  }
  
  return( -1 );  
} 
//--------------------------------------------------------------------

void touch_process( 

 
pinMode( TOUCH_IRQ, INPUT_PULLUP);
attachInterrupt( TOUCH_IRQ, touch_found, CHANGE);

touch_setup();

while(1){
  Serial.println("Gesture read...");
  xTaskNotifyWait(0,0,&notify_value,portMAX_DELAY);
    switch( what_button() ){
      case BUTTON_PREV:
           process_prev_button();
           break;
      case BUTTON_NEXT:
           process_next_button();
           break;
      case BUTTON_MUTE:
           process_mute_button();
           break;
      case BUTTON_STOP:
           process_stop_button();
           break;
      case BUTTON_BACK:
           process_back_button();
           break;
      default:
          //Unknow or no button pressed
          break;      
    }    
}

}
//--------------------------------------------------------------------

int touch_init(){

#ifdef USEPIXELS  
    initPixels();
#endif     

    xTaskCreatePinnedToCore( 
         touch_process,                                      // Task to handle special functions.
         "Touch",                                            // name of task.
         2048+1024,                                                 // Stack size of task
         NULL,                                                 // parameter of the task
         TOUCHTASKPRIO,                                                    // priority of the task
         &touchTask,                                          // Task handle to keep track of created task 
         TOUCHCORE  );                                                  // processor core
                
return(0);
}

#endif
