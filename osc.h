/*
    BSD 3-Clause License

    Copyright (c) 2023, KORG INC.
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, this
      list of conditions and the following disclaimer.

    * Redistributions in binary form must reproduce the above copyright notice,
      this list of conditions and the following disclaimer in the documentation
      and/or other materials provided with the distribution.

    * Neither the name of the copyright holder nor the names of its
      contributors may be used to endorse or promote products derived from
      this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
    AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
    FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
    DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
    SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
    CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
    OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
    OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

//*/

/*
 *  File: osc.h
 *
 *  Dummy oscillator template instance.
 *
 */
#pragma once

#include "processor.h"
#include "unit_osc.h"
#include <cstdint>
#include <cstring>

// ============================================================================
//  Constants (from unit.cc)
// ============================================================================

static constexpr uint32_t WAVE_LEN_OSC       = 32;
static constexpr uint32_t NUM_PRESETS_OSC    = 48;
static constexpr uint32_t NUM_USER_SLOTS_OSC = 6;
static constexpr uint32_t TOTAL_BANKS_OSC    = NUM_PRESETS_OSC + NUM_USER_SLOTS_OSC;

#define q31_to_f32_c 4.65661287307739e-010f
#define q31_to_f32(q) ((float)(q) * q31_to_f32_c)

// preset_waves は unit.cc と同内容をここにも定義する
static const int8_t osc_preset_waves[16][WAVE_LEN_OSC] = {
    {   0,  24,  48,  70,  89, 105, 117, 124,
      127, 124, 117, 105,  89,  70,  48,  24,
        0, -24, -48, -70, -89,-105,-117,-124,
     -128,-124,-117,-105, -89, -70, -48, -24 },
    { 127, 127, 127, 127, 127, 127, 127, 127,
      127, 127, 127, 127, 127, 127, 127, 127,
     -128,-128,-128,-128,-128,-128,-128,-128,
     -128,-128,-128,-128,-128,-128,-128,-128 },
    { -128,-120,-112,-104, -96, -88, -80, -72,
       -64, -56, -48, -40, -32, -24, -16,  -8,
         0,   8,  16,  24,  32,  40,  48,  56,
        64,  72,  80,  88,  96, 104, 112, 120 },
    { -128,-112, -96, -80, -64, -48, -32, -16,
         0,  16,  32,  48,  64,  80,  96, 112,
       127, 112,  96,  80,  64,  48,  32,  16,
         0, -16, -32, -48, -64, -80, -96,-112 },
    { 127, 127, 127, 127, 127, 127, 127, 127,
     -128,-128,-128,-128,-128,-128,-128,-128,
     -128,-128,-128,-128,-128,-128,-128,-128,
     -128,-128,-128,-128,-128,-128,-128,-128 },
    { 127, 127, 127, 127,-128,-128,-128,-128,
     -128,-128,-128,-128,-128,-128,-128,-128,
     -128,-128,-128,-128,-128,-128,-128,-128,
     -128,-128,-128,-128,-128,-128,-128,-128 },
    { 127, 112,  80,  48,  16,  -8, -32, -56,
      -72, -88, -96,-104,-112,-120,-128,-120,
     -104, -80, -56, -32,  -8,  16,  40,  64,
       80,  96, 104, 112, 120, 124, 127, 127 },
    {  64, 120, 127, 100,  48,   0, -32, -80,
     -120,-128,-100, -56, -16,  16,  32,  16,
       -8, -40, -72, -96,-112,-128,-112, -80,
      -32,   8,  40,  72,  96, 112, 120,  96 },
    {   0,  80, 120, 127,  96,  40, -16, -64,
     -100,-120,-127,-112, -80, -32,  16,  56,
       80,  96, 100,  88,  64,  32,   0, -40,
      -72, -96,-112,-120,-127,-112, -80, -32 },
    {   0,  96,  64, -32,-120, -80,  16, 112,
      100,   8, -88,-127, -64,  24, 104, 127,
       72, -16,-100,-120, -56,  32, 108, 120,
       60, -24, -96,-127, -80,  -8,  64, 112 },
    { 127, -64,  96,-128,  48, -96,  80, -48,
      112, -80,  32,-112,  64, -32, 120, -64,
       16,-120,  80, -16, 104, -80,  48,-104,
       96, -48,  64,-128, 127, -96,  32, -64 },
    {   0,  16,  40,  64,  88, 104, 120, 127,
      124, 112,  96,  72,  48,  24,   0, -24,
      -48, -72, -96,-112,-124,-127,-120,-104,
      -88, -64, -40, -16,   8,  16,   8,   0 },
    { 127, 127, 120,  96,  48,   0, -48, -96,
     -120,-128,-128,-120, -96, -48,   0,  48,
       96, 120, 127, 120,  80,  32, -16, -64,
     -104,-128,-120, -96, -56, -16,  32,  80 },
    { 127,  64,-128,  64, 127, -64,-128,  64,
      127,   0,-128,   0, 127, -64,-128,  64,
      127,  64,-128,   0, 127, -64,-128,  64,
      127,  32,-128, -32, 127,   0,-128,   0 },
    {   0,  48,  88, 116, 127, 120,  96,  56,
        8, -40, -80,-108,-124,-127,-116, -92,
      -56, -12,  32,  72, 104, 124, 127, 116,
       88,  48,   0, -48, -88,-116,-127,-120 },
    { 127, 100,  32, -40, -96,-120,-100, -48,
       16,  72, 108, 120, 100,  56,   0, -48,
      -80,-100,-108,-100, -72, -32,   8,  48,
       80, 100, 112, 116, 108,  88,  60,  24 },
};

// ============================================================================
//  Osc class
// ============================================================================

class Osc : public Processor
{
public:
    uint32_t getBufferSize() const override final { return 0; }

    // ---- 内部状態 (unit.cc の s 構造体に相当) ----
    struct State {
        int8_t  banks[TOTAL_BANKS_OSC][WAVE_LEN_OSC];
        int8_t  stage_a[WAVE_LEN_OSC];
        int8_t  stage_b[WAVE_LEN_OSC];
        int8_t  write_buf[WAVE_LEN_OSC];

        float   phase;
        float   phase2;
        float   w0;

        float   morph;
        uint8_t bank_a;
        uint8_t bank_b;
        uint8_t interp_mode;
        uint8_t bit_depth;
        float   detune;

        uint8_t write_target;
        uint8_t write_bank;
        uint8_t write_index;
        uint8_t write_count;

        uint8_t op_mode;
        uint8_t fold_type;
        float   fold_drive;
        float   sub_mix;
        float   phase_sub;
    };

    void init(float *) override final
    {
        w0_  = 0.f;
        lfo_ = 0.f;

        memset(&s_, 0, sizeof(s_));

        for (uint32_t i = 0; i < 16; i++)
            memcpy(s_.banks[i], osc_preset_waves[i], WAVE_LEN_OSC);
        genHarmonicPresets();
        for (uint32_t i = 0; i < NUM_USER_SLOTS_OSC; i++)
            memcpy(s_.banks[NUM_PRESETS_OSC + i], osc_preset_waves[0], WAVE_LEN_OSC);

        loadStageA(0);
        loadStageB(0);
    }

    void reset() override final
    {
        s_.phase     = 0.f;
        s_.phase2    = 0.f;
        s_.phase_sub = 0.f;
    }

    void setPitch(float w0)
    {
        w0_ = w0;
    }

    void setShapeLfo(float lfo)
    {
        lfo_ = lfo;
    }

    void noteOn(uint8_t /*note*/, uint8_t /*velo*/) override final {}
    void noteOff(uint8_t /*note*/) override final {}

    void setParameter(uint8_t index, int32_t value) override final
    {
        applyParam(index, value);
    }

    const char *getParameterStrValue(uint8_t /*index*/, int32_t /*value*/) const override final
    {
        return nullptr;
    }

    void process(const float *__restrict /*in*/, float *__restrict out, uint32_t frames) override final
    {
        // w0 は setPitch() で設定済み (f/samplerate 単位)
        const float w0 = w0_;

        float morph = s_.morph;
        morph = clipminmaxf(0.f, morph + lfo_, 1.f);

        const bool  detune_on = (s_.detune > 0.001f);
        const float w0_2      = detune_on ? w0 * (1.f + s_.detune * 0.000577789f) : w0;

        float ph     = s_.phase;
        float ph2    = s_.phase2;
        float ph_sub = s_.phase_sub;
        const float  w0_sub   = w0 * 0.5f;
        const bool   sub_on   = (s_.sub_mix > 0.001f);
        const float  sub_mix  = s_.sub_mix;
        const uint8_t ip      = s_.interp_mode;
        const uint8_t bd      = s_.bit_depth;
        const int8_t *ta      = s_.stage_a;
        const int8_t *tb      = s_.stage_b;

        for (uint32_t i = 0; i < frames; i++) {
            float sig = linintf(morph, readWt(ta, ph, ip), readWt(tb, ph, ip));

            if (detune_on) {
                float sig2 = linintf(morph, readWt(ta, ph2, ip), readWt(tb, ph2, ip));
                sig = (sig + sig2) * 0.5f;
            }

            sig = quantize(sig, bd);

            if (sub_on) {
                float sig_sub = readWt(ta, ph_sub, ip);
                sig = sig * (1.f - sub_mix) + sig_sub * sub_mix;
            }

            out[i] = clipminmaxf(-1.f, sig, 1.f);

            ph += w0;
            ph -= (uint32_t)ph;
            if (detune_on) {
                ph2 += w0_2;
                ph2 -= (uint32_t)ph2;
            }
            ph_sub += w0_sub;
            ph_sub -= (uint32_t)ph_sub;
        }

        s_.phase     = ph;
        s_.phase2    = ph2;
        s_.phase_sub = ph_sub;
    }

private:
    State   s_;
    float   w0_  = 0.f;
    float   lfo_ = 0.f;

    // ---- helpers ----

    void genHarmonicPresets()
    {
        for (uint32_t p = 0; p < 32; p++) {
            const uint32_t idx = 16 + p;
            uint32_t selector = p + 1;
            float h[6] = {1.f, 0.f, 0.f, 0.f, 0.f, 0.f};
            if (selector & 0x01) h[1] = 0.7f;
            if (selector & 0x02) h[2] = 0.6f;
            if (selector & 0x04) h[3] = 0.5f;
            if (selector & 0x08) h[4] = 0.4f;
            if (selector & 0x10) h[5] = 0.3f;
            float norm = 0.f;
            for (int k = 0; k < 6; k++) norm += h[k];
            for (uint32_t i = 0; i < WAVE_LEN_OSC; i++) {
                float v = 0.f;
                const float ph = (float)i / (float)WAVE_LEN_OSC;
                for (int k = 0; k < 6; k++)
                    if (h[k] > 0.f) v += h[k] * osc_sinf(ph * (float)(k + 1));
                if (norm > 0.f) v /= norm;
                s_.banks[idx][i] = (int8_t)(v * 127.f);
            }
        }
    }

    void loadStageA(uint8_t b)
    {
        if (b >= TOTAL_BANKS_OSC) b = 0;
        s_.bank_a = b;
        memcpy(s_.stage_a, s_.banks[b], WAVE_LEN_OSC);
    }

    void loadStageB(uint8_t b)
    {
        if (b >= TOTAL_BANKS_OSC) b = 0;
        s_.bank_b = b;
        memcpy(s_.stage_b, s_.banks[b], WAVE_LEN_OSC);
    }

    void commitWrite()
    {
        switch (s_.write_target) {
        case 0: memcpy(s_.stage_a, s_.write_buf, WAVE_LEN_OSC); break;
        case 1: memcpy(s_.stage_b, s_.write_buf, WAVE_LEN_OSC); break;
        case 2:
            if (s_.write_bank < NUM_USER_SLOTS_OSC)
                memcpy(s_.banks[NUM_PRESETS_OSC + s_.write_bank], s_.write_buf, WAVE_LEN_OSC);
            break;
        }
        s_.write_count = 0;
    }

    static inline float readWt(const int8_t *tbl, float phase, uint8_t interp)
    {
        const float    fi = phase * (float)WAVE_LEN_OSC;
        const uint32_t i0 = (uint32_t)fi & (WAVE_LEN_OSC - 1);
        const float    s0 = (float)tbl[i0] * 0.0078125f;
        if (interp == 0) return s0;
        const uint32_t i1 = (i0 + 1) & (WAVE_LEN_OSC - 1);
        return linintf(fi - (float)(uint32_t)fi, s0, (float)tbl[i1] * 0.0078125f);
    }

    static inline float quantize(float v, uint8_t d)
    {
        if (d == 0) return v;
        const float lv = (float)(1 << ((d < 7) ? (8 - d) : 1));
        return (float)((int32_t)(v * lv + 0.5f)) / lv;
    }

    void applyParam(uint8_t id, int32_t value)
    {
        switch (id) {
        case k_unit_osc_fixed_param_shape:
            loadStageA((uint8_t)((value * 63) / 1023));
            break;
        case k_unit_osc_fixed_param_altshape:
            s_.morph = param_val_to_f32(value);
            break;
        case k_num_unit_osc_fixed_param_id + 0:
            loadStageB((uint8_t)(value & 0x3F));
            break;
        case k_num_unit_osc_fixed_param_id + 1:
            s_.bit_depth   = (uint8_t)(value & 0x07);
            s_.interp_mode = (uint8_t)((value >> 3) & 0x01);
            s_.fold_type   = (uint8_t)((value >> 4) & 0x01);
            break;
        case k_num_unit_osc_fixed_param_id + 2:
            s_.detune = (float)value * 0.5f;
            break;
        case k_num_unit_osc_fixed_param_id + 3:
            if (value >= 240) {
                s_.write_target = 3;
                s_.write_index  = (uint8_t)(value - 240);
                s_.write_count  = 0;
            } else if (value < 16) {
                s_.write_target = 0;
                s_.write_index  = (uint8_t)(value * 2);
                s_.write_count  = 0;
                memcpy(s_.write_buf, s_.stage_a, WAVE_LEN_OSC);
            } else if (value < 32) {
                s_.write_target = 1;
                s_.write_index  = (uint8_t)((value - 16) * 2);
                s_.write_count  = 0;
                memcpy(s_.write_buf, s_.stage_b, WAVE_LEN_OSC);
            } else {
                s_.write_target = 2;
                uint8_t off     = (uint8_t)(value - 32);
                s_.write_bank   = off / 16;
                s_.write_index  = (off % 16) * 2;
                s_.write_count  = 0;
                if (s_.write_bank < NUM_USER_SLOTS_OSC)
                    memcpy(s_.write_buf, s_.banks[NUM_PRESETS_OSC + s_.write_bank], WAVE_LEN_OSC);
            }
            break;
        case k_num_unit_osc_fixed_param_id + 4:
        {
            if (s_.write_target == 3) {
                uint8_t v = (uint8_t)(value & 0xFF);
                switch (s_.write_index) {
                case 0: s_.sub_mix    = (float)(v > 100 ? 100 : v) * 0.01f; break;
                case 1: { uint8_t f = (v > 100) ? 100 : v; s_.fold_drive = (float)f * 0.04f; break; }
                case 2: s_.op_mode    = (v > 5) ? 5 : v; break;
                }
                s_.write_target = 0;
                break;
            }
            int8_t sample = (int8_t)((value & 0xFF) - 128);
            if (s_.write_index < WAVE_LEN_OSC) {
                s_.write_buf[s_.write_index] = sample;
                s_.write_index = (s_.write_index + 1) & (WAVE_LEN_OSC - 1);
                if (++s_.write_count >= WAVE_LEN_OSC)
                    commitWrite();
            }
            break;
        }
        case k_num_unit_osc_fixed_param_id + 5:
        {
            s_.op_mode = (uint8_t)((value >> 7) & 0x07);
            if (s_.op_mode > 5) s_.op_mode = 5;
            uint8_t fold_raw = (uint8_t)(value & 0x7F);
            if (fold_raw > 100) fold_raw = 100;
            s_.fold_drive = (float)fold_raw * 0.04f;
            break;
        }
        case k_num_unit_osc_fixed_param_id + 6:
            s_.sub_mix = (float)value * 0.01f;
            break;
        default: break;
        }
    }
};
