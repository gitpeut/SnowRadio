

#ifdef USESSDP
/*-----------------------------------------------------------------*/
void  setupSSDP(){

 int  chipid; 

 Serial.printf("Starting SSDP...\n");
    
    #ifdef ESP8266
      chipid = ESP.getChipId();
    #endif
    #ifdef ESP32 
      chipid = ESP.getEfuseMac();
    #endif  
    
    SSDPDevice.setName( APNAME );
    SSDPDevice.setDeviceType("urn:schemas-upnp-org:device:BinaryLight:1");
    SSDPDevice.setSchemaURL("description.xml");
    SSDPDevice.setSerialNumber( chipid);
    SSDPDevice.setURL("/");
    SSDPDevice.setModelName( APNAME );
    SSDPDevice.setModelNumber( APNAME " " APVERSION );
    SSDPDevice.setManufacturer("Peut");
    SSDPDevice.setManufacturerURL("http://www.peut.org/");


// server is the globally defined webserver above

    server.on("/description.xml", HTTP_GET, [](){
    SSDPDevice.schema( server.client());     
    });


// don't forget to add 
//  SSDPDevice.handleClient(); 
//  in loop()
}
#endif
/*-----------------------------------------------------------------*/
void send_json_status()
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
    
  server.send(200, "application/json;charset=UTF-8", output);
 
}

//------------------------------------------------------------------

static const char serverIndex[] PROGMEM =
  R"(<html><head><style>
      input{background:#f1f1f1;border:0;padding:0px;display:block}
      h1{color: orange;font-size: 5vw;}
      body{background: lightgrey;font-family:sans-serif;font-size:24px;color:#777;}
      body { font-family: Arial, Helvetica, sans-serif;}
      form{background:#fff;max-width:400px;margin:75px auto;padding:30px;border-radius:5px;text-align:center;}
      .btn{border: 1px solid black;color:#000;padding:10px;font-size:18px;cursor:pointer;display:block;margin-left:160px;margin-top:40px;}
      </style>
      </head><body><h1>GeleRadio</h1>
                  <form method='POST' action='' enctype='multipart/form-data'>
                  <h2>update firmware</h2>
                  <input type='file' id=file-input name='update'>
                  <input type='submit' class=btn value='Update' >
               </form>
         </body></html>)";
static const char successResponse[] PROGMEM = 
  "<META http-equiv=\"refresh\" content=\"15;URL=/\">Update Success! Rebooting...\n";
static const char failResponse[] PROGMEM = 
  "<META http-equiv=\"refresh\" content=\"15;URL=/\">Update FAILED, rebooting...\n";

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

void handleDel(){
  int   i, return_status = 400,rc;
  char  message[80];
  
  if ( !server.hasArg("name") || !server.hasArg("index")   ){
        sprintf( message, "Error: del host needs ?name=xx&index=xx" );
        server.send( return_status, "text/plain", message);
        return;
  }

 
  rc = del_station(  (char *)server.arg("name").c_str(), 
                     server.arg("index").toInt() ); 

  if ( rc ){
        sprintf( message, "Error: Station \"%s\" with index %d not found", server.arg("name").c_str(), server.arg("index").toInt()  );
        Serial.println( message );

  }else{
    save_stations();                
    return_status = 200;
    read_stations();
    sprintf( message,"Deleted station %s", server.arg("name") );      
  }
  
  server.send( return_status, "text/plain", message);
    
}
//------------------------------------------------------------------

void handleAdd(){
  int   return_status = 400,rc;
  char  message[80];
  
  if ( !server.hasArg("name") || !server.hasArg("host") || !server.hasArg("path") || !server.hasArg("port") || !server.hasArg("idx") || !server.hasArg("protocol") ){
        sprintf( message, "Error: add host needs ?host=xx&name=xx&path=xx&port=xx&idx=xx&protocol=xxx" );
        server.send( return_status, "text/plain", message);
        return;
  }

  if ( server.arg("idx").toInt() == -1 ){
      rc = add_station(  (char *)server.arg("name").c_str(), 
                server.arg("protocol").toInt(),
                (char *)server.arg("host").c_str(), 
                (char *)server.arg("path").c_str(), 
                server.arg("port").toInt() ); 
                 
  }else{
      int idx = server.arg("idx").toInt();

      rc = change_station(  (char *)server.arg("name").c_str(), 
                server.arg("protocol").toInt(), 
                (char *)server.arg("host").c_str(), 
                (char *)server.arg("path").c_str(), 
                server.arg("port").toInt(),
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
  
  server.send( return_status, "text/plain", message);
    
}

//------------------------------------------------------------------

void handleSet(){
  int   return_status = 400;
  char  message[80];
  
  if ( server.hasArg("volume") ){
      int desired_volume = server.arg("volume").toInt();
      if ( desired_volume >= 0 && desired_volume <= 100 ){
        return_status = 200;
        sprintf( message,"Volume set to %d", desired_volume );
        setVolume( desired_volume );
      }else{
        sprintf( message,"Volume requested %d, but value must be between 0 and 100", desired_volume);
      }
  } 
  if ( server.hasArg("station") ){
      int desired_station = server.arg("station").toInt();
      if ( desired_station < STATIONSSIZE && desired_station >= 0 && stations[ desired_station ].status == 1 ){
        return_status = 200;
        if( desired_station != getStation() ) setStation( desired_station, -1 );
        sprintf( message,"Station set to %d, %s", desired_station, stations [ desired_station].name );      
      }else{
        sprintf( message,"Station %d does not exist", desired_station);
      }
  }  
  
  server.send( return_status, "text/plain", message);
    
}
//------------------------------------------------------------------

int dossdp=0;
int uploadThreshold=0;

void handleWebServer( void *param ){

  
  Serial.printf("WebServer running on core %d\n", xPortGetCoreID()); 
//
//https://github.com/espressif/arduino-esp32/issues/595
//
  
  TIMERG0.wdt_wprotect=TIMG_WDT_WKEY_VALUE;
  TIMERG0.wdt_feed=1;
  TIMERG0.wdt_wprotect=0;

  #ifdef USESSDP
    setupSSDP();
  #else
    MDNS.begin( APNAME);
  #endif  
  server.on("/", []() {
    handleFileRead( "/index.html" );
  });

  server.on("/set", handleSet );
  server.on("/add", handleAdd );
  server.on("/del", handleDel );

  server.on("/upload", HTTP_GET, []() {
        if(!handleFileRead("/upload.html")) server.send(404, "text/plain", "FileNotFound");
  });

  //server.on("/upload", HTTP_POST, handleFileUpload);
   
  server.on("/upload", HTTP_POST, [](){ handleFileRead("/upload.html"); }, handleFileUpload);

  server.on("/reset", HTTP_GET, []() {
        server.send(200, "text/plain", "Doej!");
        delay(20);
        ESP.restart();
  });
  
  server.on( "/list",handleFileList);
  server.on( "/delete",handleFileDelete);
  server.on ("/status", send_json_status );
  server.on ("/settings", handleSettings );

   server.on("/update", HTTP_GET, []() {
    server.sendHeader("Connection", "close");
    server.send(200, "text/html", serverIndex);
  });
  //handling uploading firmware file 
  server.on("/update", HTTP_POST, []() {
    server.sendHeader("Connection", "close");
    server.send((Update.hasError())?400:200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
    delay(20);
    ESP.restart();
  }, []() {
    HTTPUpload& upload = server.upload();
    if (upload.status == UPLOAD_FILE_START) {
      
       
       dossdp = -30000; 
       xSemaphoreTake( updateSemaphore, portMAX_DELAY);
       uploadThreshold = 10000;
       tft_ShowUpload( "firmware" );
        
      Serial.printf("Update: %s\n", upload.filename.c_str());
      syslog("Installing new firmware through webupload" );
      
      if (!Update.begin(UPDATE_SIZE_UNKNOWN)) { //start with max available size
        Update.printError(Serial);
      }
    } else if (upload.status == UPLOAD_FILE_WRITE) {
      // flashing firmware to ESP
      delay(1);
      if ( upload.totalSize > uploadThreshold ){
        if ( (upload.totalSize/10000) % 10 == 0 )Serial.printf("%d ", upload.totalSize/10000 );
        tft_uploadProgress( upload.totalSize/10000  );
        uploadThreshold += 10000;
      }
      if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
        Update.printError(Serial);
      }
    } else if (upload.status == UPLOAD_FILE_END) {
       xSemaphoreGive( updateSemaphore);
      if (Update.end(true)) { //true to set the size to the current progress
        tft_uploadEnd( "success");
        Serial.printf("\nUpdate Success: written %u bytes.\nRebooting...\n", upload.totalSize);
        server.send(200, "text/html", successResponse);
        
      } else {
        tft_uploadEnd( "failed");
        Update.printError(Serial);
        Serial.printf("\n");
        server.send(500, "text/html", failResponse );
        
      }
    }
  });
  
  server.onNotFound([](){
    server.sendHeader("Connection", "close");
    if(!handleFileRead(server.uri()))
      server.send(404, "text/plain", "FileNotFound");
  });
  
 server.begin();
 #ifndef USESSDP
 MDNS.addService("http", "tcp", 80);
 #endif
 
if ( param == NULL ){
  
  dossdp= 0;
  
  int timecount=0, oldmin=987;
  time_t  rawt;
  struct tm tinfo;
  
  while(1){
   server.handleClient();
    
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
  
   if ( millis() > timecount ){

      time( &rawt );
      localtime_r( &rawt, &tinfo);
       
      if ( oldmin != tinfo.tm_min ){
         oldmin = tinfo.tm_min; 
         showClock(tinfo.tm_hour, tinfo.tm_min);
         //showBattery();
      }
   
      timecount = millis() + (10*1000);
   }
   
   delay(20);
  }

} 
   
}
//------------------------------------------------------------------

void setupWebServer(){

    xTaskCreatePinnedToCore( 
         handleWebServer,                                      // Task to handle special functions.
         "WebServer",                                            // name of task.
         1024*8,                                                 // Stack size of task
         NULL,                                                 // parameter of the task
         WEBSERVERTASKPRIO,                                    // priority of the task
         &webserverTask,                                        // Task handle to keep track of created task 
         WEBCORE );                                                 //core to run it on


}
