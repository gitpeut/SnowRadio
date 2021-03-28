#ifdef USETRAFFIC

#include "traffic.h"
//---------------------------------------------------------------------
// WARNING: this is a very rudimentary XML parser, built for a specific XML format
// of a specific site. It will need modification or rewrite if another
// site or format is used.
//
void parse_trafficxml( char *xml, struct traffic &xt ){ 

    char  *xi = NULL;
    char  *findp = NULL;
    char  *valuep = NULL; 
    char  *startregion = NULL;
    char  foundvalue[2][64];
    int   treecount=0, valuecount=0;
    const char  *findlabel[] = {"info","traffic", "region","level","time"};
    int   mode=0;

    for ( xi = xml; *xi && valuecount < 2; ++xi ){
      
      switch ( mode ){
        case 0: 
              if ( *xi == '<' && *(xi+1) != '/' ){
                mode = 1;
                findp = (char *)findlabel[treecount];
              }
              break;
        case 1:
              if (*xi == '>' || *xi == ' '){
                 treecount++; 
                 findp = (char *)findlabel[ treecount];
                 //log_d("now finding %d %s", treecount, findp);
                 if ( treecount == 2 ) startregion = xi; 
                 if ( treecount > 3 ){
                  Serial.printf("now finding value for %s\n", findlabel[ treecount - 1] );
                  valuep = foundvalue[ valuecount];
                  mode   = 2;
                 }
              }else if ( *xi != *findp || *findp == 0){            
                          findp = NULL;
                          mode = 0;
              }else{
                ++findp;
              }
              break;
       case 2:
              if ( *xi != '<' ){
                *valuep = *xi;
                ++valuep;
              }else{
                *valuep = 0;
                //log_d ( "value %d %s", valuecount, foundvalue[ valuecount] );
                xi = startregion;
                ++valuecount;
                mode = 0;
              }
              break;  
      }
    }

    //log_d ( "End of xml - %s %s",foundvalue[0], foundvalue[1]);
    xt.level = atoi( foundvalue[0] ) ;
    xt.time  = foundvalue[1] ;
    return;  
}

//----------------------------------------------------------------------------------

bool get_traffic( struct traffic &xt){

  WiFiClientSecure *traffic_client = new WiFiClientSecure;
  int   httpCode=-1;
  bool  return_status = false;

  
  if ( !traffic_client ){
    Serial.println( "failed to create new secure client for traffic");
    return( false );
  }

  traffic_client->setInsecure();  
  
  { // scoping block to guarantee httpclient to be deleted before the traffic_client ( from examples)
      HTTPClient https;
      if (https.begin(*traffic_client, trafficUrl )) {  // HTTPS
          // start connection and send HTTP header
          httpCode = https.GET();

          if (httpCode > 0) {
            // get lenght of document (is -1 when Server sends no Content-Length header)
                int document_size = https.getSize();
                int readlen = -1, totleft = document_size, totread=0;

                // create buffer for read
                if ( document_size == -1){
                  document_size = (20* 1024);
                  totleft = document_size;
                }
                char *read_buffer  = (char *)gr_calloc ( document_size + 4, 1 );
                if ( read_buffer == NULL ){
                  https.end();
                  
                  traffic_client->stop();
                  delete traffic_client;
                  log_e("Not enough memory for document of %d bytes\n", document_size );
                  return(false);
                }
                // get tcp stream
                WiFiClient * stream = https.getStreamPtr();

                // read all data from server
                while(https.connected() && ( readlen > 0 || readlen == -1) && totleft) {
                    // get available data size
                    size_t size = stream->available();

                    if(size) {
                        // read up to 128 byte
                        int readlen = stream->readBytes(read_buffer + totread, 32);
                        totread += readlen;
                        totleft -= readlen;

                    }
                    delay(1);
                }
                *(read_buffer + totread) = 0;// make sure there is a zero, although calloc should 
                                             // have taken care of that.

                if ( totread == document_size || ( document_size == -1 && totread ) ){
                  //log_d("document_size %d, read %d\n--\n%s\n----\n", document_size, totread, read_buffer );
                  parse_trafficxml( read_buffer, xt  );
                  return_status = true;      
                }
                
                free( read_buffer );
                log_d("[HTTP] connection closed or file end.\n");}
                https.end();
        } else {
            log_e("[HTTPS] Get traffic failed, error: %s\n", https.errorToString(httpCode).c_str());
        }  
                 
  }
  
  traffic_client->stop();
  delete traffic_client;

  return( return_status );
}


struct traffic traffic_info;

//---------------------------------------------------------------------
bool show_traffic(){
  
  if ( stations[ playingStation ].protocol ) return ( false );
   
  if ( get_traffic( traffic_info ) ){
      Serial.printf("Got traffic: level : %d time : %s\n", traffic_info.level, traffic_info.time.c_str() );  
      return( true );
  }else{
      Serial.println( "Failed to get traffic");
      return( false );
  } 
  

}
#endif
