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
#include <WebSerial.h> // for sending debug to web.
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include <LittleFS.h> // gonna use for file storage and reading/writing settings instead of eeprom.  Will use JSON for settings.
#include "ArduinoJson.h" // reading/writing json

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
//void TimerTick();
//void TimerDone();
//void AlarmTriggered();
void startupAnimation();
String webOnOffButtonState(String);
String webProcessor(const String&);
void toggleDownlights(int, int);
void toggleClocklights(int, int);
void toggleSchlock(int, int);
void runTestModeOnStartup();
CRGB clamp_rgb(CRGB, int);
void outputESPMemory();
void initializeAndReadConfig();
void outputConfigFile();
void readConfigJson();
void wipeAndReinitialize();
void updateSetting(String, String);

// Web Server Variables
// On/Off button states
const char* PARAM_INPUT_1 = "button";
const char* PARAM_INPUT_2 = "state";

// Default Web values
bool downlightersOnOffState = DEF_DOWNLIGHTERSONOFFSTATE;
bool clockOnOffState = DEF_CLOCKONOFFSTATE;
bool testMode = TEST_MODE;
int defaultGlobalBrightnessLevel = DEFAULT_CLOCK_BRIGHTNESS;
int currentClockBrightnessLevel = DEFAULT_CLOCK_BRIGHTNESS;
int currentDLBrightnessLevel = DEFAULT_CLOCK_BRIGHTNESS;
CRGB defaultHourColor = HOUR_COLOR;
int defaultHourColorIndex = 0;
CRGB defaultMinColor = MINUTE_COLOR;
int defaultMinColorIndex = 0;
CRGB defaultDLColor = INTERNAL_COLOR;
int defaultDLColorIndex = 0;
const char* ESPHostName = ESP_HOST_NAME;


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
function reinitSettings() {
var xhr = new XMLHttpRequest();
xhr.open("GET", "/update?button=REINITSETTINGS&state=1", true);
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

// For saving settings into littlefs
const char *settingfile = "/clockconfig.json"; // main clock config
//DynamicJsonDocument doc(1024); // doc will be our in memory json
int jsoncapacity = 1024;

// +++++++++++++++++++++++ SETUP +++++++++++++++++++++++++++++
void setup()
{
	Serial.begin(115200);
	delay(5000);
	WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);  // disable brownout detector

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
				//ESP.reset();
			}
			if (inputMessage1 == "REINITSETTINGS") {
				// Run the code to re-initialize the JSON settings file
				wipeAndReinitialize();
			}
			if (inputMessage1 == "DLOnOffState") {
				if (inputMessage2 == "0") {
					// Set downlights to black.  (turn off)
					toggleDownlights(0, 0);
				} else {
					// Set downlights back to their color.
					toggleDownlights(1, currentDLBrightnessLevel);
				}
			}
			if (inputMessage1 == "ClockOnOffState") {
				if (inputMessage2 == "0") {
					// Set clock to black.  (turn off)
					toggleClocklights(0, 0);
				} else {
					toggleClocklights(1, currentClockBrightnessLevel);
				}
			}
			if (inputMessage1 == "TestMode") {
				if (inputMessage2 == "0") {
					// Turn off test mode.
					ShelfDisplays->setHourSegmentColors(defaultHourColor);
					ShelfDisplays->setMinuteSegmentColors(defaultMinColor);
					ShelfDisplays->setGlobalBrightness(defaultGlobalBrightnessLevel);
					testMode = false;
					updateSetting("TestMode", "Off");
				} else {
					// Turn on test mode.
					testMode = true;
					updateSetting("TestMode", "On");
				}
			}
			if (inputMessage1 == "HCol") {
				// Set default color
				defaultHourColorIndex = inputMessage2.toInt();
				defaultHourColor = arr_crgbcolors[defaultHourColorIndex].colorcrgb;
				ShelfDisplays->setHourSegmentColors(defaultHourColor);
				updateSetting("HCol", inputMessage2);
			}
			if (inputMessage1 == "MCol") {
				// Set default color
				defaultMinColorIndex = inputMessage2.toInt();
				defaultMinColor = arr_crgbcolors[defaultMinColorIndex].colorcrgb;
				ShelfDisplays->setMinuteSegmentColors(defaultMinColor);
				updateSetting("MCol", inputMessage2);
			}
			if (inputMessage1 == "DLCol") {
				// Set default color
				defaultDLColorIndex = inputMessage2.toInt();
				defaultDLColor = arr_crgbcolors[defaultDLColorIndex].colorcrgb;
				ShelfDisplays->setInternalLEDColor(defaultDLColor);
				updateSetting("DLCol", inputMessage2);
			}
			if (inputMessage1 == "gSlider") {
				// Update Global Brightness
				defaultGlobalBrightnessLevel = inputMessage2.toInt();
				ShelfDisplays->setGlobalBrightness(defaultGlobalBrightnessLevel);
				updateSetting("GlobalBrightness", inputMessage2);
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
	delay(5000);

	Serial.println("Fetching time from NTP server...");
	WebSerial.println("Fetching time from NTP server...");
	if(timeM->init() == false)
	{
		Serial.printf("[E]: TimeManager failed to synchronize for the first time with the NTP server. Retrying in %d seconds", TIME_SYNC_INTERVAL);
		WebSerial.print("[E]: TimeManager failed to synchronize for the first time with the NTP server. Retrying in ");
		WebSerial.print(TIME_SYNC_INTERVAL);
		WebSerial.println(" seconds");
	}
	// I have disabled alot of code revolving around the Timer and Alarm.  Disabling setting up these callback functions.
	//timeM->setTimerTickCallback(TimerTick);
	//timeM->setTimerDoneCallback(TimerDone);
	//timeM->setAlarmCallback(AlarmTriggered);

	// Initialize our Config File
	initializeAndReadConfig();

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
int lastfstest = millis();
void loop()
{
	/*if ((millis()-lastmem)>=10000) {
		outputESPMemory();
		lastmem = millis();
	}*/
	//if ((millis()-lastfstest)>=10000) {
		//readConfigFile();
		//lastfstest = millis();
	//}
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

// Initialize our settings file.
// This will check if the file exists.  If not, we'll create it and save our default settings into it (from configuration.h)
// If file does exist, we have to check if the json is there.  If not, we will wipe the contents and re-write from configuration.h.
// If the file exists and the json is readable, then we read the json and set our default values.
void initializeAndReadConfig() {
	bool bWipeAndReinitialize = false;
	LittleFS.begin();

	if (!LittleFS.exists(settingfile)) {
		// File does not yet exist, so we will initialize with default settings from the configuration.h file
		Serial.println("Setup:  Setting File does not yet exist, creating file...");
		WebSerial.println("Setup:  Setting File does not yet exist, creating file...");
		bWipeAndReinitialize = true;

		File configfile = LittleFS.open(settingfile, "w+");

		if (!configfile) {
			Serial.print("Could not open file to write: "); Serial.println(settingfile);
			WebSerial.print("Could not open file to write: "); WebSerial.println(settingfile);
		} else {
			// File does not exist.  Wipe and write initial data to file.  
			configfile.println("This is my first time writing to littlefs...");
			configfile.close();
		}

	} else {
		// File exists
		Serial.println("Setup:  Setting File exists!  Attempting to read contents and values...");
		WebSerial.println("Setup:  Setting File exists!  Attempting to read contents and values...");
		// Try to read JSON.  If we cannot read our JSON data then we will delete contents and re-write the json data based on configuration.h values.
		// File does not exist.  Wipe and write initial data to file.  
		File configfile = LittleFS.open(settingfile, "r");
		//auto filecontents = configfile.readString();
		DynamicJsonDocument doc(jsoncapacity);
		DeserializationError error = deserializeJson(doc, configfile);
		configfile.close();
		if (error) {
			Serial.print("deserializeJson() failed: "); Serial.println(error.f_str());
			WebSerial.print("deserializeJson() failed: "); WebSerial.println(error.f_str());
			bWipeAndReinitialize = true;
		} else {
			Serial.println("deserializeJson() success!  Reading Values.");
			WebSerial.println("deserializeJson() success!  Reading Values.");
			// Try to read our ESP Host Name, if that is not successful, then we will wipe and reinitialize
			bool hasKey = doc.containsKey("ESPHostName");
			if (!hasKey) {
				bWipeAndReinitialize = true;
			}
		}
	}

	LittleFS.end();

	if (bWipeAndReinitialize) {
		// Either the config file does not exist or there was a problem reading our valid json.  We will reinitialize the file and then
		// re-read the json.
		wipeAndReinitialize();
	} else {
		// File and JSON seem ok, we will read in the entire json from the file
		readConfigJson();
	}
}

/*
	wipeAndReinitialize():  This function should be called only once (routinely) when the file hasn't been created.
	This sets the initial values in our settingfile in LittleFS based on the parameters in Configuration.h.
	Once this file is created, the values in this file will be used on startup of the device instead of the 
	values in Configuration.h.  

	There will be a button added to the webpage that allows us to "reset" all of these values manually to those in Configuration.h.
*/
void wipeAndReinitialize() {
	LittleFS.begin();

	Serial.println("wipeAndReinitialize():  ");
	WebSerial.println("wipeAndReinitialize():  ");

	// We will set all of our values from the config.h
	DynamicJsonDocument doc(jsoncapacity);
	doc["ESPHostName"] = ESP_HOST_NAME;
	doc["OTAHostName"] = OTA_UPDATE_HOST_NAME;
	doc["WifiSSID"] = WIFI_SSID;
	doc["WifiPW"] = WIFI_PW;
	doc["DLOn"] = DEF_DOWNLIGHTERSONOFFSTATE;
	doc["ClockOn"] = DEF_CLOCKONOFFSTATE;
	doc["HCol"] = defaultHourColorIndex;
	doc["MCol"] = defaultMinColorIndex;
	doc["DLCol"] = defaultDLColorIndex;
	doc["TZ"] = TIMEZONE_INFO;
	doc["GlobalBrightness"] = DEFAULT_CLOCK_BRIGHTNESS;
	doc["ClockBrightness"] = DEFAULT_CLOCK_BRIGHTNESS;
	doc["DLBrightness"] = DEFAULT_CLOCK_BRIGHTNESS;
	doc["TestMode"] = TEST_MODE;
	doc["TestModeOnStartup"] = TEST_MODE_ON_STARTUP;
	doc["MaxMilliAmps"] = MAX_MILLIAMPS;
	doc["AlexaOn"] = ENABLE_ALEXA;
	doc["Alexa1Name"] = ALEXA_LAMP_1;
	doc["Alexa2Name"] = ALEXA_LAMP_2;
	doc["Alexa3Name"] = ALEXA_LAMP_3;


	File configfile = LittleFS.open(settingfile, "w");
	if (!configfile) {
		Serial.println("wipeAndReinitialize:  Couldn't open file for write...");
		WebSerial.println("wipeAndReinitialize:  Couldn't open file for write...");
	} else {
		serializeJson(doc, configfile);
		configfile.println();
	}
	configfile.close();
	LittleFS.end();

	readConfigJson();
}

// readConfigJson():  This file will read all values from our setting file and apply them to our global variables
// for LED colors/etc.  This will be done on startup of the device only. (or if wipe is called).
void readConfigJson() {
	LittleFS.begin();

	DynamicJsonDocument doc(jsoncapacity);
	File configfile = LittleFS.open(settingfile, "r");

	if (!configfile) {
		Serial.println("readConfigJson:  Couldn't open our setting file...");
		WebSerial.println("readConfigJson:  Couldn't open our setting file...");
		configfile.close();
		LittleFS.end();
		return;
	} else {
		DeserializationError error = deserializeJson(doc, configfile);
		configfile.close();
		if (error) {
			Serial.print("readConfigJson:  Couldn't deserialize JSON:  "); Serial.println(error.f_str());
			WebSerial.println("readConfigJson:  Couldn't deserialize JSON:  "); WebSerial.println(error.f_str());
			LittleFS.end();
			return;
		}
	}
	// File is read and JSON deserialized.  Get values
	// For now just output.
	outputConfigFile();

	String settingName;
	String s = "";

	settingName = "ESPHostName";
	ESPHostName = doc[settingName].as<const char*>();
	s+="Setting: ";s+=settingName;s+=": ";s+=doc[settingName].as<const char*>();

	settingName = "DLOn";
	downlightersOnOffState = doc[settingName].as<bool>();
	s+="\nSetting: ";s+=settingName;s+=": ";s+=doc[settingName].as<bool>();

	settingName = "ClockOn";
	clockOnOffState = doc[settingName].as<bool>();
	s+="\nSetting: ";s+=settingName;s+=": ";s+=doc[settingName].as<bool>();

	settingName = "HCol";
	defaultHourColorIndex = doc[settingName].as<int>();
	defaultHourColor = arr_crgbcolors[defaultHourColorIndex].colorcrgb;
	s+="\nSetting: ";s+=settingName;s+=": ";s+=doc[settingName].as<int>();

	settingName = "MCol";
	defaultMinColorIndex = doc[settingName].as<int>();
	defaultMinColor = arr_crgbcolors[defaultMinColorIndex].colorcrgb;
	s+="\nSetting: ";s+=settingName;s+=": ";s+=doc[settingName].as<int>();

	settingName = "DLCol";
	defaultDLColorIndex = doc[settingName].as<int>();
	defaultDLColor = arr_crgbcolors[defaultDLColorIndex].colorcrgb;
	s+="\nSetting: ";s+=settingName;s+=": ";s+=doc[settingName].as<int>();

	settingName = "GlobalBrightness";
	defaultGlobalBrightnessLevel = doc[settingName].as<int>();
	s+="\nSetting: ";s+=settingName;s+=": ";s+=doc[settingName].as<int>();

	settingName = "ClockBrightness";
	currentClockBrightnessLevel = doc[settingName].as<int>();
	s+="\nSetting: ";s+=settingName;s+=": ";s+=doc[settingName].as<int>();

	settingName = "DLBrightness";
	currentDLBrightnessLevel = doc[settingName].as<int>();
	s+="\nSetting: ";s+=settingName;s+=": ";s+=doc[settingName].as<int>();

	Serial.println(s);
	WebSerial.println(s);

	/*
	settingName = "OTAHostName";
	settingName = "WifiSSID";
	settingName = "WifiPW";
	settingName = "TZ";
	settingName = "TestMode";
	settingName = "MaxMilliAmps";
	settingName = "AlexaOn";
	settingName = "Alexa1Name";
	settingName = "Alexa2Name";
	settingName = "Alexa3Name";
	*/
	LittleFS.end();
}

// updateSetting():  This is passed a single value to update in our JSON setting file.
// The only way I see how to do this is:
// 1. Read/Deserialize
// 2. Update the value in the JSON Doc
// 3. Serialize/Write the entire JSON doc back to the file.
// More efficient way to do it?  /shrug
void updateSetting(String settingName, String settingValue) {
	// Read and Deserialize
	LittleFS.begin();

	DynamicJsonDocument doc(jsoncapacity);
	File configfile = LittleFS.open(settingfile, "r");

	if (!configfile) {
		Serial.println("updateSetting:  Couldn't open our setting file...");
		WebSerial.println("updateSetting:  Couldn't open our setting file...");
		configfile.close();
		LittleFS.end();
		return;
	} else {
		DeserializationError error = deserializeJson(doc, configfile);
		configfile.close();
		if (error) {
			Serial.print("updateSetting:  Couldn't deserialize JSON:  "); Serial.println(error.f_str());
			WebSerial.println("updateSetting:  Couldn't deserialize JSON:  "); WebSerial.println(error.f_str());
			LittleFS.end();
			return;
		}
	}

	// Update the setting in the JSON Document
	// On/Off (checkbox) settings (On/Off = true/false)
	if (settingName == "DLOn" || settingName == "ClockOn" || settingName == "TestMode") {
		bool bValue;
		(settingValue == "On") ? bValue = true : bValue = false;
		doc[settingName] = bValue;
		Serial.print("updateSetting:  updating: "); Serial.print(settingName); Serial.print(", value: "); Serial.println(settingValue);
		WebSerial.print("updateSetting:  updating: "); WebSerial.print(settingName); WebSerial.print(", value: "); WebSerial.println(settingValue);
	}
	// Brightness Sliders (int 0 to 255)
	if (settingName == "GlobalBrightness" || settingName == "ClockBrightness" || settingName == "DLBrightness") {
		int iValue = settingValue.toInt();
		doc[settingName] = iValue;
		Serial.print("updateSetting:  updating: "); Serial.print(settingName); Serial.print(", value: "); Serial.println(settingValue);
		WebSerial.print("updateSetting:  updating: "); WebSerial.print(settingName); WebSerial.print(", value: "); WebSerial.println(settingValue);
	}
	// Color Values (int 0 to ?? and this is in the index inside our giant color array above)
	if (settingName == "MCol" || settingName == "HCol" || settingName == "DLCol") {
		int iValue = settingValue.toInt();
		doc[settingName] = iValue;
		Serial.print("updateSetting:  updating: "); Serial.print(settingName); Serial.print(", value: "); Serial.println(iValue);
		WebSerial.print("updateSetting:  updating: "); WebSerial.print(settingName); WebSerial.print(", value: "); WebSerial.println(iValue);
	}

	// Write the JSON Document back to the Settings file
	configfile = LittleFS.open(settingfile, "w");
	if (!configfile) {
		Serial.println("updateSetting:  Couldn't open file for write...");
		WebSerial.println("updateSetting:  Couldn't open file for write...");
	} else {
		serializeJson(doc, configfile);
		configfile.println();
	}
	configfile.close();

	LittleFS.end();
}

// outputConfigFile():  This is just for output/testing purposes.
void outputConfigFile() {
	LittleFS.begin();
	File file = LittleFS.open(settingfile, "r");
	auto filecontents = file.readString();
	file.close();
	Serial.println("outputConfigFile - reading setting file contents:");
	Serial.println(filecontents.c_str());
	WebSerial.println("outputConfigFile - reading setting file contents:");
	WebSerial.println(filecontents.c_str());
	LittleFS.end();
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
String webOnOffButtonState(String button){
	if (button == "DLOnOffState") {
		if(downlightersOnOffState){return "checked";} else {return "";}
	}
	if (button == "ClockOnOffState") {
		if(clockOnOffState){return "checked";} else {return "";}
	}
	if (button == "TestMode") {
		if(testMode){return "checked";} else {return "";}
	}
    return "";
}

// Replaces placeholder with button section in your web page
String webProcessor(const String& var){
    //Serial.println(var);
    if(var == "BUTTONPLACEHOLDER"){
        String buttons = "";
        buttons += "<label class=\"button\"><input type=\"button\" onclick=\"restartController()\" id=\"RESTART\" value=\"Restart Controller\"></label>\n";
        buttons += "<label class=\"button\"><input type=\"button\" onclick=\"reinitSettings()\" id=\"REINITSETTINGS\" value=\"Reinit Settings\"></label>\n";
        buttons += "<h4>Turn Off/On Downlighters</h4><label class=\"switch\"><input type=\"checkbox\" onchange=\"toggleCheckbox(this)\" id=\"DLOnOffState\" " + webOnOffButtonState("DLOnOffState") + "><span class=\"slider\"></span></label>\n";
        buttons += "<h4>Turn Off/On Clock</h4><label class=\"switch\"><input type=\"checkbox\" onchange=\"toggleCheckbox(this)\" id=\"ClockOnOffState\" " + webOnOffButtonState("ClockOnOffState") + "><span class=\"slider\"></span></label>\n";
        buttons += "<h4>Test Mode Off/On</h4><label class=\"switch\"><input type=\"checkbox\" onchange=\"toggleCheckbox(this)\" id=\"TestMode\" " + webOnOffButtonState("TestMode") + "><span class=\"slider\"></span></label>\n";
		// Select for Hours Color:
		String hourSelect = "<h4>Hour Digits Color</h4>\n<select name=\"hourColor\" id=\"HCol\" onchange=\"toggleColor(this)\">\n";
		for (int i = 0; i < NUM_COLORS; i++) {
			String colorName = arr_crgbcolors[i].colornamecrgb;
			String bgcolor = arr_crgbcolors[i].colorvaluenamehex;
			// Compare the current color with the value of this option and if they match, we mark "selected"
			if (i == defaultHourColorIndex) {
				hourSelect += "<option value=\"" + String(i) + "\" style=\"background-color: " + bgcolor + "\" selected>" + colorName + "</option>\n";
			} else {
				hourSelect += "<option value=\"" + String(i) + "\" style=\"background-color: " + bgcolor + "\">" + colorName + "</option>\n";
			}
		}
		hourSelect += "</select>\n";

		buttons += hourSelect;

		// Select for Min Color:
		String minSelect = "<h4>Minute Digits Color</h4>\n<select name=\"minColor\" id=\"MCol\" onchange=\"toggleColor(this)\">\n";
		for (int i = 0; i < NUM_COLORS; i++) {
			String colorName = arr_crgbcolors[i].colornamecrgb;
			String bgcolor = arr_crgbcolors[i].colorvaluenamehex;
			// Compare the current color with the value of this option and if they match, we mark "selected"
			if (i == defaultMinColorIndex) {
				minSelect += "<option value=\"" + String(i) + "\" style=\"background-color: " + bgcolor + "\" selected>" + colorName + "</option>\n";
			} else {
				minSelect += "<option value=\"" + String(i) + "\" style=\"background-color: " + bgcolor + "\">" + colorName + "</option>\n";
			}
		}
		minSelect += "</select>\n";

		buttons += minSelect;

		// Select for DL Color:
		String dlSelect = "<h4>Downlights Color</h4>\n<select name=\"dlColor\" id=\"DLCol\" onchange=\"toggleColor(this)\">\n";
		for (int i = 0; i < NUM_COLORS; i++) {
			String colorName = arr_crgbcolors[i].colornamecrgb;
			String bgcolor = arr_crgbcolors[i].colorvaluenamehex;
			// Compare the current color with the value of this option and if they match, we mark "selected"
			if (i == defaultDLColorIndex) {
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


void toggleDownlights(int state, int brightness) {
	// state is 1 = on and 0 = off.  
	// value is 0-255 for brightness
	switch(state) {
		case 0:
			ShelfDisplays->setInternalLEDColor(CRGB::Black);
			downlightersOnOffState = false;
			break;
		case 1:
			if (brightness >= 1) {
				downlightersOnOffState = true;
				defaultDLColor = clamp_rgb(defaultDLColor, brightness);
				ShelfDisplays->setInternalLEDColor(defaultDLColor);
				currentDLBrightnessLevel = brightness;
				updateSetting("DLBrightness", String(currentDLBrightnessLevel));
			}
			break;
	}
	(downlightersOnOffState) ? updateSetting("DLOn", "On") : updateSetting("DLOn", "Off");
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
			if (brightness >= 1) {
				clockOnOffState = true;
				defaultHourColor = clamp_rgb(defaultHourColor, brightness);
				defaultMinColor = clamp_rgb(defaultMinColor, brightness);
				ShelfDisplays->setHourSegmentColors(defaultHourColor);
				ShelfDisplays->setMinuteSegmentColors(defaultMinColor);
				currentClockBrightnessLevel = brightness;
				updateSetting("ClockBrightness", String(currentClockBrightnessLevel));
			}
			break;
	}
	(clockOnOffState) ? updateSetting("ClockOn", "On") : updateSetting("ClockOn", "Off");
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
			if (brightness >= 1) {
				clockOnOffState = true;
				downlightersOnOffState = true;

				defaultHourColor = clamp_rgb(defaultHourColor, brightness);
				defaultMinColor = clamp_rgb(defaultMinColor, brightness);
				ShelfDisplays->setHourSegmentColors(defaultHourColor);
				ShelfDisplays->setMinuteSegmentColors(defaultMinColor);
				currentClockBrightnessLevel = brightness;
				updateSetting("ClockBrightness", String(currentClockBrightnessLevel));

				defaultDLColor = clamp_rgb(defaultDLColor, brightness);
				ShelfDisplays->setInternalLEDColor(defaultDLColor);
				currentDLBrightnessLevel = brightness;
				updateSetting("DLBrightness", String(currentDLBrightnessLevel));
			}
			break;
	}
	(downlightersOnOffState) ? updateSetting("DLOn", "On") : updateSetting("DLOn", "Off");
	(clockOnOffState) ? updateSetting("ClockOn", "On") : updateSetting("ClockOn", "Off");
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
	if (currentTime.hours == 0 && !USE_24_HOUR_FORMAT) {
		targetHourH = 12 / 10;
		targetHourL = 12 % 10;
	} else {
		targetHourH = currentTime.hours / 10;
		targetHourL = currentTime.hours % 10;
	}
	//targetHourH = currentTime.hours / 10;
	//targetHourL = currentTime.hours % 10;
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

/*
void AlarmTriggered()
{
    states->switchMode(ClockState::ALARM_NOTIFICATION);
}

void TimerTick(){
	// Not sure what this was for.
	WebSerial.println("TimerTick Called...");
}

void TimerDone()
{
    states->switchMode(ClockState::TIMER_NOTIFICATION);
}
*/

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
