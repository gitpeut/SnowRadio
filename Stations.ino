extern void broadcast_status();
extern bool noMeta; // in radio.ino
//-----------------------------------------------------

char * ps_strndup (const char *s, size_t n)
{
  size_t len = strnlen (s, n);
  char *newstring = (char *) gr_malloc (len + 1);
  if (newstring == NULL)return NULL;
  
  newstring[len] = '\0';
  return (char *) memcpy (newstring, s, len);
}
  
 
//-----------------------------------------------------

char * ps_strdup (const char *s)
{
  size_t len = strlen (s) + 1;
  void *newstring = gr_malloc (len);
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
                  save_stations();//hiero
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

stations = (Station *) gr_calloc( STATIONSSIZE,sizeof(Station) );

  read_stations();
 
}



//-----------------------------------------------------

int read_header( int stationIdx){
char  line[1024];
char  *s = line,*t, lastchar='x';
int   http_status=0;
bool  acceptrange=false;

Serial.printf("Reading header\n");

stationChunked = false;
stationClose   = true;      

for(;;){
  //read a line
  for(line[0] = 0, s = line, lastchar=0; s - line < 1024; ++s ){ 
    radioclient->read( (uint8_t *)s, 1 );
   
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

  if ( strncasecmp(line,"icy-metaint:", 12) == 0 ){
        for( s = line; !isdigit(*s); ++s);
        stationMetaInt = atoi( s );           
        log_d( "meta interval is %d", stationMetaInt);  
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
      sclient.setInsecure(); // open the door for mind control by Internet radio.
      radioclient = &sclient;
    }
 #endif
        
    if (!radioclient->connect( stations[stationIdx].host, stations[stationIdx].port) ) {
      Serial.println("Connection failed");
      return(1);
    }
    Serial.printf("Connected via %s, send GET\n",radioclient == &iclient? "http":"https");

   
    radioclient->setTimeout(1);

    int namelen = strlen( stations[stationIdx].path ) + strlen( stations[stationIdx].host );
    
    char *getstring = (char *)gr_calloc( namelen + 256, 1 );
    if ( getstring == NULL ) {
      Serial.print( "No memory to access station!") ;
      return( 666 );
    }

    sprintf( getstring,"GET %s HTTP/1.1\r\nHost: %s\r\nAgent: %s %s\r\n%sAccept: */*\r\nConnection: close\r\n\r\n",
               stations[stationIdx].path,
               stations[stationIdx].host,
               APNAME,
               APVERSION,
               noMeta==false?"Icy-MetaData: 1\r\n":"");

                  
    Serial.print( getstring );
           
    radioclient->print( getstring );
    free( getstring);  

    if ( noMeta ) noMeta = false;   
    return(0);
}

//-----------------------------------------------------

int stationsConnect(int stationIdx){
int i,j,rc;

    Serial.printf("Connect to station idx %d\n", stationIdx );
    if ( stations[stationIdx].status == 0 ) stationIdx = 0;
    contentsize = 0;
    
    for( j = 0; j < 4 ; ++j ){ 

      stationMetaInt = 0;
      
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

            // escape tight loop of gets 
            if( getStation() == stationIdx ){
              setStation( stationIdx, -1 );
            }else{
              return(5);
            }
            Serial.printf("Now start listening to %s\n", stations[stationIdx].name );
            tft_showstation(stationIdx);
            broadcast_status();
            return(0);
        }
        Serial.print(".");
        delay(10);
      } 
      
      if ( i == 200 ){
          Serial.println("didn't receive any data in 2 seconds");
          //radioclient->flush();
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
char  filename[128];
int   idx, readlen;

switch( volstat ){
  case 0:
    sprintf( filename, "%s/last_station.txt", RadioMount);
    break;
  case 1:
    sprintf( filename, "%s/last_volume.txt", RadioMount);
    break;
  case 2:
    sprintf( filename, "%s/last_mode.txt", RadioMount);
    break;
  case 3:
    sprintf( filename, "%s/last_tone.txt", RadioMount);
    break;
}

last = fopen( filename, "r"); 
if ( last == NULL) {
  Serial.printf("Couldn't open %s/last_%s.txt\n", RadioMount, volstat?"volume":"station" );
  
  return( volstat==1?65:0);
}
readlen = fread( buffer,1, 8, last );
buffer[readlen] = 0;
fclose(last);

idx = atoi( buffer);

log_d("get_last_volstat read %s from %s, toint is %d", buffer, filename,idx);
  
if ( volstat == 0 ){
  if ( idx <0 || idx >= stationCount ) idx = 0;
}

if ( volstat == 2 ){
    if ( idx >= 4 )idx = 0;
}

return( idx );
}

//-----------------------------------------------------

int save_last_volstat( int volstat){
FILE *last=NULL;
char  filename[128];

switch( volstat ){
  case 0:
    sprintf( filename, "%s/last_station.txt", RadioMount);
    break;
  case 1:
    sprintf( filename, "%s/last_volume.txt", RadioMount);
    break;
  case 2:
    sprintf( filename, "%s/last_mode.txt", RadioMount);
    break;
  case 3:
    sprintf( filename, "%s/last_tone.txt", RadioMount);
    break;
  
}

last = fopen( filename, "w");

if ( last == NULL) {
  log_e("Couldn't open %s\n", filename );
  return(-1);
}

switch( volstat ){
  case 0:
    fprintf(last, "%d", getStation());
    break;
  case 1:
    fprintf(last, "%d", getVolume() );
    break;
  case 2:
    log_d("writing %d to last mode", currDisplayScreen );
    fprintf(last, "%d", currDisplayScreen );
    break;
  case 3:
    fprintf(last, "%d", getTone() );
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

Serial.printf("Saving stations to %s/stations.json\n", RadioMount);

time(&ltime);
ctime_r( &ltime, timebuffer);
timebuffer[24] = 0;

char filename[128];
sprintf( filename, "%s/stations.json", RadioMount);

uit = fopen( filename, "w");
if ( uit == NULL) {
  Serial.printf("Couldn't open %s\n", filename);
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

Serial.printf("Saved stations to %s\n", filename);

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
char    *source;
int     stationidx=0, targetcount=0;
char    *current_station_name = NULL;
char    *playing_station_name = NULL;
int     newcurrent = -1;
int     newplaying = -1;

//log_d("current station = %d, playing station = %d", currentStation, playingStation);

if ( currentStation >= 0 ) current_station_name = ps_strdup( stations[currentStation].name );
if ( playingStation >= 0 ) playing_station_name = ps_strdup( stations[playingStation].name );
 
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
                                        stations[stationidx].name = ps_strndup( source, s-source);
                                        //Serial.printf("%d - %s\n", stationidx, stations[stationidx].name);
                                        if ( current_station_name != NULL ){
                                          if ( strcmp( stations[stationidx].name, current_station_name ) == 0 ) newcurrent = stationidx;
                                        }
                                        if ( playing_station_name != NULL ){
                                          if ( strcmp( stations[stationidx].name, playing_station_name ) == 0 ) newplaying = stationidx;
                                        }
                                        
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
                                Serial.printf("%d - %s %d\n", stationidx, stations[stationidx].name, stations[stationidx].protocol);
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

xSemaphoreTake( updateSemaphore, portMAX_DELAY);        
if ( newcurrent >= 0 ) currentStation = newcurrent;
if ( newplaying >= 0 ) playingStation = newplaying;
xSemaphoreGive( updateSemaphore);

if ( current_station_name != NULL )free(current_station_name); 
if ( playing_station_name != NULL )free(playing_station_name); 

return( stationidx );
}

//-----------------------------------------------------
int read_stations(){
FILE    *in=NULL;
char    *readBuffer;
size_t  read_result;
struct  stat sStat;
char    filename[128];

Serial.printf("Loading stations from %s/stations.json\n", RadioMount);

sprintf( filename, "%s/stations.json", RadioMount);

if( stat( filename,&sStat) < 0){
  Serial.printf("Couldn't find %s\n", filename);
  return(-3);
}

Serial.printf("Size of %s\t%ld bytes\n",filename, sStat.st_size);

readBuffer = (char *)gr_calloc( 1, sStat.st_size+4 );
if ( readBuffer == NULL ){
    Serial.printf("Error allocating memory for stations\n"); 
    return(-2);
}

in  = fopen( filename, "r" );
if ( in == NULL ) {
        Serial.printf("Error opening stations.json\n");         
        return(-1);
}

free_stations();

read_result = fread( readBuffer, 1, sStat.st_size, in );

fclose(in);

Serial.printf("Read %d bytes from %s", read_result, filename);
readBuffer[read_result] = 0;


fill_stations_from_file( readBuffer, read_result);

free( readBuffer );
return(0);
}

//-----------------------------------------------------
