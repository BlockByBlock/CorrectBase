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

#include "ubxm8p.h"

UBXM8P::UBXM8P(){};

UBXM8P::~UBXM8P(){};

// -1 = error, 0 = no message handled, 1 = message handled, 2 = sat info message handled
int	UBXM8P::receive(unsigned timeout)
{
	uint8_t buf[GPS_READ_BUFFER_SIZE];

	/* timeout additional to poll */
	gps_abstime time_started = gps_absolute_time();

	int handled = 0;

	while (true) {
		bool ready_to_return = _configured ? (_got_posllh && _got_velned) : handled;

		/* Wait for only UBX_PACKET_TIMEOUT if something already received. */
		int ret = read(buf, sizeof(buf), ready_to_return ? UBX_PACKET_TIMEOUT : timeout);

		if (ret < 0) {
			/* something went wrong when polling or reading */
			printf("ubx poll_or_read err");
			return -1;

		} else if (ret == 0) {
			/* return success if ready */
			if (ready_to_return) {
				_got_posllh = false;
				_got_velned = false;
				return handled;
			}

		} else {
            // DEBUG
			//printf("read %d bytes", ret);

			/* pass received bytes to the packet decoder */
			for (int i = 0; i < ret; i++) {
				handled |= parseChar(buf[i]);
                // DEBUG
				//printf("parsed %d: 0x%x", i, buf[i]);
			}

			return handled;
		}

		/* abort after timeout if no useful packets received */
		if (time_started + timeout * 1000 < gps_absolute_time()) {
			printf("timed out, returning");
			return -1;
		}
	}
}

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

        /* reset all configuration on the module - this is necessary as some vendors
		 * lock bad configurations
		 */
        ubx_payload_tx_cfg_cfg_t cfg_cfg = {};
        // Clear settings
        cfg_cfg.clearMask = ((1 << 12) | (1 << 11) | (1 << 10) | (1 << 9) |
					     (1 << 8) | (1 << 4) | (1 << 3) | (1 << 2) | (1 << 1) | (1 << 0));
        // Storage devices to apply this
		cfg_cfg.deviceMask = (1 << 2) | (1 << 1) | (1 << 0);

        if (!sendMessage(UBX_MSG_CFG_CFG, (uint8_t *)&cfg_cfg, sizeof(ubx_payload_tx_cfg_cfg_t))) {
				printf("cfg reset: UART TX failed");
			}

			if (waitForAck(UBX_MSG_CFG_CFG, UBX_CONFIG_TIMEOUT, true) < 0) {
				printf("cfg reset failed");

			} else {
				printf("cfg reset ACK");
			}
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

bool UBXM8P::sendMessage(const uint16_t msg, const uint8_t *payload, const uint16_t length)
{
    ubx_header_t   header = {UBX_SYNC1, UBX_SYNC2, 0, 0};
	ubx_checksum_t checksum = {0, 0};

    // Populate header
	header.msg	= msg;
	header.length	= length;

    // Calculate checksum
	calcChecksum(((uint8_t *)&header) + 2, sizeof(header) - 2, &checksum); // skip 2 sync bytes

    if (payload != nullptr) {
		calcChecksum(payload, length, &checksum);
	}

	// Send message
	if (write((void *)&header, sizeof(header)) != sizeof(header)) {
		return false;
	}

	if (payload && write((void *)payload, length) != length) {
		return false;
	}

	if (write((void *)&checksum, sizeof(checksum)) != sizeof(checksum)) {
		return false;
	}

	return true;
}

void UBXM8P::calcChecksum(const uint8_t *buffer, const uint16_t length, ubx_checksum_t *checksum)
{
	for (uint16_t i = 0; i < length; i++) {
		checksum->ck_a = checksum->ck_a + buffer[i];
		checksum->ck_b = checksum->ck_b + checksum->ck_a;
	}
}

// -1 = NAK, error or timeout, 0 = ACK
int UBXM8P::waitForAck(const uint16_t msg, const unsigned timeout, const bool report)
{
	int ret = -1;

	_ack_state = UBX_ACK_WAITING;
	_ack_waiting_msg = msg;	// memorize sent msg class&ID for ACK check

	gps_abstime time_started = gps_absolute_time();

	while ((_ack_state == UBX_ACK_WAITING) && (gps_absolute_time() < time_started + timeout * 1000)) {
		receive(timeout);
	}

	if (_ack_state == UBX_ACK_GOT_ACK) {
		ret = 0;	// ACK received ok

	} else if (report) {
		if (_ack_state == UBX_ACK_GOT_NAK) {
			printf("ubx msg 0x%04x NAK", SWAP16((unsigned)msg));

		} else {
			printf("ubx msg 0x%04x ACK timeout", SWAP16((unsigned)msg));
		}
	}

	_ack_state = UBX_ACK_IDLE;
	return ret;
}

// 0 = decoding, 1 = message handled, 2 = sat info message handled
int	UBXM8P::parseChar(const uint8_t b)
{
	int ret = 0;

	switch (_decode_state) {

	/* Expecting Sync1 */
	case UBX_DECODE_SYNC1:
		if (b == UBX_SYNC1) {	// Sync1 found --> expecting Sync2
			UBX_TRACE_PARSER("A");
			_decode_state = UBX_DECODE_SYNC2;

		} else if (b == RTCM3_PREAMBLE && _rtcm_parsing) {
			UBX_TRACE_PARSER("RTCM");
			_decode_state = UBX_DECODE_RTCM3;
			_rtcm_parsing->addByte(b);
		}

		break;

	/* Expecting Sync2 */
	case UBX_DECODE_SYNC2:
		if (b == UBX_SYNC2) {	// Sync2 found --> expecting Class
			UBX_TRACE_PARSER("B");
			_decode_state = UBX_DECODE_CLASS;

		} else {		// Sync1 not followed by Sync2: reset parser
			decodeInit();
		}

		break;

	/* Expecting Class */
	case UBX_DECODE_CLASS:
		UBX_TRACE_PARSER("C");
		addByteToChecksum(b);  // checksum is calculated for everything except Sync and Checksum bytes
		_rx_msg = b;
		_decode_state = UBX_DECODE_ID;
		break;

	/* Expecting ID */
	case UBX_DECODE_ID:
		UBX_TRACE_PARSER("D");
		addByteToChecksum(b);
		_rx_msg |= b << 8;
		_decode_state = UBX_DECODE_LENGTH1;
		break;

	/* Expecting first length byte */
	case UBX_DECODE_LENGTH1:
		UBX_TRACE_PARSER("E");
		addByteToChecksum(b);
		_rx_payload_length = b;
		_decode_state = UBX_DECODE_LENGTH2;
		break;

	/* Expecting second length byte */
	case UBX_DECODE_LENGTH2:
		UBX_TRACE_PARSER("F");
		addByteToChecksum(b);
		_rx_payload_length |= b << 8;	// calculate payload size

		if (payloadRxInit() != 0) {	// start payload reception
			// payload will not be handled, discard message
			decodeInit();

		} else {
			_decode_state = (_rx_payload_length > 0) ? UBX_DECODE_PAYLOAD : UBX_DECODE_CHKSUM1;
		}

		break;

	/* Expecting payload */
	case UBX_DECODE_PAYLOAD:
		UBX_TRACE_PARSER(".");
		addByteToChecksum(b);

		switch (_rx_msg) {
		case UBX_MSG_NAV_SVINFO:
			ret = payloadRxAddNavSvinfo(b);	// add a NAV-SVINFO payload byte
			break;

		case UBX_MSG_MON_VER:
			ret = payloadRxAddMonVer(b);	// add a MON-VER payload byte
			break;

		default:
			ret = payloadRxAdd(b);		// add a payload byte
			break;
		}

		if (ret < 0) {
			// payload not handled, discard message
			decodeInit();

		} else if (ret > 0) {
			// payload complete, expecting checksum
			_decode_state = UBX_DECODE_CHKSUM1;

		} else {
			// expecting more payload, stay in state UBX_DECODE_PAYLOAD
		}

		ret = 0;
		break;

	/* Expecting first checksum byte */
	case UBX_DECODE_CHKSUM1:
		if (_rx_ck_a != b) {
            // DEBUG
			printf("ubx checksum err");
			decodeInit();
		} else {
			_decode_state = UBX_DECODE_CHKSUM2;
		}

		break;

	/* Expecting second checksum byte */
	case UBX_DECODE_CHKSUM2:
		if (_rx_ck_b != b) {
            // DEBUG
			printf("ubx checksum err");
		} else {
			ret = payloadRxDone();	// finish payload processing
		}

		decodeInit();
		break;

	case UBX_DECODE_RTCM3:
		if (_rtcm_parsing->addByte(b)) {
            // DEBUG
			printf("got RTCM message with length %i", (int)_rtcm_parsing->messageLength());
			gotRTCMMessage(_rtcm_parsing->message(), _rtcm_parsing->messageLength());
			decodeInit();
		}

		break;

	default:
		break;
	}

	return ret;
}
