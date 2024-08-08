#include <Arduino.h>
#include <SPI.h>      //SPI library for communicate with the nRF24L01+
#include "RF24.h"     //The main library of the nRF24L01+
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <Wire.h>
#include <Servo.h>

//Custom
#include "caras.h"
#include "musica.h"

//Macros
#define rep(i, n) for(int i=0; i<n; i++)

//Pins
const int leftServoPin = 0;
const int rightServoPin = 0;
const int buzzerPin = 0;
const int redPin = 0;
const int greenPin = 0;

//--------
//Constants
const int OUTPUT_PINS[] ={buzzerPin, redPin, greenPin};
const int INPUT_PINS[] ={};
int n_output = sizeof(OUTPUT_PINS)/sizeof(int);
int n_input = sizeof(INPUT_PINS)/sizeof(int);
//-------------

//==========
//GLOBALS

//Define packet for the direction (X axis and Y axis)
int data[4];

//Define object from RF24 library - 9 and 10 are a digital pin numbers to which signals CE and CSN are connected
RF24 radio(9,10);

//Create a pipe addresses for the communicate
const byte pipe[6] = "00001";

//LCD display
#define screen_adress 0x3c
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64 
#define OLED_RESET -1
Adafruit_SH1106G display = Adafruit_SH1106G(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

//Servos
Servo leftArm;
Servo rightArm;

const int STRAIGHT = 90;
const int HANDS_DOWN = 0;
const int HANDS_UP = 180;

//Joystick
const int xNEUTRAL = 510;
const int yNEUTRAL = 502;

//Servo dances
int servo_dances[9][2] = {
  {HANDS_DOWN, HANDS_DOWN}, 

  {HANDS_DOWN, HANDS_UP}, 
  {HANDS_UP, HANDS_DOWN}, 

  {STRAIGHT, HANDS_UP}, 
  {HANDS_UP, STRAIGHT},
  {STRAIGHT, HANDS_DOWN},
  {HANDS_DOWN, STRAIGHT},

  {STRAIGHT, STRAIGHT},

  {HANDS_UP, HANDS_UP},
  };

//=======

//-----------------
//Auxiliary for Menus
const int N_OF_MENUS = 4;
int CURRENT_MENU = 0; //0-BMO | 1-ServoDance | 2-ChangeMusic | 3-Selector | 4-TURN OFF
int CURRENT_OPTION = 0; //0-BMO | 1-ServoDance | 2-ChangeMusic | 3-TURN OFF
int LAST_MENU = 0; //0-BMO | 1-ServoDance | 2-ChangeMusic | 3-Selector | 4-TURN OFF
String menu_options[] = {"BMO", "Servo Dance", "Music Zone", "TURN OFF"};

//Control flags
int current_face = 0; //0 Frontis / 1 Left / 2 Right / 3 Happy
int joystick[2] = {0, 0}; //X | Y
bool buttons[2] = {false, false}; //A | B

//Auxliriary for music
int dancing_face = 0; //0 Frontis / 1 Left / 2 Right / 3 Happy
bool music_active = true;

int current_note = 0;
int current_melody = 0;
int notes_size = sizeof(melody) / sizeof(int);
int note_duration = 0;
int pause_between = 0;

//Auxiliary for screen
bool screen_on = true;

//Functions
void update();
void playMusic();
void initAll();
void restartMusic();

//Menus
void mainFace();
void servoDance();
void musicScreen();
void selectingMenu();
void turnedOff();

//Movements
void showFace(int idx);
void danceMoves(int idx); //TODO: Implement array for servo movements

//-----------------
void setup(){
  //PinModes
  rep(i, n_input)
    pinMode(INPUT_PINS[i], INPUT);

  rep(i, n_output)
    pinMode(OUTPUT_PINS[i], OUTPUT);

  //Setup
  Serial.begin(9600);
  radio.begin();                    //Start the nRF24 communicate            
  radio.openReadingPipe(0, pipe);   //Sets the address of the transmitter to which the program will receive data.
  radio.startListening();   

  //Servos
  leftArm.attach(leftServoPin);
  rightArm.attach(rightServoPin);

  leftArm.write(STRAIGHT);
  rightArm.write(STRAIGHT);

  // Init oled
  display.begin(screen_adress, true);
  display.display();

  //Format text
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);
  delay(3500);

  //Init messages
  Serial.println("\nIniciando sistemas...\n");
  display.println("BMO\n\nIniciandose....");
  display.display();
  delay(1500);

  initAll();
}

void loop(){
  update();
  playMusic();

  //Check if menu selector active
  if(buttons[1]){
    if(CURRENT_MENU == 3)
      CURRENT_MENU = LAST_MENU; //Toggle back to screen
    else{
      //Toggle menu
      LAST_MENU = CURRENT_MENU;
      CURRENT_MENU = 3;
      CURRENT_OPTION = LAST_MENU;
    }
  }

  //Update current menu
  switch (CURRENT_MENU){
  case 0:
    //BMO
    mainFace();
    break;

  case 1:
    //Servo Dance
    servoDance();
    break;

  case 2:
    //Change music
    musicScreen();
    break;

  case 3:
    //Selecting Menu
    selectingMenu();
    break;

  case 4:
    //Turn OFF
    turnedOff();
    break;

  default:
    break;
  }

  delay(150);
}


//===================

void initAll(){
  //Variables
  current_face = 0;
  dancing_face = 0;

  //Notes
  current_note = 0;
  current_melody = 0;
  notes_size = sizeof(melody) / sizeof(int);
  note_duration = 0;
  pause_between = 0;

  //Components
  showFace(0);

  leftArm.write(STRAIGHT);
  rightArm.write(STRAIGHT);
  delay(1500);
}

void restartMusic(){
  //Variables
  dancing_face = 0;

  //Notes
  current_note = 0;
  current_melody = 0;
  note_duration = 0;
  pause_between = 0;
}

void update(){
  if (radio.available()){
    radio.read(&data, sizeof(data));

    //Debug
    Serial.print("X: "); Serial.println(data[0]);
    Serial.print("Y: "); Serial.println(data[1]);
    Serial.print("SW: "); Serial.println(data[2]);
    Serial.print("SW2: "); Serial.println(data[3]);
    Serial.println("========");

    //Update joystick info
    joystick[0] = data[0];
    joystick[1] = data[1];

    //Update buttons info
    buttons[0] = bool(data[2]);
    buttons[1] = bool(data[3]); 
  }
  else
    Serial.println("No data :(\n");
}

void playMusic(){
  if(!music_active)
    return; //Don't play anything

  //Restart if song is over
  if(current_note >= notes_size)
    restartMusic();

  note_duration = 1000 / noteDurations[current_note];
  current_melody = melody[current_note];
  pause_between = note_duration*1.30;

  //Faces & servos
  dancing_face = melody_faces[current_note];
  current_note++;

  //Play song
  tone(buzzerPin, current_melody, note_duration);
  delay(pause_between);
}

// 0-Frontis / 1-Left / 2-Right/ 3-Wink
void showFace(int idx){
  display.clearDisplay();
  display.drawBitmap(0, 0, Caras[current_face], 128, 64, SH110X_WHITE);
  display.display();
}

//TODO Implement this
void danceMoves(int idx){

}

//==================
//MENUS
//==================
void mainFace(){
  //Based on Joystick movement or buttons see which face corresponds
  if(buttons[0])
    current_face = 3; //Wink
  else if(joystick[0] > xNEUTRAL)
    current_face = 2; //Right
  else if(joystick[0] < xNEUTRAL)
    current_face = 1; //Left
  else
    current_face = 0; //Frontal view
  
  //Apply changes
  display.clearDisplay();
  display.drawBitmap(0, 0, Caras[current_face], 128, 64, SH110X_WHITE);
  display.display();
}

void servoDance(){
  //TODO complete this menu
}

void musicScreen(){
  //Button check
  if(buttons[0])
    music_active = !music_active; //Alternate music
  
  if(!music_active){
    showFace(3); //Sad face
    //danceMoves();
  }
  else{
    showFace(dancing_face); //Current face
  }
}

void selectingMenu(){
  //Check joystick
  if(joystick[1] > yNEUTRAL)
    CURRENT_OPTION = min(CURRENT_OPTION+1, N_OF_MENUS-1);
  else if(joystick[1] < yNEUTRAL)
    CURRENT_OPTION = max(CURRENT_OPTION-1, 0);

  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("=====MENU=====\n");
  
  //Display options
  rep(i, N_OF_MENUS){
    if(i == CURRENT_OPTION)
      display.print("> ");
    else
      display.print(" ");
    display.println(menu_options[i] + "\n");
  }

  display.println("==============\n");
  display.display();

  //See if selected
  if(buttons[0]){
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("You selected:\n");
    display.println(menu_options[CURRENT_OPTION]);
    display.display();

    //Change active menu
    CURRENT_MENU = CURRENT_OPTION;
    delay(800);
  }
}

//Turn off?
void turnedOff(){
  //TO DO
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("TURN OFF BMO?\n");
  display.println("(Press A to turn off)");
  display.display();

  //Check the buttons
  if(buttons[0] && screen_on){
    screen_on = false;

    //TODO: Add music beeps for turning off mode

    //Display changes
    display.clearDisplay();
    display.setCursor(0, 0);
    display.print("Goodbye...");
    display.display();

    //Change to main screen
    CURRENT_MENU = 0;
    LAST_MENU = 0;

    //Shutdown all
    delay(800);
    leftArm.write(HANDS_DOWN);
    delay(1000);
    rightArm.write(HANDS_DOWN);
    delay(1000);

    //TURN OFF
    display.clearDisplay();
    display.setCursor(0, 0);
    display.display();
  }

  //Infinite bucle if the screen is off
  while(!screen_on){
    update(); //If button is pressed wake up

    if(buttons[0])
      screen_on = true;
  }
  
}