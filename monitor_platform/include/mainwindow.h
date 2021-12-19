#ifndef _MAINWINDOW_H_
#define _MAINWINDOW_H_

#include <QMainWindow>
#include <QWidget>
#include <QLabel>
#include <QTimer>
#include <QPushButton>
#include <QLineEdit>
#include <QTextCodec>
#include <QComboBox>
#include <QMessageBox>
#include <QTableView>
#include <QHeaderView>
#include <QStandardItemModel>
#include <QDateTime>
#include <QDateTimeEdit>
#include <QPainter>
#include <QRect>
#include <QDebug>
#include <vector>

/* C++ include C */
#ifdef __cplusplus
extern "C" {
#endif
#include "public.h"
#ifdef __cplusplus
}
#endif

/* widget locate pixel */
#define Y_INTERV_PIXEL_IN		3
#define Y_INTERV_PIXEL_EX		8
#define WIDGET_HEIGHT_PIXEL		30
#define FUNC_AREA_PIXEL_X		CONFIG_CAPTURE_WIDTH(main_mngr.config_ini)
#define VIDEO_AREA_NUM 			4
#define TIMER_INTERV_MS			1

#define WIN_BACKGRD_IMG			"resource/background.jpg"		// ���汳��ͼ
#define EXTRAINFO_USER_IMG		"resource/extraInfo_user.png"

#define TEXT_IP_PORT			"������ IP:Port"
#define TEXT_CONNECT			"����"
#define TEXT_DISCONNECT			"�Ͽ�"


using namespace std;

struct videoframe_info
{
	int index;
	pthread_t tid;
	uint8_t *frame_buf;
	uint32_t frame_len;
	pthread_mutex_t	mutex;
};


class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	MainWindow(QWidget *parent = 0);
	~MainWindow();

private slots:
	void showMainwindow(void);
	
private:
	QWidget			*mainWindow;		// main window
	vector<QLabel*> 	videoArea;			// video area
	vector<QLineEdit*> 	ipPortEdit;			// input ip and port
	vector<QPushButton*> 	connectBtn;			// connect button
	QImage			backgroundImg;		// background image
	QTimer 			*timer;				// display timer
	QLabel			*clockLabel;		// display clock
	QLabel 			*extraInfo;			// show as image
	unsigned char 	*video_buf;
	unsigned int 	buf_size;

private slots:
	void connect_svr(void);

public:
	void disconnect_svr(int index);

	vector<pthread_t> 	client_tid;			// video client tid
};


int videoframe_update(pthread_t tid, uint8_t *data, int len);
int videoframe_get_one(int index, uint8_t *data, uint32_t size, int *len);
int videoframe_clear(int index);

void mainwin_video_disconnect(pthread_t tid);
int start_mainwindow_task(void);


#endif	// _MAINWINDOW_H_