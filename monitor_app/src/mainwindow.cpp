#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <iostream>
#include "config.h"
#include "mainwindow.h"
#include "image_convert.h"


/* C++ include C */
#ifdef __cplusplus
extern "C" {
#endif
#include "capture.h"
#include "protocol.h"
#include "public.h"
#ifdef __cplusplus
}
#endif


static MainWindow *mainwindow;
extern struct main_mngr_info main_mngr;


MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
	int y_pix = 0;
	int funcArea_width;
	int funcArea_height;
	int widget_height;
	QImage image;
	QFont font;
	QPalette pa;
	QTextCodec *codec = QTextCodec::codecForName("GBK");

	funcArea_width = CONFIG_WINDOW_WIDTH(main_mngr.config_ini) -CONFIG_CAPTURE_WIDTH(main_mngr.config_ini);
	funcArea_height = CONFIG_WINDOW_HEIGHT(main_mngr.config_ini);
	
	/* can show Chinese word */
	setWindowTitle(codec->toUnicode(CONFIG_WINDOW_TITLE(main_mngr.config_ini)));
	
	resize(CONFIG_WINDOW_WIDTH(main_mngr.config_ini), CONFIG_WINDOW_HEIGHT(main_mngr.config_ini));

	mainWindow = new QWidget;
	setCentralWidget(mainWindow);

	backgroundImg.load(WIN_BACKGRD_IMG);

	/* show video area */
	videoArea = new QLabel(mainWindow);
	videoArea->setPixmap(QPixmap::fromImage(backgroundImg));
	videoArea->setGeometry(0, 0, CONFIG_CAPTURE_WIDTH(main_mngr.config_ini), CONFIG_CAPTURE_HEIGH(main_mngr.config_ini));
	videoArea->show();
	
	/* clock */
	y_pix += Y_INTERV_PIXEL_EX;
	widget_height = 90;
	clockLabel = new QLabel(mainWindow);
	clockLabel->setWordWrap(true);	// adapt to text, can show multi row
	clockLabel->setGeometry(FUNC_AREA_PIXEL_X +5, y_pix, funcArea_width, widget_height);	// height: set more bigger to adapt to arm
	clockLabel->show();
	y_pix += widget_height;

	image.load(EXTRAINFO_USER_IMG);
	widget_height = funcArea_height - y_pix;

	extraInfo = new QLabel(mainWindow);
	extraInfo->setPixmap(QPixmap::fromImage(image));
	extraInfo->setGeometry(FUNC_AREA_PIXEL_X, y_pix, funcArea_width, widget_height);
	extraInfo->show();
	y_pix += widget_height;

	buf_size = CONFIG_CAPTURE_WIDTH(main_mngr.config_ini) *CONFIG_CAPTURE_HEIGH(main_mngr.config_ini) *3;
	video_buf = (unsigned char *)malloc(buf_size);
	if(video_buf == NULL)
	{
		buf_size = 0;
		printf("ERROR: malloc for video_buf failed!");
	}

	/* set timer to show image */
	timer = new QTimer(this);
	connect(timer, SIGNAL(timeout()), this, SLOT(showMainwindow()));
	timer->start(TIMER_INTERV_MS);

}

MainWindow::~MainWindow(void)
{
	
}

void MainWindow::showMainwindow(void)
{
	uint32_t len;
	int ret;

	timer->stop();

	QDateTime time = QDateTime::currentDateTime();
	QString str = time.toString("yyyy-MM-dd hh:mm:ss dddd");
	clockLabel->setText(str);

	/* show capture image */
	ret = capture_get_newframe(video_buf, buf_size, &len);
	if(ret == 0)
	{
		QImage videoQImage;

		videoQImage = jpeg_to_QImage(video_buf, len);

		videoArea->setPixmap(QPixmap::fromImage(videoQImage));
		videoArea->show();
	}

	timer->start(TIMER_INTERV_MS);
	
}


int mainwindow_init(void)
{
	mainwindow = new MainWindow;

	mainwindow->show();
	
	return 0;
}

void mainwindow_deinit(void)
{

}

/* notice:
 * use timer to display,
 * if use thread, it will occur some error.
 */
int start_mainwindow_task(void)
{
	mainwindow_init();

	return 0;
}


