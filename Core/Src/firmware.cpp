#include <math.h>
#include <memory.h>

#include "main.h"

#include "firmware.h"
#include "utils.h"

static const uint32_t CONF_SIGNAL_RELAY_DEBOUNCE_TIME_MS = 10;
static const uint32_t CONF_POWER_RELAY_DEBOUNCE_TIME_MS = 20;

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

bool setRoutes(uint32_t routes) {
	if (g_routes != routes) {
		for (int i = 0; i < 24; i++) {
			uint32_t mask = 1 << i;
			if ((g_routes & mask) != (routes & mask)) {
				HAL_GPIO_WritePin(g_routePorts[i], g_routePins[i], routes & mask ? GPIO_PIN_SET : GPIO_PIN_RESET);
			}
		}
		g_routes = routes;
		return true;
	}
	return false;
}

////////////////////////////////////////////////////////////////////////////////

static float DAC_MIN = 0.0f;
static float DAC_MAX = 10.0f;

extern TIM_HandleTypeDef htim1;

float aoutValue[2];

uint32_t dacValueToPwm(float dac) {
	if (dac < DAC_MIN) {
		return 0;
	}
	if (dac > DAC_MAX) {
		return 1440;
	}
	return (uint32_t)roundf(dac * 1440 / 10.0f);
}

void updateDac(int i) {
	__HAL_TIM_SET_COMPARE(&htim1, i == 0 ? TIM_CHANNEL_1 : TIM_CHANNEL_4, dacValueToPwm(aoutValue[i]));
}

void initDac(void) {
	HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
	updateDac(0);

	HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_4);
	updateDac(1);
}

////////////////////////////////////////////////////////////////////////////////

#define M_PI_F ((float)M_PI)

static const float TIMER_PERIOD = 1 / 1000.0f;

typedef float (*WaveformFunction)(float);
typedef void (*TimerTickFunc)(void);

extern TIM_HandleTypeDef htim14;

WaveformParameters dacWaveformParameters[2];
WaveformFunction waveFormFunc[2];
float phi[2];
float dphi[2];

WaveformFunction getWaveformFunction(WaveformParameters &waveformParameters);

void updateWaveform(int i, WaveformParameters &waveformParameters) {
	__disable_irq();

	if (waveformParameters.waveform != WAVEFORM_NONE) {
		waveFormFunc[i] = getWaveformFunction(waveformParameters);
		phi[i] = 2.0 * M_PI * waveformParameters.phaseShift / 360.0f;
		dphi[i] = 2.0 * M_PI * waveformParameters.frequency * TIMER_PERIOD;
	}

	memcpy(&dacWaveformParameters[i], &waveformParameters, sizeof(waveformParameters));

	__enable_irq();
}

////////////////////////////////////////////////////////////////////////////////

float dcf(float t) {
	return 0.0f;
}

float sineHalfRectifiedf(float t) {
	if (t < M_PI_F) {
		return 2.0f * sinf(t);
	}

	return 0.0f;
}

float sineFullRectifiedf(float t) {
	return 2.0f * sinf(t / 2.0f);
}

float trianglef(float t) {
	float a, b, c;

	if (t < M_PI_F / 2.0f) {
		a = 0;
		b = 1;
		c = 0;
	} else if (t < 3.0f * M_PI_F / 2.0f) {
		a = 1;
		b = -1;
		c = M_PI_F / 2.0f;
	} else {
		a = -1;
		b = 1;
		c = 3.0f * M_PI_F / 2.0f;
	}

	return a + b * (t - c) / (M_PI_F / 2.0f);
}

float squaref(float t) {
	if (t < M_PI_F) {
		return 1.0f;
	}
	return -1.0f;
}

static float g_dutyCycle;

float pulsef(float t) {
	if (t < g_dutyCycle * 2.0f * M_PI_F / 100.0f) {
		return 1.0f;
	}
	return -1.0f;
}

float sawtoothf(float t) {
	return -1.0f + t / M_PI_F;
}

float arbitraryf(float t) {
	return 0.0f;
}

WaveformFunction getWaveformFunction(WaveformParameters &waveformParameters) {
	if (waveformParameters.waveform == WAVEFORM_DC) {
		return dcf;
	} else if (waveformParameters.waveform == WAVEFORM_SINE) {
		return sinf;
	} else if (waveformParameters.waveform == WAVEFORM_HALF_RECTIFIED) {
		return sineHalfRectifiedf;
	} else if (waveformParameters.waveform == WAVEFORM_FULL_RECTIFIED) {
		return sineFullRectifiedf;
	} else if (waveformParameters.waveform == WAVEFORM_TRIANGLE) {
		return trianglef;
	} else if (waveformParameters.waveform == WAVEFORM_SQUARE) {
		return squaref;
	} else if (waveformParameters.waveform == WAVEFORM_PULSE) {
		g_dutyCycle = waveformParameters.dutyCycle;
		return pulsef;
	} else if (waveformParameters.waveform == WAVEFORM_SAWTOOTH) {
		return sawtoothf;
	} else {
		return arbitraryf;
	}
}

void FuncGen_DAC(int i) {
	auto &waveformParameters = dacWaveformParameters[i];

	g_dutyCycle = waveformParameters.dutyCycle;
	float value;
	if (waveformParameters.waveform == WAVEFORM_DC) {
		value = waveformParameters.amplitude;
	} else {
		value = waveformParameters.offset + waveformParameters.amplitude * waveFormFunc[i](phi[i]) / 2.0f;
	}

	phi[i] += dphi[i];
	if (phi[i] >= 2.0f * M_PI_F) {
		phi[i] -= 2.0f * M_PI_F;
	}

	aoutValue[i] = value;
	updateDac(i);
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
	if (htim == &htim14) {
		if (dacWaveformParameters[0].waveform != WAVEFORM_NONE) {
			FuncGen_DAC(0);
		}
		if (dacWaveformParameters[1].waveform != WAVEFORM_NONE) {
			FuncGen_DAC(1);
		}
	}
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

static uint32_t input[(sizeof(Request) + 3) / 4 + 1];
static uint32_t output[(sizeof(Request) + 3) / 4];

// setup is called once at the beginning from the main.c
extern "C" void setup() {
    setRoutes(0);
	initDac();
	updateRelay();

	TIM14->ARR = (uint16_t)(TIMER_PERIOD * 1000000) - 1;
	HAL_TIM_Base_Start_IT(&htim14);
}

// loop is called, of course, inside the loop from the main.c
extern "C" void loop() {
	// start SPI transfer
    transferState = TRANSFER_STATE_WAIT;
    HAL_SPI_TransmitReceive_DMA(hspiMaster, (uint8_t *)output, (uint8_t *)input, sizeof(Request));
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
			static const uint16_t MODULE_TYPE_DIB_SMX46 = 46;
			response.getInfo.moduleType = MODULE_TYPE_DIB_SMX46;
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
			bool signalRelaysChanged = setRoutes(request.setParams.routes);

			for (int i = 0; i < 2; i++) {
				if (request.setParams.dacWaveformParameters[0].waveform == WAVEFORM_NONE) {
					if (request.setParams.aoutValue[i] != aoutValue[i]) {
						aoutValue[i] = request.setParams.aoutValue[i];
						updateDac(i);
					}
				}

				if (memcpy(&dacWaveformParameters[i], &request.setParams.dacWaveformParameters[i], sizeof(dacWaveformParameters[i])) != 0) {
					updateWaveform(i, request.setParams.dacWaveformParameters[i]);
				}
			}

			if (request.setParams.relayOn != relayOn) {
				relayOn = request.setParams.relayOn;
				updateRelay();
				HAL_Delay(CONF_POWER_RELAY_DEBOUNCE_TIME_MS);
			} else if (signalRelaysChanged) {
				HAL_Delay(CONF_SIGNAL_RELAY_DEBOUNCE_TIME_MS);
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
