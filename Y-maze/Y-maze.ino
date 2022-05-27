

# include <Keypad.h>
# include "U8glib.h"
# include <EEPROM.h> 
# include <Servo.h>
# include <string.h>

// setup u8g object
U8GLIB_ST7920_128X64_1X u8g(45, 43, 41);  // SPI Com: SCK = en = 45, MOSI = rw = 43, CS = di = 41
/*
 * Display connection:
 * GND - GND
 * VCC - 5v
 * RS  - 41
 * R/W - 43
 * E   - 45
 * PSB - GND
 * BLA - 3.3v
 * BLK - GND
 */

// keypad pins, left to right: 22 23 26 27 30 31 34 35

// keypad initialization 
const byte ROWS = 4; 
const byte COLS = 4; 
char keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};
byte rowPins[ROWS] = {35, 37, 31, 33};    //connect to the row pinouts of the keypad (right 4 pins of the keypad)
byte colPins[COLS] = {23, 25, 27, 29};    //connect to the column pinouts of the keypad (left 4 pins of the keypad)
Keypad keypad = Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS );

// LED pins initialization
const int left = 2;
const int mid1 = 5;
const int mid2 = 4;
const int mid3 = 3;
const int ced = 49;
const int right = 6;
const int door = 44;

int stp = 11;           // display's page 11 - first page: select program / new program
int stpPlay = 0;        // step of program execution in experiment
int curChoice = 0;      // Number of the chosen program in menu
int side;               // left/right
int sameSide = 0;       // number of times light went on the same side, useful for random side choice
boolean prevSide = 0;
boolean ratReady;

Servo servo;      // holds the door

// the way each program is saved in memory
struct program {
  int delays[5];        // 5 delay times in each program
  boolean door_open;    // if door needs to be opened 
};
int numProgs = 0;       // nember of programs in memory
program prg[20];        // array of all programs, max 20

void setup() {
  u8g.setFont(u8g_font_helvR08);

  servo.attach(7);
  
  pinMode(left, OUTPUT);
  pinMode(mid1, OUTPUT); 
  pinMode(mid2, OUTPUT); 
  pinMode(mid3, OUTPUT); 
  pinMode(right, OUTPUT); 
  pinMode(door, OUTPUT);  
  pinMode(evChan, OUTPUT);

  randomSeed(analogRead(0));
  
  EEPROM.get(0, numProgs); // zero byte of memory holds the number of saved program
  if (numProgs > 200) numProgs = 0;
  int address = sizeof(int);
  if (numProgs > 0){
    for (int i = 0; i < numProgs; i++){ // fill array with programs from memory
      EEPROM.get(address, prg[i]);
      address += sizeof(program);
    }
  }
}

void loop() {
  
  /* circuit check
  digitalWrite(left, HIGH);
  digitalWrite(mid1, HIGH);
  digitalWrite(mid2, HIGH);
  digitalWrite(mid3, HIGH);
  digitalWrite(right, HIGH);
  */
  

  u8g.firstPage();  
  do {
    if (stp == 14) drawPlay();               // program execution screen 
    else if (stp == 13) drawReady();         // preparation screen
    else if (stp < 10) drawNewProgram();     // typing new program
    else if (stp == 12) drawSelectProgram(); // existing program selection screen
    else if (stp == 11) drawNewOrOld();      // first screen: type new program or choose an existing one
  } while( u8g.nextPage() );  
  
  if (stp == 14) play();
  else if (stp == 13) readyPlay();
  else if (stp < 10) newProgram();  
  else if (stp == 12) selectProgram();
  else if (stp == 11) newOrOld();
  
}

void drawNewOrOld(){
  u8g.drawStr(0, 12, "Select program");
  u8g.drawStr(0, 24, "New program");
  u8g.drawStr(120, 12, "A");          // choose Select program
  u8g.drawStr(120, 24, "D");          // choose New program
}

void newOrOld(){
  char key;
  do {
    key = keypad.getKey();
  } while (key == NO_KEY);
  switch (key){
    case 'D':
      stp = 0;                  // First step of typing a new program in
      curChoice = numProgs;     // New program will be added to memory
      break;
    case 'A':
      stp = 12;                 // Step of program selection
      curChoice = 0;            // Default program choice is 1
      break;
  }
}

void drawSelectProgram(){
  // menu
  u8g.drawStr(105, 48, "up A");
  u8g.drawStr(90, 60, "down D");
  u8g.drawStr(0, 48, "#   select");
  u8g.drawStr(0, 60, "*   delete");  
  u8g.drawStr(45, 60, "B - back");

  //list of programs
  char n_progStr[24] = "";                       // Hold programs to display and their number
  if (curChoice > 0) {                           // Draw previous program in the list
    itoa(curChoice, n_progStr, 10);
    strcat(n_progStr, ". ");
    strcat(n_progStr, drawProgram(curChoice-1));
    u8g.drawStr(1, 12, n_progStr);
  }
  if (numProgs > 0) {                            // Draw current selected program
    itoa(curChoice+1, n_progStr, 10);
    strcat(n_progStr, ". ");
    strcat(n_progStr, drawProgram(curChoice));
    u8g.drawStr(1, 24, n_progStr);
  }
  if (numProgs > 1 && curChoice < numProgs-1) {  // Draw next program in the list
    itoa(curChoice+2, n_progStr, 10);
    strcat(n_progStr, ". ");
    strcat(n_progStr, drawProgram(curChoice+1));
    u8g.drawStr(1, 36, n_progStr);
  }
  u8g.drawFrame(0, 13, 115, 14);                 // Frame over selected program

  char numProgsStr[4];
  itoa(numProgs, numProgsStr, 10);
  u8g.drawStr(128 - strlen(numProgsStr)*8, 12, numProgsStr);    // wright number of saved programs in the upper right corner
 
}

boolean wasNumber = false;
void selectProgram(){
  char key;
  do {
    key = keypad.getKey();
  } while (key == NO_KEY);
  if (key >= '0' && key <= '9'){        // if is number, select program by typing it's number
    if (!wasNumber) curChoice = -1;
    curChoice = (curChoice+1)*10 + key - 1 - '0';
    if (curChoice > numProgs - 1) curChoice = numProgs - 1;  // choose the last program in the list 
    if (curChoice < 0) curChoice = 0;                        // choose the first program
    wasNumber = true;
    return;
  }
  switch(key){
    case 'A':                     // up
      if (curChoice == 0 && numProgs > 1) curChoice = numProgs-1;
      else if (curChoice > 0) curChoice--;
      wasNumber = false;
      break;
    case 'D':                     // down
      if (curChoice == numProgs-1) curChoice = 0;
      else if (curChoice < numProgs-1) curChoice++;
      wasNumber = false;
      break;
    case '#':                     // play an existing program
      if (numProgs == 0) break;
      stp = 13;
      ratReady = false;
      break;
    case '*':                     // delete program
      deleteProgram(curChoice);
      break;
    case 'B':                     // back
      stp = 11;
      break;
  }
}

char prgStr[24];
char* drawProgram(int n){           // draw program string number n
  // [0] M [1] m [2] S [3] s [4] D
  prgStr[0] = NULL;
  char timeStr[4];        // holds each time as string, max 3 digits
  strcpy(prgStr, itoa(prg[n].delays[0], timeStr, 10));  // time before middle LED on
  strcat(prgStr, "M");                                  // middle LED on
  strcat(prgStr, itoa(prg[n].delays[1], timeStr, 10));  // time of middle LED on
  strcat(prgStr, "m");                                  // middle LED off
  strcat(prgStr, itoa(prg[n].delays[2], timeStr, 10));  // time after middle LED off
  strcat(prgStr, "S");                                  // side LED on
  strcat(prgStr, itoa(prg[n].delays[3], timeStr, 10));  // time of side LED on
  strcat(prgStr, "s");                                  // side LED off
  strcat(prgStr, itoa(prg[n].delays[4], timeStr, 10));  // time after side LED off
  if (prg[n].door_open) strcat(prgStr, "D");            // open the door (if needed)
  return prgStr;
}

/*
char* makeChar(String str){
  int lineLength = str.length() + 1; 
  char * buf = (char *) malloc (lineLength);
  char charline[lineLength];
  str.toCharArray(charline, lineLength); 
  strcpy (buf, charline);
  return buf;
}
*/

boolean wasDigit = false;
char newProgStr[24] = "";
void drawNewProgram(){ 
  u8g.drawStr(1, 12, newProgStr);
  u8g.drawStr(118, 62, "B");            // B - back
  u8g.drawFrame(116, 52, 12, 12);       // frame around B
  switch(stp){                          // on typing each program step this function will be called again 
    case 0:                                    // and show different hints on the current parameter to type
      u8g.drawStr(0, 36, "Enter delay before middle");
      if (wasDigit) u8g.drawStr(0, 48, "A - finish entering"); 
      break;
    case 1:
      u8g.drawStr(0, 36, "Enter time of middle");
      if (wasDigit) u8g.drawStr(0, 48, "A - finish entering"); 
      break;
    case 2:
      u8g.drawStr(0, 36, "Enter delay before side");
      if (wasDigit) u8g.drawStr(0, 48, "C - finish entering");
      break;
    case 3:
      u8g.drawStr(0, 36, "Enter time of side");
      if (wasDigit) u8g.drawStr(0, 48, "C - finish entering");
      break;
    case 4:
      u8g.drawStr(0, 36, "Enter delay after side");
      if (wasDigit) {
        u8g.drawStr(0, 48, "D - end enter, door");
        u8g.drawStr(0, 60, "* - end enter, no door");
      }
      break;
    case 5:
      u8g.drawStr(0, 60, "# - save and select");
      break;
  }
}

void newProgram(){ 
  // [0] M [1] * [2] L/R [3] * [4] D
  char key[2];
  for (;;){                   // we'll type until typed smth correct
    key[0] = keypad.getKey();
    if (key[0] == 'B'){       // back
      stp = 11;               // ... to the first screen
      newProgStr[0] = NULL;
      return;
    }
    if (key[0] >= '0' && key[0] <= '9'){        // type the time of delay
      prg[curChoice].delays[stp] = prg[curChoice].delays[stp] * 10 + key[0] - '0';    // pass number to structure
      wasDigit = true;
      strcat(newProgStr, key);      // pass number to program string
      return;
    }
    switch (stp){
      case 0: // type delay before middle LED
        if (key[0] != 'A' || !wasDigit) continue; 
        strcat(newProgStr, "M");
        break;
      case 1: // type time of middle LED
        if (key[0] != 'A' || !wasDigit) continue;
        strcat(newProgStr, "m");
        break;
      case 2: // type delay after middle LED
        if (key[0] != 'C' || !wasDigit) continue;
        strcat(newProgStr, "S");
        break;
      case 3: // type time of side LED
        if (key[0] != 'C' || !wasDigit) continue;
        strcat(newProgStr, "s");
        break;
      case 4: // type delay after side LED
        if (key[0] != 'D' && key[0] != '*' || !wasDigit) continue; // wrong key, try again
        if (key[0] == 'D') prg[curChoice].door_open = true;        // door will open
        strcat(newProgStr, "D");
        break;
      case 5:
        if (key[0] != '#') continue; // wrong key, try again (only # available on this step)
        stp = 13;                    // next screen - experiment preparation
        ratReady = false;
        // send new program to memory
        int address = sizeof(int) + sizeof(program)*numProgs;
        EEPROM.put(address, prg[curChoice]);
        address += sizeof(program);
        numProgs++;
        EEPROM.update(0, numProgs);
        return;
    } // end switch
    wasDigit = false;
    stp++;
    return;
  } // end for  
} // end newKey()

int pickSide(){
  boolean newSide = random()%2 == 0;
  if (newSide == prevSide){
    sameSide++;

    /*   !!! This is an important parameter if you want to let program choose side randomly !!!
     *   Letting light go on the same side more than 3 times in a raw can give animal wrong clue 
     *        and cause further mistakes
     *   For DAT-KO rats parameter 2 may be suitable
     *   Change the following number according to your experiment
     */
    if (sameSide >= 3){     // no more than THREE times on the same side in a raw
      newSide = !newSide;
      sameSide = 0;
    }
  }
  prevSide = newSide;
  if (newSide) return(left);
  else return(right);
}

void deleteProgram(int n){
  if (numProgs == 0) return;
  int address = sizeof(int) + sizeof(program)*n; // address of the beginning of the program next to deleted one
  for (int i = n; i < numProgs-1; i++){          // move all next programs one place left
    prg[i] = prg[i+1];
    EEPROM.put(address, prg[i]);
    address += sizeof(program);
  }
  if (curChoice == numProgs-1) curChoice--;      // if the deleted frogram was the last one - move curChoice to previous one
  numProgs--;
  EEPROM.update(0, numProgs);
}

int done;
void drawReady(){                                // experiment preparation screen 
  // [0] M [1] m [2] L/R [3] s [4] D
  // int n = curChoice;
  /*
  String drawSide;
  String progStr = String(' ') + prg[n].delays[0] + "M" + prg[n].delays[1] + "m" + prg[n].delays[2] + "S"
             + prg[n].delays[3] + "s" + prg[n].delays[4];
  u8g.drawStr(1, 12, makeChar(progStr));
  */
  u8g.drawStr(1, 12, drawProgram(curChoice));    // draw current program on top of the screen 
  if (!ratReady) {                               // place rat behind the door, lift the door
    u8g.drawStr(2, 48, "Set rat and door");
    u8g.drawStr(2, 60, "# - ready");             // type # when rat is ready, servo will hold the door
  }
  else {
    u8g.drawStr(0, 24, "Choose side of signal:");     // manually or randomly
    u8g.drawStr(0, 36, "A - left");
    u8g.drawStr(0, 48, "C - right");
    u8g.drawStr(0, 60, "* - random");                 
  }
  u8g.drawStr(90, 60, "B - back");  
}

void readyPlay(){
  char key;
  side = NULL;
  do{
    key = keypad.getKey();
  } while (key == NO_KEY);
  if (key == 'B' && !ratReady) {              // back - return to program selection
    stp = 12;
    curChoice = 0;
    return;
  }
  else if (!ratReady && key == '#'){          // # - close the door before experiment
      servo.write(0);
      ratReady = true;
      stpPlay = 0;
      done = 0;
      return;
  }
  else if (key == 'B' && ratReady){           // back - open the door again
    ratReady = false;
    servo.write(30);
    return;
  }
  else if (ratReady){
    if (key == 'A') {
      side = left;
      stp = 14;                               // go to program execution
      stpPlay = 0;
    }
    else if (key == 'C') {
      side = right;
      stp = 14;
      stpPlay = 0;
    }
    else if (key == '*') {
      side = pickSide();
      stp = 14;
      stpPlay = 0;
    }
  }
  else return;  
}

void drawPlay(){
  // [0] M [1] m [2] L/R [3] s [4] D
  /*
  int n = curChoice;
  String drawSide;
  String progStr = String(' ') + prg[n].delays[0] + "M" + prg[n].delays[1] + "m" + prg[n].delays[2] + "S"
             + prg[n].delays[3] + "s" + prg[n].delays[4];
  if (prg[n].door_open) progStr += "D";
  */
  u8g.drawStr(1, 12, drawProgram(curChoice));
  if (side == left) u8g.drawStr(120, 12, "L");          // chosen (by you or program) side is shown in upper right corner
  else if (side == right) u8g.drawStr(120, 12, "R");              // place food accordingly
  u8g.drawHLine(1, 13, done);                           // underline executed part of program
  //u8g.drawStr(1, 12, makeChar(progStr));
  if (done == 0) {                                      // execution haven't started yet
    u8g.drawStr(2, 60, "# - start");                    // start experimemt 
    u8g.drawStr(90, 60, "B - back");                    // back to preparation
  }
}

void play(){ // проведение эксперимента
  // [0] M [1] m [2] L/R [3] l/r [4] D
  int n = curChoice;
  char timeStr[4];
  int prec = 1000; // time precision: 1000 - seconds, 100 - tenth of a second 
  char key;
  
  switch (stpPlay){
    case 0:
      do{
        key = keypad.getKey();
      } while (key != 'B' && key != '#');       // type until pressed B or # 
      if (key == 'B') {                         // back to preparation step
        stp = 13;
        return;
      }
      else if (key == '#'){                     // start experiment, no way to stop it now
        itoa(prg[n].delays[0], timeStr, 10);
        done += u8g.getStrWidth(timeStr);       // length of line underlining executed part of program
        stpPlay++;                              // next step of experiment execution
        return;
      }
      break;
    case 1:
      delay(prg[n].delays[0]*prec);  // delay before middle LED
      itoa(prg[n].delays[1], timeStr, 10);
      done += u8g.getStrWidth("M") + u8g.getStrWidth(timeStr);
      break;
    case 2:
      if (prg[n].delays[1] != 0){    // if time of middle LED is 0 it won't go on
        EventChannel();                       // signal to event channel
        digitalWrite(mid1, HIGH);    // middle LED on
        digitalWrite(mid2, HIGH);
        digitalWrite(mid3, HIGH);
        digitalWrite(ced, HIGH);
        delay(prg[n].delays[1]*prec); // time of middle LED
      }
      itoa(prg[n].delays[2], timeStr, 10);
      done += u8g.getStrWidth("m") + u8g.getStrWidth(timeStr);
      break;
    case 3:
      digitalWrite(mid1, LOW);        // middle LED off
      digitalWrite(mid2, LOW);
      digitalWrite(mid3, LOW);
      digitalWrite(ced, LOW);
      delay(prg[n].delays[2]*prec);   // delay before side LED
      itoa(prg[n].delays[3], timeStr, 10);
      done += u8g.getStrWidth("S") + u8g.getStrWidth(timeStr);
      break;
    case 4:
      EventChannel();                          // signal to event channel 
      digitalWrite(side, HIGH);       // side LED on
      digitalWrite(ced, HIGH);
      delay(prg[n].delays[3]*prec);   // time of side LED
      itoa(prg[n].delays[4], timeStr, 10);
      done += u8g.getStrWidth("s") + u8g.getStrWidth(timeStr);
      break;
    case 5:
      digitalWrite(side, LOW);        // side LED off
      digitalWrite(ced, LOW);
      delay(prg[n].delays[4]*prec);   // delay before door open
      break;
    case 6:
      if (prg[n].door_open) {
        EventChannel();                        // send signal to event channel
        servo.write(30);              // open the door (if needed)
      } 
      stp = 13;                       // return to preparation step
      ratReady = false;
      break;
  }
  stpPlay++;                          // next step of experiment execution
}

void EventChannel(){                           // signal to event channel
  digitalWrite(evChan, HIGH);
  delay(5);
  digitalWrite(evChan, LOW);
}
