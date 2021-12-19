#include <iostream>
#include <QApplication>
#include "mainwindow.h"
#include "socket_client.h"

/* C++ include C */
#ifdef __cplusplus
extern "C" {
#endif
#ifdef __cplusplus
}
#endif


using namespace std;

struct main_mngr_info main_mngr;


int main(int argc, char* argv[])
{
    QApplication qtApp(argc, argv);
	
	cout << "hello monitor_app" << endl;

	memset(&main_mngr, 0, sizeof(struct main_mngr_info));
	/* load config file */
	main_mngr.config_ini = iniparser_load(PATH_CONFIG_INI);
	if(main_mngr.config_ini == NULL)
	{
		printf("WARNING: %s: load [%s] failed, will use default value.\n", __FUNCTION__, PATH_CONFIG_INI);
		//return -1;	// will use default value
	}
	main_mngr.socket_handle = -1;
	
	start_mainwindow_task();

	socket_client_mngr_init();
	
	return qtApp.exec();		// start qt application, message loop ...
}
