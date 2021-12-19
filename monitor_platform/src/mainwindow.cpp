#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <iostream>
#include <math.h>
#include "config.h"
#include "mainwindow.h"
#include "image_convert.h"
#include "socket_client.h"


/* C++ include C */
#ifdef __cplusplus
extern "C" {
#endif
#include "protocol.h"
#include "public.h"
#ifdef __cplusplus
}
#endif


unsigned char *newframe_buf;
unsigned int newframe_len;
pthread_mutex_t	newframe_mut;
static MainWindow *mainwindow;
extern struct main_mngr_info main_mngr;

int newframe_update(unsigned char *data, int len)
{
	int flush_len = 0;

	if(len > FRAME_BUF_SIZE)
		flush_len = FRAME_BUF_SIZE;
	else
		flush_len = len;

	pthread_mutex_lock(&newframe_mut);
	memset(newframe_buf, 0, FRAME_BUF_SIZE);
	memcpy(newframe_buf, data, flush_len);
	pthread_mutex_unlock(&newframe_mut);
	newframe_len = flush_len;

	return 0;
}

int newframe_get_one(unsigned char *data, uint32_t size, int *len)
{
	uint32_t tmpLen;

	if(data==NULL || size<=0)
		return -1;

	tmpLen = (newframe_len <size ? newframe_len:size);
	if(tmpLen < newframe_len)
	{
		printf("Warning: %s: bufout size[%d] < frame size[%d] !!!\n", __FUNCTION__, size, newframe_len);
	}
	if(tmpLen <= 0)
	{
		//printf("Warning: %s: no data !!!\n", __FUNCTION__);
		return -1;
	}

	pthread_mutex_lock(&newframe_mut);
	memcpy(data, newframe_buf, tmpLen);
	pthread_mutex_unlock(&newframe_mut);
	*len = tmpLen;

	return 0;
}

int newframe_clear(int index)
{
	pthread_mutex_lock(&newframe_mut);
	memset(newframe_buf, 0, FRAME_BUF_SIZE);
	pthread_mutex_unlock(&newframe_mut);
	newframe_len = 0;

	return 0;
}

/* the frame memory use to sotre the newest one frame */
int newframe_mem_init(void)
{

	pthread_mutex_init(&newframe_mut, NULL);

	newframe_buf = (unsigned char *)calloc(1, FRAME_BUF_SIZE);
	if(newframe_buf == NULL)
	{
		printf("ERROR: %s: malloc failed\n", __FUNCTION__);
		return -1;
	}

	return 0;
}


MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
	int y_pix = 0;
	int funcArea_width;
	int widget_height;
	int width, height;
	QImage image;
	QFont font;
	QPalette pa;
	QTextCodec *codec = QTextCodec::codecForName("GBK");

	funcArea_width = CONFIG_WINDOW_WIDTH(main_mngr.config_ini) -CONFIG_VIDEO_AREA_WIDTH(main_mngr.config_ini);
	
	/* can show Chinese word */
	setWindowTitle(codec->toUnicode(CONFIG_WINDOW_TITLE(main_mngr.config_ini)));
	
	resize(CONFIG_WINDOW_WIDTH(main_mngr.config_ini), CONFIG_WINDOW_HEIGHT(main_mngr.config_ini));

	mainWindow = new QWidget;
	setCentralWidget(mainWindow);

	backgroundImg.load(WIN_BACKGRD_IMG);

	/* show video area */
	for(int i=0; i<VIDEO_AREA_NUM; i++)
	{
		QLabel *lab_video = new QLabel(mainWindow);
		QLineEdit *edit_input = new QLineEdit(lab_video);
		QPushButton *conn_btn = new QPushButton(lab_video);

		lab_video->setAlignment(Qt::AlignCenter);
		edit_input->setPlaceholderText(codec->toUnicode(TEXT_IP_PORT));
		edit_input->setAlignment(Qt::AlignCenter);
		conn_btn->setText(codec->toUnicode(TEXT_CONNECT));
		conn_btn->setGeometry(0, 25, 40, 20);
    	connect(conn_btn, SIGNAL(clicked()), this, SLOT(connect_svr()));

		videoArea.push_back(lab_video);
		ipPortEdit.push_back(edit_input);
		connectBtn.push_back(conn_btn);
		client_tid.push_back(0);
	}

	QPixmap pixmap;
	QPixmap fitpixmap;
	width = CONFIG_CAPTURE_WIDTH(main_mngr.config_ini)/sqrt(VIDEO_AREA_NUM);
	height = CONFIG_CAPTURE_HEIGH(main_mngr.config_ini)/sqrt(VIDEO_AREA_NUM);
	videoArea[0]->setGeometry(160, 40, width, height);
	pixmap = QPixmap::fromImage(backgroundImg);
	fitpixmap = pixmap.scaled(width, height, Qt::KeepAspectRatio, Qt::SmoothTransformation);
	videoArea[0]->setPixmap(fitpixmap);
	videoArea[0]->show();
	
	videoArea[1]->setGeometry(160 +width, 40, width, height);
	pixmap = QPixmap::fromImage(backgroundImg);
	width = videoArea[1]->width();
	height = videoArea[1]->height();
	fitpixmap = pixmap.scaled(width, height, Qt::KeepAspectRatio, Qt::SmoothTransformation);
	videoArea[1]->setPixmap(fitpixmap);
	videoArea[1]->show();

	videoArea[2]->setGeometry(160, 40 +height, width, height);
	pixmap = QPixmap::fromImage(backgroundImg);
	width = videoArea[2]->width();
	height = videoArea[2]->height();
	fitpixmap = pixmap.scaled(width, height, Qt::KeepAspectRatio, Qt::SmoothTransformation);
	videoArea[2]->setPixmap(fitpixmap);
	videoArea[2]->show();

	videoArea[3]->setGeometry(160 +width, 40 +height, width, height);
	pixmap = QPixmap::fromImage(backgroundImg);
	width = videoArea[3]->width();
	height = videoArea[3]->height();
	fitpixmap = pixmap.scaled(width, height, Qt::KeepAspectRatio, Qt::SmoothTransformation);
	videoArea[3]->setPixmap(fitpixmap);
	videoArea[3]->show();

	/* clock */
	y_pix += 5;
	widget_height = 90;
	clockLabel = new QLabel(mainWindow);
	clockLabel->setWordWrap(true);	// adapt to text, can show multi row
	clockLabel->setGeometry(5, y_pix, funcArea_width, widget_height);	// height: set more bigger to adapt to arm
	clockLabel->show();
	y_pix += widget_height;

	image.load(EXTRAINFO_USER_IMG);
	widget_height = 150;

	extraInfo = new QLabel(mainWindow);
	extraInfo->setPixmap(QPixmap::fromImage(image));
	extraInfo->setGeometry(0, y_pix, funcArea_width, widget_height);
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
	int len;
	int ret;
	int i;

	timer->stop();

	QDateTime time = QDateTime::currentDateTime();
	QString str = time.toString("yyyy-MM-dd hh:mm:ss dddd");
	clockLabel->setText(str);

	/* show capture image */
	for(i=0; i<VIDEO_AREA_NUM; i++)
	{
		if(client_tid[i])
		{
			ret = newframe_get_one(video_buf, buf_size, &len);
			if(ret == 0)
			{
				QImage videoQImage;
				QPixmap pixmap;

				videoQImage = jpeg_to_QImage(video_buf, len);
				pixmap = QPixmap::fromImage(videoQImage);
				pixmap = pixmap.scaled(videoArea[i]->width(), videoArea[i]->height(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
				videoArea[i]->setPixmap(pixmap);
				videoArea[i]->show();
			}
		}
		
	}

	timer->start(TIMER_INTERV_MS);
	
}

void MainWindow::connect_svr(void)
{
	// get sender obj
	QPushButton *btn = qobject_cast<QPushButton *>(sender());
	QTextCodec *codec = QTextCodec::codecForName("GBK");
	QString svr_str;
	QByteArray ba;
	char ip_str[32];
	int index;
	
	for(index=0; index<VIDEO_AREA_NUM; index++)
	{
		if(btn == connectBtn[index])
		{
			break;
		}
	}

	if(client_tid[index] == 0)	// connect
	{
		svr_str = ipPortEdit[index]->text();
		if(svr_str.length() <9 || svr_str.length() >20)
		{
			cout << "IP input error!\n" << endl;
			return ;
		}
		qDebug() << svr_str;

		ba = svr_str.toLatin1();
		memset(ip_str, 0, sizeof(ip_str));
		strncpy((char *)ip_str, ba.data(), strlen(ba.data()));

		// start socket client connect
		client_tid[index] = start_socket_client_task(ip_str);
		if(client_tid[index] > 0)
		{
			ipPortEdit[index]->setEnabled(false);
			connectBtn[index]->setText(codec->toUnicode(TEXT_DISCONNECT));
		}
		newframe_clear(index);
	}
	else	// disconnect
	{
		cout << "disconnect server." << endl;
		disconnect_svr(index);
	}

}

void MainWindow::disconnect_svr(int index)
{
	QTextCodec *codec = QTextCodec::codecForName("GBK");

	cout << "video disconnect " << index << endl;

	client_mngr_set_client_exit(client_tid[index]);

	client_tid[index] = 0;
	ipPortEdit[index]->setEnabled(true);
	connectBtn[index]->setText(codec->toUnicode(TEXT_CONNECT));

	QPixmap pixmap;
	QPixmap fitpixmap;
	int width = CONFIG_CAPTURE_WIDTH(main_mngr.config_ini)/sqrt(VIDEO_AREA_NUM);
	int height = CONFIG_CAPTURE_HEIGH(main_mngr.config_ini)/sqrt(VIDEO_AREA_NUM);
	pixmap = QPixmap::fromImage(backgroundImg);
	fitpixmap = pixmap.scaled(width, height, Qt::KeepAspectRatio, Qt::SmoothTransformation);
	videoArea[index]->setPixmap(fitpixmap);
	videoArea[index]->show();
	newframe_clear(index);
}

int mainwindow_init(void)
{
	mainwindow = new MainWindow;

	newframe_mem_init();

	mainwindow->show();
	
	return 0;
}

void mainwindow_deinit(void)
{

}

void mainwin_video_disconnect(pthread_t tid)
{
	for(int index=0; index<VIDEO_AREA_NUM; index++)
	{
		if(mainwindow->client_tid[index] == tid)
		{
			mainwindow->disconnect_svr(index);
		}
	}
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


