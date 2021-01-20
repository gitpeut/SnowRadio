

void play ( void *param ){
uint8_t   playBuffer[32];
uint32_t  VSlow=0;

 Serial.printf("Playtask running on core %d\n", xPortGetCoreID()); 
//
//https://github.com/espressif/arduino-esp32/issues/595
//
  
  TIMERG0.wdt_wprotect=TIMG_WDT_WKEY_VALUE;
  TIMERG0.wdt_feed=1;
  TIMERG0.wdt_wprotect=0;
  
    
  xSemaphoreTake( tftSemaphore, portMAX_DELAY);
  player.startSong();
  xSemaphoreGive( tftSemaphore);       
  
  
  while ( uxQueueMessagesWaiting(playQueue) <  (PLAYQUEUESIZE/2) ) {
    Serial.printf ( "Waiting for Queue to fill up, %d messages in playQueue\n", uxQueueMessagesWaiting( playQueue ) );
    delay(20);
  }
  Serial.printf ( "Queueu filled up, %d messages in playQueue\n", uxQueueMessagesWaiting( playQueue ) );
  
    while(1){
      
      delay(1);
      //delay(20);//wdt avoidance
      xQueueReceive(playQueue, &playBuffer[0], portMAX_DELAY);

      if ( strncmp( (char *) &playBuffer[0], "ChangeStationSoStartANewSongNow!",32) == 0 ){
        Serial.println("Got change station message");
        
        xSemaphoreTake( tftSemaphore, portMAX_DELAY);
        player.stopSong();
        delay(20);
        player.startSong();
        xSemaphoreGive( tftSemaphore);   
            
        xQueueReceive(playQueue, &playBuffer[0], portMAX_DELAY);
      }
        
      
        for ( int i = 0; i < 1 ; ++i ){
                    
          if ( digitalRead( VS1053_DREQ || VSlow > 10 ) ){
            xSemaphoreTake( tftSemaphore, portMAX_DELAY);
            player.playChunk(playBuffer, 32  );
            xSemaphoreGive( tftSemaphore);         
            VSlow = 0;   
          }else{
            Serial.println( "No DREQ");
            --i;  
            VSlow++;
            delay(10);
            if ( VSlow > 60 ){         
              if ( VSlow % 3 == 0 ){
                Serial.printf ( "Waiting for VS1053, VSlow %d , %d messages in playQueue\n", VSlow, uxQueueMessagesWaiting( playQueue ) );
                syslog( "VSlow for a long time.");
                if ( VSlow > 3000 ){
                  Serial.println("Reset VS1053");
                  // write 0x50 to SCI_AIADDR
                  xSemaphoreTake( tftSemaphore, portMAX_DELAY);
                  player.write_register( 0xA, 0x50 );
                  xSemaphoreGive( tftSemaphore);         
            
                  Serial.println("Reset VS1053 finished");
                  /*
                  tellPixels( PIX_BLINKRED );
                  syslog( "Reboot after VSlow");
                  ESP.restart();
                  */
                }  
              } 
            }
            delay(2);
          }
      }
     
    }


}


    /*--------------------------------------------------*/

int play_init(){
     
    xTaskCreatePinnedToCore( 
         play,                                      // Task to handle special functions.
         "Player",                                            // name of task.
         2048,                                                 // Stack size of task
         NULL,                                                 // parameter of the task
         PLAYTASKPRIO,                                                    // priority of the task
         &playTask,                                          // Task handle to keep track of created task 
         PLAYCORE);                                                  // processor core
                
return(0);
}
