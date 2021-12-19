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


static MainWindow *mainwindow;
extern struct main_mngr_info main_mngr;

struct videoframe_info videoframe_array[VIDEO_AREA_NUM];

int videoframe_update(pthread_t tid, uint8_t *data, int len)
{
	int flush_len = 0;
	int index;

	if(len > FRAME_BUF_SIZE)
		flush_len = FRAME_BUF_SIZE;
	else
		flush_len = len;

	for(index=0; index<VIDEO_AREA_NUM; index++)
	{
		if(videoframe_array[index].tid == tid)
			break;
	}
	if(index >= VIDEO_AREA_NUM)
		return -1;

	pthread_mutex_lock(&videoframe_array[index].mutex);
	memset(videoframe_array[index].frame_buf, 0, FRAME_BUF_SIZE);
	memcpy(videoframe_array[index].frame_buf, data, flush_len);
	pthread_mutex_unlock(&videoframe_array[index].mutex);
	videoframe_array[index].frame_len = flush_len;

	return 0;
}

int videoframe_get_one(int index, uint8_t *data, uint32_t size, int *len)
{
	uint32_t tmpLen;

	if(data==NULL || size<=0)
		return -1;

	tmpLen = (videoframe_array[index].frame_len <size ? videoframe_array[index].frame_len:size);
	if(tmpLen < videoframe_array[index].frame_len)
	{
		printf("Warning: %s: bufout size[%d] < frame size[%d] !!!\n", __FUNCTION__, size, videoframe_array[index].frame_len);
	}
	if(tmpLen <= 0)
	{
		//printf("Warning: %s: no data !!!\n", __FUNCTION__);
		return -1;
	}

	pthread_mutex_lock(&videoframe_array[index].mutex);
	memcpy(data, videoframe_array[index].frame_buf, tmpLen);
	pthread_mutex_unlock(&videoframe_array[index].mutex);
	*len = tmpLen;

	return 0;
}

int videoframe_clear(int index)
{
	pthread_mutex_lock(&videoframe_array[index].mutex);
	memset(videoframe_array[index].frame_buf, 0, videoframe_array[index].frame_len);
	pthread_mutex_unlock(&videoframe_array[index].mutex);
	videoframe_array[index].frame_len = 0;

	return 0;
}

int videoframe_set_tid(int index, pthread_t tid)
{
	videoframe_array[index].tid = tid;

	return 0;
}

/* the frame memory use to sotre the newest one frame */
int videoframe_mem_init(void)
{
	uint8_t *buf;
	int i;

	for(i=0; i<VIDEO_AREA_NUM; i++)
	{
		buf = (uint8_t *)calloc(1, FRAME_BUF_SIZE);
		if(buf == NULL)
		{
			printf("ERROR: %s: malloc failed\n", __FUNCTION__);
			return -1;
		}
		videoframe_array[i].tid = 0;
		videoframe_array[i].index = 0;
		videoframe_array[i].frame_len = 0;
		videoframe_array[i].frame_buf = buf;
		pthread_mutex_init(&videoframe_array[i].mutex, NULL);
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
	QPixmap pixmap;
	QPixmap fitpixmap;
	QTextCodec *codec = QTextCodec::codecForName("GBK");

	funcArea_width = CONFIG_WINDOW_WIDTH(main_mngr.config_ini) -CONFIG_VIDEO_AREA_WIDTH(main_mngr.config_ini);
	
	/* can show Chinese word */
	setWindowTitle(codec->toUnicode(CONFIG_WINDOW_TITLE(main_mngr.config_ini)));
	
	resize(CONFIG_WINDOW_WIDTH(main_mngr.config_ini), CONFIG_WINDOW_HEIGHT(main_mngr.config_ini));

	mainWindow = new QWidget;
	setCentralWidget(mainWindow);

	backgroundImg.load(WIN_BACKGRD_IMG);

	/* clock */
	y_pix += 5;
	widget_height = 30;
	clockLabel = new QLabel(mainWindow);
	clockLabel->setWordWrap(true);	// adapt to text, can show multi row
	clockLabel->setGeometry(0, y_pix, funcArea_width, widget_height);	// height: set more bigger to adapt to arm
	clockLabel->show();
	y_pix += widget_height;

	y_pix += 5;
	image.load(EXTRAINFO_USER_IMG);
	widget_height = image.height();

	extraInfo = new QLabel(mainWindow);
	extraInfo->setPixmap(QPixmap::fromImage(image));
	extraInfo->setGeometry(0, y_pix, funcArea_width, widget_height);
	extraInfo->show();
	y_pix += widget_height;

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

	/* set video area coordate and background image */
	width = CONFIG_CAPTURE_WIDTH(main_mngr.config_ini)/sqrt(VIDEO_AREA_NUM);
	height = CONFIG_CAPTURE_HEIGH(main_mngr.config_ini)/sqrt(VIDEO_AREA_NUM);
	for(int i=0; i<VIDEO_AREA_NUM; i++)
	{
		pixmap = QPixmap::fromImage(backgroundImg);
		fitpixmap = pixmap.scaled(width, height, Qt::KeepAspectRatio, Qt::SmoothTransformation);
		videoArea[i]->setPixmap(fitpixmap);
		videoArea[i]->show();
	}
	videoArea[0]->setGeometry(160, 40, width, height);
	videoArea[1]->setGeometry(160 +width, 40, width, height);
	videoArea[2]->setGeometry(160, 40 +height, width, height);
	videoArea[3]->setGeometry(160 +width, 40 +height, width, height);

	buf_size = CONFIG_CAPTURE_WIDTH(main_mngr.config_ini) *CONFIG_CAPTURE_HEIGH(main_mngr.config_ini) *3;
	video_buf = (unsigned char *)malloc(buf_size);
	if(video_buf == NULL)
	{
		buf_size = 0;
		printf("ERROR: malloc for video_buf failed!");
	}

	/* video buf to store receive frame for */
	videoframe_mem_init();

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
	int index;

	timer->stop();

	QDateTime time = QDateTime::currentDateTime();
	QString str = time.toString("yyyy-MM-dd hh:mm:ss dddd");
	clockLabel->setText(str);

	/* show capture image */
	for(index=0; index<VIDEO_AREA_NUM; index++)
	{
		if(client_tid[index])
		{
			ret = videoframe_get_one(index, video_buf, buf_size, &len);
			if(ret == 0)
			{
				QImage videoQImage;
				QPixmap pixmap;

				videoQImage = jpeg_to_QImage(video_buf, len);
				pixmap = QPixmap::fromImage(videoQImage);
				pixmap = pixmap.scaled(videoArea[index]->width(), videoArea[index]->height(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
				videoArea[index]->setPixmap(pixmap);
				videoArea[index]->show();
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
			videoframe_set_tid(index, client_tid[index]);
			ipPortEdit[index]->setEnabled(false);
			connectBtn[index]->setText(codec->toUnicode(TEXT_DISCONNECT));
		}
		videoframe_clear(index);
	}
	else	// disconnect
	{
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
	videoframe_clear(index);
}

int mainwindow_init(void)
{
	mainwindow = new MainWindow;

	videoframe_mem_init();

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


