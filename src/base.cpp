/****************************************************************************
 *   Copyright (C) Alexis Paques 2018. All rights reserved.
 *
 *   This work is based on Alexis Paques <alexis.paques@gmail.com> rtk_ros
 *   https://github.com/AlexisTM/rtk_ros
 *
 ****************************************************************************/
/**
* @file base.cpp
*
* The base calls from the GPS driver file to run the necessary services
* @author Yeap Bing Cheng <ybingcheng@gmail.com>
*/

#include <iostream>
#include "base.h"

using namespace LibSerial;

// TODO: To pass the serial port on run time instead of compile time
CorrectBase::CorrectBase()
: port("/dev/ttyACM0")
, surveyAccuracy(5.0)
, surveyDuration(90.0)
, connected(false)
{
    //surveyInStatus = new SurveyInStatus();
    pReportSatInfo = new satellite_info_s();
}

CorrectBase::~CorrectBase()
{
    if (gpsDriver) {
        delete gpsDriver;
        gpsDriver = nullptr;
    }

    if (serial) {
        serial->Close();
        delete serial;
        serial = nullptr;
    }
    
    if (pReportSatInfo) {
        delete pReportSatInfo;
        pReportSatInfo = nullptr;
    }
}

void CorrectBase::serialConnect()
{
    if (!serial) serial = new SerialStream(port);

    serial->SetBaudRate(BaudRate::BAUD_9600);
    serial->SetCharacterSize(CharacterSize::CHAR_SIZE_8);
    serial->SetParity(Parity::PARITY_NONE);
    serial->SetStopBits(StopBits::STOP_BITS_1);
    serial->SetFlowControl(FlowControl::FLOW_CONTROL_NONE);

    if (serial->IsOpen()) {
        std::cout << "serialConnect - Connected to Serial Device :" << port << std::endl;
    }

    //TODO: add exception and retries
}

void CorrectBase::gpsConnect()
{
    uint8_t stationary_model = 2;   // dynamic model
    printf("gpsConnect - Initialising GPS Driver\n");
    gpsDriver = new UBXM8P(&reportGPSPos, stationary_model);
    printf("gpsConnect - Connected to GPS\n");
    gpsDriver->setSurveyInSpecs(surveyAccuracy * 10000, surveyDuration);
    memset(&reportGPSPos, 0, sizeof(reportGPSPos)); // Reset report

    // Configuring GPS Driver
    unsigned baud = 0;  // TODO: Synch baudrate BaudRate::BAUD and baud
    printf("gpsConnect - Configuring GPS Driver ... ...\n");
    gpsDriver->configure(baud, UBXM8P::OutputMode::RTCM) == 0;
    printf("gpsConnect - GPS Driver Configuration Done!\n");
}

/*
CorrectBase::operateBase()
CorrectBase::publishPosition()
CorrectBase::publishSatellite()
CorrectBase::publishRTCM()
CorrectBase::callback()
CorrectBase::callbackEntry()
*/
