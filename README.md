# Snow Radio 
(Снежное радио - Sneeuwradio)


</p>
Bug fixed and updated OranjeRadio with spectrum analyzer and choice of filesystem and
optional gesture and touchscreen control and openweathermap data. User friendly web interface
for easy addition of internet radio stations. https and http radio stations, chunked tranfer
encoding supported. Also artist and track information is collected  if available and shown both
on the display and in the web browser.
Latency is low by design.
<p />
Snow Radio was greatly improved and extended due to the generous support, useful suggestions 
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
<li> The VS10xx series website is very interesting for more ideas:
http://www.vlsi.fi/en/support/software.html
</li>
</ul>

<p />
An effort has been made to maximize use of PSRAM, if available.
<p />

Snow Radio uses the following libraries:
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
<p />
In the current version, a separate SPI bus is used for the VS1053 (VSPI) and the screen (HSPI).
This works but so far, there is no evidence this improves speed or reliability, so feel free
to operate both devices on the same SPI bus. 

<h2>compiling</h2>
Should compile with the Arduino IDE, but make sure a file
wificredentials.h is available in folder wificredentials of your Arduino library folder.
(credits: Andreas Spiess https://www.youtube.com/channel/UCu7_D0o48KbfhpEohoP7YSQ )
This file should contain the following variables for SnowRadio.
<p />

<pre>
#ifndef WIFICREDENTIALS_H
#define WIFICREDENTIALS_H
// optional,  necessary if LOADSSIDS is defined and used
const char* wifiSsid[]      =  {"yourssid", "yourotherssid","yourmotherssid", ""};
const char* wifiPassword[]  =  ("yourWiFipassword", "yourotherssidpassword","yourmotherssidpassword", ""} ;

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
 
In SnowRadio.ino a number of defines enable you to turn on or off some features.
Generally they are called USE<option>. Be aware that not all possible combinations 
have been tested (although many have) , and that a random combination of features may 
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
                    // place under the station name.
#define METASPRITE 1// use a sprite to display metadata and scroll the sprite if text is too long
                    // in some configurations this has been seen to cause garbage on the screen for unknown reasons.                    
#define USESPTOUCH 1  //use the ESP TOUCH phone app to connect when no known WiFi network is seen. 
                      //for some this is very user friendly, for others it is a source of frustration. 
#undef LOADSSIDS      // If you want to preload the encrypted netpass file with ssid and passwords 
                      // define LOADSSIDS. If not using ESPTOUCH, this will be a necessary step.
                      // For this to work, in wificredentials.h,  
                      // WiFiNetworks are defined as follows, make sure the arrays have a "" as last element
                      // const char* wifiSsid[]      =  {"yourssid", "yourotherssid","yourmotherssid", ""};
                      // const char* wifiPassword[]  =  ("yourWiFipassword", "yourotherssidpassword","yourmotherssidpassword", ""} ;
                      // ATTENTION: After an encrypted /netpass file is generated, it is better to recompile without this option, the loading 
                      // is not necessary anymore but more importantly, the credentials will remain readable in the compiled code. 
                      //                    
</pre>
<h2>faq</h2>
<ul>
<li>Touch calibration <br />
    When attaching a touch display,touch calibration data for an other display will not work properly or not at all.
    Open a browser and connect to URL snowradio.local, or to the ip address of Snow radio and click on button
    "Delete touch calibration", then on the button "Reboot". When the Snow radio restarts, it ask you to touch 
    the 4 corners of the display. If done correctly, the display should work.
</li>
<li>Only one SSL connection <br />
    The Snow radio can receive https Internet radio stations. However, when experimenting, bear in 
    mind that Arduino does not allow for SSL to use PSRAM. For Snow radio this means that more than 1 SSL ( https) 
    connection is not possible. This also makes it unlikely newer protocols like dash or HLS can ever be supported.
</li>
<li>Gesture sensor <br />
    Gestures work as follows:
    <ul>
    <li>circle clockwise or anti-clockwise: wake up sensor ( a little hand shows up on the screen) 
        or put to sleep if active (the little hand disappears)</li>
    <li>up: increase volume</li>
    <li>down: decrease volume</li>
    <li>right: go to next station</li>
    <li>left: go to previous station</li>
    </ul>
    
    Sometimes, especially after a new flash of the ESP32, the gesture sensor does not react anymore. Only known fix
    is a power on/power off of both the ESP32 and the PAJ7620. In other circumstances this hardly ever occurs.
<li>When connection to an internet radio station fails, the radio connects to the next station in the list. This
    could surprise you, but is intentional.
</li>
<li>Restart at failing connection <br />
    When reception of the internet radio is bad, the radio connot supply the VS1053 with enough data to supply sound.
    As a last resort the radio will then restart. This usually solves the issue, but could in some extraordinary cases
    lead to a reboot loop. Via the web page or the screen you can try to switch to another station before the reboot 
    occurs again. If this doesn't work a reflash of the filesystem can be the only way out, but this will also mean
    that more data is lost, like your last station, last volume and tone settiongs, your display calibration data and your 
    netpass file. 
</li>
<li>Backup and restore<br/>
    For the netpassfile and the statonlist there are buttons on the webpage to easily backup these files.
    All other files can be downloaded and save by typing http://snowradio.local/<filename>?download=1 in your browser.
    Use the "Directory" button to see which files you may want to backup. <br/>
    Restoring files can be done by a complete flash of the file system, or by using the "Upload file" button.
    Make sure you enter the full path of the file, including the preceding /
    
</li>
<li>Delete files<br />
    Files can be can be deleted by typing in this URL: http://snowradio/delete?file=<filename>, 
    e.g. http://snowradio/delete?file=/netpass
</li>
<li>File system <br />
    For development LittleFS has been used, but the Radio can work with FFAT and SPIFFS as well. We recommend LittleFS 
    for it's robustness and better locking of files. Nevertheless, in SnowRadio.ino
    there are options to set the filesystem used. Just uncomment the one you prefer:
    <pre>
        //choose file system
        //
        //fs::FS      RadioFS     = SPIFFS;
        //const int   RadioFSNO   = FSNO_SPIFFS;
        //const char  *RadioMount = "/spiffs";

        fs::FS      RadioFS     = LITTLEFS;
        const int   RadioFSNO   = FSNO_LITTLEFS;
        const char  *RadioMount = "/littlefs";

        //fs::FS      RadioFS     = FFat;
        //const int   RadioFSNO   = FSNO_FFAT;
        //const char  *RadioMount = "/ffat";
    
</li>
<li>Pins<br />
    In file 0pins.h an example configuration of the pins can be found. Pins for the display and
    the touch sensor should be defined in the tft_eSPI setup file for your display. The  option to 
    use input selection ( switch to bluetooth/line-in ) has been realized with an I2C port extender, 
    the MCP23017. The code to initialize and use the MCP23017 has not been included.
</li>
</ul>

    
<h2>sundry</h2>

A very comprehensive and meticulously maintained website with more internet radio
stations is :
https://www.hendrikjansen.nl/henk/streaming.html

