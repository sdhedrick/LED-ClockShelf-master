This is a variation of the LED clock from [FlorianL21](https://github.com/florianL21/LED-ClockShelf/)

His was a variation of this project:  [DIY Machines](https://www.instructables.com/id/How-to-Build-a-Giant-Hidden-Shelf-Edge-Clock/).

Please refer to those projects for 3d printed parts and wire diagrams and whatnot.  

NOTE:  I used the esp32/VScode/PlatformIO version from FlorianL21, but modified it as follows.

Removed "Blynk".  

Instead of trying to figure out Blynk, I just created my own ESP Async Web Server on the board and configured it so I could connect to it from a device on my wifi network.  The options I built in:  
1. Turn Off/On Downlighters
2. Turn Off/On Clock
3. Test Mode Off/On
4. Set the Hour Digits color and brightness level
5. Set the Minute Digits color and brightness level
6. Set the Downlights color and brightness level
7. Set the Global Brightness Level
8. Manually restart the controller
9. Output the JSON info.
10.  Reinitialize settings.
11.  Add/delete Preset color combos.

Configured things so that all Async Web Settings are saved to a JSON Setting File using LittleFS and all settings are reestablished on power cycle.

In addition to the Async Web Server, also added WebSerial so that you can see certain output via <clockdevice>/webserial.

Added mDNS so that the machine can resolve on your internal mDNS network (as shelfclock.local for example).

Added functionality to enable it for Alexa.  There are three "Phillips Hue Emulated Lights".  One for the Clock, one for the Downlights and one for the entire device.  These allow you to control on/off and brightness levels for each of those three lights.  You cannot control color this way however.  

I set it up so that one cycle of the original "Test Mode" will cycle on startup.  Kinda neat to see it start up that way.

I removed everything except the "diy-machines" config.  This is because I didn't want to built a 24-hour display.  I had to fix a few bugs related to that 12-hour display.

I changed the OTA update animation (personalized it).

I created a custom way to "dim" the lights.  I think it came out pretty good.  I did not convert things to HSV to do so.  I wrote my own method to dim/brighten the lights.

Also removed the photo-sensor auto-brightness.  It seemed very overly complicated and I may revisit.
