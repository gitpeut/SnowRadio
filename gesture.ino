uint8_t             ledlast_state = 255;

/*--------------------------------------------------------------------*/

void IRAM_ATTR gesture_found(){
int started=0;
  
  if( ! digitalRead( GINTPIN ) ){
     
     xTaskNotifyFromISR( gestureTask, 1 ,eSetValueWithOverwrite, &started); 
     if( started ){
        portYIELD_FROM_ISR()
     }
  }
}

/*-------------------------------------------------------------------*/

void gesture_process( void *param){

uint8_t   data = 0, data1 = 0, gerror;
uint32_t  notify_value=0;
uint8_t   state=99; 

Serial.printf("Gesture running on core %d\n", xPortGetCoreID()); 

Wire.begin( GSDAPIN,GSCLPIN);

pinMode(GINTPIN,INPUT_PULLUP );
attachInterrupt( GINTPIN, gesture_found, CHANGE);

delay(3);

for( gerror = 1; gerror; delay(300) ){  
  gerror = paj7620Init();      // initialize Paj7620 registers
  if (gerror) {
    Serial.print("INIT ERROR,CODE:");
    Serial.println(gerror);

    tellPixels(9);     
 
  }else{
    Serial.println("PAJ7620 Gesture init ok");
  }
}

  
while(1){
 Serial.println("Gesture read...");
  
 xTaskNotifyWait(0,0,&notify_value,portMAX_DELAY);
  
 gerror = paj7620ReadReg(0x43, 1, &data );       // Read Bank_0_Reg_0x43/0x44 for gesture result.
 if ( gerror ) { Serial.println("Error reading register 0x43"); continue;}
 
 Serial.printf("data %02x - ", data);
 
    switch (data)                  // When different gestures be detected, the variable 'data' will be set to different values by paj7620ReadReg(0x43, 1, &data).
    {                              // PAJ7620 is installed upside down. updown left and right are flipped
      case GES_LEFT_FLAG:
          Serial.print("Right\n");
          tellPixels(2);
          state = 1;        
        break;
      case GES_RIGHT_FLAG:
          Serial.print("Left\n");    
          tellPixels(3);
          state = 0;
        break;
      case GES_UP_FLAG:
          Serial.print("Down\n");
          tellPixels(0);
          state = 3;
        break;
      case GES_DOWN_FLAG:
          Serial.print("Up\n");
          tellPixels(9);     
          state = 2;
        break;
      case GES_FORWARD_FLAG:
        Serial.print("Forward\n");
        state = 4;
        break;
      case GES_BACKWARD_FLAG:     
        Serial.printf("Backward, last state = %d\n", ledlast_state);
        state = 5;

        if ( ledlast_state == 4 && gmode ){
           state = 10; // forward backward
           tellPixels(10);     
           
           Serial.print(" - Doej!\n");
        }
        break;
      case GES_CLOCKWISE_FLAG:
        Serial.print("Clockwise\n");
        tellPixels(8);     
        state = 6;
        break;
      case GES_COUNT_CLOCKWISE_FLAG:
        Serial.print("anti-clockwise\n");
        tellPixels(16);     
        state = 7;
        break;  
      default:
        paj7620ReadReg(0x44, 1, &data1);
        if (data1 == GES_WAVE_FLAG) 
        {
          Serial.printf(" data1 %02x - wave\n", data1);
          state = 8;

          if ( getVolume() > 50 ){
            tellPixels(11);     
            
           }else{
            tellPixels(12);     
            
           }
          if ( ledlast_state == 8 ){
            state = 11; //two waves
          }
        }else{
          Serial.print("funny gesture\n");
          if ( data == 0xff ){
                gerror = paj7620ReadReg(0x43, 1, &data);       // Read Bank_0_Reg_0x43/0x44 for gesture result.
                if ( gerror ) { Serial.println("Error readin regster 0x43"); continue;}
                gerror = paj7620ReadReg(0x43, 1, &data);       // Read Bank_0_Reg_0x43/0x44 for gesture result.
                if ( gerror ) { Serial.println("Error readin regster 0x43"); continue;}
          }
          
          state = 9;          
        }
        break;
  }

  
  if ( state != 9 ){
       ledlast_state = state; 
  }
}
 
}

/*--------------------------------------------------*/

int gesture_init(){

    initPixels();
     
		xTaskCreatePinnedToCore( 
         gesture_process,                                      // Task to handle special functions.
         "Gesture",                                            // name of task.
         2048+1024,                                                 // Stack size of task
         NULL,                                                 // parameter of the task
         GESTURETASKPRIO,                                                    // priority of the task
         &gestureTask,                                          // Task handle to keep track of created task 
         GESTURECORE  );                                                  // processor core
                
return(0);
}
