#ifdef USEOWM

#ifndef OWM_H
#define OWM_H

#include "tft.h"
#include <wificredentials.h>
#include <ArduinoJson.h>

extern TFT_eSPI tft;
extern screenPage currDisplayScreen;


    bool getWeather();
    void fillWeatherSprite();
    void drawWeather();
    void print_owmdata();

#endif

#endif
