#ifdef USEOWM

#include "owm.h"

// graphic constants
  int weathert = TFTCLOCKB;
  int weatherh = tft.height() - weathert;  
  int weatherw = tft.width();  
  
  int iconh    = 51;
  int iconw    = 51;
  int icont    = 1;
  
  int labelh   = 15;     
  int labelw   = 66; 
  int labelo   = (weatherw - 3*labelw)/4;
  
  int label1t    = 15;
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
    char  *city;  
  }owmdata;

  TFT_eSprite weather_sprite = TFT_eSprite(&tft);    
  
  
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

  for( eresult = result; *eresult; ++eresult ){
    if ( *eresult == '[' || *eresult == ']') *eresult = ' ';
  }
  
  Serial.println( result );
  
  StaticJsonDocument<1024> root;
  
  
  if ( deserializeJson( root, result ) ){
        log_e("openweathermap deserialize failed");
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

  owmdata.description  = ps_strdup( root["weather"]["description"] );
  owmdata.city         = ps_strdup( root["name"] );
  
  switch( atoi( root["weather"]["icon"] ) ){
       case 1:
          owmdata.iconfilename = ps_strdup( "/weather_icon/sunny.bmp");
          break;
       case 2:
          owmdata.iconfilename = ps_strdup( "/weather_icon/partlysunny.bmp");
          break;
       case 3:
          owmdata.iconfilename = ps_strdup( "/weather_icon/partlycloudy.bmp");
          break;
       case 4:
          owmdata.iconfilename = ps_strdup( "/weather_icon/cloudy.bmp");
          break;
       case 9:
       case 10:
          owmdata.iconfilename = ps_strdup( "/weather_icon/rain.bmp");
          break;
       case 11:
          owmdata.iconfilename = ps_strdup( "/weather_icon/tstorms.bmp");
          break;
       case 13:
          owmdata.iconfilename = ps_strdup( "/weather_icon/snow.bmp");
          break;
       case 50:
          owmdata.iconfilename = ps_strdup( "/weather_icon/fog.bmp");
          break;
       default:
          owmdata.iconfilename = ps_strdup( "/weather_icon/unknown.bmp");
          break;
          
  }
   
free( result );   
fillWeatherSprite();
drawWeather();

return( true );
}
//--------------------------------------------------------------------------------

void drawWeather(){
    if ( currDisplayScreen != RADIO && currDisplayScreen != STNSELECT ){
       if ( xSemaphoreTake( tftSemaphore, 1000 ) == pdTRUE ){
        weather_sprite.pushSprite( 0, weathert );
        releaseTft();  
          // drawing 24bit colorbmp's to sprites gives weird colors
          // also, drawmp does it's own grabtft/releasetft, this hangs when nested
          //  drawBmp( owmdata.iconfilename,labelo, label1t - 1, &weather_sprite);

        if ( owmdata.iconfilename != NULL ){
          drawBmp( owmdata.iconfilename,labelo, weathert + label1t - 1 );
        }        
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
  
  weather_sprite.setTextDatum(L_BASELINE);
  weather_sprite.setTextFont( 4 );
  weather_sprite.setTextSize(2);  
  weather_sprite.setTextColor( TFT_MY_GOLD, TFT_BLACK );

  int   txt1t = value2t  - labelh - 7 ;//- ((label2t-label1t) - weather_sprite.fontHeight())/2; 
  int   txt2t = weatherh - 22 ;//- (( weatherh - label2t) - weather_sprite.fontHeight())/2; 
  char  scratch[32];

  sprintf( scratch, "%2.0f%c", owmdata.temperature, 248);
  //sprintf( scratch, "%2.0f%c", -52.0, 248);
  weather_sprite.drawString(scratch, labelw + 2*labelo, txt1t );
  endv[vtemp] = weather_sprite.textWidth( scratch, 4 )+ labelw + 2*labelo - 10;
      
  sprintf( scratch, "%2.0f%c", owmdata.feelslike, 248);
  weather_sprite.drawString(scratch, 2*labelw + 3*labelo, txt1t );
  endv[vfeel] = weather_sprite.textWidth( scratch, 4 ) + 2*labelw + 3*labelo - 10;
  
  
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
  weather_sprite.drawString(  owmdata.description, labelo, label1t-4 );

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
  
  tft.setFreeFont();
}


#endif
