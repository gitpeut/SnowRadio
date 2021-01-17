
//-----------------------------------------------------

char * ps_strndup (const char *s, size_t n)
{
  size_t len = strnlen (s, n);
  char *newstring = (char *) ps_malloc (len + 1);
  if (newstring == NULL)return NULL;
  
  newstring[len] = '\0';
  return (char *) memcpy (newstring, s, len);
}
  
 
//-----------------------------------------------------

char * ps_strdup (const char *s)
{
  size_t len = strlen (s) + 1;
  void *newstring = ps_malloc (len);
  if (newstring == NULL)return NULL;
  return (char *) memcpy (newstring, s, len);
}
//-----------------------------------------------------

int change_station( char *name, int protocol, char *host, char* path, int port, int idx ){


if ( stations[idx].status == 1 ){

     Serial.printf( "Changing station %d to : name %s, h %s p %d path %s\n", idx,name,  host, port, path);
     
     free( stations[idx].name ); 
     free( stations[idx].host ); 
     free( stations[idx].path ); 
     
     stations[idx].name =  ps_strdup( name);
     stations[idx].host =  ps_strdup( host);
     stations[idx].path =  ps_strdup( path);
     stations[idx].protocol = protocol;
     stations[idx].port = port;
     stations[idx].position = 0;
     
     
}else{
  Serial.printf( "station %d not changed\n",idx);
}


return(0);
}

//-----------------------------------------------------

int add_station( char *name, int protocol, char *host, char* path, int port ){
int i;

for ( i = 0; i< STATIONSSIZE; ++i ){
        if ( stations[i].status == 0 ){
                stations[i].name =  ps_strdup( name);
                stations[i].host =  ps_strdup( host);
                stations[i].path =  ps_strdup( path);
                stations[i].port = port;
                stations[i].protocol = protocol;
                stations[i].position = 0;
                stations[i].status = 1;
                break;
        }
}


if ( i == STATIONSSIZE ) return(1);
stationCount++;
return(0);
}

//-----------------------------------------------------

int del_station( char *name, int index ){
int i;

Serial.printf("Trying to delete station [%s], index %d]\n", name, index);

for ( i = 0; i< STATIONSSIZE; ++i ){

        if ( stations[i].name != NULL ){
          Serial.printf("Found station [%s], i %d]\n", stations[i].name, i);
        }
        
        if ( i == index && stations[i].status  ){
                if ( strcmp( stations[i].name, name )  == 0 ){
                  stations[i].status = 0;
                  break;
                }
        }
}
if ( i == STATIONSSIZE ) return(1);
Serial.printf("Deleted station [%s], index %d]\n", name, index);
stationCount--;
return(0);
}
//-----------------------------------------------------

void stationsInit(){

stations = (Station *) ps_calloc( STATIONSSIZE,sizeof(Station) );

/*
  add_station("NPO Radio 1","icecast.omroep.nl","/radio1-bb-mp3",80);
  add_station("NPO Radio 2","icecast.omroep.nl","/radio2-bb-mp3",80);
  
  add_station("NPO 3fm","icecast.omroep.nl","/3fm-bb-mp3",80);
  add_station("NPO Radio 4","icecast.omroep.nl","/radio4-bb-mp3",80);
  add_station("NPO Radio 5","icecast.omroep.nl","/radio5-bb-mp3",80);
  add_station("NPO Radio 6","icecast.omroep.nl","/radio6-bb-mp3",80);
  add_station("Cncrt Jazz","streams.greenhost.nl","/jazz",8080);
  add_station("Cncrt Hardbop","streams.greenhost.nl","/hardbop",8080);
  add_station("BBC 1","bbcmedia.ic.llnwd.net","/stream/bbcmedia_radio1_mf_q",80);
  add_station("BBC 2","bbcmedia.ic.llnwd.net","/stream/bbcmedia_radio2_mf_q",80);
  add_station("BBC 3","bbcmedia.ic.llnwd.net","/stream/bbcmedia_radio3_mf_q",80);
  add_station("BBC 4","bbcmedia.ic.llnwd.net","/stream/bbcmedia_radio4fm_mf_q",80);
  add_station("BBC 1xtra","bbcmedia.ic.llnwd.net","/stream/bbcmedia_radio1xtra_mf_p",80);
*/
  read_stations();
 
}



//-----------------------------------------------------

int read_header( int stationIdx){
char  line[1024];
char  *s = line,*t, lastchar='x';
int   rc=0,http_status=0;
bool  acceptrange=false;

Serial.printf("Reading header\n");

stationChunked = false;
stationClose   = true;      

for(;;){
  //read a line
  for(line[0] = 0, s = line, lastchar=0; s - line < 1024; ++s ){ 
    rc = radioclient->read( (uint8_t *)s, 1 );
   
    if ( *s == '\n' && lastchar == '\r' ){
      *(s-1) = 0;
      break;
    }
    lastchar = *s;
  }
  Serial.printf("%s\n", line);
  if ( line[0] == 0 ){
    return( http_status); // end of header
  }

  //get HTTP status
  if ( strncasecmp( line, "HTTP", 4) == 0 || strncasecmp( line, "ICY ", 4) == 0  ){
      
      for( s = line; *s != ' '; ++s);
      ++s; t = s;
      for( ; *s != ' '; ++s);
      *s = 0;
      http_status = atoi( t );
      Serial.printf("http_status %d\n", http_status );
      //if ( http_status == 200 )return(http_status); 
      //if( http_status >= 400 ) return(http_status);            
  }
  
  if ( strncasecmp(line,"Transfer-Encoding:", 18) == 0 ){
        for( s = line; *s != ' '; ++s);
        ++s;
        if (  0 == strcmp(s, "chunked") ){
            stationChunked = true;      
        }       
  }

  
  if ( strncasecmp(line,"Accept-Ranges: bytes",20) == 0 ){
        acceptrange = true;           
  }

  if ( strncasecmp(line,"Content-Length:", 15) == 0 && acceptrange){
        for( s = line; !isdigit(*s); ++s);
        //contentsize = atoi( s );           
  }


  if ( strncasecmp( line, "Connection:",11) == 0 ){
        for( s = line; *s != ' '; ++s);
        ++s;
        if (  0 == strncasecmp(s, "keep-alive", 10) ){
            stationClose = false;      
        }       
  }
  
  if ( strncasecmp( line, "Location:", 9) == 0 ){
      for( s = line; *s != ' '; ++s);
      ++s;
      stations[ stationIdx].protocol = 0; 
      if ( strncasecmp( s, "https:", 6) == 0) stations[ stationIdx].protocol = 1;
           
      for( ; *s != '/'; ++s);
      ++s;++s;     
      t = s;
      for( ; *s != '/' && *s != ':'; ++s);
      lastchar = *s;
      *s = 0;
      Serial.printf("host %s\n", t );
      
      free( stations[ stationIdx].host );
      stations[stationIdx].host = ps_strdup( t );

      Serial.printf("hostidx %s\n", stations[stationIdx].host );
      if( lastchar == ':' ){
        ++s;
        t = s;  
        for( ; *s != '/'; ++s);
        *s = 0;      
        stations[ stationIdx].port = atoi( t );
        *s = '/';
      }else if( lastchar == '/' ){
        *s = '/';
      }  
    
      free( stations[ stationIdx].path );
      stations[ stationIdx].path = ps_strdup( s );

      Serial.printf("Redirecting to %s host %s, port %d, path %s\n", stations[ stationIdx].protocol?"https:":"http:",stations[ stationIdx].host, stations[ stationIdx].port, stations[ stationIdx].path );
      return( http_status );
  }

}

return(0);
}


//-----------------------------------------------------

int justConnect( int stationIdx ){

    Serial.println("Connect");
   
    if ( radioclient->connected() ){
      //radioclient->flush();
      radioclient->stop();
    }
    
    Serial.print("connecting to ");  Serial.println(stations[stationIdx].host); 

    
    
    radioclient = &iclient;

 #ifdef USETLS
    if ( stations[stationIdx].protocol == 1 ){
      // TLS by not setting //radioclient -> setCACert(rootCACertificate); no certificates are checked.
      radioclient = &sclient;
    }
 #endif
        
    if (!radioclient->connect( stations[stationIdx].host, stations[stationIdx].port) ) {
      Serial.println("Connection failed");
      return(1);
    }
    Serial.printf("Connected via %s, send GET\n",radioclient == &iclient? "http":"https");

   
    radioclient->setTimeout(1);

    //if ( contentsize == 0  ){
          Serial.print(String("GET ") + stations[stationIdx].path + " HTTP/1.1\r\n" +
                  "Host: " + stations[stationIdx].host + "\r\n" + 
                  "User-Agent: "+  APNAME +" "+ APVERSION + "\r\n" +
                  "Accept: */*" + "\r\n" +                  
                  "Connection: close\r\n\r\n");

      
      
      radioclient->print(String("GET ") + stations[stationIdx].path + " HTTP/1.1\r\n" +
                  "Host: " + stations[stationIdx].host + "\r\n" + 
                  "User-Agent: "+  APNAME +" "+ APVERSION + "\r\n" +
                  "Accept: */*" + "\r\n" +                  
                  "Connection: close\r\n\r\n");
    // }else{
    //  Serial.print(String("GET ") + stations[stationIdx].path + " HTTP/1.1\r\n" +
    //              "Host: " + stations[stationIdx].host + "\r\n" + 
    //              "User-Agent: "+  APNAME +" "+ APVERSION + "\r\n" +
    //
    //              //"Accept: */*" + "\r\n" +                  
    //              "Range: bytes=" + stations[stationIdx].position + "-" + (contentsize - 1 - stations[stationIdx].position) + "\r\n" +
    //              "Connection: close\r\n\r\n");     
    //
    //  radioclient->print(String("GET ") + stations[stationIdx].path + " HTTP/1.1\r\n" +
    //              "Host: " + stations[stationIdx].host + "\r\n" + 
    //              "User-Agent: "+  APNAME +" "+ APVERSION + "\r\n" +
    //
    //              //"Accept: */*" + "\r\n" +                  
    //              "Range: bytes=" + stations[stationIdx].position + "-" + (contentsize - 1 - stations[stationIdx].position) + "\r\n" +
    //              "Connection: close\r\n\r\n");     
    //}
    
    
    return(0);
}

//-----------------------------------------------------

int stationsConnect(int stationIdx){
int i,j,rc;

    if ( stations[stationIdx].status == 0 ) stationIdx = 0;
    contentsize = 0;
    
    for( j = 0; j < 4 ; ++j ){ 
      
      rc = justConnect( stationIdx );
      if ( rc != 0) {
        Serial.printf( "Error, justConnect returned %d\n", rc);
        return(rc);
      }else{
        Serial.printf( "justConnect ok, returned %d\n", rc);
       
      }
      
      for( i = 0; i < 200; ++i ){
       if ( radioclient->available() > 0 ){
            rc = read_header( stationIdx );   
            Serial.printf( "read_header returned %d, %s , %s\n", rc, stationChunked?"chunked":"stream", stationClose?"HTTP Close":"HTTP Keep-alive"  );
        
            if ( rc > 300 && rc < 309 ){
              Serial.printf( "Retry connect but now to %s\n", stations[stationIdx].host);
              break;
            }

            if ( contentsize != 0 && rc == 200 ){
              if ( stations[stationIdx].position >= contentsize -1 )stations[stationIdx].position = 0; 
              break; 
            }
            
            if ( rc >= 400 && rc < 409 ){
               
                return(400);
            }
            
            if ( rc != 200 && rc != 206 ) return(4);

            setStation( stationIdx, -1 );
            Serial.printf("Now start listening to %s\n", stations[stationIdx].name );
            tft_showstation(stationIdx);
            return(0);
        }
        Serial.print(".");
        delay(10);
      } 
      
      if ( i == 200 ){
          Serial.println("didn't receive any data in 2 seconds");
          radioclient->flush();
          radioclient->stop();
          return(2);
      }
      Serial.printf("Next Connect loop\n");
    }
    return(3);
}

//-----------------------------------------------------
int get_last_volstat(int volstat){
FILE *last=NULL;
char  buffer[8];
int   idx;

switch( volstat ){
  case 0:
    last = fopen( "/spiffs/last_station.txt", "r");
    break;
  case 1:
    last = fopen( "/spiffs/last_volume.txt", "r");
    break;
}
 
if ( last == NULL) {
  Serial.printf("Couldn't open /spiffs/last_%s.txt\n", volstat?"volume":"station" );
  
  return( volstat?65:0);
}
fread( buffer,1, 8, last );
fclose(last);

idx = atoi( buffer);

if ( volstat == 0 ){
  if ( idx <0 || idx >= stationCount ) idx = 0;
}

return( idx );
}

//-----------------------------------------------------

int save_last_volstat( int volstat){
FILE *last=NULL;

switch( volstat ){
  case 0:
    last = fopen( "/spiffs/last_station.txt", "w");
    break;
  case 1:
    last = fopen( "/spiffs/last_volume.txt", "w");
    break;
}

if ( last == NULL) {
  Serial.printf("Couldn't open /last_%s.txt\n", volstat?"volume":"station" );
  return(-1);
}

switch( volstat ){
  case 0:
    fprintf(last, "%d", getStation());
    break;
  case 1:
    fprintf(last, "%d", getVolume() );
    break;
}

fclose(last);
return(0);
}
//-----------------------------------------------------

int save_stations(){
FILE    *uit;
int     i, stations_written = 0;;
time_t  ltime;
char    timebuffer[32];

Serial.printf("Saving stations to /spiffs/stations.json\n");

time(&ltime);
ctime_r( &ltime, timebuffer);
timebuffer[24] = 0;


uit = fopen( "/spiffs/stations.json", "w");
if ( uit == NULL) {
  Serial.printf("Couldn't open /spiffs/stations.json\n");
  return(-1);
}

fprintf(uit, "{ \"date\" : \"%s\", \"stations\" : [", timebuffer );

for ( i = 0; i< STATIONSSIZE; ++i ){
        if ( stations[i].status ){
                if ( stations_written ) fprintf(uit,",");
                fprintf(uit, "{ \"name\" : \"%s\", \"protocol\" : %d,\"host\" : \"%s\", \"path\" : \"%s\", \"port\" : %d, \"position\" : %d }",
                                stations[i].name,stations[i].protocol, stations[i].host,stations[i].path, stations[i].port, stations[i].position);
                stations_written++;
        }
}

fprintf(uit,"]}" );
fclose(uit);

Serial.printf("Saved stations to /spiffs/stations.json\n");

return(0);
}

//-----------------------------------------------------
void free_stations(){
int i;

for ( i = 0; i< STATIONSSIZE; ++i ){
        if ( stations[i].status == 1 ){
                free( stations[i].name);
                free( stations[i].host);
                free( stations[i].path);
                stations[i].status = 0;
                stations[i].protocol = 0;
                stations[i].position = 0;
        }
}

}


//-----------------------------------------------------
int     fill_stations_from_file( char *fileBuffer, size_t bufferlen){
char    *s=fileBuffer;
int     mode=0;
char    searchstring[32];
char    *target, *source;
int     stationidx=0, targetcount=0;

sprintf( searchstring,"\"name\"");
Serial.println("Loading stations");

for(; *s; ++s ){
        switch( mode ){
        case 0: // find name
                if ( ! strncmp( s, searchstring, strlen(searchstring)  )) {
                        s += strlen( searchstring );
                        mode = 1;

                }
                break;
        case 1: // find start of value
                if ( *s == '\"' )mode= 2;
                if ( isdigit(*s)) {
                        source = s;
                        mode = 4;
                }
                break;
        case 2:
                source = s;
                mode = 3;
                break;
        case 3:
                 if ( *s == '\"'  ){
                        mode = 0;
                        switch( targetcount ){
                                case 0:
                                        stations[stationidx].name = strndup( source, s-source);
                                        Serial.printf("%d - %s\n", stationidx, stations[stationidx].name);
                                        sprintf( searchstring,"\"protocol\"");
                                        targetcount++;
                                        break;
                                case 2:
                                        stations[stationidx].host = strndup( source, s-source);
                                        sprintf( searchstring,"\"path\"");
                                        targetcount++;
                                        break;
                                case 3:
                                        stations[stationidx].path = strndup( source, s-source);
                                        sprintf( searchstring,"\"port\"");
                                        targetcount++;
                                        break;
                                }

                 }
                 break;
         case 4: // number
                 if (! isdigit( *s )){

                        *s = 0;
                        int value = atoi ( source );
                        switch( targetcount ){
                        case 1:
                                stations[stationidx].protocol = value;
                                sprintf( searchstring,"\"host\"");
                                targetcount++;
                                mode = 0;
                                break;
                        case 4:
                                stations[stationidx].port = value;
                                stations[stationidx].status = 1;
                                sprintf( searchstring,"\"position\"");
                                targetcount++;
                                mode = 0;
                                break;
                        case 5:
                                stations[stationidx].position = value;
                                targetcount = 0;
                                stationidx++;
                                sprintf( searchstring,"\"name\"");
                                mode = 0;
                                break;
                        }
                 }
                 break;
        }
}



Serial.printf("\nFound %d stations\n", stationidx);
stationCount = stationidx;

}

//-----------------------------------------------------
int read_stations(){
FILE    *in=NULL;
char    *readBuffer;
size_t  read_result;
struct stat sStat;

Serial.printf("Loading stations fromm /spiffs/stations.json\n");

if( stat("/spiffs/stations.json",&sStat) < 0){
  Serial.printf("Couldn't find /spiffs/stations.json\n");
  return(-3);
}

Serial.printf("Size of /spiffs/stations.json\t%d bytes\n",sStat.st_size);

readBuffer = (char *)ps_calloc( 1, sStat.st_size+4 );
if ( readBuffer == NULL ){
    return(-2);
}

in  = fopen( "/spiffs/stations.json", "r" );
if ( in == NULL ) {
        
        return(-1);
}

free_stations();

read_result = fread( readBuffer, 1, sStat.st_size, in );

fclose(in);

Serial.printf("Read %d bytes from /spiffs/stations.json", read_result);
readBuffer[read_result] = 0;


fill_stations_from_file( readBuffer, read_result);

free( readBuffer );
return(0);
}

//-----------------------------------------------------
