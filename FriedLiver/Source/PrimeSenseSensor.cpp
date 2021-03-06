
#include "stdafx.h"

#include "PrimeSenseSensor.h"

//Only working with OpenNI 2 SDK (which wants to run on Win8)
#ifdef OPEN_NI

PrimeSenseSensor::PrimeSenseSensor()
{
	m_bDepthReceived = false;
	m_bColorReceived = false;

	m_bDepthImageIsUpdated = false;
	m_bDepthImageCameraIsUpdated = false;
	m_bNormalImageCameraIsUpdated = false;
}

PrimeSenseSensor::~PrimeSenseSensor()
{
	if (m_streams != NULL)
	{
		delete[] m_streams;
	}

	m_depthStream.stop();
	m_colorStream.stop();
	m_depthStream.destroy();
	m_colorStream.destroy();
	m_device.close();
	openni::OpenNI::shutdown();
}

void PrimeSenseSensor::createFirstConnected()
{
	openni::Status rc = openni::STATUS_OK;
	const char* deviceURI = openni::ANY_DEVICE;

	rc = openni::OpenNI::initialize();

	std::cout << "After initialization: " << openni::OpenNI::getExtendedError() << std::endl;

	// Create Device
	std::string filename = GlobalAppState::getInstance().s_oniFile + ".oni";
	printf("OpenNI Record File: %s\n", filename.c_str());
	rc = m_device.open(filename.c_str());//m_device.open(deviceURI);
	filename = GlobalAppState::getInstance().s_oniFile + "_frame_exp.txt";
	record_exposure = fopen(filename.c_str(), "r");
	filename = GlobalAppState::getInstance().s_oniFile + "_curve.txt";
	FILE* record_curve = fopen(filename.c_str(), "r");
	for (int i = 0; i < 256; i++) {
		fscanf(record_curve, "%f", curve + i);
		curve[i] = exp(curve[i]);
	}
	fclose(record_curve);
	if (rc != openni::STATUS_OK)
	{
		std::cout << "Device open failed: " << openni::OpenNI::getExtendedError() << std::endl;
		openni::OpenNI::shutdown();
		return;
	}

	openni::PlaybackControl* pc = m_device.getPlaybackControl();
	pc->setSpeed(-1.0);
	pc->setRepeatEnabled(false);
	// Create Depth Stream
	rc = m_depthStream.create(m_device, openni::SENSOR_DEPTH);
	if (rc == openni::STATUS_OK)
	{
		rc = m_depthStream.start();
		if (rc != openni::STATUS_OK)
		{
			std::cout << "Couldn't start depth stream: " << openni::OpenNI::getExtendedError() << std::endl;
			m_depthStream.destroy();
		}
	}
	else
	{
		std::cout << "Couldn't find depth stream: " << openni::OpenNI::getExtendedError() << std::endl;
	}

	// Create Color Stream
	rc = m_colorStream.create(m_device, openni::SENSOR_COLOR);
	if (rc == openni::STATUS_OK)
	{
		rc = m_colorStream.start();
		if (rc != openni::STATUS_OK)
		{
			std::cout << "Couldn't start color stream: " << openni::OpenNI::getExtendedError() << " Return code: " << rc << std::endl;
			m_colorStream.destroy();
		}
	}
	else
	{
		std::cout << "Couldn't find color stream: " << openni::OpenNI::getExtendedError() << std::endl;
	}

	// Check Streams
	if (!m_depthStream.isValid() || !m_colorStream.isValid())
	{
		std::cout << "No valid streams. Exiting" << std::endl;
		openni::OpenNI::shutdown();
		return;
	}

	m_colorStream.setMirroringEnabled(true);
	m_depthStream.setMirroringEnabled(true);

	// Get Dimensions
	m_depthVideoMode = m_depthStream.getVideoMode();
	m_colorVideoMode = m_colorStream.getVideoMode();

	int depthWidth = m_depthVideoMode.getResolutionX();
	int depthHeight = m_depthVideoMode.getResolutionY();
	int colorWidth = m_colorVideoMode.getResolutionX();
	int colorHeight = m_colorVideoMode.getResolutionY();

	RGBDSensor::init(depthWidth, depthHeight, colorWidth, colorHeight, 1);

	m_streams = new openni::VideoStream * [2];
	m_streams[0] = &m_depthStream;
	m_streams[1] = &m_colorStream;

	if (rc != openni::STATUS_OK)
	{
		openni::OpenNI::shutdown();
		return;
	}

	m_device.setImageRegistrationMode(openni::IMAGE_REGISTRATION_DEPTH_TO_COLOR);

	float focalLengthX = (depthWidth / 2.0f) / tan(m_depthStream.getHorizontalFieldOfView() / 2.0f);
	float focalLengthY = (depthHeight / 2.0f) / tan(m_depthStream.getVerticalFieldOfView() / 2.0f);
	initializeDepthIntrinsics(focalLengthX, focalLengthY, depthWidth / 2.0f, depthHeight / 2.0f);

	focalLengthX = (colorWidth / 2.0f) / tan(m_colorStream.getHorizontalFieldOfView() / 2.0f);
	focalLengthY = (colorHeight / 2.0f) / tan(m_colorStream.getVerticalFieldOfView() / 2.0f);
	initializeColorIntrinsics(focalLengthX, focalLengthY, colorWidth / 2.0f, colorHeight / 2.0f);

	initializeColorExtrinsics(mat4f::identity());
}

bool PrimeSenseSensor::processDepth()
{
	m_bDepthImageIsUpdated = false;
	m_bDepthImageCameraIsUpdated = false;
	m_bNormalImageCameraIsUpdated = false;

	static int cnt = 0;
	printf("primesense: %d\n", cnt++);
	if (readDepthAndColor(getDepthFloat(), m_colorRGBX))
	{
		m_bDepthImageIsUpdated = true;
		m_bDepthImageCameraIsUpdated = true;
		m_bNormalImageCameraIsUpdated = true;

		m_bDepthReceived = true;
		m_bColorReceived = true;
		return true;
	}
	return false;
}

bool PrimeSenseSensor::readDepthAndColor(float* depthFloat, vec4f* colorRGBX)
{
	bool hr = true;
	int exposure = 0;
	if (record_exposure) {
		if (fscanf(record_exposure, "%d", &exposure) != 1)
			return false;
		printf("Exposure: %d\n", exposure);
	}
	if (exposure == 0) exposure = 16;
	openni::Status sd = m_depthStream.readFrame(&m_depthFrame);
	openni::Status sc = m_colorStream.readFrame(&m_colorFrame);

	if (sd != openni::Status::STATUS_OK || sc != openni::Status::STATUS_OK)
		return false;

	assert(m_colorFrame.getWidth() == m_depthFrame.getWidth());
	assert(m_colorFrame.getHeight() == m_depthFrame.getHeight());

	const openni::DepthPixel* pDepth = (const openni::DepthPixel*)m_depthFrame.getData();
	const openni::RGB888Pixel* pImage = (const openni::RGB888Pixel*)m_colorFrame.getData();

	// check if we need to draw depth frame to texture
	if (m_depthFrame.isValid() && m_colorFrame.isValid())
	{
		unsigned int width = m_depthFrame.getWidth();
		unsigned int nPixels = m_depthFrame.getWidth() * m_depthFrame.getHeight();

		for (unsigned int i = 0; i < nPixels; i++) {
			const int x = i % width;
			const int y = i / width;
			const int src = y * width + (width - 1 - x);
			const openni::DepthPixel& p = pDepth[src];

			float dF = (float)p * 0.0001f;
			//float dF = (float)p * 0.001f;
			if (dF >= GlobalAppState::get().s_sensorDepthMin && dF <= GlobalAppState::get().s_sensorDepthMax) depthFloat[i] = dF;
			else																	 depthFloat[i] = -std::numeric_limits<float>::infinity();
		}
		incrementRingbufIdx();
	}
	// check if we need to draw depth frame to texture
	if (m_depthFrame.isValid() && m_colorFrame.isValid())
	{
		unsigned int width = m_colorFrame.getWidth();
		unsigned int height = m_colorFrame.getHeight();
		unsigned int nPixels = m_colorFrame.getWidth() * m_colorFrame.getHeight();

		for (unsigned int i = 0; i < nPixels; i++)
		{
			const int x = i % width;
			const int y = i / width;

			int y2 = 0;
			if (m_colorWidth == 1280)	y2 = y + 64 / 2 - 10 - (unsigned int)(((float)y / ((float)(height - 1))) * 64 + 0.5f);
			else						y2 = y;

			if (y2 >= 0 && y2 < (int)height)
			{
				//unsigned int Index1D = y2*width+x;
				unsigned int Index1D = y2 * width + (width - 1 - x);	//x-flip here

				const openni::RGB888Pixel& pixel = pImage[Index1D];
				float b, g, r, w = 0;
				w = max(w, min(pixel.b + 1, 256 - pixel.b));
				w = max(w, min(pixel.g + 1, 256 - pixel.g));
				w = max(w, min(pixel.r + 1, 256 - pixel.r));
				w /= 256;
				if (record_exposure)
				{
					b = curve[pixel.b] * 8 / exposure;
					g = curve[pixel.g] * 8 / exposure;
					r = curve[pixel.r] * 8 / exposure;
				}
				else {
					b = pixel.r / 255.0f;
					g = pixel.g / 255.0f;
					r = pixel.b / 255.0f;
				}
				/* unsigned int c = 0;
				c |= min(b, 255);
				c <<= 8;
				c |= min(g, 255);
				c <<= 8;
				c |= min(r, 255);
				c |= 0xFF000000; */
				colorRGBX[y * width + x][0] = b;
				colorRGBX[y * width + x][1] = g;
				colorRGBX[y * width + x][2] = r;
				colorRGBX[y * width + x][3] = w;
			}
		}
	}


	return hr;
}

#endif
