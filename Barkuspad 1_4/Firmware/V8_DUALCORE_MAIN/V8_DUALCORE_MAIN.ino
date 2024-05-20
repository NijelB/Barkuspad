//OBJECTIVES:
//1: Poll at 1000hz 100% stability.
//2: Have a UI where the user can adjust input sensitivity, see sensor values, and adjust the input pins for backwards compatibility with older pads, or custom pads.
//(3) -- For a later date, setup Bluetooth LE for input. This could be really good for travel pads.


//This sketch uses RTOS, Real Time Operating System. It lets us run the controller on Core1 where it will be uninterupted and is free to run at 1000hz, everything else is on core 0.
//So instead of functions, we have tasks and loops are created from for(;;) instead of loop(). You can still make functions and call them from the tasks but dont add anything to loop(), the reason is loop() runs on core 1 and as previously mentioned, core 1 is reserved for the actual controller input.
//The display can take up to 20ms to render sometimes, during this time previously no other code could run, which is obviously really bad when it comes to having a responsive controller.
//Because loop isnt used we have functions with never ending for(;;) loops. The ESP32 has inbuilt stack overflow protection methods but you still have to be careful what you do with the memory or else it will just crash.
//If you are confused at all the FreeRTOS documentation has really good explanations for everything and there are quite a few basic overviews on youtube.
//TLDR: DO NOT ADD CODE TO THE INPUTCONTROLLER() FUNCTION (or in this case, task), unless it actually has something to do with the controller, that segment needs to be as optimised as possible to maintain good polling rate.

//To program the board you have to press the reset button twice before uploading, it cannot be fast. press 1 second, release one second, press 1 second, release. The light will be a green that fades in and out if you have succeeded.

//THINGS THAT CHANGE BECAUSE WE ARE USING RTOS
//return; is now continue;
//loop() is unused
//we have to manually allocate memory to each task. If you make additions and the board starts crashing you likely need to just increase the #define STACK_SIZE variable to something bigger.

// Serial.println(uxTaskGetStackHighWaterMark(NULL)); <-- USE this command in a function to see how much memory the task uses

#include "USB.h"
#include "USBHIDGamepad.h"
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Preferences.h> // ESP32 equivelent of EEPROM

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C

#define STACK_SIZE 5000  //They all use ~3200 each so 4000 is a pretty good number

//display stuff
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
int highlightedBar = 3;  // Change this value to highlight a different bar
bool barSelected = false;
int highlightedSetting = 0;
int ButtonHeld = 0;
int refreshInt = 20;

//Settings
String ver = "2024 BARKUS MFG LLC";
enum ScreenSel { //which screen we are on
  MAIN,
  SETTINGS,
  PINREMAP
};
ScreenSel screenSel = MAIN;
bool settingSelected = false;
int scroll = 38;
int pinRemapSel = 0;
int pindex = 0;
bool pinSelected = 0;
int scrollLimit[] = { -52, 38 };
int settings = 9;
int highlightedsetting = 0;
String settingParams[] = { "Back", "Refresh", "Release Thresh", "Press Thresh", "Poll Tester", "Save Settings", "!!Reset All!!", "Inputs", "Pin Remap", "Smoothing" };
int autoSaveInt = 10000;
int autoSaveCounter = 0;
int resetCounter = 0;
int possiblePins[] = {A0, A1, A2, A3, A6, A7, D2, D3};
int smoothingSize = 10; //Window size for sensor smoothing
bool smoothing = true;
unsigned long lastInput;

//Storage
Preferences preferences;

//HID stuff
USBHIDGamepad Gamepad;
bool flipper = false;
bool testPoll = true;

//rotary knob stuff
int CLK_state = 0;
int prev_CLK_state = 0;
int DT_PIN = D7;
int CLK_PIN = D6;
int direction = 0;
int counter;
int buttonPin = D8;
int oldPos = 0;
bool release = true;

////////////////////////////////////////////////////////////////
/**/ int inputPins[] = { A0, A1, A2, A3, A6, A7, D2, D3 };//
////////////////////////////////////////////////////////////////

uint8_t inputs = 4;
int threshVal[] = { 200, 200, 200, 200, 900, 900, 900, 900 };
int sensVal[] = { 40, 70, 30, 50, 10, 70, 10, 10 };
bool bOn[] = { false, false, false, false, false, false, false, false };

//Adjusting these values below may help with jitter inputs (Lightly pressing the tile and getting 10s to 100s of button presses)
int releaseThresh = 10;  //Keep this below press Thresh, default = 10
int pressThresh = 15;    //Keep this above release Thresh, default = 15
void loop() {}           //<== Dont put anything in there
// 'barkusmap', 128x64px
const unsigned char myBitmap[] PROGMEM = {
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

// 'settingsgear' 8x8px
const unsigned char settmap[] PROGMEM = {
  0x24, 0x00, 0x81, 0x18, 0x18, 0x81, 0x00, 0x24
};

//Sensor value smoothing somewhat similar to the way it works on TeeJUSBs' code. Here we are just calculating WMA instead of WMA and HMA, WMA is already plenty.
class Sensor {
  private:
    int windowSize;
    int* values;
    int currentIndex;

  public:
    Sensor(int windowSize) {
      this->windowSize = windowSize;
      values = new int[windowSize];
      currentIndex = 0;
      for (int i = 0; i < windowSize; i++) {
        values[i] = 0;
      }
    }

    ~Sensor() {
      delete[] values;
    }

    int calculateWMA(int pin_) {
      // Push all values up one index
      for (int i = windowSize - 1; i > 0; i--) {
        values[i] = values[i - 1];
      }
      // Add the new sensor reading at index 0
      values[0] = analogRead(pin_);
      int sum = 0;
      int weight = windowSize;
      for (int i = 0; i < windowSize; i++) {
        sum += values[i] * weight;
        weight--; // Decrease weight for the next value
      }
      return (sum / ((windowSize * (windowSize + 1)) / 2)); // Divide by the sum of the series 1 to windowSize
    }
};

//Would be nice if it could be the same size as the amount of inputs you have selected but the ESP32 is fast enough to cope with the max possible value of 8 and memory fragmentation
//is a very real threat when using RTOS so we will keep it at 8 always
Sensor sensors[8] = {
  Sensor(smoothingSize),
  Sensor(smoothingSize),
  Sensor(smoothingSize),
  Sensor(smoothingSize),
  Sensor(smoothingSize),
  Sensor(smoothingSize),
  Sensor(smoothingSize),
  Sensor(smoothingSize)
};

void setup() {
  //Encoder pin setup
  pinMode(buttonPin, INPUT_PULLUP);
  pinMode(DT_PIN, INPUT_PULLUP);
  pinMode(CLK_PIN, INPUT_PULLUP);

  //Display setup
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ;  // Don't proceed, loop forever
  }
  display.display();  //Clear Adafruit logo
  delay(2);
  display.clearDisplay();
  display.drawBitmap(0, 0, myBitmap, 128, 64, WHITE);
  display.display();
  delay(5000);  //Change logo displat time here, set to 0 to load immediately <--
  display.clearDisplay();
  
  Gamepad.begin();
  preferences.begin("settings", false); 
  Setting(true, false); // Read settings
  Serial.begin(115200);

  pinMode(inputPins[6], INPUT);
  pinMode(inputPins[7], INPUT);

  xTaskCreatePinnedToCore(
    updateDisplay,
    "DisplayUpdate",
    STACK_SIZE,
    NULL,
    1,
    NULL,
    0);

  xTaskCreatePinnedToCore(
    InputController,
    "controllerinput",
    STACK_SIZE,
    NULL,
    2,
    NULL,
    1);

  xTaskCreatePinnedToCore(
    UIControls,
    "controllerinput",
    STACK_SIZE,
    NULL,
    1,
    NULL,
    0);
}

void Setting(bool ReadOW, bool showMessage){ // First bool decides if you want to read or write to memory, True for read. Second bool decides if you want to display the "settings saved" text
  if (ReadOW == false) { //Write to memory    
    preferences.putBool("testpoll", testPoll);
    preferences.putInt("inputs", inputs);
    for (int i = 0; i < 8; i++) {
      char pointer[50];
      ("thresh" + String(i)).toCharArray(pointer, 50);
      preferences.putInt(pointer, threshVal[i]);
    }
    for (int i = 0; i < 8; i++) {
      char pointer[50];
      ("pinmap" + String(i)).toCharArray(pointer, 50);
      preferences.putInt(pointer, inputPins[i]);
    }
    preferences.putInt("releaseThresh", releaseThresh);
    preferences.putInt("pressThresh", pressThresh);
    preferences.putInt("refreshInt", refreshInt);
  } else { //Read memory
    testPoll = preferences.getBool("testpoll", false);
    inputs = preferences.getInt("inputs", 4);
    for (int i = 0; i < 8; i++) {
      char pointer[50];
      ("thresh" + String(i)).toCharArray(pointer, 50);
      threshVal[i] = preferences.getInt(pointer, 500);
    }
    for (int i = 0; i < 8; i++) {
      char pointer[50];
      ("pinmap" + String(i)).toCharArray(pointer, 50);
      inputPins[i] = preferences.getInt(pointer, possiblePins[i]);
    }
    releaseThresh = preferences.getInt("releaseThresh", 10);
    pressThresh = preferences.getInt("pressThresh", 15);
    refreshInt = preferences.getInt("refreshInt", 20);
  }
  if(showMessage){
    //Show message
  }
}

void SetDefaults(){
  testPoll = false;
    inputs = 4;
    for (int i = 0; i < 8; i++) {
      threshVal[i] = 500;
    }
    for (int i = 0; i < 8; i++) {
      inputPins[i] = possiblePins[i];
    }
    releaseThresh = 10;
    pressThresh = 15;
    refreshInt = 20;
}

void InputController(void* parameters) {
  /* I am faced with an interesting problem here, the ESP32 HID Gamepad library doesnt have a manual send equivelent like Joystick library does. Instead they have .send(ALL OF THE CONTROLLER DATA). So we cant just do setButton(1, on)
  then send the state at the end of the loop. If we do setButton(1, on) for each button the polling rate drops all the way down to 125hz. So Ive gotta figure out how to get this input mask to work for the buttons. */

  for (;;) {
    long start = xTaskGetTickCount();
    //MAIN FSR ANALOG READ
    bool buttonInputsMask[32] = {false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false};
    for (int i = 0; i < inputs; i++) {
      if(!smoothing){
        sensVal[i] = analogRead(inputPins[i]);
      }                                                                                                                                                                                                                                  
      else{
        sensVal[i] = sensors[i].calculateWMA(inputPins[i]);
      }
      //Serial.println(sensVal[i]);
      if (sensVal[i] >= threshVal[i] + pressThresh ) {
        buttonInputsMask[i] = true;
        //Gamepad.pressButton(i);
        bOn[i] = true;
        
      } else if (sensVal[i] < threshVal[i] - releaseThresh ) {
        buttonInputsMask[i] = false;
        //Gamepad.releaseButton(i);
        bOn[i] = false;
      }
    }
    //ButtonRead
    for (int i = 0; i < 2; i++) {
      int val = digitalRead(inputPins[6 + i]);
      if (val == HIGH) {
        buttonInputsMask[i + 6] = true;
        bOn[i + 6] = true;
      } else {
        buttonInputsMask[i + 6] = false;
        bOn[i + 6] = false;
      }
    }
    //Convert the 32 digit binary value to a hexidecimal uint32 mask
    uint32_t sendMask = 0;
    for (int i = 0; i < 32; ++i) {
      // Set the corresponding bit in the compressedValue using bitwise OR and left shift
      sendMask |= (buttonInputsMask[i] ? 1 : 0) << i;
    }
    uint32_t hexSendMask = 0x00000000 | sendMask; 
    
    //SEND CONTROLLER STATE
    if (testPoll == false) {
      Gamepad.send(0, 0, 0, 0, 0, 0, 0, 0x00000000 | sendMask);
      continue;  //SEND STATE AND GO TO NEXT ITERATION OF THE FOR LOOP TO SKIP THE POLL RATE TESTER, IN THIS CASE CONTINUE IS THE SAME AS A RETURN STATEMENT
    }
    //POLL RATE TESTER
    if (flipper == true) {
      flipper = false;
      Gamepad.send(10, 100, 0, 0, 0, 0, 0, 0x00000006);
    } else {
      flipper = true;
      Gamepad.send(100, 10, 0, 0, 0, 0, 0, 0x00000006);
    }
    long stop = xTaskGetTickCount() - start;
    if(stop == 0){
      vTaskDelay(1);
    }
  } 
}

void updateDisplay(void* parameters) {
  for (;;) {
    //Clear display
    display.clearDisplay();
    if (screenSel == MAIN) {
      int barWidth = 128 / inputs - 4;
      //First draw the selection box
      if (highlightedBar != inputs) {
        display.drawRect(highlightedBar * (barWidth + 4), 0, barWidth + 4, 54, WHITE);
      }
      //display.drawRect(x + 6+ thickness, y + thickness, width - 2 * thickness, height - 2 * thickness, BLACK);
      //Draw input data bars
      for (int i = 0; i < inputs; i++) {
        int barHeights = (sensVal[i] / 19);
        int blipPositions = (threshVal[i] / 19);
        display.fillRect(i * (barWidth + 4) + 3, 54 - barHeights, barWidth - 2, barHeights, WHITE);
        int blipColor = WHITE;
        if (barHeights > blipPositions) {
          blipColor = BLACK;
        }
        display.fillRect(i * (barWidth + 4) + 3, 54 - blipPositions, barWidth - 2, 2, blipColor);
      }
      //Draw buttons
      display.setTextSize(0);
      display.fillRect(0, 54, 128, 10, WHITE);
      int selColor = BLACK;
      int startColor = BLACK;
      if (bOn[6]) {
        startColor = WHITE;
      } else {
        startColor = BLACK;
      }
      if (bOn[7]) {
        selColor = WHITE;
      } else {
        selColor = BLACK;
      }
      display.fillRoundRect(1, 55, 50, 8, 2, selColor);
      display.setCursor(14, 56);
      display.setTextColor(!selColor);
      display.println("BACK");
      display.fillRoundRect(128 - 51, 55, 50, 8, 2, startColor);
      display.setCursor(88, 56);
      display.setTextColor(!startColor);
      display.println("START");

      //Draw settings icon
      if (highlightedBar == inputs) {
        display.fillRect(60, 55, 8, 8, BLACK);
        display.drawRect(58, 53, 12, 12, BLACK);
        display.drawBitmap(60, 55, settmap, 8, 8, WHITE);
      } else {
        display.fillRect(60, 55, 8, 8, BLACK);
        display.drawBitmap(60, 55, settmap, 8, 8, WHITE);
      }
    } 
    else if (screenSel == SETTINGS) {  //display settings
      //Setup
      display.setTextColor(WHITE);
      display.drawRect(0, 26, 128, 11, WHITE);
      display.setTextSize(0);
      //Back
      display.setCursor(5, scroll - 10);
      display.println(settingParams[0]);
      //Setting 0
      display.setCursor(5, scroll + 0);
      display.println(settingParams[1]);
      display.setCursor(100, scroll + 0);
      display.println(refreshInt);
      //Setting 1
      display.setCursor(5, scroll + 10);
      display.println(settingParams[2]);
      display.setCursor(100, scroll + 10);
      display.println(releaseThresh);
      //Setting 2
      display.setCursor(5, scroll + 20);
      display.println(settingParams[3]);
      display.setCursor(100, scroll + 20);
      display.println(pressThresh);
      //Setting 3
      display.setCursor(5, scroll + 30);
      display.println(settingParams[4]);
      display.setCursor(100, scroll + 30);
      display.println(testPoll);
      //Setting 4
      display.setCursor(5, scroll + 40);
      display.println(settingParams[5]);
      //Setting 5
      display.setCursor(5, scroll + 50);
      display.println(settingParams[6]);
      //Setting 6
      display.setCursor(5, scroll + 60);
      display.println(settingParams[7]);
      display.setCursor(100, scroll + 60);
      display.println(inputs);
      //Setting 7
      display.setCursor(5, scroll + 70);
      display.println(settingParams[8]);
      //Setting 8
      display.setCursor(5, scroll + 80);
      display.println(settingParams[9]);
      display.setCursor(100, scroll + 80);
      display.println(smoothing);
      //INFO
      display.setCursor(5, scroll + 100);
      display.println(ver);
    }
    else if (screenSel == PINREMAP) {
      //setup
      //draw cursor
      display.setTextColor(WHITE);
      display.drawRect(pinRemapSel * 16, 50, 17, 12, WHITE);
      display.drawRect(pinRemapSel * 16, 18, 17, 12, WHITE);
      display.setTextSize(0);
      //draw labels
      display.setCursor(43, 5);
      display.println("OUTPUT:");
      display.setCursor(5, 20);
      display.println("1");
      display.setCursor(23, 20);
      display.println("2");
      display.setCursor(39, 20);
      display.println("3");
      display.setCursor(53, 20);
      display.println("4");
      display.setCursor(69, 20);
      display.println("5");
      display.setCursor(85, 20);
      display.println("6");
      display.setCursor(99, 20);
      display.println("ST");
      display.setCursor(114, 20);
      display.println("SE");
      display.setCursor(3, 35); 
      display.println("CORRESPONDING PIN:");
      display.setCursor(2, 52);
      display.println(inputPins[0]);
      display.setCursor(21, 52);
      display.println(inputPins[1]);
      display.setCursor(37, 52);
      display.println(inputPins[2]);
      display.setCursor(51, 52);
      display.println(inputPins[3]);
      display.setCursor(67, 52);
      display.println(inputPins[4]);
      display.setCursor(83, 52);
      display.println(inputPins[5]);
      display.setCursor(97, 52);
      display.println(inputPins[6]);
      display.setCursor(112, 52);
      display.println(inputPins[7]);
    }
    //Render result
    display.display();
    vTaskDelay(refreshInt / portTICK_PERIOD_MS);
  }
}

void UIControls(void* parameters) {
  for (;;) {
    //ENCODER
    CLK_state = digitalRead(CLK_PIN);
    if (CLK_state != prev_CLK_state && CLK_state == HIGH) {
      // the encoder is rotating in counter-clockwise direction => decrease the counter
      if (digitalRead(DT_PIN) == HIGH) {
        counter--;
        direction = -1;
      } else {
        // the encoder is rotating in clockwise direction => increase the counter
        counter++;
        direction = 1;
      }
    }
    prev_CLK_state = CLK_state;

    //Reset encoder button
    if (digitalRead(buttonPin) == LOW) {
      ButtonHeld++;
      if (ButtonHeld == 1000) {
        Setting(false, true);
      }
    }

    if (digitalRead(buttonPin) == HIGH) {
      release = true;
      ButtonHeld = 0;
    }

    //Autosave
    autoSaveCounter++;
    if(autoSaveCounter > autoSaveInt){
      autoSaveCounter = 0;
      Setting(false, false);
    }

    //Read encoder
    int encPos = counter;

    //UI interactions, follow comments for details

    // FOR A CHANGE IN PRESS
    if (screenSel == MAIN) {  //Actions for a press occuring in the main screen
      if (release == true) {
        if (digitalRead(buttonPin) == LOW && barSelected == false) {
          if (highlightedBar == inputs) {
            screenSel = SETTINGS;
            release = false;
            continue;
          }
          barSelected = true;
          release = false;
        } else if (digitalRead(buttonPin) == LOW && barSelected == true) {
          barSelected = false;
          release = false;
        }
      }
    } 
    else if (screenSel == SETTINGS) {  //Actions for a press occuring in the settings screen
      if (release == true) {
        if (digitalRead(buttonPin) == LOW && settingSelected) {  // Toggle setting selected
          settingSelected = false;
          release = false;
        } else if (digitalRead(buttonPin) == LOW && !settingSelected) {
          settingSelected = true;
          release = false;
        }
        if (digitalRead(buttonPin) == LOW) {
          release = false;
          //Reset all settings confirm
          if(highlightedSetting != 6){
            resetCounter = 0;
          }

          switch (highlightedSetting) {  // What to do for button press settings like save
            case 0:                      //Go back
              screenSel = MAIN;
              settingSelected = false;
              break;
            case 4:  //Turn on polling rate tester
              if (testPoll) {
                testPoll = false;
              } else {
                testPoll = true;
              }
              settingSelected = false;
              break;
            case 5:  //Save settings
              Setting(false, true);
              settingSelected = false;
              break;
            case 6:  //Reset all settings
              resetCounter++;
              if(resetCounter > 2){
                SetDefaults();
              }
              settingSelected = false;
              break;
            case 8:  //PinRemap
              screenSel = PINREMAP;
              settingSelected = false;
              break;
            case 9:
              if (smoothing) {
                smoothing = false;
              } else {
                smoothing = true;
              }
              break;
          }
        }
      }
    }
    else if (screenSel == PINREMAP){
      if (release == true) {
        if (digitalRead(buttonPin) == LOW && pinSelected) {
          release = false;
          lastInput = millis();
          pinSelected = false;
        }
        else if(digitalRead(buttonPin) == LOW && !pinSelected){
          release = false;
          lastInput = millis();
          pinSelected = true;
        }
      }
    }
    // FOR A CHANGE IN ROTATION
    // 1 determine change in encoder position
    if (oldPos != encPos) {
      lastInput = millis();
      oldPos = encPos;
      if (screenSel == MAIN) {  // 2 determine if we are in settings screen
        // 3 determine if a bar is selected
        if (barSelected) {
          //3.1 add to bar threshold value based on encoder change
          if (direction == -1) {
            if(threshVal[highlightedBar] > 0){
            threshVal[highlightedBar] -= 16;
            }
          } else {
            if(threshVal[highlightedBar] < 1000){
            threshVal[highlightedBar] += 16;
            }
          }
        } else {
          //2.2 change selected bar up if positive dir down if negitive dir
          if (direction == -1) {
            highlightedBar--;
          } else {
            highlightedBar++;
          }
          //make sure selected bar is always between the input range, lack of "inputs - 1" is for highlighting the settings gear
          if (highlightedBar > inputs) {
            highlightedBar = inputs;
          }
          if (highlightedBar < 0) {
            highlightedBar = 0;
          }
        }
      } 
      else if (screenSel == SETTINGS) {  //if we are in settings screen
        //3 determine if a setting is selected
        if (settingSelected) {
          switch (highlightedSetting) {  // What to do for encoder rotation settings like adjusting refresh rate
            case 1:                      //Screen refresh rate
              if (direction == -1) {
                refreshInt--;
                if (refreshInt < 20) {
                  refreshInt = 20;
                }
              } else {
                refreshInt++;
              }
              break;
            case 2:  //Release threshold
              if (direction == -1) {
                releaseThresh--;
                if (releaseThresh < 1) {
                  releaseThresh = 1;
                }
              } else {
                releaseThresh++;
              }
              break;
            case 3:  //Press threshold
              if (direction == -1) {
                pressThresh--;
                if (pressThresh < 1) {
                  pressThresh = 1;
                }
              } else {
                pressThresh++;
              }
              break;
            case 7:  //Input count
              if (direction == -1) {
                inputs--;
                if (inputs < 1) {
                  inputs = 1;
                }
              } else {
                if (inputs != 8) {
                inputs++;
                }
              }
              break;
          }
        } else {  //3.2 if no setting is selected scroll
          if (direction == -1) {
            scroll = scroll - 10;
            highlightedSetting++;
            if (scroll < scrollLimit[0]) {  //clamp to lower limit
              scroll = scrollLimit[0];
              highlightedSetting = settings;
            }
          } else {
            scroll = scroll + 10;
            highlightedSetting--;
            if (scroll > scrollLimit[1]) {  //clamp to upper limit
              scroll = scrollLimit[1];
              highlightedSetting = 0;
            }
          }
        }
      }
      else if (screenSel == PINREMAP){
        if(!pinSelected){
          if (direction == -1) {
            pinRemapSel--;
          } else {
            pinRemapSel++;
          }
          //make sure selected bar is always between the input range, lack of "inputs - 1" is for highlighting the settings gear
          if (pinRemapSel > 7) {
            pinRemapSel = 7;
          }
          if (pinRemapSel < 0) {
            pinRemapSel = 0;
          }
        }
        else{
          if (direction == -1) {
            if(pindex > 0){
              pindex--;
            }
            inputPins[pinRemapSel] = possiblePins[pindex];
          } else {
            if(pindex < 7){
              pindex++;
            }
            inputPins[pinRemapSel] = possiblePins[pindex];
          }
        }
      }
    }
    //Leave pinremap screen after 5s of no input
    if(screenSel == PINREMAP && millis() > (lastInput + 5000)){
      screenSel = SETTINGS;
      pinSelected = false;
    }
    vTaskDelay(1 / portTICK_PERIOD_MS);
  }
}
