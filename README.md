# Gele Radio

Bug fixed and updated OranjeRadio with Spectrum Analyzer and choice of Filesystem and
optional Gestures or Touchscreen. 
<p />
Gele Radio means yellow radio, OranjeRadio means orange radio. 
<p />
User friendly install of VS1053 plugins and patches, just upload the .plg 
files to your SPIFFS/LittleFS/FFat flash disk.
<p />
An effort has been made to maximize use of PSRAM, if available.
<p />

Still a work in progress. 

<p />

Gele Radio uses the following libraries:
<ul>
 <li>ArduinoJSON ( https://github.com/bblanchon/ArduinoJson.git )</li>
 <li>Gesture_PAJ7620 ( https://github.com/Seeed-Studio/Gesture_PAJ7620 )</li>
 <li>ESP_VS1053 ( https://github.com/baldram/ESP_VS1053_Library )</li>
 <li>TFT_eSPI ( https://github.com/Bodmer/TFT_eSPI )</li>
 <li> AsyncTCP (https://github.com/me-no-dev/AsyncTCP)</li>
 <li>ESPAsyncWebServer (https://github.com/me-no-dev/ESPAsyncWebServer.git )</li>
 <li>If you want to use LittleFS, LittleFS_ESP32 ( https://github.com/lorol/LITTLEFS )</li>
</ul>
<p />

Grateful use has been made of the spectrum analyzer code of blotfi at
https://github.com/blotfi/ESP32-Radio-with-Spectrum-analyzer
<p />
Honourable mentions should be made for the support and inspiration taken from:
<ul>
<li>https://github.com/Aleks-Ale</li>
<li>https://github.com/Edzelf/ESP32-Radio</li>
<li>https://github.com/RalphBacon/205-Internet-Radio</li>
</ul>

Thanks to all the authors 

<h2>hardware</h2>
An ESP32 with plenty of PSRAM is recommended. For the touchscreen an ILI9341 320x240 screen is used.
All VS1053 breakouts should work ok, consult the 0pins.h file for a possible way of connecting this hardware.

<h2>compiling</h2>
Should compile with the Arduino EID, but make sure a file
wificredentials.h is available in folder wificredentials of you Arduino library folder.
This file should contain the following variables for GeleRadio, this is j
<p />

<code>
#ifndef WIFICREDENTIALS_H
#define WIFICREDENTIALS_H


// ntp
const char* ntpServers[]	= { "nl.pool.ntp.org", "ru.pool.ntp.org", "de.pool.ntp.org"};
//Use a valid TZ string, docs e.g. https://www.di-mgt.com.au/src/wclocktz.ini
//   
const char* ntpTimezone		= "CET-1CEST,M3.5.0/2,M10.5.0/3";



// Key and IV for G Radio used to encruypts wifi credentials for networks connected to by ESP touch
const char* gr_iv      = "DummyValyeWIthallkindsofNUmbers&****ssss";
const char* gr_key     = "OtherDummyValyeWIthallkindsof*&DGD";

#ifdef USEOWM
//openweathermap ( see https://openweathermap.org/current for docs )
const char	*owm_id   = "2756252";// 2756252 = eindhoven, nl. 532615 = moscow, ru
const char	*owm_key  = "123456789012345678901234567890"; // get your key at openweathermap.org
const char	*owm_unit = "metric"; 
const char	*owm_lang = "nl"; // en, ru....
#endif

#endif
</code>
 
In GeleRadio.ino a number of defines enable you to trun on or aff some features.
Generally they are called USE<option>. Be aware that not all possible combinations 
have been tested, and that a random combination of features may show random and undefined behaviour.

 
