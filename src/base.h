/****************************************************************************
 *   Copyright (c) 2018 Yeap Bing Cheng. All rights reserved.
 ****************************************************************************/

/**
* @file base.h
*
* The base calls from the GPS driver file to run the necessary services
* @author Yeap Bing Cheng <ybingcheng@gmail.com>
*/
#include <string>
#include <SerialStream.h>

#ifndef CORR_BASE_H_
#define CORR_BASE_H_

class CorrectBase
{
public:
    CorrectBase();
    ~CorrectBase();

    void serialConnect();
    //void operateBase(){};
    //void publishPosition(){};
    //void publishSatellite(){};
    //void gpsConnect();
    //void publishRTCM();
    //void callbackEntry();
    //void callback();

private:
    //struct vehicle_gps_position_s	reportGPSPos;
	//struct satellite_info_s		*pReportSatInfo = nullptr;
    LibSerial::SerialStream* serial = nullptr;

    std::string port;
    float surveyAccuracy;
    float surveyDuration;
    bool connected;
};

#endif /* CORR_BASE_H_ */
