//weather libraries
#include <IRremote.hpp>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <LiquidCrystal_I2C.h>

//remote libraries
#include <IRremote.hpp>

//led libraries
#include <Adafruit_NeoPixel.h>

//receiver stuff
const int RECV_PIN = 12; //change this to the pin of your 

//ALL WIFI STUFF///////////////////////////////////////
//wi-fi credentials here
const char* ssid = "EZConnect";
const char* password = "RHN763Rdn3";

//URL endpoint for the API
String URL = "https://api.openweathermap.org/data/2.5/weather?";
String ApiKey = "e71cc739d9254b5089a24a71518808a2"; //<<-- KAITLYN PUT YOUR API KEY HERE

//location credentials
String lat = "34.290273231641365";
String lon = "-85.18808539881769";

//weather update timers
unsigned long lastUpdate = 0;
unsigned long updateInterval = 2000; // 30 seconds

///////////////////////////////////////////
////////LED STUFF
int ledAmount = 100;
int npPin = 14;
Adafruit_NeoPixel strip(ledAmount, npPin, NEO_GRB);

uint32_t pink = strip.Color(180, 200, 0);
////////////////////////////////////////

//////IR states
int currentIRState = 0;
int prevIRState = 0;

//state thing
int state = 1;

//color for the temperatures
uint32_t veryCold = strip.Color(140, 190, 240);
uint32_t cold = strip.Color(0,90,255);
uint32_t kindaCold = strip.Color(0,255,30);
uint32_t kindaWarm = strip.Color(250,100,0);
uint32_t warm = strip.Color(255,40,0);
uint32_t hot = strip.Color(255,5,0);

uint32_t normal = strip.Color(255,147,41);


//colors for the snow and rain patterns
//for the snow
uint32_t lb0 = strip.Color(0, 20, 60);
uint32_t lb1 = strip.Color(0, 40, 80);
uint32_t lb2 = strip.Color(0, 60, 120);
uint32_t lb3 = strip.Color(0, 80, 160);
uint32_t lb4 = strip.Color(0, 100, 200);
uint32_t lb5 = strip.Color(0, 120, 255);
uint32_t lb6 = strip.Color(30, 150, 255);
uint32_t lb7 = strip.Color(80, 180, 255);
uint32_t lb8 = strip.Color(120, 200, 255);
uint32_t lb9 = strip.Color(160, 220, 255);


//rain blue
uint32_t b0 = strip.Color(0, 0, 10);
uint32_t b1 = strip.Color(0, 0, 25);
uint32_t b2 = strip.Color(0, 0, 45);
uint32_t b3 = strip.Color(0, 0, 70);
uint32_t b4 = strip.Color(0, 0, 100);
uint32_t b5 = strip.Color(0, 0, 130);
uint32_t b6 = strip.Color(0, 0, 160);
uint32_t b7 = strip.Color(0, 0, 190);
uint32_t b8 = strip.Color(10, 10, 220);
uint32_t b9 = strip.Color(20, 20, 255);

uint32_t red = strip.Color(255, 0, 0);

//chatgpt help #1 and #2
float lastTemp = -1000;

int lastWeatherID = -1; //random placeholder number

void setup() {
  Serial.begin(115200);
  //strip stuff
  strip.begin();
  clearFullStrip(); //chatgpt help with lighting up the leds
  strip.clear();
  strip.show();
  //receiver stuff
  IrReceiver.begin(RECV_PIN, ENABLE_LED_FEEDBACK); 

  ////////////WIFI STUFF///////////////////////
  Serial.println();
Serial.println("Connecting to Wifi...");
//connect to a wifi network
WiFi.begin(ssid, password);

while (WiFi.status() != WL_CONNECTED){
  delay(500);
  Serial.print(".");

}

Serial.println("");
Serial.println("WiFi Connected.");
Serial.println("IP address: ");
Serial.println(WiFi.localIP());
///////////////////////////////////

}

//make sure strip.show is called not every frame, and only called whe you need it. like when something updates
//in the if statement when you press it and do the thing. make the change, show it.
//this will not be consistent in the serial monitor. 
void loop() {
  //ir stuff
 handleStateChange();
 handleStateMachine();

//update weather only every 30 seconds
  if (millis() - lastUpdate >= updateInterval) {
    updateWeather();
    lastUpdate = millis();
  }



}



///////STATE MACHINE STUFF

void handleStateChange() {

  if (IrReceiver.decode()) {

    uint32_t rawValue = IrReceiver.decodedIRData.decodedRawData; //HEX CODES ARE A 32 bit value, thats why it has to be uint32_t

    Serial.print("raw HEX: 0x");
    Serial.println(rawValue, HEX); //print the raw value in the HEX format

    // POWER BUTTON LOGIC:
    if (rawValue == 0xBA45FF00) {   

        if (state == 0) {
            state = 1;    // OFF → ON
        } else {
            state = 0;    // ANYTHING ELSE → OFF
        }

        
        Serial.println(state);
    }
//weatherstate button logic
    if(rawValue == 0xE916FF00){
      state = 1;
    }

    if (rawValue == 0xF30CFF00){
      state = 2;
    }
    
    //temperature state
    if (rawValue == 0xE718FF00){
      state = 3;
    }

    IrReceiver.resume();
  }
}



void handleStateMachine(){
  switch(state){
    case 0: //off
    strip.clear();
    strip.show();
    break;

    case 1: //on, warm light
    strip.setBrightness(255);
    setStripColorVariable(normal);
    break;

    case 2:
    //chatgpt help 1 and 2
    strip.setBrightness(255);
    if (lastTemp != -1000){
    updateTempColor(lastTemp); //CHANGE TO lastTemp

    }
    break;
    case 3:
    updateWeatherPattern(lastWeatherID); //CHANGE TO lastWeatherID
    break;

    

  }

}

/////////////MAIN WEATHER STUFF

void updateWeather() {
  //weather information stuff
  if (WiFi.status() == WL_CONNECTED) {

    HTTPClient http;

    String finalURL = URL + "lat=" + lat + "&lon=" + lon + "&units=metric&appid=" + ApiKey;

    http.begin(finalURL);

    int httpCode = http.GET();

    if (httpCode > 0) {
      Serial.print("HTTP response code: ");
      Serial.println(httpCode);

      // JSON data info
      String JSON_Data = http.getString();
      Serial.println("Raw JSON data:");
      Serial.println(JSON_Data);

      DynamicJsonDocument doc(2048);
      deserializeJson(doc, JSON_Data);
      JsonObject obj = doc.as<JsonObject>();

      // get the weather info
      ///WEATHER ID STUFF
      int id = obj["weather"][0]["id"];
      lastWeatherID = id;
      if (state == 3){
        updateWeatherPattern(lastWeatherID); //change to lastWeatherID
      }

      //DESCRIPTIONS AND STUFF
      const char* description = obj["weather"][0]["description"].as<const char*>();
      float tempC = obj["main"]["temp"].as<float>();
      float temp = (tempC * 1.8) + 32;
      //chatgpt help #1
      lastTemp = temp; //doing this so it is up to date all the time not just every 30 seconds

      if (state == 2) {
       updateTempColor(lastTemp); //change to lastTemp
      }

      float humidity = obj["main"]["humidity"].as<float>();

//print weather info
      Serial.println(description);
      Serial.println(temp);
      Serial.println(humidity);
      Serial.println(id);

    } 

    http.end();
  }
  

}

//updating the temperature color based on the reported number
void updateTempColor(float aTemp) {

  uint32_t colorToShow; //a color variable to change based on the temperature

  if (aTemp < 35) {
    colorToShow = veryCold;
  }

  else if (aTemp >= 35 && aTemp < 50) {
    colorToShow = cold;
  }

  else if (aTemp >= 50 && aTemp < 65) {
    colorToShow = kindaCold;
  }

  else if (aTemp >= 65 && aTemp < 75) {
    colorToShow = kindaWarm;
  }

  else if (aTemp >= 75 && aTemp < 90) {
    colorToShow = warm;
  }

  else if(aTemp >= 90){
    colorToShow = hot;
  }

  setStripColorVariable(colorToShow);
}


//////set led strip one color
void setStripColorVariable(uint32_t aColor) {
  for (int i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, aColor);
  }
  strip.show();
}

void updateWeatherPattern(int aID){

  //THUNDERSTORMS
  if(aID >= 200 && aID <= 232){
  thunderStormPattern(b8);

  }

  //drizzle
  else if(aID >= 300 && aID <= 321){
  drizzlePattern(b9);
  }

//light rain
  else if(aID == 500){
  rainPatternPartOne(b0, b1,b2,b3,b4,b5,b6,b7,b8,b9,250);
  }

//moderate rain
else if(aID == 501){
  rainPatternPartOne(b0, b1,b2,b3,b4,b5,b6,b7,b8,b9,100);
}

//heavy and extreme rain
else if(aID >= 502 && aID <= 504){
  rainPatternPartOne(b0, b1,b2,b3,b4,b5,b6,b7,b8,b9,50);

}


//specialty rain?
else if(aID >= 511 && aID <= 531){
threeColorSweep(b0,lb3, b9, 100);
}

//snow
else if(aID >=600 && aID <= 622){
rainPatternPartOne(lb0,lb1,lb2,lb3,lb4,lb5,lb6,lb7,lb8,lb9,100);

}

//clear sky
else if(aID == 800){
 rainbowTimer();

}

//clouds
else if (aID >= 801 && aID <= 804){
cloudPattern(lb8);

}

//TORNADOOOO
else if(aID == 781){
tornadoPattern(red);
}

}
////////////////////////ALL LED FUNCTIONS
///clear sky, rainbow
void rainbowTimer() { // modified from Adafruit example to make it a state machine
  static unsigned long startTime = millis();
  int interval = 20;
  static uint16_t j = 0;


  if (millis() - startTime >= interval){
    for (int i = 0; i < strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel((i + j) & 255));
    }
    strip.show();
    j++;
    if (j >= 256) j = 0;
    startTime = millis(); // time for next change to the display
  }

  
}

uint32_t Wheel(byte WheelPos) {
  if (WheelPos < 85) {
    return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  }
  else if (WheelPos < 170) {
    WheelPos -= 85;
    return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  else {
    WheelPos -= 170;
    return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
}


//cloudy sky
void patternClouds(uint32_t aColor) {
  static int brightness = 40;       //the starting brightness, REMINDER IT HAS TO BE STATIC FOR IT TO REMEMBER WHAT IT WAS THE NEXT TIME IT WAS CALLED 
  static boolean gettingBrighter = true;  //if it is true (which i want it to be when it starts) the brightness will be increasing
  static unsigned long startTime = millis(); 
  unsigned long currentTime = millis(); //not static because i want it to update every frame

  //it is already starting out at 60 so start it out by increasing brightness
  if (currentTime - startTime >= 30) {

    //if getting brighter is true then increase the brightness
    if (gettingBrighter == true) {
      brightness++;

      //if we reach the top brightness i want (180), change the boolean to start decreasing
      if (brightness >= 180) {
        gettingBrighter = false;
      }

    } 
    //if it is false then decrease the brightness
    if (gettingBrighter == false){
      brightness--;
    }

      //if we reach the low brightness, start increasing again
      if (brightness <= 30) {
        gettingBrighter = true;
      }
      strip.setBrightness(brightness); //update the brightness
    setStripColorVariable(aColor); //set the color to what ecolor i put in the paremeter

    startTime = millis(); //restart the timer
    }


    
  }

//rain and Snow patterns
//NOTE: Katherine showed me the code that she used for her MP2 robot head for this pattern. However, I did not copy and paste her code into this. I wrote it up, made sure
//I understood it before going forward with the code. I fully intended to make this into array lists, but I didn't have time unfortunately finals season is killing me hahaahhaa
//shout out to katherine for teaching me this...... there is definitely a way to do this with an array but i honestly did not have time to fix it
void rainPatternPartOne(uint32_t c1, uint32_t c2, uint32_t c3, uint32_t c4, uint32_t c5, uint32_t c6, uint32_t c7, uint32_t c8, uint32_t c9, uint32_t c10, int interval){
static int state = 0;
static int index = 0;

static unsigned long startTime = millis();
unsigned long currentTime = millis();
strip.setBrightness(255);

if(state == 0 && currentTime - startTime >= interval){
  rainPatternPartTwo(c1,c2,c3,c4,c5,c6,c7,c8,c9,c10);
  startTime = millis();
  state = 1;
}
else if(state == 1 && currentTime - startTime >= interval){
  rainPatternPartTwo(c10,c1,c2,c3,c4,c5,c6,c7,c8,c9);
  startTime = millis();
  state = 2;
}
else if(state == 2 && currentTime - startTime >= interval){
  rainPatternPartTwo(c9,c10,c1,c2,c3,c4,c5,c6,c7,c8);
  startTime = millis();
  state = 3;
}
else if(state == 3 && currentTime - startTime >= interval){
  rainPatternPartTwo(c8,c9,c10,c1,c2,c3,c4,c5,c6,c7);
  startTime = millis();
  state = 4;
}
else if(state == 4 && currentTime - startTime >= interval){
  rainPatternPartTwo(c7,c8,c9,c10,c1,c2,c3,c4,c5,c6);
  startTime = millis();
  state = 5;
}
else if(state == 5 && currentTime - startTime >= interval){
  rainPatternPartTwo(c6,c7,c8,c9,c10,c1,c2,c3,c4,c5);
  startTime = millis();
  state = 6;
}
else if(state == 6 && currentTime - startTime >= interval){
  rainPatternPartTwo(c5,c6,c7,c8,c9,c10,c1,c2,c3,c4);
  startTime = millis();
  state = 7;
}
else if(state == 7 && currentTime - startTime >= interval){
  rainPatternPartTwo(c4,c5,c6,c7,c8,c9,c10,c1,c2,c3);
  startTime = millis();
  state = 8;
}
else if(state == 8 && currentTime - startTime >= interval){
  rainPatternPartTwo(c3,c4,c5,c6,c7,c8,c9,c10,c1,c2);
  startTime = millis();
  state = 9;
}
else if(state == 9 && currentTime - startTime >= interval){
  rainPatternPartTwo(c2,c3,c4,c5,c6,c7,c8,c9,c10,c1);
  startTime = millis();
  state = 0;
}

}

void rainPatternPartTwo(uint32_t c1, uint32_t c2, uint32_t c3, uint32_t c4, uint32_t c5, uint32_t c6, uint32_t c7, uint32_t c8, uint32_t c9, uint32_t c10){
for (int i = 0; i<strip.numPixels(); i++){
  if(i%10 == 0){
    strip.setPixelColor(i, c1);
  }

  if(i%10 == 1){
    strip.setPixelColor(i, c2);
  }
  else if(i%10 == 2){
    strip.setPixelColor(i, c3);
  }
  else if(i%10 == 3){
    strip.setPixelColor(i, c4);
  }
  else if(i%10 == 4){
    strip.setPixelColor(i, c5);
  }
  else if(i%10 == 5){
    strip.setPixelColor(i, c6);
  }
  else if(i%10 == 6){
    strip.setPixelColor(i, c7);
  }
  else if(i%10 == 7){
    strip.setPixelColor(i, c8);
  }
  else if(i%10 == 8){
    strip.setPixelColor(i, c9);
  }
  else if(i%10 == 9){
    strip.setPixelColor(i, c10);
  }

}
strip.show();

}


//random rain
void threeColorSweep(uint32_t aColor1, uint32_t aColor2, uint32_t aColor3, int anInterval){
  static int index = 0;
  static unsigned long startTime = millis();
  unsigned long currentTime = millis();
  static int state = 0;

  if (currentTime - startTime >= anInterval && state == 0){
    setThreeColor(aColor1, aColor2, aColor3);
    state = 1;
    startTime = millis();

  }
 else if (currentTime - startTime >= anInterval && state == 1){
    setThreeColor(aColor2, aColor3, aColor1);
    state = 2;
    startTime = millis();
  }
  else if (currentTime - startTime >= anInterval && state == 2){
    setThreeColor(aColor3, aColor1, aColor2);
    state = 0;
    startTime = millis();

  }


}

void setThreeColor(uint32_t aColor1, uint32_t aColor2, uint32_t aColor3){

  for (int i = 0; i < strip.numPixels(); i++){
    //WHAT IS CHANGING AS WE GO THROUGH THE ENTIRE STRIP
    //hey! if index % 2 = 0, be color one. if it is 1, be color two
    
  if(i % 3 == 0){ //if it is an even number
      strip.setPixelColor(i, aColor1); //set the index pixel to color 1
  }
  else if(i% 3 == 1){ //if it is an ODD number
      strip.setPixelColor(i, aColor2); //set the index pixel to color 1


  }
  else if(i% 3 == 2){
strip.setPixelColor(i, aColor3);

  }

  }
strip.show();

}



//clouds!
void cloudPattern(uint32_t aColor) {
  static int brightness = 45;       //the starting brightness, REMINDER IT HAS TO BE STATIC FOR IT TO REMEMBER WHAT IT WAS THE NEXT TIME IT WAS CALLED 
  static boolean gettingBrighter = true;  //if it is true (which i want it to be when it starts) the brightness will be increasing
  static unsigned long startTime = millis(); 
  unsigned long currentTime = millis(); //not static because i want it to update every frame

  //it is already starting out at 60 so start it out by increasing brightness
  if (currentTime - startTime >= 30) {

    //if getting brighter is true then increase the brightness
    if (gettingBrighter == true) {
      brightness++;

      //if we reach the top brightness i want (180), change the boolean to start decreasing
      if (brightness >= 180) {
        gettingBrighter = false;
      }

    } 
    //if it is false then decrease the brightness
    if (gettingBrighter == false){
      brightness--;
    }

      //if we reach the low brightness, start increasing again
      if (brightness <= 30) {
        gettingBrighter = true;
      }
      strip.setBrightness(brightness); //update the brightness
    setStripColorVariable(aColor); //set the color to what ecolor i put in the paremeter

    startTime = millis(); //restart the timer
    }


    
  }

//thunderstorm
void thunderStormPattern(uint32_t aColor) {
 //timer stuff
  static unsigned long flashStartTime = millis();   //normal flash startTime
  static int normalFlashInterval = 500; //larger interval so it is less rare, idk it flashes more often

  static unsigned long rareFlashStartTime = millis(); //rare white flash
  static int rareFlashInterval = 3000; //random 2-6 seconds

  unsigned long currentTime = millis(); //currentTime for both of them

//normal glow of the thunderstorm color
  strip.setBrightness(100);
  setStripColorVariable(aColor); //set it blue
  strip.show();



  //normal blue flash
  if (currentTime - flashStartTime >= normalFlashInterval) {
    strip.setBrightness(255);  //increase the brightness real quick
    strip.show();
    delay(40); 

    strip.setBrightness(100);  //back to the original storm glow
    strip.show();

    flashStartTime = millis(); //reset the timer

    normalFlashInterval = random(300, 1200); //make it a random interval between 0.3 and 1.2 seconds
  }



  //rare white flash
  if (currentTime - rareFlashStartTime >= rareFlashInterval) { //randomly every 2 to 6 seconds
    setStripColorVariable(lb8); //set it randomly to this light blue
    strip.setBrightness(255);
    strip.show();

    delay(60); //a little longer delay

    //set it back to the original storm glow
    strip.setBrightness(100);
    setStripColorVariable(aColor);
    strip.show();

    rareFlashStartTime = millis();
    rareFlashInterval = random(2000, 4000); //generate a new random interval between 2 and 4 seconds. it happens less

  }

  
}


//drizzle pattern - CHATGPT HELPED MAKE THIS ONE
void drizzlePattern(uint32_t aColor) {

  // static = remember these values between frames
  static int index = 0;                       
  static unsigned long startTime = millis();  
  unsigned long currentTime = millis();       

  // update speed (drizzle is slower than rain)
  if (currentTime - startTime >= 120) {

    strip.clear();

    // 5 drizzle droplets spaced evenly
    for (int d = 0; d < 5; d++) {

      // each droplet starts at a different offset
      int dropletPos = (index + d * 10) % strip.numPixels();

      // main droplet (bright)
      strip.setPixelColor(dropletPos, aColor);

      // small fade tail behind drizzle drop
      if (dropletPos - 1 >= 0) {
        strip.setPixelColor(dropletPos - 1, strip.Color(0, 0, 150)); // lighter drizzle
      }

      if (dropletPos - 2 >= 0) {
        strip.setPixelColor(dropletPos - 2, strip.Color(0, 0, 60)); // even dimmer
      }
    }

    strip.show();

    // Move drizzle downward
    index++;

    // Loop back to the top when reaching the end
    if (index >= strip.numPixels()) {
      index = 0;
    }

    // Restart timer
    startTime = millis();
  }
}

//tornado pattern
void tornadoPattern(uint32_t aColor) {
  static int index = 0;
  static unsigned long startTime = millis();
  unsigned long currentTime = millis();

  if (currentTime - startTime >= 10) {  // SUPER FAST
    strip.clear();
    strip.setPixelColor(index, aColor);
    strip.show();

    index++;
    if (index >= strip.numPixels()) index = 0;

    startTime = millis();
  }
}

//debugging, chatgpt help
void clearFullStrip() {
  for (int i = 0; i < 300; i++) {  // your actual strip length
    strip.setPixelColor(i, 0);
  }
  strip.show();
}
