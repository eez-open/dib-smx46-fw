#include <math.h>

#include "main.h"

////////////////////////////////////////////////////////////////////////////////

extern SPI_HandleTypeDef hspi1;
extern TIM_HandleTypeDef htim1;

////////////////////////////////////////////////////////////////////////////////

#define BUFFER_SIZE 20

uint8_t output[BUFFER_SIZE];
uint8_t input[BUFFER_SIZE];

volatile int transferCompleted;

typedef struct {
    uint32_t routes;

    float dac1;
    float dac2;

	uint8_t relayOn;
} FromMasterToSlave;

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

float relayOn = 0;

void updateRelay() {
	HAL_GPIO_WritePin(K_PWR_GPIO_Port, K_PWR_Pin, relayOn ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

////////////////////////////////////////////////////////////////////////////////

#define SPI_SLAVE_SYNBYTE         0x53
#define SPI_MASTER_SYNBYTE        0xAC

void slaveSynchro(void) {
    uint32_t idw0 = HAL_GetUIDw0();
    uint32_t idw1 = HAL_GetUIDw1();
    uint32_t idw2 = HAL_GetUIDw2();

    uint8_t txBuffer[15] = {
        SPI_SLAVE_SYNBYTE,
        FIRMWARE_VERSION_MAJOR, FIRMWARE_VERSION_MINOR,
        idw0 >> 24, (idw0 >> 16) & 0xFF, (idw0 >> 8) & 0xFF, idw0 & 0xFF,
        idw1 >> 24, (idw1 >> 16) & 0xFF, (idw1 >> 8) & 0xFF, idw1 & 0xFF,
        idw2 >> 24, (idw2 >> 16) & 0xFF, (idw2 >> 8) & 0xFF, idw2 & 0xFF
    };

    uint8_t rxBuffer[15];

	while (1) {
		transferCompleted = 0;
		HAL_StatusTypeDef result = HAL_SPI_TransmitReceive_DMA(&hspi1, (uint8_t *)&txBuffer, (uint8_t *)&rxBuffer, sizeof(rxBuffer));
		if (result == HAL_OK) {
			HAL_GPIO_WritePin(DIB_IRQ_GPIO_Port, DIB_IRQ_Pin, GPIO_PIN_RESET);
			while (!transferCompleted) {
			}
			if (transferCompleted == 1) {
				break;
			}
		}
		HAL_Delay(1);
    }
}

////////////////////////////////////////////////////////////////////////////////

void beginTransfer() {
    transferCompleted = 0;
    HAL_SPI_TransmitReceive_DMA(&hspi1, output, input, BUFFER_SIZE);
    HAL_GPIO_WritePin(DIB_IRQ_GPIO_Port, DIB_IRQ_Pin, GPIO_PIN_RESET);
}

void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi) {
	HAL_GPIO_WritePin(DIB_IRQ_GPIO_Port, DIB_IRQ_Pin, GPIO_PIN_SET);
	transferCompleted = 1;
}

void HAL_SPI_ErrorCallback(SPI_HandleTypeDef *hspi) {
	HAL_GPIO_WritePin(DIB_IRQ_GPIO_Port, DIB_IRQ_Pin, GPIO_PIN_SET);
    transferCompleted = 2;
}

////////////////////////////////////////////////////////////////////////////////

void setup() {
    setRoutes(0);
	initDac();
	updateRelay();

	slaveSynchro();
    beginTransfer();
}

void loop() {
    if (transferCompleted) {
    	if (transferCompleted == 1) {
			FromMasterToSlave *data = (FromMasterToSlave *)input;

			setRoutes(data->routes);

			if (data->dac1 != dac1) {
				dac1 = data->dac1;
				updateDac1();
			}

			if (data->dac2 != dac2) {
				dac2 = data->dac2;
				updateDac2();
			}

			if (data->relayOn != relayOn) {
				relayOn = data->relayOn;
				updateRelay();
			}
    	}

	    beginTransfer();
    }
}
