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

/* update frame data for video buffer - 为视频缓存更新图像数据 */
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

/* get one video frame - 从缓存中获取一帧图像数据 */
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

/* clear frame data - 清除图像缓存数据 */
int videoframe_clear(int index)
{
	pthread_mutex_lock(&videoframe_array[index].mutex);
	memset(videoframe_array[index].frame_buf, 0, videoframe_array[index].frame_len);
	pthread_mutex_unlock(&videoframe_array[index].mutex);
	videoframe_array[index].frame_len = 0;

	return 0;
}

/* set tid to video frame, for note it whether connect server or not - 设置视频缓存的tid，为了标记该区域是否连接服务器 */
int videoframe_set_tid(int index, pthread_t tid)
{
	videoframe_array[index].tid = tid;

	return 0;
}

/* initial frame memory to sotre the newest received frame - 初始化一片缓存用来存放最新接收到的图像数据 */
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

/* QT: main window layout - 主界面布局 */
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
	
	/* set window title - 设置窗口标题 */
	setWindowTitle(codec->toUnicode(CONFIG_WINDOW_TITLE(main_mngr.config_ini)));
	
	/* set window size - 设置窗口大小 */
	resize(CONFIG_WINDOW_WIDTH(main_mngr.config_ini), CONFIG_WINDOW_HEIGHT(main_mngr.config_ini));

	/* set central widget - 设置主控件 */
	mainWindow = new QWidget;
	setCentralWidget(mainWindow);

	/* load background image - 加载背景图片 */
	backgroundImg.load(WIN_BACKGRD_IMG);

	/* set clock widget - 设置时钟控件 */
	y_pix += 5;
	widget_height = 30;
	clockLabel = new QLabel(mainWindow);
	clockLabel->setWordWrap(true);	// adapt to text, can show multi row
	clockLabel->setGeometry(0, y_pix, funcArea_width, widget_height);	// set geometry - 设置几何大小坐标
	clockLabel->show();	// display widget - 显示控制
	y_pix += widget_height;

	/* set user info image - 设置用户信息图片 */
	y_pix += 5;
	image.load(EXTRAINFO_USER_IMG);
	widget_height = image.height();
	extraInfo = new QLabel(mainWindow);
	extraInfo->setPixmap(QPixmap::fromImage(image));
	extraInfo->setGeometry(0, y_pix, funcArea_width, widget_height);
	extraInfo->show();
	y_pix += widget_height;

	/* create video area - 创建几个视频显示区域 */
	for(int i=0; i<VIDEO_AREA_NUM; i++)
	{
		QLabel *lab_video = new QLabel(mainWindow);	// video display widget - 视频显示控件
		QLineEdit *edit_input = new QLineEdit(lab_video);	// input (ip:port) widget - 输入控件（IP地址：端口）
		QPushButton *conn_btn = new QPushButton(lab_video);	// connect/disconnect button - 连接/断开按钮

		lab_video->setAlignment(Qt::AlignCenter);
		edit_input->setPlaceholderText(codec->toUnicode(TEXT_IP_PORT));
		edit_input->setAlignment(Qt::AlignCenter);
		conn_btn->setText(codec->toUnicode(TEXT_CONNECT));
		conn_btn->setGeometry(0, 25, 40, 20);
		/* button slot function - 按键的槽函数，信号与槽：当触发了按键时，会调用到槽函数 */
    	connect(conn_btn, SIGNAL(clicked()), this, SLOT(connect_svr()));

		videoArea.push_back(lab_video);
		ipPortEdit.push_back(edit_input);
		connectBtn.push_back(conn_btn);
		client_tid.push_back(0);
	}

	/* set video area geometry and background image - 设置视频区域的大小坐标及背景图*/
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

	/* malloc memory for video data translate  - 为视频数据的转换申请内存 */
	buf_size = CONFIG_CAPTURE_WIDTH(main_mngr.config_ini) *CONFIG_CAPTURE_HEIGH(main_mngr.config_ini) *3;
	video_buf = (unsigned char *)malloc(buf_size);
	if(video_buf == NULL)
	{
		buf_size = 0;
		printf("ERROR: malloc for video_buf failed!");
	}

	/* initial video buf to store receive frame for each - 初始化视频空间，用于缓存接收到的视频，每个视频区域一块缓存 */
	videoframe_mem_init();

	/* create timer to show main window - 创建定时器来显示主界面 */
	timer = new QTimer(this);
	/* if timer timeout it will call slot function - 如果定时器定时时间到了就会调用槽函数showMainwindow() */
	connect(timer, SIGNAL(timeout()), this, SLOT(showMainwindow()));
	/* set timeout time and start timer - 设置超时时间并启动定时器，TIMER_INTERV_MS=1ms */
	timer->start(TIMER_INTERV_MS);

}

MainWindow::~MainWindow(void)
{
	
}

/* timer timeout slot function, to show main window - 定时器定时时间到的槽函数，显示主界面 */
void MainWindow::showMainwindow(void)
{
	int len;
	int ret;
	int index;

	timer->stop();

	/* update clock - 更新时间显示 */
	QDateTime time = QDateTime::currentDateTime();
	QString str = time.toString("yyyy-MM-dd hh:mm:ss dddd");
	clockLabel->setText(str);

	/* show video image - 显示视频图像 */
	for(index=0; index<VIDEO_AREA_NUM; index++)
	{
		if(client_tid[index])
		{
			/* get video frame data - 获取视频区域的图像 */
			ret = videoframe_get_one(index, video_buf, buf_size, &len);
			if(ret == 0)
			{
				QImage videoQImage;
				QPixmap pixmap;

				/* transform jpeg to QImage - 将jpeg格式的图像转换为QT格式的，为了用QT显示 */
				videoQImage = jpeg_to_QImage(video_buf, len);
				pixmap = QPixmap::fromImage(videoQImage);
				pixmap = pixmap.scaled(videoArea[index]->width(), videoArea[index]->height(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);	// resize - 调整大小
				videoArea[index]->setPixmap(pixmap);	// set image data - 设置图像数据到控件
				videoArea[index]->show();	// display - 显示控件
			}
		}
	}

	/* start timer again - 再次启动定时器 */
	timer->start(TIMER_INTERV_MS);
	
}

/* connect server (video capture)  - 连接服务器即摄像头端 */
void MainWindow::connect_svr(void)
{
	// get sender obj
	QPushButton *btn = qobject_cast<QPushButton *>(sender());
	QTextCodec *codec = QTextCodec::codecForName("GBK");
	QString svr_str;
	QByteArray ba;
	char ip_str[32];
	int index;
	
	/* check which video area button clicked - 检查哪个视频区域的按钮被点击 */
	for(index=0; index<VIDEO_AREA_NUM; index++)
	{
		if(btn == connectBtn[index])
		{
			break;
		}
	}

	/* check if connected: if connect, disconnect it; if not, connect it.
	 - 检查是否连接状态，若已连接，则点击按钮为断开，若非连接状态，则为连接 */
	if(client_tid[index] == 0)	// connect state - 连接状态，通过tid判断
	{
		/* get input content - 获取输入的内容 */
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

		// start socket client connect server(capture) - 启动socket客户端去连接服务器（摄像头）
		client_tid[index] = start_socket_client_task(ip_str);
		if(client_tid[index] > 0)
		{
			videoframe_set_tid(index, client_tid[index]);
			ipPortEdit[index]->setEnabled(false);	// disable input - 禁止输入
			connectBtn[index]->setText(codec->toUnicode(TEXT_DISCONNECT));	// set button "disconnect" - 设置按钮显示“断开”
		}
		videoframe_clear(index);
	}
	else	// disconnect state - 断开状态
	{
		disconnect_svr(index);
	}

}

/* disconnect server(click button) - 断开服务器（点击按钮） */
void MainWindow::disconnect_svr(int index)
{
	QTextCodec *codec = QTextCodec::codecForName("GBK");

	cout << "video disconnect " << index << endl;

	/* make client to exit. disconncect server - 让客户断退出，断开与服务器的连接 */
	client_mngr_set_client_exit(client_tid[index]);

	client_tid[index] = 0;
	ipPortEdit[index]->setEnabled(true);
	connectBtn[index]->setText(codec->toUnicode(TEXT_CONNECT));	// set button "connect" - 设置按钮“连接”

	/* show background image - 显示背景图片 */
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

/* main window initial - 主界面初始化 */
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

/* disconnect video server - 断开视频服务器 */
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

/* notice: use timer to display, if use thread, it will occur some error.
 - 注意：要用定时器来驱动显示主界面，如果用线程会出现一些错误
 */
int start_mainwindow_task(void)
{
	mainwindow_init();

	return 0;
}


