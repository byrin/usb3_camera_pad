#ifndef IMAGEMODEL_H
#define IMAGEMODEL_H

#include "cydevice.h"
#include "imageprocess.h"
#include <QtWidgets>
//#include <opencv2\highgui\highgui.hpp>
//#include <opencv2\imgproc\imgproc.hpp>

using namespace cv;

class ImageModel: public QObject
{
	Q_OBJECT

public:
	ImageModel(QWidget *mainWindow, int height, int width);
	~ImageModel();

	void setImageColorFlag(ImageProcess::ImageColorFlag flag);
	
	bool openUSBCamera();
	void closeUSBCamera();
	void readData();

	void beginVideo();
	void finishVideo();
	
	void setResolution(int height, int width);
	void saveData();

	void whetherPausingUSBCamera(bool flag);

	void changeWidthTo8bitsPerPixel(bool flag);
	
	
	void sendSettingCommand(uchar u1, uchar u2, uchar u3, uchar u4);

	void setSavingPath(QString path);
	void takeImage();
	void initialImageFifo();

	void openFile();

signals:
	void resolutionChanged(int width, int height, int req, long sizePerXfer, int xferQueueSize, int timeOut);

private:
	CyDevice m_camera;
	ImageProcess m_imageProcess;
	QWidget *m_mainWindow;
	int m_imageHeight;
	int m_imageWidth;
	

	QThread m_receiveThread;
	QThread m_imageProcessThread;

	uchar** m_imageDataSavingSpace;



	
};

#endif