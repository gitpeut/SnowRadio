
void drawBmp(const char *filename, int16_t x, int16_t y, TFT_eSprite *sprite=NULL );

// to display image in sprite, provide poiter to sprite. 
// to display on screen, omit this argument or fill it with NULL );
// display in sprite:
//  TFT_eSprite sprite;
//  drawBmp( "/tets.bmp", 5,5, &sprite); 
// display on screen:
//  tft ( will use TFT_eSPI tft variable declared globally )
//  drawBmp("/OranjeRadio24.bmp", 55, 15 );
