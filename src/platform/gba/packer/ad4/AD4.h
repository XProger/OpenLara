#pragma once

//! Uncommenting this line writes the output to Debug.sb as 8bit signed PCM
//#define DEBUG_OUTPUT

#include <stdint.h>
#include <stdio.h>

struct AD4State_t {
	 int32_t zM1, zM2;
	 int32_t Quant;
	 int32_t Output;
	uint32_t MaxOutputLevel;
};

void AD4_Init(struct AD4State_t *State) {
	State->zM1    = 0;
	State->zM2    = 0;
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
	 int32_t Quant  = State->Quant;
	 int32_t Output = State->Output;
	uint32_t MaxOutputLevel = State->MaxOutputLevel;
	uint32_t FrameData = 0;
#ifdef DEBUG_OUTPUT
	static FILE *DebugFile = NULL;
	if(!DebugFile) DebugFile = fopen("Debug.sb", "ab");
#endif
	for(n=0;n<8;n++) {
		//! Get input, compute prediction, and quantize residue
		//! Note that we minimize error of Output rather than Y, which implies
		//! applying the post-filter in the analysis equation to get the residue.
		int32_t X = Data[n];
		int32_t P = zM1 - zM2;
		int32_t R = X - (P + Output - (Output >> 3)); {
#if 0 //! Lower RMSE, but sounds noisier
			R = (2*R + ((R < 0) ? (-Quant) : (+Quant))) / (2*Quant); //! (R + Sign[R]*(Quant/2)) / Quant
#else
			//! Round positive residues up, and negative residues towards 0.
			//! I have no idea why, but this fixes limit cycles.
			if(R > 0) R = (R + (Quant-1)) / Quant;
			else      R = (R +         0) / Quant;
#endif
			if(R < -8) R = -8;
			if(R > +7) R = +7;
		}

		//! Calculate output value and apply limiting
		//! Post-filter: Hpost(z) = 1 / Hpre(z) = 1 / (1 - (7/8)z^-1)
		int32_t Y;
		Output = Output - (Output >> 3);
		for(;;) {
			Y = P + R*Quant;
			if(Output+Y < -32768) {
				if(R < +7) R++;
				else break;
			} else if(Output+Y > +32767) {
				if(R > -8) R--;
				else break;
			} else break;
		}
		Output += Y;

		//! Update maximum level after attempted clipping
		uint32_t Level = (uint32_t)((Output < 0) ? (-Output) : (+Output));
		if(Level > MaxOutputLevel) MaxOutputLevel = Level;

		//! Write debug output
#ifdef DEBUG_OUTPUT
		int8_t DebugSample = Output >> 8;
		fwrite(&DebugSample, sizeof(int8_t), 1, DebugFile);
#endif

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
	State->Quant  = Quant;
	State->Output = Output;
	State->MaxOutputLevel = MaxOutputLevel;
	return FrameData;
}
