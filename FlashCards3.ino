// include the SD library:
#include <SD.h>
#include <SPI.h>

// For display
#include <rgb_lcd.h>
#include <Wire.h>

/*
  Flash Cards application with 
  Grove - 2x16 LCD RGB Backlight.

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
 */

#define LINE_1 0
#define LINE_2 1
#define MAX_LINE_LEN 255
char fcBuffer[MAX_LINE_LEN + 1]; // room for \0
const int  FC_DELAY = 2 * 1000;
int curCardNum = 0;

struct FlashCard {
  String Tagalog;
  String English;
  char fName[32]; //e.g. /original/001.txt
};

bool TagalogFirst = true;

FlashCard currentCard;

// LCD constants
const int colorR = 255;
const int colorG = 0;
const int colorB = 0;
const int colorR_Answer = 0;
const int colorG_Answer = 255;
const int colorB_Answer = 0;

#define CS_SELECT_PIN 5 // Set for you SD card
#define BUTTON_NEXT_PIN 33 // set for correct button
#define BUTTON_INCORRECT_PIN 34 // set for incorrect button
int lastMillis = 0;
const int buttonTestDelay = 100;

const int SCREEN_ROWS = 2;
const int SCREEN_COLS = 16;
const String TRUNC_CHARS = "...";
const String ORIG_DIR = "/original/"; // original copy of cards
const String CURR_DIR = "/current"; //current working copy of cards

rgb_lcd lcd;

File fcCurDir;
File fcFile;

const bool is_debug = true;

// Lines are 1 based
void printScreen(const char* line, bool cls = true, int whichLine = 1, bool isAnswer = false) {
  // Clear screen
  if(cls) {
    lcd.clear();
  }

  if(isAnswer) {
    lcd.setRGB(colorR_Answer, colorG_Answer, colorB_Answer);
  } else {
    lcd.setRGB(colorR, colorG, colorB);
  }
  
  lcd.setCursor(0,whichLine - 1  );
  lcd.print(line);
}

void debugOut(String value, bool addLF = true) {
  if(is_debug) {
    if(addLF) {
      Serial.println(value);
    } else {
      Serial.print(value);
    }
  }
}

void setupLCD() {
  // set up the LCD's number of columns and rows:
  lcd.begin(SCREEN_COLS, SCREEN_ROWS);
  
  lcd.setRGB(colorR, colorG, colorB);
  
  // Print a message to the LCD.
  displayString("FlashCards by Tom Dison", true);
}

bool setupSDCard() {
    debugOut("\nInitializing SD card...", false);


  // we'll use the initialization code from the utility libraries
  // since we're just testing if the card is working!
  if (!SD.begin(CS_SELECT_PIN)) {
    debugOut("initialization failed. Things to check:");
    debugOut("* is a card inserted?");
    debugOut("* is your wiring correct?");
    debugOut("* did you change the chipSelect pin to match your shield or module?");
    return false;
  } else {
   debugOut("Wiring is correct and a card is present.");
  }

  return true;
}

void setup()
{
 // Open serial communications and wait for port to open:
  if(is_debug) {
    Serial.begin(9600);
     
    while (!Serial) {
      ; // wait for serial port to connect.
    }
  }

  currentCard.Tagalog = "";
  currentCard.English = "";
  currentCard.fName[0] = '0';

  pinMode(BUTTON_NEXT_PIN, INPUT);
  pinMode(BUTTON_INCORRECT_PIN, INPUT);
  setupLCD();
  
  if(!setupSDCard()) {
    printScreen("Failed to access SD card!", true);
    debugOut("Failed to access SD card!");
    while(1); // don't continue
  }

  fcCurDir = SD.open(CURR_DIR, FILE_READ);

  if(!fcCurDir) {
    printScreen("Could not open flash card files!");
    debugOut("Failed to access SD card!");
    while(1);
  }

  fcCurDir.rewindDirectory();
}

bool isLineTerminator(char c) {
  return '\r' == c || '\n' == c;
}

String readLine() {
  int charsRead = 0;

  while(!isLineTerminator(fcFile.peek())
  && charsRead <= MAX_LINE_LEN && fcFile.available()) {
     fcBuffer[charsRead] = fcFile.read();
     charsRead++; // Yes could be done in one line
  }

  fcBuffer[charsRead] = '\0';

  // Now remove line terminator(s)
  while(fcFile.available() && isLineTerminator(fcFile.peek())) {
    fcFile.read(); // discard
    debugOut("Discarded terminator");
  }
  
  return String(fcBuffer);
}

void readNextCard() {
  currentCard.Tagalog = "";
  currentCard.English = "";

  fcFile = fcCurDir.openNextFile();

  if(!fcFile) {
    //
    fcCurDir.rewindDirectory();
    fcFile = fcCurDir.openNextFile();

    if(!fcFile) {
      debugOut("All files completed", true);
      displayString("All cards completed!", true);
      while(1);
    }
  }

  currentCard.Tagalog = readLine();
  currentCard.English = readLine();
  strcpy(currentCard.fName, fcFile.name());
  
  debugOut("Current Card File is ", false);
  debugOut(currentCard.fName, true);
  debugOut(currentCard.Tagalog);
  debugOut(currentCard.English);
  
  fcFile.flush();
  fcFile.close();
}

int getNextChunkPos(int startPos, String value, int max_chars,
    bool truncate) {
  int spacePos = 0;
  int lastPos = startPos + max_chars;
  
  if(lastPos > value.length()) {
    lastPos = value.length();
  } else {
    spacePos = value.lastIndexOf(' ', lastPos);
    if(spacePos != -1 && spacePos > startPos) {
      lastPos = spacePos;
    }

    if(truncate && 
        ((lastPos - startPos) > (max_chars - TRUNC_CHARS.length()))) {
      // We need to allow room for ...
      spacePos = value.lastIndexOf(' ', lastPos - 1);
      
      if(spacePos != -1 && spacePos > startPos) {
        lastPos = spacePos; 
      }
    }
  }

  return lastPos;
}

void displayString(String value, bool isAnswer) {
  int startPos = 0;
  int curLine = 1;
  int lastPos = 0;
  bool moreChunks = true;
  String line = "";

  while(moreChunks) {
    lastPos = getNextChunkPos(startPos, value, SCREEN_COLS, curLine == SCREEN_ROWS);

    if(lastPos == -1) {
      moreChunks = false;
    } else {
      moreChunks = lastPos < value.length();

      if(curLine == SCREEN_ROWS && moreChunks) {
        line = value.substring(startPos, lastPos) + TRUNC_CHARS;
      } else {
        line = value.substring(startPos, lastPos);
      }

      printScreen(line.c_str(), curLine == 1, curLine, isAnswer);
  
      if(moreChunks) {
        startPos = lastPos;
        
        if(value.charAt(startPos) == ' ') {
          startPos++;
        }
        
        if(++curLine == SCREEN_ROWS + 1) {
          curLine = 1;
          delay(FC_DELAY); // Leave this part of the verse up
        }
      } else {
        if(!isAnswer) {
            delay(FC_DELAY); // we are done with card
        }
      }
    }
  }
}


bool displayCard() {
  if(currentCard.Tagalog.length() == 0 ||
      currentCard.English.length() == 0) 
  {
    debugOut("Empty Flash Card!");
    return false;
  }

  if(TagalogFirst) {
    displayString(currentCard.Tagalog.c_str(), false);
    displayString(currentCard.English.c_str(), true);
  } else {
    displayString(currentCard.English.c_str(), false);
    displayString(currentCard.Tagalog.c_str(), true);
  }

  return true;
}

bool showNextCard() {
  debugOut("Reading next Card");
  readNextCard();

  curCardNum++;
  
  bool result = displayCard();
  debugOut("Result of display card is: ", false);
  debugOut(String(result)); 

  return result;
}

void doNext(boolean deleteCard) {
  if(deleteCard) {
    if(strlen(currentCard.fName) > 0 && SD.exists(currentCard.fName)) {
      bool result = SD.remove(currentCard.fName);
      debugOut("Deleting file ", false);
      debugOut(currentCard.fName, false);
      debugOut(result ? " succeeded." : " failed.", true);
    }
  }
  
  if(!showNextCard()) {
    printScreen("Error showing flash cards!");
    debugOut("Error showing flash cards");
    while(1);    
  }
}

void loop(void) {
   int curMillis = millis();
    
   if((curMillis - lastMillis) > buttonTestDelay) {
    lastMillis = curMillis;
    int test = digitalRead(BUTTON_NEXT_PIN);

    if(test == LOW) {
      Serial.println(" Next Pressed");
      // We got it right, delete the card
      doNext(true);
    } else {
      test = digitalRead(BUTTON_INCORRECT_PIN);
      if(test == LOW) {
        Serial.println("Incorrect Pressed");
        // Leave the card in the rotation
        doNext(false);
      }
    }
  }
}
