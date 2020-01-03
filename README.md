# Basic Config
Install Arduino (latest, 1.8.10), clone this repo into Documents/Arduino and open it up in Arduino

# Add ESP32 board support to Arduino
File->Preferences and add board manager URL:   
```
https://dl.espressif.com/dl/package_esp32_index.json
```

# Install libraries
Sketch->Include Library->Library Manager

FastLED 3.3.2  
WiFi 1.2.7 (I think this one is installed by default)  
NTPClient 3.2.0

# Setup Board
Plug in the clock to computer w/ Arduino using USB. You should only need to do this once, subsequent flashes can go over Wifi.  
  
Tools->Board->Node32s  
Upload Speed: 115200  
Port: Look for a COM Port named 'Silicon Labs CP210x USB to UART Bridge' in Windows Device Manager  
Flash Frequency: 80MHz

# Edit Sketch 
Edit to add wifi SSID and password, it's much easier to hard code it here than deal with autoconnect when you're modifying things.

# Upload
With this all done you should be able to hit upload and it will take a minute to compile and upload it. The clock should start and you should be able to access http://epochclock.local in a browser if mDNS works on your network (it might take a minute to start resolving). Otherwise you'll need to find the IP and connect that way. 

Once you can access in a browser, you can login with admin:admin and use the web interface to upload new code to it (as long as the last code you uploaded didn't break code uploading!). Use Sketch->Export compiled Binary and then look for the .bin file in the Documents/Arduino/RGBEpoch folder.