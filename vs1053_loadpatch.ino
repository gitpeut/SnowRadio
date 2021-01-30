

#undef TESTREAD

#ifdef TESTREAD
#include "vs1053b-patches.plg" 
#endif

// allows for unedited raw .plg files to be uploaded to SPIFFS
// and to be read and patch VS1053. After the .plg file has been 
// read, a .plg.bin is written and the plg file is deleted.
// if a new .plg file isplaced on SPIFFS this new plg file is used 
// to patch VS1053.

size_t skiplast=0;
//-----------------------------------------------------

enum patchMode{
    pFindPlugin,
    pCheckPlugin,
    pFind0x,
    pFindx,
    pCopyValue,
    pComment,
    pIsComment,
    pFindEndStarComment,
    pEndStarComment,
    pFindEndSlashComment
};

//-----------------------------------------------------
// requires write_register to be public in the VS1053 
// class.

void WriteVS10xxRegister( unsigned short addr, unsigned short value){
    //printf( "%04x %04x\n", addr,value);
    vs1053player->write_register((uint8_t)addr, (uint16_t)value);
}

//-----------------------------------------------------
int VS1053_rename( const char *oldname, const char *newname){
    return( rename( oldname, newname ) );
}
//-----------------------------------------------------
int VS1053_file_size( const char *filename){
struct stat buf;
FILE *binfile;

    binfile = fopen( filename, "rb");
    
    if ( binfile == NULL ) {
      Serial.printf("Couldn't open %s for read\n", filename );
      return(-1); 
    }
    
    int fd = fileno( binfile);    
    fstat(fd, &buf);
    fclose(binfile);

return( buf.st_size );    

}


//-----------------------------------------------------------------------
// pasre RLE values in bin file and write to VS1053 

void write_VS1553_registers( unsigned short *pluginr, size_t valuecount){
    
    size_t i = 0, shortcount=0;
    unsigned short addr, n, val;

    while (i< valuecount - skiplast ) {
        addr = pluginr[i++];
        n = pluginr[i++];
        if (n & 0x8000U) { // RLE run, replicate n samples
          n &= 0x7FFF;
          val = pluginr[i++];
          while (n--) {
            WriteVS10xxRegister(addr, val);
            shortcount++; shortcount++;
          }
        } else {           // Copy run, copy n samples 
          while (n--) {
            val = pluginr[i++];
            WriteVS10xxRegister(addr, val);
            shortcount++; shortcount++;
          }
        }
    }

    Serial.printf( "VS1053 found %u values, wrote %u shorts to VS1053\n", valuecount, shortcount);
    
}

//--------------------------------------------------------------------
// read saved bin file and write registers to vs1053

int read_VS1053_bin( const char *bin_filename){
size_t  size, readnumber, shortsize;    
FILE *binfile;
unsigned short *pluginv; 

   
   
    size = VS1053_file_size( bin_filename );
    if ( size < 0 ) return( 2);
    
    Serial.printf("reading binfile %s of %u bytes\n", bin_filename, size);  
    
    shortsize = (size / sizeof( unsigned short ));
    
    binfile = fopen( bin_filename, "rb");
    
    if ( binfile == NULL ) {
      Serial.printf("Couldn't open %s for read\n", bin_filename );
      return(1); 
    }
    
    pluginv = (unsigned short *)gr_calloc( shortsize , sizeof( unsigned short ) );
    if ( ! pluginv ) {
        Serial.printf( "no memory for patch, abort\n");
        return(2);
    }
    
    for( readnumber = 0; readnumber < shortsize; ){
        readnumber += fread( (pluginv+readnumber) , sizeof( unsigned short ) , 1 , binfile);
        if ( feof( binfile) ) break;
    }   
 
    if ( readnumber != shortsize ){
    
        Serial.printf( "Error reading binfile, should read %llu, read %llu\n", shortsize, readnumber);
        return(3);
    }
    
    write_VS1553_registers( pluginv, shortsize);
    
    free( pluginv );
    return(0);  
}

//--------------------------------------------------------------------

int write_VS1053_binfile( unsigned short *pluginv, size_t valuecount, const char *bin_filename ){
FILE *binfile;

    binfile = fopen( bin_filename, "w");
    
    if ( binfile == NULL ) {
      Serial.printf("Couldn't open %s for write\n", bin_filename );
      return(1); 
    }

    for( size_t i=0; i < valuecount;++i ){
        fwrite( &pluginv[i], sizeof( unsigned short ),1,binfile); 
    }
    
    fclose( binfile );
    return(0);
}

//--------------------------------------------------------------------
// good enough state machine to parse plg files
//
int read_VS1053_plg( const char *filename ){
FILE  *plgfile=NULL;
char  c, *v, value[64]="";
int   mode=pFindPlugin;
size_t valuecount=0;
int   actionMode=0;
unsigned short *pluginv=NULL;  
  

for ( actionMode = 0; actionMode < 2; ++actionMode ){

    plgfile = fopen( filename, "r");
    if ( plgfile == NULL ) {
      Serial.printf("Couldn't open %s for read\n", filename );
      return(1); 
    }

    v = value;

    while ( fread(&c, sizeof( unsigned char ) ,1, plgfile)){
        switch( mode ){
            case pFindPlugin:
                if ( c == 'S' ){
                   mode = pCheckPlugin;
                   *v = c; ++v;                
                }
                break;
            case pCheckPlugin:
                if ( isalpha( c ) || c == '_'  ){
                    *v = c; ++v; 
                }else{
                    *v = 0;
                    if ( strcmp(value,"SKIP_PLUGIN_VARNAME") == 0 ){
                       mode = pFind0x;
                    }else{
                       mode = pFindPlugin; 
                    }
                    v = value;
                    *v = 0;
                } 
                break;
            case pFind0x:
                if ( c == '/' && mode < pComment ) {
                    mode = pComment;
                    break;
                }
                if ( c == '0' )mode = pFindx;
                break;
            case pFindx:
                mode = pFind0x;
                if ( c == 'x'){
                    mode = pCopyValue;
                    *v = '0'; ++v;    
                    *v = 'x'; ++v;    
                }
                break;
            case pCopyValue:
                if ( isxdigit( c ) ){
                   *v = c; ++v; 
                }else{
                    *v = 0;
                    if ( actionMode ){
                        pluginv[ valuecount ] =  (unsigned short) strtoul( value, NULL, 0) ;
                    }
                    valuecount++;
                    mode = pFind0x;
                    //cout << "Pushed " << value << " to pluginv" << std::endl;
                    v   = value;
                    *v  = 0;
                }
                break;
            case pComment:
                mode = pIsComment;
                break;
            case pIsComment:
                if ( c != '*' && c != '/' ) mode = pFind0x;
                if ( c == '*' ) mode = pFindEndStarComment;
                if ( c == '/' ) mode = pFindEndSlashComment;
                break;
            case pFindEndStarComment:
                if ( c == '*') mode=pEndStarComment;
                break;
            case pEndStarComment:
                mode = pFindEndStarComment;
                if ( c == '/')mode=pFind0x;
                break;
            case pFindEndSlashComment:
                if ( c == '\n' ) mode=pFind0x;
                break;
            default:
                return(3);
                break;
        }     

     }

    fclose( plgfile);
  
    
    if ( 0 == actionMode ){
        Serial.printf("Closed file after having read %u values\n", valuecount);
        Serial.printf("Allocating %u bytes of memory for array \n", valuecount * sizeof(unsigned short ) );
        
        pluginv = (unsigned short *)gr_calloc( valuecount, sizeof( unsigned short ) );
        if ( ! pluginv ) {
            Serial.printf( "no memory for patch, abort\n");
            return(2);
        }
        
        Serial.printf("Done, read again\n");
        valuecount=0;
        v = value;
        *v = 0;
        mode = pFindPlugin;
        //break;
    }

 
}    
    char bin_filename[80];
    sprintf( bin_filename, "%s.bin", filename );
    if ( 0 != write_VS1053_binfile( pluginv, valuecount, bin_filename ) ){
       remove( bin_filename );        
    }else{
       remove ( filename ); 
    }

    write_VS1553_registers( pluginv, valuecount);
    
    free( pluginv );
    return(0);  
}


#ifdef TESTREAD
void compare_plugin( unsigned short *pluginv,  size_t valuecount ){
int plugidx = 0, difcount=0;    
    for ( pluginidx = 0; pluginidx < PLUGIN_SIZE; ++pluginidx ){ 
        if (  pluginv [ plugidx ] != plugin [ plugidx ] ) {
            difcount++;
            cout << "Difference " << plugidx << " pluginv " << pluginv[ plugidx] << " plugin " << plugin[ plugidx] << std::endl; 
        }
        ++plugidx;
    }
    if ( difcount ){
        cout << "Found "<<difcount << "differences" << std::endl;
    }else{
        cout << "Found no differences" << std::endl;
    }

}
#endif


void patch_VS1053( const char *filename, size_t skip_last_bytes){
int rc;
char    bin_filename[80];

// if plg file exists read it, convert it to bin, rename it and apply included patches
// plg file does not exist, look for ....plg.bin, red it amd apply patches.
// new patches can be installed by placing a new .plg file
    Serial.printf( "Loading patch/plugin file %s\n", filename);
    
    skiplast = skip_last_bytes;
    
    rc = read_VS1053_plg( filename );
    if ( rc == 1){ // plg file not available   
       sprintf( bin_filename,"%s.bin", filename);
       rc = read_VS1053_bin( bin_filename ); 
       if( rc ) printf( "applying patches to VS1053 failed\n");
    }
    if ( !rc ) printf( "patch file %s applied to VS1053\n", filename);    
}
