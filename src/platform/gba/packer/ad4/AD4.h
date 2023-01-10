#pragma once

#include <stdint.h>

struct AD4State_t {
	 int32_t zM1, zM2;
	 int32_t Tap;
	 int32_t Quant;
	 int32_t Output;
	uint32_t MaxOutputLevel;
};

void AD4_Init(struct AD4State_t *State) {
	State->zM1    = 0;
	State->zM2    = 0;
	State->Tap    = 0;
	State->Quant  = 0x0800;
	State->Output = 0;
	State->MaxOutputLevel = 0;
}

uint32_t AD4_EncodeFrame(struct AD4State_t *State, const int16_t *Data) {
	static const uint8_t AdaptTable[] = {
	    192,192,136,136,128,128,128,128, // -8..-1
	    112,128,128,128,128,136,136,192, //  0..+7
	};

	uint8_t  n;
	 int32_t zM1    = State->zM1;
	 int32_t zM2    = State->zM2;
	 int32_t Tap    = State->Tap;
	 int32_t Quant  = State->Quant;
	 int32_t Output = State->Output;
	uint32_t MaxOutputLevel = State->MaxOutputLevel;
	uint32_t FrameData = 0;
	for(n=0;n<8;n++) {
		//! Get input and apply pre-filter
		//! Pre-filter: Hpre(z) = 1 - (7/8)z^-1
		int32_t X = Data[n];
		{
			int32_t d = Tap - (Tap >> 3);
			X = X - d, Tap = X + d;
		}

		//! Compute prediction and quantize residue
		int32_t P = zM1 - zM2;
		int32_t R = X - P; {
			R = (2*R + ((R < 0) ? (-Quant) : (+Quant))) / (2*Quant); //! (R + Sign[R]*(Quant/2)) / Quant
			if(R < -8) R = -8;
			if(R > +7) R = +7;
		}
		int32_t Y = P + R*Quant;

		//! Calculate output value and update maximum level
		//! Post-filter: Hpost(z) = 1 / Hpre(z) = 1 / (1 - (7/8)z^-1)
		Output = Y + Output - (Output >> 3);
		uint32_t Level = (uint32_t)((Output < 0) ? (-Output) : (+Output));
		if(Level > MaxOutputLevel) MaxOutputLevel = Level;

		//! Update taps and push residue to frame
		zM2 = zM1;
		zM1 = Y;
		FrameData |= (R&0xF) << (n*4);

		//! Adapt quantizer
		//! Rounding up means that Quant can never collapse to 0
		Quant = (Quant * AdaptTable[R+8] + 127) >> 7;
	}
	State->zM1    = zM1;
	State->zM2    = zM2;
	State->Tap    = Tap;
	State->Quant  = Quant;
	State->Output = Output;
	State->MaxOutputLevel = MaxOutputLevel;
	return FrameData;
}
