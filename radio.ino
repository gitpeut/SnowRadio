
static struct {
  int mode;
  int h;
  int bufsize;
  int sendsize;
} chunkstate;

//---------------------------------------------------------------------------

void reset_chunkstate(){
  chunkstate.mode     = 0;
  chunkstate.h        = 0;
  chunkstate.bufsize  = 0;
  chunkstate.sendsize = 0;
 
}


//---------------------------------------------------------------------------

void filter_buffer ( uint8_t *rBuffer, int bytecount ){
uint8_t     *r,*f;  
static char hexstring[16], sendbuffer[32];

if ( ! stationChunked ){
   for ( r = rBuffer ; r - rBuffer < bytecount; ++r){
    
          sendbuffer[ chunkstate.sendsize] = *r;
          ++chunkstate.sendsize;
                     
          if ( chunkstate.sendsize == 32 ){
             xQueueSend( playQueue, sendbuffer, portMAX_DELAY);
             chunkstate.sendsize = 0;
          }
   }
   return;                              
}

if ( stationChunked ){
 delay(1);
 for ( r = rBuffer ; r - rBuffer < bytecount; ++r){
  switch ( chunkstate.mode ){
    case 0:
          if ( *r == ';' || *r == '\r' ){
              hexstring[chunkstate.h] = 0;
              chunkstate.h = 0;
              chunkstate.bufsize = strtol( hexstring, NULL, 16 );
              //Serial.printf( " String %s = Hex %X\n", hexstring, chunkstate.bufsize);
              chunkstate.mode = 1; 
              if ( chunkstate.bufsize == 0 )chunkstate.mode = 4;
          }
          hexstring[chunkstate.h] = *r;
          ++chunkstate.h;
          break;
     case 1:
          if (*r  == '\n' ) chunkstate.mode = 2;
          break;
     case 2:
          if( chunkstate.bufsize == 0 ){
            chunkstate.mode = 3;
            break;
          }
          sendbuffer[ chunkstate.sendsize] = *r;
          ++chunkstate.sendsize;
          --chunkstate.bufsize;
                     
          if ( chunkstate.sendsize == 32 ){
             xQueueSend( playQueue, sendbuffer, portMAX_DELAY);
             chunkstate.sendsize = 0;
          }
          break;
    case 3:
          if (*r  == '\n' ) chunkstate.mode = 0;          
          break;
    case 4:
          if (*r  == '\n' ) chunkstate.mode = 5;
          break;
    case 5:
          if (*r  == '\n' ) chunkstate.mode = 6;
          break;
    case 6:
          if (*r  == '\n' ) chunkstate.mode = 0;
          break;

  }
 }
}    

  return;  
}

//--------------------------------------------------------------------

void radio( void *param) {

static int connectmillis;
int   bufbegin=1, rc;
int   noreads = 0, lowqueue=0;
int   totalbytes=0, threshbytes=5000;
//uint8_t radioBuffer[32];
uint8_t radioBuffer[256];
         
  Serial.printf("Radiotask running on core %d\n", xPortGetCoreID()); 
   
/*
  TIMERG0.wdt_wprotect=TIMG_WDT_WKEY_VALUE;
  TIMERG0.wdt_feed=1;
  TIMERG0.wdt_wprotect=0;
*/

  setStation( get_last_volstat(0),-1 );
  Serial.printf("Radiotask starting loop\n", xPortGetCoreID()); 

while(1){
      if ( xSemaphoreGetMutexHolder( updateSemaphore ) != NULL ){
        xSemaphoreTake( updateSemaphore, portMAX_DELAY);
        xSemaphoreGive( updateSemaphore);
      }
      
    if(radioclient->available() > 0 ){

      if ( unavailablecount > topunavailable ) topunavailable = unavailablecount;
      unavailablecount = 0;   
      lowqueue--; 
      if ( lowqueue < 0 ) lowqueue = 0;
      
      int bytesread = 0;

//      bytesread = radioclient->read( &radioBuffer[0], 32 );  
        bytesread = radioclient->read( &radioBuffer[0], 128 );  

       
        
         if ( contentsize != 0 ){
            stations[ playingStation].position += bytesread;
         }
         
         totalbytes += bytesread;
         
         if ( bytesread <= 0 ){
          noreads++; 
          delay(2); //server may be slow. Wait a bit to please wd.
          Serial.printf("noreads : %d (read rc %d)\n", noreads, bytesread);
          if ( bytesread < 0 ) bytesread = 0;
         }else{
            noreads = 0;
            filter_buffer( &radioBuffer[0], bytesread );
         }
      
      if ( ( errno != EWOULDBLOCK && errno != EINPROGRESS ) ||  noreads > 5 ) {
        // ewouldblock is eagain = 11 on esp32, einprogress (= 119) happens if station is being connected
        
          Serial.printf("Disconnect radio errno %d bytesread %d\n", errno, bytesread );
          disconnectcount++; 
          //radioclient->flush(); //experimental
          radioclient->stop();       
          
      }            
     
    }else{ //no bytes available
      if (  millis() > connectmillis )unavailablecount++;
      
        if( unavailablecount > 10 && unavailablecount%10 == 0 )Serial.printf("nobytes available, connectmillis %d millis %d connected %d unavailablecount %d\n", connectmillis, millis(), radioclient->connected(), unavailablecount);
        delay(1);

        if ( uxQueueMessagesWaiting(playQueue) < 20 ){
          lowqueue++; 
        }
        
        if(!radioclient->connected() ){
                        
            //Serial.printf("Connect (again?) to %s, totalbytes %d\n",stations[ getStation() ].name, totalbytes);

            if ( unavailablecount > MAXUNAVAILABLE ){
              unavailablecount = 0;
            }
            
            if ( totalbytes < 100 ){
               if ( totalbytes > 0 ){
                 tellPixels( PIX_BLINKYELLOW );
                 tft_notAvailable( getStation() );                        
                 delay(2000 );
               }
            }
            totalbytes=0;
            
            if( 0 == (rc = stationsConnect( getStation()) ) ){
              playingStation = getStation();
              save_last_volstat( 0 );
              failed_connects = 0;
              connectmillis = millis() + 5000;
               //reset chunked encoding counters
              reset_chunkstate();
              lowqueue = 0;
               tellPixels( PIX_DECO );
            }else{
              if ( rc >= 400 ){
                  tellPixels( PIX_BLINKYELLOW );
                  tft_notAvailable( getStation() );                        
                  delay(2000 );
                  //setStation( 0,-1);
              }else{
                  failed_connects++;
                  delay(100);
                  if ( failed_connects > 3 ) {
                   
                    tellPixels( PIX_BLINKRED );
                    syslog( (char *)"Reboot after 3 failed connects");
                    ESP.restart();
                  }
              }
            }
       }
     
    }

//        Serial.printf("player playptr %d getptr %d endptr %d \n", mp3Playptr, mp3Getptr, mp3Endptr);  
  
      
//    if ( getStation() != playingStation && radioclient->connected() || unavailablecount > MAXUNAVAILABLE ){
    if ( getStation() != playingStation || unavailablecount > MAXUNAVAILABLE || lowqueue > 100 ){
        Serial.printf("playingStation %d != currentStation %d (lowqueue %d unavailable %d) reconnect...\n", playingStation, getStation(), lowqueue, unavailablecount );        
        //radioclient->flush();
        radioclient->stop();  
        
        tellPixels( PIX_YELLOW );

        for ( int curvol = getVolume(); curvol; --curvol ){
          vs1053player->setVolume( curvol  );
          delay( 5 );         
        }
        
        xQueueReset( playQueue);
        xQueueSend( playQueue, "ChangeStationSoStartANewSongNow!" , portMAX_DELAY);
        lowqueue = 0;
        
        
        
        if ( unavailablecount > MAXUNAVAILABLE ){
            Serial.printf("errno %d unavailble more than %d. reconnect...\n", errno, MAXUNAVAILABLE );
            syslog( (char *)"reconnect after data has been unavailable.");           
            disconnectcount++;
        }else{
            Serial.println("switch station...");
        }

    }

}//end while

}

/*--------------------------------------------------*/

int radio_init(){
     
    xTaskCreatePinnedToCore( 
         radio,                                      // Task to handle special functions.
         "Radio",                                            // name of task.
         2048*8,                                                 // Stack size of task
         NULL,                                                 // parameter of the task
         RADIOTASKPRIO,                                                    // priority of the task
         &radioTask,                                          // Task handle to keep track of created task 
         RADIOCORE);                                                  // processor core
                
return(0);
}
