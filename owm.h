#ifdef USEOWM

#ifndef OWM_H
#define OWM_H

#include "tft.h"
#include "fonts.h"
#include <wificredentials.h>
#include <ArduinoJson.h>

extern TFT_eSPI tft;
extern screenPage currDisplayScreen;
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
