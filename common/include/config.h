#ifndef _CONFIG_H_
#define _CONFIG_H_

/******************** get configure value ********************/

// [WINDOW]
#define CONFIG_WINDOW_TITLE(ini)        iniparser_getstring(ini, "WINDOW:windowTitle", DEFAULT_WINDOW_TITLE)
#define CONFIG_WINDOW_WIDTH(ini)        iniparser_getint(ini, "WINDOW:windowWidth", DEFAULT_WINDOW_WIDTH)
#define CONFIG_WINDOW_HEIGHT(ini)       iniparser_getint(ini, "WINDOW:windowHeight", DEFAULT_WINDOW_HEIGHT)
#define CONFIG_WINDOW_FONT_SIZE(ini)   iniparser_getint(ini, "WINDOW:windowFontSize", DEFAULT_WINDOW_FONT_SIZE)

// [CAPTURE]
#define CONFIG_CAPTURE_DEV(ini)     iniparser_getstring(ini, "CAPTURE:captureDev", DEFAULT_CAPTURE_DEV)
#define CONFIG_CAPTURE_WIDTH(ini)    iniparser_getint(ini, "CAPTURE:capuretWidth", DEFAULT_CAPTURE_WIDTH)
#define CONFIG_CAPTURE_HEIGH(ini)    iniparser_getint(ini, "CAPTURE:captureHeight", DEFAULT_CAPTURE_HEIGH)

// [SERVER]
#define CONFIG_SERVER_IP(ini)       iniparser_getstring(ini, "SERVER:serverIp", DEFAULT_SERVER_IP)
#define CONFIG_SERVER_PORT(ini)     iniparser_getint(ini, "SERVER:serverPort", DEFAULT_SERVER_PORT)


/******************** default configure value ********************/

// [WINDOW]
#define DEFAULT_WINDOW_TITLE		"Web ”∆µº‡øÿœµÕ≥"
#define DEFAULT_WINDOW_WIDTH		800
#define DEFAULT_WINDOW_HEIGHT		480
#define DEFAULT_WINDOW_FONT_SIZE	24


// [CAPTURE]
#define DEFAULT_CAPTURE_DEV 		"/dev/video0"
#define DEFAULT_CAPTURE_WIDTH		640
#define DEFAULT_CAPTURE_HEIGH		480


// [SERVER]
#define DEFAULT_SERVER_IP			"127.0.0.1"
#define DEFAULT_SERVER_PORT			9100


#endif	// _CONFIG_H_
