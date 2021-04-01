#include "tft.h"
#include "touch.h"



#define BATVREF     1.1f
#define BATPINCOEF  1.95f // tune -6 db
#define BATDIV      5.54f // (1M + 220k )/220k



#define SCROLLPIN 0
int station_scroll_h=TFTSTATIONH;


//change rotation if necessary
//depends on you set up your display.
int tftrotation = TFT_ROTATION;

int verysmallfont= 1;
int smallfont= 2;
int bigfont=4;
int segmentfont = CLOCK_FONT;

std::vector<bmpFile *> bmpCache;



//
//----------------------------------------------------------
void IRAM_ATTR grabTft(){
  //printf("grab TFT\n");
  xSemaphoreTake( tftSemaphore, portMAX_DELAY);
  //printf("grabbbed TFT\n");

}
//----------------------------------------------------------
void IRAM_ATTR releaseTft(){
  
//tft.fillRect( 4,12 , 1, 1, TFT_REALGOLD ); //flicker kludge, not necessary when using rmt_write_items in sk.h
xSemaphoreGive( tftSemaphore); 
//printf("released TFT\n");
}


//----------------------------------------------------------


char *utf8torus(const char *source, char *target){
    unsigned char *s  = (unsigned char *) source;
    unsigned char *t  = (unsigned char *) target;
    bool after208 = false;
    
    if ( *s != 208 && *s != 209 ) {
      strcpy( target, source );
      return( target );
    }
    
    while (*s){
        switch( *s ){
            case 208:
                after208 = true;
                break;
            case 209:
                after208 = false;
                break;
            default:
                // fix for mistake in font
                *t = *s;
                if (  after208 && *t == 129 ) *t=192;
                if ( !after208 && *t == 145 ) *t=193;
                ++t;
                break;        
            }
            ++s;            
    }
    *t = 0;    
    return( target );
}


//---------------------------------------------------------------------
void tft_message( const char  *message1, const char *message2 ){
static int yposition = 0;
bool progress = true;

if ( !msg.created() ){
  msg.createSprite( tft.width(), progress?tft.height() - 90:100);

  msg.setTextColor( TFT_WHITE, TFT_BLACK ); 
  msg.setTextSize(1);
  msg.fillSprite(TFT_BLACK);
}

if ( !progress ){
  msg.fillSprite(TFT_BLACK);
  yposition = 0; 
}

msg.drawString( message1, 0, yposition, 2);  
yposition += 16;
if (message2 ){
  msg.drawString( message2, 0, yposition, 2);  
  yposition += 16;
}

 
xSemaphoreTake( tftSemaphore, portMAX_DELAY);  
  msg.pushSprite( 0, 90 ); 
xSemaphoreGive( tftSemaphore); 

}
//-----------------------------------------------------
void tft_show_gesture( bool showonscreen ){
int   w=32;
//int   h=TFTINDICH;
//int   xpos=33 + (BUTOFFSET*2),ypos=TFTINDICT;
int   h    = 20;
int   xpos = BUTOFFSET;
int   ypos = TFTCLOCKT + 12 + h; 
char  geststring[4];
    
   if ( currDisplayScreen != RADIO) return;
   if (   screenUpdateInProgress ) return;
 
   if ( !gest.created() )gest.createSprite(w,h);
   gest.fillSprite(TFT_BLACK);          

   if ( showonscreen ){
      gest.setFreeFont( &indicator );
      geststring[0] = 81; //little hand
      geststring[1] = 0;   
      gest.setTextColor( TFT_WHITE, TFT_BLACK ); 
      gest.drawString( geststring , 0,1 );
   }


grabTft();
  gest.pushSprite( xpos, ypos);
releaseTft();  

}
//----------------------------------------------------------

int read_battery(){
  int i;
  int     batread=0, batotal=0;
  
  for(i=0;i<5;i++){
      batread = analogRead( BATPIN );
      batotal += batread;  
      delay(50);
  }

  batread = batotal/5;
  batvolt = ( BATVREF * ( batread * BATDIV)/4096)* BATPINCOEF;

  //Serial.printf( "read : %d, voltage: %f\n", batread, batvolt);
  if ( batread > 1424 ) return(100);
  if ( batread > 1404 ) return( 90);
  if ( batread > 1362 ) return( 80);
  if ( batread > 1345 ) return( 70);
  if ( batread > 1321 ) return( 60);
  if ( batread > 1300 ) return( 50);
  if ( batread > 1280 ) return( 40);
  if ( batread > 1262 ) return( 30);
  if ( batread > 1242 ) return( 20);
  if ( batread > 1220 ) return( 10);
  return(0);  
 
}
//-------------------------------------------------------------------------------
void showBattery(bool force ){

//  int   ypos = TFTINDICT;
  int   ypos = TFTCLOCKT;
  int   xpos = tft.width() - 32 - BUTOFFSET;
  int   percentage = read_battery();
  static int previous_percentage = -1;
  char  batstring[2]; 

  if ( currDisplayScreen == STNSELECT )return;
  if (   screenUpdateInProgress ) return;
  if ( percentage == previous_percentage && !force ) return;
  previous_percentage = percentage;
  
  batstring[0]= 'L' + percentage/25;
  batstring[1]= 0;
 
    if ( !bats.created() ) bats.createSprite(32,24);
    bats.setFreeFont( &indicator );
    bats.setTextColor( TFT_REALGOLD, TFT_BLACK ); 
    bats.fillSprite(TFT_BLACK);          
    bats.drawString( batstring , 0,0 );

    grabTft();
      //log_d("Push battery sprite to x %d, y%d", 0, ypos);
    bats.pushSprite( xpos, ypos);
    releaseTft();


}

//----------------------------------------------------------
void showVolume( int percentage, bool force){
// indicator font is 32 wide and 20 high.

  int   ypos = TFTCLOCKT;
  char  volstring[2];
  static int previous_percentage=-1; 
  
  if ( currDisplayScreen == STNSELECT )return;
  if (   screenUpdateInProgress ) return;
  if ( force == false){
    if ( percentage == previous_percentage ) return;
  }
  
  previous_percentage = percentage;

  volstring[0]= 'A' + percentage/10;
  volstring[1]= 0;
    
  if ( ! vols.created() )vols.createSprite(32,24);
  vols.setFreeFont( &indicator );
  vols.setTextColor( TFT_REALGOLD, TFT_BLACK ); 
  vols.fillSprite(TFT_BLACK);  

  if ( currDisplayScreen == RADIO ){
    vols.drawString( volstring ,0 ,0 );
  }
  
  grabTft();
  vols.pushSprite( BUTOFFSET, ypos);
  releaseTft();

 
  return;  
}

//--------------------------------------------------------------------------------------------------------

void showCloud( bool force){
  int   ypos = TFTCLOCKT + 23;
  int   xpos = tft.width() - 32 - BUTOFFSET;static char  previous_char = 0; 
  char  tmpmon[8];

  if ( !owmdata.valid ){
    log_e("no valid weather data");
    return;
  }
  if ( currDisplayScreen == STNSELECT )return;
  if (   screenUpdateInProgress ) return;
 
  if ( force == false ){
    if ( owmdata.iconchar == previous_char )return;
  }  
  previous_char = owmdata.iconchar; 

  if ( !cloud_sprite.created() )cloud_sprite.createSprite(  30, 30 );
 
  cloud_sprite.fillSprite(TFT_BLACK);    
  cloud_sprite.setTextColor( TFT_REALGOLD, TFT_BLACK ); 
  cloud_sprite.setFreeFont( &weather ); 
  
  tmpmon[0] = owmdata.iconchar; 
  tmpmon[1] = 0;         
  
  cloud_sprite.drawString( tmpmon,0, 0 );
          
  grabTft();
   cloud_sprite.pushSprite( xpos, ypos );// 128 is the bottom of the date label
  releaseTft();
      
}
//--------------------------------------------------------------------------------------------------------
void showClock ( bool force ){
  int     clockh, clockx, clocky, clockw;
  int     datex, datey, datew;
  int     dspritex,dspritey,dspritew;
  int     yy;
  char    tijd[32] = "", datestring[128] = "";
  char    tmpday[128] = "", tmpmon[ 128 ]= "";
  static  int previous_date = -1;
  static  int previous_min  = -1;
  
  time_t  rawt;
  struct tm tinfo;    
        
  if ( currDisplayScreen == STNSELECT )return;
  if (   screenUpdateInProgress ) return;
   
  tinfo.tm_year = 0;
  time( &rawt );
  localtime_r( &rawt, &tinfo);

 
  // no clock after year 2100
  if ( tinfo.tm_year < 100 || tinfo.tm_year > 200 )return;
  
  if ( force == false ){
    if ( tinfo.tm_min == previous_min )return;
  }else{  
    delay(100);
  }
  if ( xSemaphoreTake( clockSemaphore, 50) != pdTRUE) return;
  
  previous_min = tinfo.tm_min;
  yy = tinfo.tm_year + 1900 ;
  
  //log_d( "hour %d, minute %d", tinfo.tm_hour, tinfo.tm_min);
 
  sprintf(tijd,"%02d:%02d",tinfo.tm_hour, tinfo.tm_min);   
  clockw = tft.textWidth( tijd, segmentfont ) + 4;
  clockh = tft.fontHeight( segmentfont) + 2;
  clockx = (tft.width() - clockw)/2;
  clocky = TFTCLOCKT;

 
  if ( !clocks.created() )clocks.createSprite(  clockw, clockh );
  
  clocks.fillSprite(TFT_BLACK);    
  clocks.setTextColor( TFT_REALGOLD, TFT_BLACK ); 
  clocks.drawString( tijd, 0, 1, segmentfont);
  
  if ( (tinfo.tm_mday != previous_date) || force ){
      previous_date = tinfo.tm_mday;
      
      dspritew = tft.width();
      if ( !date_sprite.created() )date_sprite.createSprite(  dspritew, 32 );
      
      date_sprite.fillSprite(TFT_BLACK);
      date_sprite.setFreeFont( DATE_FONT );    
      
      int labelh  = date_sprite.fontHeight(1)-1 ;
      int labelw  = 230;
      int labelx  = (dspritew - labelw)/2;
      int labely  = 0;
      date_sprite.fillRoundRect( labelx, labely, labelw , labelh, 8, TFT_REALGOLD);  
 
      sprintf( datestring,"%s %d %s %d", utf8torus( daynames[tinfo.tm_wday], tmpday ), tinfo.tm_mday, utf8torus(monthnames[tinfo.tm_mon], tmpmon), yy);   
      datew = date_sprite.textWidth( datestring, 1 );

      datex    = (dspritew - datew )/2;
      datey    = 0;
    
      date_sprite.setTextColor( TFT_BLACK, TFT_REALGOLD ); 
      date_sprite.drawString( datestring, datex, datey ); 
    
      dspritex = 0;
      dspritey = TFTCLOCKT + tft.fontHeight( segmentfont) + 6;
  
      //log_d( "date label bottom (top) %d + (height) %d = %d", dspritey, labelh, dspritey+labelh);
      
      grabTft();
        date_sprite.pushSprite( dspritex, dspritey );
      releaseTft();
  
      
   
  }
    
  grabTft();
    clocks.pushSprite( clockx, clocky );
  releaseTft();
    
  showBattery( force);
  showVolume( getVolume(), force );

  #ifdef USEOWM
    showCloud( force );
  #endif
  
  if ( currDisplayScreen == RADIO ){
  
        
        date_sprite.fillSprite(TFT_BLACK);    
        date_sprite.setTextColor( TFT_REALGOLD, TFT_BLACK );       

        if ( owmdata.valid ){
          char tmprus[128];
          sprintf( tmpmon, "%2.1f*C %s",owmdata.temperature, utf8torus( owmdata.description, tmprus ) );
          if( strlen( tmpmon ) > 15 ){
            date_sprite.setFreeFont( LABEL_FONT );         
          }else{
            date_sprite.setFreeFont( DATE_FONT ); 
          }
          
          int dw = date_sprite.textWidth( tmpmon, 1 );
          int dx = ( tft.width() - dw )/2;
          
          date_sprite.drawString( tmpmon, dx, 6 );
        }
        grabTft();
          date_sprite.pushSprite( dspritex, 130 );// 128 is the bottom of the date label
        releaseTft();
  }
  
  xSemaphoreGive( clockSemaphore );

}

//---------------------------------------------------------------------
void tft_showstation( int stationIdx){

  int   xpos = 0, ypos=7;
  char  *s = stations[stationIdx].name;
  char  *station_name; 
  
  if ( currDisplayScreen != RADIO ) return;
  if ( xSemaphoreTake( stationSemaphore, 50) != pdTRUE) return;
  
  if ( strlen( s ) > 18 ) { //omit the first word of the station name
               s += 5;
               while( *s && *s != ' ') ++s;
               if ( *s ) ++s; 
  }

   
  if( !station_sprite.created() ) station_sprite.createSprite(tft.width(), station_scroll_h );
  station_sprite.fillSprite(TFT_BLACK);
  station_sprite.setTextColor( TFT_WHITE, TFT_BLACK ); 

  if ( *s == 0xd0 || *s == 0xd1 ){
    station_name = (char *)gr_calloc( strlen(s) + 4,1);
    utf8torus( s, station_name);            
    log_d("Cyrillic station name length %d ", strlen ( station_name ) );
    station_sprite.setFreeFont( STATION_FONT );
  }else{
    latin2utf( (unsigned char *) s, (unsigned char **)&station_name );              
    station_sprite.setFreeFont( STATION_FONT_LATIN );
  }

  
  int wholew = station_sprite.textWidth( station_name, 1 );
  
  xpos = (tft.width() - wholew)/2; 
  ypos = 2;
  station_sprite.drawString( station_name , xpos, ypos,1);
  
  free( station_name );
  
  
  grabTft();
  station_sprite.pushSprite( 0, TFTSTATIONT );
  releaseTft();

   
  xSemaphoreGive( stationSemaphore );
}


//---------------------------------------------------------------------

const int nonscroll_metawidth   = tft.width() - 20;
const int meta_height           = TFTMETAH;
const int meta_xposition        = 0 + 10;
const int meta_yposition        = TFTMETAT;
int       metatextx              = 0; // will be calculated in tft_fillmeta
char      *metatxt[10]           ={NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL};

enum metaAlphabet {LATIN, CYRILLIC};
metaAlphabet  metalang;

// unwanted strings. Last element MUST be ""
const char  *unwanted_strings[] = { "Chromanova.fm presents - ", " - 0:00", "MegaNight RADIO | ", ""};

//---------------------------------------------------------------------

char *cleanup_meta( const char *meta , char **out){
    char *m  = ( char *) meta;
    char *o = *out;
    
    while ( *m ){

        // remove everything in square brackets, including the square brackets
        if ( *m == '[' ){
           while (*m != ']') ++m;
           ++m;
           continue; // another bracket maybe following 
        }
        
        // remove any string in unwanted_strings 
        for ( int remcount = 0; unwanted_strings[ remcount][0]; ++remcount ){    
            
            if ( *m != unwanted_strings[ remcount][0] ){
                continue;
            }
            if ( strncmp( m, unwanted_strings[remcount], strlen( unwanted_strings[remcount]) ) ){
                continue;            
            }
            //remove by skipping over it in the input 
            m += strlen( unwanted_strings[remcount] );
            remcount = -1; // start again, an unwanted string we checked before may be following this one
        }
        
        
        if ( *m ){
           
           if (o == *out && *m == ' '){
            ;  // Skip leading blanks
           }else{ 
                *o = *m;
                ++o;
           }
           ++m;
        }
    }
    // close the output string
    *o = 0;
// for convenience also return a pointer to the output string.
return(*out);
}

//---------------------------------------------------------------------
void tft_create_meta( int spritew){
#ifndef SHOWMETA
  return;
#endif
    
  if ( meta_sprite.created() )return;

  meta_sprite.createSprite(spritew?spritew:nonscroll_metawidth, meta_height );
  meta_sprite.fillSprite(TFT_BLACK);
  meta_sprite.setTextColor( TFT_WHITE, TFT_BLACK ); 

}
#ifdef METAPOPUP

int popup_linecount = 0;

    void tft_fillmeta(){
        char  *mymeta=NULL, *cleanmeta = NULL; ;
        unsigned char  *utfmeta=NULL;
        
      #ifndef SHOWMETA
        return;
      #endif
        //log_d( "- fillmeta in transit : %d , metadata %s-", meta.intransit, meta.intransit?"": meta.metadata);
        
        
        tft_create_meta();

        popup_linecount = 0;    
                              
        for (int i =0; i < 10; ++i ){
            if ( metatxt[i] != NULL ){
              free( metatxt[i] ); 
              metatxt[i] = NULL;
            }
        }
        
        if ( meta.metadata[0] == 0xd0 || meta.metadata[0] == 0xd1 ){
          //Cyrillic characters  
          metalang = CYRILLIC;
          meta_sprite.setFreeFont( META_FONTRUS );
          utfmeta = (unsigned char *)gr_calloc( strlen( meta.metadata) + 4,1);
          mymeta = utf8torus( meta.metadata, (char *)utfmeta);            
        }else{
          //Latin characters
          metalang = LATIN;
          meta_sprite.setFreeFont( META_FONT );
          latin2utf( (unsigned char *) meta.metadata, (unsigned char **)&utfmeta );              
          mymeta = ( char *) utfmeta;
        }

         cleanmeta = (char *)gr_calloc( strlen( (char *)utfmeta ) + 4 , 1 );
         mymeta = cleanup_meta( (const char *) utfmeta , &cleanmeta );

         free( utfmeta );
          
        int textw   = meta_sprite.textWidth( mymeta, 1 ); 

        //log_d("textw = %d", textw );
        
        if ( textw < nonscroll_metawidth ){
           popup_linecount = 1;
           //log_d ("metadata fits on 1 line: %s", meta.metadata);
           metatxt[0] = ps_strdup( mymeta );
        }else{ // multiple lines
                char *start = mymeta, *end = mymeta, *previous=start;

                bool endreached = false;
                for ( end = mymeta; popup_linecount < 10; ++end ){
                    
                    if ( *end == ' ' || *end == 0 ){
                       if ( *end == 0 ){
                          endreached = true;
                       }else{
                          *end = 0;
                       }
                       textw   = meta_sprite.textWidth( start, 1 );
                       if ( !endreached ) *end = ' ';
                            
                       if ( textw > nonscroll_metawidth || endreached ){
                           
                           if ( previous != (start -1 ) && textw > nonscroll_metawidth  ){ 
                            end = previous;
                            endreached = false;
                           }
                           *end = 0;
                            
                            metatxt[popup_linecount] = strdup( start );
                            //log_d( "Added %s ", start) ;
                            popup_linecount++;
                            if ( !endreached ){ 
                                start = end + 1;
                                previous = end;
                            }
                       }else{
                            if ( !endreached ) {
                              previous = end;
                            }                  
                       }
                   }
                   if ( endreached ) break;
                } 
        }

        mymeta = NULL;
        if ( cleanmeta != NULL)free( cleanmeta ); 
        log_d ("%d lines of metatdata filled", popup_linecount);
                             
        tft_showmeta( true );
     }

     //---------------------------------------------------------------------
      void tft_showmeta(bool resetx){
      static int currentx=0;
      static int lastline=0;
      
        if ( resetx ){
          currentx = 0;
          lastline = 0;
          return;
        }

        if ( screenUpdateInProgress ) return;
        if ( currDisplayScreen != RADIO ) return;
        if ( meta.intransit ) return;
        
        if ( currentx > 0 ){
            currentx--;
            return;
        }else{
            if ( lastline >= popup_linecount ) lastline = 0;            
            currentx = 20;
            if ( popup_linecount < 3 ) currentx += 100;
        }
          

        meta_sprite.fillSprite(TFT_BLACK);
        meta_sprite.setTextColor( TFT_WHITE, TFT_BLACK ); 

        if ( metatxt[lastline] != NULL ) {

          //log_d( "show meta # %d/%d  %s ", lastline,popup_linecount, metatxt[ lastline ] );
          int xpos = 0;
          int ypos = 0;
          if ( popup_linecount == 1){
            xpos = ( meta_sprite.width() - meta_sprite.textWidth( metatxt[ lastline ], 1 ) )/2; 
            ypos = meta_sprite.height() / 4;
          }
          meta_sprite.drawString( metatxt[ lastline ], xpos, ypos, 1);
          currentx += strlen( metatxt[ lastline ] ) / 2;
          ++lastline;
        }


        if ( lastline < popup_linecount && metatxt[lastline] != NULL ){

          //log_d( "show meta # %d/%d  %s ", lastline,popup_linecount, metatxt[ lastline ] );

          meta_sprite.drawString( metatxt[ lastline ], 0,14, 1);
          currentx += strlen( metatxt[ lastline ] ) / 2 ;
          ++lastline;                                 
        }
        
        grabTft();
          tft.setViewport( meta_xposition, meta_yposition, nonscroll_metawidth, meta_height, false );
          meta_sprite.pushSprite( meta_xposition, meta_yposition );
          tft.resetViewport();         
        releaseTft();
      }      
      
#endif


#ifdef METASPRITE
                    
          //---------------------------------------------------------------------
          void tft_fillmeta(){
            int   textw, textx=0, spritew;
            char  *txt[2] = {NULL,NULL};
            char  *middle=NULL, *mymeta=NULL;
            unsigned char  *utfmeta=NULL;
            
          #ifndef SHOWMETA
            return;
          #endif
            
            tft_create_meta();

            
            if ( meta.metadata[0] == 0xd0 || meta.metadata[0] == 0xd1 ){
              //Cyrillic characters  
              metalang = CYRILLIC;
              meta_sprite.setFreeFont( META_FONTRUS );
              utfmeta = (unsigned char *)gr_calloc( strlen( meta.metadata) + 4,1);
              mymeta = utf8torus( meta.metadata, (char *)utfmeta);            
            }else{
              //Latin characters
              metalang = LATIN;
              meta_sprite.setFreeFont( META_FONT );
              latin2utf( (unsigned char *) meta.metadata, (unsigned char **)&utfmeta );              
              mymeta = ( char *) utfmeta;
            }
         
            

            textw   = meta_sprite.textWidth( mymeta, 1 ); 
            if ( textw < nonscroll_metawidth ){           // text fits on one line
               textx    = ( nonscroll_metawidth - textw )/2;
               spritew  = nonscroll_metawidth;
            }else if ( textw < ( 2*nonscroll_metawidth ) ){ //text fits on 2 lines, if it can be divided evenly on word borders
          
               middle = mymeta + strlen( mymeta ) / 2;
               for( ; middle > mymeta && *middle != ' ' && middle > mymeta; --middle);
               if ( middle - mymeta < strlen( mymeta ) / 3 ){
                  middle = mymeta + strlen( mymeta ) / 2;
                  for( ; middle > mymeta && *middle != ' ' && *middle; ++middle);
               }
               txt[0] = ps_strndup( mymeta, middle - mymeta );
               txt[1] = ps_strdup(  middle + 1);
          
                
               char *longest = txt[0];
               if ( strlen( txt[1]) > strlen(txt[0]) ){
                longest = txt[1];
               }
               
               textx   = ( nonscroll_metawidth - meta_sprite.textWidth( longest, 1 ))/2;
               if ( textx < 0 ) textx = 0;
                
               spritew = nonscroll_metawidth;   
            }else{                                // text is very long, make a scrolling sprite
               textx    = 0;
               spritew  = textw + nonscroll_metawidth;
            }
            
            meta_sprite.deleteSprite();
            tft_create_meta( spritew );

            
            
            if ( txt[0] != NULL ){ 
              meta_sprite.drawString( txt[0], textx, 0, 1);
              free( txt[0]);
              if ( txt[1] != NULL){
                log_d( "txt 1 : %s\n", txt[1]);
                meta_sprite.drawString( (char *)txt[1],textx,12, 1);
                free( txt[1] );
              } 
            }else{
              meta_sprite.drawString( (char *)mymeta, textx, 6, 1);
            }

            mymeta = NULL;
            if ( utfmeta != NULL)free( utfmeta );
            
            tft_showmeta( true );
          }
          //---------------------------------------------------------------------
          void tft_showmeta(bool resetx){
          static int currentx=0;
          static int onscreen=0; 
          
            //log_d("onscreen %d", onscreen);
            
 
            if ( resetx ){
              currentx = 0;
              onscreen = 0;
              return;
            }

            if ( screenUpdateInProgress ) return;
            if ( currDisplayScreen != RADIO ) return;
            if ( meta.intransit ) return;
            if ( onscreen )return;
             
            if ( meta_sprite.width() == nonscroll_metawidth ){  
                 currentx = nonscroll_metawidth;
                 onscreen = 1;
            }else{
                if ( currentx < meta_sprite.width() ){
                  currentx += 1;
                }else{
                  currentx = 0;
                }
            }
          
            grabTft();
              tft.setViewport( meta_xposition, meta_yposition, nonscroll_metawidth, meta_height, false );
              //tft.frameViewport(TFT_DARKGREY,  -1);     
              meta_sprite.pushSprite( meta_xposition + nonscroll_metawidth - currentx, meta_yposition );
              tft.resetViewport();         
            releaseTft();
          }
#endif

#ifdef METASTATIC
//---------------------------------------------------------------------
void tft_fillmeta(){
  int   textw;
  char  *middle, *mymetaedit;
    char    *tmprumeta = (char *)gr_calloc( strlen( meta.metadata ) + 4 , 1 );
  char    *cleanmeta = (char *)gr_calloc( strlen( meta.metadata ) + 4 , 1 );
  String  mymeta;
  GFXfont *metafont; 
  
#ifndef SHOWMETA
  return;
#endif
  
  log_d("%s", meta.metadata);

  if ( *meta.metadata == 208 || *meta.metadata == 209 ){
      metafont = (GFXfont *) META_FONTRUS;
      metalang = CYRILLIC;
  }else{
      metafont = (GFXfont *) META_FONT;
      metalang = LATIN;      
  }

  meta_sprite.setFreeFont( metafont );

  // use the sprite to measure text width
  // this way, font changes do not affect the screen.  
  tft_create_meta();
  
  mymeta = String( cleanup_meta( (const char *)utf8torus(meta.metadata, tmprumeta ), &cleanmeta ) );

  if ( metalang == LATIN ){
    latin2utf( (unsigned char *) mymeta.c_str(), (unsigned char **)&mymetaedit );    
  }else{
    mymetaedit = ps_strdup( cleanmeta );
  }
  
  log_d( "Mymeta:\n%s", mymeta.c_str());

  free( tmprumeta );  
  free( cleanmeta ); 
   

#ifndef SHOWMETA
  return;
#endif
  if ( metatxt[0] != NULL ){
    free ( metatxt[0] );
    metatxt[0] = NULL;
  }
  if ( metatxt[1] != NULL ){
    free ( metatxt[1] );
    metatxt[1] = NULL;
  }

  textw   = meta_sprite.textWidth( mymetaedit, 1 ); 
  if ( textw < nonscroll_metawidth ){             // text fits on one line
     metatextx    = ( nonscroll_metawidth - textw )/2;
     metatxt[0] = ps_strdup( mymetaedit );

  }else{
     // don't scroll text, but chop off text so it fits on 2 lines.

     for ( char *end = mymetaedit + strlen( mymetaedit) ; 
           ( meta_sprite.textWidth( mymetaedit ) > (2 * nonscroll_metawidth) ) && end > mymetaedit; 
           --end ){ 
            *end = 0;
           }

     middle = mymetaedit + strlen( mymetaedit ) / 2;
     for( ; middle > mymetaedit && *middle != ' ' && middle > mymetaedit; --middle);
                    //     if ( middle - mymetaedit < strlen( mymetaedit ) / 3 ){
                    //        middle = mymetaedit + strlen( mymetaedit ) / 2;
                    //        for( ; middle > mymetaedit && *middle != ' '; ++middle);
     
     if ( middle - mymetaedit < (strlen( mymetaedit )/2 - 8) ){
          middle = mymetaedit + strlen( mymetaedit ) / 2;
          for( ; middle > mymetaedit && *middle != ' ' && *middle; ++middle);   
     }
     
     metatxt[0] = ps_strndup( mymetaedit, middle - mymetaedit );
     metatxt[1] = ps_strdup( middle + 1);
      
     char *longest = metatxt[0];
     if ( strlen( metatxt[1]) > strlen( metatxt[0]) ){
      longest = metatxt[1];
     }
     
     metatextx   = ( nonscroll_metawidth - meta_sprite.textWidth( longest, 1 ))/2;
     if ( metatextx < 0 ) metatextx = 0;
      
  }
  
  meta_sprite.deleteSprite();
  free( mymetaedit );
  
  tft_showmeta( true );
}
//---------------------------------------------------------------------
void tft_showmeta(bool resetx){
static int onscreen=0; 

  //log_d("onscreen %d", onscreen);
  
  if ( resetx ){
    onscreen = 0;
    return;
  }
  
  
  if ( currDisplayScreen != RADIO ) return;
  if ( meta.intransit ) return;
  if ( onscreen )return;
  if ( screenUpdateInProgress ) return;
   
  grabTft();

    tft.fillRect (meta_xposition, meta_yposition, nonscroll_metawidth, meta_height, TFT_BLACK);

    tft.setViewport( meta_xposition, meta_yposition, nonscroll_metawidth, meta_height, true );
       
    tft.setTextColor( TFT_WHITE, TFT_BLACK );
    
    if ( metalang == CYRILLIC ){
     tft.setFreeFont( META_FONTRUS );   
    }
    if( metalang == LATIN ){
     tft.setFreeFont( META_FONT );         
    }
    
    if ( metatxt[0] != NULL )tft.drawString( metatxt[0], metatextx, (metatxt[1] == NULL)?8:2, 1);
    if ( metatxt[1] != NULL )tft.drawString( metatxt[1], metatextx, 15, 1);
    
    tft.resetViewport();             
 releaseTft();

 onscreen = 1;
 
}
#endif
//---------------------------------------------------------------------

#ifdef MULTILEVELGESTURES
//---------------------------------------------------------------------
void tft_scrollstations( void *param ){
int direction;
int t,begint, endt, inct, tcount;
int beginx, endx, incx;
int halve = (tft.width()/2); 
int stopped =0;
int delaytime;

  xSemaphoreTake( scrollSemaphore, portMAX_DELAY);

  stopgTimer();
  
  scrollStation = -1;
  chosenStation = 0;
  begint        = currentStation;
  
  //pinMode( SCROLLPIN,INPUT_PULLUP );
  //attachInterrupt( SCROLLPIN, choose_scrollstation, CHANGE);
 
  direction = *((int *)param);
  begint  = currentStation;
  endt    = stationCount;
  if( endt > 40 ) endt = 40; 
  
  img.createSprite(tft.width(), station_scroll_h);
  img.setTextColor( TFT_REALGOLD, TFT_BLACK ); 
  img.setTextSize(1);

  
  if ( direction == SCROLLUP ){ //station increase, scroll right to left 
    inct    = 1; 
    beginx  = tft.width();
    endx    = ( -1*tft.width() );
    incx    = -20;
    tcount = 0;
    Serial.printf("scrollup\n");
  }

  if ( direction == SCROLLDOWN ){ //station increase, scroll right to left 
    inct    = -1;
    beginx  = (-1*tft.width());
    endx    = tft.width() + 5;
    incx    = 20;
    tcount = 0;
    Serial.printf("scrolldown\n");
  }
  
    
for ( t=begint; tcount < endt; t+= inct, tcount++ ){

  //delaytime = (tcount*tcount*100/endt)+100;
  delaytime = (tcount*tcount*100/endt)+300;
  
  if ( direction == SCROLLUP )incx = tcount - endt;
  if ( direction == SCROLLDOWN )incx = endt - tcount;
  
  if ( t > (stationCount-1) ) t = 0;
  if ( t < 0 ) t = (stationCount-1);

  int z = t + inct;           
  if ( z > (stationCount-1) ) z = 0;
  if ( z < 0 ) z = (stationCount-1);

 
  for ( int x = 0, stopped=0;1; x += incx ){

        //Serial.printf("%s incx %d x = %d endx %d \n", direction == SCROLLDOWN?"scrolldown":"scrollup", incx,  x, endx );
        if ( direction == SCROLLUP   && x <= endx )break;
        if ( direction == SCROLLDOWN && x >= endx )break;
                  
        img.fillSprite(TFT_BLACK);

        tft_showstations( t,  x ); 

        if ( direction == SCROLLUP ){
          tft_showstations( z,  x + tft.width()); 
        }else{
          tft_showstations( z,  x - tft.width()); 
          img.drawFastVLine(0,0, img.height(), TFT_BLACK);//bug somewhere in lib or display            
        }
        
        grabTft();
        img.pushSprite( 0, tft.height()-station_scroll_h );
        releaseTft();
                    
        if ( stopped == 0 && x <= halve ){ 
           scrollStation = t;
           delay(  delaytime );
           stopped = 1;     
           if ( x == 0 && tcount == (endt-1) ) break;

          xSemaphoreTake( chooseSemaphore, portMAX_DELAY);
          if (chosenStation)break;                        
          xSemaphoreGive( chooseSemaphore); 

           
        }             
  }//end for x loop

  if (chosenStation){
      if ( chosenStation == 2) tcount = endt; // abort
      chosenStation  = 0;
      xSemaphoreGive( chooseSemaphore); 
      break;
  }

  delay(5);
}//end for t looop

img.deleteSprite();

if ( tcount < endt ){
   setStation( t, -1 );
   tft_showstation( t );
   //currentStation = t;
   //delay(4000);
}else{
     tft_showstation( getStation() );
}

//detachInterrupt( SCROLLPIN );
scrollDirection = -1;

gmode = 2;
setgTimer();

xSemaphoreGive( scrollSemaphore); 

 
vTaskDelete( NULL );
    
}


// -------------------------------------------------------------------------

void tft_scrollstation(int whatway){
  
    xSemaphoreTake( scrollSemaphore, portMAX_DELAY);
    scrollDirection = whatway;

    Serial.printf("scroll %d %s\n", whatway, whatway?"from left to right":"from right to left" );
        
    int rc = xTaskCreate( 
         tft_scrollstations,                                      // Task to handle special functions.
         "Scroll",                                            // name of task.
         32*1024,                                                 // Stack size of task
         &scrollDirection,                                                 // parameter of the task
         SCROLLTASKPRIO,                                                    // priority of the task
         &scrollTask);                                               // Task handle to keep track of created task 

    Serial.printf("xTaskCreate rc %d \n", rc);

    delay(10);
    xSemaphoreGive( scrollSemaphore);
      
    
}
#endif

//------------------------------------------------------

void tft_uploadProgress( int percent ){

int hi;
int percentage = percent;

if ( percentage > 100 ) percentage = 100;
  
  hi = (tft.height() * percentage )/100; 

  grabTft();
  tft.fillRect( tft.width() - 12, tft.height() - hi, 12, hi, TFT_YELLOW );
  releaseTft();
}

//------------------------------------------------------
void tft_notAvailable( int stationIdx){
int w;

return;

grabTft();
tft.fillRect(0,80, tft.width(), tft.height()-80, TFT_BLACK );
tft.setTextColor( TFT_RED, TFT_WHITE );

w = tft.textWidth( stations[stationIdx].name,2 );
tft.drawString( stations[stationIdx].name, 80-(w/2), 80, 2 );    
w = tft.textWidth( "not available",2 );             
tft.drawString( "not available", 80-(w/2), 94, 2 );                 
releaseTft();

}


//------------------------------------------------------

void tft_ShowUpload(String uploadtype){
 grabTft(); 
 tft.fillScreen(TFT_BLACK);
 tft.setTextColor(TFT_WHITE );

 tft.drawString( uploadtype, 10, 26, 4 );

 tft.drawString( "upload", 10, 52, 4 );
 tft.drawString( "in progress", 10, 78, 4 );
 releaseTft(); 
}

//------------------------------------------------------

void tft_uploadEnd( String uploadstatus){
 
grabTft();
 if ( uploadstatus.startsWith("s") ){
   tft.fillScreen(TFT_WHITE);
   tft.setTextColor( TFT_BLACK );
 }else{
   tft.fillScreen(TFT_GREEN); 
   tft.setTextColor( TFT_RED );
 }

 tft.drawString( "upload", 10, 52, bigfont );
 tft.drawString( uploadstatus, 10, 78, bigfont );
releaseTft();
 
 if ( ! uploadstatus.startsWith("s") ){
      delay(5000);
      ESP.restart();
 }
}

//------------------------------------------------------------------------------------------
bmpFile *findBmpInCache( char *bmpfile ){
    
  for ( auto &bf : bmpCache ){
    if ( strcmp( bf->name, bmpfile ) == 0 ) return ( bf );
  }
  return( NULL );
}

//------------------------------------------------------------------------------------------
// Bodmers BMP image rendering function with (PS)RAM caching

void drawBmp(const char *filename, int16_t x, int16_t y, TFT_eSprite *sprite, bool show  ) {

  if ((x >= tft.width()) || (y >= tft.height())) return;

  uint32_t startTime = millis();
  bmpFile *cachedbmp = findBmpInCache( (char *) filename );
  
  if ( cachedbmp == NULL ){
    //log_i( "%s not found in cache", filename); 
    
    fs::File bmpFS; 
    bmpFS = RadioFS.open(filename, "r");
    
    if (!bmpFS)
    {
      log_e("File not found");
      return;
    }
   
    uint32_t seekOffset;
    uint16_t w, h, row;
    uint8_t  r, g, b;
    int      dataoffset;
     
    
    if (read16(bmpFS) == 0x4D42)
    {
      read32(bmpFS);
      read32(bmpFS);
      seekOffset = read32(bmpFS);
      read32(bmpFS);
      w = read32(bmpFS);
      h = read32(bmpFS);
      
      if ((read16(bmpFS) == 1) && (read16(bmpFS) == 24) && (read32(bmpFS) == 0))
      {
        cachedbmp = (bmpFile *)gr_calloc( sizeof(bmpFile),1); 
        if ( cachedbmp == NULL ){
          log_e("No memory for bmpcache");
          return;
        }else{
          cachedbmp->name = ps_strdup( filename );
          cachedbmp->w = w;
          cachedbmp->h = h;
        }
        
        y += h - 1;
          
        tft.setSwapBytes(true);
        bmpFS.seek(seekOffset);
    
        uint16_t  padding = (4 - ((w * 3) & 3)) & 3;
        int       linesize = w * 3 + padding;
        uint8_t  *lineBuffer = (uint8_t *) gr_calloc( linesize, sizeof(uint8_t) );
       
        cachedbmp->data = (uint8_t *)gr_calloc( (w*2)*h, sizeof( uint8_t) );
        dataoffset      = ( (w*2) * h ) - (w*2);

        for (row = 0; row < h; row++) {
          
          bmpFS.read(lineBuffer, linesize);
          uint8_t*  bptr = lineBuffer;
          uint16_t* tptr = (uint16_t*)lineBuffer;
          // Convert 24 to 16 bit colours
          for (uint16_t col = 0; col < w; col++)
          {
            b = *bptr++;
            g = *bptr++;
            r = *bptr++;
            *tptr++ = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
          }
    
          if ( show ){
            // Push the pixel row to screen, pushImage will crop the line if needed
            // y is decremented as the BMP image is drawn bottom up
            grabTft();
            if ( sprite == NULL ){
              tft.pushImage(x, y--, w, 1, (uint16_t*)lineBuffer);
            }else{
              sprite->pushImage(x, y--, w, 1, (uint16_t*)lineBuffer);
            }
            releaseTft();
          }
          //for( int i =0; i < (w*2) ; ++i )cachedbmp->data[dataoffset + i] = lineBuffer[i];
          memcpy(  cachedbmp->data + dataoffset , lineBuffer, (w*2) ); 
          dataoffset -= (w*2);
      
        }
        
        //log_d("Image read %sin %u ms", show?"and rendered ":"", millis() - startTime);
  
        bmpCache.push_back( cachedbmp );
        free( lineBuffer );
      }
      else {
        log_e("BMP format not recognized.");
      }
    }
    bmpFS.close();
    tft.setSwapBytes( false );// handy when proper colors are expected afterwards :-)jb
  }else{
     //log_i( "%s found in cache", filename);

     if ( show){
       tft.setSwapBytes(true);   
       grabTft();
            if ( sprite == NULL ){
              tft.pushImage( x, y, cachedbmp->w, cachedbmp->h, (uint16_t *)cachedbmp->data);              
            }else{
              sprite->pushImage( x, y, cachedbmp->w, cachedbmp->h, (uint16_t *)cachedbmp->data);
            }
       releaseTft();       
       tft.setSwapBytes( false );

       log_d("Image rendered from cache in %u ms", millis() - startTime);
    }      
  }
}

//------------------------------------------------------

uint16_t read16(fs::File &f) {
  uint16_t result;
  ((uint8_t *)&result)[0] = f.read(); // LSB
  ((uint8_t *)&result)[1] = f.read(); // MSB
  return result;
}
//------------------------------------------------------

uint32_t read32(fs::File &f) {
  uint32_t result;
  ((uint8_t *)&result)[0] = f.read(); // LSB
  ((uint8_t *)&result)[1] = f.read();
  ((uint8_t *)&result)[2] = f.read();
  ((uint8_t *)&result)[3] = f.read(); // MSB
  return result;
}

//------------------------------------------------------

void tft_init(){

   
  pinMode( TFT_RST , OUTPUT);
  digitalWrite( TFT_RST , LOW);
  delay( 10);
  digitalWrite( TFT_RST , HIGH);

// read battery
//  pinMode(BATPIN, INPUT);
//  analogSetAttenuation(ADC_6db);

  //PSRAM_ENABLE == 3 psramFound test is already done by tft_espi,
  //no need to duplicate.
  tft.setAttribute( 3,1);//enable PSRAM
  //tft.setAttribute( 2,0);// disable UTF8, use extended ASCII( will NOT enable extended ascii)
  tft.init();
  
  //Serial.printf("------- tft width = %d tft height = %d\n", tft.width(), tft.height() ); 
  if( tft.width() > tft.height() ) tftrotation = 1;
  tft.setRotation( tftrotation );

  tft.fillScreen(TFT_BLACK);
  drawBmp( "/images/SnowRadio.bmp", (tft.width()/2) - 25, 20);
  
  tft_message( tft.getAttribute(3)?"tft will use PSRAM": "tft will not use PSRAM");
  tft_message( CONFIG_SPIRAM_SUPPORT?"sprites will so, too":"sprites will not" );

  // make sure meta sprite exists
  tft_create_meta();
   
  log_i("tft initialized");
  
}
