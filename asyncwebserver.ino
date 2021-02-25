#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>
#include <Update.h>
#include "soc/timer_group_struct.h"
#include "soc/timer_group_reg.h"
#include <esp_task_wdt.h>
#include <dirent.h>
#include "tft.h"
#include "owm.h"

AsyncWebServer    fsxserver(80);
TaskHandle_t      FSXServerTask;
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

//------------------------------------------------------------------------------
size_t fileoffset( const char *fullfilename ){   
    const char *s;  
    for ( s= fullfilename + 1; *s && *s != '/' ;++s );
    if ( *s ){
      return( s - fullfilename ); 
    }else{
      return( 0 );
    }
}
//------------------------------------------------------------------------------------
 
unsigned char h2int(char c)
{
    if (c >= '0' && c <='9'){
        return((unsigned char)c - '0');
    }
    if (c >= 'a' && c <='f'){
        return((unsigned char)c - 'a' + 10);
    }
    if (c >= 'A' && c <='F'){
        return((unsigned char)c - 'A' + 10);
    }
    return(0);
}
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

/*-----------------------------------------------------------------*/
 
String urlencode(String str){
    String encodedString="";
    char c;
    char code0;
    char code1;
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
        encodedString+='%';
        encodedString+=code0;
        encodedString+=code1;
        
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
  } else if (filename.endsWith(".json")) {
    return "application/json";
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

  output += "\t\"RAMsize\" : ";
  output += ESP.getHeapSize();
  output += ",\r\n";

  output += "\t\"RAMused\" : ";
  output += ( ESP.getHeapSize() - ESP.getFreeHeap() );
  output += ",\r\n";
  
  output += "\t\"RAMfree\" : ";
  output += ESP.getFreeHeap();
  output += ",\r\n";

  output += "\t\"PSRAMsize\" : ";
  output += psramFound()?ESP.getPsramSize():-1;
  output += ",\r\n";

  output += "\t\"PSRAMused\" : ";
  output += psramFound()?( ESP.getPsramSize()-ESP.getMaxAllocPsram() ) : -1; 
  output += ",\r\n";

  output += "\t\"PSRAMfree\" : ";
  output += psramFound()? ESP.getMaxAllocPsram() : -1; 
  output += ",\r\n";

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

void handleSettings(AsyncWebServerRequest *request){


  if ( request->hasArg("json") ){
    String output = "{";
  //nonsense data
    output += "\"currentStation\" : \"";
    output += stations[ getStation() ].name;
    output += "\"";
  
    output += ",\"currentVolume\" : ";
    output += getVolume();
     
    output += "}" ;
  }else{
    request->send( RadioFS, "/settings.html","text/html");  
  }  
}
//------------------------------------------------------------------

void handleDel( AsyncWebServerRequest *request ){
  int   return_status = 400,rc;
  char  message[80];
  
  if ( !request->hasParam("name") || !request->hasParam("index")   ){
        sprintf( message, "Error: del host needs ?name=xx&index=xx" );
        request->send( return_status, "text/plain", message);
        return;
  }

 
  rc = del_station(  (char *)request->getParam("name")->value().c_str(), 
                     request->getParam("index")->value().toInt() ); 

  if ( rc ){
    sprintf( message, "Error: Station \"%s\" with index %ld not found", request->getParam("name")->value().c_str(), request->getParam("index")->value().toInt()  );
    Serial.println( message );
  }else{
    save_stations();                
    return_status = 200;
    read_stations();
    sprintf( message,"Deleted station %s", server.arg("name").c_str() );      
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

      rc = change_station(  (char *)request->getParam("name")->value().c_str(), 
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
    sprintf( message,"Added station %s", server.arg("name").c_str() );      
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
//---------------------------------------------------------------------------

void find_json_tree ( String &output, const char *path, int level=0 ){
    
    fs::File dir = RadioFS.open( path );     
    fs::File entry;
    
    if ( dir.isDirectory() ){
      for (int i=0; i < (level*3);++i)output += " ";
      output += "{\"filename\" : \"";
      output += path;
      output += "\", \"type\" : \"directory\",\"files\" :[";
    }else{
      dir.close();
      return;
    }

    int fcount = 0;       
    while ( entry = dir.openNextFile() ){                

      char  *fname = strdup( entry.name() );
      size_t fsize = entry.size();
      bool   isdir = entry.isDirectory();
      
      entry.close();
      
      if ( isdir  ){
         output += ",\r\n"; 
         find_json_tree( output, fname, level+1 );
      } else{ 
         for (int i=0; i < (level*3);++i)Serial.print(" ");

         if ( fcount ){output += ",\r\n";} else{ output += "\r\n";}
         fcount++;
          
         for (int i=0; i < (level*3);++i)output += " ";
         output += "{\"filename\" : \"";
         output += fname;
         output += "\", \"type\" : \"file\",\"size\" :";
         output += fsize;
         output += "}"; 
      }
      free(fname);
    }    
    dir.close();
    
    output += "]}";        
    if ( level )output += "\r\n";        

}

//---------------------------------------------------------------------------

void find_treefiles ( const char *path, int level=0 ){
    
    fs::File dir = RadioFS.open( path );     
    fs::File entry;
    
    if ( dir.isDirectory() ){
      for (int i=0; i < (level*3);++i)Serial.print(" ");
      Serial.printf( "%s directory\n", path );
    }else{
      dir.close();
      return;
    }
           
    while ( entry = dir.openNextFile() ){                
      if ( entry.isDirectory()  ){
         find_treefiles( entry.name(), level+1 );        
      } else{ 
         for (int i=0; i < (level*3);++i)Serial.print(" ");
         Serial.printf( "%s %u\n", entry.name(), entry.size() );         
      }
      entry.close();          
    }    
    dir.close();
}

//----------------------------------------------
void setupFS(void) {

      switch ( RadioFSNO ){
        case FSNO_SPIFFS:
            SPIFFS.begin();
            break;
        case FSNO_LITTLEFS:
            LITTLEFS.begin();
            break;
        case FSNO_FFAT:    
            FFat.begin();
            break;
      }


      find_treefiles ( "/" );

  
}

//------------------------------------------------

void handleFileDelete( AsyncWebServerRequest *request ) {
  
  if ( request->params() == 0) {
    request->send(500, "text/plain", "BAD ARGS for delete");
    return;
  }
  
  AsyncWebParameter* p = request->getParam(0);
  String path = p->value();

  path = urldecode( path);
  log_i("Delete file %s ", path.c_str() );

  if ( path == "/") {
      request->send(500, "text/plain", "/ is not a valid filename");
      return;
  }
  if ( !RadioFS.exists( path)) {
    request->send(404, "text/plain", "FileNotFound");
    return;
  }
  if ( RadioFS.remove(path) ){
    log_i("file %s deleted", path.c_str() );
  }else{
    log_i("could not delete %s", path.c_str() );    
  }
  request->send(200, "text/plain", "");
}


//--------------------------------------------------------------
void handleFileList(  AsyncWebServerRequest *request ) {

  String output= "{ \"fs\": [\r\n";
  find_json_tree ( output, "/", 0 );
  output += "]\r\n}";

  request->send(200, "application/json;charset=UTF-8", output);
}
//--------------------------------------------------------------

void handleFileRead(  AsyncWebServerRequest *request ) {

  String path = request->url();
  
  log_i("handleFileRead: %s", path.c_str());
  if (path.endsWith("/")) {
    path += "index.html";
  }
  path = urldecode( path);
  
  FBuf *pathbuf = findFBuf( path );
  
  fs::FS  *ff = &RadioFS;

  String contentType = getAContentType(path);

  if ( !pathbuf ){
    
      String pathWithGz = path + ".gz";
      
      if ( ff->exists(pathWithGz) || ff->exists(path)) {
        if ( ff->exists(pathWithGz)) {
          path += ".gz";
        }
      }else{
        request->send( 404, "text/plain", "FileNotFound"); 
      }
  } 
    
  if ( request->hasParam("download") )contentType = "application/octet-stream";

  if ( pathbuf ){
    AsyncWebServerResponse *response = request->beginResponse_P(200, contentType, pathbuf->buffer, pathbuf->size );
    request->send(response);
    log_i("File has been streamed from buffer");
  }else{
    request->send( *ff, path, contentType );
    log_i("File has been streamed from filesystem");
  }  
  
  return;
}

//---------------------------------------------------------------------------

void handleFileUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final){
 
  static File fsUploadFile;
  static String fspath;
  static int  bcount;
  
  if(!index){
        String path;       
        log_i("UploadStart: %s\n", filename.c_str());

        if ( request->hasParam("filename") ){
           path = request->getParam("filename")->value();                         
            
        }else{
          log_i( "upload request has no param filename" );
          path = filename;          
        }
        
        if (!path.startsWith("/")) {
          path = "/" + path;
        }
        
        log_i("handleFileUpload Name %s ", path.c_str() ); 
        
        fs::FS  *ff = &RadioFS;
        
        if ( path == "/") {
          request->send(500, "text/plain", "/ is not a valid filename");
          return;
        }
  
        fspath = path;
              
        bcount=0;
        fsUploadFile = ff->open( fspath, "w");
        if ( !fsUploadFile  ){
              log_e("Error opening file %s %s", fspath.c_str(), (strerror(errno)) );
              request->send(400, "text/plain", String("Error opening ") + fspath + String(" ") + String ( strerror(errno) ) );
              return;
        }
        
  }
  int olderrno = errno;
  if (fsUploadFile) {
      if ( fsUploadFile.write( data, len) < len ){
           fsUploadFile.close();
           if ( olderrno != errno ){
            request->send(500, "text/plain", "Error during write : " + String ( strerror(errno) ) );
            log_e("Error writing file %s %s", fspath.c_str(),strerror(errno) );
            return;
           }
      }
      delay(2);
      bcount +=len;
  }

  if ( final ){
    if (fsUploadFile) {
      fsUploadFile.close();

      if ( delFBuf( fspath ) ) addFBuf( fspath );        
    }

    log_i("Uploaded file %s of %u bytes\n", fspath.c_str(), bcount);
    fspath = "";
  }
}

size_t  update_size = 0;
//---------------------------------------------------------------------------------------
void printProgress(size_t progress, size_t size) {
  static int printstep=0; 
  size_t percent = (progress*100)/update_size;
  if ( percent >= printstep ){
    printstep += 10;
    Serial.printf(" %u", percent );
    tft_uploadProgress( percent  );
  }
}
//--------------------------------------------------------------------------

void handleUpdate(AsyncWebServerRequest *request, const String& filename, size_t index, uint8_t *data, size_t len, bool final) {
  
  uint32_t free_space = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
  
  if (!index){
  
    Serial.println("Update firmware");

        for ( int curvol = vs1053player->getVolume(); curvol; --curvol ){
          vs1053player->setVolume( curvol  );
          delay( 5 );
        }

       xSemaphoreTake( updateSemaphore, portMAX_DELAY);
       
       update_size = request->contentLength();
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

  delay(2);
  
  if (final) {
    
    AsyncWebServerResponse *response = request->beginResponse(302, "text/plain", "Please wait while the device reboots");
    response->addHeader("Refresh", "20");  
    response->addHeader("Location", "/");
    request->send(response);
    
    syslog((char *)"Installed new firmware through webupload" );
    
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


//------------------------------------------------------------------
void startWebServer( void *param ){
 
  Serial.printf("Async WebServer started from core %d\n", xPortGetCoreID()); 
 
  MDNS.begin( APNAME);

  

  fsxserver.serveStatic("/stations.json", RadioFS, "/stations.json");
  fsxserver.on("/favicon.ico", HTTP_GET, handleFileRead);
  fsxserver.on("/index.html", HTTP_GET, handleFileRead);
  
  fsxserver.on("/fbuf", HTTP_GET, showFBuf );
  fsxserver.on("/list", HTTP_GET, handleFileList);

  fsxserver.on("/delete", HTTP_GET, handleFileDelete); //usage: .../delete?file=/blurb.bmp
  fsxserver.on("/upload", HTTP_GET, []( AsyncWebServerRequest *request ) 
        {request->send(200, "text/html", uploadpage);});
        
  fsxserver.on("/upload", HTTP_POST, [](AsyncWebServerRequest *request)
    {request->send(200, "text/html", uploadpage); }, handleFileUpload);

  fsxserver.on("/set", HTTP_GET, handleSet );
  fsxserver.on("/add", HTTP_GET, handleAdd );
  fsxserver.on("/del", HTTP_GET, handleDel );
  fsxserver.on("/status", HTTP_GET, send_json_status );

  fsxserver.on("/reset", HTTP_GET, []( AsyncWebServerRequest *request ) {
        request->send(200, "text/plain", "Goodbye!");
        delay(20);
        ESP.restart();
  });

  fsxserver.on("/update", HTTP_GET, []( AsyncWebServerRequest *request ) {
        request->send( 200, "text/html", updateform );
  });  
  fsxserver.on("/update", HTTP_POST, [](AsyncWebServerRequest *request){ request->send( 200, "text/html",updateform); }, handleUpdate);

  fsxserver.onNotFound( handleFileRead );
     
  fsxserver.begin();
  Update.onProgress(printProgress);

#ifndef USESSDP
    MDNS.addService("http", "tcp", 80);
#else
    int dossdp= 0;
#endif


// loop for frequent updates

int     delaytime = 60;
int     timecount = (1000/delaytime), oldmin=987;
int     weathercount = 0;  // open weather, every hour, but also at start
time_t  rawt;
struct tm tinfo;

  while(1){
    
    
    
    #ifdef USESSDP
     ++dossdp;
     if ( dossdp >= 40 ){
        SSDPDevice.handleClient(); 
        dossdp = 0;
     }
    #endif   
    
    #ifdef USEOTA
        ArduinoOTA.handle();    
    #endif
  
    --timecount;
    if ( timecount <= 0  ){
  
        time( &rawt );
        localtime_r( &rawt, &tinfo);

        
        if ( oldmin != tinfo.tm_min && currDisplayScreen != STNSELECT ){
           oldmin = tinfo.tm_min; 
           
           showClock(tinfo.tm_hour, tinfo.tm_min, tinfo.tm_mday, tinfo.tm_mon, tinfo.tm_wday, tinfo.tm_year + 1900 );
           
           timecount = (1000/delaytime); 
        }
     }

     #ifdef USEOWM

     --weathercount;
     
     if ( weathercount <= 0  ){
        if( getWeather() ){
          weathercount = ((10*60*1000) /delaytime); // every 10 minutes
        }else{
          weathercount = ( 20 * 1000 ) / delaytime; 
        }
     }
     
     #endif  
          
     delay( delaytime );
  }
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
