#include <FS.h>
#include <FFat.h>
#include <SPIFFS.h>
#include <LITTLEFS.h>
#include <esp_littlefs.h>

//--------------------------------------------------------------------------------
FBuf *findFBuf( String filename ){
  
  for( int i=0; i < FBUFSIZE; ++i ){
      if ( FBFiles[i].filename == filename ) return (&FBFiles[i]);   
  }

  return( NULL );
}
//--------------------------------------------------------------------------------
FBuf *newFBuf( String filename ){
  
  for( int i=0; i < FBUFSIZE; ++i ){
      if ( FBFiles[i].filename == "" ) return (&FBFiles[i]);   
  }

  return( NULL );
}


//--------------------------------------------------------------------------------
bool delFBuf( String filename ){
  FBuf *dbuf = findFBuf( filename );
  if ( dbuf ) {
    dbuf->filename = "";
    dbuf->size = 0;
    free( dbuf->buffer );
    dbuf->buffer = NULL;
    log_d("deleted buffer for %s", filename.c_str() );
    return( true );
  }
  log_d("could not find buffer for %s", filename.c_str() );
  return( false );
}
//--------------------------------------------------------------------------------
FBuf *addFBuf( String filename ){
  if( !psramFound() ){
    log_i( "No PSRAM, no file buffers");
    return NULL ; 
  }
  
  //log_i( "buffering %s", filename.c_str() ); 
  
  FBuf *fb = findFBuf( filename );
  if ( fb ){
    log_i( "%s already buffered", filename.c_str() );
    return fb;
  }

  fb = newFBuf( filename );
  if ( fb == NULL ) {
    log_e( "no file buffers left for %s", filename.c_str() );
    return NULL;
  }
     
  File sourcefile;
  size_t filesize;
  
  sourcefile  = RadioFS.open ( filename, FILE_READ ); 
  if ( !sourcefile ){ log_e( "Error opening %s for read ", filename.c_str()); return( NULL );}     

  filesize    = sourcefile.size();
  //log_i( "Opened %s for read, filesize %u \n",filename.c_str(), filesize);
    
  fb->buffer = (uint8_t *) ps_calloc( filesize ,1 );
  if ( ! fb->buffer ) {
    sourcefile.close();
    return (NULL);
  }
  
  size_t   totalbytesread=0;
  int      bytesread = 0;
  uint8_t *endpsbuffer = fb->buffer;
   
      while( sourcefile.available() ){   
        bytesread = sourcefile.read( endpsbuffer, (filesize - totalbytesread) );
        if ( bytesread < 0 ){ 
             sourcefile.close(); 
             free ( fb->buffer );
             
             log_e( "Read returned < 0 while reading file %s ", filename.c_str() ); 
             return( NULL );
          }
          
          totalbytesread += bytesread;
          endpsbuffer += bytesread;
      }
      sourcefile.close();
      
   fb->size       = endpsbuffer - fb->buffer;
   fb->filename   = filename;
       
   return ( fb );   

}

//---------------------------------------------------------------------------

void FBuffAll ( const char *path ){
    
    fs::File dir = RadioFS.open( path );     
    fs::File entry;
    
    if ( !dir.isDirectory() ){
      dir.close();
      return;
    }

    char  *fname;
    bool   isdir;

    while ( entry = dir.openNextFile() ){                

      fname = ps_strdup( entry.name() );
      isdir = entry.isDirectory();
      
      entry.close();
      
      if ( isdir  ){
         FBuffAll(fname);
      }else{
         if ( !String(fname).endsWith(".bin") && !String(fname).endsWith(".plg") && !String(fname).endsWith(".txt") && !String(fname).endsWith("stations.json")){
            //log_i( "add fbuf %s", fname );
            if ( strcasecmp( (fname + strlen(fname) - 4), ".bmp" ) ){
              addFBuf( String(fname) );
            }else{
              drawBmp( fname, 0, 0, NULL, false);
            }
         }else{
            //log_i( "%s not buffered, wrong type", fname );         
         }          
      }
      free(fname);
    }    
    dir.close();
}

/*-----------------------------------------------------------------*/
void showFBuf(AsyncWebServerRequest *request)
{
  String output = "{ \"bufferedfiles\" : [\r\n";
  uint32_t   foundcount = 0, totalsize=0;
  
  for ( int i=0; i < FBUFSIZE; ++i ){

    if ( FBFiles[i].size != 0 ){ 

      totalsize += FBFiles[i].size;
      
      if ( foundcount) output += ",\r\n";
      foundcount++;
      
      output += "   {\"filename\" : \"";
      output += FBFiles[i].filename; 
      output += "\"," ;
      output += "\"size\" : "; 
      output += FBFiles[i].size; 
      output += "}" ;
    }
  }   
  
  output += "],\r\n" ;
  output += "\"buffercount\" : " ;
  output += foundcount; 
  output += ",\r\n" ;
  output += "\"totalsize\" : " ;
  output += totalsize; 
  output += "}\r\n" ;
    
  request->send(200, "application/json;charset=UTF-8", output);
}

//-----------------------------------------------------

int syslog( char *message){
FILE *slog=NULL;
time_t now;
now = time(nullptr);
char  tijd[32];
char  filename[128];
struct stat buf;

sprintf( filename, "%s/syslog.txt", RadioMount);
slog = fopen( filename, "a");


if ( slog == NULL) {
  log_d("Couldn't open %s (errno %d) for append, trying for write", filename, errno );
  slog = fopen( filename, "w");
  if ( slog == NULL) {
    log_d("Couldn't open %s (errno %d) for write either", filename, errno );
    return(-1);
  }
}

int fd = fileno( slog );    
fstat(fd, &buf);

if ( buf.st_size > 20000 ){
  fclose( slog );
  log_i("remove syslog.txt, as it was larger than 20k");
  remove( filename );

  slog = fopen( filename, "w");
}


sprintf( tijd,"%s", asctime( localtime(&now)) );
tijd[ strlen(tijd) - 1 ] = 0;

fprintf( slog,"%s - %s\n", tijd, message);
fclose(slog);

return(0);
}

     
