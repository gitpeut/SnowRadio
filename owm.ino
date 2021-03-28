#ifdef USEOWM

#include "owm.h"

#undef TWOLINEDESC

// graphic constants
  int weathert; 
  int weatherh;
  int weatherw;
  
  int iconh;
  int iconw;
  int icont;
  
  int labelh;
  int labelw;
  int labelo;
  
  int label1t;
  int label2t;
  int value1t;
  int value2t;

  int fcw;
  int fch;
  int fclabelh;
  int fclabelo;
  int fclabelw;
  int fcx=0;
  int fcy=0;

//----------------------------------------------------------------------------  
  struct Owmdata owmdata;

  struct {
      time_t stamp[3];      // UTC timestamp requested
      time_t owmstamp[3];   // UTC timestamp found in owm forecast
      String d[3];          // tomorrow's date as dd.mm in owmforecast.d[0], next day in owmforecast.d[1]... 
      String t[3];          // temp at 12:00 tomorrow in owmforecast.t[0]
      bool valid = false;
  }owmforecast;
  
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

void init_owm(){
  weathert = TFTCLOCKB;
  weatherh = tft.height() - weathert - radio_button_font.yAdvance - 4;  
//  weatherw = tft.width();  
  weatherw = 230;  
  
  iconh    = 51;
  iconw    = 51;
  icont    = 1 + 32;
  
  labelh   = 15;     
  labelw   = 66; 
  labelo   = (weatherw - 3*labelw)/4;
  
  label1t    = 1 + 32;
  label2t    = label1t + 62;
  value1t    = label1t + labelh;   
  value2t    = label2t + labelh;   

  fcw        = 2*labelo + 2*labelw;
  fch        = labelh;
  fclabelh   = fch;
  fclabelo   = labelo/2;
  fclabelw   = (fcw - fclabelo*3)/3;
//  fcx        = tft.width()  - fcw;
  fcx        = 230  - fcw;
  fcy        = tft.height() - radio_button_font.yAdvance - 5;

  log_d( "fcw %d fcx %d ", fcw, fcx); 
}

//------------------------------------------------------------------------------

void print_owmdata(){
  
  if ( ! owmdata.valid ){
    log_e("no valid weather data");
    return;
  }
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

  if ( !owmdata.valid ) return( NULL ); 
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

  owmdata.valid = false;
  
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


  Serial.println( result );

  DeserializationError error;
  
//    SpiRamJsonDocument root(10*1024);
//    error = deserializeJson( root, result );
//    works, but should be conditional  

  WhateverRamJsonDocument root(10*1024);

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

owmdata.valid = true;
  
free( result );   
fillWeatherSprite();
drawWeather();

return( true );
}


//---------------------------------------------------------------------------------
void drawWeather(){

       if ( xSemaphoreTake( tftSemaphore, 1000 ) == pdTRUE ){
          if ( owmdata.iconfilename != NULL ){
            weather_sprite.pushSprite( 16, 130 );
            releaseTft();  
            drawBmp( owmdata.iconfilename, 24, 146 ); 
            delay (1);        
          }else{
            weather_sprite.pushSprite( 16, 130 );
            releaseTft();
          }  
          // drawing 24bit colorbmp's to sprites gives weird colors
          // also, drawmp does it's own grabtft/releasetft, this hangs when nested
          // drawBmp( owmdata.iconfilename,labelo, label1t - 1, &weather_sprite);               
       }else{
            log_w("Couldn't take tft semaphore in time to draw weather");
       }
    }

//--------------------------------------------------------------------------------
void fillWeatherSprite(){
  
 // two rows:
 // <nothing> <temp> <feel> 
 //  icon      tempval feelval
 // <wind>    <pres> <hum>
 // windval   presval humval

  char  scratch[32];
 
 if ( ! weather_sprite.created() ){  
    weather_sprite.createSprite( 210 , 114 );  
    weather_sprite.fillSprite(TFT_MY_DARKGRAY); 

 }
    
//-------------- NO WEATHER DATA

// if no data has been collected yet, display an empty black rectangle
  if ( owmdata.city == NULL ) {
    weather_sprite.setFreeFont( STATION_FONT );      
    weather_sprite.setTextColor( TFT_MY_GRAY, TFT_MY_DARKGRAY ); 
    weather_sprite.setTextDatum( TC_DATUM );
    sprintf( scratch, "NO WEATHER DATA");
    weather_sprite.setTextDatum( TL_DATUM ); 
    return;
  }else{
    //weather data available, erase sprite
    weather_sprite.fillSprite(TFT_MY_DARKGRAY); 
  }

//--------------

  weather_sprite.fillRect(  73, 17, 66, 15, TFT_MY_BLACK); // for TEMP
  weather_sprite.fillRect( 145, 17, 66, 15, TFT_MY_BLACK); // for FEEL
  weather_sprite.fillRect(   0, 67, 66, 15, TFT_MY_BLACK); // for WIND
  weather_sprite.fillRect(  73, 67, 66, 15, TFT_MY_BLACK); // for HUM
  weather_sprite.fillRect( 145, 67, 66, 15, TFT_MY_BLACK); // for PRES

//--------------

  weather_sprite.setFreeFont( LABEL_FONT );
  weather_sprite.setTextColor( TFT_MY_GRAY, TFT_MY_BLACK );
  weather_sprite.setTextDatum( TC_DATUM );

  weather_sprite.drawString("TEMP", 105, 18 );
  weather_sprite.drawString("FEEL", 177, 18 );
  weather_sprite.drawString("WIND",  32, 68 );
  weather_sprite.drawString("HUM",  105, 68 );
  weather_sprite.drawString("PRES", 177, 68 );  
  weather_sprite.setTextDatum( TL_DATUM ); 
  
//--------------

    char tmprus[128];
    char tmpline[128];
    
  sprintf( tmpline, "%s",utf8torus( owmdata.description, tmprus ) );
  weather_sprite.setFreeFont( LABEL_FONT );
  weather_sprite.setTextColor( TFT_MY_GRAY, TFT_MY_DARKGRAY );
  weather_sprite.setTextDatum( TC_DATUM );
  weather_sprite.drawString( tmpline, 105, 1 );
  weather_sprite.setTextDatum( TL_DATUM ); 
    
//--------------

  String t_full = String ( owmdata.temperature, 0 ) + String ("*");
  String t_feel = String ( owmdata.feelslike , 0 ) + String ("*");
  String sp     = String ( owmdata.windspeed, 0 ) + String ( "&" );
  if (owmdata.humidity > 99) owmdata.humidity = 99;
  String hm     = String ( owmdata.humidity, 0 ) + String ( "%" );

  weather_sprite.setFreeFont  ( NUM_FONT );
  weather_sprite.setTextColor ( TFT_MY_GRAY, TFT_MY_DARKGRAY ); 

  #ifdef PRESSURE_IN_HPA
    weather_sprite.setTextDatum( TC_DATUM ); // position using center of string
    String pres_hpa    = String( owmdata.pressure, 0); // no decimals in string
    int    pres_center = weather_sprite.textWidth( pres_hpa )/2; // center is half the string width
    int    pres_x      = weather_sprite.width() - pres_center;  // make sure we do not cut off value.
                                                             // otherwise we would have used center of label
    
    weather_sprite.drawString( pres_hpa, pres_x, 86 );
  #else
    int pres_mm   = round  (owmdata.pressure * 0.75) ;
    weather_sprite.setCursor( 150, 108 );  
    weather_sprite.print(pres_mm);   
  #endif
  
  weather_sprite.setTextDatum ( TC_DATUM );
  weather_sprite.drawString(t_full, 106, 37 );
  weather_sprite.drawString(t_feel, 178, 37 );
  weather_sprite.drawString(sp, 33, 86 );
  weather_sprite.drawString(hm, 106, 86 );

  weather_sprite.setTextDatum( TL_DATUM ); 
  
}


//--------------------------------------------------------------------
void drawForecastSprite(){  
  if ( !owmforecast.valid ){
    log_d("not drawing forecast, forecast not valid");  
    return;
  }
  if ( fcx <= 0 ){    
    log_d("not drawing forecast, fcx = %d", fcx);
    return;
  }

  log_d("draw forecast sprite at %d %d", fcx, fcy );
  grabTft();
  forecast_sprite.pushSprite( 16, 264 );
  releaseTft();
}
//--------------------------------------------------------------------

void fillForecastSprite(){

  if ( !owmforecast.valid ) return;
  if ( ! forecast_sprite.created() ){
    forecast_sprite.createSprite( 210, 46 ); 
    forecast_sprite.fillSprite(TFT_MY_DARKGRAY);  
  }
    
  forecast_sprite.fillRect( 0 ,0, 66, 16, TFT_MY_BLUE);
  forecast_sprite.fillRect( 73 ,0, 66, 16, TFT_MY_BLUE);
  forecast_sprite.fillRect( 145 ,0, 66, 16, TFT_MY_BLUE);

//  forecast_sprite.setFreeFont( LABEL_FONT );                 // FreeSansBold6pt8b
  forecast_sprite.setFreeFont( LABELW_FONT );                 // FreeSansBold6pt8b
  forecast_sprite.setTextColor( TFT_MY_DARKGRAY, TFT_MY_BLUE );
  forecast_sprite.setTextDatum( TC_DATUM );
  forecast_sprite.drawString( owmforecast.d[0], 32, 2 );
  forecast_sprite.drawString( owmforecast.d[1], 105, 2);
  forecast_sprite.drawString( owmforecast.d[2], 177, 2 );
  forecast_sprite.setTextDatum( TL_DATUM );

  forecast_sprite.setFreeFont( NUM_FONT );
  forecast_sprite.setTextDatum( TC_DATUM );
  forecast_sprite.setTextColor( TFT_MY_GRAY, TFT_MY_DARKGRAY );
  forecast_sprite.drawString( owmforecast.t[0] + "*",  33, 22 );
  forecast_sprite.drawString( owmforecast.t[1] + "*", 106, 22 );
  forecast_sprite.drawString( owmforecast.t[2] + "*", 178, 22 );
  forecast_sprite.setTextDatum( TL_DATUM );

}

//-----------------------------------------------------------------------

time_t local2UTC12( int plusdays ){  

  time_t         nowstamp, thenstamp;
  struct tm      nowtm;
  
    nowstamp  = time(nullptr);                // get the timestamp; the timestamp is a UTC timestamp
                                              // localtime wil add the tz offset, if applicable
    localtime_r( &nowstamp, &nowtm );
  
    nowtm.tm_mday += plusdays;  // mktime will correct this
                                // to a valid calendar date.
    nowtm.tm_sec =0;
    nowtm.tm_min =0;
    nowtm.tm_hour=13;           // chosen 13:00 to get 12:00 weather data both in Eindhoven and Moscow
  
    thenstamp = mktime(&nowtm);

    //Serial.printf("%ld localtime 12:00 in %d days: %s", thenstamp, plusdays, asctime_r( localtime_r(&thenstamp, &tmptm), tmpstring ) );
    //Serial.printf("%ld   utctime 12:00 local time in %d days: %s", thenstamp, plusdays, asctime_r(    gmtime_r(&thenstamp, &tmptm), tmpstring ) );
    
    return( thenstamp );
}

//---------------------------------------------------------------------------------------------------------------
void print_forecast(){
struct tm request_tm, found_tm;

  if ( !owmforecast.valid ) {
    Serial.println("No valid forecast data");
    return;  
  }
  
  for ( int i=0; i<3 ; ++i ){
      
      localtime_r(&owmforecast.stamp[i], &request_tm);            
         gmtime_r(&owmforecast.owmstamp[i], &found_tm);            
    
      Serial.printf( "Day %s : %d:00 UTC (requested: %d-%d %d:00 (local time))- %s *C\n", 
                                                    owmforecast.d[i].c_str(),
                                                    found_tm.tm_hour, 
                                                    request_tm.tm_mday,request_tm.tm_mon+1,request_tm.tm_hour,
                                                    owmforecast.t[i].c_str() );  
  }


}

//---------------------------------------------------------------------------------------------------------------

bool getForecast(){
  WiFiClient  owmclient;
  const char *owmserver = (const char *)"api.openweathermap.org";              // remote server we will connect to  
  char *result, *eresult; 
  char *request = (char *)gr_calloc( 1024,1 );
  char tmpdat[8]; 

  owmforecast.valid = false;

  if ( owmclient.connect( owmserver, 80) ){   
    
      sprintf( request, "GET /data/2.5/forecast?id=%s&units=%s&lang=%s&APPID=%s\r\nHost: api.openweathermap.org\r\nUser-Agent: %s/%s\r\nConnection: close\r\n",
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
//      log_e("Connection to openweathermap disconnected");        //error message if no client connect
      return( false );
    }
  }
    
  result = (char *)gr_calloc( 32*1024,1 );


  eresult = result;
  
  int bytesread = 0;  
  while ( owmclient.available() ){
         bytesread = owmclient.read( (uint8_t *)eresult, 1 );
         eresult  += bytesread; 
  }  
  owmclient.stop();                                      //stop client


//  if ( (read_forecast_fromfs( &result )) < 0 ){
//    Serial.printf( "Error reading forecast");
//  }
//  Serial.println( result );

  DeserializationError error;
  
//    SpiRamJsonDocument root(10*1024);
//    error = deserializeJson( root, result );
//    works, but should be conditional  

  WhateverRamJsonDocument root(32*1024);
  error = deserializeJson( root, result );

  if ( error  ){
        log_e("openweathermap deserialize failed: %s", error.c_str());
        free( result );     
        return( false );
  }

  struct  tm tmptm;
  double  tmptemp;
  int     owmoffset = 0;
  
  for ( int i=0; i<3 ; ++i ){ 
      
      owmforecast.stamp[i] = local2UTC12( (i+1) );// get timestamp of next i day 13:00
      gmtime_r(&owmforecast.stamp[i], &tmptm);    
              
              // OWM presents a rolling list of 40 timeslots for every 3 hours 
              // (0:00,3:00,6:00,9:00,12:00,15:00,18:00,21:00)in the future, 
              // this is 5 days total. To find data for a specific time ( here 13:00 
              // of tomorrow, tomorrow +1 and tomorrow +2 ) the list can best be
              // traversed finding the weather with the timestamp nearest to the time requested.

      for ( ; owmoffset < root["cnt"]; ++owmoffset ){  
        time_t foundstamp  = (time_t)root["list"][owmoffset]["dt"];
        if( abs( foundstamp - owmforecast.stamp[i] ) <= (2*3600) ) break;
      }
      
      if ( owmoffset == root["cnt"] ){
        free(result);
        log_e("openweathermap\n forecast could not find weather for UTC time\n %s", asctime(&tmptm) );
        return( false );  
      }
      
      //Serial.printf("owmoffset = %d\n", owmoffset );
      
      tmptemp = root["list"][ owmoffset ]["main"]["temp"];    
      owmforecast.t[i] = (int) tmptemp; 
      
      owmforecast.owmstamp[i] = (time_t) root["list"][ owmoffset ]["dt"];
      
      gmtime_r(&owmforecast.owmstamp[i], &tmptm);            
      sprintf( tmpdat, "%02d.%02d", tmptm.tm_mday, tmptm.tm_mon + 1);
      owmforecast.d[i]  = String( tmpdat);
   
  }
  owmforecast.valid = true;

 
  free( result );  

  fillForecastSprite();
  drawForecastSprite(); 

  return( true );
}

#else // do not USEOWM
String json_owmdata(){return("\t\"openweathermap\" : {}");}

void drawWeather(){

  if ( ! weather_sprite.created() ){
    weather_sprite.createSprite( 210 , 114 ); 
  }
    weather_sprite.fillSprite(TFT_MY_DARKGRAY); 
    
  grabTft();
  weather_sprite.pushSprite( 16, 130 );
  releaseTft(); 
}  
#endif
