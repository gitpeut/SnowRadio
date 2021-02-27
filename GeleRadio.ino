 
// Oranje radio
// Jose Baars, 2019
// public domain
// uses the follwing libraries:
// wifi manager ( https://github.com/tzapu/WiFiManager.git )
// Gesture_PAJ7620 ( https://github.com/Seeed-Studio/Gesture_PAJ7620 )
// ESP_VS1053 ( https://github.com/baldram/ESP_VS1053_Library )
// TFT_eSPI ( https://github.com/Bodmer/TFT_eSPI )
// If you want to use LittleFS
// LittleFS_ESP32 ( https://github.com/lorol/LITTLEFS )
// AsyncTCP (https://github.com/me-no-dev/AsyncTCP)
// ESPAsyncWebServer (https://github.com/me-no-dev/ESPAsyncWebServer.git )
// Thanks to all the authors 
//

/*define or undefine */
#define USEOWM // to enable Open weathermap owm_key, 
               // owm_id (city id), owm_lang (language),
               //owm_unit (metric, imperial) in wificredentials.h
               // should be defined as const char *, e.g.
               // const char* owm_unit = "metric";
#undef USESSDP
#undef USEOTA
#define USETLS 1
#undef USEPIXELS   
#define USEGESTURES 1
#undef MULTILEVELGESTURES
#define USETOUCH
#undef USETOUCH2

//#define MONTHNAMES_RU
// Cyrillic characters must be supported by the font chosen
#ifdef MONTHNAMES_RU
const char *monthnames[] = {"ЯНВАРЯ", "ФЕВРАЛЯ", "МАРТА", "АПРЕЛЯ", "МАЯ", "ИЮНЯ", "ИЮЛЯ", "АВГУСТА", "СЕНТЯБРЯ", "ОКТЯБРЯ", "НОЯБРЯ", "ДЕКАБРЯ"};
const char *daynames[]   = {"ПН", "ВТ", "СР", "ЧТ", "ПТ", "СБ", "ВС"};
#elif MONTHNAMES_EN
const char *monthnames[] = {"January","February","March","April","May","June","July","August","September","October","November","December"};
const char *daynames[] = {"Sun","Mon","Tue","Wed","Thu", "Fri","Sat"};
#else
const char *monthnames[] = {"januari","februari","maart","april","may","juni","juli","augustus","september","october","november","december"};
const char *daynames[] = {"zo","ma","di","wo","do", "vr","za"};
#endif

#include "soc/timer_group_struct.h"
#include "soc/timer_group_reg.h"
#include <freertos/queue.h>
#include <freertos/task.h>
#include <freertos/portmacro.h>
#include <freertos/semphr.h> 

#include <SPI.h>
#include "FS.h"
#include <SPIFFS.h>
#include <FFat.h>
#include <LITTLEFS.h>
#include <esp_littlefs.h>
#include <TFT_eSPI.h> // Graphics and font library for ST7735 driver chip
#include <WiFi.h>
#include <esp_wifi.h>
#include <WiFiMulti.h>
#include <AsyncTCP.h>
#include <WiFiClient.h>
#ifdef USETLS
#include <WiFiClientSecure.h>
#endif
#include <ESPAsyncWebServer.h>
#include <rom/rtc.h>

#ifdef USEOTA
  #include <ArduinoOTA.h>
#endif
//E][WiFiUdp.cpp:219] parsePacket(): could not receive data: 9
//WIFI_STATIC_RX_BUFFER_NUM=10
//CONFIG_ESP32_WIFI_DYNAMIC_RX_BUFFER_NUM 32
//CONFIG_ESP32_WIFI_RX_BA_WIN 6
//...Documents\ArduinoData\packages\esp32\hardware\esp32\1.0.3-rc1\tools\sdk\include\config\\sdkconfig.h
//https://github.com/espressif/esp-idf/issues/3646

#ifdef USESSDP
  #include <SSDPDevice.h>
#else
  #include <ESPmDNS.h>
#endif
#include <Update.h>
#include <Wire.h>

#include <wificredentials.h>
#include "VS1053g.h"
#include "paj7620.h"
#include "sk.h"
#include "0pins.h"
#include "tft.h"
#include "owm.h"

screenPage currDisplayScreen = RADIO;

enum FSnumber{
  FSNO_SPIFFS,
  FSNO_LITTLEFS,
  FSNO_FFAT 
};

//choose file system
//For now and historic reasons, SPIFFS is used.
//
//fs::FS      RadioFS     = SPIFFS;
//const int   RadioFSNO   = FSNO_SPIFFS;
//const char  *RadioMount = "/spiffs";

fs::FS      RadioFS     = LITTLEFS;
const int   RadioFSNO   = FSNO_LITTLEFS;
const char  *RadioMount = "/littlefs";

//fs::FS      RadioFS     = FFat;
//const int   RadioFSNO   = FSNO_FFAT;
//const char  *RadioMount = "/ffat";

typedef struct{
  String    filename;
  size_t    size;  
  uint8_t   *buffer;
} FBuf;

#define FBUFSIZE 100

FBuf FBFiles[ FBUFSIZE ];



// pointers to memory allocation functions to be set 
// to alloc and malloc or if PSRAM is available to
// ps_calloc and ps_malloc

void *(*gr_calloc)(size_t num, size_t size);
void *(*gr_realloc)(void *pointer, size_t size);
void *(*gr_malloc)( size_t size);

//
void tft_message( const char  *message1, const char *message = NULL );

WiFiClient        *radioclient;
WiFiClient        iclient;
#ifdef USETLS
WiFiClientSecure  sclient;
#endif
int               contentsize=0;


TFT_eSPI tft = TFT_eSPI();  // Invoke library, pins defined in User_Setup.h

TFT_eSprite img     = TFT_eSprite(&tft);  
TFT_eSprite msg     = TFT_eSprite(&tft);  
TFT_eSprite bats    = TFT_eSprite(&tft); 
TFT_eSprite vols    = TFT_eSprite(&tft);  
TFT_eSprite clocks  = TFT_eSprite(&tft);  
TFT_eSprite bmp     = TFT_eSprite(&tft);  
TFT_eSprite gest    = TFT_eSprite(&tft); 


//TFT_eSprite spa  = TFT_eSprite(&tft);

//play soft fade and spectrum parameters

#define GETBANDFREQ 50 // call getbands after every GETBANDFREQ chunks
#define SKIPSTART 450
uint32_t skipstartsound=SKIPSTART;
bool ModeChange = false;

//hangdetection
#define MAXUNAVAILABLE 50000
#define RESTART_AFTER_LOWQ_COUNT 100

int   unavailablecount=0;
int   failed_connects=0;
int   disconnectcount=0;
int   topunavailable=0;

//  int8_t         vs_shutdown_pin ;                    // GPIO to shut down the amplifier
//  int8_t         vs_shutdownx_pin ;                   // GPIO to shut down the amplifier (inversed logic)

//OTA password
#define APNAME   "GeleRadio"
#define APVERSION "V4.9"
#define APPAS     "oranjeboven"

SemaphoreHandle_t wifiSemaphore;
SemaphoreHandle_t staSemaphore;
SemaphoreHandle_t volSemaphore;
SemaphoreHandle_t tftSemaphore;
SemaphoreHandle_t updateSemaphore;
SemaphoreHandle_t scrollSemaphore;
SemaphoreHandle_t chooseSemaphore;
SemaphoreHandle_t radioSemaphore;
SemaphoreHandle_t clockSemaphore;
SemaphoreHandle_t stationSemaphore;


TaskHandle_t      gestureTask;   
TaskHandle_t      pixelTask;   
TaskHandle_t      webserverTask;  
TaskHandle_t      radioTask;
TaskHandle_t      playTask;
TaskHandle_t      scrollTask;
TaskHandle_t      touchTask;


#define WEBCORE     0
#define RADIOCORE   1
#define GESTURECORE 1
#define PLAYCORE    1
#define TOUCHCORE   1


#define PIXELTASKPRIO     3
#define GESTURETASKPRIO   4
#define TOUCHTASKPRIO     5
#define RADIOTASKPRIO     6
#define PLAYTASKPRIO      5
#define WEBSERVERTASKPRIO 2
#define SCROLLTASKPRIO    4

QueueHandle_t playQueue;
#define PLAYQUEUESIZE (512 + 256) 

// stations

typedef struct {
char *name;
int  protocol;
char *host;
char *path;
int   port;
int   status;
unsigned int   position;
}Station;


#define STATIONSSIZE 100
Station *stations; 

static volatile int     currentStation;
static volatile int     stationCount;
int                     playingStation = -1;
int                     chosenStation = 0;
int                     scrollStation = -1;
int                     scrollDirection;
int                     DEBUG = 1 ;                            // Debug on/off



sk gstrip;

enum gmenuLevel{
  gOff,
  gVolume,
  gStation,
  gScroll
};
int gmode=gOff;



// The object for the MP3 player
VS1053g* vs1053player;


//tft
#define SCROLLUP 0
#define SCROLLDOWN 1
// webserver
//WebServer server(80);

//battery
float   batvolt = 0.0;

// Default volume
int currentVolume=65; 

bool stationChunked = false;
bool stationClose   = false;


/*------------------------------------------------------*/
// https://github.com/espressif/arduino-esp32/blob/master/libraries/ESP32/examples/ResetReason/ResetReason.ino

void verbose_print_reset_reason(RESET_REASON reason, char *textbuffer)
{
  switch ( reason)
  {
    case 1  : strcpy(textbuffer,"POWERON_RESET Vbat power on reset") ;break;
    case 3  : strcpy(textbuffer,"SW_RESET Software reset digital core");break;
    case 4  : strcpy(textbuffer,"OWDT_RESET Legacy watch dog reset digital core");break;
    case 5  : strcpy(textbuffer,"DEEPSLEEP_RESET Deep Sleep reset digital core");break;
    case 6  : strcpy(textbuffer,"SDIO_RESET Reset by SLC module, reset digital core");break;
    case 7  : strcpy(textbuffer,"TG0WDT_SYS_RESET Timer Group0 Watch dog reset digital core");break;
    case 8  : strcpy(textbuffer,"TG1WDT_SYS_RESET Timer Group1 Watch dog reset digital core");break;
    case 9  : strcpy(textbuffer,"RTCWDT_SYS_RESET RTC Watch dog Reset digital core");break;
    case 10 : strcpy(textbuffer,"INTRUSION_RESET Instrusion tested to reset CPU");break;
    case 11 : strcpy(textbuffer,"TGWDT_CPU_RESET Time Group reset CPU");break;
    case 12 : strcpy(textbuffer,"SW_CPU_RESET Software reset CPU");break;
    case 13 : strcpy(textbuffer,"RTCWDT_CPU_RESET  RTC Watch dog Reset CPU");break;
    case 14 : strcpy(textbuffer,"EXT_CPU_RESET for APP CPU, reseted by PRO CPU");break;
    case 15 : strcpy(textbuffer,"RTCWDT_BROWN_OUT_RESET Reset when the vdd voltage is not stable");break;
    case 16 : strcpy(textbuffer,"RTCWDT_RTC_RESET RTC Watch dog reset digital core and rtc module");break;
    default : strcpy(textbuffer,"NO_REASON unknown reason");break;
  }
}
/*------------------------------------------------------*/
void log_boot(){ 
char textbuffer[132];

sprintf ( textbuffer, "Core 0 reset reason:  ");
//cpu 0 reason  
verbose_print_reset_reason(rtc_get_reset_reason(0), textbuffer + 20);
syslog( textbuffer);
Serial.println( textbuffer );

sprintf ( textbuffer, "Core 1 reset reason:  ");
//cpu 1 reason  
verbose_print_reset_reason(rtc_get_reset_reason(1), textbuffer + 20);
syslog( textbuffer);
Serial.println( textbuffer );


}
/*------------------------------------------------------*/

#ifdef USEOTA
void initOTA( char *apname, char *appass){
  

 // Port defaults to 8266
  // ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  ArduinoOTA.setHostname(apname);

  // No authentication by default
  ArduinoOTA.setPassword( appass );

  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

  ArduinoOTA.onStart([]() {
    String type;

    xSemaphoreTake( updateSemaphore, portMAX_DELAY);
    
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "Firmware";
      syslog((char *)"Installing new firmware over ArduinoOTA");
    } else { // U_SPIFFS
      type = "filesystem";
      RadioFS.end();
    }

    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    Serial.println("Start updating " + type);

      tft_ShowUpload( type );

  });
  ArduinoOTA.onEnd([]() {
   xSemaphoreGive( updateSemaphore); 
   tft_uploadEnd( "success");
   Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    //Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    tft_uploadProgress( (progress / (total / 100))  );

  });
  ArduinoOTA.onError([](ota_error_t error) {
    //tft_uploadEnd("failed");

    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      tft_uploadEnd("Auth failed");
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      tft_uploadEnd("Begin failed");
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      tft_uploadEnd("Connect failed");
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      tft_uploadEnd("Receive failed");  
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      tft_uploadEnd("End failed");
      Serial.println("End Failed");
    }
  });
  ArduinoOTA.begin();
}

#endif


//----------------------------------------------------------------

void startAfterWifi(){


    delay(500);
    msg.deleteSprite();
          
    tft.fillScreen(TFT_BLACK);
  
    #ifdef USETOUCH    
      touch_init();    
    #endif  

    Serial.println("Start WebServer...");
    setupAsyncWebServer();
 
    Serial.println("Start play task...");  
    play_init();
 
    Serial.println("Start radio task...");    
    radio_init();

    // time is set in getwifi, so valid timestamps in syslog from here, not earlier 
    Serial.println("log boot");    
    log_boot();
    
}

//----------------------------------------------------------------

void initSemaphores(){
  
    Serial.println("Creating semaphores...");
    
     staSemaphore = xSemaphoreCreateMutex();
     volSemaphore = xSemaphoreCreateMutex();
     tftSemaphore = xSemaphoreCreateMutex();
     updateSemaphore = xSemaphoreCreateMutex();
     scrollSemaphore = xSemaphoreCreateMutex();
     chooseSemaphore = xSemaphoreCreateMutex();
     radioSemaphore = xSemaphoreCreateMutex();
     clockSemaphore = xSemaphoreCreateMutex();
     stationSemaphore = xSemaphoreCreateMutex();
      
     Serial.println("Take sta semaphore...");
     xSemaphoreTake(staSemaphore, 10);
     xSemaphoreGive(staSemaphore);
     
     Serial.println("Take vol semaphore...");
     xSemaphoreTake(volSemaphore, 10);
     xSemaphoreGive(volSemaphore);

     Serial.println("Take tft semaphore...");
     xSemaphoreTake(tftSemaphore, 10);
     xSemaphoreGive(tftSemaphore);

     Serial.println("Take update semaphore...");
     xSemaphoreTake(updateSemaphore, 10);
     xSemaphoreGive(updateSemaphore);
    
     Serial.println("Take radio semaphore...");
     xSemaphoreTake(radioSemaphore, 10);
     xSemaphoreGive(radioSemaphore);

     Serial.println("Take clock semaphore...");
     xSemaphoreTake( clockSemaphore, 10);
     xSemaphoreGive( clockSemaphore);

     Serial.println("Take station semaphore...");
     xSemaphoreTake( stationSemaphore, 10);
     xSemaphoreGive( stationSemaphore);

}

//----------------------------------------------------------

void setup () {

    Serial.begin(115200);
    Serial.printf("\n%s %s  %s %s\n", APNAME, APVERSION, __DATE__, __TIME__);   

    if ( psramFound() ){
         Serial.println ( "PSRAM found");
         gr_calloc = ps_calloc;
         gr_malloc = ps_malloc;
         gr_realloc = ps_realloc;
    }else{
        Serial.println ( "No PSRAM found");
        gr_calloc  = calloc;
        gr_malloc  = malloc;
        gr_realloc = realloc;
    }

    initSemaphores();
    
    Serial.println("Create playQueue...");
    playQueue = xQueueCreate( PLAYQUEUESIZE, 32);

    Serial.println("SPI begin...");
    SPI.begin(); 

    //unreset the VS1053
    pinMode( VS1053_RST , OUTPUT);
    digitalWrite( VS1053_RST , LOW);
    delay( 200);
    digitalWrite( VS1053_RST , HIGH);
    delay(200);
    
    vs1053player = new VS1053g ( VS_CS_PIN,       // Make instance of player
                                 VS_DCS_PIN,
                                 VS_DREQ_PIN);

     // position and colors for the spectrum analyzer
     vs1053player->spectrum_height    = TFTSPECTRUMH;
     vs1053player->spectrum_top       = TFTSPECTRUMT;
     vs1053player->setSpectrumBarColor(TFT_REALGOLD);
     vs1053player->setSpectrumPeakColor(TFT_WHITE);
     
  
     Serial.println("Start File System...");
     setupFS();   

      if ( psramFound() ){ 
        log_w("Adding File buffers");
        addFBuf( "/stations.json");
        addFBuf( "/favicon.ico");
        addFBuf( "/index.html");
        FBuffAll("/");
      }else{
        log_w("no PSRAM ");
      }
  
     Serial.println("TFT init...");
     tft_init();

    
    // https://github.com/espressif/arduino-esp32/issues/3701#issuecomment-744706173
      
    #ifdef USEGESTURES
       Serial.println("Start gestures..."); 
       tft_message("Start gesture sensor" );  
       
       if ( gesture_init() ) Serial.println ( "FAILED to init gesture control");
       delay(200);
    
       tellPixels( PIX_BLINKYELLOW );
    #endif
    
     Serial.println("player begin...");
     tft_message("Start VS1053 decoder" );  
       
     vs1053player->begin();

     Serial.println("Test VS1053 chip...");
     while(1){ 
        bool   isconnected = vs1053player->isChipConnected();
        Serial.printf( " Chip connected ? %s \n", isconnected?"true":"false: wait a bit");
        if ( isconnected ) break;
        
        delay(500);
     }

     tft_message("Apply patches for decoder" );  
       
// apply patches and plugin.
// Apparetly after applying a soft reset is mandatory for the
// plugin to load succesfully.
// The switch to MP3 includes a soft reset.

     char patchname[128];     
     sprintf( patchname,"%s%s", RadioMount, "/patches/vs1053b-patches.plg");
     vs1053player->patch_VS1053( patchname );

     delay(200); 
     tft_message("Switch to MP3 mode and soft reset" );  
     log_i("Switch to MP3.../Soft reset");
     vs1053player->switchToMp3Mode();
     
     tft_message("Apply spectrum analyzer plugin" );  
     sprintf( patchname,"%s%s", RadioMount, "/patches/spectrum1053b-2.plg");
     vs1053player->patch_VS1053( patchname );
     
     currentVolume = get_last_volstat(1);
     vs1053player->setVolume(0);
    
     #ifdef USEPIXELS
       Serial.println("Start pixeltask...");    
       tft_message("Start pixels" );          
       initPixels();
     #endif
      
     Serial.println("point radioclient to insecure WiFiclient");
     radioclient = &iclient;

     tft_message("Read station list" );  
        
     Serial.println("Getstations...");
     stationsInit();

     tft_message("Start WiFi" );  
       
     Serial.println("Start WiFi en web...");
     //getWiFi( APNAME,APPAS);
     startWiFi();    

 delay(10000);
 

}

void loop(void){

   vTaskDelete( NULL );
   //ArduinoOTA.handle();    
   //server.handleClient();
   //SSDPDevice.handleClient(); 
   delay(100);

}
