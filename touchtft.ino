#ifdef USETOUCH2

// There are 10 bytes (5 integers @ 2 bytes each) for TFT calibration
#define totCalibrationBytes 10

//TFT_eSPI_Button muteBtn, brightBtn, dimBtn;
TFT_eSPI_Button muteBtn, listBtn, Back ;

//--------------------------------------------------------------------

touch_setup(){

   // Black icon, no text                       //x   y     w   h  outline     fill       textcolor  label      textsize
     touchbutton[BUTTON_MUTE].initButtonUL(&tft, 100, 140, 60, 60, TFT_YELLOW, TFT_BLACK, TFT_WHITE, (char *)"", 1);
     touchbutton[BUTTON_LIST].initButtonUL(&tft, 35,  140, 60, 60, TFT_YELLOW, TFT_BLACK, TFT_WHITE, (char *)"", 1);
     
     tft.setFreeFont(&indicator);
  
     drawMuteButton(false);
     drawMuteBitmap(false);
     drawListButton ();
     drawListBitmap (false);
    
  
}

//----------------------------------------------------------------
void drawMuteButton(bool invert)
{
  muteBtn.drawButton(invert);
}
//----------------------------------------------------------------
void drawMuteBitmap(bool isMuted)
{
  drawBmp(isMuted ? "/MuteIconOn2.bmp" : "/MuteIconOff2.bmp", 110, 150);
}
//----------------------------------------------------------------
void drawListButton ()
{
 listBtn.drawButton();
}
//----------------------------------------------------------------
void drawNextStnBitmap (bool pressed)
{
  drawBmp(pressed ? "/NextStnPressed.bmp": "/NextStn.bmp", 45, 150);  
}
//----------------------------------------------------------------
void drawListBitmap (bool pressed)
{
  drawBmp("/List.bmp", 45, 150);  
}
//----------------------------------------------------------------
void drawBackButton ()
{
 Back.drawButton();
 drawBmp("/Back.bmp", 175, 150 );
}
//----------------------------------------------------------------

// Mute button uses the Button class
static bool isMutedState = false;

void getMuteButtonPress()
{
  static unsigned long prevMillis = millis();

  // Only do this infrequently
  if (millis() - prevMillis > 150)
  {
    uint16_t t_x = 0, t_y = 0; // To store the touch coordinates

    // Update the last time we were here
    prevMillis = millis();

    // Pressed will be set true is there is a valid touch on the screen
    boolean pressed = tft.getTouch(&t_x, &t_y);

    // Check if any key coordinate boxes contain the touch coordinates
    if (pressed && muteBtn.contains(t_x, t_y))
    {
      muteBtn.press(true);
    }
    else
    {
      muteBtn.press(false);
    }

    if (muteBtn.justPressed())
    {
      isMutedState = !isMutedState;
    test_led_pin_val = !test_led_pin_val;
      log_d("Mute: %s", isMutedState ? "Muted" : "UNmuted");
      vs1053player->setVolume(isMutedState ? 0 : 90);
      drawMuteBitmap(isMutedState);
      digitalWrite(TEST_LED_PIN, test_led_pin_val);


    }
  }
}

//--------------------------------------------------------------------------------


void ListOnScreen()
{
     int listvoffset  = 30; // leave clock and volume on screen ( maybe 25 ?)
     int listhoffset = 10;
     int listw  = 220;
     int listh  = 200;
  
     int buttonw = 60;
     int buttonh = 60;
     int buttonx  =  listhoffset + listw - buttonw - 10;
     int buttony  =  listvoffset + listh  - buttonh - 10;
    
  
     tft.drawRoundRect(listhoffset, listvoffset, listw, listh, 5, TFT_RED);
     tft.drawString( "You can place", listhoffset+10, listvoffset+10 ); 
     tft.drawString( "something here.", listhoffset+10, listvoffset+20 ); 
      
     Back.initButtonUL(&tft, buttonx,buttony, buttonw, buttonh, TFT_YELLOW, TFT_BLACK, TFT_WHITE, (char *)"", 1);
     drawBackButton ();
       
}  

#endif
