#ifndef H_RASTERIZER_MODE4
#define H_RASTERIZER_MODE4

#include "common.h"

extern uint8 lightmap[256 * 32];
extern const uint8* tile;

#ifdef USE_ASM
    extern "C" {
        void rasterize_dummy(uint16* pixel, const VertexLink* L, const VertexLink* R);
        void rasterizeS_asm(uint16* pixel, const VertexLink* L, const VertexLink* R);
        void rasterizeF_asm(uint16* pixel, const VertexLink* L, const VertexLink* R, int32 index);
        void rasterizeG_asm(uint16* pixel, const VertexLink* L, const VertexLink* R, int32 index);
        void rasterizeFT_asm(uint16* pixel, const VertexLink* L, const VertexLink* R);
        void rasterizeGT_asm(uint16* pixel, const VertexLink* L, const VertexLink* R);
        void rasterizeFTA_asm(uint16* pixel, const VertexLink* L, const VertexLink* R);
        void rasterizeGTA_asm(uint16* pixel, const VertexLink* L, const VertexLink* R);
    }

    #define rasterizeS rasterizeS_asm
    #define rasterizeF rasterizeF_asm
    #define rasterizeG rasterizeG_asm
    #define rasterizeFT rasterizeFT_asm
    #define rasterizeGT rasterizeGT_asm
    #define rasterizeFTA rasterizeFTA_asm
    #define rasterizeGTA rasterizeGTA_asm
    #define rasterizeSprite rasterizeSprite_c
    #define rasterizeLineH rasterizeLineH_c
    #define rasterizeLineV rasterizeLineV_c
    #define rasterizeFillS rasterizeFillS_c
#else
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
    const uint8* ft_lightmap = &lightmap[0x1A00];

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
            const VertexLink* N = L->prev;

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
            const VertexLink* N = R->next;

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
            }

            pixel += VRAM_WIDTH;

            Lx += Ldx;
            Rx += Rdx;
        }
    }
}

void rasterizeF_c(uint16* pixel, const VertexLink* L, const VertexLink* R, int32 index)
{
    uint16 color = lightmap[(L->v.g << 8) | index];
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
            const VertexLink* N = L->prev;

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
            const VertexLink* N = R->next;

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
                    ptr--;
                    *(uint16*)ptr = *ptr | (color << 8);
                    ptr += 2;
                    width--;
                }

                if (width & 1)
                {
                    *(uint16*)(ptr + width - 1) = (ptr[width] << 8) | (color >> 8);
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

void rasterizeG_c(uint16* pixel, const VertexLink* L, const VertexLink* R, int32 index)
{
    int32 Lh = 0, Rh = 0;
    int32 Lx, Rx, Ldx = 0, Rdx = 0;
    int32 Lg, Rg, Ldg = 0, Rdg = 0;

    const uint8* ft_lightmap = lightmap + index;

    while (1)
    {
        while (!Lh)
        {
            const VertexLink* N = L->prev;

            if (N->v.y < L->v.y) return;

            Lh = N->v.y - L->v.y;
            Lx = L->v.x;
            Lg = L->v.g;

            if (Lh > 1)
            {
                int32 tmp = FixedInvU(Lh);
                Ldx = tmp * (N->v.x - Lx);
                Ldg = tmp * (N->v.g - Lg);
            }

            Lx <<= 16;
            Lg <<= 16;
            L = N;
        }

        while (!Rh) 
        {
            const VertexLink* N = R->next;

            if (N->v.y < R->v.y) return;

            Rh = N->v.y - R->v.y;
            Rx = R->v.x;
            Rg = R->v.g;

            if (Rh > 1)
            {
                int32 tmp = FixedInvU(Rh);
                Rdx = tmp * (N->v.x - Rx);
                Rdg = tmp * (N->v.g - Rg);
            }

            Rx <<= 16;
            Rg <<= 16;
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

                int32 dgdx = tmp * ((Rg - Lg) >> 5) >> 10;

                int32 g = Lg;

                volatile uint8* ptr = (uint8*)pixel + x1;

                if (intptr_t(ptr) & 1)
                {
                    ptr--;
                    *(uint16*)ptr = *ptr | (ft_lightmap[g >> 16 << 8] << 8);
                    g += dgdx >> 1;
                    ptr += 2;
                    width--;
                }

                if (width & 1)
                {
                    *(uint16*)(ptr + width - 1) = (ptr[width] << 8) | ft_lightmap[Rg >> 16 << 8];
                }

                if (width & 2)
                {
                    uint8 p = ft_lightmap[g >> 16 << 8];
                    g += dgdx;
                    *(uint16*)ptr = p | (p << 8);
                    ptr += 2;
                }

                width >>= 2;
                while (width--)
                {
                    uint8 p;

                    p = ft_lightmap[g >> 16 << 8];
                    *(uint16*)ptr = p | (p << 8);
                    g += dgdx;
                    ptr += 2;

                    p = ft_lightmap[g >> 16 << 8];
                    *(uint16*)ptr = p | (p << 8);
                    g += dgdx;
                    ptr += 2;
                }
            }

            pixel += VRAM_WIDTH;

            Lx += Ldx;
            Rx += Rdx;
            Lg += Ldg;
            Rg += Rdg;
        }
    }
}

void rasterizeFT_c(uint16* pixel, const VertexLink* L, const VertexLink* R)
{
    const uint8* ft_lightmap = &lightmap[L->v.g << 8];

    int32 Lh = 0, Rh = 0;
    int32 Lx, Rx, Ldx = 0, Rdx = 0;
    uint32 Lt, Rt, Ldt, Rdt;
    Ldt = 0;
    Rdt = 0;

    while (1)
    {
        while (!Lh)
        {
            const VertexLink* N = L->prev;

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
            const VertexLink* N = R->next;

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
                    ptr--;
                    *(uint16*)ptr = *ptr | (ft_lightmap[tile[(t & 0xFF00) | (t >> 24)]] << 8);
                    ptr += 2;
                    t += dtdx;
                    width--;
                }

                if (width & 1)
                {
                    uint32 tmp = Rt - dtdx;
                    *(uint16*)(ptr + width - 1) = (ptr[width] << 8) | ft_lightmap[tile[(tmp & 0xFF00) | (tmp >> 24)]];
                }

                width >>= 1;
                while (width--)
                {
                    uint16 p;

                    p = ft_lightmap[tile[(t & 0xFF00) | (t >> 24)]];
                    t += dtdx;
                    p |= ft_lightmap[tile[(t & 0xFF00) | (t >> 24)]] << 8;
                    t += dtdx;
                    
                    *(uint16*)ptr = p;
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
    ASSERT((intptr_t(lightmap) & 0xFFFF) == 0); // lightmap should be 64k aligned
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
            const VertexLink* N = L->prev;

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
            const VertexLink* N = R->next;

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
                    ptr--;
                    *(uint16*)ptr = *ptr | (lightmap[(g >> 8 << 8) | tile[(t & 0xFF00) | (t >> 24)]] << 8);
                    ptr += 2;
                    t += dtdx;
                    g += dgdx >> 1;
                    width--;
                }

                if (width & 1)
                {
                    uint32 tmp = Rt - dtdx;
                    *(uint16*)(ptr + width - 1) = (ptr[width] << 8) | lightmap[(Rg >> 8 << 8) | tile[(tmp & 0xFF00) | (tmp >> 24)]];
                }

            #ifdef ALIGNED_LIGHTMAP
                g += intptr_t(lightmap);
            #endif

                width >>= 1;

                while (width--)
                {
                #ifdef ALIGNED_LIGHTMAP
                    const uint8* LMAP = (uint8*)(g >> 8 << 8); 

                    uint16 p = LMAP[tile[(t & 0xFF00) | (t >> 24)]];
                    t += dtdx;
                    p |= LMAP[tile[(t & 0xFF00) | (t >> 24)]] << 8;
                    t += dtdx;
                    g += dgdx;
                #else
                    uint16 p = lightmap[(g >> 8 << 8) | tile[(t & 0xFF00) | (t >> 24)]];
                    t += dtdx;
                    p |= lightmap[(g >> 8 << 8) | tile[(t & 0xFF00) | (t >> 24)]] << 8;
                    t += dtdx;
                    g += dgdx;
                #endif
                    
                    *(uint16*)ptr = p;
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
    const uint8* ft_lightmap = &lightmap[L->v.g << 8];

    int32 Lh = 0, Rh = 0;
    int32 Lx, Rx, Ldx = 0, Rdx = 0;
    uint32 Lt, Rt, Ldt, Rdt;
    Ldt = 0;
    Rdt = 0;

    while (1)
    {
        while (!Lh)
        {
            const VertexLink* N = L->prev;

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
            const VertexLink* N = R->next;

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
                    uint8 p = tile[(t & 0xFF00) | (t >> 24)];
                    ptr--;
                    if (p) {
                        *(uint16*)ptr = *ptr | (ft_lightmap[p] << 8);
                    }
                    ptr += 2;
                    t += dtdx;
                    width--;
                }

                if (width & 1)
                {
                    uint32 tmp = Rt - dtdx;
                    uint8 p = tile[(tmp & 0xFF00) | (tmp >> 24)];
                    if (p) {
                        *(uint16*)(ptr + width - 1) = (ptr[width] << 8) | ft_lightmap[p];
                    }
                }

                width >>= 1;
                while (width--)
                {
                    uint8 indexA = tile[(t & 0xFF00) | (t >> 24)];
                    t += dtdx;
                    uint8 indexB = tile[(t & 0xFF00) | (t >> 24)];
                    t += dtdx;

                    if (indexA && indexB) {
                        *(uint16*)ptr = ft_lightmap[indexA] | (ft_lightmap[indexB] << 8);
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
#ifdef ALIGNED_LIGHTMAP
    ASSERT((intptr_t(lightmap) & 0xFFFF) == 0); // lightmap should be 64k aligned
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
            const VertexLink* N = L->prev;

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
            const VertexLink* N = R->next;

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
                    ptr--;

                    uint8 indexB = tile[(t & 0xFF00) | (t >> 24)];

                    if (indexB) {
                        *(uint16*)ptr = *ptr | (lightmap[(g >> 8 << 8) | indexB] << 8);
                    }

                    ptr += 2;
                    t += dtdx;
                    g += dgdx >> 1;
                    width--;
                }

                if (width & 1)
                {
                    uint32 tmp = Rt - dtdx;

                    uint8 indexA = tile[(tmp & 0xFF00) | (tmp >> 24)];

                    if (indexA) {
                        *(uint16*)(ptr + width - 1) = (ptr[width] << 8) | lightmap[(Rg >> 8 << 8) | indexA];
                    }
                }

            #ifdef ALIGNED_LIGHTMAP
                g += intptr_t(lightmap);
            #endif

                width >>= 1;

                while (width--)
                {
                #ifdef ALIGNED_LIGHTMAP
                    uint8 indexA = tile[(t & 0xFF00) | (t >> 24)];
                    t += dtdx;
                    uint8 indexB = tile[(t & 0xFF00) | (t >> 24)];
                    t += dtdx;
                    g += dgdx;


                    if (indexA && indexB) {
                        const uint8* LMAP = (uint8*)(g >> 8 << 8); 
                        *(uint16*)ptr = LMAP[indexA] | (LMAP[indexB] << 8);
                    }
                #else
                    uint8 indexA = tile[(t & 0xFF00) | (t >> 24)];
                    t += dtdx;
                    uint8 indexB = tile[(t & 0xFF00) | (t >> 24)];
                    t += dtdx;
                    g += dgdx;

                    if (indexA && indexB) {
                        *(uint16*)ptr = lightmap[(g >> 8 << 8) | indexA] | (lightmap[(g >> 8 << 8) | indexB] << 8);
                    }
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

#endif

// TODO ARM version
void rasterizeSprite_c(uint16* pixel, const VertexLink* L, const VertexLink* R)
{
    const uint8* ft_lightmap = &lightmap[L->v.g << 8];

    int32 w = R->v.x - L->v.x;
    if (w <= 0 || w >= DIV_TABLE_SIZE) return;

    int32 h = R->v.y - L->v.y;
    if (h <= 0 || h >= DIV_TABLE_SIZE) return;

    int32 u = L->t.uv.u << 8;
    int32 v = L->t.uv.v << 8;

    int32 iw = FixedInvU(w);
    int32 ih = FixedInvU(h);

    int32 du = (R->t.uv.u << 8) * iw >> 16;
    int32 dv = (R->t.uv.v << 8) * ih >> 16;

    if (L->v.y < 0)
    {
        pixel -= L->v.y * VRAM_WIDTH;
        v -= L->v.y * dv;
        h += L->v.y;
    }

    if (R->v.y > FRAME_HEIGHT)
    {
        h -= R->v.y - FRAME_HEIGHT;
    }

    uint8* ptr = (uint8*)pixel;

    if (h <= 0) return;

    ptr += L->v.x;

    if (L->v.x < 0)
    {
        ptr -= L->v.x;
        u -= L->v.x * du;
        w += L->v.x;
    }

    if (R->v.x > FRAME_WIDTH)
    {
        w -= R->v.x - FRAME_WIDTH;
    }

    if (w <= 0) return;

    bool alignL = intptr_t(ptr) & 1;
    if (alignL)
    {
        ptr--;
        w--;
    }

    bool alignR = w & 1;

    w >>= 1;

    for (int32 y = 0; y < h; y++)
    {
        const uint8* xtile = tile + (v & 0xFF00);

        volatile uint8* xptr = ptr;

        int32 xu = u;

        if (alignL)
        {
            uint8 indexB = xtile[xu >> 8];
            if (indexB) {
                *(uint16*)xptr = *xptr | (ft_lightmap[indexB] << 8);
            }

            xptr += 2;
            xu += du;
        }

        for (int32 x = 0; x < w; x++)
        {
            uint8 indexA = xtile[xu >> 8];
            xu += du;
            uint8 indexB = xtile[xu >> 8];
            xu += du;

            if (indexA | indexB)
            {
                indexA = (indexA) ? ft_lightmap[indexA] : xptr[0];
                indexB = (indexB) ? ft_lightmap[indexB] : xptr[1];
                *(uint16*)xptr = indexA | (indexB << 8);
            }

            xptr += 2;
        }

        if (alignR)
        {
            uint8 indexA = xtile[xu >> 8];
            if (indexA) {
                *(uint16*)xptr = ft_lightmap[indexA] | (xptr[1] << 8);
            }
        }

        v += dv;

        ptr += FRAME_WIDTH;
    }
}

X_NOINLINE void rasterizeLineH_c(int32 x, int32 y, int32 width, int32 index)
{
    volatile uint8* ptr = (uint8*)fb + y * FRAME_WIDTH + x;

    if (intptr_t(ptr) & 1)
    {
        ptr--;
        *(uint16*)ptr = *ptr | (index << 8);
        ptr += 2;
        width--;
    }

    if (width & 1)
    {
        *(uint16*)(ptr + width - 1) = index | (ptr[width] << 8);
    }

    for (int32 i = 0; i < width / 2; i++)
    {
        *(uint16*)ptr = index | (index << 8);
        ptr += 2;
    }
}

X_NOINLINE void rasterizeLineV_c(int32 x, int32 y, int32 height, int32 index)
{
    volatile uint8* ptr = (uint8*)fb + y * FRAME_WIDTH + x;

    for (int32 i = 0; i < height; i++)
    {
        if (intptr_t(ptr) & 1) {
            *(uint16*)(ptr - 1) = *(ptr - 1) | (index << 8);
        } else {
            *(uint16*)ptr = index | (*ptr << 8);
        }
        ptr += FRAME_WIDTH;
    }
}

X_NOINLINE void rasterizeFillS_c(int32 x, int32 y, int32 width, int32 height, int32 shade)
{
    const uint8* lm = &lightmap[shade * 256];

    for (int32 i = 0; i < height; i++)
    {
        volatile uint8* ptr = (uint8*)fb + (y + i) * FRAME_WIDTH + x;
        int32 w = width;

        if (intptr_t(ptr) & 1)
        {
            ptr--;
            *(uint16*)ptr = ptr[0] | (lm[ptr[1]] << 8);
            ptr += 2;
            w--;
        }

        if (w & 1)
        {
            *(uint16*)(ptr + w - 1) = lm[ptr[w - 1]] | (ptr[w] << 8);
        }

        for (int32 i = 0; i < w / 2; i++)
        {
            *(uint16*)ptr = lm[ptr[0]] | (lm[ptr[1]] << 8);
            ptr += 2;
        }
    }
}

#endif
