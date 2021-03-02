// Using WIFiMulti all networks in encrypted file /netpass are 
// tried, if in reach. If no networks available in /netpass are 
// found, smart config is started, and with the ESP touch app 
// on a phone connected to the network you want to connect to
// you can supply the network passord to the Radio. 
// Once connected succesfully, the radio will save the network 
// and password in encrypted file /netpass.
// In wificredentials the iv and key needed for encryption and
// decryption should be defined as
// const char* gr_iv      = "1234567890123456"; // length minimal 16
// const char* gr_key     = "12345678901234567890123456789012"; // length minimal 32
// This is obviously not super safe, but protects against the dumbest
// attempts to get this sensitive information, and offers the benefit 
// of automatic connection to several networks ( e.g. home, work and family)

#include <hwcrypto/aes.h>

struct netp{
  uint8_t  ssid[48]; // max 32
  uint8_t  pass[64]; //max 63
};

TaskHandle_t      startwifiTask;   
std::vector<struct netp>  netpass;

uint8_t key[32];
uint8_t iv[16];

WiFiMulti wifiMulti;

fs::FS multifs = RadioFS;

//---------------------------------------------------------------------------------------

void WiFiLostIP(WiFiEvent_t event, WiFiEventInfo_t info)
{     
      tft_message( "Connection lost to", "network"); 
      return;
}
//---------------------------------------------------------------------------------------
void add2netp( String nssid = WiFi.SSID(), String npsk = WiFi.psk() ){

  bool foundssid = false; 
 
  for ( auto n : netpass ){
    if ( !strcmp( (char *)n.ssid, nssid.c_str() ) ){
      // Maybe the pasword changed? If so, change it.
      if ( strcmp( (char *)n.pass, npsk.c_str() ) ){
           memset( n.pass, 0, sizeof( n.pass ) );
           strcpy( (char *)n.pass, npsk.c_str() );
      }else{
        foundssid = true;
      }
      break;  
    }
  }
  
  if ( !foundssid ) {
    struct netp n;
    
    strcpy( (char *)n.pass, npsk.c_str() );
    strcpy( (char *)n.ssid, nssid.c_str() );
    
    netpass.push_back( n );
    netp2file();
  }else{
    log_i( "network %s already known", nssid.c_str() );
  }
  //displaynetp();
}

//---------------------------------------------------------------------------------------

void WiFiGotIP(WiFiEvent_t event, WiFiEventInfo_t info)
{
    Serial.println("\n\nWiFi connected");
    Serial.print("Obtained IP address: ");
    Serial.print(WiFi.localIP());
    Serial.print( " on WiFi network with SSID ");
    Serial.println(WiFi.SSID() );
    
    const char  *foundname;
    tcpip_adapter_get_hostname(TCPIP_ADAPTER_IF_STA, &foundname);
    Serial.printf("hostname : %s\n", foundname );
    
    add2netp();
    tft_message( "Connected to WiFi, IP address: " , WiFi.localIP().toString().c_str() ); 
    tft_message( "Retrieving time from NTP server");
    ntp_setup( true );
    tellPixels( PIX_BLINKBLUE );
    
    #ifdef USEOTA
      initOTA( APNAME, APPAS );
    #endif
  
    tft_message( "Start radio");
  
    startAfterWifi();  

}
//---------------------------------------------------------------------------------------

void WiFisetHostname(WiFiEvent_t event, WiFiEventInfo_t info){
  tcpip_adapter_set_hostname(TCPIP_ADAPTER_IF_STA, APNAME );
}


//---------------------------------------------------------------------------------------

void runWiFi( void *param){

  //deleteFile(multifs, "/netpass");

  file2netp();
  //displaynetp();

  //add2netp( ssid , password ); // add wifi and password in wificredentials 
    
  WiFi.onEvent(WiFiGotIP, WiFiEvent_t::SYSTEM_EVENT_STA_GOT_IP); 
  WiFi.onEvent(WiFiLostIP, WiFiEvent_t::SYSTEM_EVENT_STA_LOST_IP); 
  WiFi.onEvent(WiFisetHostname, WiFiEvent_t::SYSTEM_EVENT_STA_CONNECTED);
   
  WiFi.mode(WIFI_STA);

  log_i("Waiting for WiFi");
  
  for ( int i=0 ; i < 10; ++i){
    int mstatus;
    if ( (mstatus = wifiMulti.run()) == WL_CONNECTED) break;  
    Serial.printf( "%d mstatus = %d\n", i, mstatus);  
    if ( mstatus == 6) break;
    delay(2000);
  }
    
  if ( WiFi.status() != WL_CONNECTED  ){
    
    //Init WiFi as Station, start SmartConfig
     //WiFi.mode(WIFI_AP_STA);
    tft_message("No network", "Use ESP Touch" );  
    WiFi.beginSmartConfig();

    Serial.println("Started smartconfig");

    // 5 minutes to connect using smartconfig
    for (int i=0; !WiFi.smartConfigDone() && i < 300 ; ++i ) {
      delay(500);
      Serial.print(".");
    }
    
    if ( !WiFi.smartConfigDone() ){
      Serial.printf( "\n%s stop of smartconfig. No WiFi.", WiFi.stopSmartConfig()?"Successful":"Failed");    
    }
      
  }

  delay(100);
  netpass.clear();
  netpass.shrink_to_fit();
  vTaskDelete( NULL );
 
}
//---------------------------------------------------------------------------------------

void startWiFi(){

     xTaskCreatePinnedToCore( 
         runWiFi,                                      // Task to handle special functions.
         "WiFiStart",                                  // name of task.
         1024*16,                                      // Stack size of task
         NULL,                                         // parameter of the task
         2,                                            // priority of the task
         &startwifiTask,                               // Task handle to keep track of created task 
         0 );                                          //core to run it on

  
}

//------------------------------------------------------------------------------
void netp2file(){
  log_i("netp2file");
  size_t netpsize = netpass.size() * sizeof ( struct netp );

  memset( iv, 0, sizeof( iv ) );
  snprintf( (char *) iv, sizeof( iv) , gr_iv); // gr_key and gr_iv defined in WiFicredentials.h 
  memset( key, 0, sizeof( key ) );
  snprintf( (char *)key, sizeof( key), gr_key);

  uint8_t* cryptbuf = (uint8_t *) gr_calloc( netpsize , sizeof( uint8_t) );
 
  esp_aes_context ctx;
  esp_aes_init( &ctx );
  esp_aes_setkey( &ctx, key, 256 );

  uint8_t *plain = (uint8_t *)netpass.data();
  esp_aes_crypt_cbc( &ctx, ESP_AES_ENCRYPT, netpsize, iv, plain, cryptbuf );

/* //debug
  Serial.printf("Encrypted\n");
  for ( int i =0; i < netpsize; ++i ){
    Serial.printf( "%02x",cryptbuf[i]);
    if ( 0 == (i+1)%32)Serial.printf("\n"); 
  }
*/

  File file = multifs.open("/netpass", FILE_WRITE);
  if(!file){
    log_e("- failed to open file for writing");
  }else{
    log_i("written %d bytes to file", file.write( cryptbuf, netpsize ) );    
  }
    
free( cryptbuf );
esp_aes_free( &ctx );

}

//------------------------------------------------------------------------------

void file2netp(){

log_i("file2netp");
  
memset( iv, 0, sizeof( iv ) );
snprintf( (char *) iv, sizeof( iv) , gr_iv ); // to be defined in wificredentials as const char *encrypt_iv = "randomcharacterstobekeptsecret";
memset( key, 0, sizeof( key ) );
snprintf( (char *)key, sizeof( key), gr_key); // to be defined in wificredentials as const char *encrypt_key = "randomcharacterstobekeptsecret";

File file = multifs.open("/netpass");
if ( ! file ) {
    log_e( "No file found ");
    return;
}
size_t filesize = file.size();
uint8_t* cryptbuf = (uint8_t *) gr_calloc( filesize, sizeof( uint8_t) );

file.read( cryptbuf, filesize );

file.close();
/* //debug
Serial.printf("Encrypted - from file with %d bytes\n", filesize);
for ( int i =0; i < filesize ; ++i ){
    Serial.printf( "%02x",cryptbuf[i]);
    if ( 0 == (i+1)%32)Serial.printf("\n"); 
}
Serial.printf( "\n");
*/

esp_aes_context ctx;
esp_aes_init( &ctx );
esp_aes_setkey( &ctx, key, 256 );


uint8_t* plainbuf = (uint8_t *) gr_calloc( filesize, sizeof( uint8_t) );

esp_aes_crypt_cbc( &ctx, ESP_AES_DECRYPT, filesize, iv, cryptbuf, plainbuf );

struct  netp* n = (struct netp *) plainbuf;
size_t  plaincount = filesize / sizeof ( struct netp);

for ( int i = 0; i < plaincount; ++i ){
    netpass.push_back( *n );
    wifiMulti.addAP((char *)n->ssid, (char *)n->pass);
    ++n;
}
log_i("read the following networks from file");
//displaynetp();

esp_aes_free( &ctx );
free( cryptbuf );
free( plainbuf );

}


//----debug--------------------------------------------------------------------------
/*
void displaynetp(){
  int count=0;
  log_i("Display netpass");
  for ( auto n : netpass ){
    Serial.printf( "[%s] - [%s]\n", n.ssid, n.pass );
    count++;
  }
  log_i("Found %d networks\n", count);
  
}
*/

//----debug-----------------------------------------------------------------------------
/*void deleteFile(fs::FS &fs, const char * path){
    Serial.printf("Deleting file: %s\r\n", path);
    if(fs.remove(path)){
        Serial.println("- file deleted");
    } else {
        Serial.println("- delete failed");
    }
}
*/
