#ifdef USETRAFFIC

  #ifndef TRAFFIC_H
  #define TRAFFIC_H
    #include <HTTPClient.h>
    #include <WiFiClientSecure.h>
    
    struct  traffic{
      int    level = 99;
      String time  = "88:88";
    };
    
    const char *trafficUrl = "https://export.yandex.ru/bar/reginfo.xml?region=213&lang=en";
    
    bool get_traffic( struct traffic &xt);
    bool show_traffic();
    
    extern struct traffic traffic_info;
    
  #endif
#endif
