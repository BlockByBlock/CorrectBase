/****************************************************************************
 *   Copyright (c) 2012-2014 PX4 Development Team. All rights reserved.
 *
 *   Author: Yeap Bing Cheng <ybingcheng@gmail.com> 
 *   (rework from px4 gpsdriver for m8p only and non-mavlink use)
 * 
 ****************************************************************************/
#ifndef UBXM8P_H_
#define UBXM8P_H_

#include <stdint.h>
#include <stdio.h>
#include "rtcm.h"

/* MON_VER from u-blox modules can be ~190 bytes */
#ifndef GPS_READ_BUFFER_SIZE
#define GPS_READ_BUFFER_SIZE 250
#endif

#define UBX_CONFIG_TIMEOUT	200		// ms, timeout for waiting ACK
#define UBX_PACKET_TIMEOUT	2		// ms, if now data during this delay assume that full update received

#define UBX_SYNC1 0xB5
#define UBX_SYNC2 0x62

/* Message Classes */
#define UBX_CLASS_NAV		0x01
#define UBX_CLASS_INF		0x04
#define UBX_CLASS_ACK		0x05
#define UBX_CLASS_CFG		0x06
#define UBX_CLASS_MON		0x0A
#define UBX_CLASS_RTCM3	0xF5 /**< This is undocumented (?) */

/* Message IDs */
#define UBX_ID_NAV_POSLLH	0x02
#define UBX_ID_NAV_DOP		0x04
#define UBX_ID_NAV_SOL		0x06
#define UBX_ID_NAV_PVT		0x07
#define UBX_ID_NAV_VELNED	0x12
#define UBX_ID_NAV_TIMEUTC	0x21
#define UBX_ID_NAV_SVINFO	0x30
#define UBX_ID_NAV_SAT		0x35
#define UBX_ID_NAV_SVIN  	0x3B
#define UBX_ID_NAV_RELPOSNED  	0x3C
#define UBX_ID_INF_DEBUG  	0x04
#define UBX_ID_INF_ERROR  	0x00
#define UBX_ID_INF_NOTICE  	0x02
#define UBX_ID_INF_WARNING 	0x01
#define UBX_ID_ACK_NAK		0x00
#define UBX_ID_ACK_ACK		0x01
#define UBX_ID_CFG_PRT		0x00
#define UBX_ID_CFG_MSG		0x01
#define UBX_ID_CFG_RATE		0x08
#define UBX_ID_CFG_CFG		0x09
#define UBX_ID_CFG_NAV5		0x24
#define UBX_ID_CFG_SBAS		0x16
#define UBX_ID_CFG_TMODE3	0x71
#define UBX_ID_MON_VER		0x04
#define UBX_ID_MON_HW		0x09

#define UBX_ID_RTCM3_1005	0x05	/**< Stationary RTK reference station ARP */
#define UBX_ID_RTCM3_1074	0x4A	/**< GPS MSM4 */
#define UBX_ID_RTCM3_1077	0x4D	/**< GPS MSM7 */
#define UBX_ID_RTCM3_1084	0x54	/**< GLONASS MSM4 */
#define UBX_ID_RTCM3_1087	0x57	/**< GLONASS MSM7 */
#define UBX_ID_RTCM3_1124	0x7C	/**< BeiDou MSM4 */
#define UBX_ID_RTCM3_1127	0x7F	/**< BeiDou MSM7 */
#define UBX_ID_RTCM3_1230	0xE6	/**< GLONASS code-phase biases */
#define UBX_ID_RTCM3_4072	0xFE	/**< Reference station PVT (u-blox proprietary RTCM Message) */

#define SWAP16(X)	((((X) >>  8) & 0x00ff) | (((X) << 8) & 0xff00))

/* Message Classes & IDs */
#define UBX_MSG_NAV_POSLLH	((UBX_CLASS_NAV) | UBX_ID_NAV_POSLLH << 8)
#define UBX_MSG_NAV_SOL		((UBX_CLASS_NAV) | UBX_ID_NAV_SOL << 8)
#define UBX_MSG_NAV_DOP		((UBX_CLASS_NAV) | UBX_ID_NAV_DOP << 8)
#define UBX_MSG_NAV_PVT		((UBX_CLASS_NAV) | UBX_ID_NAV_PVT << 8)
#define UBX_MSG_NAV_VELNED	((UBX_CLASS_NAV) | UBX_ID_NAV_VELNED << 8)
#define UBX_MSG_NAV_TIMEUTC	((UBX_CLASS_NAV) | UBX_ID_NAV_TIMEUTC << 8)
#define UBX_MSG_NAV_SVINFO	((UBX_CLASS_NAV) | UBX_ID_NAV_SVINFO << 8)
#define UBX_MSG_NAV_SAT	((UBX_CLASS_NAV) | UBX_ID_NAV_SAT << 8)
#define UBX_MSG_NAV_SVIN	((UBX_CLASS_NAV) | UBX_ID_NAV_SVIN << 8)
#define UBX_MSG_NAV_RELPOSNED	((UBX_CLASS_NAV) | UBX_ID_NAV_RELPOSNED << 8)
#define UBX_MSG_INF_DEBUG	((UBX_CLASS_INF) | UBX_ID_INF_DEBUG << 8)
#define UBX_MSG_INF_ERROR	((UBX_CLASS_INF) | UBX_ID_INF_ERROR << 8)
#define UBX_MSG_INF_NOTICE	((UBX_CLASS_INF) | UBX_ID_INF_NOTICE << 8)
#define UBX_MSG_INF_WARNING	((UBX_CLASS_INF) | UBX_ID_INF_WARNING << 8)
#define UBX_MSG_ACK_NAK		((UBX_CLASS_ACK) | UBX_ID_ACK_NAK << 8)
#define UBX_MSG_ACK_ACK		((UBX_CLASS_ACK) | UBX_ID_ACK_ACK << 8)
#define UBX_MSG_CFG_PRT		((UBX_CLASS_CFG) | UBX_ID_CFG_PRT << 8)
#define UBX_MSG_CFG_MSG		((UBX_CLASS_CFG) | UBX_ID_CFG_MSG << 8)
#define UBX_MSG_CFG_RATE	((UBX_CLASS_CFG) | UBX_ID_CFG_RATE << 8)
#define UBX_MSG_CFG_CFG		((UBX_CLASS_CFG) | UBX_ID_CFG_CFG << 8)
#define UBX_MSG_CFG_NAV5	((UBX_CLASS_CFG) | UBX_ID_CFG_NAV5 << 8)
#define UBX_MSG_CFG_SBAS	((UBX_CLASS_CFG) | UBX_ID_CFG_SBAS << 8)
#define UBX_MSG_CFG_TMODE3	((UBX_CLASS_CFG) | UBX_ID_CFG_TMODE3 << 8)
#define UBX_MSG_MON_HW		((UBX_CLASS_MON) | UBX_ID_MON_HW << 8)
#define UBX_MSG_MON_VER		((UBX_CLASS_MON) | UBX_ID_MON_VER << 8)
#define UBX_MSG_RTCM3_1005	((UBX_CLASS_RTCM3) | UBX_ID_RTCM3_1005 << 8)
#define UBX_MSG_RTCM3_1077	((UBX_CLASS_RTCM3) | UBX_ID_RTCM3_1077 << 8)
#define UBX_MSG_RTCM3_1087	((UBX_CLASS_RTCM3) | UBX_ID_RTCM3_1087 << 8)

/**** Trace macros, disable for production builds */
#define UBX_TRACE_PARSER(...)	{/*GPS_INFO(__VA_ARGS__);*/}	/* decoding progress in parse_char() */

/* TX CFG-PRT message contents */
#define UBX_TX_CFG_PRT_INPROTOMASK_GPS	((1<<5) | 0x01)	/**< RTCM3 in and UBX in */
#define UBX_TX_CFG_PRT_INPROTOMASK_RTCM	(0x01)	/**< UBX in */
#define UBX_TX_CFG_PRT_OUTPROTOMASK_GPS	(0x01)			/**< UBX out */
#define UBX_TX_CFG_PRT_OUTPROTOMASK_RTCM	((1<<5) | 0x01)		/**< RTCM3 out and UBX out */

/* General: Header */
typedef struct {
	uint8_t		sync1;
	uint8_t		sync2;
	uint16_t	msg;
	uint16_t	length;
} ubx_header_t;

/* General: Checksum */
typedef struct {
	uint8_t		ck_a;
	uint8_t		ck_b;
} ubx_checksum_t ;

/* Tx CFG-PRT */
typedef struct {
	uint8_t		portID;
	uint8_t		reserved0;
	uint16_t	txReady;
	uint32_t	mode;
	uint32_t	baudRate;
	uint16_t	inProtoMask;
	uint16_t	outProtoMask;
	uint16_t	flags;
	uint16_t	reserved5;
} ubx_payload_tx_cfg_prt_t;

/* Tx CFG-CFG */
typedef struct {
	uint32_t	clearMask;	/**< Clear settings */
	uint32_t	saveMask;	/**< Save settings */
	uint32_t	loadMask;	/**< Load settings */
	uint8_t		deviceMask; /**< Storage devices to apply this top */
} ubx_payload_tx_cfg_cfg_t;

/* Decoder state */
typedef enum {
	UBX_DECODE_SYNC1 = 0,
	UBX_DECODE_SYNC2,
	UBX_DECODE_CLASS,
	UBX_DECODE_ID,
	UBX_DECODE_LENGTH1,
	UBX_DECODE_LENGTH2,
	UBX_DECODE_PAYLOAD,
	UBX_DECODE_CHKSUM1,
	UBX_DECODE_CHKSUM2,

	UBX_DECODE_RTCM3
} ubx_decode_state_t;

/* ACK state */
typedef enum {
	UBX_ACK_IDLE = 0,
	UBX_ACK_WAITING,
	UBX_ACK_GOT_ACK,
	UBX_ACK_GOT_NAK
} ubx_ack_state_t;

typedef uint64_t gps_absolute_time;
typedef uint64_t gps_abstime;

enum class GPSCallbackType {
	/**
	 * Read data from device. This is a blocking operation with a timeout.
	 * data1: points to a buffer to be written to. The first sizeof(int) bytes contain the
	 *        timeout in ms when calling the method.
	 * data2: buffer length in bytes. Less bytes than this can be read.
	 * return: num read bytes, 0 on timeout (the method can actually also return 0 before
	 *         the timeout happens).
	 */
	readDeviceData = 0,

	/**
	 * Write data to device
	 * data1: data to be written
	 * data2: number of bytes to write
	 * return: num written bytes
	 */
	writeDeviceData,

	/**
	 * set Baudrate
	 * data1: ignored
	 * data2: baudrate
	 * return: 0 on success
	 */
	setBaudrate,

	/**
	 * Got an RTCM message from the device.
	 * data1: pointer to the message
	 * data2: message length
	 * return: ignored
	 */
	gotRTCMMessage,

	/**
	 * message about current survey-in status
	 * data1: points to a SurveyInStatus struct
	 * data2: ignored
	 * return: ignored
	 */
	surveyInStatus,

	/**
	 * can be used to set the current clock accurately
	 * data1: pointer to a timespec struct
	 * data2: ignored
	 * return: ignored
	 */
	setClock,
};

/** Callback function for platform-specific stuff.
 * data1 and data2 depend on type and user is the custom user-supplied argument.
 * @return <0 on error, >=0 on success (depending on type)
 */
typedef int (*GPSCallbackPtr)(GPSCallbackType type, void *data1, int data2, void *user);

class UBXM8P
{
public:
    /**
     * For setting the mode between Base or Rover
     */
    enum class OutputMode {
        GPS = 0,    // Normal GPS output
        RTCM        // request RTCM output. For base station
    };

    /**
	 * Constructor - WIP
	 * @param interface UART
     * @param gpscallbackptr callback
     * @param callback_user user
     * @param veh_gps_position_s gps_position
     * @param satellite_info_s satellite_info
     * @param dynamic_model 0 Portable, 2 Stationary, 3 Pedestrian, 4 Automotive
	 * @return <0 on error, otherwise a bitset:
	 *         bit 0 set: got gps position update
	 *         bit 1 set: got satellite info update
	 */
    UBXM8P();

    virtual ~UBXM8P();

    /**
	 * Receive and process data
	 * @param timeout [ms]
	 * @return <0 on error, otherwise a bitset:
	 *         bit 0 set: got gps position update
	 *         bit 1 set: got satellite info update
	 */
    int receive(unsigned timeout);

    /**
	 * Configure the device
	 * @param baud Input and output parameter: if set to 0, the baudrate will be automatically detected and set to
	 *             the detected baudrate. If not 0, a fixed baudrate is used.
	 * @return 0 on success, <0 otherwise
	 */
    int configure(unsigned &baud, OutputMode output_mode);

    /**
	 * set survey-in specs for RTK base station setup (for finding an accurate base station position
	 * by averaging the position measurements over time).
	 * @param survey_in_acc_limit minimum accuracy in 0.1mm
	 * @param survey_in_min_dur minimum duration in seconds
	 */
    void setSurveySpecs(uint32_t survey_in_acc_limit, uint32_t survey_in_min_dur);

    /**
	 * set the Baudrate
	 * @param baudrate
	 * @return 0 on success, <0 otherwise
	 */
    int setBaudrate(int baudrate);

    /**
	 * Send a message
	 * @return true on success, false on write error (errno set)
	 */
    bool sendMessage(const uint16_t msg, const uint8_t *payload, const uint16_t length);

    /**
	 * Calculate & add checksum for given buffer
	 */
	void calcChecksum(const uint8_t *buffer, const uint16_t length, ubx_checksum_t *checksum);

protected:
    /**
	 * write to the device
	 * @param buf
	 * @param buf_length
	 * @return num written bytes, -1 on error
	 */
	int write(const void *buf, int buf_length)
	{
		return _callback(GPSCallbackType::writeDeviceData, (void *)buf, buf_length, _callback_user);
	}

    GPSCallbackPtr _callback{nullptr};
    void *_callback_user{};

    /**
	 * read from device
	 * @param buf: pointer to read buffer
	 * @param buf_length: size of read buffer
	 * @param timeout: timeout in ms
	 * @return: 0 for nothing read, or poll timed out
	 *	    < 0 for error
	 *	    > 0 number of bytes read
	 */
	int read(uint8_t *buf, int buf_length, int timeout)
	{
		*((int *)buf) = timeout;
		return _callback(GPSCallbackType::readDeviceData, buf, buf_length, _callback_user);
	}

    /** got an RTCM message from the device */
	void gotRTCMMessage(uint8_t *buf, int buf_length)
	{
		_callback(GPSCallbackType::gotRTCMMessage, buf, buf_length, _callback_user);
	}

private:
    uint32_t _survey_in_acc_limit;
	uint32_t _survey_in_min_dur;
    uint16_t _ack_waiting_msg{0};
    uint16_t _rx_msg{};
    uint16_t _rx_payload_length{};
	uint16_t _rx_payload_index{};
    uint8_t	_rx_ck_a{};
	uint8_t	_rx_ck_b{};

    ubx_decode_state_t	_decode_state{};
    ubx_ack_state_t		_ack_state{UBX_ACK_IDLE};

    bool _configured{false};    // Configuration status
    bool _got_posllh{false};
	bool _got_velned{false};
    
    gps_abstime		_disable_cmd_last{0};
    OutputMode _output_mode{OutputMode::GPS};
    
    RTCMParsing	*_rtcm_parsing{nullptr};

    /**
	 * Wait for message acknowledge
	 */
	int waitForAck(const uint16_t msg, const unsigned timeout, const bool report);

    /**
	 * Parse the binary UBX packet
	 */
	int parseChar(const uint8_t b);

    /**
	 * Reset the parse state machine for a fresh start
	 */
	void decodeInit(void);

    /**
	 * While parsing add every byte (except the sync bytes) to the checksum
	 */
	void addByteToChecksum(const uint8_t);

    /**
	 * Start payload rx
	 */
	int payloadRxInit(void);

    /**
	 * Finish payload rx
	 */
	int payloadRxDone(void);

    /**
	 * Add payload rx byte
	 */
	int payloadRxAdd(const uint8_t b);
	int payloadRxAddNavSvinfo(const uint8_t b);
	int payloadRxAddMonVer(const uint8_t b);

 };

 #endif /* UBXM8P_H_ */
