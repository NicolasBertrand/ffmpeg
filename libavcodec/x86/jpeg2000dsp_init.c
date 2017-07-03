/*
 * SIMD optimized JPEG 2000 DSP functions
 * Copyright (c) 2015 James Almer
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "libavutil/attributes.h"
#include "libavutil/cpu.h"
#include "libavutil/x86/cpu.h"
#include "libavcodec/jpeg2000dsp.h"
#include "libavcodec/jpeg2000dwt.h"

void ff_ict_float_sse(void *src0, void *src1, void *src2, int csize);
void ff_ict_float_avx(void *src0, void *src1, void *src2, int csize);
void ff_rct_int_sse2 (void *src0, void *src1, void *src2, int csize);
void ff_rct_int_avx2 (void *src0, void *src1, void *src2, int csize);

void ff_sr_1d97_float_sse(float *line, int i0, int i1);
void ff_hor_sd_float_sse(float *line, float *data, int mh, int lh, int lv, int w);

av_cold void ff_jpeg2000dsp_init_x86(Jpeg2000DSPContext *c)
{
    int cpu_flags = av_get_cpu_flags();
    if (EXTERNAL_SSE(cpu_flags)) {
        c->mct_decode[FF_DWT97] = ff_ict_float_sse;
    }

    if (EXTERNAL_SSE2(cpu_flags)) {
        c->mct_decode[FF_DWT53] = ff_rct_int_sse2;
    }

    if (EXTERNAL_AVX_FAST(cpu_flags)) {
        c->mct_decode[FF_DWT97] = ff_ict_float_avx;
    }

    if (EXTERNAL_AVX2_FAST(cpu_flags)) {
        c->mct_decode[FF_DWT53] = ff_rct_int_avx2;
    }
}

av_cold void ff_jpeg2000dwt_init_x86(DWTContext *s, int type)
{
    int cpu_flags = av_get_cpu_flags();
    if (EXTERNAL_SSE(cpu_flags)) {
        if (type == FF_DWT97){
            dwt_decode = dwt_decode97_float_sse;
        }
    }
}

void dwt_decode97_float_sse(DWTContext *s, float *t)
{
    int lev;
    int w       = s->linelen[s->ndeclevels - 1][0];
    float *line = s->f_linebuf;
    float *data = t;
    /* position at index O of line range [0-5,w+5] cf. extend function */
    line += 5;

    int i, j = 0;

    for (lev = 0; lev < s->ndeclevels; lev++) {
        int lh = s->linelen[lev][0],
            lv = s->linelen[lev][1],
            mh = s->mod[lev][0],
            mv = s->mod[lev][1],
            lp;
        float *l;
        // HOR_SD
        ff_hor_sd_float_sse(line, data, mh, lh, lv, w);

        // VER_SD
        l = line + mv;
        for (lp = 0; lp < lh; lp++) {
            //printf("hello \n");
            i = 0;
            j = 0;
            // copy with interleaving
            for (i = mv; i < lv; i += 2, j++)
                l[i] = data[w * j + lp];
            for (i = 1 - mv; i < lv; i += 2, j++)
                l[i] = data[w * j + lp];

            ff_sr_1d97_float_sse(line, mv, mv + lv);

            for (i = 0; i < lv; i++)
                data[w * i + lp] = l[i];
        }
    }
}
