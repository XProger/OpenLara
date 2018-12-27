#ifndef DEBUG_SCREEN_H
#define DEBUG_SCREEN_H

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <inttypes.h>

typedef struct PsvDebugScreenFont {
	unsigned char* glyphs, width, height, first, last, size_w, size_h;
} PsvDebugScreenFont;

#include "debugScreenFont.c"

#define SCREEN_WIDTH    (960)
#define SCREEN_HEIGHT   (544)
#define SCREEN_FB_WIDTH (960)
#define SCREEN_FB_SIZE  (2 * 1024 * 1024) //Must be 256KB aligned
#ifndef SCREEN_TAB_SIZE /* this allow easy overriding */
#define SCREEN_TAB_SIZE (8)
#endif
#define SCREEN_TAB_W    ((F.size_w) * SCREEN_TAB_SIZE)
#define F psvDebugScreenFont

#define FROM_GREY(c     ) ((((c)*9)    <<16)  |  (((c)*9)       <<8)  | ((c)*9))
#define FROM_3BIT(c,dark) (((!!((c)&4))<<23)  | ((!!((c)&2))<<15)     | ((!!((c)&1))<<7) | (dark ? 0 : 0x7F7F7F))
#define FROM_6BIT(c     ) ((((c)%6)*(51<<16)) | ((((c)/6)%6)*(51<<8)) | ((((c)/36)%6)*51))
#define FROM_FULL(r,g,b ) ((r<<16) | (g<<8) | (b))
#define CLEARSCRN(H,toH,W,toW) for(int h = H; h < toH; h++)for(int w = W; w < toW; w++)((uint32_t*)base)[h*SCREEN_FB_WIDTH + w] = colorBg;

static int mutex, coordX, savedX, coordY, savedY;
static uint32_t defaultFg = 0xFFFFFFFF, colorFg = 0xFFFFFFFF;
static uint32_t defaultBg = 0xFF000000, colorBg = 0xFF000000;


#ifdef __vita__
#include <psp2/display.h>
#include <psp2/kernel/sysmem.h>
#include <psp2/kernel/threadmgr.h>
static void* base; // pointer to frame buffer
#else
#define sceKernelLockMutex(m,v,x) m=v
#define sceKernelUnlockMutex(m,v) m=v
static char base[SCREEN_FB_WIDTH * SCREEN_HEIGHT * 4];
#endif

static size_t psvDebugScreenEscape(const unsigned char *str) {
	for(unsigned i = 0, argc = 0, arg[32] = {0}; argc < (sizeof(arg)/sizeof(*arg)) && str[i]!='\0'; i++)
		switch(str[i]) {
		case '0':case '1':case '2':case '3':case '4':case '5':case '6':case '7':case '8':case '9':
			arg[argc]=(arg[argc]*10) + (str[i] - '0');continue;
		case ';': argc++;continue;
		case 's': savedX = coordX; savedY = coordY; return i;
		case 'u': coordX = savedX; coordY = savedY; return i;
		case 'A': coordY -= arg[0]    * (F.size_h); return i;
		case 'B': coordY += arg[0]    * (F.size_h); return i;
		case 'C': coordX += arg[0]    * (F.size_w); return i;
		case 'D': coordX -= arg[0]    * (F.size_w); return i;
		case 'E': coordY += arg[0]    * (F.size_h); coordX = 0; return i;
		case 'F': coordY -= arg[0]    * (F.size_h); coordX = 0; return i;
		case 'G': coordX = (arg[0]-1) * (F.size_w); return i;
		case 'H': 
		case 'f': coordY = (arg[0]-1) * (F.size_h);
		          coordX = (arg[1]-1) * (F.size_w); return i;
		case 'J': //clear part of (J=screen, K=Line) so J code reuse part of K
		case 'K': if(arg[0]==0)CLEARSCRN(coordY, coordY + F.size_h, coordX, SCREEN_WIDTH);//from curosr to end
		          if(arg[0]==1)CLEARSCRN(coordY, coordY + F.size_h, 0, coordX);//from begining to cursor
		          if(arg[0]==2)CLEARSCRN(coordY, coordY + F.size_h, 0, SCREEN_WIDTH);//whole line
		          if(str[i]=='K')return i;
		          if(arg[0]==0)CLEARSCRN(coordY, SCREEN_HEIGHT, 0, SCREEN_WIDTH);
		          if(arg[0]==1)CLEARSCRN(0, coordY, 0, SCREEN_WIDTH);
		          if(arg[0]==2)CLEARSCRN(0, SCREEN_HEIGHT, 0, SCREEN_WIDTH);
		          return i;
		case 'm':// Color
			if(!arg[0]) {arg[0] = 39;arg[1] = 49;argc = 1;}//no/0 args == reset BG + FG
			for(unsigned c = 0; c <= argc; c++) {
				uint32_t unit = arg[c] % 10, mode = arg[c] / 10, *color = mode&1 ? &colorFg : &colorBg;
				if (arg[c]==1)colorFg|=0x808080;
				if (arg[c]==2)colorFg&=0x7F7F7F;
				if (mode!=3 && mode!=4 && mode!=9 && mode!=10)continue;//skip unsported modes
				if (unit == 9){ // reset FG or BG
					*color = mode&1 ? defaultFg : defaultBg;
				} else if ((unit==8) && (arg[c+1]==5)) { // 8bit : [0-15][16-231][232-256] color map
					c+=2;*color = arg[c]<=15?FROM_3BIT(arg[c],mode<9):arg[c]>=232?FROM_GREY(arg[c]-232):FROM_6BIT(arg[c]-16);
				} else if ((unit==8) && (arg[c+1]==2)) { // 24b color space
					*color = FROM_FULL(arg[c+4], arg[c+3], arg[c+2]);c+=4;
				} else *color = FROM_3BIT(unit,mode<9); // standard 8+8 colors
			}
			return i;
		}
	return 0;
}
int psvDebugScreenInit() {
#ifdef NO_psvDebugScreenInit
	return 0;/* avoid linking non-initializer (prx) with sceDisplay/sceMemory */
#else
	mutex = sceKernelCreateMutex("log_mutex", 0, 0, NULL);
	SceUID displayblock = sceKernelAllocMemBlock("display", SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_RW, SCREEN_FB_SIZE, NULL);
	sceKernelGetMemBlockBase(displayblock, (void**)&base);
	SceDisplayFrameBuf frame = { sizeof(frame), base, SCREEN_FB_WIDTH, 0, SCREEN_WIDTH, SCREEN_HEIGHT};
	return sceDisplaySetFrameBuf(&frame, SCE_DISPLAY_SETBUF_NEXTFRAME);
#endif
}

int psvDebugScreenPuts(const char * _text) {
	const unsigned char*text = (const unsigned char*)_text;
	int bytes_per_glyph = (F.width * F.height) / 8;
	sceKernelLockMutex(mutex, 1, NULL);
	int c;
	for (c = 0; text[c] ; c++) {
		unsigned char t = text[c];
		if (t == '\t') {
			coordX += SCREEN_TAB_W - coordX % SCREEN_TAB_W;
			continue;
		}
		if (coordX + F.width > SCREEN_WIDTH) {
			coordY += F.size_h;
			coordX = 0;
		}
		if (coordY + F.height > SCREEN_HEIGHT) {
			coordX = coordY = 0;
		}
		if (t == '\n') {
			coordX = 0;
			coordY += F.size_h;
			continue;
		} else if (t == '\r') {
			coordX = 0;
			continue;
		} else if ((t == '\e') && (text[c+1] == '[')) {
			c += psvDebugScreenEscape(text + c + 2) + 2;
			if(coordX < 0)coordX = 0;// CSI position are 1-based, 
			if(coordY < 0)coordY = 0;// prevent 0-based coordiate from producing a negativ X/Y
			continue;
		}else if ((t > F.last) || (t < F.first))
			continue; // skip non printable glyph
		uint32_t *vram = ((uint32_t*)base) + coordX + coordY * SCREEN_FB_WIDTH;
		uint8_t *font = &F.glyphs[ (t - F.first) * bytes_per_glyph];
		for (int row = 0, mask = 1 << 7; row < F.height; row++, vram += SCREEN_FB_WIDTH) {
			for (uint32_t *pixel = vram, col = 0; col < F.width ; col++, mask>>=1) {
				if (!mask) {font++; mask = 1 << 7;}// no more mask : we exausted this byte
				*pixel++ = (*font&mask)?colorFg:colorBg;
			}
			for (uint32_t *pixel = vram + F.width, col = F.width; col < F.size_w ; col++)
				*pixel++ = colorBg;// right margin
		}
		for (int row = F.height; row < F.size_h; row++, vram += SCREEN_FB_WIDTH)
			for (uint32_t *pixel = vram, col = 0; col < F.size_w ; col++)
				*pixel++ = colorBg;// bottom margin
		coordX += F.size_w;
	}
	sceKernelUnlockMutex(mutex, 1);
	return c;
}

__attribute__((__format__ (__printf__, 1, 2)))
int psvDebugScreenPrintf(const char *format, ...) {
	char buf[4096];

	va_list opt;
	va_start(opt, format);
	int ret = vsnprintf(buf, sizeof(buf), format, opt);
	psvDebugScreenPuts(buf);
	va_end(opt);

	return ret;
}

void psvDebugScreenSetFgColor(uint32_t rgb){
	psvDebugScreenPrintf("\e[38;2;%lu;%lu;%lum", (rgb>>16)&0xFF, (rgb>>8)&0xFF, rgb&0xFF);
}
void psvDebugScreenSetBgColor(uint32_t rgb){
	psvDebugScreenPrintf("\e[48;2;%lu;%lu;%lum", (rgb>>16)&0xFF, (rgb>>8)&0xFF, rgb&0xFF);
}
#undef F
#endif
