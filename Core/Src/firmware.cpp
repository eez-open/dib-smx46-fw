#include <math.h>

#include "main.h"

#include "firmware.h"
#include "utils.h"

////////////////////////////////////////////////////////////////////////////////

static uint32_t g_routes = 0xFFFFFFFF;

static GPIO_TypeDef* g_routePorts[24] = {
	OUT0_GPIO_Port, OUT4_GPIO_Port,  OUT8_GPIO_Port, OUT12_GPIO_Port, OUT16_GPIO_Port, OUT20_GPIO_Port,
	OUT1_GPIO_Port, OUT5_GPIO_Port,  OUT9_GPIO_Port, OUT13_GPIO_Port, OUT17_GPIO_Port, OUT21_GPIO_Port,
	OUT2_GPIO_Port, OUT6_GPIO_Port, OUT10_GPIO_Port, OUT14_GPIO_Port, OUT18_GPIO_Port, OUT22_GPIO_Port,
	OUT3_GPIO_Port, OUT7_GPIO_Port, OUT11_GPIO_Port, OUT15_GPIO_Port, OUT19_GPIO_Port, OUT23_GPIO_Port,
};

static uint16_t g_routePins[24] = {
	OUT0_Pin, OUT4_Pin,  OUT8_Pin, OUT12_Pin, OUT16_Pin, OUT20_Pin,
	OUT1_Pin, OUT5_Pin,  OUT9_Pin, OUT13_Pin, OUT17_Pin, OUT21_Pin,
	OUT2_Pin, OUT6_Pin, OUT10_Pin, OUT14_Pin, OUT18_Pin, OUT22_Pin,
	OUT3_Pin, OUT7_Pin, OUT11_Pin, OUT15_Pin, OUT19_Pin, OUT23_Pin,
};

void setRoutes(uint32_t routes) {
	if (g_routes != routes) {
		for (int i = 0; i < 24; i++) {
			uint32_t mask = 1 << i;
			if ((g_routes & mask) != (routes & mask)) {
				HAL_GPIO_WritePin(g_routePorts[i], g_routePins[i], routes & mask ? GPIO_PIN_SET : GPIO_PIN_RESET);
			}
		}
		g_routes = routes;
	}
}

////////////////////////////////////////////////////////////////////////////////

extern TIM_HandleTypeDef htim1;

float dac1 = 0.0f;
float dac2 = 0.0f;

uint32_t dacValueToPwm(float dac) {
	if (dac < 0) {
		return 0;
	}
	if (dac > 10.0f) {
		return 1440;
	}
	return (uint32_t)roundf(dac * 1440 / 10.0f);
}

void updateDac1() {
	__HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, dacValueToPwm(dac1));
}

void updateDac2() {
	__HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_4, dacValueToPwm(dac2));
}

void initDac(void) {
	HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
	updateDac1();

	HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_4);
	updateDac2();
}

////////////////////////////////////////////////////////////////////////////////

uint8_t relayOn = 0;

void updateRelay() {
	HAL_GPIO_WritePin(K_PWR_GPIO_Port, K_PWR_Pin, relayOn ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

////////////////////////////////////////////////////////////////////////////////
// master-slave communication

//static const uint32_t CONF_SPI_TRANSFER_TIMEOUT_MS = 2000;

extern "C" void SPI1_Init(void);
extern SPI_HandleTypeDef hspi1;
SPI_HandleTypeDef *hspiMaster = &hspi1; // SPI for MASTER-SLAVE communication
volatile enum {
	TRANSFER_STATE_WAIT,
	TRANSFER_STATE_SUCCESS,
	TRANSFER_STATE_ERROR
} transferState;

void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi) {
	SET_PIN(DIB_IRQ_GPIO_Port, DIB_IRQ_Pin);
	transferState = TRANSFER_STATE_SUCCESS;
}

void HAL_SPI_ErrorCallback(SPI_HandleTypeDef *hspi) {
	SET_PIN(DIB_IRQ_GPIO_Port, DIB_IRQ_Pin);
	transferState = TRANSFER_STATE_ERROR;
}

////////////////////////////////////////////////////////////////////////////////

// setup is called once at the beginning from the main.c
extern "C" void setup() {
    setRoutes(0);
	initDac();
	updateRelay();
}

// loop is called, of course, inside the loop from the main.c
extern "C" void loop() {
	// start SPI transfer
	static const size_t BUFFER_SIZE = 20;
	uint32_t input[(BUFFER_SIZE + 3) / 4 + 1];
	uint32_t output[(BUFFER_SIZE + 3) / 4];
    transferState = TRANSFER_STATE_WAIT;
    HAL_SPI_TransmitReceive_DMA(hspiMaster, (uint8_t *)output, (uint8_t *)input, BUFFER_SIZE);
    RESET_PIN(DIB_IRQ_GPIO_Port, DIB_IRQ_Pin); // inform master that module is ready for the SPI communication

    // wait for the transfer to finish
//	uint32_t startTick = HAL_GetTick();
	while (transferState == TRANSFER_STATE_WAIT) {
//		if (HAL_GetTick() - startTick > CONF_SPI_TRANSFER_TIMEOUT_MS) {
//			// transfer is taking too long to finish, maybe something is stuck, abort it
//			__disable_irq();
//			HAL_SPI_Abort(hspiMaster);
//			SET_PIN(DIB_IRQ_GPIO_Port, DIB_IRQ_Pin);
//			transferState = TRANSFER_STATE_ERROR;
//			__enable_irq();
//			break;
//		}
	}

	// Request and Response are defined in firmware.h
    Request &request = *(Request *)input;
    Response &response = *(Response *)output;

    if (transferState == TRANSFER_STATE_SUCCESS) {
    	// a way to tell the master that command was handled
    	response.command = 0x8000 | request.command;

		if (request.command == COMMAND_GET_INFO) {
			// return back to the master firmware version and MCU id
			response.getInfo.firmwareMajorVersion = FIRMWARE_VERSION_MAJOR;
			response.getInfo.firmwareMinorVersion = FIRMWARE_VERSION_MINOR;
			response.getInfo.idw0 = HAL_GetUIDw0();
			response.getInfo.idw1 = HAL_GetUIDw1();
			response.getInfo.idw2 = HAL_GetUIDw2();
		}

		else if (request.command == COMMAND_GET_STATE) {
			// master periodically asks for the state of the module,
			// for this module there is nothing much to return really
			response.getState.tickCount = HAL_GetTick();
		}

		else if (request.command == COMMAND_SET_PARAMS) {
			setRoutes(request.setParams.routes);

			if (request.setParams.dac1 != dac1) {
				dac1 = request.setParams.dac1;
				updateDac1();
			}

			if (request.setParams.dac2 != dac2) {
				dac2 = request.setParams.dac2;
				updateDac2();
			}

			if (request.setParams.relayOn != relayOn) {
				relayOn = request.setParams.relayOn;
				updateRelay();
			}

			response.setParams.result = 1; // success
		}

		else {
			// unknown command received, tell the master that no command was handled
			response.command = COMMAND_NONE;
		}
    } else {
    	// invalid transfer, reinitialize SPI just in case
    	HAL_SPI_DeInit(hspiMaster);
		SPI1_Init();

		// tell the master that no command was handled
		response.command = COMMAND_NONE;
    }
}
