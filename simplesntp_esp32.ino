
#undef SNTP_MAIN
 
//#include <wificredentials.h>

// in wificredentials the following are define:
// char* ntpServers[] = { "nl.pool.ntp.org", "be.pool.ntp.org", "de.pool.ntp.org"};
// const char* ntpTimezone   = "CET-1CEST,M3.5.0/2,M10.5.0/3";
// const char* wifiSsid[]      =  {"yourssid", "yourotherssid","yourmothersssi"};
// const char* wifiPassword[]  =  ("yourWiFipassword","passwordOfyourOtherssid","yourmotherssidspassword"} ;

#include "WiFi.h"
#include "time.h"
#include "lwip/err.h"
#include "lwip/apps/sntp.h"

/*----------------------------------------------------------------*/

int time_to_jurassic(void )
{
  int            result;
 
  time_t         new_time;
  struct tm      trex;
  struct timeval ptero;

  trex.tm_sec =0;
  trex.tm_min =0;
  trex.tm_hour=0;
  trex.tm_mday=30;
  trex.tm_mon =8;          
  trex.tm_year=1993-1900; // set time to release date of Jurassic Park

  new_time=mktime(&trex);

  ptero.tv_sec =new_time;
  ptero.tv_usec=0;

  result=settimeofday(&ptero,NULL);
  return result;

}

//----------------------------------------------------------------
// workaround for Arduino and maybe IDF only using 1 ntpserver 

void restart_sntp(){
  static int first_server = 0;
  int              server = first_server;

  log_d("sntp server[0] = %s", ntpServers[ server] );
  
  sntp_stop(); 
  sntp_setoperatingmode(SNTP_OPMODE_POLL); 

  for( int i = 0; i < 3; ++i ){
    sntp_setservername( i , (char *)ntpServers[ server]); 
    ++server;
    if ( server > 2 ) server = 0;
  }
  first_server++;
  if ( first_server > 2 ) first_server = 0;
  
  sntp_init(); 
}  

/*--------------------------------------------------------------*/

void ntp_waitforsync(){
time_t  rawt;
struct tm *tinfo;
struct tm  linfo, ginfo;
int y, count;

   // workaround for Arduino and maybe IDF only using 1 ntpserver 
   for(count = 0;; ++count){
     if ( count > 2 ){
        restart_sntp();
        delay(10); 
        count = 0;
     }
     time( &rawt );
     tinfo = localtime( &rawt );
     
     Serial.printf( "Waiting for ntp sync, time is now " );
     Serial.print(ctime(&rawt));

     y = tinfo->tm_year + 1900;
     if ( y > 2000 ) break;
     
     delay(500); 
   }


gmtime_r( &rawt, &ginfo);
localtime_r( &rawt, &linfo);
 
//Serial.printf(" localtime hour       %d\n", linfo.tm_hour);
//Serial.printf(" gmtime hour          %d\n", ginfo.tm_hour);

time_t g = mktime(&ginfo);
time_t l = mktime(&linfo);

double offsetSeconds = difftime( l, g );
int    offsetHours   = (int)offsetSeconds /(60*60);

//Assuming DST is always + 1 hour, which apparently is not always true.

Serial.printf("Timezone offset is %d hour%s, %s, current offset is %d hour%s\n\n", 
          offsetHours, 
          (offsetHours>1 || offsetHours < -1)?"s":"",
          linfo.tm_isdst?"DST in effect": "DST not in effect", 
          linfo.tm_isdst?offsetHours+1:offsetHours,
          ( (offsetHours+1)>1 || (offsetHours+1) < -1)?"s":""); 
    
}
/*--------------------------------------------------------------*/

void ntp_setup(bool waitforsync){ 
  // sometimes the esp boots with a time in the future. This defeats the
  // test. The settimeofday_cb function is not available on the ESP32 AFAIK
  
  if ( waitforsync && time_to_jurassic() ) Serial.println( "Error setting time to long long ago");
  
  // set timezone
  setenv( "TZ" , ntpTimezone, 1);
  tzset();

  // configTime on the ESP32 does not honor the TZ env, unlike the ESP8266
  
  restart_sntp();
  
  // A new NTP request will be done every hour (hopefully, to be verified 
  // with tcpdump one day.
  
  if( waitforsync )ntp_waitforsync();
 
}
/*----------------------------------------------------------------*/
void tellTime(){
  time_t now;
  now = time(nullptr);

  Serial.printf(" localtime : %s", asctime( localtime(&now) ) );
  Serial.printf("    gmtime : %s\n", asctime( gmtime(&now) ) );
}
/*----------------------------------------------------------------*/
 
#ifdef SNTP_MAIN 
void setup() {
 Serial.begin(115200);
 
  Serial.printf("Connecting to %s ", ssid);
  
  WiFi.setHostname("simplesntp_esp32");
  WiFi.begin(wifiSsid, wifiPassword);


   for (int wifipoll = 0; WiFi.status() != WL_CONNECTED; ++wifipoll) {
      delay(500);
      Serial.print(".");
      if ( wifipoll%10 == 0 ){
        WiFi.begin(wifiSsid, wifiPassword);
      }
   }
  
  Serial.println(" CONNECTED ");
  Serial.print("IP address = ");
  Serial.println(WiFi.localIP());
 
  ntp_setup( true );
}

void loop() {
  delay(50000);

  tellTime();
  
  // put your main code here, to run repeatedly:

}
#endif
