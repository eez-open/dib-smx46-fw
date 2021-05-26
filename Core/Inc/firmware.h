#pragma once

enum Command {
	COMMAND_NONE       = 0x113B3759,
    COMMAND_GET_INFO   = 0x21EC18D4,
    COMMAND_GET_STATE  = 0x3C1D2EF4,
    COMMAND_SET_PARAMS = 0x4B723BFF
};

enum Waveform {
	WAVEFORM_NONE,
	WAVEFORM_DC,
	WAVEFORM_SINE_WAVE,
	WAVEFORM_TRIANGLE,
	WAVEFORM_SQUARE_WAVE,
	WAVEFORM_PULSE,
	WAVEFORM_SAWTOOTH,
	WAVEFORM_ARBITRARY
};

struct WaveformParameters {
	Waveform waveform;
	float frequency;
	float phaseShift;
	float amplitude;
	float offset;
	float pulseWidth;
};

struct SetParams {
    uint32_t routes;
    float aoutValue[2];
    WaveformParameters dacWaveformParameters[2];
    uint8_t relayOn;
};

struct Request {
	uint32_t command;

    union {
    	SetParams setParams;
    };
};

struct Response {
	uint32_t command;

    union {
        struct {
            uint8_t firmwareMajorVersion;
            uint8_t firmwareMinorVersion;
            uint32_t idw0;
            uint32_t idw1;
            uint32_t idw2;
        } getInfo;

        struct {
            uint32_t tickCount;
        } getState;

        struct {
            uint8_t result; // 1 - success, 0 - failure
        } setParams;
    };
};
