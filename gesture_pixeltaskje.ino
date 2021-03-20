#include "touch.h"

hw_timer_t       *gTimer = NULL;
uint32_t          goodbyeCount=0, oldGoodbyeCount=-1;
bool              MuteActive = false;


/*------------------------------------------------------------------------------*/
void IRAM_ATTR gTmo(){
 BaseType_t    pxHighP=0;

 goodbyeCount++;
 #ifdef USEPIXELS
  xTaskNotifyFromISR( pixelTask, PIX_BLACK,eSetValueWithOverwrite, &pxHighP); 
  if( pxHighP ){
    portYIELD_FROM_ISR();
  }
 #else
  xTaskNotifyFromISR( gestureTask, 321 ,eSetValueWithOverwrite, &pxHighP); 
  if( pxHighP ){
        portYIELD_FROM_ISR()
  }
  gmode = gOff;
 #endif  
}

/*------------------------------------------------------------------------------*/
void stopgTimer(){

 if( gTimer != NULL ){
    timerAlarmDisable( gTimer);
    timerEnd(gTimer); //timerEnd will also detach interrupt
    gTimer = NULL;
 }

} 
/*------------------------------------------------------------------------------*/

void setgTimer(){ 
  
 if( gTimer != NULL ){   
   timerAttachInterrupt(gTimer, gTmo, true);   
   timerAlarmEnable(gTimer);  
   timerRestart( gTimer );
 }else{
   gTimer = timerBegin(0, 80, true);                // use time 1 to stop command mode ( = anything not zero )
   timerAttachInterrupt(gTimer, gTmo, true);     // let it run endMode when due
   timerAlarmWrite(gTimer, 10000000, false );       // set it to 10 seconds, no repeat 
   timerAlarmEnable(gTimer);                        // turn timer 1 on
 }
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

  log_d( "Changing volume to %d\n", v); 

  xSemaphoreTake(volSemaphore, portMAX_DELAY);
    currentVolume = v;
    vs1053player->setVolume( v );    
  xSemaphoreGive(volSemaphore);

  if ( currDisplayScreen != STNSELECT && currDisplayScreen != POWEROFF ) showVolume(v);
  if ( MuteActive )toggleMute();  
 
  save_last_volstat(1);
  
  return(v);
}

/*------------------------------------------------------------------------------*/
bool setSpatial( uint16_t newspatial ){
  

  if ( newspatial > 3 ) return false;
  
  xSemaphoreTake(volSemaphore, portMAX_DELAY);
  vs1053player->setSpatial( newspatial );
  xSemaphoreGive(volSemaphore);

  if ( vs1053player->getSpatial() == newspatial ){
    
    log_d( "currSpatial = %d, newspatial = %d", vs1053player->currspatial, newspatial);
    return true;
  }
  log_d( "currSpatial = %d, newspatial = %d", vs1053player->currspatial, newspatial);
  return(false);
  
}

/*------------------------------------------------------------------------------*/
uint16_t getSpatial(){
  uint16_t t;
  
  xSemaphoreTake(volSemaphore, portMAX_DELAY);
  t = vs1053player->getSpatial();
  xSemaphoreGive(volSemaphore);

  return(t);
}

/*------------------------------------------------------------------------------*/
uint16_t getTone(){
  uint16_t t;
  
  xSemaphoreTake(volSemaphore, portMAX_DELAY);
  t = currentTone;
  xSemaphoreGive(volSemaphore);

  return(t);
}

/*------------------------------------------------------------------------------*/
uint16_t setTone( uint16_t newtone){

  log_d( "Changing tone to %04x\n", newtone); 

  xSemaphoreTake(volSemaphore, portMAX_DELAY);
    currentTone = newtone;
    vs1053player->setTone( newtone );    
  xSemaphoreGive(volSemaphore);

  save_last_volstat(3);
  
  return( newtone);
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
#ifdef MULTILEVELGESTURES
void change_volstat(int dir){
#else
void change_volstat(int dir, int gmode){
#endif

int current_volume;  
int current_station;

  if ( gmode == gVolume ){
    
      current_volume  = getVolume();
      
      current_volume += (dir*5); // number of volume increase or decrease
  
      if ( current_volume > 100 )current_volume = 100;    
      if ( current_volume < 0  ) current_volume = 0;    
  
      #ifdef USETOUCH
       if ( currDisplayScreen == RADIO )touchbutton[BUTTON_MUTE].draw();
      #endif 
       setVolume(current_volume );
       save_last_volstat(1);
  }
  
  if ( gmode == gStation ){
    
      current_station = getStation();
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
//------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------
#ifndef USEPIXELS
//--------------------------------------------------------------------------
 void tellPixels( uint32_t command ){
  ; //do nothing if pixels are not used, or soemthing else if screen is used.
 }

//--------------------------------------------------------------------------

void toggleStop( bool nostop ){
  int curvol;
  
  

        if ( currDisplayScreen == RADIO || currDisplayScreen == STNSELECT ){ 
          #ifndef USETOUCH
              log_i("releasing radio semaphore, if needed");
              if ( xSemaphoreGetMutexHolder( radioSemaphore ) != NULL ){
                   vs1053player->setVolume(0);
                   xSemaphoreGive( radioSemaphore); 
                   log_i("released radio semaphore %s", MuteActive?"mute is active":"mute is NOT active" );
              }
              #ifndef USESPECTRUM
                  if ( ! blackweather.created() ){
                    blackweather.createSprite( tft.width(), tft.height() - TFTCLOCKB  );  
                  }
    
                  blackweather.fillRect(0,0, tft.width(), tft.height() - TFTCLOCKB, TFT_BLACK);
  
                  grabTft();
                    blackweather.pushSprite( 0, TFTCLOCKB );
                  releaseTft(); 

                  blackweather.deleteSprite();
                  
              #endif
              drawWeather();
              tft_showstation( getStation() ); 
              
          #endif         
          
          // now play() will turn up the volume 
      }else{
          
          
          // turn radio off i.e. turn volume down 
          for ( curvol = getVolume(); curvol; --curvol ){
              vs1053player->setVolume( curvol  );
              delay( 5 );         
          }
          
          #ifndef USETOUCH
          if ( xSemaphoreGetMutexHolder( radioSemaphore ) == NULL ){  
               xSemaphoreTake(radioSemaphore, portMAX_DELAY);
          }
          #endif   
                                     
        }
        
        if ( currDisplayScreen == BLUETOOTH ){ 
            // turn on Bluetooth 
        }else{
            // turn off bluetooth 
        }

        if ( currDisplayScreen == LINEIN ){ 
            // turn on LINEIN 
        }else{
            // turn off LINEIN 
        }

        if ( currDisplayScreen == POWEROFF ){ 
            // power anything else off 
#ifndef USETOUCH
  
            drawWeather();
  
#endif
        }else{
            // power anything else on

        }

  
    showClock( true );
    
}
//--------------------------------------------------------------------------

void toggleMute(){
int curvol;
  if ( MuteActive ){
    
        if ( currDisplayScreen == RADIO ) {
          // turn the volume up
          skipstartsound=SKIPSTART;
          log_d("radio mute off- sound on");
        }
      
        if ( currDisplayScreen == BLUETOOTH ) {
          
          log_d("bluetooth mute off- sound on");
          // turn the volume up
        }
        if ( currDisplayScreen == LINEIN ) {
          // turn the volume up
          log_d("line in mute off- sound on");

        }

        #ifdef USETOUCH
          if ( currDisplayScreen != POWEROFF )touchbutton[BUTTON_MUTE].draw( false );
        #endif
        
        MuteActive = false;
          
  }else{
        #ifdef USETOUCH
          if ( currDisplayScreen != POWEROFF )touchbutton[BUTTON_MUTE].draw( true );
        #endif

        if ( currDisplayScreen == RADIO ) {
          
          
          for ( curvol = getVolume(); curvol; --curvol ){
            vs1053player->setVolume( curvol  );
            delay( 5 );         
          }
          log_d("radio mute on- sound off");

        }      

       if ( currDisplayScreen == BLUETOOTH ) {
          log_d("bluetooth mute on- sound off");
          // turn the volume down
        }
        if ( currDisplayScreen == LINEIN ) {
          log_d("linein mute on- sound off");

          // turn the volume down
        }

        MuteActive = true;
  }
  return; 
}
//--------------------------------------------------------------------------

//static uint16_t previous_barcolor;

void show_gesture_on(){
  //previous_barcolor = vs1053player->setSpectrumBarColor(TFT_RED);
  tft_show_gesture( true );
  
}

void show_gesture_off(){

  tft_show_gesture( false );
  //vs1053player->setSpectrumBarColor( previous_barcolor);
}


//--------------------------------------------------------------------------

#ifndef MULTILEVELGESTURES

// Single level gestures
  
  int parse_gestures( uint8_t data){
  int rc = 0;
    
    switch( gmode ){
      case gOff:
          switch(data){
              case GES_CLOCKWISE_FLAG:
              case GES_COUNT_CLOCKWISE_FLAG:
                gmode = gVolume;
                show_gesture_on();
                log_i("wake gesture sensor");
                setgTimer();
                break;
              default:
                //log_i("not listening to this gesture until wakeup");
                break;  
          }
          break;
      case gVolume:
          switch(data){
              case GES_DOWN_FLAG:
                log_i("lower volume");
                change_volstat( -1, gVolume );
                setgTimer();
                break;
              case GES_UP_FLAG:
                log_i("higher volume");
                change_volstat( 1, gVolume );
                setgTimer();
                break;
              case GES_LEFT_FLAG:
                log_i("previous station");
                change_volstat( -1, gStation );
                setgTimer();
                break;
              case GES_RIGHT_FLAG:
                log_i("next station");
                change_volstat( 1, gStation );
                setgTimer();
                break;
              case GES_FORWARD_FLAG:  
              case GES_BACKWARD_FLAG:  
                log_i("toggle mute");
                setgTimer();
                toggleMute();
                break;
              case GES_CLOCKWISE_FLAG:
              case GES_COUNT_CLOCKWISE_FLAG:
                show_gesture_off();
                log_i("stop listening for gestures");
                stopgTimer();
                gmode = gOff;
                break; 
              default:
                break;  
          }
          break;
      default:
            log_i( "Unknown gmode %d", gmode);
            break;
    }

   return(rc);
}

#else 


// Oranje radio gestures

int parse_gestures( uint8_t data){
int rc = 0;
    
    switch( gmode ){
      case gOff:
          switch(data){
              case GES_UP_FLAG:
                gmode = gVolume;
                show_gesture_on();
                log_i("wake gesture sensor, mode to gVolume");
                setgTimer();
                break;
              default:
                log_i("not listening to this gesture until wakeup");
                break;  
          }
          break;
      case gVolume:
          switch(data){
              case GES_LEFT_FLAG:
                log_i("lower volume");
                change_volstat( -1 );
                setgTimer();
                break;
              case GES_RIGHT_FLAG:
                log_i("higher volume");
                change_volstat( 1 );
                setgTimer();
                break;
              case GES_CLOCKWISE_FLAG:
                log_i("mode to gStation");
                gmode = gStation;
                setgTimer();
                break;
              case GES_DOWN_FLAG:
                log_i("stop listening for gestures");
                stopgTimer();
                gmode = gOff;
                show_gesture_off();
                break; 
              default:
                break;  
          }
          break;
      case gStation:
          switch(data){
              case GES_LEFT_FLAG:
                log_i("previous station");
                change_volstat( -1 );
                setgTimer();
                break;
              case GES_RIGHT_FLAG:
                log_i("next station");
                change_volstat( 1 );
                setgTimer();
                break;
              case GES_CLOCKWISE_FLAG:
                log_i("scroll mode");
                gmode = gScroll;
                setgTimer();
                break;
              case GES_DOWN_FLAG:
                log_i("stop listening for gestures");
                stopgTimer();
                gmode = gOff;
                show_gesture_off();
                break; 
              default:
                break;  
          }
          break;
       case gScroll:
          switch(data){
              case GES_CLOCKWISE_FLAG:
                log_i("scroll down the list of stations");
                tft_scrollstation( SCROLLUP );
                stopgTimer();
                break;
               case GES_COUNT_CLOCKWISE_FLAG:
                log_i("scroll up the list of stations");
                tft_scrollstation( SCROLLDOWN );
                stopgTimer();
                break;
              case GES_DOWN_FLAG:
                log_i("stop listening to gestures");
                
                xSemaphoreTake( chooseSemaphore, portMAX_DELAY);
                chosenStation = 2;
                xSemaphoreGive( chooseSemaphore);
               
                stopgTimer();
                gmode = gOff;
                show_gesture_off();
                break; 
              case GES_UP_FLAG:
                log_i("Change station to current station displayed");
                xSemaphoreTake( chooseSemaphore, portMAX_DELAY);
                chosenStation = 1;
                xSemaphoreGive( chooseSemaphore);
                break;   
              default:
                break;  
          }
          break;
          default:
            log_i( "Unknown gmode %d", gmode);
            break;
    }

   return(rc);
}
#endif
 
#else //USEPIXELS



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
void gToSleep( uint32_t fgcolor , int wait , int length ){

uint32_t color = fgcolor;
uint32_t b,e,half, whole, dimperc;

//if( length < 2 ) return; // prevent division by zero, no use of animating

whole = gstrip.ledcount() -1;
half  = whole / 2;
 
for ( b=0; b < half + length; ++b ){

       e = whole - b;
     
       for ( int j = 0 ; j < length; ++j ){ 
        
        dimperc = (100 - j*( 100/( length - 1) ));
        if ( b-j > half )dimperc = 0; 

  
        if ( b-j >= whole) gstrip.color32( b-j, color, dimperc );
        if ( e+j <= whole) gstrip.color32( e+j, color, dimperc );
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

//if( length < 2 ) return; // prevent division by zero, no use of animating


whole = gstrip.ledcount(); // -1
half  = whole / 2;
   
for ( b = 0; b <= half; ++b ){
        
        dimperc = 100; //j*( 100/( length - 1) );

        if ( half+b < whole){
            //Serial.printf( "%d to bg+\n", half+b);
            gstrip.color32( half+b, bg, dimperc );
        }
        if ( half-b >= 0){
            //Serial.printf( "%d to bg-\n", half-b);
            gstrip.color32( half-b, bg, dimperc );
        }      
       
      gstrip.show();
      
      delay( wait); 
      
}

return;
}


//--------------------------------------------------------------------------------------

// max LED brightness (1 to 255) – start with low values!
// (please note that high brightness requires a LOT of power)
// max brightnessis already reduced in gpixelbegin ,gstrip.setbrightness( brightness);
#define DECOBRIGHT 80
// increase to get narrow spots, decrease to get wider spots
#define DECOFOCUS  40
// decrease to speed up, increase to slow down (it's not a delay actually)
#define DECODELAY  3500

void gDeco( bool decoinit){
static float offset;
static float spdr, spdg, spdb,spdw;

 
  if ( decoinit ){          
          randomSeed( esp_random());

          // assign random speed to each spot
          spdr = 1.0 + random(200) / 100.0;
          spdg = 1.0 + random(200) / 100.0;
          spdb = 1.0 + random(200) / 100.0;
          spdw = 1.0 + random(200) / 100.0;

          offset = random(10000) / 100.0;

          Serial.printf( "Deco init...\n");
  }
         
  long ms = millis();
  // scale time to float value
  float m = offset + (float)ms/DECODELAY;
  // add some non-linearity
  m = m - 42.5*cos(m/552.0) - 6.5*cos(m/142.0);


  // recalculate position of each spot (measured on a scale of 0 to 1)
  float posr = 0.5 + 0.55*sin(m*spdr);
  float posg = 0.5 + 0.55*sin(m*spdg);
  float posb = 0.5 + 0.55*sin(m*spdb);
  float posw = 0.5 + 0.55*sin(m*spdw);

  // now iterate over each pixel and calculate it's color
  for (int i=0; i< gstrip.ledcount() ; i++) {
    // pixel position on a scale from 0.0 to 1.0
    float ppos = (float)i / gstrip.ledcount();
 
    // distance from this pixel to the center of each color spot
    float dr = ppos-posr;
    float dg = ppos-posg;
    float db = ppos-posb;
    float dw = ppos-posw;
    uint8_t R,G,B,W;
          
    // set each color component from 0 to max BRIGHTNESS, according to Gaussian distribution
    uint8_t ave=0,min=255;
    R = (uint8_t) constrain(DECOBRIGHT*myexp(-DECOFOCUS*dr*dr),0,DECOBRIGHT);
    if ( R < min){ min = R; ave = R;}
    G = (uint8_t) constrain(DECOBRIGHT*myexp(-DECOFOCUS*dg*dg),0,DECOBRIGHT);
    if ( G < min) { min = G; ave += G;}
    B = (uint8_t) constrain(DECOBRIGHT*myexp(-DECOFOCUS*db*db),0,DECOBRIGHT);
    if ( B < min) { min = B; ave += B;}
    W = (uint8_t) (uint8_t) constrain(DECOBRIGHT*myexp(-DECOFOCUS*dw*dw),0,DECOBRIGHT);
    if ( W < min) { min = W; ave += W;}
          
    //Serial.printf("deco R: %d G: %d B: %d W: %d\n", R,G,B, W);
    gstrip.color(i, R,G,B,W,-1);
                  
 }
        
gstrip.show();

}


/*------------------------------------------------------------------------------*/
float myexp(float x) { //exponent approximation
  return (1.0/(1.0-(0.634-1.344*x)*x));
}
/*------------------------------------------------------------------------------*/
void doPixels( void *param ){
static uint32_t  bg=0;
uint32_t         pixelCommand, longwait=1;

int   previous_command=0;

  
  Serial.printf("Pixels running on core %d\n", xPortGetCoreID()); 
     
  while(1){

  
    if ( longwait ){
      Serial.printf("Pixels waiting on events\n" ); 
      
      xTaskNotifyWait(0,0,&pixelCommand,portMAX_DELAY);
    
      Serial.printf("doPixels got command %d (mode = %d, goodbyeCount %d, oldGoodbyeCount %d)\n", pixelCommand, gmode, goodbyeCount,oldGoodbyeCount );
    }

    if ( pixelCommand >= PIX_BLINKRED && pixelCommand <= PIX_BLINKYELLOW ){
      
      uint32_t kleur; 

      longwait = 0;
      previous_command = pixelCommand;
          
      switch( pixelCommand ){
        
        case PIX_BLINKRED:
          kleur = gstrip.getcolor(50,0,0,0 );
          break;      
        case PIX_BLINKGREEN:
          kleur = gstrip.getcolor(0,50,0,0 );
          break;  
        case PIX_BLINKBLUE:
          kleur = gstrip.getcolor(0,0,50,0 );
          break;      
        case PIX_BLINKYELLOW:
          kleur = gstrip.getcolor(0,0,0,50 );
          break;      
      }  
          gWakeup( kleur, 20, 5 );
          delay(200);
          gToSleep( kleur, 20, 3 );
          delay(300);

          xTaskNotifyWait(0,0,&pixelCommand,10);
                          
      continue;
    }

    if ( pixelCommand == PIX_DECO ){

        longwait = 0;

        if ( previous_command != PIX_DECO ){
          previous_command = pixelCommand;
          gDeco( true );  
        }else{
          gDeco( false );
        }

        delay(50);
        
        xTaskNotifyWait(0,0,&pixelCommand,10);
        continue;     
    }

    longwait = 1;
    previous_command = pixelCommand;
    
    if ( pixelCommand >= PIX_RED && pixelCommand <= PIX_YELLOW ){
      switch( pixelCommand ){
        case PIX_RED:
          gWakeup( gstrip.getcolor(50,0,0,0 ), 50, 5 );
          break;      
        case PIX_GREEN:
          gWakeup( gstrip.getcolor(0,5,0,0 ), 50, 5 );         
          break;  
        case PIX_BLUE:
          gWakeup( gstrip.getcolor(0,0,20,0 ), 50, 5 );
          break;      
        case PIX_YELLOW:
          gWakeup( gstrip.getcolor(0,0,0,20 ), 50, 5 );
          break;      
      }

      continue;
    }


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
      case PIX_BLACKC: // turn pixels off
        if ( gmode == 3){
            xSemaphoreTake( chooseSemaphore, portMAX_DELAY);
            chosenStation = 2;
            xSemaphoreGive( chooseSemaphore); 
        }
        gmode = 0;
        gToSleep( bg , 100, 5 );
        break;
      case PIX_WAKEUP: // turn pixels on
        gWakeup( bg, 50, 5 );
        break;          
      case PIX_RIGHT: // right, green snake
        gLeft( gstrip.getcolor( 0,100,0,0) , 40, 8, bg);
        change_volstat( 1);
        break;
      case PIX_LEFT: // left red snake
        gRight( gstrip.getcolor( 100,0,0,0), 40, 8, bg );
        change_volstat( -1 );
        break;
      case PIX_CONFIRM: // confirm
        if ( gmode == 3){
            xSemaphoreTake( chooseSemaphore, portMAX_DELAY);
            chosenStation = 1;
            xSemaphoreGive( chooseSemaphore); 
        }
        gWakeup( gstrip.getcolor( 0,100,0,0) , 10, 6 );
        delay(400);
        gWakeup( bg , 1, 6 );
        break;          
      case PIX_SCROLLUP:// clockwise only in mode 3
        tft_scrollstation( SCROLLUP );
        gWakeup( bg, 50, 5 );
        break;  
      case PIX_SCROLLDOWN:
        tft_scrollstation( SCROLLDOWN );
        gWakeup( bg, 50, 5 );
        break;            
      case 10:
        gToSleep( bg , 100, 5 );
        delay(100);
        gToSleep( bg , 100, 5 );
        break;
      case PIX_MUTE:
        doMute(bg,0);
        break;
      case PIX_UNMUTE:
        doMute(bg,1);
        break;
      case PIX_BLACK:
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

static uint32_t previous_command=-1;

  Serial.printf("tellPixels gmode = %d :  command %d \n", gmode, command );
  
  if ( command == PIX_MUTE ){ //mute
         xTaskNotify( pixelTask, 11,eSetValueWithOverwrite);          
         //return; 
  }
  if ( command == PIX_UNMUTE ){ //unmute
         xTaskNotify( pixelTask, 12,eSetValueWithOverwrite);          
         //return; 
  }
  
  
  if ( gmode == 0 ){
     switch( command ){
       case PIX_BLACK:
          if ( oldGoodbyeCount != goodbyeCount) {
              Serial.printf("gmode = 0 : goodbyeCount %d, oldGoodbyeCount %d\n", goodbyeCount,oldGoodbyeCount );
              oldGoodbyeCount = goodbyeCount;
          }
          stopgTimer();
          break; 
       case 0: // down
          stopgTimer();
          xTaskNotify( pixelTask, PIX_BLACKC,eSetValueWithOverwrite);     
          break;
       case 9: //up
        gmode = 1;
        setgTimer(); 
        xTaskNotify( pixelTask, PIX_WAKEUP,eSetValueWithOverwrite);     
        break;
      default:
        break;       
      }
      return;
  }
  

  if ( gmode ){
    switch(command){
        case PIX_BLACK:
          if ( oldGoodbyeCount != goodbyeCount){
            Serial.printf("Do 911, oldgodbye = %d, goodbye %d\n", oldGoodbyeCount, goodbyeCount ); 
            oldGoodbyeCount = goodbyeCount;
            gmode = 0;
            xTaskNotify( pixelTask, PIX_BLACK,eSetValueWithOverwrite);     
          }
          break;          
        case PIX_BLACKC: //down
         
            //if( gmode != 3 ) gmode = 0;
            stopgTimer();
            xTaskNotify( pixelTask, PIX_BLACKC,eSetValueWithOverwrite);     
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
          xTaskNotify( pixelTask, PIX_CONFIRM,eSetValueWithOverwrite);     
          break;
        case 10: //fw backw; stop
          stopgTimer();
          gmode = 0;        
          xTaskNotify( pixelTask, PIX_STOP,eSetValueWithOverwrite);              
          break;
        case 11:
        case 12:
          break;  
        case 21:  
        case 22:  
        case 23:  
        case 24:  
        case 41:  
        case 42:  
        case 43:
        case 44:
        case 51:
            if ( command != previous_command ){
              xTaskNotify( pixelTask, command,eSetValueWithOverwrite);     
              previous_command = command;
            }
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

void gpixelBegin( int brightness )
{ 

gstrip.begin( NEOPIN, NEONUMBER );
gstrip.setbrightness(brightness);
gstrip.clear();



}

/*------------------------------------------------------------------------------*/

void initPixels(){
  
    gpixelBegin( 80);

    delay(100);
    
    xTaskCreate( 
         doPixels,                                      // Task to handle special functions.
         "Pixels",                                            // name of task.
         3*1024,                                                 // Stack size of task
         NULL,                                                 // parameter of the task
         PIXELTASKPRIO,                                                    // priority of the task
         &pixelTask);                                               // Task handle to keep track of created task 

   
}
#endif
