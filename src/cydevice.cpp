#include "cydevice.h"
#include "qmessagebox.h"
#include "qdebug.h"

//#define     PACKETS_PER_TRANSFER				120                      //1020: 100      034: 120 
//#define		PACKETS_SIZE						1024
//#define     NUM_TRANSFER_PER_TRANSACTION        10                         //1020: 9       034: 10
//#define     TIMEOUT_MILLI_SEC_PER_TRANSFER      50


CyDevice::CyDevice(int height, int width)
	:m_height(height),
	m_width(width),
	m_device(nullptr),
	//m_pauseFlag(false),
	m_bitsPerPixel(14),
	m_whichBuffer(0)
{
	m_size = m_height * m_width * 2; 
}

CyDevice::~CyDevice()
{
	delete m_imageBuffer;
}

bool CyDevice::openDevice(HANDLE hnd)
{ 
	if (m_device != nullptr) return false;
	m_device = new CCyUSBDevice(hnd);
	if (!m_device->Open(0))
	{
		delete m_device;
		m_device = nullptr;
		return false;
	}
	m_dataInEndPoint = m_device->BulkInEndPt;
	//m_dataInEndPoint = m_device->EndPointOf(0x82);
	m_controlEndPoint = m_device->ControlEndPt;



	sendControlCode(0xb2);    //usb�豸��ʼ��

	
	sendControlCode(0xb4);    //����豸֡����

	//m_dataInEndPoint->SetXferSize(m_size);

	m_whichBuffer = 0;
	return true;
}

void CyDevice::closeDevice()
{
	//DWORD x = GetCurrentThreadId();
	if (m_device != nullptr)
	{
		delete m_device;
		m_device = nullptr;
	}
}

void CyDevice::setDataSavingSpace(UCHAR **data, int size)
{
	m_dataBuffer = data;
	m_bufferSize = size;

	m_imageBuffer = new ImageData[size];
}


void CyDevice::enableReceving()
{
	m_mutex.lock();
	m_recevingFlag = true;
	m_mutex.unlock();
}

void CyDevice::disableReceving()
{
	m_mutex.lock();
	m_recevingFlag = false;
	m_mutex.unlock();
}

bool CyDevice::sendControlCode(int code)
{
	m_controlEndPoint->Target = TGT_DEVICE;
	m_controlEndPoint->ReqType = REQ_VENDOR;
	m_controlEndPoint->Direction = DIR_TO_DEVICE;
	m_controlEndPoint->Value = 0;
	m_controlEndPoint->Index = 0;

	UCHAR buf = 0;
	LONG len = 0;
	m_controlEndPoint->ReqCode = code;
	return m_controlEndPoint->XferData(&buf, len);
}

bool CyDevice::sendRequestCode(int code, uchar *buf, LONG bufLen)
{
	m_controlEndPoint->Target = TGT_DEVICE;
	m_controlEndPoint->ReqType = REQ_VENDOR;
	m_controlEndPoint->Direction = DIR_TO_DEVICE;
	m_controlEndPoint->Value = 0;
	m_controlEndPoint->Index = 0;

	m_controlEndPoint->ReqCode = code;
	return m_controlEndPoint->XferData(buf, bufLen);
}

void CyDevice::setWidth(int width)
{
	m_width = width;
}

void CyDevice::setHeight(int height)
{
	m_height = height;
}

//void CyDevice::setPauseFlag(bool flag)
//{
//	m_pauseFlag = flag;
//}

void CyDevice::changeWidthTo8bitsPerPixel()
{
	m_mutex.lock();
	sendControlCode(0xb8);
	m_bitsPerPixel = 8;
	m_mutex.unlock();
}

void CyDevice::changeWidthTo16bitsPerPixel()
{
	m_mutex.lock();
	sendControlCode(0xb9);
	m_bitsPerPixel = 12;
	m_mutex.unlock();
}


void CyDevice::receiveData()
{
	receiveData(324 * 256 * 2, 1, 75);
}

void CyDevice::receiveData(LONG sizePerXfer, int xferQueueSize, int timeOut)
{
	uchar *data;

	//bool flag;
	int count = 0;

	LONG size = m_size;//m_size = m_height * m_width * 2

	data = m_dataBuffer[m_whichBuffer];

	LONG sizePerDataXfer;
	OVERLAPPED* inOverlaps = new OVERLAPPED[xferQueueSize];
	UCHAR **inContexts = new UCHAR*[xferQueueSize];

	int transfer_count;
	int transfer_queue_count;
	int transfer_count_limit;


	//m_dataInEndPoint->SetXferSize(PACKETS_SIZE * PACKETS_PER_TRANSFER);

	for (int i = 0; i < xferQueueSize; ++i)
	{
		inOverlaps[i].hEvent = CreateEvent(NULL, false, false, L"CYUSB_IN");
	}

	
	while (m_recevingFlag)
	{
		/**********receive frames in the loop*****************/
		m_mutex.lock();
		m_imageBuffer[m_whichBuffer].m_data = data;
		m_imageBuffer[m_whichBuffer].m_bitsPerPixel = m_bitsPerPixel;
		m_imageBuffer[m_whichBuffer].m_imageHeight = m_height;
		m_imageBuffer[m_whichBuffer].m_imageWidth = m_width;
		  
		m_size = m_bitsPerPixel > 8 ? m_width * m_height * 2 : m_width * m_height;

		//sendControlCode(0xb4);
		sendControlCode(0xb5);

		m_mutex.unlock();

		transfer_count = 0;
		sizePerDataXfer = sizePerXfer;     // FinishDataXfer will modify value of sizePerDataXfer, do the copy
		transfer_count_limit = m_size / sizePerDataXfer;



		for (int i = 0; i < xferQueueSize; ++i)
		{
			inContexts[i] = m_dataInEndPoint->BeginDataXfer(data + i * sizePerDataXfer, sizePerDataXfer, &inOverlaps[i]);
			if (m_dataInEndPoint->NtStatus || m_dataInEndPoint->UsbdStatus)      //BeginDataXfer failed
			{
				qDebug() << "BeginDataXfer failed!";
				return;
				//TODO 
				//��ЩBeginDataXferʧ�ܵ���
			}
		}

		transfer_queue_count = xferQueueSize;

		int i = 0;

		/*******receive a frame in the loop***********/
		while (true)
		{
			

			if (!m_dataInEndPoint->WaitForXfer(&inOverlaps[i], timeOut))
			{
				m_dataInEndPoint->Abort();
				/*if (m_dataInEndPoint->LastError == ERROR_IO_PENDING)
				WaitForSingleObject(inOverlaps[i].hEvent, 2000);*/

				//TODO
				qDebug() << "WaitForXfer failed!" << i << "  " << transfer_count;
				break;
			}

			if (!m_dataInEndPoint->FinishDataXfer(data + transfer_count * sizePerDataXfer, sizePerDataXfer, &inOverlaps[i], inContexts[i]))
			{
				//TODO
				qDebug() << "FinishDataXfer failed!";
				break;
			}

			//qDebug() << "xfer succeed" << transfer_count << "  " << transfer_queue_count;
			++transfer_count;

			sizePerDataXfer = sizePerXfer;

			if (transfer_queue_count < transfer_count_limit)
			{

				inContexts[i] = m_dataInEndPoint->BeginDataXfer(data + transfer_queue_count * sizePerDataXfer, sizePerDataXfer, &inOverlaps[i]);
				if (m_dataInEndPoint->NtStatus || m_dataInEndPoint->UsbdStatus)      //BeginDataXfer failed
				{
					qDebug() << "BeginDataXfer in loop failed!";
					return;
					//TODO 
					//��ЩBeginDataXferʧ�ܵ���
				}


				++transfer_queue_count;
			}

			++i;

			if (i == xferQueueSize)
				i = 0;

			if (transfer_count == transfer_count_limit)
			{

				qDebug() << "..."
				;//emit completeFrameTransmission();
				//if (!m_pauseFlag)
				//{
				if (ImageFifo::inFifo(&m_imageBuffer[m_whichBuffer]))
				{
					++m_whichBuffer;
					if (m_whichBuffer == m_bufferSize)
						m_whichBuffer = 0;
					data = m_dataBuffer[m_whichBuffer];
				}
				//}

				break;
			}

		}

	}

	for (int i = 0; i < xferQueueSize; ++i)
	{
		CloseHandle(inOverlaps[i].hEvent);
	}

	delete[] inOverlaps;
	delete[] inContexts;
}

//void CyDevice::changeResolution(int width, int height, int req, long sizePerXfer, int xferQueueSize, int timeOut)
//{
//	changeResolution(width, height, req);
//	receiveData(sizePerXfer, xferQueueSize, timeOut);
//}

bool CyDevice::isReceving()
{
	QMutexLocker locker(&m_mutex);
	return m_recevingFlag;
}