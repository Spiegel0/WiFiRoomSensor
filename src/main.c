/**
 * \file main.c
 * \brief Provides the reset vector and the main loop
 * \details The main file implements the main application logic. It queries the
 * sensors and responds to any request. If the preprocessor variable
 * USE_AM2303_CHN1 is defined, the second sensor channel will be queried.
 * Similarly, defining the variable USE_WS2801 will enable the LED controller
 * and defining USE_BUTTON_CNT will enable the user input module.
 *
 * \author Michael Spiegel, <michael.h.spiegel@gmail.com>
 *
 * Copyright (C) 2016 Michael Spiegel
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "am2303.h"
#include "esp8266_transceiver.h"
#include "esp8266_session.h"
#include "iec61499_com.h"
#include "system_timer.h"
#include "debug.h"
#include "oscillator.h"
#include "ws2801.h"
#include "button_cnt.h"

#include <avr/io.h>
#include <util/delay.h>
#include <util/atomic.h>
#include <avr/sfr_defs.h>
#include <avr/interrupt.h>

/** \brief Defines possible states of the sensor modules */
typedef enum {
	IDLE, ///< \brief Nothing to do
	READ_AM2303_CHN0, ///< \brief Reads the first channel of the humidity sensor
	READ_AM2303_CHN1, ///< \brief Reads the second channel of the humidity sensor
} main_sensorState_t;

/**
 * \brief The state of the sensor module
 * \brief The variable may be written in an interrupt context. If it is not
 * IDLE, it must not be written outside the callback. Hence, the callback does
 * not need to synchronize the variable access.
 */
static volatile main_sensorState_t main_sensor_state;
/**
 * \brief The last temperature result of channel 0
 * \details The variable can be safely accessed outside an interrupt context if
 * the main_sensor_state is IDLE.
 */
static volatile uint16_t main_am2303_temperature_chn0;
/**
 * \brief The last humidity result of channel 0
 * \details The variable can be safely accessed outside an interrupt context if
 * the main_sensor_state is IDLE.
 */
static volatile uint16_t main_am2303_humidity_chn0;
#ifdef USE_AM2303_CHN1
/**
 * \brief The last temperature result of channel 1
 * \details The variable can be safely accessed outside an interrupt context if
 * the main_sensor_state is IDLE.
 */
static volatile uint16_t main_am2303_temperature_chn1;
/**
 * \brief The last humidity result of channel 1
 * \details The variable can be safely accessed outside an interrupt context if
 * the main_sensor_state is IDLE.
 */
static volatile uint16_t main_am2303_humidity_chn1;
#endif

/** \brief The number of ticks until the am2303 sensors may be read again */
static uint8_t main_am2303_lockedTicks;

/**
 * \brief Encapsulates some of the data belonging to the main module.
 */
static struct {
	/**
	 * \brief Flags which indicate that a given channel needs service
	 * \details The bit number corresponds to the network channel
	 */
	uint8_t requestFlags :4;
	/** \brief Flags which indicate pressed buttons */
	uint8_t buttonFlags :3;
	/** \brief Flag which indicates whether the message buffer is busy */
	int8_t bufferBusy :1;
} main_data;

// Function Prototypes
static void main_init(void);
static void main_timedTick(void);
static void main_tick(void);
static void main_fetchData(void);
void main_recordData(status_t status, uint16_t temperature, uint16_t humidity,
		uint8_t channel);
static void main_sendData(uint8_t channel);
void main_freeReplyBuffer(status_t status);
void main_decodeMessage(status_t status, uint8_t channel, uint8_t size,
		uint8_t rrbID);
#ifdef USE_BUTTON_CNT
void main_handleButtonEvent(int16_t cnt, uint8_t btn);
#endif
#ifdef USE_WS2801
void main_decodeWS2801Command(uint8_t size, uint8_t rrbID);
#endif

int main(void) {

	main_init();

	sei();

	DEBUG_INIT
	;
	DEBUG_PRINT_START(0x00);
	DEBUG_BYTE(0x01);
	DEBUG_BYTE(0x02);
	DEBUG_BYTE(0x04);
	DEBUG_BYTE(0x08);
	DEBUG_BYTE(0x10);
	DEBUG_BYTE(0x20);
	DEBUG_BYTE(0x40);
	DEBUG_BYTE(0x80);

	// Main tasking scheme:
	while (1) {
		esp8266_transc_tick();
		main_tick();

#ifdef USE_BUTTON_CNT
		// Fast timer tick
		if (system_timer_queryFast()) {
			button_cnt_timedFastTick();
		}
#endif

		// Standard timer tick
		if (system_timer_query()) {
			esp8266_session_timedTick();
			main_timedTick();
#ifdef USE_WS2801
			ws2801_timedTick();
#endif
		}
	}

	return 0;
}

/**
 * \brief Initializes the application and every sub module
 * \details It is assumed that interrupts are globally disabled while calling
 * the function.
 */
static void main_init(void) {
	oscillator_init();
	system_timer_init();
#ifdef USE_BUTTON_CNT
	button_cnt_init(main_handleButtonEvent);
#endif
#ifdef USE_WS2801
	ws2801_init();
#endif
	am2303_init();
	esp8266_session_init(main_decodeMessage);
}

/**
 * \brief Maintains the \ref main_am2303_lockedTicks variable
 */
static void main_timedTick(void) {
	if (main_am2303_lockedTicks > 0) {
		main_am2303_lockedTicks--;
	}
}

/**
 * \brief Implements the network task which initiates new sending operations.
 * \details The task checks the sensor status and the request flags. If recent
 * data is available it assembles the message and send it.
 */
static void main_tick(void) {
	main_sensorState_t sensorState;
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		sensorState = main_sensor_state;
	}

	if (main_data.buttonFlags && !main_data.bufferBusy) {

		// Push data initiated by the user
		DEBUG_PRINT(0x03, main_data.buttonFlags);
		main_sendData(0xFF);
		main_data.buttonFlags = 0;

	} else if (sensorState == IDLE && main_data.requestFlags) {

		// Data requested by a connected client
		DEBUG_PRINT(0x01, main_data.requestFlags);

		if (main_am2303_lockedTicks == 0) {
			main_fetchData();
		} else if (!main_data.bufferBusy) {
			uint8_t chn = 0;
			// Determine channel number
			while (!(main_data.requestFlags & (1 << chn)) || chn >= 3) {
				chn++;
			}
			main_data.requestFlags &= ~(1 << chn);

			main_sendData(chn);
		}
	}

}

/**
 * \brief Assembles a data message and initiates the transmission
 * \details It is assumed that the bufferBusy flag is cleared before calling the
 * function. Any transmission error will be ignored. The connected client has to
 * initiate a re-transmission if the server fails.
 * \param channel A valid channel identifier which specifies the destination
 * channel or 0xFF to send a broadcast message
 */
static void main_sendData(uint8_t channel) {
	static uint8_t replyBuffer[4 * IEC61499_COM_INT_ENC_SIZE];
	uint8_t nextIndex = 0;
	status_t status;

	// Encodes the values
	iec61499_com_encodeINT(replyBuffer,
			sizeof(replyBuffer) / sizeof(replyBuffer[0]), &nextIndex,
			main_am2303_temperature_chn0);
	iec61499_com_encodeINT(replyBuffer,
			sizeof(replyBuffer) / sizeof(replyBuffer[0]), &nextIndex,
			main_am2303_humidity_chn0);
#ifdef USE_AM2303_CHN1
	iec61499_com_encodeINT(replyBuffer,
			sizeof(replyBuffer) / sizeof(replyBuffer[0]), &nextIndex,
			main_am2303_temperature_chn1);
	iec61499_com_encodeINT(replyBuffer,
			sizeof(replyBuffer) / sizeof(replyBuffer[0]), &nextIndex,
			main_am2303_humidity_chn1);
#endif
#ifdef USE_BUTTON_CNT
	iec61499_com_encodeINT(replyBuffer,
			sizeof(replyBuffer) / sizeof(replyBuffer[0]), &nextIndex,
			button_cnt_getCounter());
	iec61499_com_encodeINT(replyBuffer,
			sizeof(replyBuffer) / sizeof(replyBuffer[0]), &nextIndex,
			(int16_t) main_data.buttonFlags);
#endif

	// Sends the message
	if (channel == 0xFF) {
		status = esp8266_session_sendToAll(replyBuffer, nextIndex,
				main_freeReplyBuffer);
	} else {
		status = esp8266_session_send(channel, replyBuffer, nextIndex,
				main_freeReplyBuffer);
	}

	if (status == success) {
		main_data.bufferBusy = 1;
	}

}

/**
 * \brief Clears the busy flag of the reply buffer
 * \param status The status of the previously performed operation. Since
 * re-transmission is delegated to the client, any error will be ignored.
 */
void main_freeReplyBuffer(status_t status) {
	main_data.bufferBusy = 0;
}

/**
 * \brief Decodes the previously received message and takes corresponding
 * actions
 * \details Any message with a status code other than success will be ignored.
 * Currently, every received message will result in a reply request. It is
 * assumed that every given parameter is valid. See
 * \ref esp8266_transc_messageReceived for a detailed description of the
 * parameters
 * \see esp8266_transc_messageReceived
 */
void main_decodeMessage(status_t status, uint8_t channel, uint8_t size,
		uint8_t rrbID) {
	if (status == success) {
		main_data.requestFlags |= (1 << channel);

#ifdef USE_WS2801
		main_decodeWS2801Command(size, rrbID);
#endif
	}
}

#ifdef USE_WS2801
/**
 * \brief Tries to decode the WS2801 command in the receive buffer
 * \details If the command was parsed successfully, it will be executed
 * immediately. The first USINT corresponds to the pixel number, the next three
 * USINT values denote the RGB value and the BOOL flag indicates whether to
 * update the values.
 * \param size The number of received bytes
 * \param rrbID The round robin buffer ID of the first byte.
 */
void main_decodeWS2801Command(uint8_t size, uint8_t rrbID) {
	status_t err;
	uint8_t nextIndex = 0;
	uint8_t pos = 0, rVal = 0, gVal = 0, bVal = 0, update = 0;

	err = iec61499_com_decodeUSINT(rrbID, size, &nextIndex, &pos);
	IEC6199_COM_TRY(err,
			iec61499_com_decodeUSINT(rrbID, size, &nextIndex, &rVal));
	IEC6199_COM_TRY(err,
			iec61499_com_decodeUSINT(rrbID, size, &nextIndex, &gVal));
	IEC6199_COM_TRY(err,
			iec61499_com_decodeUSINT(rrbID, size, &nextIndex, &bVal));
	IEC6199_COM_TRY(err,
			iec61499_com_decodeBOOL(rrbID, size, &nextIndex, &update));

	DEBUG_PRINT(0x03, err);

	if (err == success) {
		(void) ws2801_setValue(pos, rVal, gVal, bVal);
		if (update) {
			(void) ws2801_update();
		}
	}

}
#endif

/**
 * \brief Initiates fetching the sensor data and maintains the sensor status
 * \details It is assumed that the current sensor status in
 * \ref main_sensor_state is IDLE and that \ref main_am2303_lockedTicks equals
 * zero.
 */
static void main_fetchData(void) {
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		main_sensor_state = READ_AM2303_CHN0;
	}
	main_am2303_lockedTicks = SYSTEM_TIMER_MS_TO_TICKS(10000);
	am2303_startReading(0, main_recordData);
}

#ifdef USE_AM2303_CHN1
/**
 * \brief Stores the fetched data locally and sets the sensor state
 * \details The function starts reading the second humidity sensor if the first
 * one was queried. If the status is not successful, the readings are skipped
 * and the next sensor is queried.
 */
void main_recordData(status_t status, uint16_t temperature, uint16_t humidity,
		uint8_t channel) {

	if (channel == 0) {

		if (status == success) {
			main_am2303_temperature_chn0 = temperature;
			main_am2303_humidity_chn0 = humidity;
		}
		main_sensor_state = READ_AM2303_CHN1;
		am2303_startReading(1, main_recordData);

	} else if (channel == 1) {
		if (status == success) {
			main_am2303_temperature_chn1 = temperature;
			main_am2303_humidity_chn1 = humidity;
		}
		main_sensor_state = IDLE;
	}

	DEBUG_PRINT(0x02, status);

}
#else
/**
 * \brief Stores the fetched data locally and sets the sensor state
 * \details If the status is not successful, the readings are skipped and the
 * state is set to IDLE
 */
void main_recordData(status_t status, uint16_t temperature, uint16_t humidity,
		uint8_t channel) {

	if (channel == 0) {
		if (status == success) {
			main_am2303_temperature_chn0 = temperature;
			main_am2303_humidity_chn0 = humidity;
		}
		main_sensor_state = IDLE;
	}

	DEBUG_PRINT(0x02, status);
}
#endif

#ifdef USE_BUTTON_CNT
/**
 * \brief Registers the button event to be sent as soon as possible
 */
void main_handleButtonEvent(int16_t cnt, uint8_t btn) {
	main_data.buttonFlags |= btn;
}
#endif

