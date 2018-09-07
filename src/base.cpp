/****************************************************************************
 *   Copyright (c) 2018 Yeap Bing Cheng. All rights reserved.
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

CorrectBase::CorrectBase()
: port("/dev/ttyUSB0")
, surveyAccuracy(5.0)
, surveyDuration(90.0)
, connected(false)
{
    //surveyInStatus = new SurveyInStatus();
    //pReportSatInfo = new satellite_info_s();
}

CorrectBase::~CorrectBase()
{
    // TODO: delete dynamic GPS driver
    
    if (serial) {
        delete serial;
        serial = nullptr;
    }
    
    // TODO: delete dynamic reportsatinfo
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

/*
CorrectBase::operateBase()
CorrectBase::publishPosition()
CorrectBase::publishSatellite()
CorrectBase::gpsConnect()
CorrectBase::publishRTCM()
CorrectBase::callbackEntry()
CorrectBase::callback()
*/
