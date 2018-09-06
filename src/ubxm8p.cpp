/****************************************************************************
 *   Copyright (c) 2012-2014 PX4 Development Team. All rights reserved.
 *
 *   Author: Yeap Bing Cheng <ybingcheng@gmail.com> 
 *   (rework from px4 gpsdriver for m8p only and non-mavlink use)
 * 
 ****************************************************************************/

 /**
 * @file ubxm8p.cpp
 *
 * U-Blox M8P protocol implementation. 
 * @author Yeap Bing Cheng <ybingcheng@gmail.com>
 *
 * @see https://www.u-blox.com/sites/default/files/products/documents/u-bloxM8_ReceiverDescrProtSpec_%28UBX-13003221%29_Public.pdf
 */
#include <stdio.h>

#include "ubxm8p.h"

UBXM8P::UBXM8P(){};

UBXM8P::~UBXM8P(){};

int UBXM8P::configure(unsigned &baudrate, OutputMode output_mode)
{
    _configured = false;
    _output_mode = output_mode;
    /* try different baudrates */
	const unsigned baudrates[] = {38400, 57600, 9600, 19200, 115200};

    unsigned baud_i;
    ubx_payload_tx_cfg_prt_t cfg_prt[2];
    /* Set CFG_PRT GPS or RTCM mode*/
    uint16_t out_proto_mask = output_mode == OutputMode::GPS ?
				  UBX_TX_CFG_PRT_OUTPROTOMASK_GPS :
				  UBX_TX_CFG_PRT_OUTPROTOMASK_RTCM;
	uint16_t in_proto_mask = output_mode == OutputMode::GPS ?
				 UBX_TX_CFG_PRT_INPROTOMASK_GPS :
				 UBX_TX_CFG_PRT_INPROTOMASK_RTCM;
    
    for (baud_i = 0; baud_i < sizeof(baudrates) / sizeof(baudrates[0]); baud_i++){
        unsigned test_baudrate = baudrates[baud_i];

        printf("Baudrate is set to %i", test_baudrate);

        if (baudrate > 0 && baudrate != test_baudrate){
            continue;   // skip to next baudrate
        }

        setBaudrate(test_baudrate);
    }

}

void UBXM8P::setSurveySpecs(uint32_t survey_in_acc_limit, uint32_t survey_in_min_dur)
{
    _survey_in_acc_limit = survey_in_acc_limit;
    _survey_in_min_dur = survey_in_min_dur;
}

int UBXM8P::setBaudrate(int baudrate)
{
    return _callback(GPSCallbackType::setBaudrate, nullptr, baudrate, _callback_user);
}
