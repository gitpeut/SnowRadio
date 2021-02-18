#include "tft.h"

#define BATVREF     1.1f
#define BATPINCOEF  1.95f // tune -6 db
#define BATDIV      5.54f // (1M + 220k )/220k


#define SCROLLPIN 0
int station_scroll_h=TFTSTATIONH;

//change rotation if necessary
//depends on you set up your display.
int tftrotation = 2;

int verysmallfont= 1;
int smallfont= 2;
int bigfont=4;
int segmentfont = CLOCK_FONT;



//
//----------------------------------------------------------
void IRAM_ATTR grabTft(){
  //printf("grab TFT\n");
  xSemaphoreTake( tftSemaphore, portMAX_DELAY);
  //printf("grabbbed TFT\n");

}
//----------------------------------------------------------
void IRAM_ATTR releaseTft(){
  
//tft.fillRect( 4,12 , 1, 1, TFT_REALGOLD ); //flicker kludge, not necessary when using rmt_write_items in sk.h
xSemaphoreGive( tftSemaphore); 
//printf("released TFT\n");
}
//----------------------------------------------------------

void tft_show_gesture( bool showonscreen ){
int   w=32;
int   h=TFTINDICH;
int   xpos=33 + (BUTOFFSET*2),ypos=TFTINDICT;
char  geststring[4];

   gest.createSprite(w,h);
   gest.fillSprite(TFT_BLACK);          

   if ( showonscreen ){
      gest.setFreeFont( &indicator );
      geststring[0] = 92; //little hand
      geststring[1] = 0;   
      gest.setTextColor( TFT_WHITE, TFT_BLACK ); 
      gest.drawString( geststring , 0,0 );
   }


grabTft();
  gest.pushSprite( xpos, ypos);
releaseTft();  

gest.deleteSprite();  
}
//----------------------------------------------------------

int read_battery(){
  int i;
  int     batread=0, batotal=0;
  
  for(i=0;i<5;i++){
      batread = analogRead( BATPIN );
      batotal += batread;  
      delay(50);
  }

  batread = batotal/5;
  batvolt = ( BATVREF * ( batread * BATDIV)/4096)* BATPINCOEF;

  //Serial.printf( "read : %d, voltage: %f\n", batread, batvolt);
  if ( batread > 1424 ) return(100);
  if ( batread > 1404 ) return( 90);
  if ( batread > 1362 ) return( 80);
  if ( batread > 1345 ) return( 70);
  if ( batread > 1321 ) return( 60);
  if ( batread > 1300 ) return( 50);
  if ( batread > 1280 ) return( 40);
  if ( batread > 1262 ) return( 30);
  if ( batread > 1242 ) return( 20);
  if ( batread > 1220 ) return( 10);
  return(0);  
 
}
//-------------------------------------------------------------------------------
void showBattery(){

  int   ypos = TFTINDICT;
  int   xpos = tft.width() - 32 - BUTOFFSET;
  int   percentage = read_battery();
  char  batstring[2]; 
  
  batstring[0]= 'L' + percentage/25;
  batstring[1]= 0;
 
    bats.createSprite(32,32);
    bats.setFreeFont( &indicator );
    bats.setTextColor( TFT_WHITE, TFT_BLACK ); 
    bats.fillSprite(TFT_BLACK);          
    bats.drawString( batstring , 0,0 );

    grabTft();
      //log_d("Push battery sprite to x %d, y%d", 0, ypos);
    bats.pushSprite( xpos, ypos);
    releaseTft();

    bats.deleteSprite();

}

//----------------------------------------------------------
void showVolume( int percentage ){
// indicator font is 32 wide and 32 high.
  int   ypos = TFTINDICT;
  char  volstring[2]; 
  
  volstring[0]= 'A' + percentage/10;
  volstring[1]= 0;

 
    vols.createSprite(32,32);
    vols.setFreeFont( &indicator );
    vols.setTextColor( TFT_WHITE, TFT_BLACK ); 
    vols.fillSprite(TFT_BLACK);          
    vols.drawString( volstring ,0 ,0 );

    grabTft();
      //log_d("Push sprite to x %d, y%d", 0, ypos);
    vols.pushSprite( BUTOFFSET, ypos);
    releaseTft();

    vols.deleteSprite();
 
  return;  
}


//--------------------------------------------------------------------------------------------------------
void showClock ( int hour, int min, int date, int mon, int yy ){
int     spritey =  TFTCLOCKT;
int     clockx, datex, clockw, datew, spritew, spritex;                                                     
int     spriteh = TFTCLOCKH;
char    tijd[8], datestring[32];

sprintf(tijd,"%02d:%02d", hour, min);   
clockw = tft.textWidth( tijd, segmentfont );

sprintf( datestring,"%d %s %d", date, monthnames[mon], yy);   
datew = tft.textWidth( datestring, bigfont );


if (datew > clockw){
  spritew = datew + 2;
  datex   = 1;
  clockx  = (spritew-clockw)/2;  
}else{
  spritew = clockw + 2;
  clockx  = 1;
  datex   = (spritew-datew)/2;    
}

spritex = (tft.width() - spritew)/2;

//log_d("spriteh %d spritey %d spritex %d clockw = %d clockx = %d, datew %d, datex %d", spriteh, spritey, spritex, clockw, clockx, datew, datex);

clocks.createSprite(  spritew, spriteh );
clocks.setTextColor( TFT_REALGOLD, TFT_BLACK ); 
clocks.fillSprite(TFT_BLACK);

clocks.drawString( tijd, clockx, 2, segmentfont); 
clocks.drawString( datestring, datex, tft.fontHeight( segmentfont) + 4, bigfont); 



grabTft();
clocks.pushSprite( spritex, spritey );
releaseTft();
  
clocks.deleteSprite();

showBattery();
showVolume( getVolume() );

}


//--------------------------------------------------------------------------------------------------------
//used for testing
/*
void IRAM_ATTR choose_scrollstation(){
int started=0;
  
    if (  xSemaphoreTakeFromISR( chooseSemaphore, NULL ) == pdTRUE ){
        chosenStation = 1;
        xSemaphoreGiveFromISR( chooseSemaphore, &started );  
        if ( started ){
           portYIELD_FROM_ISR()
        }

    }   
    
}
*/
//---------------------------------------------------------------------
void tft_showstation( int stationIdx){

int   xpos = 0, ypos=2;
char  *t, *s = stations[stationIdx].name;
char  topline[64], bottomline[64];



img.createSprite(tft.width(), station_scroll_h );
img.setTextColor( TFT_WHITE, TFT_BLACK ); 

img.fillSprite(TFT_BLACK);
img.setFreeFont( &STATION_FONT );


int wholew = img.textWidth( stations[stationIdx].name, GFXFF );
//int h = img.fontHeight(GFXFF);


if ( wholew >= tft.width() ){
   for( s = stations[stationIdx].name, t = topline; !isspace(*s ); ++s){
      *t = *s;
      ++t;
   }
   *t =  0;
   ++s;
   for( t = bottomline; *s ; ++s){
      *t = *s;
      ++t;
   }
   *t =  0;

   int linew = img.textWidth( topline, GFXFF );
   xpos = (tft.width() - linew)/2; 
   
   img.drawString( topline, xpos, ypos, GFXFF);

   ypos += img.fontHeight( GFXFF);
    
   linew = img.textWidth( bottomline, GFXFF );
   xpos = (tft.width() - linew)/2; 
   img.drawString( bottomline, xpos, ypos, GFXFF);
 
}else{
   xpos = (tft.width() - wholew)/2; 
   ypos = (station_scroll_h - img.fontHeight(GFXFF) )/2;
   img.drawString( stations[stationIdx].name, xpos, ypos,GFXFF);
}

grabTft();
img.pushSprite( 0, TFTSTATIONT );
releaseTft();

img.deleteSprite();
}



#ifdef MULTILEVELGESTURES
//---------------------------------------------------------------------
void tft_scrollstations( void *param ){
int direction;
int t,begint, endt, inct, tcount;
int beginx, endx, incx;
int halve = (tft.width()/2); 
int stopped =0;
int delaytime;

  xSemaphoreTake( scrollSemaphore, portMAX_DELAY);

  stopgTimer();
  
  scrollStation = -1;
  chosenStation = 0;
  begint        = currentStation;
  
  //pinMode( SCROLLPIN,INPUT_PULLUP );
  //attachInterrupt( SCROLLPIN, choose_scrollstation, CHANGE);
 
  direction = *((int *)param);
  begint  = currentStation;
  endt    = stationCount;
  if( endt > 40 ) endt = 40; 
  
  img.createSprite(tft.width(), station_scroll_h);
  img.setTextColor( TFT_REALGOLD, TFT_BLACK ); 
  img.setTextSize(1);

  
  if ( direction == SCROLLUP ){ //station increase, scroll right to left 
    inct    = 1; 
    beginx  = tft.width();
    endx    = ( -1*tft.width() );
    incx    = -20;
    tcount = 0;
    Serial.printf("scrollup\n");
  }

  if ( direction == SCROLLDOWN ){ //station increase, scroll right to left 
    inct    = -1;
    beginx  = (-1*tft.width());
    endx    = tft.width() + 5;
    incx    = 20;
    tcount = 0;
    Serial.printf("scrolldown\n");
  }
  
    
for ( t=begint; tcount < endt; t+= inct, tcount++ ){

  //delaytime = (tcount*tcount*100/endt)+100;
  delaytime = (tcount*tcount*100/endt)+300;
  
  if ( direction == SCROLLUP )incx = tcount - endt;
  if ( direction == SCROLLDOWN )incx = endt - tcount;
  
  if ( t > (stationCount-1) ) t = 0;
  if ( t < 0 ) t = (stationCount-1);

  int z = t + inct;           
  if ( z > (stationCount-1) ) z = 0;
  if ( z < 0 ) z = (stationCount-1);

 
  for ( int x = 0, stopped=0;1; x += incx ){

        //Serial.printf("%s incx %d x = %d endx %d \n", direction == SCROLLDOWN?"scrolldown":"scrollup", incx,  x, endx );
        if ( direction == SCROLLUP   && x <= endx )break;
        if ( direction == SCROLLDOWN && x >= endx )break;
                  
        img.fillSprite(TFT_BLACK);

        tft_showstations( t,  x ); 

        if ( direction == SCROLLUP ){
          tft_showstations( z,  x + tft.width()); 
        }else{
          tft_showstations( z,  x - tft.width()); 
          img.drawFastVLine(0,0, img.height(), TFT_BLACK);//bug somewhere in lib or display            
        }
        
        grabTft();
        img.pushSprite( 0, tft.height()-station_scroll_h );
        releaseTft();
                    
        if ( stopped == 0 && x <= halve ){ 
           scrollStation = t;
           delay(  delaytime );
           stopped = 1;     
           if ( x == 0 && tcount == (endt-1) ) break;

          xSemaphoreTake( chooseSemaphore, portMAX_DELAY);
          if (chosenStation)break;                        
          xSemaphoreGive( chooseSemaphore); 

           
        }             
  }//end for x loop

  if (chosenStation){
      if ( chosenStation == 2) tcount = endt; // abort
      chosenStation  = 0;
      xSemaphoreGive( chooseSemaphore); 
      break;
  }

  delay(5);
}//end for t looop

img.deleteSprite();

if ( tcount < endt ){
   setStation( t, -1 );
   tft_showstation( t );
   //currentStation = t;
   //delay(4000);
}else{
     tft_showstation( getStation() );
}

//detachInterrupt( SCROLLPIN );
scrollDirection = -1;

gmode = 2;
setgTimer();

xSemaphoreGive( scrollSemaphore); 

 
vTaskDelete( NULL );
    
}


// -------------------------------------------------------------------------

void tft_scrollstation(int whatway){
  
    xSemaphoreTake( scrollSemaphore, portMAX_DELAY);
    scrollDirection = whatway;

    Serial.printf("scroll %d %s\n", whatway, whatway?"from left to right":"from right to left" );
        
    int rc = xTaskCreate( 
         tft_scrollstations,                                      // Task to handle special functions.
         "Scroll",                                            // name of task.
         32*1024,                                                 // Stack size of task
         &scrollDirection,                                                 // parameter of the task
         SCROLLTASKPRIO,                                                    // priority of the task
         &scrollTask);                                               // Task handle to keep track of created task 

    Serial.printf("xTaskCreate rc %d \n", rc);

    delay(10);
    xSemaphoreGive( scrollSemaphore);
      
    
}
#endif

//------------------------------------------------------

void tft_uploadProgress( int percent ){

int hi;
int percentage = percent;

if ( percentage > 100 ) percentage = 100;
  
  hi = (tft.height() * percentage )/100; 

  grabTft();
  tft.fillRect( tft.width() - 12, tft.height() - hi, 12, hi, TFT_YELLOW );
  releaseTft();
}

//------------------------------------------------------
void tft_notAvailable( int stationIdx){
int w;

grabTft();
tft.fillRect(0,80, tft.width(), tft.height()-80, TFT_BLACK );
tft.setTextColor( TFT_RED, TFT_WHITE );

w = tft.textWidth( stations[stationIdx].name,2 );
tft.drawString( stations[stationIdx].name, 80-(w/2), 80, 2 );    
w = tft.textWidth( "not available",2 );             
tft.drawString( "not available", 80-(w/2), 94, 2 );                 
releaseTft();

}


//------------------------------------------------------

void tft_ShowUpload(String uploadtype){
 grabTft(); 
 tft.fillScreen(TFT_BLACK);
 tft.setTextColor(TFT_WHITE );

 tft.drawString( uploadtype, 10, 26, 4 );

 tft.drawString( "upload", 10, 52, 4 );
 tft.drawString( "in progress", 10, 78, 4 );
 releaseTft(); 
}

//------------------------------------------------------

void tft_uploadEnd( String uploadstatus){
 
grabTft();
 if ( uploadstatus.startsWith("s") ){
   tft.fillScreen(TFT_WHITE);
   tft.setTextColor( TFT_BLACK );
 }else{
   tft.fillScreen(TFT_GREEN); 
   tft.setTextColor( TFT_RED );
 }

 tft.drawString( "upload", 10, 52, bigfont );
 tft.drawString( uploadstatus, 10, 78, bigfont );
releaseTft();
 
 if ( ! uploadstatus.startsWith("s") ){
      delay(5000);
      ESP.restart();
 }
}
//------------------------------------------------------

void tft_NoConnect( WiFiManager *wm) {
 tft.setRotation( tftrotation );  

// tft.fillScreen(TFT_BLACK);
// tft.setTextColor( TFT_WHITE );
log_d("tftnoconnect");
 tft.fillScreen(TFT_WHITE);
 tft.setTextColor( TFT_BLACK );


 tft.drawString( "Connect to network ", 10, 20, smallfont );
 tft.drawString( wm->getConfigPortalSSID(), 10, 34,bigfont );
 tft.drawString( "WiFi password ", 10, 52, smallfont );

 tft.drawString( APPAS, 10, 66, bigfont );
 tft.drawString( "Browse to", 10, 86,smallfont );
 tft.drawString( "192.168.4.1", 10, 102, bigfont );

 tellPixels( PIX_RED );

}

//------------------------------------------------------------------------------------------
// Bodmers BMP image rendering function

void drawBmp(const char *filename, int16_t x, int16_t y, TFT_eSprite *sprite ) {

  if ((x >= tft.width()) || (y >= tft.height())) return;

  fs::File bmpFS;

  // Open requested file on SD card
  bmpFS = RadioFS.open(filename, "r");

  if (!bmpFS)
  {
    Serial.print("File not found");
    return;
  }

  uint32_t seekOffset;
  uint16_t w, h, row;
  uint8_t  r, g, b;

  uint32_t startTime = millis();

  if (read16(bmpFS) == 0x4D42)
  {
    read32(bmpFS);
    read32(bmpFS);
    seekOffset = read32(bmpFS);
    read32(bmpFS);
    w = read32(bmpFS);
    h = read32(bmpFS);

    if ((read16(bmpFS) == 1) && (read16(bmpFS) == 24) && (read32(bmpFS) == 0))
    {
      y += h - 1;

      tft.setSwapBytes(true);
      bmpFS.seek(seekOffset);

      uint16_t padding = (4 - ((w * 3) & 3)) & 3;
      uint8_t lineBuffer[w * 3 + padding];

      for (row = 0; row < h; row++) {
        
        bmpFS.read(lineBuffer, sizeof(lineBuffer));
        uint8_t*  bptr = lineBuffer;
        uint16_t* tptr = (uint16_t*)lineBuffer;
        // Convert 24 to 16 bit colours
        for (uint16_t col = 0; col < w; col++)
        {
          b = *bptr++;
          g = *bptr++;
          r = *bptr++;
          *tptr++ = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
        }

        // Push the pixel row to screen, pushImage will crop the line if needed
        // y is decremented as the BMP image is drawn bottom up
        grabTft();
        if ( sprite == NULL ){
          tft.pushImage(x, y--, w, 1, (uint16_t*)lineBuffer);
        }else{
          sprite->pushImage(x, y--, w, 1, (uint16_t*)lineBuffer);
        }
        releaseTft();
      }
      Serial.print("Loaded in "); Serial.print(millis() - startTime);
      Serial.println(" ms");
    }
    else Serial.println("BMP format not recognized.");
  }
  bmpFS.close();
  tft.setSwapBytes( false );// handy when proper colors are expected afterwards :-)jb
}

// These read 16- and 32-bit types from the SD card file.
// BMP data is stored little-endian, Arduino is little-endian too.
// May need to reverse subscript order if porting elsewhere.

uint16_t read16(fs::File &f) {
  uint16_t result;
  ((uint8_t *)&result)[0] = f.read(); // LSB
  ((uint8_t *)&result)[1] = f.read(); // MSB
  return result;
}

uint32_t read32(fs::File &f) {
  uint32_t result;
  ((uint8_t *)&result)[0] = f.read(); // LSB
  ((uint8_t *)&result)[1] = f.read();
  ((uint8_t *)&result)[2] = f.read();
  ((uint8_t *)&result)[3] = f.read(); // MSB
  return result;
}
//------------------------------------------------------

void tft_backlight( int onoff ){ 

  if ( onoff ){
    digitalWrite( TFT_LED , HIGH);
  }else{
    digitalWrite( TFT_LED , LOW);
  }
}

//------------------------------------------------------

void tft_init(){

   
  pinMode( TFT_RST , OUTPUT);
  digitalWrite( TFT_RST , LOW);
  delay( 10);
  digitalWrite( TFT_RST , HIGH);

  pinMode( TFT_LED , OUTPUT);
  tft_backlight( 0 );


// read battery
//  pinMode(BATPIN, INPUT);
//  analogSetAttenuation(ADC_6db);


  tft.init();
  //Serial.printf("------- tft width = %d tft height = %d\n", tft.width(), tft.height() ); 
  if( tft.width() > tft.height() ) tftrotation = 1;
  tft.setRotation( tftrotation );
  tft.fillScreen(TFT_BLACK);
  
  #ifdef USETOUCH    
    touch_init();    
  #endif  

  delay(200);

  log_i("tft initialized");
  
}
