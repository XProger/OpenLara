#ifndef H_RASTERIZER_MODE5
#define H_RASTERIZER_MODE5

#include "common.h"

#define rasterizeS rasterizeS_mode5_c
#define rasterizeF rasterizeF_mode5_c
#define rasterizeG rasterizeG_mode5_c
#define rasterizeFT rasterizeFT_mode5_c
#define rasterizeGT rasterizeGT_mode5_c
#define rasterizeFTA rasterizeFTA_mode5_c
#define rasterizeGTA rasterizeGTA_mode5_c
#define rasterizeSprite rasterizeSprite_mode5_c

extern uint16 palette[256];
extern uint8 lightmap[256 * 32];
extern const uint8* tile;

void rasterize_overdraw(uint16* pixel, const VertexUV* L, const VertexUV* R)
{
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
                uint16* ptr = pixel + x1;

                while (width--)
                {
                    *ptr++ += 0x1084;
                }
            }

            pixel += VRAM_WIDTH;

            Lx += Ldx;
            Rx += Rdx;
        }
    }
}

void rasterizeS_mode5_c(uint16* pixel, const VertexUV* L, const VertexUV* R)
{
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
                uint16* ptr = pixel + x1;

                while (1)
                {
                    *ptr = (*ptr >> 1) & 0b11110111101111;
                    ptr++;
                    if (!--width) break;

                    *ptr = (*ptr >> 1) & 0b11110111101111;
                    ptr++;
                    if (!--width) break;
                }
            }

            pixel += VRAM_WIDTH;

            Lx += Ldx;
            Rx += Rdx;
        }
    }
}

void rasterizeF_mode5_c(uint16* pixel, const VertexUV* L, const VertexUV* R, int32 index)
{
    uint32 color = palette[lightmap[(L->v.g << 8) | index]];

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
                uint16* ptr = pixel + x1;

                switch (width & 7)
                {
                    case 7: *ptr++ = color;
                    case 6: *ptr++ = color;
                    case 5: *ptr++ = color;
                    case 4: *ptr++ = color;
                    case 3: *ptr++ = color;
                    case 2: *ptr++ = color;
                    case 1: *ptr++ = color;
                }

                int32 n = width >> 3;

                while (n--) {
                    *ptr++ = color;
                    *ptr++ = color;
                    *ptr++ = color;
                    *ptr++ = color;
                    *ptr++ = color;
                    *ptr++ = color;
                    *ptr++ = color;
                    *ptr++ = color;
                }
            }

            pixel += VRAM_WIDTH;

            Lx += Ldx;
            Rx += Rdx;
        }
    }
}

void rasterizeG_mode5_c(uint16* pixel, const VertexUV* L, const VertexUV* R, int32 index)
{
    const uint8* ft_lightmap = lightmap + index; // faster with global variable than local

    int32 Lh = 0;
    int32 Rh = 0;
    int32 Ldx = 0;
    int32 Rdx = 0;
    int32 Lx;
    int32 Rx;
    int32 Lg;
    int32 Rg;
    int32 Ldg = 0;
    int32 Rdg = 0;

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

            if (Rh > 1) {
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
                uint16* ptr = pixel + x1;

                int32 d = FixedInvU(width);
                int32 dgdx = d * ((Rg - Lg) >> 5) >> 10;

                int32 g = Lg;

                while (1) // 2px per iteration (faster than 8px on C)
                {
                    uint32 color = palette[ft_lightmap[(g >> 8) & 0x1F00]];

                    *ptr++ = color;
                    if (!--width) break;
        
                    *ptr++ = color;
                    if (!--width) break;

                    g += dgdx;
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

void rasterizeFT_mode5_c(uint16* pixel, const VertexUV* L, const VertexUV* R)
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
                uint16* ptr = pixel + x1;

                int32 d = FixedInvU(width);

                uint32 duv = Rt - Lt;
                uint32 u = d * int16(duv >> 16);
                uint32 v = d * int16(duv);
                uint32 dtdx = (u & 0xFFFF0000) | (v >> 16);

                uint32 t = Lt;

                while (1)
                {
                    *ptr++ = palette[ft_lightmap[tile[(t & 0xFF00) | (t >> 24)]]];
                    if (!--width) break;
                    t += dtdx;
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

void rasterizeGT_mode5_c(uint16* pixel, const VertexUV* L, const VertexUV* R)
{
    int32 Lh = 0, Rh = 0;
    int32 Lx, Rx, Lg, Rg, Ldx = 0, Rdx = 0, Ldg = 0, Rdg = 0;
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
            Lg = L->v.g;
            Lt = L->t.uv;

            if (Lh > 1)
            {
                int32 tmp = FixedInvU(Lh);
                Ldx = tmp * (N->v.x - Lx);
                Ldg = tmp * (N->v.g - Lg);

                uint32 duv = N->t.uv - Lt;
                uint32 du = tmp * int16(duv >> 16);
                uint32 dv = tmp * int16(duv);
                Ldt = (du & 0xFFFF0000) | (dv >> 16);
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
            Rt = R->t.uv;

            if (Rh > 1)
            {
                int32 tmp = FixedInvU(Rh);
                Rdx = tmp * (N->v.x - Rx);
                Rdg = tmp * (N->v.g - Rg);

                uint32 duv = N->t.uv - Rt;
                uint32 du = tmp * int16(duv >> 16);
                uint32 dv = tmp * int16(duv);
                Rdt = (du & 0xFFFF0000) | (dv >> 16);
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
                uint16* ptr = pixel + x1;

                uint32 tmp = FixedInvU(width);

                int32 dgdx = tmp * ((Rg - Lg) >> 5) >> 10;

                uint32 duv = Rt - Lt;
                uint32 u = tmp * int16(duv >> 16);
                uint32 v = tmp * int16(duv);
                uint32 dtdx = (u & 0xFFFF0000) | (v >> 16);

                int32 g = Lg;
                uint32 t = Lt;

                while (1)
                {
                    *ptr++ = palette[lightmap[((g >> 8) & 0x1F00) | tile[(t & 0xFF00) | (t >> 24)]]];
                    if (!--width) break;
                    t += dtdx;
                    
                    *ptr++ = palette[lightmap[((g >> 8) & 0x1F00) | tile[(t & 0xFF00) | (t >> 24)]]];
                    if (!--width) break;
                    t += dtdx;

                    g += dgdx;
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

void rasterizeFTA_mode5_c(uint16* pixel, const VertexUV* L, const VertexUV* R)
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
                uint16* ptr = pixel + x1;

                int32 d = FixedInvU(width);

                uint32 duv = Rt - Lt;
                uint32 u = d * int16(duv >> 16);
                uint32 v = d * int16(duv);
                uint32 dtdx = (u & 0xFFFF0000) | (v >> 16);

                uint32 t = Lt;

                while (1)
                {
                    uint16 p = palette[ft_lightmap[tile[(t & 0xFF00) | (t >> 24)]]];
                    if (p) *ptr = p;
                    ptr++;
                    if (!--width) break;
                    t += dtdx;
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

void rasterizeGTA_mode5_c(uint16* pixel, const VertexUV* L, const VertexUV* R)
{
    int32 Lh = 0, Rh = 0;
    int32 Lx, Rx, Lg, Rg, Ldx = 0, Rdx = 0, Ldg = 0, Rdg = 0;
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
            Lg = L->v.g;
            Lt = L->t.uv;

            if (Lh > 1)
            {
                int32 tmp = FixedInvU(Lh);
                Ldx = tmp * (N->v.x - Lx);
                Ldg = tmp * (N->v.g - Lg);

                uint32 duv = N->t.uv - Lt;
                uint32 du = tmp * int16(duv >> 16);
                uint32 dv = tmp * int16(duv);
                Ldt = (du & 0xFFFF0000) | (dv >> 16);
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
            Rt = R->t.uv;

            if (Rh > 1)
            {
                int32 tmp = FixedInvU(Rh);
                Rdx = tmp * (N->v.x - Rx);
                Rdg = tmp * (N->v.g - Rg);

                uint32 duv = N->t.uv - Rt;
                uint32 du = tmp * int16(duv >> 16);
                uint32 dv = tmp * int16(duv);
                Rdt = (du & 0xFFFF0000) | (dv >> 16);
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
                uint16* ptr = pixel + x1;

                uint32 tmp = FixedInvU(width);

                int32 dgdx = tmp * ((Rg - Lg) >> 5) >> 10;

                uint32 duv = Rt - Lt;
                uint32 u = tmp * int16(duv >> 16);
                uint32 v = tmp * int16(duv);
                uint32 dtdx = (u & 0xFFFF0000) | (v >> 16);

                int32 g = Lg;
                uint32 t = Lt;

                while (1)
                {
                    uint16 p;
                    p = palette[lightmap[((g >> 8) & 0x1F00) | tile[(t & 0xFF00) | (t >> 24)]]];
                    if (p) *ptr = p;
                    ptr++;
                    if (!--width) break;
                    t += dtdx;
                    
                    p = palette[lightmap[((g >> 8) & 0x1F00) | tile[(t & 0xFF00) | (t >> 24)]]];
                    if (p) *ptr = p;
                    ptr++;
                    if (!--width) break;
                    t += dtdx;

                    g += dgdx;
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

void rasterizeSprite_mode5_c(uint16* pixel, const VertexUV* L, const VertexUV* R)
{
    // TODO
}

#endif
