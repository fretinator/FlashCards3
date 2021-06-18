# FlashCards3
  Flash Cards application with 
  Grove - 2x16 LCD RGB Backlight.

  This version uses a separate file for each card, and
  deletes ones you get correct, allowing you to make progress.
  
  I keep the following directory structure on the SD card
  
  /current/001.txt
          /002.txt
          ....
          995.txt
  /original/001.txt
            /002.txt
            /...
            995.txt
            
   When you are done, copy original back over to restart.
   

  Hardware needed:
    I'm using Wemos ESP32, but anthing with and enough inputs
    for hardware below should work
    SD Card - I'm using external with SPI. 
    Grove 2x16 LCD RGB Backlight. 
      You could moidfy for other screens.
    2 Buttons for correct and incorrect response.
      You decide if you are correct or not.

    Adjust FC_DELAY for you reading comfort.

    Program assumes files, /current/[your files] have alternating lines,
      first line is one language, next line is translation.
      While I am coding for Tagalog and English, you can use any
      2 languages. Set TagalogFirst to false if you want second 
      language displayed first.

  Copyright 2021 Tom Dison
  Licensed under GPL 3
  https://gnu.org/licenses/gpl-3.0.html
