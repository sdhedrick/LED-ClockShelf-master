/**
 * \file main.cpp
 * \author Florian Laschober
 * \brief Glue logic to bring together all the modules and some additional stuff
 */

#include "Configuration.h"
#include <Arduino.h>
#include "DisplayManager.h"
#include "ClockState.h"
#include <ESPmDNS.h> // For mDNS
#include <AsyncTCP.h> // For Async webserver
#include <ESPAsyncWebServer.h> // For Async webserver
#include <EEPROM.h> // For reading/writing to EEPROM on the board (save values during power cycle)
#include <WebSerial.h> // for sending debug to web.

#if ENABLE_ALEXA == true
	#include "fauxmoESP.h"
#endif

#if RUN_WITHOUT_WIFI == false
	#include "WiFi.h"
#endif
#if ENABLE_OTA_UPLOAD == true
	#include <ArduinoOTA.h>
#endif

DisplayManager* ShelfDisplays = DisplayManager::getInstance();
TimeManager* timeM = TimeManager::getInstance();
ClockState* states = ClockState::getInstance();

#if ENABLE_OTA_UPLOAD == true
	void setupOTA();
#endif
#if RUN_WITHOUT_WIFI == false
	void wifiSetup();
#endif

// Declarations of functions that are below
void TimerTick();
void TimerDone();
void AlarmTriggered();
void startupAnimation();
void initEEPROMOnStartup();
String webOnOffButtonState(int);
String webProcessor(const String&);
void toggleDownlights(int, int);
void toggleClocklights(int, int);
void toggleSchlock(int, int);
void runTestModeOnStartup();
CRGB clamp_rgb(CRGB, int);
void outputEEPROMValues();
void outputESPMemory();

// Web Server Variables
// On/Off button states
const char* PARAM_INPUT_1 = "button";
const char* PARAM_INPUT_2 = "state";

// Tracking variables for web/eeprom/power-cycle
/*
	EEPROM Address usage
	0: 			downlighter On/Off state
	1:			clock On/Off state
	2:			Test Mode.  0 or 255 for normal and 1 for Test Mode.
	3:			saved global default brightness level
*/
#define EEPROM_SIZE 9

// EEPROM Addresses
const int EE_downlighterOnOffState = 0;
const int EE_clockOnOffState = 1;
const int EE_testMode = 2;
const int EE_defaultGlobalBrightnessLevel = 3;  // 0 (off) thru 253
const int EE_defaultHourColor = 4; // index into an array of CRGB.  Black 0, White 255.
const int EE_defaultMinColor = 5;
const int EE_defaultDLColor = 6;
const int EE_currentClockBrightnessLevel = 7;
const int EE_currentDLBrightnessLevel = 8;

// Default Web values
bool downlightersOnOffState = DEF_DOWNLIGHTERSONOFFSTATE;
bool clockOnOffState = DEF_CLOCKONOFFSTATE;
bool testMode = TEST_MODE;
int defaultGlobalBrightnessLevel = DEFAULT_CLOCK_BRIGHTNESS;
int currentClockBrightnessLevel = DEFAULT_CLOCK_BRIGHTNESS;
int currentDLBrightnessLevel = DEFAULT_CLOCK_BRIGHTNESS;
CRGB defaultHourColor = HOUR_COLOR;
CRGB defaultMinColor = MINUTE_COLOR;
CRGB defaultDLColor = INTERNAL_COLOR;


struct crgbcolor {
	String colornamecrgb;
	uint32_t colorcrgb;
	String colorvaluenamehex;
	uint32_t colorvaluehex;
};

#define NUM_COLORS 148
struct crgbcolor arr_crgbcolors[NUM_COLORS] =
{
{"CRGB::AliceBlue", CRGB::AliceBlue, "#F0F8FF", 0xF0F8FF},
{"CRGB::Amethyst", CRGB::Amethyst, "#9966CC", 0x9966CC},
{"CRGB::AntiqueWhite", CRGB::AntiqueWhite, "#FAEBD7", 0xFAEBD7},
{"CRGB::Aqua", CRGB::Aqua, "#00FFFF", 0x00FFFF},
{"CRGB::Aquamarine", CRGB::Aquamarine, "#7FFFD4", 0x7FFFD4},
{"CRGB::Azure", CRGB::Azure, "#F0FFFF", 0xF0FFFF},
{"CRGB::Beige", CRGB::Beige, "#F5F5DC", 0xF5F5DC},
{"CRGB::Bisque", CRGB::Bisque, "#FFE4C4", 0xFFE4C4},
{"CRGB::Black", CRGB::Black, "#000000", 0x000000},
{"CRGB::BlanchedAlmond", CRGB::BlanchedAlmond, "#FFEBCD", 0xFFEBCD},
{"CRGB::Blue", CRGB::Blue, "#0000FF", 0x0000FF},
{"CRGB::BlueViolet", CRGB::BlueViolet, "#8A2BE2", 0x8A2BE2},
{"CRGB::Brown", CRGB::Brown, "#A52A2A", 0xA52A2A},
{"CRGB::BurlyWood", CRGB::BurlyWood, "#DEB887", 0xDEB887},
{"CRGB::CadetBlue", CRGB::CadetBlue, "#5F9EA0", 0x5F9EA0},
{"CRGB::Chartreuse", CRGB::Chartreuse, "#7FFF00", 0x7FFF00},
{"CRGB::Chocolate", CRGB::Chocolate, "#D2691E", 0xD2691E},
{"CRGB::Coral", CRGB::Coral, "#FF7F50", 0xFF7F50},
{"CRGB::CornflowerBlue", CRGB::CornflowerBlue, "#6495ED", 0x6495ED},
{"CRGB::Cornsilk", CRGB::Cornsilk, "#FFF8DC", 0xFFF8DC},
{"CRGB::Crimson", CRGB::Crimson, "#DC143C", 0xDC143C},
{"CRGB::Cyan", CRGB::Cyan, "#00FFFF", 0x00FFFF},
{"CRGB::DarkBlue", CRGB::DarkBlue, "#00008B", 0x00008B},
{"CRGB::DarkCyan", CRGB::DarkCyan, "#008B8B", 0x008B8B},
{"CRGB::DarkGoldenrod", CRGB::DarkGoldenrod, "#B8860B", 0xB8860B},
{"CRGB::DarkGray", CRGB::DarkGray, "#A9A9A9", 0xA9A9A9},
{"CRGB::DarkGrey", CRGB::DarkGrey, "#A9A9A9", 0xA9A9A9},
{"CRGB::DarkGreen", CRGB::DarkGreen, "#006400", 0x006400},
{"CRGB::DarkKhaki", CRGB::DarkKhaki, "#BDB76B", 0xBDB76B},
{"CRGB::DarkMagenta", CRGB::DarkMagenta, "#8B008B", 0x8B008B},
{"CRGB::DarkOliveGreen", CRGB::DarkOliveGreen, "#556B2F", 0x556B2F},
{"CRGB::DarkOrange", CRGB::DarkOrange, "#FF8C00", 0xFF8C00},
{"CRGB::DarkOrchid", CRGB::DarkOrchid, "#9932CC", 0x9932CC},
{"CRGB::DarkRed", CRGB::DarkRed, "#8B0000", 0x8B0000},
{"CRGB::DarkSalmon", CRGB::DarkSalmon, "#E9967A", 0xE9967A},
{"CRGB::DarkSeaGreen", CRGB::DarkSeaGreen, "#8FBC8F", 0x8FBC8F},
{"CRGB::DarkSlateBlue", CRGB::DarkSlateBlue, "#483D8B", 0x483D8B},
{"CRGB::DarkSlateGray", CRGB::DarkSlateGray, "#2F4F4F", 0x2F4F4F},
{"CRGB::DarkSlateGrey", CRGB::DarkSlateGrey, "#2F4F4F", 0x2F4F4F},
{"CRGB::DarkTurquoise", CRGB::DarkTurquoise, "#00CED1", 0x00CED1},
{"CRGB::DarkViolet", CRGB::DarkViolet, "#9400D3", 0x9400D3},
{"CRGB::DeepPink", CRGB::DeepPink, "#FF1493", 0xFF1493},
{"CRGB::DeepSkyBlue", CRGB::DeepSkyBlue, "#00BFFF", 0x00BFFF},
{"CRGB::DimGray", CRGB::DimGray, "#696969", 0x696969},
{"CRGB::DimGrey", CRGB::DimGrey, "#696969", 0x696969},
{"CRGB::DodgerBlue", CRGB::DodgerBlue, "#1E90FF", 0x1E90FF},
{"CRGB::FireBrick", CRGB::FireBrick, "#B22222", 0xB22222},
{"CRGB::FloralWhite", CRGB::FloralWhite, "#FFFAF0", 0xFFFAF0},
{"CRGB::ForestGreen", CRGB::ForestGreen, "#228B22", 0x228B22},
{"CRGB::Fuchsia", CRGB::Fuchsia, "#FF00FF", 0xFF00FF},
{"CRGB::Gainsboro", CRGB::Gainsboro, "#DCDCDC", 0xDCDCDC},
{"CRGB::GhostWhite", CRGB::GhostWhite, "#F8F8FF", 0xF8F8FF},
{"CRGB::Gold", CRGB::Gold, "#FFD700", 0xFFD700},
{"CRGB::Goldenrod", CRGB::Goldenrod, "#DAA520", 0xDAA520},
{"CRGB::Gray", CRGB::Gray, "#808080", 0x808080},
{"CRGB::Grey", CRGB::Grey, "#808080", 0x808080},
{"CRGB::Green", CRGB::Green, "#008000", 0x008000},
{"CRGB::GreenYellow", CRGB::GreenYellow, "#ADFF2F", 0xADFF2F},
{"CRGB::Honeydew", CRGB::Honeydew, "#F0FFF0", 0xF0FFF0},
{"CRGB::HotPink", CRGB::HotPink, "#FF69B4", 0xFF69B4},
{"CRGB::IndianRed", CRGB::IndianRed, "#CD5C5C", 0xCD5C5C},
{"CRGB::Indigo", CRGB::Indigo, "#4B0082", 0x4B0082},
{"CRGB::Ivory", CRGB::Ivory, "#FFFFF0", 0xFFFFF0},
{"CRGB::Khaki", CRGB::Khaki, "#F0E68C", 0xF0E68C},
{"CRGB::Lavender", CRGB::Lavender, "#E6E6FA", 0xE6E6FA},
{"CRGB::LavenderBlush", CRGB::LavenderBlush, "#FFF0F5", 0xFFF0F5},
{"CRGB::LawnGreen", CRGB::LawnGreen, "#7CFC00", 0x7CFC00},
{"CRGB::LemonChiffon", CRGB::LemonChiffon, "#FFFACD", 0xFFFACD},
{"CRGB::LightBlue", CRGB::LightBlue, "#ADD8E6", 0xADD8E6},
{"CRGB::LightCoral", CRGB::LightCoral, "#F08080", 0xF08080},
{"CRGB::LightCyan", CRGB::LightCyan, "#E0FFFF", 0xE0FFFF},
{"CRGB::LightGoldenrodYellow", CRGB::LightGoldenrodYellow, "#FAFAD2", 0xFAFAD2},
{"CRGB::LightGreen", CRGB::LightGreen, "#90EE90", 0x90EE90},
{"CRGB::LightGrey", CRGB::LightGrey, "#D3D3D3", 0xD3D3D3},
{"CRGB::LightPink", CRGB::LightPink, "#FFB6C1", 0xFFB6C1},
{"CRGB::LightSalmon", CRGB::LightSalmon, "#FFA07A", 0xFFA07A},
{"CRGB::LightSeaGreen", CRGB::LightSeaGreen, "#20B2AA", 0x20B2AA},
{"CRGB::LightSkyBlue", CRGB::LightSkyBlue, "#87CEFA", 0x87CEFA},
{"CRGB::LightSlateGray", CRGB::LightSlateGray, "#778899", 0x778899},
{"CRGB::LightSlateGrey", CRGB::LightSlateGrey, "#778899", 0x778899},
{"CRGB::LightSteelBlue", CRGB::LightSteelBlue, "#B0C4DE", 0xB0C4DE},
{"CRGB::LightYellow", CRGB::LightYellow, "#FFFFE0", 0xFFFFE0},
{"CRGB::Lime", CRGB::Lime, "#00FF00", 0x00FF00},
{"CRGB::LimeGreen", CRGB::LimeGreen, "#32CD32", 0x32CD32},
{"CRGB::Linen", CRGB::Linen, "#FAF0E6", 0xFAF0E6},
{"CRGB::Magenta", CRGB::Magenta, "#FF00FF", 0xFF00FF},
{"CRGB::Maroon", CRGB::Maroon, "#800000", 0x800000},
{"CRGB::MediumAquamarine", CRGB::MediumAquamarine, "#66CDAA", 0x66CDAA},
{"CRGB::MediumBlue", CRGB::MediumBlue, "#0000CD", 0x0000CD},
{"CRGB::MediumOrchid", CRGB::MediumOrchid, "#BA55D3", 0xBA55D3},
{"CRGB::MediumPurple", CRGB::MediumPurple, "#9370DB", 0x9370DB},
{"CRGB::MediumSeaGreen", CRGB::MediumSeaGreen, "#3CB371", 0x3CB371},
{"CRGB::MediumSlateBlue", CRGB::MediumSlateBlue, "#7B68EE", 0x7B68EE},
{"CRGB::MediumSpringGreen", CRGB::MediumSpringGreen, "#00FA9A", 0x00FA9A},
{"CRGB::MediumTurquoise", CRGB::MediumTurquoise, "#48D1CC", 0x48D1CC},
{"CRGB::MediumVioletRed", CRGB::MediumVioletRed, "#C71585", 0xC71585},
{"CRGB::MidnightBlue", CRGB::MidnightBlue, "#191970", 0x191970},
{"CRGB::MintCream", CRGB::MintCream, "#F5FFFA", 0xF5FFFA},
{"CRGB::MistyRose", CRGB::MistyRose, "#FFE4E1", 0xFFE4E1},
{"CRGB::Moccasin", CRGB::Moccasin, "#FFE4B5", 0xFFE4B5},
{"CRGB::NavajoWhite", CRGB::NavajoWhite, "#FFDEAD", 0xFFDEAD},
{"CRGB::Navy", CRGB::Navy, "#000080", 0x000080},
{"CRGB::OldLace", CRGB::OldLace, "#FDF5E6", 0xFDF5E6},
{"CRGB::Olive", CRGB::Olive, "#808000", 0x808000},
{"CRGB::OliveDrab", CRGB::OliveDrab, "#6B8E23", 0x6B8E23},
{"CRGB::Orange", CRGB::Orange, "#FFA500", 0xFFA500},
{"CRGB::OrangeRed", CRGB::OrangeRed, "#FF4500", 0xFF4500},
{"CRGB::Orchid", CRGB::Orchid, "#DA70D6", 0xDA70D6},
{"CRGB::PaleGoldenrod", CRGB::PaleGoldenrod, "#EEE8AA", 0xEEE8AA},
{"CRGB::PaleGreen", CRGB::PaleGreen, "#98FB98", 0x98FB98},
{"CRGB::PaleTurquoise", CRGB::PaleTurquoise, "#AFEEEE", 0xAFEEEE},
{"CRGB::PaleVioletRed", CRGB::PaleVioletRed, "#DB7093", 0xDB7093},
{"CRGB::PapayaWhip", CRGB::PapayaWhip, "#FFEFD5", 0xFFEFD5},
{"CRGB::PeachPuff", CRGB::PeachPuff, "#FFDAB9", 0xFFDAB9},
{"CRGB::Peru", CRGB::Peru, "#CD853F", 0xCD853F},
{"CRGB::Pink", CRGB::Pink, "#FFC0CB", 0xFFC0CB},
{"CRGB::Plaid", CRGB::Plaid, "#CC5533", 0xCC5533},
{"CRGB::Plum", CRGB::Plum, "#DDA0DD", 0xDDA0DD},
{"CRGB::PowderBlue", CRGB::PowderBlue, "#B0E0E6", 0xB0E0E6},
{"CRGB::Purple", CRGB::Purple, "#800080", 0x800080},
{"CRGB::Red", CRGB::Red, "#FF0000", 0xFF0000},
{"CRGB::RosyBrown", CRGB::RosyBrown, "#BC8F8F", 0xBC8F8F},
{"CRGB::RoyalBlue", CRGB::RoyalBlue, "#4169E1", 0x4169E1},
{"CRGB::SaddleBrown", CRGB::SaddleBrown, "#8B4513", 0x8B4513},
{"CRGB::Salmon", CRGB::Salmon, "#FA8072", 0xFA8072},
{"CRGB::SandyBrown", CRGB::SandyBrown, "#F4A460", 0xF4A460},
{"CRGB::SeaGreen", CRGB::SeaGreen, "#2E8B57", 0x2E8B57},
{"CRGB::Seashell", CRGB::Seashell, "#FFF5EE", 0xFFF5EE},
{"CRGB::Sienna", CRGB::Sienna, "#A0522D", 0xA0522D},
{"CRGB::Silver", CRGB::Silver, "#C0C0C0", 0xC0C0C0},
{"CRGB::SkyBlue", CRGB::SkyBlue, "#87CEEB", 0x87CEEB},
{"CRGB::SlateBlue", CRGB::SlateBlue, "#6A5ACD", 0x6A5ACD},
{"CRGB::SlateGray", CRGB::SlateGray, "#708090", 0x708090},
{"CRGB::SlateGrey", CRGB::SlateGrey, "#708090", 0x708090},
{"CRGB::Snow", CRGB::Snow, "#FFFAFA", 0xFFFAFA},
{"CRGB::SpringGreen", CRGB::SpringGreen, "#00FF7F", 0x00FF7F},
{"CRGB::SteelBlue", CRGB::SteelBlue, "#4682B4", 0x4682B4},
{"CRGB::Tan", CRGB::Tan, "#D2B48C", 0xD2B48C},
{"CRGB::Teal", CRGB::Teal, "#008080", 0x008080},
{"CRGB::Thistle", CRGB::Thistle, "#D8BFD8", 0xD8BFD8},
{"CRGB::Tomato", CRGB::Tomato, "#FF6347", 0xFF6347},
{"CRGB::Turquoise", CRGB::Turquoise, "#40E0D0", 0x40E0D0},
{"CRGB::Violet", CRGB::Violet, "#EE82EE", 0xEE82EE},
{"CRGB::Wheat", CRGB::Wheat, "#F5DEB3", 0xF5DEB3},
{"CRGB::White", CRGB::White, "#FFFFFF", 0xFFFFFF},
{"CRGB::WhiteSmoke", CRGB::WhiteSmoke, "#F5F5F5", 0xF5F5F5},
{"CRGB::Yellow", CRGB::Yellow, "#FFFF00", 0xFFFF00},
{"CRGB::YellowGreen", CRGB::YellowGreen, "#9ACD32", 0x9ACD32}
};

// For fauxmo (connecting to alexa)
#if ENABLE_ALEXA == true
	fauxmoESP fauxmo;
#endif

// Receive message for WebSerial
void recvMsg(uint8_t *data, size_t len){
  WebSerial.println("Received Data...");
  String d = "";
  for(int i=0; i < len; i++){
    d += char(data[i]);
  }
  WebSerial.println(d);
}

AsyncWebServer server(88); // Set this to 88 so that fauxmo can be 80

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
<title>SHedrick Shelf-Clock Settings</title>
<meta name="viewport" content="width=device-width, initial-scale=1">
<link rel="icon" href="data:,">
<style>
	html {font-family: Arial; display: inline-block; text-align: center;}
	h2 {font-size: 1.5rem;}
	h4 {font-size: 1.0rem; font-weight: bold;}
	p {font-size: 1.5rem;}
	body {max-width: 800px; margin:0px auto; padding-bottom: 25px;}
	.switch {position: relative; display: inline-block; width: 56px; height: 35px} 
	.switch input {display: none}
	.slider {position: absolute; top: 0; left: 0; right: 0; bottom: 0; background-color: #ccc; border-radius: 6px}
	.slider:before {position: absolute; content: ""; height: 20px; width: 20px; left: 8px; bottom: 8px; background-color: #fff; -webkit-transition: .4s; transition: .4s; border-radius: 3px}
	input:checked+.slider {background-color: #b30000}
	input:checked+.slider:before {-webkit-transform: translateX(20px); -ms-transform: translateX(20px); transform: translateX(20px)}
    .bslider { -webkit-appearance: none; margin: 14px; width: 360px; height: 25px; background: #FFD65C;
      outline: none; -webkit-transition: .2s; transition: opacity .2s;}
    .bslider::-webkit-slider-thumb {-webkit-appearance: none; appearance: none; width: 35px; height: 35px; background: #003249; cursor: pointer;}
    .bslider::-moz-range-thumb { width: 35px; height: 35px; background: #003249; cursor: pointer; } 
</style>
</head>
<body>
<h2>SDH Shelf-Clock Settings</h2>
%BUTTONPLACEHOLDER%
<script>function toggleCheckbox(element) {
var xhr = new XMLHttpRequest();
if(element.checked){ xhr.open("GET", "/update?button="+element.id+"&state=1", true); }
else { xhr.open("GET", "/update?button="+element.id+"&state=0", true); }
xhr.send();
}
function toggleColor(element) {
var xhr = new XMLHttpRequest();
var selval = element.value;
xhr.open("GET", "/update?button="+element.id+"&state="+selval, true);
xhr.send();
}
function restartController() {
var xhr = new XMLHttpRequest();
xhr.open("GET", "/update?button=RESTART&state=1", true);
xhr.send();
}
function updateSliderG(element) {
  var sValue = document.getElementById("gSlider").value;
  document.getElementById("gSliderText").innerHTML = sValue;
  var xhr = new XMLHttpRequest();
  xhr.open("GET", "/update?button=gSlider&state="+sValue, true);
  xhr.send();
}
function updateSliderC(element) {
  var sValue = document.getElementById("cSlider").value;
  document.getElementById("cSliderText").innerHTML = sValue;
  var xhr = new XMLHttpRequest();
  xhr.open("GET", "/update?button=cSlider&state="+sValue, true);
  xhr.send();
}
function updateSliderDL(element) {
  var sValue = document.getElementById("dlSlider").value;
  document.getElementById("dlSliderText").innerHTML = sValue;
  var xhr = new XMLHttpRequest();
  xhr.open("GET", "/update?button=dlSlider&state="+sValue, true);
  xhr.send();
}
</script>
</body>
</html>
)rawliteral";


// +++++++++++++++++++++++ SETUP +++++++++++++++++++++++++++++
void setup()
{
	Serial.begin(115200);

	// Read any EEPROM saved values.  NOTE:  255 is the value of an UNSET variable as of yet.
	EEPROM.begin(EEPROM_SIZE);
	initEEPROMOnStartup();
	
	ShelfDisplays->InitSegments(0, NUM_LEDS_PER_SEGMENT, WIFI_CONNECTING_COLOR, 50);

	ShelfDisplays->setHourSegmentColors(WIFI_CONNECTING_COLOR);
	ShelfDisplays->setMinuteSegmentColors(WIFI_CONNECTING_COLOR);
	//ShelfDisplays->setInternalLEDColor(WIFI_CONNECTING_COLOR);
	//ShelfDisplays->setDotLEDColor(WIFI_CONNECTING_COLOR);

	#if RUN_WITHOUT_WIFI == false
		wifiSetup();
	#endif

	#if ENABLE_OTA_UPLOAD == true
		setupOTA();
	#endif

	#if ENABLE_OTA_UPLOAD == true
		ArduinoOTA.handle(); //give ota the opportunity to update before the main loop starts in case we have a crash in there
	#endif

	#if ENABLE_ALEXA == true
		fauxmo.createServer(true);
		fauxmo.setPort(80);

		fauxmo.enable(false);
		fauxmo.enable(true);
		// 1, turn "shelf down lights" on.
		// 2, turn "shelf clock lights" on.
		// 3, turn "schlock" on/off.
		fauxmo.addDevice(ALEXA_LAMP_1);
		fauxmo.addDevice(ALEXA_LAMP_2);
		fauxmo.addDevice(ALEXA_LAMP_3);

		fauxmo.onSetState([](unsigned char device_id, const char * device_name, bool state, unsigned char value) {
			// Callback when a command from Alexa is received. 
			// You can use device_id or device_name to choose the element to perform an action onto (relay, LED,...)
			// State is a boolean (ON/OFF) and value a number from 0 to 255 (if you say "set kitchen light to 50%" you will receive a 128 here).
			// Just remember not to delay too much here, this is a callback, exit as soon as possible.
			// If you have to do something more involved here set a flag and process it in your main loop.
				
			Serial.printf("[ALEXA] Device #%d (%s) state: %s value: %d\n", device_id, device_name, state ? "ON" : "OFF", value);
			WebSerial.print("[ALEXA] Device "); WebSerial.print(device_id); WebSerial.print(" ("); WebSerial.print(device_name);
			WebSerial.print (") state: "); WebSerial.print(state ? "ON" : "OFF"); WebSerial.print (" value: "); WebSerial.println(value);
			if ( (strcmp(device_name, ALEXA_LAMP_1) == 0) ) {
				toggleDownlights(state ? 1 : 0, value);
			}
			if ( (strcmp(device_name, ALEXA_LAMP_2) == 0) ) {
				toggleClocklights(state ? 1 : 0, value);
			}
			if ( (strcmp(device_name, ALEXA_LAMP_3) == 0) ) {
				toggleSchlock(state ? 1 : 0, value);
			}
		});

	#endif

	Serial.println("Starting up ESP Async Web Server...");
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send_P(200, "text/html", index_html, webProcessor);
  	});

  	// Send a GET request to <ESP_IP>/update?output=<inputMessage1>&state=<inputMessage2>
  	server.on("/update", HTTP_GET, [] (AsyncWebServerRequest *request) {
    	String inputMessage1;
    	String inputMessage2;
		if (request->hasParam(PARAM_INPUT_1) && request->hasParam(PARAM_INPUT_2)) {
			inputMessage1 = request->getParam(PARAM_INPUT_1)->value();
			inputMessage2 = request->getParam(PARAM_INPUT_2)->value();
			if (inputMessage1 == "RESTART") {
				// Restarting controller
				ESP.restart();
			}
			if (inputMessage1 == String(EE_downlighterOnOffState)) {
				if (inputMessage2 == "0") {
					// Set downlights to black.  (turn off)
					toggleDownlights(0, 0);
				} else {
					// Set downlights back to their color.
					toggleDownlights(1, currentDLBrightnessLevel);
				}
			}
			if (inputMessage1 == String(EE_clockOnOffState)) {
				if (inputMessage2 == "0") {
					// Set clock to black.  (turn off)
					toggleClocklights(0, 0);
				} else {
					toggleClocklights(1, currentClockBrightnessLevel);
				}
			}
			if (inputMessage1 == String(EE_testMode)) {
				if (inputMessage2 == "0") {
					// Turn off test mode.
					ShelfDisplays->setHourSegmentColors(defaultHourColor);
					ShelfDisplays->setMinuteSegmentColors(defaultMinColor);
					ShelfDisplays->setGlobalBrightness(defaultGlobalBrightnessLevel);
					testMode = false;
					EEPROM.write(EE_testMode,0);
					EEPROM.commit();
				} else {
					// Turn on test mode.
					testMode = true;
					EEPROM.write(EE_testMode,1);
					EEPROM.commit();
				}
			}
			if (inputMessage1 == String(EE_defaultHourColor)) {
				// Set default color
				defaultHourColor = arr_crgbcolors[inputMessage2.toInt()].colorcrgb;
				ShelfDisplays->setHourSegmentColors(defaultHourColor);
				EEPROM.write(EE_defaultHourColor,inputMessage2.toInt());
				EEPROM.commit();
			}
			if (inputMessage1 == String(EE_defaultMinColor)) {
				// Set default color
				defaultMinColor = arr_crgbcolors[inputMessage2.toInt()].colorcrgb;
				ShelfDisplays->setMinuteSegmentColors(defaultMinColor);
				EEPROM.write(EE_defaultMinColor,inputMessage2.toInt());
				EEPROM.commit();
			}
			if (inputMessage1 == String(EE_defaultDLColor)) {
				// Set default color
				defaultDLColor = arr_crgbcolors[inputMessage2.toInt()].colorcrgb;
				ShelfDisplays->setInternalLEDColor(defaultDLColor);
				EEPROM.write(EE_defaultDLColor,inputMessage2.toInt());
				EEPROM.commit();
			}
			if (inputMessage1 == "gSlider") {
				// Update Global Brightness
				defaultGlobalBrightnessLevel = inputMessage2.toInt();
				ShelfDisplays->setGlobalBrightness(defaultGlobalBrightnessLevel);
				EEPROM.write(EE_defaultGlobalBrightnessLevel,inputMessage2.toInt());
				EEPROM.commit();
			}
			if (inputMessage1 == "cSlider") {
				// Update Clock Brightness
				if (inputMessage2.toInt() == 0)
					toggleClocklights(0, 0);
				else
					toggleClocklights(1, inputMessage2.toInt());
			}
			if (inputMessage1 == "dlSlider") {
				// Update DL Brightness
				if (inputMessage2.toInt() == 0)
					toggleDownlights(0, 0);
				else
					toggleDownlights(1, inputMessage2.toInt());
			}
		} else {
			inputMessage1 = "No message sent";
			inputMessage2 = "No message sent"; 
		}
		Serial.print("Button: ");
		Serial.print(inputMessage1);
		Serial.print(" - Set to: ");
		Serial.println(inputMessage2);
		WebSerial.print("Button: ");
		WebSerial.print(inputMessage1);
		WebSerial.print(" - Set to: ");
		WebSerial.println(inputMessage2);
		request->send(200, "text/plain", "OK");
	});

	// WebSerial is accessible at "<IP Address>/webserial" in browser
	WebSerial.begin(&server);
	WebSerial.msgCallback(recvMsg);

	// Start Web server
	server.begin();
	delay(10000);

	Serial.println("Fetching time from NTP server...");
	WebSerial.println("Fetching time from NTP server...");
	if(timeM->init() == false)
	{
		Serial.printf("[E]: TimeManager failed to synchronize for the first time with the NTP server. Retrying in %d seconds", TIME_SYNC_INTERVAL);
		WebSerial.print("[E]: TimeManager failed to synchronize for the first time with the NTP server. Retrying in ");
		WebSerial.print(TIME_SYNC_INTERVAL);
		WebSerial.println(" seconds");
	}
	timeM->setTimerTickCallback(TimerTick);
	timeM->setTimerDoneCallback(TimerDone);
	timeM->setAlarmCallback(AlarmTriggered);

	// Output EEPROM Values
	outputEEPROMValues();

	bool ranTestModeOnStartup = false;
	if (TEST_MODE_ON_STARTUP == true && !testMode && !ranTestModeOnStartup) {
		bool ranTestModeOnStartup = true;
		runTestModeOnStartup();
	} else {
		if (clockOnOffState) {
			ShelfDisplays->setHourSegmentColors(defaultHourColor);
			ShelfDisplays->setMinuteSegmentColors(defaultMinColor);
		}
		if (downlightersOnOffState) {
			ShelfDisplays->setInternalLEDColor(defaultDLColor);
		}
	}

	if (!testMode) {
		Serial.println("Displaying startup animation...");
		WebSerial.println("Displaying startup animation...");
		startupAnimation();
	}

	Serial.println("Setup done. Main Loop starting...");
	WebSerial.println("Setup done. Main Loop starting...");
}

// +++++++++++++++++++++++ MAIN LOOP +++++++++++++++++++++++++++++
int lasttest = millis();
int lastmem = millis();
void loop()
{
	/*if ((millis()-lastmem)>=10000) {
		outputESPMemory();
		lastmem = millis();
	}*/
	#if ENABLE_OTA_UPLOAD == true
		ArduinoOTA.handle();
	#endif
	#if ENABLE_ALEXA == true
		fauxmo.handle();
	#endif
	if (!testMode) {
		states->handleStates(); //updates display states, switches between modes etc.
	}

	if (testMode) {
		// Test code:
		ShelfDisplays->setSegmentColor(0, CRGB::Blue);
		ShelfDisplays->setSegmentColor(1, CRGB::Red);
		ShelfDisplays->setSegmentColor(2, CRGB::Green);
		ShelfDisplays->setSegmentColor(3, CRGB::Purple);
		ShelfDisplays->setGlobalBrightness(defaultGlobalBrightnessLevel);

		if((millis()-lasttest)>= 1500)
		{
			ShelfDisplays->test();
			lasttest = millis();
		}
	}

    ShelfDisplays->handle();
}

void outputESPMemory(){
	Serial.println("[DEBUG] Free Heap: " + String(ESP.getFreeHeap()));
	WebSerial.println("[DEBUG] Free Heap: " + String(ESP.getFreeHeap()));
}

// Clamp RGB to a percentage from 0 to 255.  This will be my attempt to dim/brighten colors.  
// Basically, clamping down to say 5 and clamping up to 255 (but keeping other rgb the same)
// When going down, after you get to the "bottom" darkest, then 0/0/0 (black/off) is what happens next.
// So examples:
// For brightening:
// RGB = 255, 0, 0: because R is already clamped at 255, its as bright as it can go.
// RGB = 200, 210, 30:  Set to 80% brightness.  We need to first scale rgb up to max brightness: 245, 255, 75.  
// If we say to darken, then we do the same basic thing.  Go to brightest and multiply by 0-1.0.  
CRGB clamp_rgb(CRGB color, int scale) {
	int r = color.r, g = color.g, b = color.b;
	int r1, g1, b1;
	CRGB returncolor;

	// get a number from 0.0 to 1.0 that is equivalent to the integer passed in of 0 to 255
	// NOTE:  This is a scale down number.
	double scaledownfactor = (double)scale / 255.0;
	WebSerial.println("Scale Down Factor: " + String(scaledownfactor));

	// Scale the existing color up to full brightness first.
	double scaleupfactor = min(255.0/(double)r, min(255.0/(double)g, 255.0/(double)b));
	WebSerial.println("Scale Up Factor (step 1): " + String(scaleupfactor));


	// Now, to get final number, just multiply by scaleup and scaledown
	r1 = r * scaleupfactor * scaledownfactor;
	g1 = g * scaleupfactor * scaledownfactor;
	b1 = b * scaleupfactor * scaledownfactor;

	returncolor.r = r1;
	returncolor.g = g1;
	returncolor.b = b1;

	WebSerial.println("r: " + String(r) + ", g: " + String(g) + ", b: " + String(b));
	WebSerial.println("r1: " + String(r1) + ", g1: " + String(g1) + ", b1: " + String(b1));

	return returncolor;
}

void runTestModeOnStartup() {
	ShelfDisplays->turnAllLEDsOff();
	delay(500);
	ShelfDisplays->setSegmentColor(0, CRGB::Blue);
	delay(500);
	ShelfDisplays->setSegmentColor(1, CRGB::Red);
	delay(500);
	ShelfDisplays->setSegmentColor(2, CRGB::Green);
	delay(500);
	ShelfDisplays->setSegmentColor(3, CRGB::Purple);
	delay(500);
	ShelfDisplays->setInternalLEDColor(CRGB::DarkOrange);
	delay(500);
	ShelfDisplays->setGlobalBrightness(defaultGlobalBrightnessLevel);
	delay(500);
	for (uint8_t i = 0; i <= 9; i++) {
		ShelfDisplays->testOnStartup(i);
		ShelfDisplays->handle();
		ShelfDisplays->delay(1000);
	}
	if (clockOnOffState) {
		ShelfDisplays->setHourSegmentColors(defaultHourColor);
		ShelfDisplays->setMinuteSegmentColors(defaultMinColor);
		toggleClocklights(1, currentClockBrightnessLevel);
	} else {
		ShelfDisplays->setHourSegmentColors(CRGB::Black);
		ShelfDisplays->setMinuteSegmentColors(CRGB::Black);
	}
	if (downlightersOnOffState) {
		ShelfDisplays->setInternalLEDColor(defaultDLColor);
		toggleDownlights(1, currentDLBrightnessLevel);
	} else {
		ShelfDisplays->setInternalLEDColor(CRGB::Black);
	}
}

// Used by web
String webOnOffButtonState(int button){
	switch(button) {
		case EE_downlighterOnOffState:
			if(downlightersOnOffState){return "checked";} else {return "";}
			break;
		case EE_clockOnOffState:
			if(clockOnOffState){return "checked";} else {return "";}
			break;
		case EE_testMode:
			if(testMode){return "checked";} else {return "";}
			break;
	}
    return "";
}

// Replaces placeholder with button section in your web page
String webProcessor(const String& var){
    //Serial.println(var);
    if(var == "BUTTONPLACEHOLDER"){
        String buttons = "";
        buttons += "<label class=\"button\"><input type=\"button\" onclick=\"restartController()\" id=\"RESTART\" value=\"Restart Controller\"></label>\n";
        buttons += "<h4>Turn Off/On Downlighters</h4><label class=\"switch\"><input type=\"checkbox\" onchange=\"toggleCheckbox(this)\" id=\"" + String(EE_downlighterOnOffState) + "\" " + webOnOffButtonState(EE_downlighterOnOffState) + "><span class=\"slider\"></span></label>\n";
        buttons += "<h4>Turn Off/On Clock</h4><label class=\"switch\"><input type=\"checkbox\" onchange=\"toggleCheckbox(this)\" id=\"" + String(EE_clockOnOffState) + "\" " + webOnOffButtonState(EE_clockOnOffState) + "><span class=\"slider\"></span></label>\n";
        buttons += "<h4>Test Mode Off/On</h4><label class=\"switch\"><input type=\"checkbox\" onchange=\"toggleCheckbox(this)\" id=\"" + String(EE_testMode) + "\" " + webOnOffButtonState(EE_testMode) + "><span class=\"slider\"></span></label>\n";
		// Select for Hours Color:
		String hourSelect = "<h4>Hour Digits Color</h4>\n<select name=\"hourColor\" id=\"" + String(EE_defaultHourColor) + "\" onchange=\"toggleColor(this)\">\n";
		for (int i = 0; i < NUM_COLORS; i++) {
			String colorName = arr_crgbcolors[i].colornamecrgb;
			String bgcolor = arr_crgbcolors[i].colorvaluenamehex;
			// Compare the color in EEPROM with the value of this option and if they match, we mark "selected"
			if (arr_crgbcolors[i].colorcrgb == arr_crgbcolors[EEPROM.read(EE_defaultHourColor)].colorcrgb) {
				hourSelect += "<option value=\"" + String(i) + "\" style=\"background-color: " + bgcolor + "\" selected>" + colorName + "</option>\n";
			} else {
				hourSelect += "<option value=\"" + String(i) + "\" style=\"background-color: " + bgcolor + "\">" + colorName + "</option>\n";
			}
		}
		hourSelect += "</select>\n";

		buttons += hourSelect;

		// Select for Min Color:
		String minSelect = "<h4>Minute Digits Color</h4>\n<select name=\"minColor\" id=\"" + String(EE_defaultMinColor) + "\" onchange=\"toggleColor(this)\">\n";
		for (int i = 0; i < NUM_COLORS; i++) {
			String colorName = arr_crgbcolors[i].colornamecrgb;
			String bgcolor = arr_crgbcolors[i].colorvaluenamehex;
			// Compare the color in EEPROM with the value of this option and if they match, we mark "selected"
			if (arr_crgbcolors[i].colorcrgb == arr_crgbcolors[EEPROM.read(EE_defaultMinColor)].colorcrgb) {
				minSelect += "<option value=\"" + String(i) + "\" style=\"background-color: " + bgcolor + "\" selected>" + colorName + "</option>\n";
			} else {
				minSelect += "<option value=\"" + String(i) + "\" style=\"background-color: " + bgcolor + "\">" + colorName + "</option>\n";
			}
		}
		minSelect += "</select>\n";

		buttons += minSelect;

		// Select for DL Color:
		String dlSelect = "<h4>Downlights Color</h4>\n<select name=\"dlColor\" id=\"" + String(EE_defaultDLColor) + "\" onchange=\"toggleColor(this)\">\n";
		for (int i = 0; i < NUM_COLORS; i++) {
			String colorName = arr_crgbcolors[i].colornamecrgb;
			String bgcolor = arr_crgbcolors[i].colorvaluenamehex;
			// Compare the color in EEPROM with the value of this option and if they match, we mark "selected"
			if (arr_crgbcolors[i].colorcrgb == arr_crgbcolors[EEPROM.read(EE_defaultDLColor)].colorcrgb) {
				dlSelect += "<option value=\"" + String(i) + "\" style=\"background-color: " + bgcolor + "\" selected>" + colorName + "</option>\n";
			} else {
				dlSelect += "<option value=\"" + String(i) + "\" style=\"background-color: " + bgcolor + "\">" + colorName + "</option>\n";
			}
		}
		dlSelect += "</select>\n";

		buttons += dlSelect;

		// Sliders for Brightness
		String bsliders = "<h4>Global Brightness Level</h4>\n<span id=\"gSliderText\">" + String(defaultGlobalBrightnessLevel) + "</span>\n";
  		bsliders += "<p><input type=\"range\" onchange=\"updateSliderG(this)\" id=\"gSlider\" min=\"0\" max=\"255\" value=\"" + String(defaultGlobalBrightnessLevel) + "\" step=\"1\" class=\"bslider\"></p>\n";
		bsliders += "<h4>Clock Brightness Level</h4>\n<span id=\"cSliderText\">" + String(currentClockBrightnessLevel) + "</span>\n";
  		bsliders += "<p><input type=\"range\" onchange=\"updateSliderC(this)\" id=\"cSlider\" min=\"0\" max=\"255\" value=\"" + String(currentClockBrightnessLevel) + "\" step=\"1\" class=\"bslider\"></p>\n";
		bsliders += "<h4>Down Lights Brightness Level</h4>\n<span id=\"dlSliderText\">" + String(currentDLBrightnessLevel) + "</span>\n";
  		bsliders += "<p><input type=\"range\" onchange=\"updateSliderDL(this)\" id=\"dlSlider\" min=\"0\" max=\"255\" value=\"" + String(currentDLBrightnessLevel) + "\" step=\"1\" class=\"bslider\"></p>\n";

		buttons += bsliders;

        return buttons;
    }
    return String();
}

void initEEPROMOnStartup() {
	Serial.println("Reading EEPROM...");

	// Is downlighter supposed to be on or off
	int EE_value = EEPROM.read(EE_downlighterOnOffState);
	Serial.print("EE_downlighterOnOffState: ");	Serial.println(EE_value);
	if (EE_value != 255) {
		downlightersOnOffState = (EE_value == 0) ? false : true;
	}

	// Is clock supposed to be on or off
	EE_value = EEPROM.read(EE_clockOnOffState);
	Serial.print("EE_clockOnOffState: "); Serial.println(EE_value);
	if (EE_value != 255) {
		clockOnOffState = (EE_value == 0) ? false : true;
	}

	// Is testMode to be engaged
	EE_value = EEPROM.read(EE_testMode);
	Serial.print("EE_testMode: "); Serial.println(EE_value);
	testMode = (EE_value == 0 || EE_value == 255) ? false : true;

	// Default Global Brightness
	EE_value = EEPROM.read(EE_defaultGlobalBrightnessLevel);
	Serial.print("EE_defaultGlobalBrightnessLevel: "); Serial.println(EE_value);
	defaultGlobalBrightnessLevel = EE_value;

	// Hour Digits Color
	EE_value = EEPROM.read(EE_defaultHourColor);
	Serial.print("EE_defaultHourColor: "); Serial.println(EE_value);
	defaultHourColor = arr_crgbcolors[EE_value].colorcrgb;

	// Min Digits Color
	EE_value = EEPROM.read(EE_defaultMinColor);
	Serial.print("EE_defaultMinColor: "); Serial.println(EE_value);
	defaultMinColor = arr_crgbcolors[EE_value].colorcrgb;

	// DL Color
	EE_value = EEPROM.read(EE_defaultDLColor);
	Serial.print("EE_defaultDLColor: "); Serial.println(EE_value);
	defaultDLColor = arr_crgbcolors[EE_value].colorcrgb;

	// Clock Brightness
	EE_value = EEPROM.read(EE_currentClockBrightnessLevel);
	Serial.print("EE_currentClockBrightnessLevel: "); Serial.println(EE_value);
	currentClockBrightnessLevel=EE_value;

	// DL Brightness
	EE_value = EEPROM.read(EE_currentDLBrightnessLevel);
	Serial.print("EE_currentDLBrightnessLevel: "); Serial.println(EE_value);
	currentDLBrightnessLevel=EE_value;
}

void outputEEPROMValues() {
	Serial.println("Reading EEPROM...");
	WebSerial.println("Reading EEPROM...");

	int EE_value = EEPROM.read(EE_downlighterOnOffState);
	Serial.print("EE_downlighterOnOffState: ");	Serial.println(EE_value);
	WebSerial.print("EE_downlighterOnOffState: ");	WebSerial.println(EE_value);

	EE_value = EEPROM.read(EE_clockOnOffState);
	Serial.print("EE_clockOnOffState: "); Serial.println(EE_value);
	WebSerial.print("EE_clockOnOffState: "); WebSerial.println(EE_value);

	EE_value = EEPROM.read(EE_testMode);
	Serial.print("EE_testMode: "); Serial.println(EE_value);
	WebSerial.print("EE_testMode: "); WebSerial.println(EE_value);

	EE_value = EEPROM.read(EE_defaultGlobalBrightnessLevel);
	Serial.print("EE_defaultGlobalBrightnessLevel: "); Serial.println(EE_value);
	WebSerial.print("EE_defaultGlobalBrightnessLevel: "); WebSerial.println(EE_value);

	EE_value = EEPROM.read(EE_defaultHourColor);
	Serial.print("EE_defaultHourColor: "); Serial.println(EE_value);
	WebSerial.print("EE_defaultHourColor: "); WebSerial.println(EE_value);

	EE_value = EEPROM.read(EE_defaultMinColor);
	Serial.print("EE_defaultMinColor: "); Serial.println(EE_value);
	WebSerial.print("EE_defaultMinColor: "); WebSerial.println(EE_value);

	EE_value = EEPROM.read(EE_defaultDLColor);
	Serial.print("EE_defaultDLColor: "); Serial.println(EE_value);
	WebSerial.print("EE_defaultDLColor: "); WebSerial.println(EE_value);

	EE_value = EEPROM.read(EE_currentClockBrightnessLevel);
	Serial.print("EE_currentClockBrightnessLevel: "); Serial.println(EE_value);
	WebSerial.print("EE_currentClockBrightnessLevel: "); WebSerial.println(EE_value);

	EE_value = EEPROM.read(EE_currentDLBrightnessLevel);
	Serial.print("EE_currentDLBrightnessLevel: "); Serial.println(EE_value);
	WebSerial.print("EE_currentDLBrightnessLevel: "); WebSerial.println(EE_value);
}

void toggleDownlights(int state, int brightness) {
	// state is 1 = on and 0 = off.  
	// value is 0-255 for brightness
	switch(state) {
		case 0:
			ShelfDisplays->setInternalLEDColor(CRGB::Black);
			downlightersOnOffState = false;
			break;
		case 1:
			ShelfDisplays->setInternalLEDColor(defaultDLColor);
			downlightersOnOffState = true;
			if (brightness >= 1) {
				//ShelfDisplays->setGlobalBrightness(brightness, true);
				defaultDLColor = clamp_rgb(defaultDLColor, brightness);
				ShelfDisplays->setInternalLEDColor(defaultDLColor);
				currentDLBrightnessLevel = brightness;
				EEPROM.write(EE_currentDLBrightnessLevel, currentDLBrightnessLevel);
				EEPROM.commit();
			}
			break;
	}
	EEPROM.write(EE_downlighterOnOffState,state);
	EEPROM.commit();
	outputEEPROMValues();
}

void toggleClocklights(int state, int brightness) {
	// state is 1 = on and 0 = off.  
	// value is 0-255 for brightness
	switch(state) {
		case 0:
			ShelfDisplays->setHourSegmentColors(CRGB::Black);
			ShelfDisplays->setMinuteSegmentColors(CRGB::Black);
			clockOnOffState = false;
			break;
		case 1:
			// Set clock back to their color.
			clockOnOffState = true;
			if (brightness >= 1) {
				//ShelfDisplays->setGlobalBrightness(brightness, true);
				defaultHourColor = clamp_rgb(defaultHourColor, brightness);
				defaultMinColor = clamp_rgb(defaultMinColor, brightness);
				ShelfDisplays->setHourSegmentColors(defaultHourColor);
				ShelfDisplays->setMinuteSegmentColors(defaultMinColor);
				currentClockBrightnessLevel = brightness;
				EEPROM.write(EE_currentClockBrightnessLevel,currentClockBrightnessLevel);
				EEPROM.commit();
			}
			break;
	}
	EEPROM.write(EE_clockOnOffState,state);
	EEPROM.commit();
	outputEEPROMValues();
}

void toggleSchlock(int state, int brightness) {
	// state is 1 = on and 0 = off.  
	// value is 0-255 for brightness
	switch(state) {
		case 0:
			ShelfDisplays->setHourSegmentColors(CRGB::Black);
			ShelfDisplays->setMinuteSegmentColors(CRGB::Black);
			ShelfDisplays->setInternalLEDColor(CRGB::Black);
			clockOnOffState = false;
			downlightersOnOffState = false;
			break;
		case 1:
			// Set all back to their color.
			clockOnOffState = true;
			downlightersOnOffState = true;
			if (brightness >= 1) {
				ShelfDisplays->setGlobalBrightness(brightness, true);
				EEPROM.write(EE_defaultGlobalBrightnessLevel,state);
				EEPROM.commit();
			}
			break;
	}
	EEPROM.write(EE_clockOnOffState,state);
	EEPROM.commit();
	EEPROM.write(EE_downlighterOnOffState,state);
	EEPROM.commit();
}

void startupAnimation()
{
	uint8_t currHourH = 0;
	uint8_t currHourL = 0;
	uint8_t currMinH = 0;
	uint8_t currMinL = 0;
	uint8_t targetHourH = 0;
	uint8_t targetHourL = 0;
	uint8_t targetMinH = 0;
	uint8_t targetMinL = 0;

	TimeManager::TimeInfo currentTime;
	currentTime = timeM->getCurrentTime();
	targetHourH = currentTime.hours / 10;
	targetHourL = currentTime.hours % 10;
	targetMinH = currentTime.minutes / 10;
	targetMinL = currentTime.minutes % 10;

	ShelfDisplays->displayTime(0, 0);
	ShelfDisplays->delay(DIGIT_ANIMATION_SPEED + 10);

	while (currHourH != targetHourH || currHourL != targetHourL || currMinH != targetMinH || currMinL != targetMinL)
	{
		if(currHourH < targetHourH)
		{
			currHourH++;
		}
		if(currHourL < targetHourL)
		{
			currHourL++;
		}
		if(currMinH < targetMinH)
		{
			currMinH++;
		}
		if(currMinL < targetMinL)
		{
			currMinL++;
		}
		ShelfDisplays->displayTime(currHourH * 10 + currHourL, currMinH * 10 + currMinL);
		ShelfDisplays->delay(DIGIT_ANIMATION_SPEED + 100);
	}
}

void AlarmTriggered()
{
    states->switchMode(ClockState::ALARM_NOTIFICATION);
}

void TimerTick(){
	// Not sure what this was for.
}

void TimerDone()
{
    states->switchMode(ClockState::TIMER_NOTIFICATION);
}

#if RUN_WITHOUT_WIFI == false
	void WiFiStationDisconnected(WiFiEvent_t event, WiFiEventInfo_t info)
	{
		Serial.print("WiFi lost connection. Reason: ");
		Serial.println(info.wifi_sta_disconnected.reason);
		Serial.println("Trying to Reconnect");
		WiFi.disconnect();
		WiFi.reconnect();
	}

	void printWifiData() {
		// print your WiFi shield's IP address:
		IPAddress ip = WiFi.localIP();
		Serial.print("IP Address: ");
		Serial.println(ip);

		// print your MAC address:
		byte mac[6];
		WiFi.macAddress(mac);
		Serial.print("MAC address: ");
		Serial.print(mac[5], HEX);
		Serial.print(":");
		Serial.print(mac[4], HEX);
		Serial.print(":");
		Serial.print(mac[3], HEX);
		Serial.print(":");
		Serial.print(mac[2], HEX);
		Serial.print(":");
		Serial.print(mac[1], HEX);
		Serial.print(":");
		Serial.println(mac[0], HEX);
	}

	void wifiSetup()
	{
		#if USE_ESPTOUCH_SMART_CONFIG == true
			WiFi.reconnect(); //try to reconnect
			Serial.println("Trying to reconnect to previous wifi network");
		#else
			WiFi.mode(WIFI_STA);
			WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE, INADDR_NONE);
			WiFi.setHostname(ESP_HOST_NAME);
			WiFi.begin(WIFI_SSID, WIFI_PW);
		#endif
		ShelfDisplays->setAllSegmentColors(WIFI_CONNECTING_COLOR);
		ShelfDisplays->showLoadingAnimation();
		for (int i = 0; i < NUM_RETRIES; i++)
		{
			Serial.print(".");
			#if USE_ESPTOUCH_SMART_CONFIG == true
				if(WiFi.begin() == WL_CONNECTED)
			#else
				if(WiFi.status() == WL_CONNECTED)
			#endif
			{
				Serial.println("Reconnect successful");
				ShelfDisplays->setAllSegmentColors(WIFI_CONNECTION_SUCCESSFUL_COLOR);
				break;
			}
			ShelfDisplays->delay(1000);
		}

		if(WiFi.status() != WL_CONNECTED)
		{
			#if USE_ESPTOUCH_SMART_CONFIG == true
				Serial.println("Reconnect failed. starting smart config");
				WiFi.mode(WIFI_AP_STA);
				// start SmartConfig
				WiFi.beginSmartConfig();

				// Wait for SmartConfig packet from mobile
				Serial.println("Waiting for SmartConfig.");
				ShelfDisplays->setAllSegmentColors(WIFI_SMART_CONFIG_COLOR);
				while (!WiFi.smartConfigDone())
				{
					Serial.print(".");
					ShelfDisplays->delay(500);
				}
				ShelfDisplays->setAllSegmentColors(WIFI_CONNECTING_COLOR);
				Serial.println("");
				Serial.println("SmartConfig done.");

				// Wait for WiFi to connect to AP
				Serial.println("Waiting for WiFi");
				while (WiFi.status() != WL_CONNECTED)
				{
					Serial.print(".");
					ShelfDisplays->setAllSegmentColors(WIFI_CONNECTION_SUCCESSFUL_COLOR);
					ShelfDisplays->delay(500);
				}
				Serial.println("WiFi Connected.");
				Serial.print("IP Address: ");
				Serial.println(WiFi.localIP());
			#else
				Serial.println("WIFI connection failed");
				ShelfDisplays->setAllSegmentColors(ERROR_COLOR);
			#endif
			if(WiFi.status() != WL_CONNECTED)
			{
				Serial.println("WIFI connection failed. Aborting execution.");
				abort();
			}
		}
		// Register mDNS (so you can access via hostname.local instead of knowing IP address)
		if(!MDNS.begin(ESP_HOST_NAME)) {
			Serial.println("Error starting mDNS");
		} else {
			Serial.printf("Successfully set mDNS to hostname: %s\n", ESP_HOST_NAME);
		}
		WiFi.onEvent(WiFiStationDisconnected, ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
		ShelfDisplays->stopLoadingAnimation();
		Serial.println("Waiting for loading animation to finish...");
		ShelfDisplays->waitForLoadingAnimationFinish();
		ShelfDisplays->turnAllSegmentsOff();
		printWifiData();
	}
#endif

#if ENABLE_OTA_UPLOAD == true
	bool progressFirstStep = true;
	void setupOTA()
	{
		// Port defaults to 3232
		// ArduinoOTA.setPort(3232);

		// Hostname defaults to esp3232-[MAC]
		ArduinoOTA.setHostname(OTA_UPDATE_HOST_NAME);

		// No authentication by default
		//ArduinoOTA.setPassword("admin");

		// Password can be set with it's md5 value as well
		// MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
		// ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

		ArduinoOTA.onStart([]() {
			String type;
			if (ArduinoOTA.getCommand() == U_FLASH)
			{
				type = "sketch";
			}
			else // U_SPIFFS
			{
				type = "filesystem";
			}
			// NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
			Serial.println("Start updating " + type);
			timeM->disableTimer();
			ShelfDisplays->setAllSegmentColors(OTA_UPDATE_COLOR);
			ShelfDisplays->turnAllLEDsOff(); //instead of the loading animation show a progress bar
			ShelfDisplays->setGlobalBrightness(50);
		})
		.onEnd([]()
		{
			Serial.println("\nOTA End");
		})
		.onProgress([](unsigned int progress, unsigned int total)
		{
			Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
			if(progressFirstStep == true)
			{
				ShelfDisplays->displayProgress(total);
				progressFirstStep = false;
			}
			ShelfDisplays->updateProgress(progress);
		})
		.onError([](ota_error_t error)
		{
			Serial.printf("Error[%u]: ", error);
			if (error == OTA_AUTH_ERROR)
			{
				Serial.println("Auth Failed");
			}
			else if (error == OTA_BEGIN_ERROR)
			{
				Serial.println("Begin Failed");
			}
			else if (error == OTA_CONNECT_ERROR)
			{
				Serial.println("Connect Failed");
			}
			else if (error == OTA_RECEIVE_ERROR)
			{
				Serial.println("Receive Failed");
			}
			else if (error == OTA_END_ERROR)
			{
				Serial.println("End Failed");
			}
			ShelfDisplays->setAllSegmentColors(ERROR_COLOR);
		});

		ArduinoOTA.begin();

		Serial.println("OTA update functionality is ready");
		Serial.print("IP address: ");
		Serial.println(WiFi.localIP());
	}
#endif
