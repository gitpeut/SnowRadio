

hw_timer_t       *gTimer = NULL;
uint32_t          goodbyeCount=0, oldGoodbyeCount=-1;


/*------------------------------------------------------------------------------*/

void gRight( uint32_t fgcolor, int wait, int length, uint32_t bg){
  
    uint32_t color   = fgcolor;
    uint32_t bgcolor = bg;
    
   
    if( length < 2 ) return; // prevent division by zero, no use of animating

   for( int i = 0; i < (gstrip.ledcount() + length); ++i ){

      for ( int j = 0; j < length ; ++j ){
        int dimperc = 100 - j*( 100/( length - 1)); 
        
        if ( dimperc < 10 ){
          gstrip.color32(i - j, bgcolor,100); 
        }else{
          gstrip.color32(i - j, color, dimperc );  
        }
       
        yield();
      }
      
      gstrip.show();
      
      delay( wait);
  }  
}


/*------------------------------------------------------------------------------*/

void gLeft( uint32_t fgcolor, int wait, int length, uint32_t bg){
  
    uint32_t color = fgcolor;
    uint32_t bgcolor = bg;

   
    if( length < 2 ) return; // prevent division by zero, no use of animating

 
   for( int i = gstrip.ledcount() -1 ; i > ( -1 * length); --i ){

      for ( int j = 0; j < length ; ++j ){  
        int dimperc = 100 - j*( 100/( length - 1)); 
        if ( dimperc <  10 ){
          gstrip.color32(i + j, bgcolor, 100 );
        }else{
          gstrip.color32(i + j, color, dimperc );
        }
      }  

      gstrip.show();
      delay( wait); 
  }  
}


/*------------------------------------------------------------------------------*/
void gToSleep( uint32_t fgcolor , int wait, int length ){

uint32_t color = fgcolor;
uint32_t b,e,half, whole, dimperc;

if( length < 2 ) return; // prevent division by zero, no use of animating

whole = gstrip.ledcount() -1;
half  = whole / 2;
 
for ( b=0; b < half + length; ++b ){

       e = whole - b;
     
       for ( int j = 0 ; j < length; ++j ){ 
        
        dimperc = (100 - j*( 100/( length - 1) ));
        if ( b-j > half )dimperc = 0; 

        gstrip.color32( b-j, color, dimperc );
        gstrip.color32( e+j, color, dimperc );
       }
       
     
      gstrip.show();
      
      delay( wait); 

}

gstrip.clear();  
gstrip.show();


return;
}

/*------------------------------------------------------------------------------*/
void doMute( int bg, bool loud ){
int    mcolor   = gstrip.getcolor( 250,0,0,0);
int    dimperc  = 100, dim=-5, endvol=40; 
int    curvol = getVolume(), volstep=-2;


if ( loud ){
  
  mcolor   = gstrip.getcolor( 250,0,100,0);
  volstep =   2;
  dimperc =   0;
  dim     =   5;
  endvol     =  75;
}
 
for ( int i=0; i < 20; ++i ){

      dimperc += dim;
      mcolor   = gstrip.getcolor( 250,0,100 - dimperc,0);

      for ( int s=0; s < gstrip.ledcount(); ++s ){      
        gstrip.color32( s, mcolor, dimperc );
      }
      
      gstrip.show();    
      
      curvol   += volstep;
      setVolume( curvol );
      
      delay( 80 );

      if ( loud ){
        if( curvol + volstep >= endvol ) break;  
      }else{
        if( curvol + volstep <= endvol ) break;          
      }
}

for ( int s=0; s < gstrip.ledcount(); ++s ){      
      gstrip.color32( s, bg, 100 );
}

gstrip.show();
setVolume( endvol );

return;
}

/*------------------------------------------------------------------------------*/
void gWakeup( int bg , int wait, int length ){

uint32_t b,half, whole, dimperc;

if( length < 2 ) return; // prevent division by zero, no use of animating


whole = gstrip.ledcount() -1;
half  = whole / 2;
 
    
for ( b = 0; b <= half; ++b ){

       for ( int j = b + 1 ; j > 0 ; --j ){ 
        
        dimperc = 100; //j*( 100/( length - 1) );
        gstrip.color32( half+j, bg, dimperc );
        gstrip.color32( half-j, bg, dimperc );

       }
       
      gstrip.show();
      
      delay( wait); 
}
gstrip.color32( half, bg, 100 );

gstrip.show();

return;
}

/*------------------------------------------------------------------------------*/
int getVolume(){
  int v;
  
  xSemaphoreTake(volSemaphore, portMAX_DELAY);
  v = currentVolume;
  xSemaphoreGive(volSemaphore);

  return(v);
}
/*------------------------------------------------------------------------------*/
int setVolume( int v){

  //Serial.printf( "Changing volume to %d\n", v); 

  xSemaphoreTake(volSemaphore, portMAX_DELAY);
  currentVolume = v;
  player.setVolume( v );
  xSemaphoreGive(volSemaphore);

 
  showVolume(v);
  
  save_last_volstat(1);
  
  return(v);
}
/*------------------------------------------------------------------------------*/
int getStation(){
  int s;
  
  xSemaphoreTake(staSemaphore, portMAX_DELAY);
  s = currentStation;
  xSemaphoreGive(staSemaphore);

  return(s);
}
/*------------------------------------------------------------------------------*/
int setStation(int s, int p){
  
  xSemaphoreTake(staSemaphore, portMAX_DELAY);
  currentStation = s;
  playingStation = p;
  xSemaphoreGive(staSemaphore);
  
  return(s);
}

/*------------------------------------------------------------------------------*/
void change_volstat(int dir){
int current_volume  = getVolume();
int current_station = getStation();

if ( gmode == 1 ){
    current_volume += (dir*5);

    if ( current_volume > 100 )current_volume = 100;    
    if ( current_volume < 0  ) current_volume = 0;    

    
     
     setVolume(current_volume );
     save_last_volstat(1);
}

if ( gmode == 2 ){
    current_station += dir;  
    
    Serial.printf( "Changing station\n"); 
    while(1){
      if ( current_station < 0 )current_station = STATIONSSIZE - 1;
      if ( current_station >= STATIONSSIZE )current_station = 0;
      if ( stations[ current_station].status == 1 ) break; 
      current_station += dir;
    }
    Serial.printf( "Changing station to %d-%s\n", current_station,stations[ current_station].name ); 
         
    setStation( current_station, -1 );
    
}

}
/*------------------------------------------------------------------------------*/

void doPixels( void *param ){
static uint32_t  bg=0;
uint32_t         pixelCommand;
  
  Serial.printf("Pixels running on core %d\n", xPortGetCoreID()); 
  
  gToSleep( 0 , 50, 5 );

  while(1){
    gLeft( gstrip.getcolor( 0,100,0,0) , 40, 8, bg);
    delay(500);  
    gRight( gstrip.getcolor( 100,0,0,0), 40, 8, gstrip.getcolor(20,0,0,20 ) );
    delay(500);
  }
   

    
  while(1){

   //Serial.printf("Pixels waiting on events\n" ); 
  
    xTaskNotifyWait(0,0,&pixelCommand,portMAX_DELAY);

    Serial.printf("doPixels got command %d (mode = %d, goodbyeCount %d, oldGoodbyeCount %d)\n", pixelCommand, gmode, goodbyeCount,oldGoodbyeCount );

    switch( gmode ){
      case 0:
        bg = 0;
        break;
      case 2: 
        bg = gstrip.getcolor(20,0,0,20 );
        break;
      case 3:
        bg = gstrip.getcolor(150,0,0,30 );
        break;     
      case 1:  
      default:
        bg = gstrip.getcolor(0,0,100,0 );
       break;
    }

     
    switch( pixelCommand ){
      case 0: // turn pixels off
        if ( gmode == 3){
            xSemaphoreTake( chooseSemaphore, portMAX_DELAY);
            chosenStation = 2;
            xSemaphoreGive( chooseSemaphore); 
        }
        gmode = 0;
        gToSleep( bg , 100, 5 );
        break;
      case 1: // turn pixels on
        gWakeup( bg, 50, 5 );
        break;          
      case 2: // right, green snake
        gLeft( gstrip.getcolor( 0,100,0,0) , 40, 8, bg);
        change_volstat( 1);
        break;
      case 3: // left red snake
        gRight( gstrip.getcolor( 100,0,0,0), 40, 8, bg );
        change_volstat( -1 );
        break;
      case 4: // confirm
        if ( gmode == 3){
            xSemaphoreTake( chooseSemaphore, portMAX_DELAY);
            chosenStation = 1;
            xSemaphoreGive( chooseSemaphore); 
        }
        gWakeup( gstrip.getcolor( 0,100,0,0) , 10, 6 );
        delay(400);
        gWakeup( bg , 1, 6 );
        break;          
      case 8:// clockwise only in mode 3
        tft_scrollstation( SCROLLUP );
        gWakeup( bg, 50, 5 );
        break;  
      case 16:
        tft_scrollstation( SCROLLDOWN );
        gWakeup( bg, 50, 5 );
        break;            
      case 10:
        gToSleep( bg , 100, 5 );
        delay(100);
        gToSleep( bg , 100, 5 );
        break;
      case 11:
        doMute(bg,0);
        break;
      case 12:
        doMute(bg,1);
        break;
      case 911:
        //Serial.printf("timer fired # %d, do gToSleep\n", goodbyeCount); 
        gmode = 0;     
        gToSleep( 0, 100, 5 );
        break;
      default:
        break;   
    }
  }
}



/*------------------------------------------------------------------------------*/
void tellPixels( uint32_t command ){

  Serial.printf("tellPixels gmode = %d :  command %d \n", gmode, command );
  
  if ( command == 11 ){ //mute
         xTaskNotify( pixelTask, 11,eSetValueWithOverwrite);          
         //return; 
  }
  if ( command == 12 ){ //unmute
         xTaskNotify( pixelTask, 12,eSetValueWithOverwrite);          
         //return; 
  }
  
  
  if ( gmode == 0 ){
     switch( command ){
       case 911:
          if ( oldGoodbyeCount != goodbyeCount) {
              Serial.printf("gmode = 0 : goodbyeCount %d, oldGoodbyeCount %d\n", goodbyeCount,oldGoodbyeCount );
              oldGoodbyeCount = goodbyeCount;
          }
          stopgTimer();
          break; 
       case 0: // down
          stopgTimer();
          xTaskNotify( pixelTask, 0,eSetValueWithOverwrite);     
          break;
       case 9: //up
        gmode = 1;
        setgTimer(); 
        xTaskNotify( pixelTask, 1,eSetValueWithOverwrite);     
        break;
      default:
        break;       
      }
      return;
  }
  

  if ( gmode ){
    switch(command){
        case 911:
          if ( oldGoodbyeCount != goodbyeCount){
            Serial.printf("Do 911, oldgodbye = %d, goodbye %d\n", oldGoodbyeCount, goodbyeCount ); 
            oldGoodbyeCount = goodbyeCount;
            gmode = 0;
            xTaskNotify( pixelTask, 911,eSetValueWithOverwrite);     
          }
          break;          
        case 0: //down
         
            //if( gmode != 3 ) gmode = 0;
            stopgTimer();
            xTaskNotify( pixelTask, 0,eSetValueWithOverwrite);     
          break;
        case 8: //circle
        case 16: //circle
          setgTimer(); 
          if ( gmode == 1 ){
            gmode = 2;
            xTaskNotify( pixelTask, 1,eSetValueWithOverwrite);     
          } else if ( gmode == 2 || gmode == 3){
            gmode = 3;
            xTaskNotify( pixelTask, command,eSetValueWithOverwrite);     
          }
          break;
        case 9: // up; confirm
          setgTimer(); 
          xTaskNotify( pixelTask, 4,eSetValueWithOverwrite);     
          break;
        case 10: //fw backw; stop
          stopgTimer();
          gmode = 0;        
          xTaskNotify( pixelTask, 10,eSetValueWithOverwrite);              
          break;
        case 11:
        case 12:
          break;  
        default:
          setgTimer();      
          xTaskNotify( pixelTask, command,eSetValueWithOverwrite);     
          break;
    }
    return;
  }
}


/*------------------------------------------------------------------------------*/
void IRAM_ATTR gTmo(){
 BaseType_t    pxHighP=0;

 goodbyeCount++;

 xTaskNotifyFromISR( pixelTask, 911,eSetValueWithOverwrite, &pxHighP); 
 if( pxHighP ){
    portYIELD_FROM_ISR();
 } 
}

/*------------------------------------------------------------------------------*/
void stopgTimer(){

 if( gTimer != NULL ){
    timerAlarmDisable( gTimer);
    gTimer = NULL;
 }

} 
/*------------------------------------------------------------------------------*/

void setgTimer(){
  
 if( gTimer != NULL ){
    timerAlarmDisable( gTimer);
    gTimer = NULL;
 }

 gTimer = timerBegin(0, 80, true);                // use time 1 to stop command mode ( = anything not zero )
 timerAttachInterrupt(gTimer, gTmo, true);     // let it run endMode when due
 timerAlarmWrite(gTimer, 10000000, false );       // set it to 10 seconds, no repeat 
 timerAlarmEnable(gTimer);                        // turn timer 1 on

}

/*------------------------------------------------------------------------------*/

void gpixelBegin( int brightness )
{ 

gstrip.begin( NEOPIN, NEONUMBER );
gstrip.setbrightness( brightness);
gstrip.clear();

}

/*------------------------------------------------------------------------------*/

void initPixels(){
  
    gpixelBegin( 80 );

    delay(100);
    
    xTaskCreate( 
         doPixels,                                      // Task to handle special functions.
         "Pixels",                                            // name of task.
         3*1024,                                                 // Stack size of task
         NULL,                                                 // parameter of the task
         PIXELTASKPRIO,                                                    // priority of the task
         &pixelTask);                                               // Task handle to keep track of created task 

   
}
