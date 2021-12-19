#ifndef _PUBLIC_H_
#define _PUBLIC_H_

#include <time.h>
#include "type.h"
#include "dictionary.h"
#include "iniparser.h"


/* config.ini path */
#define PATH_CONFIG_INI	"./config.ini"


#define FRAME_BUF_SIZE		(CONFIG_CAPTURE_WIDTH(main_mngr.config_ini) *CONFIG_CAPTURE_HEIGH(main_mngr.config_ini) *3)


struct daytm
{
	int hour;
	int min;
	int sec;
};

struct main_mngr_info
{
	dictionary *config_ini;	// .ini config file
};


void print_hex(char *text, uint8_t *buf, int len);

uint32_t time_t_to_sec_day(time_t time);
int sec_day_to_daytm(int sec_day, struct daytm *tm_day);

#endif	// _PUBLIC_H_
