/****************************************************************************
 *   Copyright (c) 2012-2014 PX4 Development Team. All rights reserved.
 *
 *   Author: Yeap Bing Cheng <ybingcheng@gmail.com> 
 *   (rework from px4 gpsdriver for m8p only and non-mavlink use)
 * 
 ****************************************************************************/
#include <stdint.h>

#ifndef UBXM8P_H_
#define UBXM8P_H_

/* TX CFG-PRT message contents */
#define UBX_TX_CFG_PRT_INPROTOMASK_GPS	((1<<5) | 0x01)	/**< RTCM3 in and UBX in */
#define UBX_TX_CFG_PRT_INPROTOMASK_RTCM	(0x01)	/**< UBX in */
#define UBX_TX_CFG_PRT_OUTPROTOMASK_GPS	(0x01)			/**< UBX out */
#define UBX_TX_CFG_PRT_OUTPROTOMASK_RTCM	((1<<5) | 0x01)		/**< RTCM3 out and UBX out */

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

protected:
    GPSCallbackPtr _callback{nullptr};
    void *_callback_user{};

private:
    uint32_t _survey_in_acc_limit;
	uint32_t _survey_in_min_dur;

    bool _configured{false};    // Configuration status
    OutputMode _output_mode{OutputMode::GPS};
 };

 #endif /* UBXM8P_H_ */
