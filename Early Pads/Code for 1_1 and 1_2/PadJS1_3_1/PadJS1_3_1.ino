//SCRIPT 1.3.1
//FULL V3 SCRIPT FIRST VERSION
//PAD 1_3_1 FOR USE ONLY ON BOARD VERSION 5
//2024 BARKUS MFG

//SET USB TYPE TO KEYBOARD+MOUSE+JOYSTICK BEFORE UPLOAD
//150MHZ IS ALSO RECCOMENDED AS THE TEENSY 4.0 HAS POOR COOLING

//Axis pins (The arduino / teensy anolog pins connected to each axis) enter in the corresponding order: Left pin, Right pin, Up pin, Down pin
//For example if Left was on 18, Right was on 19, Down 16, and Up 17; aPins would = [18, 19, 16, 17]
//To figure out what pin each tile is bound to, run the test program from the website.

/////////////////////////////////////////////
/**/int aPins[] = {14, 18, 15, 19, 16, 17};// <-- FSR PINS GO HERE: LEFT, LEFT, RIGHT, RIGHT, UP, DOWN (if your pad only has 4 sensor inputs its ok just leave the second left and second right as the default value (18 and 19))
/////////////////////////////////////////////

bool bOn[] = {false, false, false, false, false, false};
bool lOn = false;
bool rOn = false;

//Knob pins (Same concept as above but for the adjustment knobs)
int kPins[] = {20, 20, 21, 21, 22, 23};

//Start and select button pins (Start is the button on the right (Enter) and select is the button on the left (Escape))
int ssPins[] = {9, 10};

//Light stuff
//RGB Values to change color, functional on this version! play around with them as you would like 255 = off 0 = completely on
int RGB[] = {200, 200, 200};
int lPins[] = {2, 3, 4, 5};
int RGBPins[] = {6, 7, 8};

//Adjusting these values below may help with jitter inputs (Lightly pressing the tile and getting 10s to 100s of button presses)
int releaseThresh = 10; //Keep this below press Thresh, default = 10
int pressThresh = 15; //Keep this above release Thresh, default = 15

void setup() {
  // setup button pins
  pinMode(ssPins[0], INPUT);
  pinMode(ssPins[1], INPUT);

  // put your setup code here, to run once:
  Joystick.begin();
  Serial.begin(9600);

  //Set RGB Values
  for (int i = 0; i < 3; i++) {
    analogWrite(RGBPins[i], RGB[i]);
  }

  //Stop random button input
  for (int i = 0; i < 32; i++) {
    Joystick.button(i, 0);
  }

  Joystick.useManualSend(true);
}

void loop() {
  // put your main code here, to run repeatedly:

  // Iterate through all FSR input values to determine if they are greater than the knob values
  for (int i = 0; i < 6; i++) 
  {
    if(analogRead(aPins[i]) >= analogRead(kPins[i]) + pressThresh && bOn[i] == false) {
      Joystick.button(i + 1, 1);
      bOn[i] = true;
      digitalWrite(lPins[i], HIGH);

      // Check if one of the two left or right sensors is on
      if(i == 0 || 1){
        lOn = true;
      }

      if(i == 2 || 3){
        rOn = true;
      }
    }
    else if (analogRead(aPins[i]) < analogRead(kPins[i]) - releaseThresh && bOn[i] == true) {
      Joystick.button(i + 1, 0);
      bOn[i] = false;
      digitalWrite(lPins[i], LOW);
    }
  }
  // Check if both of the two left or right sensors is off
  if(bOn[0] && bOn[1] == false){
    lOn = false;
  }
  if(bOn[2] && bOn[3] == false){
    rOn = false;
  }

  // If any of the left FSRS are actuated, call left
  if(lOn == true) {
    digitalWrite(lPins[0], HIGH);
    Joystick.button(0 + 1, 1);
    Joystick.button(1 + 1, 1);
  }
  //if both of the left FSRS are unactuated, turn off left
  else{
    digitalWrite(lPins[0], LOW);
    Joystick.button(0 + 1, 0);
    Joystick.button(1 + 1, 0);
  }
  // If any of the right FSRS are actuated, call right 
  if(rOn == true) {
    digitalWrite(lPins[1], HIGH);
    Joystick.button(2 + 1, 1);
    Joystick.button(3 + 1, 1);
  }
  //if both of the right FSRS are unactuated, turn off right
  else{
    digitalWrite(lPins[1], LOW);
    Joystick.button(2 + 1, 0);
    Joystick.button(3 + 1, 0);
  }

  // Iterate through button inputs (start and select)
  for (int i = 0; i < 2; i++) 
  {
    if(digitalRead(ssPins[i]) == HIGH) {
      Joystick.button(i + 5, 1);
    }
    else {
      Joystick.button(i + 5, 0);
    }
  }
  Joystick.send_now();
}