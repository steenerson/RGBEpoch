
#include <FastLED.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Update.h>
#include <NTPClient.h>
// #include <AutoConnect.h>

// autoconnect is a lot more annoying than just setting SSID+pass here, 
const char* ssid = "ENTER SSID HERE";
const char* password = "ENTER PASSWORD HERE";

// sets hostname, used for wifi+mDNS
const char* host = "epochclock"; 

// How many leds are in the strip? Setting to something besides 32 probably will need major rework
#define NUM_LEDS 32
// LED data pins
#define BACK_LED_PIN 12
#define FRONT_LED_PIN 13

// These are arrays of LEDs.  Each has an item for each led in your strip.
CRGB back_leds[NUM_LEDS];
CRGB front_leds[NUM_LEDS];

int currentEpochTime = 0;

// holds LED hue because is translated to RGB values so you can't read back the hue after setting it. so we need to store it to increment it. we'll fill it later when we get iterative
byte ledHue[NUM_LEDS];

// Define NTP Client to get time
WiFiUDP ntpUDP;

// By default 'pool.ntp.org' is used with 60 seconds update interval and
// no offset
NTPClient timeClient(ntpUDP);

WebServer server(80);

// start wifi autoconnect
// AutoConnect      Portal(server);

/*
   Web server login page, you can change username/password from admin:admin to something else here
*/
const char* loginIndex =
  "<form name='loginForm'>"
  "<table width='20%' bgcolor='A09F9F' align='center'>"
  "<tr>"
  "<td colspan=2>"
  "<center><font size=4><b>ESP32 Login Page</b></font></center>"
  "<br>"
  "</td>"
  "<br>"
  "<br>"
  "</tr>"
  "<td>Username:</td>"
  "<td><input type='text' size=25 name='userid'><br></td>"
  "</tr>"
  "<br>"
  "<br>"
  "<tr>"
  "<td>Password:</td>"
  "<td><input type='Password' size=25 name='pwd'><br></td>"
  "<br>"
  "<br>"
  "</tr>"
  "<tr>"
  "<td><input type='submit' onclick='check(this.form)' value='Login'></td>"
  "</tr>"
  "</table>"
  "</form>"
  "<script>"
  "function check(form)"
  "{"
  "if(form.userid.value=='admin' && form.pwd.value=='admin')"
  "{"
  "window.open('/serverIndex')"
  "}"
  "else"
  "{"
  " alert('Error Password or Username')/*displays error message*/"
  "}"
  "}"
  "</script>";

/*
   Server Index Page
*/
const char* serverIndex =
  "<script src='https://ajax.googleapis.com/ajax/libs/jquery/3.2.1/jquery.min.js'></script>"
  "<form method='POST' action='#' enctype='multipart/form-data' id='upload_form'>"
  "<input type='file' name='update'>"
  "<input type='submit' value='Update'>"
  "</form>"
  "<div id='prg'>progress: 0%</div>"
  "<script>"
  "$('form').submit(function(e){"
  "e.preventDefault();"
  "var form = $('#upload_form')[0];"
  "var data = new FormData(form);"
  " $.ajax({"
  "url: '/update',"
  "type: 'POST',"
  "data: data,"
  "contentType: false,"
  "processData:false,"
  "xhr: function() {"
  "var xhr = new window.XMLHttpRequest();"
  "xhr.upload.addEventListener('progress', function(evt) {"
  "if (evt.lengthComputable) {"
  "var per = evt.loaded / evt.total;"
  "$('#prg').html('progress: ' + Math.round(per*100) + '%');"
  "}"
  "}, false);"
  "return xhr;"
  "},"
  "success:function(d, s) {"
  "console.log('success!')"
  "},"
  "error: function (a, b, c) {"
  "}"
  "});"
  "});"
  "</script>";

/*
   setup function, runs once on boot before the loop function
*/
void setup(void) {
  // sanity check delay - allows reprogramming if accidently blowing power w/leds
  delay(2000);

  // initialize LED strings
  FastLED.addLeds<WS2811, BACK_LED_PIN, RGB>(front_leds, NUM_LEDS);
  FastLED.addLeds<WS2811, FRONT_LED_PIN, RGB>(back_leds, NUM_LEDS);

  // start serial monitor
  Serial.begin(115200);

  // Connect to WiFi network using hard coded creds
  WiFi.begin(ssid, password);
  // Connect to WiFi using autoconnect
  //WiFi.begin();

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  //use mDNS for host name resolution, so you don't have to look up which IP it gets
  if (!MDNS.begin(host)) { //http://epochclock.local or whatever you set host to above
    Serial.println("Error setting up MDNS responder!");
  }
  else {
    Serial.println("mDNS responder started");
  }

  /*setup web server for OTAs, return index page which is stored in serverIndex */
  server.on("/", HTTP_GET, []() {
    server.sendHeader("Connection", "close");
    server.send(200, "text/html", loginIndex);
  });
  server.on("/serverIndex", HTTP_GET, []() {
    server.sendHeader("Connection", "close");
    server.send(200, "text/html", serverIndex);
  });
  /*handling uploading firmware file */
  server.on("/update", HTTP_POST, []() {
    server.sendHeader("Connection", "close");
    server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
    ESP.restart();
  }, []() {
    HTTPUpload& upload = server.upload();
    if (upload.status == UPLOAD_FILE_START) {
      Serial.printf("Update: %s\n", upload.filename.c_str());
      if (!Update.begin(UPDATE_SIZE_UNKNOWN)) { // start with max available size
        Update.printError(Serial);
      }
    } else if (upload.status == UPLOAD_FILE_WRITE) {
      /* flashing firmware to ESP*/
      if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
        Update.printError(Serial);
      }
    } else if (upload.status == UPLOAD_FILE_END) {
      if (Update.end(true)) { // true to set the size to the current progress
        Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
      } else {
        Update.printError(Serial);
      }
    }
  });
  server.begin();

  // more autoconnect stuff
  //if (Portal.begin()) {
  //  Serial.println("HTTP server:" + WiFi.localIP().toString());
  //}

  Serial.println("");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // start NTP client so it keeps time updated
  timeClient.begin();
  timeClient.update();
  delay(500);

  // set current time. this is a 32-bit signed integer, and we can just read individual
  // bits from it using bitRead() to control the LEDs
  currentEpochTime = timeClient.getEpochTime();

  // loop through LEDs and initialize them
  for (int ledPosition = 0; ledPosition < NUM_LEDS; ledPosition = ledPosition + 1) {

    // read the bit at this spot from the current time integer
    int ledStatus = bitRead(currentEpochTime, ledPosition);

    // update Hue array with a random color for each LED
    ledHue[ledPosition] = random8();

    // need this mapping because the LED string is reversed, otherwise the 1s place would be the LED closest to the controller
    int mapping = NUM_LEDS - 1 - ledPosition;

    // turn on the 1s
    if (ledStatus == 1) {
      front_leds[mapping] = CHSV(ledHue[mapping], 255, 255);
      back_leds[mapping] = CHSV(ledHue[mapping], 255, 255);
    }

    // turn off the 0s
    else {
      back_leds[mapping] = CRGB::White;
      front_leds[mapping] = CRGB::Black;
    }
  }
  // updating front_leds/back_leds doesn't actually visibly change the LEDs until you run FastLED.show()
  FastLED.show();
}

// loop function, this runs after setup and repeats forever
void loop(void) {
  // handle web server requests and OTA updates
  server.handleClient();
  // wifi autoconfig
  //Portal.handleClient();

  // update time variables. need the time used for the last loop and the current time to know which LEDs to fade in/out
  int lastEpochTime = currentEpochTime;
  currentEpochTime = timeClient.getEpochTime();

  // create 32-bit integers where each bit will control if an LED needs to fade on or off. bitwise AND NOT does this by magic
  // each bit of fadeUpLeds is 1 only if the same bit in currentEpochTime is 1 and lastEpochTime is 0, so we have a list of LEDs to turn on
  int fadeUpLeds = currentEpochTime & ~lastEpochTime;
  // each bit of fadeDownLeds is 1 only if the same bit in currentEpochTime is 0 and lastEpochTime is 1, so we have a list of LEDs to turn off
  int fadeDownLeds = lastEpochTime & ~currentEpochTime;

  // does the same thing bit by bit
  /*
    for (int fadeBit = 0; fadeBit < 32; fadeBit = fadeBit + 1) {
      if (bitRead(currentEpochTime, fadeBit) == 0 && bitRead(lastEpochTime, fadeBit) == 1) {
        bitSet(fadeDownLeds, fadeBit);
      }
      else if (bitRead(currentEpochTime, fadeBit) == 1 && bitRead(lastEpochTime, fadeBit) == 0) {
        bitSet(fadeUpLeds, fadeBit);
      }
    }
  */

  // loop through LEDs to update colors.
  for (int ledPosition = 0; ledPosition < NUM_LEDS; ledPosition = ledPosition + 1) {

    // update hue of each LED. add 2 then subtract random number so they drift randomly
    // (0, 4) means 0-3 inclusive with random8()
    ledHue[ledPosition] = ledHue[ledPosition] + 2 - random8(0, 4);

    // randomize LED 0 if it's about to turn on
    if (bitRead(fadeUpLeds, 0) == 1) {
      ledHue[0] = random8();
    }

    // only one LED should turn on each second. when we get to that LED,
    // copy the color from the one under it, unless it's LED 0
    if (bitRead(fadeUpLeds, ledPosition) == 1 && ledPosition > 0 ) {
      // you can start a loop here counting down from ledPosition to shift everything over but it's hard for eyes to follow
      ledHue[ledPosition] = ledHue[ledPosition - 1];
    }

    // reverse order mapping
    int mapping = NUM_LEDS - 1 - ledPosition;

    // update all ON LEDs to apply incremented hue. this also makes the LED that's turning on blink
    // full brightness for a bit before going back to 0 and fading in which looks kinda cool
    if (bitRead(currentEpochTime, ledPosition) == 1) {
      front_leds[mapping] = CHSV(ledHue[ledPosition], 255, 255);
      back_leds[mapping] = CHSV(ledHue[ledPosition], 255, 255);
    }
  }

  // this command actually applies the updates to all of the LEDs.
  // comment out to remove the blink from the LED that turns on each cycle
  FastLED.show();
  delay(14);

  // fade LED strings, loop through 64 times for buttery smooth fading.
  // 10ms delay or less between string updates caused problems during testing,
  // but 64 works with 14 ms delays and seems reliable
  for (int fadeVal = 0; fadeVal <= 63; fadeVal = fadeVal + 1) {
    
    // loop through each LED position
    for (int ledPosition = 0; ledPosition < NUM_LEDS; ledPosition = ledPosition + 1) {
      
      // reverse order mapping
      int mapping = NUM_LEDS - 1 - ledPosition;

      // update fade up LEDs
      if (bitRead(fadeUpLeds, ledPosition) == 1) {
        front_leds[mapping] = CHSV(ledHue[ledPosition], 255, fadeVal * 4);
        // back_leds[mapping] = CHSV(ledHue[ledPosition], 255, fadeVal * 4);
      }
      
      // update fade down LEDs
      else if (bitRead(fadeDownLeds, ledPosition) == 1) {
        front_leds[mapping] = CHSV(ledHue[ledPosition], 255, 253 - (fadeVal * 4));
        // back_leds[mapping] = CHSV(ledHue[ledPosition], 255, 253 - (fadeVal * 4));
      }

      // if fade is divisible by 31, randomly tweak colors again. you can make it
      //constantly rainbow swirly if you crank these a bit
      else {
        if ( (fadeVal % 31) == 0 ) {
          ledHue[ledPosition] = ledHue[ledPosition] + 1 - random8(0, 3); // random8(0, 3) returns 0, 1 or 2
          if (bitRead(currentEpochTime, ledPosition) == 1) {
            front_leds[mapping] = CHSV(ledHue[ledPosition], 255, 255);
            back_leds[mapping] = CHSV(ledHue[ledPosition], 255, 255);
          }
        }
      }
    }
    //display changes and wait a bit before next fade loop
    FastLED.show();
    delay(13);
  }

  // loop through again and totally turn off fade down LEDs, in case fading leaves them non zero
  for (int ledPosition = 1; ledPosition < NUM_LEDS; ledPosition = ledPosition + 1) {
    int mapping = NUM_LEDS - 1 - ledPosition;
    if (bitRead(fadeDownLeds, ledPosition) == 1) {
      // back LEDs are white when 0 for backlighting
      back_leds[mapping] = CRGB::White;
      // front LEDs are black for 0
      front_leds[mapping] = CRGB::Black;
    }
  }
  FastLED.show();

  // wait for clock to tick to next second before starting next loop
  while ( timeClient.getEpochTime() < currentEpochTime + 1 ) {
    delay(1);
  }
}
