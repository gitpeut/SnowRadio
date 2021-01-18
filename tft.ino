
#include <TFT_eSPI.h> // Graphics and font library for ST7735 driver chip


TFT_eSPI tft = TFT_eSPI();  // Invoke library, pins defined in User_Setup.h

TFT_eSprite img  = TFT_eSprite(&tft);  // Create Sprite object "img" with pointer to "tft" object
TFT_eSprite bats  = TFT_eSprite(&tft); 
TFT_eSprite vols  = TFT_eSprite(&tft);  
TFT_eSprite clocks  = TFT_eSprite(&tft);  
TFT_eSprite bmp  = TFT_eSprite(&tft);  


#define BATVREF     1.1f
#define BATPINCOEF  1.95f // tune -6 db
#define BATDIV      5.54f // (1M + 220k )/220k


#define SCROLLPIN 0
int STATIONSCROLLH=55;
int tftrotation = 4;

int verysmallfont= 1;
int smallfont= 2;
int bigfont=4;


//----------------------------------------------------------
void IRAM_ATTR grabTft(){
  //printf("grab TFT\n");
  xSemaphoreTake( tftSemaphore, portMAX_DELAY);
  //printf("grabbbed TFT\n");

}
//----------------------------------------------------------
void IRAM_ATTR releaseTft(){
  
//tft.fillRect( 4,12 , 1, 1, TFT_ORANGE ); //flicker kludge, not necessary when using rmt_write_items in sk.h
xSemaphoreGive( tftSemaphore); 
//printf("released TFT\n");
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

  Serial.printf( "read : %d, voltage: %f\n", batread, batvolt);
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
int   w=tft.width()/6,h=tft.height()/14;
int   xpos,ypos;
int   percentage = read_battery();
int   multiplier=1;

if ( tft.width() > 200 ) multiplier = 2;

//bats.createSprite(23+2,h);
bats.createSprite(w+2,h);
bats.setTextColor( TFT_WHITE, TFT_ORANGE); 
bats.setTextSize( multiplier );
bats.fillSprite(TFT_BLACK);


bats.fillRect( w, h/3, 2, h/3, TFT_ORANGE );//battery positive
bats.drawRoundRect( 0, 0, w , h, 2,TFT_ORANGE );//rectangle
bats.fillRect( 1, 1, w-2, h-2, TFT_BLACK );// inside

if ( percentage > 20 ){
  bats.fillRect( 2, 2, 4, h-4, TFT_ORANGE );
}
if ( percentage > 40 ){
  bats.fillRect( 2 + 5, 2, 4, h-4, TFT_ORANGE );
}
if ( percentage > 60 ){
  bats.fillRect( 2 + 10, 2, 4, h-4, TFT_ORANGE );
}
if ( percentage > 80 ){
  //printf(">80\n");
  bats.fillRect( 2 + 15, 2, 4, h-4, TFT_ORANGE );
}

//tft.fillRect( w+2, h/2, 1, 1, TFT_ORANGE ); //flicker kludge
  
grabTft();
xpos = tft.width()/2 - (w+2)/2;

ypos =  bats.fontHeight( smallfont ) * multiplier/ 3;

//ypos = 2;
bats.pushSprite( xpos, ypos);
releaseTft();

bats.deleteSprite();

}

//----------------------------------------------------------

void showVolume( int percentage){
//int   w=30,h=10;
int   w,h,percw,multiplier=1;
int   xpos=2, ypos=1;
char  pstring[8];

if ( tft.width() > 200 ) multiplier = 2;

w = (tft.width() * 3)/16;
h =  tft.fontHeight( smallfont ) + 2;

percw = vols.textWidth( "999",smallfont) * multiplier; 

vols.createSprite(w + percw+1,h + 2);
vols.setTextColor( TFT_WHITE, TFT_ORANGE ); 
vols.setTextSize( multiplier );
vols.fillSprite(TFT_BLACK);


sprintf( pstring,"%-3d", percentage);

vols.fillRect( 0, 1, w , h,TFT_ORANGE );//rectangle
vols.fillRect( 0, 0, w-1 , h, TFT_BLACK );//rectangle
                         
vols.fillTriangle( 0,     h,   //lower left     
                  (w * percentage)/100 - 1,     h,  //lower right   
                  (w * percentage)/100 - 1,     (h * (100-percentage))/100, //upper right   
                  TFT_ORANGE );//rectangle

vols.setTextColor(TFT_ORANGE);
vols.fillRect( w +1, 1, percw , h,TFT_BLACK );//rectangle

ypos =  h/4;
vols.drawString( pstring,w + 2, ypos, verysmallfont); 

grabTft();
xpos = 2;
ypos = 2;
vols.pushSprite( xpos, ypos);
releaseTft();

vols.deleteSprite();

}

//--------------------------------------------------------------------------------------------------------

void showClock ( int hour, int min){
int clockx=tft.width(), clocky=2,w;
int multiplier=1;
char   tijd[8];

if ( tft.width() > 200 ) multiplier = 2;

sprintf(tijd,"%d : %02d", hour, min);   
w = tft.textWidth( tijd, smallfont ) * multiplier; //smallfont

clockx = clockx - w - 5;

clocks.createSprite(w+20, tft.fontHeight( smallfont)*multiplier +2 );
clocks.setTextColor( TFT_GREEN, TFT_BLACK ); 
clocks.fillSprite(TFT_BLACK);
clocks.setTextSize( multiplier );

clocks.drawString( tijd,0, 1, smallfont); 

grabTft();
clocks.pushSprite( clockx, clocky );
releaseTft();
  
clocks.deleteSprite();

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
//--------------------------------------------------------------------------------------------------------

void tft_showstations( int stationIdx, int  spritex){

int   xpos, w = 0,f=4,f1=2, halve;
char  sname[80];
char  *s, *word[2]={NULL,NULL}; 


  //Serial.printf("--------Scrollheight %d ( bigfontheight = %d)\n", STATIONSCROLLH, tft.fontHeight( bigfont)  );
  strcpy( sname , stations[stationIdx].name); 

  halve = (img.width()/2);
  
  if ( (w = img.textWidth(sname, bigfont)) <= img.width() ){

    //Serial.printf( "----- text width %d  < %d, using font %d\n", img.textWidth(sname, bigfont), img.width(), bigfont);
      
    w = img.textWidth( sname, bigfont ); 
    xpos = halve - w/2;               //22//STATIONSCROLLH/2 
    img.drawString(sname, spritex+xpos, STATIONSCROLLH/2 - img.fontHeight( bigfont)/2 , bigfont);    

    //Serial.printf( "----- drawstring %s at x %d y %d\n", sname, spritex+xpos, STATIONSCROLLH/2);

  }else{
        //Serial.printf( "----- text width %d  > %d\n", img.textWidth(sname, bigfont), img.width());
        
        word[0] = sname;
        for( s = sname; *s && *s != ' '; ++s );
        if ( *s ) word[1] = s + 1;
        *s = 0; 
      
         if ( word[1] == NULL ){
                 w = img.textWidth( word[0], smallfont );
                 xpos = halve - w/2;                  //img.height()/2 -4 //tft.fontHeight( smallfont)
                 img.drawString(word[0], spritex+xpos, img.height()/2 - img.fontHeight( smallfont)/2, smallfont );                 
                 //Serial.printf( "----- Oneword text width %d at x %d y %d\n", img.textWidth(sname, smallfont),spritex+xpos, img.height()/2 - img.fontHeight( smallfont)/2 );

         }else{
                 f1 = bigfont;
                 w = img.textWidth( word[0], f1 );
                 if ( w > tft.width() + 2 ) {
                   f1 = smallfont;
                   w = img.textWidth( word[0], f1 );                    
                 }
                 xpos = halve - w/2;
                 img.drawString(word[0], spritex+xpos, 3, f1 );
                 //Serial.printf( "----- Word 0 text width %d at x %d y %d\n", img.textWidth(sname, f1),spritex+xpos, 3 );
 
                 f = bigfont; 
                 w = img.textWidth( word[1], f );
                 if ( w > (img.width() + 2) ) {
                   f = smallfont;
                   w = img.textWidth( word[1], f );                    
                 }
                 xpos = halve - w/2;
                 img.drawString(word[1], spritex+xpos, img.fontHeight(f1) + 1, f );
                 //Serial.printf( "----- Word rest text width %d at x %d y %d\n", img.textWidth(sname, f),spritex+xpos, img.fontHeight( smallfont) + 2 );
 
         }
  }
}

//---------------------------------------------------------------------
void tft_showstation( int stationIdx){

img.createSprite(tft.width(), STATIONSCROLLH);
img.setTextColor( TFT_WHITE, TFT_BLACK ); 
if ( tft.width() > 200 ){
  img.setTextSize( 2 );
}else{
  img.setTextSize( 1 );
}
img.fillSprite(TFT_BLACK);

tft_showstations( stationIdx, 0);

grabTft();
img.pushSprite( 0, tft.height() - STATIONSCROLLH);

//Serial.printf( "----- img push at x %d y %d\n", 0, tft.height() - STATIONSCROLLH );

releaseTft();

img.deleteSprite();
}

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
  chosenStation = false;
  begint        = currentStation;
  
  //pinMode( SCROLLPIN,INPUT_PULLUP );
  //attachInterrupt( SCROLLPIN, choose_scrollstation, CHANGE);
 
  direction = *((int *)param);
  begint  = currentStation;
  endt    = stationCount;
  if( endt > 40 ) endt = 40; 
  
  img.createSprite(tft.width(), STATIONSCROLLH);
  img.setTextColor( TFT_ORANGE, TFT_BLACK ); 
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
        img.pushSprite( 0, tft.height()-STATIONSCROLLH );
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

void drawBmp(const char *filename, int16_t x, int16_t y) {

  if ((x >= tft.width()) || (y >= tft.height())) return;

  fs::File bmpFS;

  // Open requested file on SD card
  bmpFS = SPIFFS.open(filename, "r");

  if (!bmpFS)
  {
    Serial.print("File not found");
    return;
  }

  uint32_t seekOffset;
  uint16_t w, h, row, col;
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
        tft.pushImage(x, y--, w, 1, (uint16_t*)lineBuffer);
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

  //drawBmp("/OranjeRadio24.bmp", 55, 15 );

  int halfwidth = tft.width() / 2;
  int bmpx      = halfwidth - 25;
  int bmpy      = tft.height() * 10 / 50;
  
  drawBmp("/OranjeRadio24.bmp", bmpx, bmpy );


  if ( tft.width() > 200 ){
     STATIONSCROLLH = tft.fontHeight( bigfont) * 4 + 4;
  }else{
     STATIONSCROLLH = tft.fontHeight( bigfont) * 2 + 4;
  }
  
  delay(100);
 
  tft_backlight(1);

  setVolume( get_last_volstat(1) );
  delay(10);
 // showBattery();
 // delay(10);


  Serial.println("tft initialized");
  
}
