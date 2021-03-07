#ifdef USEOWM

#include "owm.h"

#undef TWOLINEDESC

// graphic constants

  int weathert = TFTCLOCKB;
  int weatherh = tft.height() - weathert;  
  int weatherw = tft.width();  
  
  int iconh    = 51;
  int iconw    = 51;
  int icont    = 1 + 32;
  
  int labelh   = 15;     
  int labelw   = 66; 
  int labelo   = (weatherw - 3*labelw)/4;
  
  int label1t    = 1 + 32;
  int label2t    = label1t + 62;
  int value1t    = label1t + labelh;   
  int value2t    = label2t + labelh;   

  struct {
    float temperature;
    float humidity;
    float pressure;
    float windspeed;
    float feelslike;
    char  *description  = NULL;
    char  *iconfilename = NULL;
    char  *city         = NULL;
    bool  valid         = false;
    char  iconchar;  
  }owmdata;

  char  *jsonowm = NULL;

  
  
struct GrAllocator {
  void* allocate(size_t size) {
    return gr_malloc( size );
  }

  void deallocate(void* pointer) {
    free( pointer );
  }

  void* reallocate(void* ptr, size_t new_size) {
    return gr_realloc(ptr, new_size);
  }
}; 
using WhateverRamJsonDocument = BasicJsonDocument<GrAllocator>;


//------------------------------------------------------------------------------

void print_owmdata(){
  Serial.printf( "---owmdata---\ntemp %f\nhum %f\npress %f\nwind %f\nfeel %f\ndesc %s\nicon %s\ncity %s\n---end owmdata---\n",
  owmdata.temperature,
  owmdata.humidity,
  owmdata.pressure,
  owmdata.windspeed,
  owmdata.feelslike,
  owmdata.description,
  owmdata.iconfilename,
  owmdata.city);  
}
//------------------------------------------------------------------------------


char *json_owmdata(){
  
  if ( !jsonowm ) jsonowm = (char *)gr_calloc( 2048,1);
  
  sprintf( jsonowm,"\t\"openweathermap\" : {\n\t\t\"temperature\" : %2.2f,\n \t\t\"feelslike\" : %2.2f,\n\
  \t\t\"humidity\" : %2.0f,\n\t\t\"pressure\" : %4.0f,\n\t\t\"wind\" : %3.0f,\n\t\t\
  \"description\" :  \"%s\",\n\t\t\"city\" : \"%s\"\n\t}",
  owmdata.temperature,
  owmdata.feelslike,
  owmdata.humidity,
  owmdata.pressure,
  owmdata.windspeed,
  owmdata.description,
  owmdata.city);  

  return( jsonowm );
}

//------------------------------------------------------------------------------

bool getWeather(){
  WiFiClient  owmclient;
  const char *owmserver = (const char *)"api.openweathermap.org";              // remote server we will connect to

  
  char *request = (char *)gr_calloc( 2048,1 );
  char *result, *eresult; 

  if ( owmclient.connect( owmserver, 80) ){   
    
      sprintf( request, "GET /data/2.5/weather?id=%s&units=%s&lang=%s&APPID=%s\r\nHost: api.openweathermap.org\r\nUser-Agent: %s/%s\r\nConnection: close\r\n",
               owm_id, owm_unit, owm_lang, owm_key, APNAME, APVERSION); 

      owmclient.println( request );
           
  }else{
         log_e("Connection to openweathermap failed");        //error message if no client connect
         free( request );
         return( false );
  }

  free( request);

  while( owmclient.connected() && ! owmclient.available()){
    delay( 50 );
    if ( ! owmclient.connected() ){
      log_e("Connection to openweathermap disconnected");        //error message if no client connect
      return( false );
    }
  }
    
  result = (char *)gr_calloc( 4096,1 );
  eresult = result;
  
  int bytesread = 0;  
  while ( owmclient.available() ){
         bytesread = owmclient.read( (uint8_t *)eresult, 256 );
         eresult  += bytesread; 
  }  
  owmclient.stop();                                      //stop client

/*
  for( eresult = result; *eresult; ++eresult ){
    if ( *eresult == '[' || *eresult == ']') *eresult = ' ';
  }
 */ 
  Serial.println( result );

  DeserializationError error;
  
//    SpiRamJsonDocument root(10*1024);
//    error = deserializeJson( root, result );
//    works, but should be conditional  

  WhateverRamJsonDocument root(10*1024);
//  StaticJsonDocument<2048> root;
  error = deserializeJson( root, result );

  if ( error  ){
        log_e("openweathermap deserialize failed: %s", error.c_str());
        free( result );     
        return( false );
  }

  if ( owmdata.description ) free ( owmdata.description );
  if ( owmdata.iconfilename ) free ( owmdata.iconfilename );
  if ( owmdata.city ) free ( owmdata.city );

  owmdata.temperature = root["main"]["temp"];
  owmdata.humidity    = root["main"]["humidity"];
  owmdata.pressure    = root["main"]["pressure"];
  owmdata.windspeed   = root["wind"]["speed"];
  owmdata.feelslike   = root["main"]["feels_like"];

  owmdata.city         = ps_strdup( root["name"] );
  char  iconname[32];

  strcpy( iconname, "9999");
    
  if ( root["weather"]["description"]){  
    owmdata.description  = ps_strdup( root["weather"]["description"] );
    strcpy( iconname, root["weather"]["icon"] );
  }else if ( root["weather"][0]["description"] ){
    owmdata.description  = ps_strdup( root["weather"][0]["description"] );    
    strcpy( iconname, root["weather"][0]["icon"] );
  }
      
 
  switch( atoi( iconname ) ){
       case 1:
          owmdata.iconfilename = ps_strdup( "/weather_icon/sunny.bmp");
          owmdata.iconchar = 'A';
          break;
       case 2:
          owmdata.iconfilename = ps_strdup( "/weather_icon/partlysunny.bmp");
          owmdata.iconchar = 'E';
          break;
       case 3:
          owmdata.iconfilename = ps_strdup( "/weather_icon/partlycloudy.bmp");
          owmdata.iconchar = 'F';
          break;
       case 4:
          owmdata.iconfilename = ps_strdup( "/weather_icon/cloudy.bmp");
          owmdata.iconchar = 'H';
          break;
       case 9:
       case 10:
          owmdata.iconfilename = ps_strdup( "/weather_icon/rain.bmp");
          owmdata.iconchar = 'D';
          break;
       case 11:
          owmdata.iconfilename = ps_strdup( "/weather_icon/tstorms.bmp");
          owmdata.iconchar = 'B';          
          break;
       case 13:
          owmdata.iconfilename = ps_strdup( "/weather_icon/snow.bmp");
          owmdata.iconchar = '@';
          break;
       case 50:
          owmdata.iconfilename = ps_strdup( "/weather_icon/fog.bmp");
          owmdata.iconchar = 'H';
          break;
       default:
          owmdata.iconfilename = ps_strdup( "/weather_icon/unknown.bmp");
          owmdata.iconchar = 'D';
          break;
          
  }
   
free( result );   
fillWeatherSprite();
drawWeather();

return( true );
}


//---------------------------------------------------------------------------------
void drawWeather(){

int extray = 0;

#ifdef USETOUCH
    if ( currDisplayScreen != RADIO && currDisplayScreen != STNSELECT ){
#else
    if (1){
       if ( currDisplayScreen == RADIO ){ 
          extray = tft.height() - weathert - label2t; 
       }
#endif
       if ( xSemaphoreTake( tftSemaphore, 1000 ) == pdTRUE ){
          if ( owmdata.iconfilename != NULL ){
            weather_sprite.pushSprite( 0, weathert + extray );
            releaseTft();  
#ifdef TWOLINEDESC            
            drawBmp( owmdata.iconfilename,labelo, weathert + extray );           
#else
            drawBmp( owmdata.iconfilename,labelo, weathert + label1t + labelh/2 + extray );         
#endif
          }else{
            weather_sprite.pushSprite( 0, weathert + labelh + extray );
            releaseTft();
          }  
          // drawing 24bit colorbmp's to sprites gives weird colors
          // also, drawmp does it's own grabtft/releasetft, this hangs when nested
          // drawBmp( owmdata.iconfilename,labelo, label1t - 1, &weather_sprite);               
       }else{
            log_w("Couldn't take tft semaphore in time to draw weather");
       }
    }
}
//--------------------------------------------------------------------------------
void fillWeatherSprite(){
  
 // two rows:
 // <nothing> <temp> <feel> 
 //  icon      tempval feelval
 // <wind>    <pres> <hum>
 // windval   presval humval
 
 enum vfield{vtemp,vfeel,vhum,vpres,vwind};
 int  endv[ vwind +1 ]; 
 
 if ( ! weather_sprite.created() ){
    weather_sprite.createSprite( weatherw, weatherh );  
 }
    
  weather_sprite.fillRect(0,0, weatherw, weatherh, TFT_BLACK);

//values  
// drawing 24bit colorbmp's to sprites gives weird colors
//  drawBmp( owmdata.iconfilename,labelo, label1t - 1, &weather_sprite);
//  drawBmp( owmdata.iconfilename,labelo, weathert + label1t - 1 );

#ifdef USEOWM
  weather_sprite.setTextDatum(L_BASELINE);
  weather_sprite.setTextFont( 4 );
  weather_sprite.setTextSize(2);  
  weather_sprite.setTextColor( TFT_MY_GOLD, TFT_BLACK );

  int   txt1t = value2t  - labelh - 7 ;//- ((label2t-label1t) - weather_sprite.fontHeight())/2; 
  int   txt2t = weatherh - 22 ;//- (( weatherh - label2t) - weather_sprite.fontHeight())/2; 
  char  scratch[32];

// if no data has been collected yet, display an empty black rectangle
  if ( owmdata.city == NULL ) {
      
    weather_sprite.setTextSize(1);  
    sprintf( scratch, "No weather data");
    weather_sprite.drawString(scratch, 5, txt1t );
  
    return;
  }
  

  
  sprintf( scratch, "%2.0f", owmdata.temperature );
  //sprintf( scratch, "%2.0f", -52.0, 248);
  weather_sprite.drawString(scratch, labelw + 2*labelo, txt1t );
  endv[vtemp] = weather_sprite.textWidth( scratch, 4 )+ labelw + 2*labelo;
      
  sprintf( scratch, "%2.0f", owmdata.feelslike);
  weather_sprite.drawString(scratch, 2*labelw + 3*labelo, txt1t );
  endv[vfeel] = weather_sprite.textWidth( scratch, 4 ) + 2*labelw + 3*labelo;
  
  
  weather_sprite.setTextSize(1);  
  txt1t = value1t + weather_sprite.fontHeight() ;
  txt2t = value2t + weather_sprite.fontHeight() ;
  
  sprintf( scratch, "%2.0f", owmdata.humidity);
  weather_sprite.drawString(scratch, labelo, txt2t );
  endv[vhum] = weather_sprite.textWidth( scratch, 4 ) + labelo+ 1;
  
  sprintf( scratch, "%4.0f", owmdata.pressure);
  weather_sprite.drawString(scratch, labelw + 2*labelo, txt2t );
  endv[vpres] = weather_sprite.textWidth( scratch, 4 ) + labelw + 2*labelo + 1;
  
  sprintf( scratch, "%2.0f", owmdata.windspeed);
  weather_sprite.drawString(scratch, 2*labelw + 3*labelo + 5, txt2t );
  endv[vwind] = weather_sprite.textWidth( scratch, 4 ) + 2*labelw + 3*labelo + 5+ 1;
  
// labels 

  weather_sprite.fillRoundRect( labelw + 2*labelo,   label1t, labelw, labelh, 4, TFT_MY_GOLD); // for TEMP
  weather_sprite.fillRoundRect( 2*labelw + 3*labelo, label1t, labelw, labelh, 4, TFT_MY_GOLD); // for FEEL
  weather_sprite.fillRoundRect( labelo,              label2t, labelw, labelh, 4, TFT_MY_GOLD); // for WIND
  weather_sprite.fillRoundRect( labelo*2 + labelw,   label2t, labelw, labelh, 4, TFT_MY_GOLD); // for PRES
  weather_sprite.fillRoundRect( labelo*3 + 2*labelw, label2t, labelw, labelh, 4, TFT_MY_GOLD); // for HUM

// label text

  weather_sprite.setFreeFont( LABEL_FONT);                 // FreeSansBold6pt8b
  weather_sprite.setTextDatum(L_BASELINE);
  
  weather_sprite.setTextColor( TFT_MY_GOLD, TFT_BLACK );
  
  //int desoffset = weather_sprite.textWidth( owmdata.description, 1 );
  //desoffset = (tft.width() - 2*labelo - desoffset)/2;
  //weather_sprite.drawString(  owmdata.description, labelo, label1t-4 );
  
#ifdef TWOLINEDESC  
   char *torusdesc = ps_strdup( owmdata.description );
   utf8torus( owmdata.description, torusdesc );     
   int dw = weather_sprite.textWidth( torusdesc ,1);
  
   if( dw > labelw ){
    char *line[2];
    char *t = ps_strdup( torusdesc );
    char *s;
    
    line[0] = t;   
    line[1] = t;
    
    for ( s = t+5; *s && *s != ' '; ++s );
    
    if (*s){
      *s = 0;++s;
      line[1] = s;
    }
    
    weather_sprite.drawString( line[0], labelo, 58 );
    if(*s)weather_sprite.drawString( line[1], labelo, 72 );
    
    free(t);
    
  }else{
    log_d("description shorter than labelw ");
    weather_sprite.drawString(  torusdesc, labelo, 50 + 12 );
  }
  
  free( torusdesc );
#else
    weather_sprite.setTextColor( TFT_REALGOLD, TFT_BLACK ); 
    weather_sprite.setFreeFont( LABEL_FONT ); 

    char tmprus[128];
    char tmpline[128];
    
    sprintf( tmpline, "%s",utf8torus( owmdata.description, tmprus ) );

    if( strlen( tmpline ) > 15 ){
        date_sprite.setFreeFont( LABEL_FONT );         
    }else{
        date_sprite.setFreeFont( DATE_FONT ); 
    }
    
    int dw = weather_sprite.textWidth( tmpline, 1 );
    int dx = (weatherw - dw )/2;
    weather_sprite.drawString( tmpline, dx, 20 );
#endif

  weather_sprite.setTextColor( TFT_BLACK, TFT_MY_GOLD );
  
  int txtxo4 = (labelw - weather_sprite.textWidth( "TEMP",1) )/2;
  int txtxo3 = (labelw - weather_sprite.textWidth( "HUM", 1) )/2;
  
  weather_sprite.drawString("TEMP", labelw + 2*labelo + txtxo4,     label1t + 11 );
  weather_sprite.setTextColor( TFT_MY_GOLD, TFT_BLACK );
  weather_sprite.drawString("o",  endv[ vtemp],     label1t + 28 );
  weather_sprite.setTextColor( TFT_BLACK, TFT_MY_GOLD );
  
  weather_sprite.drawString("FEEL", 2*labelw + 3*labelo + txtxo4,   label1t + 11 );
  weather_sprite.setTextColor( TFT_MY_GOLD, TFT_BLACK );
  weather_sprite.drawString("o", endv[vfeel],     label1t + 28 );
  weather_sprite.setTextColor( TFT_BLACK, TFT_MY_GOLD );
  
  weather_sprite.drawString("HUM",  labelo + txtxo3,                label2t + 11 );
  weather_sprite.setTextColor( TFT_MY_GOLD, TFT_BLACK );
  weather_sprite.drawString("%",  endv[vhum],     label2t + 40 );
  weather_sprite.setTextColor( TFT_BLACK, TFT_MY_GOLD );
  
  weather_sprite.drawString("PRES", labelo*2 + labelw + txtxo4,     label2t + 11 );
  weather_sprite.setTextColor( TFT_MY_GOLD, TFT_BLACK );
  weather_sprite.drawString("hPa", endv[ vpres],        label2t + 40 );
  weather_sprite.setTextColor( TFT_BLACK, TFT_MY_GOLD );
  
  weather_sprite.drawString("WIND",  labelo*3 + 2*labelw + txtxo4,   label2t + 11 );
  weather_sprite.setTextColor( TFT_MY_GOLD, TFT_BLACK );
  weather_sprite.drawString("m/s",   endv[ vwind], label2t + 40 );
  weather_sprite.setTextColor( TFT_BLACK, TFT_MY_GOLD );

#endif
  
}

#else
String json_owmdata(){return("\t\"openweathermap\" : {}");}

void drawWeather(){

  if ( ! weather_sprite.created() ){
    weather_sprite.createSprite( tft.width(), TFTSTATIONB - TFTCLOCKB );  
  }
    
  weather_sprite.fillRect(0,0, tft.width(), TFTSTATIONB - TFTCLOCKB, TFT_BLACK);
  
  grabTft();
  weather_sprite.pushSprite( 0, TFTCLOCKB );
  releaseTft(); 
}  
#endif
