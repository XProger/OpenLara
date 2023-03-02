#ifndef __GNUC__
# warning "Compile with GCC-compatible compiler for endianness checking."
#endif

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "AD4.h"

int main(int argc, const char *argv[]) {
	if(argc < 3 || argc > 4) {
		printf(
			"Usage: ad4 Input.raw Output.ad4 [dBGain]\n"
			"Input.raw must be mono signed PCM16.\n"
			"Output will be aligned to 4 bytes.\n"
		);
		return 1;
	}

	FILE *InFile = fopen(argv[1], "rb");
	if(!InFile) { printf("Couldn't open input file.\n"); goto Error_InFile; }
	FILE *OutFile = fopen(argv[2], "wb");
	if(!OutFile) { printf("Couldn't open output file.\n"); goto Error_OutFile; }
	double Volume = 1.0;
	if(argc >= 4) Volume = pow(10.0, atof(argv[3]) / 20.0);

	size_t nSamples; {
		fseek(InFile, 0, SEEK_END);
		nSamples = ftell(InFile) / sizeof(int16_t);
		rewind(InFile);
	}

	struct AD4State_t AD4State; AD4_Init(&AD4State);
	size_t Frame, nFrames = (nSamples + 7) / 8;
	for(Frame=0;Frame<nFrames;Frame++) {
		int16_t Buffer[8]; //! 1 frame = 8 samples
		size_t n, nRead = fread(Buffer, sizeof(int16_t), 8, InFile);
		for(n=0;n<nRead;n++) Buffer[n] = (int16_t)(Buffer[n] * Volume);
		while(nRead < 8) Buffer[nRead++] = 0;
		uint32_t FrameData = AD4_EncodeFrame(&AD4State, Buffer);
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
		//! Target is actually intended to be bytes, so swap the
		//! endianness of the frame data on big-endian systems
		FrameData = ((FrameData & 0x000000FF) << 24) |
			    ((FrameData & 0x0000FF00) <<  8) |
			    ((FrameData & 0x00FF0000) >>  8) |
			    ((FrameData & 0xFF000000) >> 24) ;
#endif
		fwrite(&FrameData, 1, sizeof(FrameData), OutFile);
	}
	printf("Maximum output level: %u", AD4State.MaxOutputLevel);
	if(AD4State.MaxOutputLevel < 32768) putchar('\n');
	else printf(" (overflow by %u)\n", AD4State.MaxOutputLevel - 32767);

	fclose(OutFile);
Error_OutFile:
	fclose(InFile);
Error_InFile:
	return 0;
}
