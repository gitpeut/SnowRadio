#include "VS1053g.h"
#include "tft.h"

//----------------------------------------------------------------------------------

VS1053g::VS1053g( uint8_t _cs_ping, uint8_t _dcs_ping, uint8_t _dreq_ping) : VS1053 (_cs_ping, _dcs_ping, _dreq_ping) { 
 
 if ( psramFound() ){
        log_i( "PSRAM found");
        g_calloc = ps_calloc;
        g_malloc = ps_malloc;
    }else{
        log_i( "No PSRAM found");
        g_calloc  = calloc;
        g_malloc  = malloc;
    }  

  for (int i = 0; i < 14; i++) {
    spectrum[i][1] = 0; // dato precedente // lotfi
    spectrum[i][2] = 0; // picco grafico (non da Vs1053)
  } 
}
//----------------------------------------------------------------------------------
#define SCI_BASS  0x2
uint16_t VS1053g::getTone(){
    await_data_request();
    return( read_register(SCI_BASS) ); 
}
//----------------------------------------------------------------------------------
void     VS1053g::setTone( uint16_t value ){
  await_data_request();
  write_register(SCI_BASS, value); 
}
//----------------------------------------------------------------------------------

void VS1053g::setSpatial( uint8_t spatial) {
    uint8_t  sci_mode   = 0;  // register number
    uint16_t spatial_lo = (spatial&1)?(1<<4):0;
    uint16_t spatial_hi = (spatial&2)?(1<<7):0;
    uint16_t sm_sdinew  = (1<<11); 

    await_data_request();
    write_register( sci_mode, (spatial_lo | spatial_hi| sm_sdinew)  );   
    
    currspatial = -1;
    getSpatial();
    
}
//----------------------------------------------------------------------------------

uint16_t VS1053g::getSpatial() {
    uint8_t  sci_mode   = 0;  // register number
    uint16_t spatial_lo = (1<<4);
    uint16_t spatial_hi = (1<<7);
    uint16_t spatial;

    if ( currspatial == 65535 ){
      
      await_data_request();
      spatial    = read_register( sci_mode ); 

      spatial = ( ((spatial&spatial_lo)?1:0) | ((spatial&spatial_hi)?2:0) );   
      await_data_request();

      currspatial = spatial;
    }else{
      spatial = currspatial;
    }
    
    return( spatial );
}
//---------------------------------------------------------------------------------
bool VS1053g::stop_song(){
 uint16_t modereg; // Read from mode register
 int      i;            // Loop control
 uint8_t  sci_mode   = 0; 
 uint16_t sm_cancel  = (1<<3); 
 uint16_t sm_sdinew  = (1<<11); 
 
    sdi_send_fillers(2052);
    delay(10);
    write_register(sci_mode, sm_cancel | sm_sdinew );
    for (i = 0; i < 200; i++) {
        sdi_send_fillers(32);
        modereg = read_register( sci_mode); // Read status
        if ((modereg & sm_cancel ) == 0) {
            sdi_send_fillers(2052);
            LOG("Song stopped correctly after %d msec\n", i * 10);
            return true;
        }
        delay(10);
    }
    printDetails("Song stopped incorrectly!");
    return false;
}
//----------------------------------------------------------------------------------
void VS1053g::toMp3() {
    await_data_request();
    log_d("now doing actual switch to MP3 mode");
    switchToMp3Mode();
   
}

//----------------------------------------------------------------------------------

uint16_t VS1053g::setSpectrumBarColor( uint16_t newbarcolor){
  uint16_t oldcolor = spectrum_barcolor;
  
  spectrum_barcolor  = newbarcolor;
  return( oldcolor );
}
//----------------------------------------------------------------------------------

uint16_t VS1053g::setSpectrumPeakColor( uint16_t newpeakcolor){
  uint16_t oldcolor = spectrum_peakcolor;
  
  spectrum_peakcolor  = newpeakcolor;
  return( oldcolor );
}

//----------------------------------------------------------------------------------

int VS1053g::VS1053_file_size( const char *filename){
struct stat buf;
FILE *binfile;

    binfile = fopen( filename, "rb");
    
    if ( binfile == NULL ) {
      log_e("Couldn't open %s for read", filename );
      return(-1); 
    }
    
    int fd = fileno( binfile);    
    fstat(fd, &buf);
    fclose(binfile);

return( buf.st_size );    

}

//----------------------------------------------------------------------------------
void VS1053g::getBands()
{
  const uint16_t base = 0x1810;
  const uint8_t  wramaddr=7,wram =6;  
  //await_data_request(); should be verified in play() jb
  
  write_register( wramaddr, base + 2);
  bands = read_register( wram);
  
  write_register(wramaddr, base + 4);

  uint8_t current_volume = getVolume();
  
  for (uint8_t i = 0; i < 14; i++) {
    uint8_t val = read_register( wram );
    /* current value in bits 5..0, normally 0..31
      peak value in bits 11..6, normally 0..31 
     */
    uint32_t fivebitvalue   = (val&31);
    uint32_t barheightperc  = ( fivebitvalue * (uint32_t)current_volume ) / 31; // min = 0, max = 31*100 div by 100 = 0-100
    
      // by using a value lower than 100 to divide the value, 
      // we "amplify" the difference in volume between calls to getBands().
      // The lower this value, the more nerve-wrecking the display becomes. 
      // As we are writing to a sprite, bars that are too long will be 
      // cropped automatically. 
      
    int barheight =  (barheightperc * spectrum_height)/60;
    spectrum[i][0] = barheight;
  }

}
//----------------------------------------------------------------------------------

void VS1053g::displaySpectrum() {
 
  if (bands <= 0 || bands > 14)  return;
  uint8_t   bar_width   = 13;  
  uint16_t  barx = 0; // start location of the first bar
  static int nextx = 0;
  String mystation_sp;


  spectrum_width  = 208;
  
  if ( ! spectrum_sprite.created() ){
      spectrum_sprite.createSprite( spectrum_width, spectrum_height );      
  }
  
  if (bands != prevbands) {
    prevbands = bands;
    spectrum_sprite.fillRect (0,0, spectrum_width, spectrum_height, TFT_MY_DARKGRAY);
  }

  if ( nextprevChannel || MuteActive ){
    prevbands = 0;
    spectrum_sprite.fillRect (0,0, spectrum_width, spectrum_height, TFT_MY_DARKGRAY);
#ifndef RADIOANIMATION    // define in vs1053g.h
    if ( nextprevChannel ){
        nextx += 10;
        int curx = (nextprevChannel>0)?50+nextx:spectrum_width-50-nextx;
        if( curx > spectrum_width - 7 || curx < 0 ) nextx = 0;    

        char tmprustation_sp[128] = "";     
        mystation_sp = String( utf8torus((char *)stations[stationIdx].name, tmprustation_sp ) );      
        
        spectrum_sprite.setFreeFont( DATE_FONT );
        spectrum_sprite.setTextColor( TFT_MY_GOLD, TFT_MY_DARKGRAY );  
        spectrum_sprite.setTextDatum(TC_DATUM);

        spectrum_sprite.drawString( mystation_sp, 104, 61 );  
        spectrum_sprite.setFreeFont( ARROW_FONT );
        spectrum_sprite.setTextColor( TFT_MY_BLUE, TFT_MY_DARKGRAY ); 
        spectrum_sprite.drawString( (nextprevChannel>0)?"54545":"64646", spectrum_width/2, 5);
       
      
    }          
    if ( MuteActive && !nextprevChannel ){
          spectrum_sprite.setFreeFont( ARROW_FONT );
          spectrum_sprite.setTextColor( TFT_MY_BLUE, TFT_MY_DARKGRAY ); 
          spectrum_sprite.setTextDatum(TC_DATUM);
          spectrum_sprite.drawString( "7", spectrum_width/2 , 15);
    }  
#else
    if ( nextprevChannel ){
      
        char *station_name;
        spectrum_sprite.setTextColor( TFT_MY_GOLD, TFT_MY_DARKGRAY );
        if ( stations[ currentStation].name[0] == 0xd0 || stations[ currentStation].name[0] == 0xd1 ){
              //Cyrillic characters  
              spectrum_sprite.setFreeFont( META_FONTRUS );
              station_name = (char *)g_calloc( strlen(stations[ currentStation].name)+4,1);
              station_name = utf8torus( stations[ currentStation].name, station_name);              
        }else{
              spectrum_sprite.setFreeFont( META_FONT );
              latin2utf( (unsigned char *) stations[ currentStation].name, (unsigned char **)&station_name );
        }
      
        nextx += 10;
        
        int curx = (nextprevChannel>0)?50+nextx:spectrum_width-50-nextx;
        if( curx > tft.width() || curx < 0 ) nextx = 0;    
          int sline_y     = 2*(spectrum_height/3);
          int sline_start = 10;
          int sline_end   = spectrum_width-10;
    
            spectrum_sprite.drawString( station_name,
                                    (nextprevChannel>0)?sline_end - spectrum_sprite.textWidth( station_name):sline_start , 0, 1);
            free( station_name);
            
          spectrum_sprite.drawLine( sline_start, sline_y, sline_end,sline_y, TFT_GREEN);
          for( int i = 0; i < (tft.width() - 10); i += 20 ){
               spectrum_sprite.drawLine( sline_start + i, sline_y -10 , sline_start+i, sline_y, TFT_GREEN);        
          }
      
          spectrum_sprite.fillRect ( sline_start + curx, sline_y - 20, 3, spectrum_height - sline_y + 20, TFT_RED );
    }          
    if ( MuteActive && !nextprevChannel ){
      spectrum_sprite.setFreeFont( &radio_button_font );
      spectrum_sprite.drawString( "8", (tft.width() - 50)/2 , 0, 1);
    }  
#endif    
  }else{
    
        bar_width = spectrum_width / bands;
  
        while( (bar_width * bands + (bands-1) * 2 > spectrum_width) ){
            bar_width--;
        }
         
        barx = (spectrum_width - (bar_width * bands + (bands-1) * 2)) /2; 
        
        nextx = 0;
        for (uint8_t i = 0; i < bands; i++) // Handle all sections
        {
          int bartop    = spectrum_height - spectrum[i][0];
          int barheight = spectrum[i][0];

          if (spectrum[i][0] > spectrum[i][1]) {
            spectrum_sprite.fillRect (barx, bartop, bar_width, barheight, spectrum_barcolor );
            spectrum_sprite.fillRect (barx, 0, bar_width, bartop, TFT_MY_DARKGRAY);
          } else {
            spectrum_sprite.fillRect (barx, 0, bar_width, bartop, TFT_MY_DARKGRAY);
          }  
   
          if (spectrum[i][2] > 0) {         
            spectrum[i][2]--;
          }
          if (spectrum[i][0] > spectrum[i][2]) {
            spectrum[i][2] = spectrum[i][0];
          }
 
          spectrum_sprite.fillRect (barx, spectrum_height - spectrum[i][2] - 3, bar_width, 2, spectrum_peakcolor );
          
          spectrum[i][1] = spectrum[i][0];
          barx += bar_width + 2;
        }
  }
    
  if ( currDisplayScreen == RADIO && xSemaphoreGetMutexHolder( tftSemaphore ) == NULL){
     if ( xSemaphoreTake( tftSemaphore, 10 ) == pdTRUE ){
      spectrum_sprite.pushSprite( 255, 82 );
      releaseTft();  
    }else{
      log_d("failed to get tft semaphore");  
    }
  }
}


//-----------------------------------------------------------------------
// pasre RLE values in bin file and write to VS1053 

void VS1053g::write_VS1053_registers( unsigned short *pluginr, size_t valuecount){
    
    size_t i = 0, shortcount=0;
    unsigned short addr, n, val;


    while (i< valuecount - skiplast ) {
        addr = pluginr[i++];
        n = pluginr[i++];
        if (n & 0x8000U) { // RLE run, replicate n samples
          n &= 0x7FFF;
          val = pluginr[i++];
          while (n--) {
            write_register(addr, val);
            shortcount++; shortcount++;
          }
        } else {           // Copy run, copy n samples 
          while (n--) {
            val = pluginr[i++];
            write_register(addr,val);
            shortcount++; shortcount++;
          }
        }
    }

    log_i( "VS1053 found %u values, wrote %u shorts to VS1053", valuecount, shortcount);
    
}

//--------------------------------------------------------------------
// read saved bin file and write registers to vs1053

int VS1053g::read_VS1053_bin( const char *bin_filename){
int   size,readnumber, shortsize;    
FILE *binfile;
unsigned short *pluginv; 

   
   
    size = VS1053_file_size( bin_filename );
    if ( size < 0 ) return( 2);
    
    log_i("reading binfile %s of %u bytes", bin_filename, size);  
    
    shortsize = (size / sizeof( unsigned short ));
    
    binfile = fopen( bin_filename, "rb");
    
    if ( binfile == NULL ) {
      log_e("Couldn't open %s for read\n", bin_filename );
      return(1); 
    }
    
    pluginv = (unsigned short *)g_calloc( shortsize , sizeof( unsigned short ) );
    if ( ! pluginv ) {
        log_e( "no memory for patch, abort\n");
        return(2);
    }
    
    for( readnumber = 0; readnumber < shortsize; ){
        readnumber += fread( (pluginv+readnumber) , sizeof( unsigned short ) , 1 , binfile);
        if ( feof( binfile) ) break;
    }   
 
    if ( readnumber != shortsize ){
    
        log_e( "Error reading binfile, should read %llu, read %llu\n", shortsize, readnumber);
        return(3);
    }
    
    write_VS1053_registers( pluginv, shortsize);
    
    free( pluginv );
    return(0);  
}

//--------------------------------------------------------------------

int VS1053g::write_VS1053_binfile( unsigned short *pluginv, size_t valuecount, const char *bin_filename ){
FILE *binfile;

    binfile = fopen( bin_filename, "w");
    
    if ( binfile == NULL ) {
      log_e("Couldn't open %s for write\n", bin_filename );
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
int VS1053g::read_VS1053_plg( const char *filename ){
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
FILE  *plgfile=NULL;
char  c, *v, value[64]="";
int   mode=pFindPlugin;
size_t valuecount=0;
int   actionMode=0;
unsigned short *pluginv=NULL;  
  

for ( actionMode = 0; actionMode < 2; ++actionMode ){

    plgfile = fopen( filename, "r");
    if ( plgfile == NULL ) {
      log_w("Couldn't open %s for read", filename );
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
        log_v("Closed file after having read %u values\n", valuecount);
        log_v("Allocating %u bytes of memory for array \n", valuecount * sizeof(unsigned short ) );
        
        pluginv = (unsigned short *)g_calloc( valuecount, sizeof( unsigned short ) );
        if ( ! pluginv ) {
            log_e( "no memory for patch, abort\n");
            return(2);
        }
        
        log_v("Done, read again\n");
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

    write_VS1053_registers( pluginv, valuecount);
    
    free( pluginv );
    return(0);  
}

/*------------------------------------------------------------------------*/

void VS1053g::patch_VS1053( const char *filename, size_t skip_last_bytes){
int rc;
char    bin_filename[80];

// if plg file exists read it, convert it to bin, rename it and apply included patches
// plg file does not exist, look for ....plg.bin, red it amd apply patches.
// new patches can be installed by placing a new .plg file
    log_i( "Loading patch/plugin file %s", filename);
    
    skiplast = skip_last_bytes;
    
    rc = read_VS1053_plg( filename );
    if ( rc == 1){ // plg file not available   
       sprintf( bin_filename,"%s.bin", filename);
       rc = read_VS1053_bin( bin_filename ); 
       if( rc ){
        log_e( "applying patches to VS1053 failed");
       }
    }
    if ( !rc ){
      log_i( "Patch file %s (or the previously saved bin version) applied to VS1053", filename);    
    }
}
