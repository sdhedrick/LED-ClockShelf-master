/**
 * \file Configuration.h
 * \author Florian Laschober
 * \brief This is the place to tweak and setup everything. Individual features can be turned on or off here
 *        as well as many other configuration parameters.
 * 		  This configuration is for a 12h display without intermediate segments
 */

#ifndef _CONFIGURATIONS_H_
#define _CONFIGURATIONS_H_

/**
 * \addtogroup MainConfiguration
 * \brief Main configuration settings
 * \{
 */

/**
 * \brief If you want to run the system in a minimal mode to test some basic functionality or
 * 		  debug something it could be useful to disable wifi functionality completely.
 */
#define RUN_WITHOUT_WIFI 		false
#define ESP_HOST_NAME	"ShelfClock"

#define MAX_MILLIAMPS 4000 // 4 amps at 5v

#define ENABLE_ALEXA true
#define ALEXA_LAMP_1 "Shelf Down Lights"
#define ALEXA_LAMP_2 "Shelf Clock Lights"
#define ALEXA_LAMP_3 "Schlock"

#if RUN_WITHOUT_WIFI == false

	/**
	 * \brief If you want to use OTA upload instead or in addition to the normal cable upload set this option to true.
	 * To actually flash something via OTA you have to uncomment the OTA flash lines in the \ref platformio.ini file
	 * This is a nice addition to cable upload but it doesn't replace it completely.
	 * If the microcontroller crashes because of bad configuration you still have to use a cable
	 */
	#define ENABLE_OTA_UPLOAD			true

	#if ENABLE_OTA_UPLOAD == true
		/**
		 * \brief The host name that shall be used for OTA updates. If you change this here it must also be changed
         *        in the \ref platformio.ini file
		 */
		#define OTA_UPDATE_HOST_NAME	"ShelfClock"
	#endif

	/**
	 * \brief The number of times the controller tries to connect to wifi before it fails and goes into
     *        smartConfig mode (if that is enabled)
	 */
	#define NUM_RETRIES 			50

	/**
	 * \brief Use the ESP smart config to setup the wifi network. If you want to set it manually set this to false
	 */
	#define USE_ESPTOUCH_SMART_CONFIG	false

	#if USE_ESPTOUCH_SMART_CONFIG == false
		/**
		 * \brief WIFI_SSID and WIFI_PW are only needed if smart setup is disabled
		 */
		#define WIFI_SSID	"YOUR_WIFI_SSID"
		#define WIFI_PW		"YOUR_WIFI_PW"
	#endif

#endif

/**
 * \brief The Default OnOff State of the Downlighters (Internal LEDs)
 */
#define DEF_DOWNLIGHTERSONOFFSTATE			true

/**
 * \brief The Default OnOff State of the Clock (Internal LEDs)
 */
#define DEF_CLOCKONOFFSTATE					true

/**
 * \brief Color of the hour segments, this will be the default color
 */
#define HOUR_COLOR							CRGB::DarkBlue

/**
 * \brief Color of the minute segments, this will be the default color
 */
#define MINUTE_COLOR						CRGB::DarkOrange

/**
 * \brief Color of the internal LEDs, this will be the default color
 */
#define INTERNAL_COLOR						CRGB::Tan

/**
 * \brief Color of the separation dot LEDs, this will be the default color
 */
#define SEPARATION_DOT_COLOR				CRGB::Blue

/**
 * \brief Color of the LEDs for the OTA update progress bar
 */
#define OTA_UPDATE_COLOR					CRGB::Orange

/**
 * \brief Color of the LEDs while searching for a WIFI network
 */
#define WIFI_CONNECTING_COLOR				CRGB::Blue

/**
 * \brief Color of the LEDs signaling a successful WIFI connection
 */
#define WIFI_CONNECTION_SUCCESSFUL_COLOR	CRGB::Green

/**
 * \brief Color of the LEDs if system is waiting for WIFI smart config
 */
#define WIFI_SMART_CONFIG_COLOR				CRGB::Yellow

/**
 * \brief Color of the LEDs signaling an error of some sort
 */
#define ERROR_COLOR							CRGB::Red

/**
 * \brief Server for the time
 */
#define NTP_SERVER "pool.ntp.org"

/**
 * \brief Enter the string for your timezone according to this webpage: https://remotemonitoringsystems.ca/time-zone-abbreviations.php
 */
//#define TIMEZONE_INFO "CET-1CEST-2,M3.5.0/02:00:00,M10.5.0/03:00:00"
#define TIMEZONE_INFO "CST6CDT"

/**
 * \brief Time in seconds for the interval in which the time should be synchronized with the time server
 */
#define TIME_SYNC_INTERVAL 1800

/**
 * \brief Flash the current time in case a timer is expired instead of flashing 00:00
 */
#define TIMER_FLASH_TIME false

/**
 * \brief Number of flashes until an alarm is considered complete and the system goes back to normal
 */
#define TIMER_FLASH_COUNT 10

/**
 * \brief For how long the Display should flash when an alarm was fired in seconds
 */
#define ALARM_NOTIFICATION_PERIOD 600

/**
 * \brief How bright the clock should blink when an alarm or timer was triggered, range: 0 - 255
 */
#define NOTIFICATION_BRIGHTNESS 125

/**
 * \brief How often the time is checked and the displays are updated
 */
#define TIME_UPDATE_INTERVAL	500

/**
 * \brief Default brightness of the display. 
 */
#define DEFAULT_CLOCK_BRIGHTNESS 128

/**
 * \brief Whether to activate night mode or not. If you want the clock to reduce brightness/switch off during certain hours set this to true.
 */
#define USE_NIGHT_MODE true

#if USE_NIGHT_MODE == true

	/**
	 * \brief Start hour for the night mode
	 */
	#define DEFAULT_NIGHT_MODE_START_HOUR 23

	/**
	 * \brief Start minute for the night mode
	 */
	#define DEFAULT_NIGHT_MODE_START_MINUTE 0

	/**
	 * \brief End hour for the night mode
	 */
	#define DEFAULT_NIGHT_MODE_END_HOUR 7

	/**
	 * \brief End minute for the night mode
	 */
	#define DEFAULT_NIGHT_MODE_END_MINUTE 0

	/**
	 * \brief Brightness that the clock should be set to while night mode is active
	 */
	#define DEFAULT_NIGHT_MODE_BRIGHTNESS 0

#endif


/***************************
*
* LED Configuration
*
*****************************/

/**
 * \brief Pin to which the led strip data pin is connected to.  Default was 21
 */
#define LED_DATA_PIN			23 // ESP32
//#define LED_DATA_PIN 			5 // ESP8266 D1 WeMos

/**
 * \brief Total number of segments that have LEDs in the shelf
 */
#define NUM_SEGMENTS 			23

/**
 * \brief Number of LEDs in each segment
 */
#define NUM_LEDS_PER_SEGMENT	9

/**
 * \brief If you wired the down lighter LEDs to the end of the LED strips set this to true
 */
#define APPEND_DOWN_LIGHTERS	false

/**
 * \brief Number of LEDs For interior lights
 */
#define ADDITIONAL_LEDS			12

/**
 * \brief Automatically calculated total number of LEDs used
 */
#if APPEND_DOWN_LIGHTERS == true
	#define NUM_LEDS 				(NUM_SEGMENTS * NUM_LEDS_PER_SEGMENT + ADDITIONAL_LEDS)
#else
	#define NUM_LEDS 				(NUM_SEGMENTS * NUM_LEDS_PER_SEGMENT)

	/**
	 * \brief Pin to which the downlight led Strip data pin is connected to.  Default is 22.
	 */
	#define DOWNLIGHT_LED_DATA_PIN			22	// ESP32
	//#define DOWNLIGHT_LED_DATA_PIN			4	// D1 WeMos Mini
#endif

/**
 * \brief Number of displays in the shelf
 */
#define NUM_DISPLAYS			4

/**
 * \brief These enum definitions are used in the code do address the different Seven segment displays.
 * 		  The numbers have to match with the place of the display in the #DisplayManager::SegmentDisplayModes array in the file \ref DisplayConfiguration.cpp
 */
enum DisplayIDs {
	LOWER_DIGIT_MINUTE_DISPLAY = 0,
	HIGHER_DIGIT_MINUTE_DISPLAY = 1,
	LOWER_DIGIT_HOUR_DISPLAY = 2,
	HIGHER_DIGIT_HOUR_DISPLAY = 3
	};

/**
 * \brief If set to true the display will show 0 at midnight and 12 otherwise
 */
#define DISPLAY_0_AT_MIDNIGHT 		false

/**
 * \brief If set to true the higher displays will turn off in case they would show 0
 */
#define DISPLAY_SWITCH_OFF_AT_0 	false

/**
 * \brief If set to true 24 hour format will be used. For this one additional column is needed in the shelf to display it correctly
 */
#define USE_24_HOUR_FORMAT			false

/**
 * \brief The number of segments to use for displaying a progress bar for the OTA updates
 */
#define NUM_SEGMENTS_PROGRESS		14

/**
 * \brief The time is shall take for one iteration of the loading animation
 */
#define LOADING_ANIMATION_DURATION		3000

/**
 * \brief How fast the brightness interpolation shall react to brightness changes
 */
#define BRIGHTNESS_INTERPOLATION	3000

/**
 * \brief If set to -1 the flashing middle dot is disabled, otherwise this is the index of the Display segment that should display the dot.
 */
#define DISPLAY_FOR_SEPARATION_DOT -1

/**
 * \brief Target Frames per second for the smoothness of animations
 */
#define ANIMATION_TARGET_FPS		60 // was 60

/**
 * \brief Length of sooth animation transition from fully on to black and vice versa in percent
 * 		  NOTE: The higher this number the less obvious easing effects like bounce or elastic will be
 */
#define ANIMATION_AFTERGLOW			0.2

/**
 * \brief Length of the dot/s fading animation. One flash fades in and out
 */
#define DOT_FLASH_SPEED 2000

/**
 * \brief Interval in which the dot/s should flash
 */
#define DOT_FLASH_INTERVAL	4000

/**
 * \brief Number of separation dots to use by default
 * 		  allowed values are 1, 2 and 0 to turn it off
 */
#define NUM_SEPARATION_DOTS	2


/***************************
*
* Light sensor settings
*
*****************************/

/**
 * \brief Enable automatic brightness adjustments based on a light sensor
 */
#define ENABLE_LIGHT_SENSOR			false

#if ENABLE_LIGHT_SENSOR == true
	/**
	 * \brief ADC pin to which the light sensor is connected to
	 */
	#define LIGHT_SENSOR_PIN			36

	/**
	 * \brief How many measurements shall be averaged. Higher number -> smoother but slower change
	 */
	#define LIGHT_SENSOR_AVERAGE		15

	/**
	 * \brief Width of the median calculation. Higher number -> smoother change
	 * 		  Should never be higher than the LIGHT_SENSOR_AVERAGE
	 */
	#define LIGHT_SENSOR_MEDIAN_WIDTH	5

	/**
	 * \brief Time that should pass before the light sensor is read again. Higher number -> slower adjustments but also changes will be more sudden
	 */
	#define LIGHT_SENSOR_READ_DELAY		500

	/**
	 * \brief AnalogRead value if the light sensor reads complete darkness
	 */
	#define LIGHT_SENSOR_MIN			5

	/**
	 * \brief AnalogRead value if the light sensor reads the brightest
	 */
	#define LIGHT_SENSOR_MAX			4095

	/**
	 * \brief Value between 0 and 255 that determines how much the light sensor values can influence the led brightness
	 */
	#define LIGHT_SENSOR_SENSITIVITY	100

#endif


/*********************************
*
*	Misc settings:
*
**********************************/


/**
 * \brief enable for wifi less operation or to demo all the animations
 */
#define TIME_MANAGER_DEMO_MODE	false

/**
 * \brief enable for displaying a test pattern instead of time
 */
#define TEST_MODE	false

/**
 * \brief enable for displaying a test pattern during each startup.  Will cycle up once 0-9.
 */
#define TEST_MODE_ON_STARTUP	true

/**
 * \brief The time it takes for one digit to morph into another
 */
#define DIGIT_ANIMATION_SPEED 900

/**
 * \brief the minimum delay between calls of FastLED.show()
 */
#define FASTLED_SAFE_DELAY_MS 20 // was 20


/** \} */

#endif