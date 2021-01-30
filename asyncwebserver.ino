#ifdef ASYNCWEB

#include <WiFi.h>
#include <WiFiClient.h>
#include <esp_wifi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>
#include <Update.h>

AsyncWebServer    fsxserver(80);
TaskHandle_t      FSXServerTask;
SemaphoreHandle_t updateSemaphore;
static const char* updateform PROGMEM= "<form method='POST' action='/update' enctype='multipart/form-data'><input type='file' name='update'><input type='submit' value='Update'></form>";
static const char* uploadpage PROGMEM =
  R"(
<!DOCTYPE html>
<html>
<head>
 <meta charset='UTF-8'>
 <meta name='viewport' content='width=device-width, initial-scale=1.0'>

<style>
 body {
  font-family: Arial, Helvetica, sans-serif;  
  background: lightgrey;  
}
</style>
</head>
<html>
<h2>Upload file</h2>
<form method='POST' action='/upload' enctype='multipart/form-data' id='f' '>
<table>
<tr><td>Local file to upload </td><td><input type='file' name='blob' style='width: 300px;' id='pt'></td></tr>
<tr><td colspan=2> <input type='submit' value='Upload'></td></tr> 
</table>
</form>
 </html>)";

/*-----------------------------------------------------------------*/
//URL Encode Decode Functions
//https://circuits4you.com/2019/03/21/esp8266-url-encode-decode-example/

String urldecode(String str)
{
    
    String encodedString="";
    char c;
    char code0;
    char code1;
    for (int i =0; i < str.length(); i++){
        c=str.charAt(i);
      if (c == '+'){
        encodedString+=' ';  
      }else if (c == '%') {
        i++;
        code0=str.charAt(i);
        i++;
        code1=str.charAt(i);
        c = (h2int(code0) << 4) | h2int(code1);
        encodedString+=c;
      } else{
        
        encodedString+=c;  
      }
      
      yield();
    }
    
   return encodedString;
}

//------------------------------------------------------------------------------
size_t fileoffset( const char *fullfilename ){   
    const char *s;  
    for ( s= fullfilename + 1; *s != '/';++s );
    return( s - fullfilename ); 
}

/*-----------------------------------------------------------------*/
 
String urlencode(String str)
{
    String encodedString="";
    char c;
    char code0;
    char code1;
    char code2;
    for (int i =0; i < str.length(); i++){
      c=str.charAt(i);
      if (c == ' '){
        encodedString+= '+';
      } else if (isalnum(c)){
        encodedString+=c;
      } else{
        code1=(c & 0xf)+'0';
        if ((c & 0xf) >9){
            code1=(c & 0xf) - 10 + 'A';
        }
        c=(c>>4)&0xf;
        code0=c+'0';
        if (c > 9){
            code0=c - 10 + 'A';
        }
        code2='\0';
        encodedString+='%';
        encodedString+=code0;
        encodedString+=code1;
        //encodedString+=code2;
      }
      yield();
    }
    return encodedString;
    
}
//---------------------------------------------------------------------------

String getAContentType(String filename) {
  
  if (filename.endsWith(".htm")) {
    return "text/html";
  } else if (filename.endsWith(".html")) {
    return "text/html";
  } else if (filename.endsWith(".css")) {
    return "text/css";
  } else if (filename.endsWith(".js")) {
    return "application/javascript";
  } else if (filename.endsWith(".png")) {
    return "image/png";
  } else if (filename.endsWith(".gif")) {
    return "image/gif";
   } else if (filename.endsWith(".bmp")) {
    return "image/bmp";
  } else if (filename.endsWith(".jpg")) {
    return "image/jpeg";
  } else if (filename.endsWith(".ico")) {
    return "image/x-icon";
  } else if (filename.endsWith(".xml")) {
    return "text/xml";
  } else if (filename.endsWith(".pdf")) {
    return "application/x-pdf";
  } else if (filename.endsWith(".zip")) {
    return "application/x-zip";
  } else if (filename.endsWith(".gz")) {
    return "application/x-gzip";
  }else if (filename.endsWith(".wav")) {
    return "audio/wav";
  }else if (filename.endsWith(".mp3")) {
    return "audio/mp3";
  }else if (filename.endsWith(".m4a")) {
    return "audio/m4a";
  }else if (filename.endsWith(".flac")) {
    return "audio/flac";
  }
  return "text/plain";
}

 
/*-----------------------------------------------------------------*/
void send_json_status(AsyncWebServerRequest *request)
{
char uptime[32];
int sec = millis() / 1000;
int upsec,upminute,uphr,updays;
int retval=0;
 
upminute = (sec / 60) % 60;
uphr     = (sec / (60*60)) % 24;
updays   = sec  / (24*60*60);
upsec    = sec % 60;

sprintf( uptime, "%d %02d:%02d:%02d", updays, uphr, upminute,upsec);

  String output = "{\r\n";

  output += "\t\"Application\" : \"";
  output += APNAME " " APVERSION;
  output += "\",\r\n";

  output += "\t\"CompileDate\" : \"";
  output += __DATE__  " " __TIME__ ;
  output += "\",\r\n";

  output += "\t\"uptime\" : \"";
  output += uptime;
  output += "\",\r\n";

  //output += "\t\"Battery\" : ";
  //output += batvolt;
  //output += ",\r\n";


  output += "\t\"failed_connects\" : ";
  output += failed_connects;
  output += ",\r\n";

  output += "\t\"disconnectcount\" : ";
  output += disconnectcount;
  output += ",\r\n";

  output += "\t\"topunavailable\" : ";
  output += topunavailable;
  output += ",\r\n";

  output += "\t\"queueDepth\" : ";
  output += uxQueueMessagesWaiting(playQueue);
  output += ",\r\n";
  
  output += "\t\"currentStation\" : \"";
  output += stations[ getStation()].name;
  output += "\",\r\n";

  output += "\t\"currentStationIndex\" : ";
  output += getStation();
  output += ",\r\n";

  output += "\t\"currentVolume\" : ";
  output += getVolume();
  output += "\r\n";
   
  output += "}" ;
    
  request->send(200, "application/json;charset=UTF-8", output);
}

//------------------------------------------------------------------

void handleSettings(){
int hasargs=0;
int reset_ESP=0;


if ( server.hasArg("json") ){
  String output = "{";
//nonsense data
  output += "\"currentStation\" : \"";
  output += stations[ getStation() ].name;
  output += "\"";

  output += ",\"currentVolume\" : ";
  output += getVolume();
   
  output += "}" ;
    
  server.send(200, "text/json", output);
     
}

if ( ! hasargs ){
  handleFileRead( "/settings.html" );
}else{
   handleFileRead( "/settings.html" );
}
  
}
//------------------------------------------------------------------

void handleDel( AsyncWebServerRequest *request ){
  int   i, return_status = 400,rc;
  char  message[80];
  
  if ( !request->hasParam("name") || !request->hasParam("index")   ){
        sprintf( message, "Error: del host needs ?name=xx&index=xx" );
        request->send( return_status, "text/plain", message);
        return;
  }

 
  rc = del_station(  (char *)request->getParam("name")->value().c_str(), 
                     request->getParam("index")->value().toInt() ); 

  if ( rc ){
        sprintf( message, "Error: Station \"%s\" with index %d not found", request->getParam("name")->value().c_str(), request->getParam("index")->value().toInt()  );
        Serial.println( message );

  }else{
    save_stations();                
    return_status = 200;
    read_stations();
    sprintf( message,"Deleted station %s", server.arg("name") );      
  }
  
  request->send( return_status, "text/plain", message);
    
}
//------------------------------------------------------------------

void handleAdd( AsyncWebServerRequest *request ){
  int   return_status = 400,rc;
  char  message[80];
  
  if ( !request->hasParam("name") || !request->hasParam("host") || !request->hasParam("path") || !request->hasParam("port") || !request->hasParam("idx") || !request->hasParam("protocol") ){
        sprintf( message, "Error: add host needs ?host=xx&name=xx&path=xx&port=xx&idx=xx&protocol=xxx" );
        request->send( return_status, "text/plain", message);
        return;
  }

  if ( request->getParam("idx")->value().toInt() == -1 ){
      rc = add_station(  (char *)request->getParam("name")->value().c_str(), 
                request->getParam("protocol")->value().toInt(),
                (char *)request->getParam("host")->value().c_str(), 
                (char *)request->getParam("path")->value().c_str(), 
                request->getParam("port")->value().toInt() ); 
                 
  }else{
      int idx = request->getParam("idx")->value().toInt();

      rc = add_station(  (char *)request->getParam("name")->value().c_str(), 
                request->getParam("protocol")->value().toInt(),
                (char *)request->getParam("host")->value().c_str(), 
                (char *)request->getParam("path")->value().c_str(), 
                request->getParam("port")->value().toInt(),
                idx ); 
      Serial.printf( "- Changed station %d to : name %s, h %s p %d path %s\n", idx, stations[idx].name,  stations[idx].host, stations[idx].port, stations[idx].path);
  }
  if ( rc ){
        sprintf( message, "Error: No more stations can be added" );
  }else{
    
    save_stations();                
    return_status = 200;
    sprintf( message,"Added station %s", server.arg("name") );      
  }
  
  request->send( return_status, "text/plain", message);
    
}

//------------------------------------------------------------------

void handleSet( AsyncWebServerRequest *request ){
  int   return_status = 400;
  char  message[80];
  
  if ( request->hasParam("volume") ){
      int desired_volume = request->getParam("volume")->value().toInt();
      if ( desired_volume >= 0 && desired_volume <= 100 ){
        return_status = 200;
        sprintf( message,"Volume set to %d", desired_volume );
        setVolume( desired_volume );
      }else{
        sprintf( message,"Volume requested %d, but value must be between 0 and 100", desired_volume);
      }
  } 
  if ( request->hasParam("station") ){
      int desired_station = request->getParam("station")->value().toInt();
      if ( desired_station < STATIONSSIZE && desired_station >= 0 && stations[ desired_station ].status == 1 ){
        return_status = 200;
        if( desired_station != getStation() ) setStation( desired_station, -1 );
        sprintf( message,"Station set to %d, %s", desired_station, stations [ desired_station].name );      
      }else{
        sprintf( message,"Station %d does not exist", desired_station);
      }
  }  
  
  request->send( return_status, "text/plain", message );   
}

//--------------------------------------------------------------------------------

void handleFileRead(  AsyncWebServerRequest *request ) {

  String path = request->url();
  
  Serial.println("handleFileRead: " + path);
  if (path.endsWith("/")) {
    path += "index.html";
  }
  path = urldecode( path);
  
  fs::FS  *ff = &RadioFS

  String fspath;
  
  fspath = path.substring( fileoffset( path.c_str()) );

  String contentType = getAContentType(path);
  String pathWithGz = fspath + ".gz";
  
  if ( ff->exists(pathWithGz) || ff->exists(fspath)) {
    if ( ff->exists(pathWithGz)) {
      fspath += ".gz";
    }

    if ( request->hasParam("download") )contentType = "application/octet-stream";

    request->send( *ff, fspath, contentType );
    Serial.printf("File has been streamed\n");
    
    return;
  }
  request->send( 404, "text/plain", "FileNotFound"); 
}


//--------------------------------------------------------------------------

void handleUpdate(AsyncWebServerRequest *request, const String& filename, size_t index, uint8_t *data, size_t len, bool final) {
  
  uint32_t free_space = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
  
  if (!index){
  
    Serial.println("Update firmware");

        for ( int curvol = getVolume(); curvol; --curvol ){
          vs1053player->setVolume( curvol  );
          delay( 5 );
        }
       
     
       xSemaphoreTake( updateSemaphore, portMAX_DELAY);
      
       tft_ShowUpload( "firmware" );
        
       syslog((char *)"Installing new firmware through webupload" );
      
       
      //Update.runAsync(true);
      //content_len = request->contentLength();
      // if filename includes spiffs, update the spiffs partition
      //int cmd = (filename.indexOf("spiffs") > -1) ? U_PART : U_FLASH;
      if (!Update.begin( free_space )) {
        Update.printError(Serial);
      }
  }

  if (Update.write(data, len) != len) {
    Update.printError(Serial);
  }
  esp_task_wdt_reset();

  if (final) {
    
    AsyncWebServerResponse *response = request->beginResponse(302, "text/plain", "Please wait while the device reboots");
    response->addHeader("Refresh", "20");  
    response->addHeader("Location", "/");
    request->send(response);

    xSemaphoreGive( updateSemaphore);
    
    if (!Update.end(true)){
      Update.printError(Serial);
    } else {
      Serial.println("\nUpdate complete");
      Serial.flush();
      delay(100);
      ESP.restart();
    }
  }
}
//---------------------------------------------------------------------------------------
void printProgress(size_t progress, size_t size) {
  size_t percent = (progress*1000)/size;
  if ( (percent%100)  == 0 ){
    Serial.printf(" %u%%\r", percent/10);
     tft_uploadProgress( percent/10  );
  }
}

//------------------------------------------------------------------
void startWebServer( void *param ){
 
  Serial.printf("Async WebServer sterted from core %d\n", xPortGetCoreID()); 
 
  MDNS.begin( APNAME);
 
  fsxserver.on("/", handleRoot); 
  fsxserver.on("/list", HTTP_GET, handleFileList);
  fsxserver.on("/move", HTTP_POST, handleMove); 
  fsxserver.on("/rename", HTTP_GET, handleRename);
  
  fsxserver.on("/mkdir", HTTP_GET, handleMkdir);
  fsxserver.on("/delete", HTTP_GET, handleFileDelete);
  fsxserver.on("/delete", HTTP_POST, handleMultiDelete);
  
  fsxserver.on("/upload", HTTP_GET, showUploadform);
  fsxserver.on("/upload", HTTP_POST, [](AsyncWebServerRequest *request){ request->send(200, "text/html", uploadpage); }, handleFileUpload);

  fsxserver.on("/settings", HTTP_GET, send_settings);
  fsxserver.on("/settings", HTTP_POST, handleSettings);
  
  fsxserver.on("/status", HTTP_GET, send_json_status );
  fsxserver.on("/favicon.ico", send_logo );

  fsxserver.onNotFound( handleFileRead );

  fsxserver.on("/reset", HTTP_GET, []( AsyncWebServerRequest *request ) {
        request->send(200, "text/plain", "Doej!");
        delay(20);
        ESP.restart();
  });

  fsxserver.on("/update", HTTP_GET, []( AsyncWebServerRequest *request ) {
        request->send( 200, "text/html", updateform );
  });  
  fsxserver.on("/update", HTTP_POST, [](AsyncWebServerRequest *request){ request->send( 200, "text/html",updateform); }, handleUpdate);
 
  fsxserver.begin();
  Update.onProgress(printProgress);
  
  while(1){
 //   fsxserver.handleClient();
    delay(100);
  }
  
//------------------------------------------------------------------

void setupAsyncWebServer(){

    xTaskCreatePinnedToCore( 
         startWebServer,                                      // Task to handle special functions.
         "WebServer",                                            // name of task.
         1024*4,                                                 // Stack size of task
         NULL,                                                 // parameter of the task
         WEBSERVERTASKPRIO,                                    // priority of the task
         &webserverTask,                                        // Task handle to keep track of created task 
         WEBCORE );                                                 //core to run it on


}
#endif
