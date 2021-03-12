# Gele Radio
<p />
Gele Radio means yellow radio. 
</p>
Bug fixed and updated OranjeRadio with spectrum analyzer and choice of filesystem and
optional gesture and touchscreen control and openweathermap data. User friendly web interface
for easy addition of internet radio stations. https and http radio stations, chunked tranfer
encoding supported. Also artist and track information is collected, if available.
Latency is low by design.
<p />
Gele radio was greatly improved and extended due to the generous support, useful suggestions 
and rigorous testing of Alexander Semenov.
<p />
Playlists ( .pls, .m3u, .dash etc.) are not supported. 
No plans exist to support these, but this may change one day.  
<p />
User friendly install of VS1053 plugins and patches, just upload the .plg 
files to your SPIFFS/LittleFS/FFat flash disk.
<ul>
<li>plg files for patches can be downloaded from the VLSI website.
http://www.vlsi.fi/en/support/software/vs10xxpatches.html
</li>
<li>plg files for plugins ( like the spectrum analyzer plugin) can be downloaded here:
http://www.vlsi.fi/en/support/software/vs10xxplugins.html
</li>
<li> The VS10xx series is very interesting for more ideas:
http://www.vlsi.fi/en/support/software.html
</li>
</ul>

<p />
An effort has been made to maximize use of PSRAM, if available.
<p />

Gele Radio uses the following libraries:
<ul>
 <li>ArduinoJSON ( https://github.com/bblanchon/ArduinoJson.git )</li>
 <li>Gesture_PAJ7620 ( https://github.com/Seeed-Studio/Gesture_PAJ7620 )</li>
 <li>ESP_VS1053 ( https://github.com/baldram/ESP_VS1053_Library )</li>
 <li>TFT_eSPI ( https://github.com/Bodmer/TFT_eSPI )</li>
 <li> AsyncTCP (https://github.com/me-no-dev/AsyncTCP)</li>
 <li>ESPAsyncWebServer (https://github.com/me-no-dev/ESPAsyncWebServer.git )</li>
 <li>If you want to use LittleFS (recommended), LittleFS_ESP32 ( https://github.com/lorol/LITTLEFS )</li>
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
An ESP32 with plenty of PSRAM is recommended. For the touchscreen an ILI9341 320x240 SPI ( NOT parallel, you 
will run out of pins) screen is used. All VS1053 breakouts should work ok, consult the 0pins.h file for a 
possible way of connecting this hardware.

<h2>compiling</h2>
Should compile with the Arduino IDE, but make sure a file
wificredentials.h is available in folder wificredentials of your Arduino library folder.
( credits: Andres Spiess https://www.youtube.com/channel/UCu7_D0o48KbfhpEohoP7YSQ )
This file should contain the following variables for GeleRadio.
<p />

<pre>
#ifndef WIFICREDENTIALS_H
#define WIFICREDENTIALS_H


// ntp
const char* ntpServers[]	= { "nl.pool.ntp.org", "ru.pool.ntp.org", "de.pool.ntp.org"};
//Use a valid TZ string, docs e.g. https://www.di-mgt.com.au/src/wclocktz.ini
//   
const char* ntpTimezone		= "CET-1CEST,M3.5.0/2,M10.5.0/3";



// Key and IV for G Radio used to encrypt wifi credentials for networks connected to by ESP touch
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
</pre>
 
In GeleRadio.ino a number of defines enable you to turn on or off some features.
Generally they are called USE<option>. Be aware that not all possible combinations 
have been tested (although most have) , and that a random combination of features may 
show random and undefined behaviour.
Currently, the following options are defined:
<pre>
/*define or undef */
#define USEOWM // to enable Open weathermap owm_key, 
               // owm_id (city id), owm_lang (language),
               //owm_unit (metric, imperial) in wificredentials.h
               // should be defined as const char *, e.g.
               // const char* owm_unit = "metric";

#undef  USEOTA      // disable Arduino OTA. http update is always on and works 
                    // as well.
#define USETLS 1    // allow for https
#undef  USEPIXELS   // use pixels as an indicator for the gestures
#define USEGESTURES // Use the PAJ7620 gesture sensor
#undef  MULTILEVELGESTURES // gestures as used in Oranje radio. Not very well tested or maintained
#define USETOUCH       // use a touchscreen. Tested and developed with an ILI9341
#define USEINPUTSELECT // input selection between AV(LINE IN), BLUETOOTH and RADIO
                       // if undefined, volume buttons are displayed on the touchscreen, otherwise 
                       // buttons to select BLUETOOTH and AV (LINE IN) 
#define USESPECTRUM // install and run the spectrum patch as supplied by VLSI
                    // and gracefully adapted from the Web Radio of Blotfi
#define SHOWMETA    // show meta data ( artist/track info in variable meta.metadata ) in the default
                    // place under the station name.</pre>
</pre>
<h2>sundry</h2>
New url's for BBC stations that work for GeleRadio have been found here:
https://gist.github.com/bpsib/67089b959e4fa898af69fea59ad74bc3
