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

#define TIMER_INTERV_MS			1

#define WIN_BACKGRD_IMG			"resource/background.jpg"		// ΩÁ√Ê±≥æ∞Õº
#define EXTRAINFO_USER_IMG		"resource/extraInfo_user.png"


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
	QLabel 			*videoArea;			// video area
	QImage			backgroundImg;		// background image
	QTimer 			*timer;				// display timer
	QLabel			*clockLabel;		// display clock
	QLabel 			*extraInfo;			// show as image
	unsigned char 	*video_buf;
	unsigned int 	buf_size;
	
public:
	int 			mainwin_mode;
};


int start_mainwindow_task(void);


#endif	// _MAINWINDOW_H_