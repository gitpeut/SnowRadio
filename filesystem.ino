#include <FS.h>
#include <FFat.h>
#include <SPIFFS.h>
#include <LITTLEFS.h>
#include <esp_littlefs.h>


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

     
