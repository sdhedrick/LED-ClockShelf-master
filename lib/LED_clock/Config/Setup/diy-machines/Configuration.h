/**
 * \file Configuration.h
 * \author Florian Laschober
 * \brief This is the place to tweak and setup everything. Individual features can be turned on or off here
 *        as well as many other configuration parameters.
 * 		  This configuration is for a 12h display without intermediate segments
 */

#ifndef _CONFIGURATIONS_H_
#define _CONFIGURATIONS_H_

#define ESP_HOST_NAME	"ShelfClock"

#define MAX_MILLIAMPS 4000 // 4 amps at 5v

#define ENABLE_ALEXA true
#define ALEXA_LAMP_1 "Shelf Down Lights"
#define ALEXA_LAMP_2 "Shelf Clock Lights"
#define ALEXA_LAMP_3 "Schlock"

#define RUN_WITHOUT_WIFI 		false
#if RUN_WITHOUT_WIFI == false

	/**
	 * \brief If you want to use OTA upload instead or in addition to the normal cable upload set this option to true.
	 * To actually flash something via OTA you have to uncomment the OTA flash lines in the \ref platformio.ini file
	 * This is a nice addition to cable upload but it doesn't replace it completely.
	 * If the microcontroller crashes because of bad configuration you still have to use a cable
	 */
	#define ENABLE_OTA_UPLOAD			true

	#if ENABLE_OTA_UPLOAD == true
		#define OTA_UPDATE_HOST_NAME	"ShelfClock"
	#endif

	#define NUM_RETRIES 			50

	#define USE_ESPTOUCH_SMART_CONFIG	false

	#if USE_ESPTOUCH_SMART_CONFIG == false
		#define WIFI_SSID	"YOURWIFISSID"
		#define WIFI_PW		"YOURWIFIPW"
	#endif

#endif

#define DEF_DOWNLIGHTERSONOFFSTATE			true
#define DEF_CLOCKONOFFSTATE					true

#define HOUR_COLOR							CRGB::DarkBlue
#define MINUTE_COLOR						CRGB::DarkOrange

#define INTERNAL_COLOR						CRGB::Tan
#define SEPARATION_DOT_COLOR				CRGB::Blue
#define OTA_UPDATE_COLOR					CRGB::Orange
#define WIFI_CONNECTING_COLOR				CRGB::Blue
#define WIFI_CONNECTION_SUCCESSFUL_COLOR	CRGB::Green
#define WIFI_SMART_CONFIG_COLOR				CRGB::Yellow
#define ERROR_COLOR							CRGB::Red

#define NTP_SERVER "pool.ntp.org"
#define TIMEZONE_INFO "CST6CDT"
#define TIME_SYNC_INTERVAL 86400

#define TIMER_FLASH_TIME false
#define TIMER_FLASH_COUNT 10

#define ALARM_NOTIFICATION_PERIOD 600
#define NOTIFICATION_BRIGHTNESS 125

// How often the time is checked and the displays are updated
#define TIME_UPDATE_INTERVAL	500

#define DEFAULT_CLOCK_BRIGHTNESS 128

#define USE_NIGHT_MODE false
#define DEFAULT_NIGHT_MODE_START_HOUR 23
#define DEFAULT_NIGHT_MODE_START_MINUTE 0
#define DEFAULT_NIGHT_MODE_END_HOUR 7
#define DEFAULT_NIGHT_MODE_END_MINUTE 0
#define DEFAULT_NIGHT_MODE_BRIGHTNESS 0


/***************************
*
* LED Configuration
*
*****************************/
#define LED_DATA_PIN			23 // ESP32
#define NUM_SEGMENTS 			23
#define NUM_LEDS_PER_SEGMENT	9
#define APPEND_DOWN_LIGHTERS	false
#define ADDITIONAL_LEDS			12

#if APPEND_DOWN_LIGHTERS == true
	#define NUM_LEDS 				(NUM_SEGMENTS * NUM_LEDS_PER_SEGMENT + ADDITIONAL_LEDS)
#else
	#define NUM_LEDS 				(NUM_SEGMENTS * NUM_LEDS_PER_SEGMENT)
	#define DOWNLIGHT_LED_DATA_PIN			22	// ESP32
#endif

#define NUM_DISPLAYS			4
enum DisplayIDs {
	LOWER_DIGIT_MINUTE_DISPLAY = 0,
	HIGHER_DIGIT_MINUTE_DISPLAY = 1,
	LOWER_DIGIT_HOUR_DISPLAY = 2,
	HIGHER_DIGIT_HOUR_DISPLAY = 3
	};

#define DISPLAY_SWITCH_OFF_AT_0 	false
#define USE_24_HOUR_FORMAT			false

// The number of segments to use for displaying a progress bar for the OTA updates
#define NUM_SEGMENTS_PROGRESS		14

// The time is shall take for one iteration of the loading animation
#define LOADING_ANIMATION_DURATION		3000

// How fast the brightness interpolation shall react to brightness changes
#define BRIGHTNESS_INTERPOLATION	3000

// If set to -1 the flashing middle dot is disabled, otherwise this is the index of the Display segment that should display the dot.
#define DISPLAY_FOR_SEPARATION_DOT -1
#define DOT_FLASH_SPEED 2000
#define DOT_FLASH_INTERVAL	4000
#define NUM_SEPARATION_DOTS	2

#define ANIMATION_TARGET_FPS		60 // was 60

// Length of sooth animation transition from fully on to black and vice versa in percent
// NOTE: The higher this number the less obvious easing effects like bounce or elastic will be
#define ANIMATION_AFTERGLOW			0.2

/***************************
*
* Light sensor settings
*
*****************************/

#define ENABLE_LIGHT_SENSOR			false

#if ENABLE_LIGHT_SENSOR == true
	#define LIGHT_SENSOR_PIN			36
	#define LIGHT_SENSOR_AVERAGE		15
	#define LIGHT_SENSOR_MEDIAN_WIDTH	5
	#define LIGHT_SENSOR_READ_DELAY		500
	#define LIGHT_SENSOR_MIN			5
	#define LIGHT_SENSOR_MAX			4095
	#define LIGHT_SENSOR_SENSITIVITY	100
#endif


/*********************************
*
*	Misc settings:
*
**********************************/
#define TIME_MANAGER_DEMO_MODE	false
#define TEST_MODE	false
#define TEST_MODE_ON_STARTUP	true

// The time it takes for one digit to morph into another
#define DIGIT_ANIMATION_SPEED 900

// The minimum delay between calls of FastLED.show()
#define FASTLED_SAFE_DELAY_MS 20 // was 20

#endif
