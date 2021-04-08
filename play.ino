

void play ( void *param ){
uint8_t   playBuffer[32];
uint32_t  bandcounter=GETBANDFREQ, VSlow=0; 
int       setvolume = getVolume();
  Serial.printf("Playtask running on core %d\n", xPortGetCoreID()); 
  
  
  vs1053player->startSong();

  int qfillcount = 0;
  int qminimumcount = (PLAYQUEUESIZE/2);
  while ( uxQueueMessagesWaiting(playQueue) <  qminimumcount  ) {    
    qfillcount++;
    
    if ( xSemaphoreGetMutexHolder( radioSemaphore ) != NULL ) qminimumcount = 0;
    
    if ( qfillcount%10 == 0 ){
      log_i ( "Waiting for Queue to fill up, %d messages in playQueue", uxQueueMessagesWaiting( playQueue ) );
    }
    
    if ( qfillcount > 800 ){
      unavailablecount =  MAXUNAVAILABLE + 1;
      log_d( "Queue does not fill up, reconnect" );
      if ( qfillcount > 40000 ){
        log_d( "Queue still empty, trying a restart." );        
        ESP.restart();
      }
    }
    
    delay(40);
  }
  
  if ( qminimumcount ){
    log_i ( "Queueu filled up, %d messages in playQueue\n", uxQueueMessagesWaiting( playQueue ) );
  }else{
    log_i ( "Radio inactive, waiting for messages");
  }
  
    while(1){
      
      xQueueReceive(playQueue, &playBuffer[0], portMAX_DELAY);

      if ( strncmp( (char *) &playBuffer[0], "ChangeStationSoStartANewSongNow!",32) == 0 ){

        log_i("start playing again");
        vs1053player->setVolume(0);
        if ( !ModeChange ){
          
          log_d("play stops song");
            
          if ( !vs1053player->stop_song()  ){
            log_d("play reboots as VS1053 was confused");
            syslog((char *)"play reboots as VS1053 was confused");
            ESP.restart();
          }
          
        }
        
        skipstartsound = SKIPSTART;
        
        setvolume = getVolume();        
        ModeChange = false; 
          
        delay(5);
        vs1053player->startSong();
        vs1053player->setVolume(0);
      }
      
      
        for ( int i = 0; i < 1 ; ++i ){
          if ( digitalRead( VS_DREQ_PIN ) ){
            
            if ( !MuteActive && !ModeChange ) {
              vs1053player->playChunk(playBuffer, 32  );
            }else{
              delay(4);
            }

#ifdef USESPECTRUM            
            --bandcounter;
            if ( ! bandcounter ){
                
                vs1053player->getBands();                
                
                delay(10); // please the task watchdog
                if ( !skipstartsound && !screenUpdateInProgress 
                )vs1053player->displaySpectrum();

                bandcounter = GETBANDFREQ;         
            }
#endif
            if ( skipstartsound ){
                
                if ( skipstartsound < 200 ){ 
                  int newvolume = ( setvolume - (skipstartsound/2) + 1);
                  int divider = 16;
                  if ( (newvolume & divider) )vs1053player->setVolume( newvolume );
                }

                if ( skipstartsound == 1 ) {
                  log_d("** end fade in **");
                  vs1053player->setVolume( getVolume());
                }
                --skipstartsound;
                
            }

            VSlow = 0;   
          }else{
            --i;  
            VSlow++;
            if ( VSlow > 60 ){         
              if ( VSlow %10 == 0 ){
                Serial.printf ( "Waiting for VS1053, VSlow %d , %d messages in playQueue\n", VSlow, uxQueueMessagesWaiting( playQueue ) );
                syslog( (char *)"VSlow for a long time.");
              }
            }
            delay(3);
          }
      }
     
    }


}


    /*--------------------------------------------------*/

int play_init(){
     
    xTaskCreatePinnedToCore( 
         play,                                      // Task to handle special functions.
         "Player",                                            // name of task.
         4*1024,                                                 // Stack size of task
         NULL,                                                 // parameter of the task
         PLAYTASKPRIO,                                                    // priority of the task
         &playTask,                                          // Task handle to keep track of created task 
         PLAYCORE);                                                  // processor core
                
return(0);
}
