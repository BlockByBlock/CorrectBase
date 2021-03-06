/****************************************************************************
 *   Copyright (c) 2012-2014 PX4 Development Team. All rights reserved.
 *
 *	 Original Work from ubx.cpp by
 * 	 @author Thomas Gubler <thomasgubler@student.ethz.ch>
 *   @author Julian Oes <joes@student.ethz.ch>
 *   @author Anton Babushkin <anton.babushkin@me.com>
 *   @author Beat Kueng <beat-kueng@gmx.net>
 *   @author Hannes Delago
 * 
 ****************************************************************************/

 /**
 * @file ubxm8p.cpp
 *
 * U-Blox M8P protocol implementation. 
 * @author Yeap Bing Cheng <ybingcheng@gmail.com>
 * (rework from px4 gpsdriver for non-mavlink use)
 *
 * @see https://www.u-blox.com/sites/default/files/products/documents/u-bloxM8_ReceiverDescrProtSpec_%28UBX-13003221%29_Public.pdf
 */
 
#include <unistd.h>
#include <cmath>
#include "ubxm8p.h"

#define UBX_CONFIG_TIMEOUT	200		// ms, timeout for waiting ACK
#define UBX_PACKET_TIMEOUT	2		// ms, if now data during this delay assume that full update received
#define DISABLE_MSG_INTERVAL	1000000		// us, try to disable message with this interval

#define MIN(X,Y)	((X) < (Y) ? (X) : (Y))
#define SWAP16(X)	((((X) >>  8) & 0x00ff) | (((X) << 8) & 0xff00))

#define FNV1_32_INIT	((uint32_t)0x811c9dc5)	// init value for FNV1 hash algorithm
#define FNV1_32_PRIME	((uint32_t)0x01000193)	// magic prime for FNV1 hash algorithm

UBXM8P::UBXM8P(struct vehicle_gps_position_s *gps_position, 
				uint8_t dynamic_model)
: _survey_in_acc_limit(UBX_TX_CFG_TMODE3_SVINACCLIMIT)
, _survey_in_min_dur(UBX_TX_CFG_TMODE3_SVINMINDUR)
, _gps_position(gps_position)
, _dyn_model(dynamic_model)
{
	decodeInit();
	// Not present in normal operation, unless for dual-antenna RTK units
	_gps_position->heading = NAN;
};

UBXM8P::~UBXM8P()
{
	if (_rtcm_parsing) {
		delete (_rtcm_parsing);
	}
};

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
	uint16_t out_proto_mask = output_mode == OutputMode::GPS ?
				  UBX_TX_CFG_PRT_OUTPROTOMASK_GPS :
				  UBX_TX_CFG_PRT_OUTPROTOMASK_RTCM;
	uint16_t in_proto_mask = output_mode == OutputMode::GPS ?
				 UBX_TX_CFG_PRT_INPROTOMASK_GPS :
				 UBX_TX_CFG_PRT_INPROTOMASK_RTCM;
	//FIXME: RTCM3 output needs at least protocol version 20. The protocol version can be checked via the version
	//output:
	//WARN  VER ext "                  PROTVER=20.00"
	//However this is a string and it is not well documented, that PROTVER is always contained. Maybe there is a
	//better way to check the protocol version?
	
	for (baud_i = 0; baud_i < sizeof(baudrates) / sizeof(baudrates[0]); baud_i++) {
		unsigned test_baudrate = baudrates[baud_i];

		printf("baudrate set to %i\n", test_baudrate);

		if (baudrate > 0 && baudrate != test_baudrate) {
			continue; // skip to next baudrate
		}
		// Due to callback - Comment out for now
		setBaudrate(test_baudrate);
	
		/* reset all configuration on the module - this is necessary as some vendors lock
		 * lock bad configurations */
			
		ubx_payload_tx_cfg_cfg_t cfg_cfg = {};
		cfg_cfg.clearMask = ((1 << 12) | (1 << 11) | (1 << 10) | (1 << 9) |
						(1 << 8) | (1 << 4) | (1 << 3) | (1 << 2) | (1 << 1) | (1 << 0));
		cfg_cfg.deviceMask = (1 << 2) | (1 << 1) | (1 << 0);
		
		if (!sendMessage(UBX_MSG_CFG_CFG, (uint8_t *)&cfg_cfg, sizeof(ubx_payload_tx_cfg_cfg_t))) 
			printf("cfg reset: UART TX failed\n");
		
		if (waitForAck(UBX_MSG_CFG_CFG, UBX_CONFIG_TIMEOUT, true) < 0) 
			printf("cfg reset failed\n");
		else 
			printf("cfg reset ACK\n");
		
		// allow the module to re-initialize 
		usleep(100000);
	}
		/* flush input and wait for at least 20 ms silence 
		decodeInit();
		receive(20);
		decodeInit();
		*/
		/* Send a CFG-PRT message to set the UBX protocol for in and out
			* and leave the baudrate as it is, we just want an ACK-ACK for this 
		memset(cfg_prt, 0, 2 * sizeof(ubx_payload_tx_cfg_prt_t));
		cfg_prt[0].portID		= UBX_TX_CFG_PRT_PORTID;
		cfg_prt[0].mode		= UBX_TX_CFG_PRT_MODE;
		cfg_prt[0].baudRate	= test_baudrate;
		cfg_prt[0].inProtoMask	= in_proto_mask;
		cfg_prt[0].outProtoMask	= out_proto_mask;
		cfg_prt[1].portID		= UBX_TX_CFG_PRT_PORTID_USB;
		cfg_prt[1].mode		= UBX_TX_CFG_PRT_MODE;
		cfg_prt[1].baudRate	= test_baudrate;
		cfg_prt[1].inProtoMask	= in_proto_mask;
		cfg_prt[1].outProtoMask	= out_proto_mask; */

		/*
		if (!sendMessage(UBX_MSG_CFG_PRT, (uint8_t *)cfg_prt, 2 * sizeof(ubx_payload_tx_cfg_prt_t))) {
			continue;
		}

		if (waitForAck(UBX_MSG_CFG_PRT, UBX_CONFIG_TIMEOUT, false) < 0) {
			// try next baudrate 
			continue;
		}
		*/

		/* Send a CFG-PRT message again, this time change the baudrate 
		memset(cfg_prt, 0, 2 * sizeof(ubx_payload_tx_cfg_prt_t));
		cfg_prt[0].portID		= UBX_TX_CFG_PRT_PORTID;
		cfg_prt[0].mode		= UBX_TX_CFG_PRT_MODE;
		cfg_prt[0].baudRate	= UBX_TX_CFG_PRT_BAUDRATE;
		cfg_prt[0].inProtoMask	= in_proto_mask;
		cfg_prt[0].outProtoMask	= out_proto_mask;
		cfg_prt[1].portID		= UBX_TX_CFG_PRT_PORTID_USB;
		cfg_prt[1].mode		= UBX_TX_CFG_PRT_MODE;
		cfg_prt[1].baudRate	= UBX_TX_CFG_PRT_BAUDRATE;
		cfg_prt[1].inProtoMask	= in_proto_mask;
		cfg_prt[1].outProtoMask	= out_proto_mask;

		if (!sendMessage(UBX_MSG_CFG_PRT, (uint8_t *)cfg_prt, 2 * sizeof(ubx_payload_tx_cfg_prt_t))) {
			continue;
		}

		// no ACK is expected here, but read the buffer anyway in case we actually get an ACK 
		waitForAck(UBX_MSG_CFG_PRT, UBX_CONFIG_TIMEOUT, false);

		if (UBX_TX_CFG_PRT_BAUDRATE != test_baudrate) {
			setBaudrate(UBX_TX_CFG_PRT_BAUDRATE);
		}

		// at this point we have correct baudrate on both ends 
		baudrate = UBX_TX_CFG_PRT_BAUDRATE;
		break;
		
	}*/
	/*
	if (baud_i >= sizeof(baudrates) / sizeof(baudrates[0])) {
		return -1;	// connection and/or baudrate detection failed
	}
	*/

	/* Send a CFG-RATE message to define update rate 
	memset(&_buf.payload_tx_cfg_rate, 0, sizeof(_buf.payload_tx_cfg_rate));
	_buf.payload_tx_cfg_rate.measRate	= UBX_TX_CFG_RATE_MEASINTERVAL;
	_buf.payload_tx_cfg_rate.navRate	= UBX_TX_CFG_RATE_NAVRATE;
	_buf.payload_tx_cfg_rate.timeRef	= UBX_TX_CFG_RATE_TIMEREF;

	if (!sendMessage(UBX_MSG_CFG_RATE, (uint8_t *)&_buf, sizeof(_buf.payload_tx_cfg_rate))) {
		return -1;
	}

	if (waitForAck(UBX_MSG_CFG_RATE, UBX_CONFIG_TIMEOUT, true) < 0) {
		return -1;
	}

	if (output_mode != OutputMode::GPS) {
		// RTCM mode force stationary dynamic model
		_dyn_model = 2;
	}
	*/
	/* send a NAV5 message to set the options for the internal filter 
	memset(&_buf.payload_tx_cfg_nav5, 0, sizeof(_buf.payload_tx_cfg_nav5));
	_buf.payload_tx_cfg_nav5.mask		= UBX_TX_CFG_NAV5_MASK;
	_buf.payload_tx_cfg_nav5.dynModel	= _dyn_model;
	_buf.payload_tx_cfg_nav5.fixMode	= UBX_TX_CFG_NAV5_FIXMODE;

	if (!sendMessage(UBX_MSG_CFG_NAV5, (uint8_t *)&_buf, sizeof(_buf.payload_tx_cfg_nav5))) {
		return -1;
	}

	if (waitForAck(UBX_MSG_CFG_NAV5, UBX_CONFIG_TIMEOUT, true) < 0) {
		return -1;
	}
	*/

#ifdef UBX_CONFIGURE_SBAS
	/* send a SBAS message to set the SBAS options 
	memset(&_buf.payload_tx_cfg_sbas, 0, sizeof(_buf.payload_tx_cfg_sbas));
	_buf.payload_tx_cfg_sbas.mode		= UBX_TX_CFG_SBAS_MODE;

	if (!sendMessage(UBX_MSG_CFG_SBAS, (uint8_t *)&_buf, sizeof(_buf.payload_tx_cfg_sbas))) {
		return -1;
	}

	if (waitForAck(UBX_MSG_CFG_SBAS, UBX_CONFIG_TIMEOUT, true) < 0) {
		return -1;
	}
	*/
#endif

	/* configure message rates 
	 * the last argument is divisor for measurement rate (set by CFG RATE), i.e. 1 means 5Hz 

	 * try to set rate for NAV-PVT 
	 * (implemented for ubx7+ modules only, use NAV-SOL, NAV-POSLLH, NAV-VELNED and NAV-TIMEUTC for ubx6) 
	if (!configureMessageRate(UBX_MSG_NAV_PVT, 1)) {
		return -1;
	}

	if (waitForAck(UBX_MSG_CFG_MSG, UBX_CONFIG_TIMEOUT, true) < 0) {
		_use_nav_pvt = false;

	} else {
		_use_nav_pvt = true;
	}

	printf("%susing NAV-PVT", _use_nav_pvt ? "" : "not ");

	if (!_use_nav_pvt) {
		if (!configureMessageRateAndAck(UBX_MSG_NAV_TIMEUTC, 5, true)) {
			return -1;
		}

		if (!configureMessageRateAndAck(UBX_MSG_NAV_POSLLH, 1, true)) {
			return -1;
		}

		if (!configureMessageRateAndAck(UBX_MSG_NAV_SOL, 1, true)) {
			return -1;
		}

		if (!configureMessageRateAndAck(UBX_MSG_NAV_VELNED, 1, true)) {
			return -1;
		}
	}

	if (!configureMessageRateAndAck(UBX_MSG_NAV_DOP, 1, true)) {
		return -1;
	}

	if (!configureMessageRateAndAck(UBX_MSG_NAV_SVINFO, (_satellite_info != nullptr) ? 5 : 0, true)) {
		return -1;
	}

	if (!configureMessageRateAndAck(UBX_MSG_MON_HW, 1, true)) {
		return -1;
	}
	*/
	/* request module version information by sending an empty MON-VER message 
	if (!sendMessage(UBX_MSG_MON_VER, nullptr, 0)) {
		return -1;
	}

	if (output_mode == OutputMode::RTCM) {
		if (restartSurveyIn() < 0) {
			return -1;
		}
	}

	_configured = true;
	*/
	return 0;
}

int UBXM8P::restartSurveyIn()
{
	if (_output_mode != OutputMode::RTCM) {
		return -1;
	}

	//disable RTCM output
	configureMessageRate(UBX_MSG_RTCM3_1005, 0);
	configureMessageRate(UBX_MSG_RTCM3_1077, 0);
	configureMessageRate(UBX_MSG_RTCM3_1087, 0);

	//stop it first
	//FIXME: stopping the survey-in process does not seem to work
	memset(&_buf.payload_tx_cfg_tmode3, 0, sizeof(_buf.payload_tx_cfg_tmode3));
	_buf.payload_tx_cfg_tmode3.flags        = 0; /* disable time mode */

	if (!sendMessage(UBX_MSG_CFG_TMODE3, (uint8_t *)&_buf, sizeof(_buf.payload_tx_cfg_tmode3))) {
		return -1;
	}

	if (waitForAck(UBX_MSG_CFG_TMODE3, UBX_CONFIG_TIMEOUT, true) < 0) {
		return -1;
	}

	printf("Starting Survey-in");

	memset(&_buf.payload_tx_cfg_tmode3, 0, sizeof(_buf.payload_tx_cfg_tmode3));
	_buf.payload_tx_cfg_tmode3.flags        = UBX_TX_CFG_TMODE3_FLAGS;
	_buf.payload_tx_cfg_tmode3.svinMinDur   = _survey_in_min_dur;
	_buf.payload_tx_cfg_tmode3.svinAccLimit = _survey_in_acc_limit;

	if (!sendMessage(UBX_MSG_CFG_TMODE3, (uint8_t *)&_buf, sizeof(_buf.payload_tx_cfg_tmode3))) {
		return -1;
	}

	if (waitForAck(UBX_MSG_CFG_TMODE3, UBX_CONFIG_TIMEOUT, true) < 0) {
		return -1;
	}

	/* enable status output of survey-in */
	if (!configureMessageRateAndAck(UBX_MSG_NAV_SVIN, 5, true)) {
		return -1;
	}

	return 0;
}

void UBXM8P::setSurveySpecs(uint32_t survey_in_acc_limit, uint32_t survey_in_min_dur)
{
    _survey_in_acc_limit = survey_in_acc_limit;
    _survey_in_min_dur = survey_in_min_dur;
}

int UBXM8P::setBaudrate(int baudrate)
{
	printf("Setting Baudrate - THIS IS NOT RIGHT WITHOUT CALLBACK\n");
    //return _callback(GPSCallbackType::setBaudrate, nullptr, baudrate, _callback_user);
	return baudrate;
}

bool UBXM8P::configureMessageRateAndAck(uint16_t msg, uint8_t rate, bool report_ack_error)
{
	if (!configureMessageRate(msg, rate)) {
		return false;
	}

	return waitForAck(UBX_MSG_CFG_MSG, UBX_CONFIG_TIMEOUT, report_ack_error) >= 0;
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
			printf("A");
			_decode_state = UBX_DECODE_SYNC2;

		} else if (b == RTCM3_PREAMBLE && _rtcm_parsing) {
			printf("RTCM");
			_decode_state = UBX_DECODE_RTCM3;
			_rtcm_parsing->addByte(b);
		}

		break;

	/* Expecting Sync2 */
	case UBX_DECODE_SYNC2:
		if (b == UBX_SYNC2) {	// Sync2 found --> expecting Class
			printf("B");
			_decode_state = UBX_DECODE_CLASS;

		} else {		// Sync1 not followed by Sync2: reset parser
			decodeInit();
		}

		break;

	/* Expecting Class */
	case UBX_DECODE_CLASS:
		printf("C");
		addByteToChecksum(b);  // checksum is calculated for everything except Sync and Checksum bytes
		_rx_msg = b;
		_decode_state = UBX_DECODE_ID;
		break;

	/* Expecting ID */
	case UBX_DECODE_ID:
		printf("D");
		addByteToChecksum(b);
		_rx_msg |= b << 8;
		_decode_state = UBX_DECODE_LENGTH1;
		break;

	/* Expecting first length byte */
	case UBX_DECODE_LENGTH1:
		printf("E");
		addByteToChecksum(b);
		_rx_payload_length = b;
		_decode_state = UBX_DECODE_LENGTH2;
		break;

	/* Expecting second length byte */
	case UBX_DECODE_LENGTH2:
		printf("F");
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
		printf(".");
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

void UBXM8P::decodeInit()
{
	_decode_state = UBX_DECODE_SYNC1;
	_rx_ck_a = 0;
	_rx_ck_b = 0;
	_rx_payload_length = 0;
	_rx_payload_index = 0;

	if (_output_mode == OutputMode::RTCM) {
		if (!_rtcm_parsing) {
			_rtcm_parsing = new RTCMParsing();
		}

		_rtcm_parsing->reset();
	}
}

void UBXM8P::addByteToChecksum(const uint8_t b)
{
	_rx_ck_a = _rx_ck_a + b;
	_rx_ck_b = _rx_ck_b + _rx_ck_a;
}

bool UBXM8P::configureMessageRate(const uint16_t msg, const uint8_t rate)
{
	ubx_payload_tx_cfg_msg_t cfg_msg;	// don't use _buf (allow interleaved operation)
	memset(&cfg_msg, 0, sizeof(cfg_msg));

	cfg_msg.msg	= msg;
	cfg_msg.rate	= rate;

	return sendMessage(UBX_MSG_CFG_MSG, (uint8_t *)&cfg_msg, sizeof(cfg_msg));
}

/**
 * Start payload rx: -1 = abort, 0 = continue
 */
int	UBXM8P::payloadRxInit()
{
	int ret = 0;

	_rx_state = UBX_RXMSG_HANDLE;	// handle by default

	switch (_rx_msg) {
	case UBX_MSG_NAV_PVT:
		if ((_rx_payload_length != UBX_PAYLOAD_RX_NAV_PVT_SIZE_UBX7)		/* u-blox 7 msg format */
		    && (_rx_payload_length != UBX_PAYLOAD_RX_NAV_PVT_SIZE_UBX8)) {	/* u-blox 8+ msg format */
			_rx_state = UBX_RXMSG_ERROR_LENGTH;

		} else if (!_configured) {
			_rx_state = UBX_RXMSG_IGNORE;        // ignore if not _configured

		} else if (!_use_nav_pvt) {
			_rx_state = UBX_RXMSG_DISABLE;        // disable if not using NAV-PVT
		}

		break;

	case UBX_MSG_INF_DEBUG:
	case UBX_MSG_INF_ERROR:
	case UBX_MSG_INF_NOTICE:
	case UBX_MSG_INF_WARNING:
		if (_rx_payload_length >= sizeof(ubx_buf_t)) {
			_rx_payload_length = sizeof(ubx_buf_t) - 1; //avoid buffer overflow
		}

		break;

	case UBX_MSG_NAV_POSLLH:
		if (_rx_payload_length != sizeof(ubx_payload_rx_nav_posllh_t)) {
			_rx_state = UBX_RXMSG_ERROR_LENGTH;

		} else if (!_configured) {
			_rx_state = UBX_RXMSG_IGNORE;        // ignore if not _configured

		} else if (_use_nav_pvt) {
			_rx_state = UBX_RXMSG_DISABLE;        // disable if using NAV-PVT instead
		}

		break;

	case UBX_MSG_NAV_SOL:
		if (_rx_payload_length != sizeof(ubx_payload_rx_nav_sol_t)) {
			_rx_state = UBX_RXMSG_ERROR_LENGTH;

		} else if (!_configured) {
			_rx_state = UBX_RXMSG_IGNORE;        // ignore if not _configured

		} else if (_use_nav_pvt) {
			_rx_state = UBX_RXMSG_DISABLE;        // disable if using NAV-PVT instead
		}

		break;

	case UBX_MSG_NAV_DOP:
		if (_rx_payload_length != sizeof(ubx_payload_rx_nav_dop_t)) {
			_rx_state = UBX_RXMSG_ERROR_LENGTH;

		} else if (!_configured) {
			_rx_state = UBX_RXMSG_IGNORE;        // ignore if not _configured

		}

		break;

	case UBX_MSG_NAV_TIMEUTC:
		if (_rx_payload_length != sizeof(ubx_payload_rx_nav_timeutc_t)) {
			_rx_state = UBX_RXMSG_ERROR_LENGTH;

		} else if (!_configured) {
			_rx_state = UBX_RXMSG_IGNORE;        // ignore if not _configured

		} else if (_use_nav_pvt) {
			_rx_state = UBX_RXMSG_DISABLE;        // disable if using NAV-PVT instead
		}

		break;

	case UBX_MSG_NAV_SVINFO:
		if (_satellite_info == nullptr) {
			_rx_state = UBX_RXMSG_DISABLE;        // disable if sat info not requested

		} else if (!_configured) {
			_rx_state = UBX_RXMSG_IGNORE;        // ignore if not _configured

		} else {
			memset(_satellite_info, 0, sizeof(*_satellite_info));        // initialize sat info
		}

		break;

	case UBX_MSG_NAV_SVIN:
		if (_rx_payload_length != sizeof(ubx_payload_rx_nav_svin_t)) {
			_rx_state = UBX_RXMSG_ERROR_LENGTH;

		} else if (!_configured) {
			_rx_state = UBX_RXMSG_IGNORE;        // ignore if not _configured

		}

		break;

	case UBX_MSG_NAV_VELNED:
		if (_rx_payload_length != sizeof(ubx_payload_rx_nav_velned_t)) {
			_rx_state = UBX_RXMSG_ERROR_LENGTH;

		} else if (!_configured) {
			_rx_state = UBX_RXMSG_IGNORE;        // ignore if not _configured

		} else if (_use_nav_pvt) {
			_rx_state = UBX_RXMSG_DISABLE;        // disable if using NAV-PVT instead
		}

		break;

	case UBX_MSG_MON_VER:
		break;		// unconditionally handle this message

	case UBX_MSG_MON_HW:
		if ((_rx_payload_length != sizeof(ubx_payload_rx_mon_hw_ubx6_t))	/* u-blox 6 msg format */
		    && (_rx_payload_length != sizeof(ubx_payload_rx_mon_hw_ubx7_t))) {	/* u-blox 7+ msg format */
			_rx_state = UBX_RXMSG_ERROR_LENGTH;

		} else if (!_configured) {
			_rx_state = UBX_RXMSG_IGNORE;        // ignore if not _configured
		}

		break;

	case UBX_MSG_ACK_ACK:
		if (_rx_payload_length != sizeof(ubx_payload_rx_ack_ack_t)) {
			_rx_state = UBX_RXMSG_ERROR_LENGTH;

		} else if (_configured) {
			_rx_state = UBX_RXMSG_IGNORE;        // ignore if _configured
		}

		break;

	case UBX_MSG_ACK_NAK:
		if (_rx_payload_length != sizeof(ubx_payload_rx_ack_nak_t)) {
			_rx_state = UBX_RXMSG_ERROR_LENGTH;

		} else if (_configured) {
			_rx_state = UBX_RXMSG_IGNORE;        // ignore if _configured
		}

		break;

	default:
		_rx_state = UBX_RXMSG_DISABLE;	// disable all other messages
		break;
	}

	switch (_rx_state) {
	case UBX_RXMSG_HANDLE:	// handle message
	case UBX_RXMSG_IGNORE:	// ignore message but don't report error
		ret = 0;
		break;

	case UBX_RXMSG_DISABLE:	// disable unexpected messages
		printf("ubx msg 0x%04x len %u unexpected", SWAP16((unsigned)_rx_msg), (unsigned)_rx_payload_length);

		{
			gps_abstime t = gps_absolute_time();

			if (t > _disable_cmd_last + DISABLE_MSG_INTERVAL) {
				/* don't attempt for every message to disable, some might not be disabled */
				_disable_cmd_last = t;
				printf("ubx disabling msg 0x%04x", SWAP16((unsigned)_rx_msg));

				if (!configureMessageRate(_rx_msg, 0)) {
					ret = -1;
				}
			}
		}

		ret = -1;	// return error, abort handling this message
		break;

	case UBX_RXMSG_ERROR_LENGTH:	// error: invalid length
		printf("ubx msg 0x%04x invalid len %u", SWAP16((unsigned)_rx_msg), (unsigned)_rx_payload_length);
		ret = -1;	// return error, abort handling this message
		break;

	default:	// invalid message state
		printf("ubx internal err1");
		ret = -1;	// return error, abort handling this message
		break;
	}

	return ret;
}

/**
 * Add NAV-SVINFO payload rx byte: -1 = error, 0 = ok, 1 = payload completed
 */
int UBXM8P::payloadRxAddNavSvinfo(const uint8_t b)
{
	int ret = 0;
	uint8_t *p_buf = (uint8_t *)&_buf;

	if (_rx_payload_index < sizeof(ubx_payload_rx_nav_svinfo_part1_t)) {
		// Fill Part 1 buffer
		p_buf[_rx_payload_index] = b;

	} else {
		if (_rx_payload_index == sizeof(ubx_payload_rx_nav_svinfo_part1_t)) {
			// Part 1 complete: decode Part 1 buffer
			_satellite_info->count = MIN(_buf.payload_rx_nav_svinfo_part1.numCh, satellite_info_s::SAT_INFO_MAX_SATELLITES);
			printf("SVINFO len %u  numCh %u", (unsigned)_rx_payload_length,
					 (unsigned)_buf.payload_rx_nav_svinfo_part1.numCh);
		}

		if (_rx_payload_index < sizeof(ubx_payload_rx_nav_svinfo_part1_t) + _satellite_info->count * sizeof(
			    ubx_payload_rx_nav_svinfo_part2_t)) {
			// Still room in _satellite_info: fill Part 2 buffer
			unsigned buf_index = (_rx_payload_index - sizeof(ubx_payload_rx_nav_svinfo_part1_t)) % sizeof(
						     ubx_payload_rx_nav_svinfo_part2_t);
			p_buf[buf_index] = b;

			if (buf_index == sizeof(ubx_payload_rx_nav_svinfo_part2_t) - 1) {
				// Part 2 complete: decode Part 2 buffer
				unsigned sat_index = (_rx_payload_index - sizeof(ubx_payload_rx_nav_svinfo_part1_t)) / sizeof(
							     ubx_payload_rx_nav_svinfo_part2_t);
				_satellite_info->used[sat_index]	= (uint8_t)(_buf.payload_rx_nav_svinfo_part2.flags & 0x01);
				_satellite_info->snr[sat_index]		= (uint8_t)(_buf.payload_rx_nav_svinfo_part2.cno);
				_satellite_info->elevation[sat_index]	= (uint8_t)(_buf.payload_rx_nav_svinfo_part2.elev);
				_satellite_info->azimuth[sat_index]	= (uint8_t)((float)_buf.payload_rx_nav_svinfo_part2.azim * 255.0f / 360.0f);
				_satellite_info->svid[sat_index]	= (uint8_t)(_buf.payload_rx_nav_svinfo_part2.svid);
				printf("SVINFO #%02u  used %u  snr %3u  elevation %3u  azimuth %3u  svid %3u",
						 (unsigned)sat_index + 1,
						 (unsigned)_satellite_info->used[sat_index],
						 (unsigned)_satellite_info->snr[sat_index],
						 (unsigned)_satellite_info->elevation[sat_index],
						 (unsigned)_satellite_info->azimuth[sat_index],
						 (unsigned)_satellite_info->svid[sat_index]
						);
			}
		}
	}

	if (++_rx_payload_index >= _rx_payload_length) {
		ret = 1;	// payload received completely
	}

	return ret;
}

/**
 * Add MON-VER payload rx byte: -1 = error, 0 = ok, 1 = payload completed
 */
int UBXM8P::payloadRxAddMonVer(const uint8_t b)
{
	int ret = 0;
	uint8_t *p_buf = (uint8_t *)&_buf;

	if (_rx_payload_index < sizeof(ubx_payload_rx_mon_ver_part1_t)) {
		// Fill Part 1 buffer
		p_buf[_rx_payload_index] = b;

	} else {
		if (_rx_payload_index == sizeof(ubx_payload_rx_mon_ver_part1_t)) {
			// Part 1 complete: decode Part 1 buffer and calculate hash for SW&HW version strings
			_ubx_version = fnv1_32_str(_buf.payload_rx_mon_ver_part1.swVersion, FNV1_32_INIT);
			_ubx_version = fnv1_32_str(_buf.payload_rx_mon_ver_part1.hwVersion, _ubx_version);
			printf("VER hash 0x%08x", _ubx_version);
			printf("VER hw  \"%10s\"", _buf.payload_rx_mon_ver_part1.hwVersion);
			printf("VER sw  \"%30s\"", _buf.payload_rx_mon_ver_part1.swVersion);
		}

		// fill Part 2 buffer
		unsigned buf_index = (_rx_payload_index - sizeof(ubx_payload_rx_mon_ver_part1_t)) % sizeof(
					     ubx_payload_rx_mon_ver_part2_t);
		p_buf[buf_index] = b;

		if (buf_index == sizeof(ubx_payload_rx_mon_ver_part2_t) - 1) {
			// Part 2 complete: decode Part 2 buffer
			printf("VER ext \" %30s\"", _buf.payload_rx_mon_ver_part2.extension);
		}
	}

	if (++_rx_payload_index >= _rx_payload_length) {
		ret = 1;	// payload received completely
	}

	return ret;
}

/**
 * Add payload rx byte: -1 = error, 0 = ok, 1 = payload completed
 */
int UBXM8P::payloadRxAdd(const uint8_t b)
{
	int ret = 0;
	uint8_t *p_buf = (uint8_t *)&_buf;

	p_buf[_rx_payload_index] = b;

	if (++_rx_payload_index >= _rx_payload_length) {
		ret = 1;	// payload received completely
	}

	return ret;
}

/**
 * Finish payload rx - 0 = no message handled, 1 = message handled, 2 = sat info message handled
 */
int UBXM8P::payloadRxDone()
{
	int ret = 0;

	// return if no message handled
	if (_rx_state != UBX_RXMSG_HANDLE) {
		return ret;
	}

	// handle message
	switch (_rx_msg) {

	case UBX_MSG_NAV_PVT:
		printf("Rx NAV-PVT");

		//Check if position fix flag is good
		if ((_buf.payload_rx_nav_pvt.flags & UBX_RX_NAV_PVT_FLAGS_GNSSFIXOK) == 1) {
			_gps_position->fix_type		 = _buf.payload_rx_nav_pvt.fixType;

			if (_buf.payload_rx_nav_pvt.flags & UBX_RX_NAV_PVT_FLAGS_DIFFSOLN) {
				_gps_position->fix_type = 4; //DGPS
			}

			uint8_t carr_soln = _buf.payload_rx_nav_pvt.flags >> 6;

			if (carr_soln == 1) {
				_gps_position->fix_type = 5; //Float RTK

			} else if (carr_soln == 2) {
				_gps_position->fix_type = 6; //Fixed RTK
			}

			_gps_position->vel_ned_valid = true;

		} else {
			_gps_position->fix_type		 = 0;
			_gps_position->vel_ned_valid = false;
		}

		_gps_position->satellites_used	= _buf.payload_rx_nav_pvt.numSV;

		_gps_position->lat		= _buf.payload_rx_nav_pvt.lat;
		_gps_position->lon		= _buf.payload_rx_nav_pvt.lon;
		_gps_position->alt		= _buf.payload_rx_nav_pvt.hMSL;
		_gps_position->alt_ellipsoid	= _buf.payload_rx_nav_pvt.height;

		_gps_position->eph		= (float)_buf.payload_rx_nav_pvt.hAcc * 1e-3f;
		_gps_position->epv		= (float)_buf.payload_rx_nav_pvt.vAcc * 1e-3f;
		_gps_position->s_variance_m_s	= (float)_buf.payload_rx_nav_pvt.sAcc * 1e-3f;

		_gps_position->vel_m_s		= (float)_buf.payload_rx_nav_pvt.gSpeed * 1e-3f;

		_gps_position->vel_n_m_s	= (float)_buf.payload_rx_nav_pvt.velN * 1e-3f;
		_gps_position->vel_e_m_s	= (float)_buf.payload_rx_nav_pvt.velE * 1e-3f;
		_gps_position->vel_d_m_s	= (float)_buf.payload_rx_nav_pvt.velD * 1e-3f;

		_gps_position->cog_rad		= (float)_buf.payload_rx_nav_pvt.headMot * M_DEG_TO_RAD_F * 1e-5f;
		_gps_position->c_variance_rad	= (float)_buf.payload_rx_nav_pvt.headAcc * M_DEG_TO_RAD_F * 1e-5f;

		//Check if time and date fix flags are good
		if ((_buf.payload_rx_nav_pvt.valid & UBX_RX_NAV_PVT_VALID_VALIDDATE)
		    && (_buf.payload_rx_nav_pvt.valid & UBX_RX_NAV_PVT_VALID_VALIDTIME)
		    && (_buf.payload_rx_nav_pvt.valid & UBX_RX_NAV_PVT_VALID_FULLYRESOLVED)) {
			/* convert to unix timestamp */
			struct tm timeinfo;
			memset(&timeinfo, 0, sizeof(timeinfo));
			timeinfo.tm_year	= _buf.payload_rx_nav_pvt.year - 1900;
			timeinfo.tm_mon		= _buf.payload_rx_nav_pvt.month - 1;
			timeinfo.tm_mday	= _buf.payload_rx_nav_pvt.day;
			timeinfo.tm_hour	= _buf.payload_rx_nav_pvt.hour;
			timeinfo.tm_min		= _buf.payload_rx_nav_pvt.min;
			timeinfo.tm_sec		= _buf.payload_rx_nav_pvt.sec;

#ifndef NO_MKTIME
			time_t epoch = mktime(&timeinfo);

			if (epoch > GPS_EPOCH_SECS) {
				// FMUv2+ boards have a hardware RTC, but GPS helps us to configure it
				// and control its drift. Since we rely on the HRT for our monotonic
				// clock, updating it from time to time is safe.

				timespec ts;
				memset(&ts, 0, sizeof(ts));
				ts.tv_sec = epoch;
				ts.tv_nsec = _buf.payload_rx_nav_pvt.nano;

				setClock(ts);

				_gps_position->time_utc_usec = static_cast<uint64_t>(epoch) * 1000000ULL;
				_gps_position->time_utc_usec += _buf.payload_rx_nav_pvt.nano / 1000;

			} else {
				_gps_position->time_utc_usec = 0;
			}

#else
			_gps_position->time_utc_usec = 0;
#endif
		}

		_gps_position->timestamp = gps_absolute_time();
		_last_timestamp_time = _gps_position->timestamp;

		_rate_count_vel++;
		_rate_count_lat_lon++;

		_got_posllh = true;
		_got_velned = true;

		ret = 1;
		break;

	case UBX_MSG_INF_DEBUG:
	case UBX_MSG_INF_NOTICE: {
			uint8_t *p_buf = (uint8_t *)&_buf;
			p_buf[_rx_payload_length] = 0;
			printf("ubx msg: %s", p_buf);
		}
		break;

	case UBX_MSG_INF_ERROR:
	case UBX_MSG_INF_WARNING: {
			uint8_t *p_buf = (uint8_t *)&_buf;
			p_buf[_rx_payload_length] = 0;
			printf("ubx msg: %s", p_buf);
		}
		break;

	case UBX_MSG_NAV_POSLLH:
		printf("Rx NAV-POSLLH");

		_gps_position->lat	= _buf.payload_rx_nav_posllh.lat;
		_gps_position->lon	= _buf.payload_rx_nav_posllh.lon;
		_gps_position->alt	= _buf.payload_rx_nav_posllh.hMSL;
		_gps_position->eph	= (float)_buf.payload_rx_nav_posllh.hAcc * 1e-3f; // from mm to m
		_gps_position->epv	= (float)_buf.payload_rx_nav_posllh.vAcc * 1e-3f; // from mm to m
		_gps_position->alt_ellipsoid = _buf.payload_rx_nav_posllh.height;

		_gps_position->timestamp = gps_absolute_time();

		_rate_count_lat_lon++;
		_got_posllh = true;

		ret = 1;
		break;

	case UBX_MSG_NAV_SOL:
		printf("Rx NAV-SOL");

		_gps_position->fix_type		= _buf.payload_rx_nav_sol.gpsFix;
		_gps_position->s_variance_m_s	= (float)_buf.payload_rx_nav_sol.sAcc * 1e-2f;	// from cm to m
		_gps_position->satellites_used	= _buf.payload_rx_nav_sol.numSV;

		ret = 1;
		break;

	case UBX_MSG_NAV_DOP:
		printf("Rx NAV-DOP");

		_gps_position->hdop		= _buf.payload_rx_nav_dop.hDOP * 0.01f;	// from cm to m
		_gps_position->vdop		= _buf.payload_rx_nav_dop.vDOP * 0.01f;	// from cm to m

		ret = 1;
		break;

	case UBX_MSG_NAV_TIMEUTC:
		printf("Rx NAV-TIMEUTC");

		if (_buf.payload_rx_nav_timeutc.valid & UBX_RX_NAV_TIMEUTC_VALID_VALIDUTC) {
			// convert to unix timestamp
			struct tm timeinfo;
			memset(&timeinfo, 0, sizeof(tm));
			timeinfo.tm_year	= _buf.payload_rx_nav_timeutc.year - 1900;
			timeinfo.tm_mon		= _buf.payload_rx_nav_timeutc.month - 1;
			timeinfo.tm_mday	= _buf.payload_rx_nav_timeutc.day;
			timeinfo.tm_hour	= _buf.payload_rx_nav_timeutc.hour;
			timeinfo.tm_min		= _buf.payload_rx_nav_timeutc.min;
			timeinfo.tm_sec		= _buf.payload_rx_nav_timeutc.sec;
			timeinfo.tm_isdst	= 0;
#ifndef NO_MKTIME
			time_t epoch = mktime(&timeinfo);

			// only set the time if it makes sense

			if (epoch > GPS_EPOCH_SECS) {
				// FMUv2+ boards have a hardware RTC, but GPS helps us to configure it
				// and control its drift. Since we rely on the HRT for our monotonic
				// clock, updating it from time to time is safe.

				timespec ts;
				memset(&ts, 0, sizeof(ts));
				ts.tv_sec = epoch;
				ts.tv_nsec = _buf.payload_rx_nav_timeutc.nano;

				setClock(ts);

				_gps_position->time_utc_usec = static_cast<uint64_t>(epoch) * 1000000ULL;
				_gps_position->time_utc_usec += _buf.payload_rx_nav_timeutc.nano / 1000;

			} else {
				_gps_position->time_utc_usec = 0;
			}

#else
			_gps_position->time_utc_usec = 0;
#endif
		}

		_last_timestamp_time = gps_absolute_time();

		ret = 1;
		break;

	case UBX_MSG_NAV_SVINFO:
		printf("Rx NAV-SVINFO");

		// _satellite_info already populated by payload_rx_add_svinfo(), just add a timestamp
		_satellite_info->timestamp = gps_absolute_time();

		ret = 2;
		break;

	case UBX_MSG_NAV_SVIN:
		printf("Rx NAV-SVIN");
		{
			ubx_payload_rx_nav_svin_t &svin = _buf.payload_rx_nav_svin;

			printf("Survey-in status: %is cur accuracy: %imm nr obs: %i valid: %i active: %i",
				  svin.dur, svin.meanAcc / 10, svin.obs, (int)svin.valid, (int)svin.active);

			SurveyInStatus status;
			status.duration = svin.dur;
			status.mean_accuracy = svin.meanAcc / 10;
			status.flags = (svin.valid & 1) | ((svin.active & 1) << 1);
			surveyInStatus(status);

			if (svin.valid == 1 && svin.active == 0) {
				/* We now switch to 1 Hz update rate, which is enough for RTCM output.
				 * For the survey-in, we still want 5 Hz, because this speeds up the process */
				memset(&_buf.payload_tx_cfg_rate, 0, sizeof(_buf.payload_tx_cfg_rate));
				_buf.payload_tx_cfg_rate.measRate	= 1000;
				_buf.payload_tx_cfg_rate.navRate	= UBX_TX_CFG_RATE_NAVRATE;
				_buf.payload_tx_cfg_rate.timeRef	= UBX_TX_CFG_RATE_TIMEREF;

				if (!sendMessage(UBX_MSG_CFG_RATE, (uint8_t *)&_buf, sizeof(_buf.payload_tx_cfg_rate))) {
					return -1;
				}

				//according to the spec, we should receive an (N)ACK here, but we don't
//				decodeInit();
//				if (waitForAck(UBX_MSG_CFG_RATE, UBX_CONFIG_TIMEOUT, true) < 0) {
//					return -1;
//				}

				configureMessageRate(UBX_MSG_NAV_SVIN, 0);

				/* enable RTCM3 messages */
				if (!configureMessageRate(UBX_MSG_RTCM3_1005, 1)) {
					return -1;
				}

				if (!configureMessageRate(UBX_MSG_RTCM3_1077, 1)) {
					return -1;
				}

				if (!configureMessageRate(UBX_MSG_RTCM3_1087, 1)) {
					return -1;
				}
			}
		}

		ret = 1;
		break;

	case UBX_MSG_NAV_VELNED:
		printf("Rx NAV-VELNED");

		_gps_position->vel_m_s		= (float)_buf.payload_rx_nav_velned.speed * 1e-2f;
		_gps_position->vel_n_m_s	= (float)_buf.payload_rx_nav_velned.velN * 1e-2f; /* NED NORTH velocity */
		_gps_position->vel_e_m_s	= (float)_buf.payload_rx_nav_velned.velE * 1e-2f; /* NED EAST velocity */
		_gps_position->vel_d_m_s	= (float)_buf.payload_rx_nav_velned.velD * 1e-2f; /* NED DOWN velocity */
		_gps_position->cog_rad		= (float)_buf.payload_rx_nav_velned.heading * M_DEG_TO_RAD_F * 1e-5f;
		_gps_position->c_variance_rad	= (float)_buf.payload_rx_nav_velned.cAcc * M_DEG_TO_RAD_F * 1e-5f;
		_gps_position->vel_ned_valid	= true;

		_rate_count_vel++;
		_got_velned = true;

		ret = 1;
		break;

	case UBX_MSG_MON_VER:
		printf("Rx MON-VER");

		ret = 1;
		break;

	case UBX_MSG_MON_HW:
		printf("Rx MON-HW");

		switch (_rx_payload_length) {

		case sizeof(ubx_payload_rx_mon_hw_ubx6_t):	/* u-blox 6 msg format */
			_gps_position->noise_per_ms		= _buf.payload_rx_mon_hw_ubx6.noisePerMS;
			_gps_position->jamming_indicator	= _buf.payload_rx_mon_hw_ubx6.jamInd;

			ret = 1;
			break;

		case sizeof(ubx_payload_rx_mon_hw_ubx7_t):	/* u-blox 7+ msg format */
			_gps_position->noise_per_ms		= _buf.payload_rx_mon_hw_ubx7.noisePerMS;
			_gps_position->jamming_indicator	= _buf.payload_rx_mon_hw_ubx7.jamInd;

			ret = 1;
			break;

		default:		// unexpected payload size:
			ret = 0;	// don't handle message
			break;
		}

		break;

	case UBX_MSG_ACK_ACK:
		printf("Rx ACK-ACK");

		if ((_ack_state == UBX_ACK_WAITING) && (_buf.payload_rx_ack_ack.msg == _ack_waiting_msg)) {
			_ack_state = UBX_ACK_GOT_ACK;
		}

		ret = 1;
		break;

	case UBX_MSG_ACK_NAK:
		printf("Rx ACK-NAK");

		if ((_ack_state == UBX_ACK_WAITING) && (_buf.payload_rx_ack_ack.msg == _ack_waiting_msg)) {
			_ack_state = UBX_ACK_GOT_NAK;
		}

		ret = 1;
		break;

	default:
		break;
	}

	if (ret > 0) {
		_gps_position->timestamp_time_relative = (int32_t)(_last_timestamp_time - _gps_position->timestamp);
	}

	return ret;
}

uint32_t UBXM8P::fnv1_32_str(uint8_t *str, uint32_t hval)
{
	uint8_t *s = str;

	/*
	 * FNV-1 hash each octet in the buffer
	 */
	while (*s) {

		/* multiply by the 32 bit FNV magic prime mod 2^32 */
#if defined(NO_FNV_GCC_OPTIMIZATION)
		hval *= FNV1_32_PRIME;
#else
		hval += (hval << 1) + (hval << 4) + (hval << 7) + (hval << 8) + (hval << 24);
#endif

		/* xor the bottom with the current octet */
		hval ^= (uint32_t) * s++;
	}

	/* return our new hash value */
	return hval;
}

void UBXM8P::setSurveyInSpecs(uint32_t survey_in_acc_limit, uint32_t survey_in_min_dur)
{
	_survey_in_acc_limit = survey_in_acc_limit;
	_survey_in_min_dur = survey_in_min_dur;
}
