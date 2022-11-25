#ifndef H_RASTERIZER_MODE13
#define H_RASTERIZER_MODE13

#include "common.h"

extern uint8 gLightmap[256 * 32];
extern const uint8* gTile;

#define rasterizeS rasterizeS_c
#define rasterizeF rasterizeF_c
#define rasterizeG rasterizeG_c
#define rasterizeFT rasterizeFT_c
#define rasterizeGT rasterizeGT_c
#define rasterizeFTA rasterizeFTA_c
#define rasterizeGTA rasterizeGTA_c
#define rasterizeSprite rasterizeSprite_c
#define rasterizeLineH rasterizeLineH_c
#define rasterizeLineV rasterizeLineV_c
#define rasterizeFillS rasterizeFillS_c

void rasterizeS_c(uint16* pixel, const VertexLink* L, const VertexLink* R)
{
    const uint8* ft_lightmap = &gLightmap[0x1A00];

    int32 Lh = 0;
    int32 Rh = 0;
    int32 Ldx = 0;
    int32 Rdx = 0;
    int32 Rx;
    int32 Lx;

    while (1)
    {
        while (!Lh)
        {
            const VertexLink* N = L + L->prev;

            if (N->v.y < L->v.y) return;

            Lh = N->v.y - L->v.y;
            Lx = L->v.x;

            if (Lh > 1)
            {
                uint32 tmp = FixedInvU(Lh);
                Ldx = tmp * (N->v.x - Lx);
            }

            Lx <<= 16;
            L = N;
        }

        while (!Rh) 
        {
            const VertexLink* N = R + R->next;

            if (N->v.y < R->v.y) return;

            Rh = N->v.y - R->v.y;
            Rx = R->v.x;

            if (Rh > 1) {
                uint32 tmp = FixedInvU(Rh);
                Rdx = tmp * (N->v.x - Rx);
            }

            Rx <<= 16;
            R = N;
        }

        int32 h = X_MIN(Lh, Rh);
        Lh -= h;
        Rh -= h;

        while (h--)
        {
            int32 x1 = Lx >> 16;
            int32 x2 = Rx >> 16;

            int32 width = x2 - x1;

            if (width > 0)
            {
                volatile uint16* ptr = pixel + (x1 >> 1);

                if (x1 & 1)
                {
                    uint16 p = ptr[0];

                    uint16 index = ft_lightmap[p >> 8];

                    ptr[0] = (p & 0x00FF) | (index << 8);
                    ptr++;
                    width--;
                }

                if (width & 1)
                {
                    uint16 p = ptr[width >> 1];

                    uint16 index = ft_lightmap[p & 0xFF];

                    ptr[width >> 1] = (p & 0xFF00) | index;
                    width--;
                }

                while (width)
                {
                    uint16 p = *ptr;

                    uint16 index = ft_lightmap[p & 0xFF];
                    index |= ft_lightmap[p >> 8] << 8;

                    *ptr++ = index;
                    width -= 2;
                }

                #undef SHADE
            }

            pixel += VRAM_WIDTH;

            Lx += Ldx;
            Rx += Rdx;
        }
    }
}

void rasterizeF_c(uint16* pixel, const VertexLink* L, const VertexLink* R)
{
    uint32 color = (uint32)R;
    color = gLightmap[(L->v.g << 8) | color];
    color |= (color << 8);

    int32 Lh = 0;
    int32 Rh = 0;
    int32 Ldx = 0;
    int32 Rdx = 0;
    int32 Rx;
    int32 Lx;

    while (1)
    {
        while (!Lh)
        {
            const VertexLink* N = L + L->prev;

            ASSERT(L->v.y >= 0);

            if (N->v.y < L->v.y) return;

            Lh = N->v.y - L->v.y;
            Lx = L->v.x;

            if (Lh > 1)
            {
                uint32 tmp = FixedInvU(Lh);
                Ldx = tmp * (N->v.x - Lx);
            }

            Lx <<= 16;
            L = N;
        }

        while (!Rh) 
        {
            const VertexLink* N = R + R->next;

            ASSERT(R->v.y >= 0);

            if (N->v.y < R->v.y) return;

            Rh = N->v.y - R->v.y;
            Rx = R->v.x;

            if (Rh > 1) {
                uint32 tmp = FixedInvU(Rh);
                Rdx = tmp * (N->v.x - Rx);
            }

            Rx <<= 16;
            R = N;
        }

        int32 h = X_MIN(Lh, Rh);
        Lh -= h;
        Rh -= h;

        while (h--)
        {
            int32 x1 = Lx >> 16;
            int32 x2 = Rx >> 16;

            int32 width = x2 - x1;

            if (width > 0)
            {
                volatile uint8* ptr = (uint8*)pixel + x1;

                if (intptr_t(ptr) & 1)
                {
                    *ptr++ = uint8(color);
                    width--;
                }

                if (width & 1)
                {
                    ptr[width - 1] = uint8(color);
                }

                if (width & 2)
                {
                    *(uint16*)ptr = color;
                    ptr += 2;
                }

                width >>= 2;
                while (width--)
                {
                    *(uint16*)ptr = color;
                    ptr += 2;
                    *(uint16*)ptr = color;
                    ptr += 2;
                }
            }

            pixel += VRAM_WIDTH;

            Lx += Ldx;
            Rx += Rdx;
        }
    }
}

void rasterizeFT_c(uint16* pixel, const VertexLink* L, const VertexLink* R)
{
    const uint8* ft_lightmap = &gLightmap[L->v.g << 8];

    int32 Lh = 0, Rh = 0;
    int32 Lx, Rx, Ldx = 0, Rdx = 0;
    uint32 Lt, Rt, Ldt, Rdt;
    Ldt = 0;
    Rdt = 0;

    while (1)
    {
        while (!Lh)
        {
            const VertexLink* N = L + L->prev;

            if (N->v.y < L->v.y) return;

            Lh = N->v.y - L->v.y;
            Lx = L->v.x;
            Lt = L->t.t;

            if (Lh > 1)
            {
                int32 tmp = FixedInvU(Lh);
                Ldx = tmp * (N->v.x - Lx);

                uint32 duv = N->t.t - Lt;
                uint32 du = tmp * int16(duv >> 16);
                uint32 dv = tmp * int16(duv);
                Ldt = (du & 0xFFFF0000) | (dv >> 16);
            }

            Lx <<= 16;
            L = N;
        }

        while (!Rh) 
        {
            const VertexLink* N = R + R->next;

            if (N->v.y < R->v.y) return;

            Rh = N->v.y - R->v.y;
            Rx = R->v.x;
            Rt = R->t.t;

            if (Rh > 1)
            {
                int32 tmp = FixedInvU(Rh);
                Rdx = tmp * (N->v.x - Rx);

                uint32 duv = N->t.t - Rt;
                uint32 du = tmp * int16(duv >> 16);
                uint32 dv = tmp * int16(duv);
                Rdt = (du & 0xFFFF0000) | (dv >> 16);
            }

            Rx <<= 16;
            R = N;
        }

        int32 h = X_MIN(Lh, Rh);
        Lh -= h;
        Rh -= h;

        while (h--)
        {
            int32 x1 = Lx >> 16;
            int32 x2 = Rx >> 16;

            int32 width = x2 - x1;

            if (width > 0)
            {
                uint32 tmp = FixedInvU(width);

                uint32 duv = Rt - Lt;
                uint32 du = tmp * int16(duv >> 16);
                uint32 dv = tmp * int16(duv);
                uint32 dtdx = (du & 0xFFFF0000) | (dv >> 16);

                uint32 t = Lt;

                volatile uint8* ptr = (uint8*)pixel + x1;

                if (intptr_t(ptr) & 1)
                {
                    *ptr++ = ft_lightmap[gTile[(t & 0xFF00) | (t >> 24)]];
                    t += dtdx;
                    width--;
                }

                if (width & 1)
                {
                    uint32 tmp = Rt - dtdx;
                    ptr[width - 1] = ft_lightmap[gTile[(tmp & 0xFF00) | (tmp >> 24)]];
                }

                width >>= 1;
                while (width--)
                {
                    uint8 indexA = ft_lightmap[gTile[(t & 0xFF00) | (t >> 24)]];
                    t += dtdx;
                    uint8 indexB = ft_lightmap[gTile[(t & 0xFF00) | (t >> 24)]];
                    t += dtdx;
                    
                #ifdef CPU_BIG_ENDIAN
                    *(uint16*)ptr = indexB | (indexA << 8);
                #else
                    *(uint16*)ptr = indexA | (indexB << 8);
                #endif

                    ptr += 2;
                }
            }

            pixel += VRAM_WIDTH;

            Lx += Ldx;
            Rx += Rdx;
            Lt += Ldt;
            Rt += Rdt;
        }
    }
}

void rasterizeGT_c(uint16* pixel, const VertexLink* L, const VertexLink* R)
{
#ifdef ALIGNED_LIGHTMAP
    ASSERT((intptr_t(gLightmap) & 0xFFFF) == 0); // lightmap should be 64k aligned
#endif

    int32 Lh = 0, Rh = 0;
    int32 Lx, Rx, Lg, Rg, Ldx = 0, Rdx = 0, Ldg = 0, Rdg = 0;
    uint32 Lt, Rt, Ldt, Rdt;
    Ldt = 0;
    Rdt = 0;

    // 8-bit fractional part precision for Gouraud component
    // has some artifacts but allow to save one reg for inner loop
    // with aligned by 64k address of lightmap array

    while (1)
    {
        while (!Lh)
        {
            const VertexLink* N = L + L->prev;

            if (N->v.y < L->v.y) return;

            Lh = N->v.y - L->v.y;
            Lx = L->v.x;
            Lg = L->v.g;
            Lt = L->t.t;

            if (Lh > 1)
            {
                int32 tmp = FixedInvU(Lh);
                Ldx = tmp * (N->v.x - Lx);
                Ldg = tmp * (N->v.g - Lg) >> 8;

                uint32 duv = N->t.t - Lt;
                uint32 du = tmp * int16(duv >> 16);
                uint32 dv = tmp * int16(duv);
                Ldt = (du & 0xFFFF0000) | (dv >> 16);
            }

            Lx <<= 16;
            Lg <<= 8;
            L = N;
        }

        while (!Rh) 
        {
            const VertexLink* N = R + R->next;

            if (N->v.y < R->v.y) return;

            Rh = N->v.y - R->v.y;
            Rx = R->v.x;
            Rg = R->v.g;
            Rt = R->t.t;

            if (Rh > 1)
            {
                int32 tmp = FixedInvU(Rh);
                Rdx = tmp * (N->v.x - Rx);
                Rdg = tmp * (N->v.g - Rg) >> 8;

                uint32 duv = N->t.t - Rt;
                uint32 du = tmp * int16(duv >> 16);
                uint32 dv = tmp * int16(duv);
                Rdt = (du & 0xFFFF0000) | (dv >> 16);
            }

            Rx <<= 16;
            Rg <<= 8;
            R = N;
        }

        int32 h = X_MIN(Lh, Rh);
        Lh -= h;
        Rh -= h;

        while (h--)
        {
            int32 x1 = Lx >> 16;
            int32 x2 = Rx >> 16;

            int32 width = x2 - x1;

            if (width > 0)
            {
                int32 tmp = FixedInvU(width);

                int32 dgdx = tmp * (Rg - Lg) >> 15;

                uint32 duv = Rt - Lt;
                uint32 u = tmp * int16(duv >> 16);
                uint32 v = tmp * int16(duv);
                uint32 dtdx = (u & 0xFFFF0000) | (v >> 16);

                int32 g = Lg;
                uint32 t = Lt;

                volatile uint8* ptr = (uint8*)pixel + x1;

                if (intptr_t(ptr) & 1)
                {
                    *ptr++ = gLightmap[(g >> 8 << 8) | gTile[(t & 0xFF00) | (t >> 24)]];
                    t += dtdx;
                    g += dgdx >> 1;
                    width--;
                }

                if (width & 1)
                {
                    uint32 tmp = Rt - dtdx;
                    ptr[width - 1] = gLightmap[(Rg >> 8 << 8) | gTile[(tmp & 0xFF00) | (tmp >> 24)]];
                }

            #ifdef ALIGNED_LIGHTMAP
                g += intptr_t(gLightmap);
            #endif

                width >>= 1;

                while (width--)
                {
                #ifdef ALIGNED_LIGHTMAP
                    const uint8* LMAP = (uint8*)(g >> 8 << 8); 

                    uint8 indexA = LMAP[gTile[(t & 0xFF00) | (t >> 24)]];
                    t += dtdx;
                    uint8 indexB = LMAP[gTile[(t & 0xFF00) | (t >> 24)]];
                    t += dtdx;
                    g += dgdx;
                #else
                    uint8 indexA = gLightmap[(g >> 8 << 8) | gTile[(t & 0xFF00) | (t >> 24)]];
                    t += dtdx;
                    uint8 indexB = gLightmap[(g >> 8 << 8) | gTile[(t & 0xFF00) | (t >> 24)]];
                    t += dtdx;
                    g += dgdx;
                #endif
                    
                #ifdef CPU_BIG_ENDIAN
                    *(uint16*)ptr = indexB | (indexA << 8);
                #else
                    *(uint16*)ptr = indexA | (indexB << 8);
                #endif

                    ptr += 2;
                }
            }

            pixel += VRAM_WIDTH;

            Lx += Ldx;
            Rx += Rdx;
            Lg += Ldg;
            Rg += Rdg;
            Lt += Ldt;
            Rt += Rdt;
        }
    }
}

void rasterizeFTA_c(uint16* pixel, const VertexLink* L, const VertexLink* R)
{
    const uint8* ft_lightmap = &gLightmap[L->v.g << 8];

    int32 Lh = 0, Rh = 0;
    int32 Lx, Rx, Ldx = 0, Rdx = 0;
    uint32 Lt, Rt, Ldt, Rdt;
    Ldt = 0;
    Rdt = 0;

    while (1)
    {
        while (!Lh)
        {
            const VertexLink* N = L + L->prev;

            if (N->v.y < L->v.y) return;

            Lh = N->v.y - L->v.y;
            Lx = L->v.x;
            Lt = L->t.t;

            if (Lh > 1)
            {
                int32 tmp = FixedInvU(Lh);
                Ldx = tmp * (N->v.x - Lx);

                uint32 duv = N->t.t - Lt;
                uint32 du = tmp * int16(duv >> 16);
                uint32 dv = tmp * int16(duv);
                Ldt = (du & 0xFFFF0000) | (dv >> 16);
            }

            Lx <<= 16;
            L = N;
        }

        while (!Rh) 
        {
            const VertexLink* N = R + R->next;

            if (N->v.y < R->v.y) return;

            Rh = N->v.y - R->v.y;
            Rx = R->v.x;
            Rt = R->t.t;

            if (Rh > 1)
            {
                int32 tmp = FixedInvU(Rh);
                Rdx = tmp * (N->v.x - Rx);

                uint32 duv = N->t.t - Rt;
                uint32 du = tmp * int16(duv >> 16);
                uint32 dv = tmp * int16(duv);
                Rdt = (du & 0xFFFF0000) | (dv >> 16);
            }

            Rx <<= 16;
            R = N;
        }

        int32 h = X_MIN(Lh, Rh);
        Lh -= h;
        Rh -= h;

        while (h--)
        {
            int32 x1 = Lx >> 16;
            int32 x2 = Rx >> 16;

            int32 width = x2 - x1;

            if (width > 0)
            {
                uint32 tmp = FixedInvU(width);

                uint32 duv = Rt - Lt;
                uint32 du = tmp * int16(duv >> 16);
                uint32 dv = tmp * int16(duv);
                uint32 dtdx = (du & 0xFFFF0000) | (dv >> 16);

                uint32 t = Lt;

                volatile uint8* ptr = (uint8*)pixel + x1;

                if (intptr_t(ptr) & 1)
                {
                    uint8 p = gTile[(t & 0xFF00) | (t >> 24)];
                    if (p) {
                        *ptr = ft_lightmap[p];
                    }
                    ptr++;
                    t += dtdx;
                    width--;
                }

                if (width & 1)
                {
                    uint32 tmp = Rt - dtdx;
                    uint8 p = gTile[(tmp & 0xFF00) | (tmp >> 24)];
                    if (p) {
                        ptr[width - 1] = ft_lightmap[p];
                    }
                }

                width >>= 1;
                while (width--)
                {
                    uint8 indexA = gTile[(t & 0xFF00) | (t >> 24)];
                    t += dtdx;
                    uint8 indexB = gTile[(t & 0xFF00) | (t >> 24)];
                    t += dtdx;


                    if (indexA && indexB)
                    {
                        indexA = ft_lightmap[indexA];
                        indexB = ft_lightmap[indexB];

                        #ifdef CPU_BIG_ENDIAN
                            *(uint16*)ptr = indexB | (indexA << 8);
                        #else
                            *(uint16*)ptr = indexA | (indexB << 8);
                        #endif

                    }/* else if (indexA) {
                        *(uint16*)ptr = (*(uint16*)ptr & 0xFF00) | ft_lightmap[indexA];
                    } else if (indexB) {
                        *(uint16*)ptr = (*(uint16*)ptr & 0x00FF) | (ft_lightmap[indexB] << 8);
                    }*/

                    ptr += 2;
                }
            }

            pixel += VRAM_WIDTH;

            Lx += Ldx;
            Rx += Rdx;
            Lt += Ldt;
            Rt += Rdt;
        }
    }
}

void rasterizeGTA_c(uint16* pixel, const VertexLink* L, const VertexLink* R)
{
    rasterizeFTA(pixel, L, R);
}

void rasterizeSprite_c(uint16* pixel, const VertexLink* L, const VertexLink* R)
{
    // TODO
}

void rasterizeLineH_c(uint16* pixel, const VertexLink* L, const VertexLink* R)
{
    // TODO
}

void rasterizeLineV_c(uint16* pixel, const VertexLink* L, const VertexLink* R)
{
    // TODO
}

void rasterizeFillS_c(uint16* pixel, const VertexLink* L, const VertexLink* R)
{
    // TODO
}

#endif
