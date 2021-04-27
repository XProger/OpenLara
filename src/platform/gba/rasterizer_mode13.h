#ifndef H_RASTERIZER_MODE13
#define H_RASTERIZER_MODE13

#include "common.h"

extern uint8 lightmap[256 * 32];
extern const uint8* tile;

#define rasterizeS rasterizeS_mode13_c
#define rasterizeF rasterizeF_mode13_c
#define rasterizeG rasterizeG_mode13_c
#define rasterizeFT rasterizeFT_mode13_c
#define rasterizeGT rasterizeGT_mode13_c
#define rasterizeFTA rasterizeFTA_mode13_c
#define rasterizeGTA rasterizeGTA_mode13_c

void rasterizeS_mode13_c(uint16* pixel, const VertexUV* L, const VertexUV* R)
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
            const VertexUV* N = L->prev;

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
            const VertexUV* N = R->next;

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

void rasterizeF_mode13_c(uint16* pixel, const VertexUV* L, const VertexUV* R, int32 index)
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
            const VertexUV* N = L->prev;

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
            const VertexUV* N = R->next;

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

void rasterizeG_mode13_c(uint16* pixel, const VertexUV* L, const VertexUV* R, int32 index)
{
    int32 Lh = 0, Rh = 0;
    int32 Lx, Rx, Ldx = 0, Rdx = 0;
    int32 Lg, Rg, Ldg = 0, Rdg = 0;

    const uint8* ft_lightmap = lightmap + index;

    while (1)
    {
        while (!Lh)
        {
            const VertexUV* N = L->prev;

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
            const VertexUV* N = R->next;

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
                    *ptr++ = ft_lightmap[g >> 16 << 8];
                    g += dgdx >> 1;
                    width--;
                }

                if (width & 1)
                {
                    ptr[width - 1] = ft_lightmap[Rg >> 16 << 8];
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

void rasterizeFT_mode13_c(uint16* pixel, const VertexUV* L, const VertexUV* R)
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
            const VertexUV* N = L->prev;

            if (N->v.y < L->v.y) return;

            Lh = N->v.y - L->v.y;
            Lx = L->v.x;
            Lt = L->t.uv;

            if (Lh > 1)
            {
                int32 tmp = FixedInvU(Lh);
                Ldx = tmp * (N->v.x - Lx);

                uint32 duv = N->t.uv - Lt;
                uint32 du = tmp * int16(duv >> 16);
                uint32 dv = tmp * int16(duv);
                Ldt = (du & 0xFFFF0000) | (dv >> 16);
            }

            Lx <<= 16;
            L = N;
        }

        while (!Rh) 
        {
            const VertexUV* N = R->next;

            if (N->v.y < R->v.y) return;

            Rh = N->v.y - R->v.y;
            Rx = R->v.x;
            Rt = R->t.uv;

            if (Rh > 1)
            {
                int32 tmp = FixedInvU(Rh);
                Rdx = tmp * (N->v.x - Rx);

                uint32 duv = N->t.uv - Rt;
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
                    *ptr++ = ft_lightmap[tile[(t & 0xFF00) | (t >> 24)]];
                    t += dtdx;
                    width--;
                }

                if (width & 1)
                {
                    uint32 tmp = Rt - dtdx;
                    ptr[width - 1] = ft_lightmap[tile[(tmp & 0xFF00) | (tmp >> 24)]];
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

void rasterizeGT_mode13_c(uint16* pixel, const VertexUV* L, const VertexUV* R)
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
    // for aligned by 64k address of lightmap array

    while (1)
    {
        while (!Lh)
        {
            const VertexUV* N = L->prev;

            if (N->v.y < L->v.y) return;

            Lh = N->v.y - L->v.y;
            Lx = L->v.x;
            Lg = L->v.g;
            Lt = L->t.uv;

            if (Lh > 1)
            {
                int32 tmp = FixedInvU(Lh);
                Ldx = tmp * (N->v.x - Lx);
                Ldg = tmp * (N->v.g - Lg) >> 8;

                uint32 duv = N->t.uv - Lt;
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
            const VertexUV* N = R->next;

            if (N->v.y < R->v.y) return;

            Rh = N->v.y - R->v.y;
            Rx = R->v.x;
            Rg = R->v.g;
            Rt = R->t.uv;

            if (Rh > 1)
            {
                int32 tmp = FixedInvU(Rh);
                Rdx = tmp * (N->v.x - Rx);
                Rdg = tmp * (N->v.g - Rg) >> 8;

                uint32 duv = N->t.uv - Rt;
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
                    *ptr++ = lightmap[(g >> 8 << 8) | tile[(t & 0xFF00) | (t >> 24)]];
                    t += dtdx;
                    g += dgdx >> 1;
                    width--;
                }

                if (width & 1)
                {
                    uint32 tmp = Rt - dtdx;
                    ptr[width - 1] = lightmap[(Rg >> 8 << 8) | tile[(tmp & 0xFF00) | (tmp >> 24)]];
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

void rasterizeFTA_mode13_c(uint16* pixel, const VertexUV* L, const VertexUV* R)
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
            const VertexUV* N = L->prev;

            if (N->v.y < L->v.y) return;

            Lh = N->v.y - L->v.y;
            Lx = L->v.x;
            Lt = L->t.uv;

            if (Lh > 1)
            {
                int32 tmp = FixedInvU(Lh);
                Ldx = tmp * (N->v.x - Lx);

                uint32 duv = N->t.uv - Lt;
                uint32 du = tmp * int16(duv >> 16);
                uint32 dv = tmp * int16(duv);
                Ldt = (du & 0xFFFF0000) | (dv >> 16);
            }

            Lx <<= 16;
            L = N;
        }

        while (!Rh) 
        {
            const VertexUV* N = R->next;

            if (N->v.y < R->v.y) return;

            Rh = N->v.y - R->v.y;
            Rx = R->v.x;
            Rt = R->t.uv;

            if (Rh > 1)
            {
                int32 tmp = FixedInvU(Rh);
                Rdx = tmp * (N->v.x - Rx);

                uint32 duv = N->t.uv - Rt;
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
                    uint8 p = tile[(tmp & 0xFF00) | (tmp >> 24)];
                    if (p) {
                        ptr[width - 1] = ft_lightmap[p];
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

void rasterizeGTA_mode13_c(uint16* pixel, const VertexUV* L, const VertexUV* R)
{
    rasterizeGT(pixel, L, R);
}

#endif
