#include "tft.h"

static struct {
  int mode;
  int h;
  int bufsize;
  int sendsize;
} chunkstate;

//---------------------------------------------------------------------------

void reset_chunkstate(){
  
  log_d("reset chunkstate");
  
  chunkstate.mode     = 0;
  chunkstate.h        = 0;
  chunkstate.bufsize  = 0;
  chunkstate.sendsize = 0;

  reset_meta();
}
//---------------------------------------------------------------------------

struct metaInfo meta;
bool noMeta = false;
//---------------------------------------------------------------------------

void reset_meta(){  
  meta.metacount    = 0;
  meta.metalen      = 0;
  meta.metar        = NULL;
  meta.metadata[0]  = 0;
  meta.qoffset      = 0;
  meta.inquote      = false;
  meta.ignorequote  = false;
  meta.intransit    = false;
  meta.intervalcount = 0;

  tft_fillmeta();
}
//---------------------------------------------------------------------------
void dump_meta( char *m ){
char *s = m;

Serial.printf( "\n");
for ( ; *s; ++s ){
  Serial.printf( "%02x ", (int) *s);  
}
Serial.printf( "\n");

}
//---------------------------------------------------------------------------
int extractMeta ( uint8_t *r ){

  
  if ( meta.metalen ){
     meta.intransit = true;                                             
     if ( (meta.metar - meta.metadata) < 1000 ){
      if ( *r == '\"' ) {
        meta.ignorequote = !meta.ignorequote;
      }
      
      if ( meta.inquote){
          if ( !meta.ignorequote ){
            if ( *r == ';') {
              //log_d("found ending semicolon at %d", meta.metalen);
              if ( meta.metar > meta.metadata){
                if ( *(meta.metar-1) == '\'' ){
                  *(meta.metar-1)= 0;
                  meta.inquote = 0; 
                }
              }else{
                *(meta.metar) = 0;
              }
            }
          }
          if ( meta.inquote ){
            *meta.metar = *r;
            ++meta.metar;
          }
      }else{
        
        if ( *r == '\'' ){
           //log_d("found beginquote at %d", meta.qoffset );
            
           meta.inquote = 1;
           // StreamTitle='... quote should be at 13 
           if ( (meta.metalen > 256) || (meta.qoffset > 23) || (meta.qoffset < 0) ){
              // choppy reception of data, out of sync
              char tmpstring[132];
              sprintf( tmpstring, "stream of %s out of sync at interval %d, no more updates of trackinfo", 
                        stations[ getStation() ].name, meta.intervalcount);
              Serial.println( tmpstring); 
              syslog( tmpstring );
              stationMetaInt = 0;
              reset_meta();
              return( 777 );
           }
        }
      }
      
     }              
     --meta.metalen; 
     if( meta.metalen == 0 ){
       *meta.metar  = 0;
       meta.metalen = 0;
       meta.metar   = NULL;
       meta.qoffset =0;                       
       log_d( "Metadata:\n%s", meta.metadata);
       dump_meta( meta.metadata );
       tft_fillmeta();
       meta.intransit = false;
       broadcast_meta();                                                                                             
     }     
  }else{
    if ( stationMetaInt && (meta.metacount >= stationMetaInt) ){
     
       meta.metalen   = (int)*r;
       meta.metalen   = meta.metalen * 16; 
       meta.metar     = meta.metadata;
       *meta.metar    = 0; 
       meta.metacount = 0 - meta.metalen - 1; 
       meta.inquote   = 0;
       meta.ignorequote = false;
       meta.qoffset   = 0;
       meta.intervalcount++;
       
       if ( meta.metadata[0] ){
        log_d( "Metadata:\n%s", meta.metadata);;
       }
                     
//       log_d ("\nchunkmode %d stationMetaInt %d, metalen %d metacount %d ", 
//        chunkstate.mode, stationMetaInt, meta.metalen, meta.metacount);        
    }else{
       return(1);
    }
  }
  return(0);             
}
//---------------------------------------------------------------------------

int filter_buffer ( uint8_t *rBuffer, int bytecount ){
uint8_t     *r;  
static char hexstring[16], sendbuffer[32];


if ( ! stationChunked ){
   for ( r = rBuffer ; r - rBuffer < bytecount; ++r){
          
          int outofsync = extractMeta(r);
          if ( outofsync > 1 ) return( outofsync );
            
          if ( outofsync == 1 ){            
             sendbuffer[ chunkstate.sendsize] = *r;
             ++chunkstate.sendsize;
          }

          ++meta.metacount;

          if ( chunkstate.sendsize == 32 ){
             xQueueSend( playQueue, sendbuffer, portMAX_DELAY);
             chunkstate.sendsize = 0;
          }
   }
   return(0);                              
}

if ( stationChunked ){
 //delay(1);
 //log_d("starting chunked metacount = %d", meta.metacount);
  
 for ( r = rBuffer ; r - rBuffer < bytecount; ++r){
                    
  int outofsync = extractMeta(r);
  if ( outofsync > 1 ) return( outofsync );
   
  switch ( chunkstate.mode ){
    case 0:
          if ( *r == ';' || *r == '\r' ){
              hexstring[chunkstate.h] = 0;
              chunkstate.h = 0;
              chunkstate.bufsize = strtol( hexstring, NULL, 16 );
              
              chunkstate.mode = 1; 
              if ( chunkstate.bufsize == 0 )chunkstate.mode = 4;
              //log_d( " String %s = Hex %X. mode now %d\n", hexstring, chunkstate.bufsize, chunkstate.mode);
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
          
          if ( outofsync == 1 ){
            sendbuffer[ chunkstate.sendsize] = *r;
            ++chunkstate.sendsize;            
          }
          --chunkstate.bufsize;
          ++meta.metacount;
                   
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

  return(0);  
}

//--------------------------------------------------------------------

void disconnect_radioclient(){
      
  if ( radioclient->connected() ) {
    radioclient->stop();  
    tellPixels( PIX_YELLOW );
  
    for ( int curvol = vs1053player->getVolume(); curvol; --curvol ){
      vs1053player->setVolume( curvol  );
      delay( 7 );         
    }
    
    
    ModeChange = true;
    xQueueReset( playQueue); //empty queue   
    delay(200);
  } 
  
}
//--------------------------------------------------------------------

void radio( void *param) {

  static int connectmillis;
  int   rc;
  int   noreads = 0, lowqueue=0;
  int   totalbytes=0;
  //uint8_t radioBuffer[32];
  uint8_t radioBuffer[256];
           
  log_i("Radiotask running on core %d", xPortGetCoreID()); 

  setStation( get_last_volstat(0),-1 );
  vs1053player->setVolume(0); // fade in at startup
     
  log_i("Radiotask starting loop"); 

  while(1){
    //log_i("read %d bytes", totalbytes);
    if ( xSemaphoreGetMutexHolder( updateSemaphore ) != NULL ){
        xSemaphoreTake( updateSemaphore, portMAX_DELAY);
        xSemaphoreGive( updateSemaphore);
    }
    if ( xSemaphoreGetMutexHolder( radioSemaphore ) != NULL ){

        vs1053player->setVolume(0);
          
        ModeChange = true;
        
        //disconnect_radioclient();
        vs1053player->setVolume(0);
        log_d("stop song before waiting for radio semaphore");        
        vs1053player->stop_song();

        broadcast_meta( true );
        
        playingStation = -1;
        log_d("volume when stopping radiotask %d", vs1053player->getVolume()); 

       
        xSemaphoreTake( radioSemaphore, portMAX_DELAY);
        xSemaphoreGive( radioSemaphore); 
               
       
    }
    
    delay(2);

    if ( uxQueueMessagesWaiting(playQueue) < 20 ){
        lowqueue++; 
        if ( lowqueue > ( RESTART_AFTER_LOWQ_COUNT + ( 50 * stations[ currentStation ].protocol))   ){
           syslog( (char *)"Restart to solve low queue");  
           disconnect_radioclient();
           ModeChange = false;                       
           lowqueue = 0;     
           ESP.restart();   
        }
    }
      
    if(radioclient->available() > 0 ){

      if (unavailablecount){
        if ( unavailablecount > topunavailable ) topunavailable = unavailablecount;
        if ( unavailablecount > 3)Serial.printf("- data available again after %d fruitless poll%s (lowqueue = %d)\n", unavailablecount, (unavailablecount==1)?"":"s", lowqueue);   
      }
      
      unavailablecount = 0;
      if( lowqueue > 0 )lowqueue--; 
      
      int bytesread = 0;

        
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
            noreads = filter_buffer( &radioBuffer[0], bytesread );            
         }
      
      if ( ( errno != EWOULDBLOCK && errno != EINPROGRESS ) ||  noreads > 5 ) {
        // ewouldblock is eagain = 11 on esp32, einprogress (= 119) happens if station is being connected
        
          Serial.printf("Disconnect radio errno %d bytesread %d\n", errno, bytesread );
          disconnectcount++; 
          if ( noreads == 777 ) { // metadata out of sync, reconnect again without metadata
             noMeta = true;
          }
          radioclient->stop(); 
          
      }            

      
    }else{ //no bytes available
      
      if (  millis() > connectmillis )unavailablecount++;
      
        if( unavailablecount > 10 && unavailablecount%10 == 0 ){
          //Serial.printf("nobytes available, connectmillis %d millis %d connected %d unavailablecount %d\n", connectmillis, millis(), radioclient->connected(), unavailablecount);
          Serial.print("-");
        }
        delay(20);
        
        
        if(!radioclient->connected() ){
                        
            Serial.printf("Connect (again?) to %s, totalbytes %d\n",stations[ getStation() ].name, totalbytes);

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
              broadcast_meta( true );

              if ( nextprevChannel ){
                if (nextprevChannel == -1 )touchbutton[BUTTON_PREV ].draw( false );  
                if (nextprevChannel ==  1 )touchbutton[BUTTON_NEXT ].draw( false );  
                nextprevChannel = 0;
              }

              xQueueSend( playQueue, "ChangeStationSoStartANewSongNow!" , portMAX_DELAY);
              
            }else{
              
              bool tonextstation=false;
              
              Serial.printf( "Failed connecting to station, failed connects %d\n", failed_connects );
              if ( rc >= 400 ){
                  tellPixels( PIX_BLINKYELLOW );
                  tft_notAvailable( getStation() );                        
                  tonextstation = true;
                  delay(2000 );
              }else{
                  failed_connects++;
                  delay(100);
                  if ( failed_connects > 3 ) {
                   
                    tellPixels( PIX_BLINKRED );
                    syslog( (char *)"to next station after 3 failed connects");
                    //ESP.restart();
                    tonextstation = true;
                  }
              }
              if ( tonextstation ){
                  int following_station = getStation() + 1;
                  if ( following_station >= stationCount ) following_station = 0;
                  setStation( following_station,-1);                
              }
              
            }
       }
     
    }

//        Serial.printf("player playptr %d getptr %d endptr %d \n", mp3Playptr, mp3Getptr, mp3Endptr);  
   
      
   if ( getStation() != playingStation || unavailablecount > MAXUNAVAILABLE  ){
        Serial.printf("playingStation %d != currentStation %d (lowqueue %d unavailable %d) reconnect...\n", playingStation, getStation(), lowqueue, unavailablecount );        
        //radioclient->flush();
        log_d ( "station change, disconnect from playingStation");
        broadcast_meta( true );
        disconnect_radioclient();
        lowqueue = 0;
       
        
        if ( unavailablecount > MAXUNAVAILABLE ){
            Serial.printf("errno %d unavailable more than %d. reconnect...\n", errno, MAXUNAVAILABLE );
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
