#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <RotaryEncoder.h>
#include <Joystick.h>
#include <EEPROM.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
#define SCREEN_ADDRESS 0x3C // You may need to change this address depending on your OLED module

//Joystick
Joystick_ Joystick;

//display stuff
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
int highlightedBar = 3; // Change this value to highlight a different bar
bool barSelected = false;
int ButtonHeld = 0;

//display update stuff
int refreshInt = 100; //60 is the lowest it will go without messing up rotary encoder input
unsigned long lastRefresh = 0;
int autoOffInterval = 30; //auto off in seconds, default: 10. Pad polls at 250HZ with screen on, with screen off it polls at 800HZ
unsigned long lastInput = 10000;
bool screenidle = false;

//savingstuff

//rotary knob stuff
RotaryEncoder encoder(5, 14, RotaryEncoder::LatchMode::TWO03);
int oldPos = 0;
bool release = true;

//test poll
int poll;
int pollStart = 0;
bool flipper = false;

//////////////////////////////////////////
/**/uint8_t aPins[] = {A9, A8, A3, A2, A0, A7};//
//////////////////////////////////////////
int threshVal[] = {200, 200, 200, 200, 200, 200};
int inputVal[] = {40, 70, 30, 50, 10, 70};
bool bOn[] = {false, false, false, false, false, false};
bool lOn = false;
bool rOn = false;

//Start and select button pins (Start is the button on the right (Enter) and select is the button on the left (Escape))
int ssPins[] = {7, 16};

//Adjusting these values below may help with jitter inputs (Lightly pressing the tile and getting 10s to 100s of button presses)
int releaseThresh = 10; //Keep this below press Thresh, default = 10
int pressThresh = 15; //Keep this above release Thresh, default = 15

// 'barkusmap', 128x64px
const unsigned char myBitmap [] PROGMEM = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3f, 0xff, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xff, 0xff, 0xf8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1f, 0xff, 0xff, 0xff, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7f, 0xff, 0xff, 0xff, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xe0, 0x3e, 0x7f, 0xf8, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0xff, 0x80, 0x1f, 0x8f, 0xfc, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0xfc, 0x00, 0x1f, 0x03, 0xfe, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0xf8, 0x00, 0x1e, 0x00, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x1f, 0xe0, 0x00, 0x1e, 0x00, 0x3f, 0x80, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x1f, 0xc0, 0x00, 0x1c, 0x00, 0x1f, 0xc0, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x3f, 0x80, 0x00, 0x1c, 0x0f, 0x0f, 0xc0, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x7f, 0x00, 0x00, 0x1c, 0xff, 0xe7, 0xe0, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0xfe, 0x00, 0x00, 0x3f, 0xff, 0xf3, 0xf0, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0xfc, 0x00, 0x1f, 0xff, 0xff, 0xf1, 0xf8, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x01, 0xf8, 0x07, 0xff, 0xff, 0xc0, 0x01, 0xf8, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x01, 0xf8, 0x03, 0xff, 0x38, 0x00, 0x00, 0xfc, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x03, 0xf0, 0x01, 0xf0, 0x38, 0x00, 0x00, 0x7c, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x03, 0xe0, 0x00, 0x00, 0x38, 0x00, 0x00, 0x7c, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x07, 0xe0, 0x40, 0x00, 0x38, 0x00, 0x00, 0x3e, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x07, 0xe0, 0x78, 0x00, 0x38, 0x00, 0x00, 0x3e, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x07, 0xc0, 0x7f, 0xfc, 0x70, 0x00, 0x00, 0x1f, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x0f, 0xc0, 0x3f, 0xff, 0xfc, 0x00, 0x00, 0x1f, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x0f, 0x80, 0x38, 0x7f, 0xff, 0xfc, 0x00, 0x1f, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x0f, 0x80, 0x38, 0x00, 0xff, 0xfe, 0x00, 0x1f, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x0f, 0x80, 0x38, 0x00, 0x00, 0x7f, 0x00, 0x0f, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x0f, 0x80, 0x38, 0x00, 0x00, 0x03, 0x00, 0x0f, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x0f, 0x80, 0x38, 0x00, 0x00, 0x03, 0x00, 0x0f, 0x80, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x0f, 0x80, 0x18, 0x00, 0x00, 0x03, 0x80, 0x0f, 0x80, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x0f, 0x80, 0x18, 0x00, 0x00, 0x03, 0x80, 0x0f, 0x80, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x1f, 0x80, 0x18, 0x00, 0x00, 0x01, 0x80, 0x0f, 0x80, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x0f, 0x80, 0x18, 0x00, 0x00, 0x01, 0x80, 0x0f, 0x80, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x0f, 0x80, 0x18, 0x00, 0x00, 0x01, 0x80, 0x0f, 0x80, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x0f, 0x80, 0x1c, 0x00, 0x00, 0x0f, 0x80, 0x0f, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x0f, 0x80, 0x1c, 0x00, 0x03, 0xff, 0x80, 0x0f, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x0f, 0x80, 0x1c, 0x00, 0xff, 0xff, 0xc0, 0x0f, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x0f, 0x80, 0x1c, 0x3f, 0xff, 0xe0, 0x00, 0x1f, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x0f, 0xc0, 0x1f, 0xff, 0xfc, 0x00, 0x00, 0x1f, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x07, 0xc0, 0x0f, 0xe0, 0x1c, 0x00, 0x00, 0x1f, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x07, 0xc0, 0x08, 0x60, 0x1c, 0x00, 0x00, 0x3e, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x07, 0xe0, 0x00, 0x70, 0x1c, 0x00, 0x00, 0x3e, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x03, 0xe0, 0x00, 0xf0, 0x1c, 0x00, 0x00, 0x7c, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x03, 0xf0, 0x00, 0xf0, 0x1c, 0x00, 0x00, 0x7c, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x01, 0xf0, 0x01, 0xe0, 0x1c, 0x00, 0x10, 0xfc, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x01, 0xf8, 0x03, 0xe0, 0x1c, 0x00, 0x10, 0xf8, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0xfc, 0x03, 0xc0, 0x0c, 0x00, 0x11, 0xf8, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0xfe, 0x07, 0x80, 0x0c, 0x00, 0x3b, 0xf0, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x7e, 0x0f, 0x00, 0x0c, 0x01, 0xff, 0xe0, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x3f, 0x1e, 0x00, 0x0f, 0xff, 0xff, 0xe0, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x3f, 0xfc, 0x00, 0x03, 0xff, 0xff, 0xc0, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x1f, 0xf8, 0x00, 0x00, 0x00, 0x3f, 0x80, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0xf0, 0x00, 0x00, 0x00, 0x7f, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0xfc, 0x00, 0x00, 0x01, 0xfe, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0xfe, 0x00, 0x00, 0x07, 0xfc, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0xff, 0xc0, 0x00, 0x1f, 0xf8, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7f, 0xf8, 0x01, 0xff, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1f, 0xff, 0xff, 0xff, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xff, 0xff, 0xf8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7f, 0xff, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xf8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

void setup() {
  // Misc setup
  pinMode(10, INPUT_PULLUP);
  pinMode(16, INPUT);
  pinMode(7, INPUT);
  //Serial.begin(9600);

  // Initialize with the I2C addr 0x3D (for the 128x64)
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }

  // Display initialization
  display.display(); //Clear Adafruit logo
  delay(2);
  display.clearDisplay();
  display.drawBitmap(0, 0, myBitmap, 128, 64, WHITE);
  display.display();
  delay(1000); //Change logo displat time here, set to 0 to load immediately <--
  display.clearDisplay();
  pollStart = millis();
  Joystick.begin(false);

  for (int i = 0; i < 6; i++) {
      threshVal[i] = EEPROM.read(i) * 4;
  }
}

void inputTest(){
  Serial.println(String(digitalRead(16)) + " 16");
  Serial.println(String(digitalRead(7)) + " 7");
  Serial.println(String(analogRead(21)) + " 21");
  Serial.println(String(analogRead(20)) + " 20");
  Serial.println(String(analogRead(19)) + " 19");
  Serial.println(String(analogRead(4)) + " 4");
  Serial.println(String(analogRead(8)) + " 8");
  Serial.println(String(analogRead(6)) + " 6");
}

void SaveSetting(){
  for (int i = 0; i < 6; i++) {
    EEPROM.put(i, int(threshVal[i] / 4));
  }
}

void UIControls(){
  //Reset encoder button
  if(digitalRead(10) == LOW){
    lastInput = millis() + autoOffInterval * 1000;
    screenidle = false;
    Serial.println("notIdle " + (String)lastInput);
    ButtonHeld++;
    if(ButtonHeld == 500){
      SaveSetting();
      display.clearDisplay();
      display.setTextSize(2);
      display.setTextColor(WHITE);
      display.setCursor(0, 10);
      display.println("Saved");
      display.println("Settings");
      display.display();
      lastRefresh = millis() + 1000;
    }
  }

  if(digitalRead(10) == HIGH){
    release = true;
    ButtonHeld = 0;
  }

  if(millis() > lastInput){
    if(screenidle == false){
      display.clearDisplay();
      display.setTextSize(1);
      display.setTextColor(WHITE);
      display.setCursor(0, 10);
      display.println("Display idle for");
      display.println("polling rate boost.");
      display.println("Press knob to view");
      display.println("sensors again.");
      display.println("Hold down knob to");
      display.println("save current settings");
      display.display();
      Serial.println((String)millis() + " " + (String)lastInput);
      screenidle = true;
    }
    return;
  }
  
  //Read encoder
  encoder.tick();
  int encPos = encoder.getPosition();
  //Display Update
  if(millis() > lastRefresh){
    lastRefresh = millis() + refreshInt;
    updateDisplay();
  }
  //UI interactions, follow comments for details
  // FOR A CHANGE IN PRESS
  if(release == true){
    if(digitalRead(10) == LOW && barSelected == false){
      barSelected = true;
      release = false;
      Serial.println("tOn");
    }
    else if(digitalRead(10) == LOW && barSelected == true){
      barSelected = false;
      release = false;
      Serial.println("tOff");
    }
  }
  // FOR A CHANGE IN ROTATION
  // 1 determine change in encoder position
  if(oldPos != encPos){
    Serial.println(encPos);
    oldPos = encPos;
    // 2 determine if a bar is selected
    if(barSelected){
      //2.1 add to bar threshold value based on encoder change
      if((int)encoder.getDirection() == -1){
        threshVal[highlightedBar] -= 16;
      }
      else {
        threshVal[highlightedBar] += 16;
      }
    }
    else {
      //2.2 change selected bar up if positive dir down if negitive dir
      if((int)encoder.getDirection() == -1){
        highlightedBar--;
      }
      else {
        highlightedBar++;
      }
      //make sure selected bar is always between 0 and 8
      if(highlightedBar > 5){
        highlightedBar = 5;
      }
      if(highlightedBar < 0){
        highlightedBar = 0;
      }
    }
  }
}

void polltest(){
  if(pollStart != 0){
    poll++;
  }
  if(millis() > pollStart + 1000 && millis() < pollStart + 1050){
    Serial.println(poll);
  }
}

void InputController(){

  // Iterate through all FSR input values to determine if they are greater than the knob values
  for (int i = 0; i < 6; i++) 
  {
    inputVal[i] = analogRead(aPins[i]);

    if(inputVal[i] >= threshVal[i] + pressThresh && bOn[i] == false) {
      Joystick.setButton(i + 1, 1);
      Serial.println("Set " + String(i + 1) + " on" );
      bOn[i] = true;
      /*
      // Check if one of the two left or right sensors is on
      if(i == 0 || 1){
        lOn = true;
      }

      if(i == 2 || 3){
        rOn = true;
      }
      */
    }
    else if (inputVal[i] < threshVal[i] - releaseThresh && bOn[i] == true) {
      Joystick.setButton(i + 1, 0);
      bOn[i] = false;
    }
  }
  // Check if both of the two left or right sensors is off
  /*if(bOn[0] && bOn[1] == false){
    lOn = false;
  }
  if(bOn[2] && bOn[3] == false){
    rOn = false;
  }

  // If any of the left FSRS are actuated, call left
  if(lOn == true) {
    Joystick.setButton(0 + 1, 1);
    Joystick.setButton(1 + 1, 1);
  }
  //if both of the left FSRS are unactuated, turn off left
  else{
    Joystick.setButton(0 + 1, 0);
    Joystick.setButton(1 + 1, 0);
  }
  // If any of the right FSRS are actuated, call right 
  if(rOn == true) {
    Joystick.setButton(2 + 1, 1);
    Joystick.setButton(3 + 1, 1);
  }
  //if both of the right FSRS are unactuated, turn off right
  else{
    Joystick.setButton(2 + 1, 0);
    Joystick.setButton(3 + 1, 0);
  }
*/
  // Iterate through button inputs (start and select)
  for (int i = 0; i < 2; i++) 
  {
    if(digitalRead(ssPins[i]) == HIGH) {
      Joystick.setButton(i + 10, 1);
    }
    else {
      Joystick.setButton(i + 10, 0);
    }
  }
  Joystick.sendState();
}

void loop() {
  //gamepadlapolltest(); //uncomment to test polling with GamepadLA
  UIControls(); //Controls screen and UI
  InputController(); //Handles input of sensors and controller output
}

void gamepadlapolltest(){
  if(flipper){
  Joystick.setXAxis(1000);
  flipper = false;
  }
  else {
  Joystick.setXAxis(10);
  flipper = true;
  }
  Joystick.sendState();
}

void updateDisplay(){
  // Input test
  //inputTest();
  

  // Clear the display
  display.clearDisplay();

  // Draw highlight box
  drawHollowBox(highlightedBar * 20 - 2, 0, 19, SCREEN_HEIGHT, 2);

  // Draw bars and blips
  for (int i = 0; i < 6; i++) {
    int barHeights = (inputVal[i] / 16);
    int blipPositions = (threshVal[i] / 16);
    drawBar(i * 20, SCREEN_HEIGHT - barHeights, 15, barHeights, WHITE);
    int blipColor = WHITE;
    if(barHeights > blipPositions) {
      blipColor = BLACK;
    }
    drawBlip(i * 20, SCREEN_HEIGHT - blipPositions, 15, 2, blipColor);
  }
  display.display();
}

void drawBar(int x, int y, int width, int height, int color) {
  display.fillRect(x + 6, y, width, height, WHITE);
}

void drawBlip(int x, int y, int width, int height, int color) {
  display.fillRect(x + 6, y, width, height, color);
}

void drawHollowBox(int x, int y, int width, int height, int thickness) {
  display.drawRect(x + 6, y, width, height, WHITE);
  display.drawRect(x + 6+ thickness, y + thickness, width - 2 * thickness, height - 2 * thickness, BLACK);
}