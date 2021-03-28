#ifdef USEOWM

#ifndef OWM_H
#define OWM_H

#include "tft.h"
#include "fonts.h"
#include <wificredentials.h>
#include <ArduinoJson.h>

extern TFT_eSPI tft;
extern screenPage currDisplayScreen;

// default is to display air pressure in mm Hg
#define PRESSURE_IN_HPA 1

struct Owmdata{
    float temperature;
    float humidity;
    float pressure;
    float windspeed;
    float feelslike;
    char  *description  = NULL;
    char  *iconfilename = NULL;
    char  *city         = NULL;
    bool  valid         = false;
    char  iconchar;  
  };
extern struct Owmdata owmdata;  
extern char  *jsonowm; 
    void init_owm();
    bool getWeather();
    void fillWeatherSprite();
    void drawWeather();
    void print_owmdata();
    bool getForecast();
    void fillForecastSprite();
    void drawForecastSprite();
    void print_forecast();  
#endif

#endif
