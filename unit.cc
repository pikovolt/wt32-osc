/*
 * WT32-OSC: 32-Byte Programmable Wavetable Oscillator
 * Target: KORG NTS-1 digital kit mkII (logue SDK v2)
 */

#include "unit_osc.h"
#include "utils/int_math.h"

#include <cstdint>
#include <cstring>

// ============================================================================
//  Constants
// ============================================================================

static constexpr uint32_t WAVE_LEN       = 32;
static constexpr uint32_t NUM_PRESETS    = 48;
static constexpr uint32_t NUM_USER_SLOTS = 16;
static constexpr uint32_t TOTAL_BANKS    = NUM_PRESETS + NUM_USER_SLOTS;

// from fixed_math.h
#define q31_to_f32_c 4.65661287307739e-010f
#define q31_to_f32(q) ((float)(q) * q31_to_f32_c)

// ============================================================================
//  Preset Wavetables (signed 8-bit, 32 samples each)
// ============================================================================

static const int8_t preset_waves[16][WAVE_LEN] = {
    {   0,  24,  48,  70,  89, 105, 117, 124,      // 0: Sine
      127, 124, 117, 105,  89,  70,  48,  24,
        0, -24, -48, -70, -89,-105,-117,-124,
     -128,-124,-117,-105, -89, -70, -48, -24 },
    { 127, 127, 127, 127, 127, 127, 127, 127,      // 1: Square
      127, 127, 127, 127, 127, 127, 127, 127,
     -128,-128,-128,-128,-128,-128,-128,-128,
     -128,-128,-128,-128,-128,-128,-128,-128 },
    { -128,-120,-112,-104, -96, -88, -80, -72,      // 2: Sawtooth
       -64, -56, -48, -40, -32, -24, -16,  -8,
         0,   8,  16,  24,  32,  40,  48,  56,
        64,  72,  80,  88,  96, 104, 112, 120 },
    { -128,-112, -96, -80, -64, -48, -32, -16,      // 3: Triangle
         0,  16,  32,  48,  64,  80,  96, 112,
       127, 112,  96,  80,  64,  48,  32,  16,
         0, -16, -32, -48, -64, -80, -96,-112 },
    { 127, 127, 127, 127, 127, 127, 127, 127,      // 4: Pulse 25%
     -128,-128,-128,-128,-128,-128,-128,-128,
     -128,-128,-128,-128,-128,-128,-128,-128,
     -128,-128,-128,-128,-128,-128,-128,-128 },
    { 127, 127, 127, 127,-128,-128,-128,-128,      // 5: Pulse 12.5%
     -128,-128,-128,-128,-128,-128,-128,-128,
     -128,-128,-128,-128,-128,-128,-128,-128,
     -128,-128,-128,-128,-128,-128,-128,-128 },
    { 127, 112,  80,  48,  16,  -8, -32, -56,      // 6: Brass
      -72, -88, -96,-104,-112,-120,-128,-120,
     -104, -80, -56, -32,  -8,  16,  40,  64,
       80,  96, 104, 112, 120, 124, 127, 127 },
    {  64, 120, 127, 100,  48,   0, -32, -80,      // 7: Strings
     -120,-128,-100, -56, -16,  16,  32,  16,
       -8, -40, -72, -96,-112,-128,-112, -80,
      -32,   8,  40,  72,  96, 112, 120,  96 },
    {   0,  80, 120, 127,  96,  40, -16, -64,      // 8: Organ
     -100,-120,-127,-112, -80, -32,  16,  56,
       80,  96, 100,  88,  64,  32,   0, -40,
      -72, -96,-112,-120,-127,-112, -80, -32 },
    {   0,  96,  64, -32,-120, -80,  16, 112,      // 9: Bell
      100,   8, -88,-127, -64,  24, 104, 127,
       72, -16,-100,-120, -56,  32, 108, 120,
       60, -24, -96,-127, -80,  -8,  64, 112 },
    { 127, -64,  96,-128,  48, -96,  80, -48,      // 10: Metallic
      112, -80,  32,-112,  64, -32, 120, -64,
       16,-120,  80, -16, 104, -80,  48,-104,
       96, -48,  64,-128, 127, -96,  32, -64 },
    {   0,  16,  40,  64,  88, 104, 120, 127,      // 11: Soft pad
      124, 112,  96,  72,  48,  24,   0, -24,
      -48, -72, -96,-112,-124,-127,-120,-104,
      -88, -64, -40, -16,   8,  16,   8,   0 },
    { 127, 127, 120,  96,  48,   0, -48, -96,      // 12: Bass
     -120,-128,-128,-120, -96, -48,   0,  48,
       96, 120, 127, 120,  80,  32, -16, -64,
     -104,-128,-120, -96, -56, -16,  32,  80 },
    { 127,  64,-128,  64, 127, -64,-128,  64,      // 13: Harsh lead
      127,   0,-128,   0, 127, -64,-128,  64,
      127,  64,-128,   0, 127, -64,-128,  64,
      127,  32,-128, -32, 127,   0,-128,   0 },
    {   0,  48,  88, 116, 127, 120,  96,  56,      // 14: Whistle
        8, -40, -80,-108,-124,-127,-116, -92,
      -56, -12,  32,  72, 104, 124, 127, 116,
       88,  48,   0, -48, -88,-116,-127,-120 },
    { 127, 100,  32, -40, -96,-120,-100, -48,      // 15: Vowel-A
       16,  72, 108, 120, 100,  56,   0, -48,
      -80,-100,-108,-100, -72, -32,   8,  48,
       80, 100, 112, 116, 108,  88,  60,  24 },
};

// ============================================================================
//  State
// ============================================================================

static const unit_runtime_osc_context_t *s_context = nullptr;
static int32_t s_cached_values[UNIT_OSC_MAX_PARAM_COUNT];

static struct {
    int8_t  banks[TOTAL_BANKS][WAVE_LEN];
    int8_t  stage_a[WAVE_LEN];
    int8_t  stage_b[WAVE_LEN];
    int8_t  write_buf[WAVE_LEN];

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
} s;

// ============================================================================
//  Helpers
// ============================================================================

static void gen_harmonic_presets()
{
    for (uint32_t p = 0; p < 32; p++) {
        const uint32_t idx = 16 + p;
        // Start from p+1 so bank 16 (p=0) is not pure sine
        // p=0: h2 only, p=1: h3 only, p=2: h2+h3, ...
        uint32_t selector = p + 1;
        float h[6] = {1.f, 0.f, 0.f, 0.f, 0.f, 0.f};
        if (selector & 0x01) h[1] = 0.7f;
        if (selector & 0x02) h[2] = 0.6f;
        if (selector & 0x04) h[3] = 0.5f;
        if (selector & 0x08) h[4] = 0.4f;
        if (selector & 0x10) h[5] = 0.3f;

        float norm = 0.f;
        for (int k = 0; k < 6; k++) norm += h[k];

        for (uint32_t i = 0; i < WAVE_LEN; i++) {
            float v = 0.f;
            const float ph = (float)i / (float)WAVE_LEN;
            for (int k = 0; k < 6; k++) {
                if (h[k] > 0.f)
                    v += h[k] * osc_sinf(ph * (float)(k + 1));
            }
            if (norm > 0.f) v /= norm;
            s.banks[idx][i] = (int8_t)(v * 127.f);
        }
    }
}

static void load_stage_a(uint8_t b)
{
    if (b >= TOTAL_BANKS) b = 0;
    s.bank_a = b;
    memcpy(s.stage_a, s.banks[b], WAVE_LEN);
}

static void load_stage_b(uint8_t b)
{
    if (b >= TOTAL_BANKS) b = 0;
    s.bank_b = b;
    memcpy(s.stage_b, s.banks[b], WAVE_LEN);
}

static void commit_write()
{
    switch (s.write_target) {
    case 0: memcpy(s.stage_a, s.write_buf, WAVE_LEN); break;
    case 1: memcpy(s.stage_b, s.write_buf, WAVE_LEN); break;
    case 2:
        if (s.write_bank < NUM_USER_SLOTS)
            memcpy(s.banks[NUM_PRESETS + s.write_bank], s.write_buf, WAVE_LEN);
        break;
    }
    s.write_count = 0;
}

static inline float read_wt(const int8_t *tbl, float phase, uint8_t interp)
{
    const float fi = phase * (float)WAVE_LEN;
    const uint32_t i0 = (uint32_t)fi & (WAVE_LEN - 1);
    const float s0 = (float)tbl[i0] * 0.0078125f;
    if (interp == 0)
        return s0;
    const uint32_t i1 = (i0 + 1) & (WAVE_LEN - 1);
    return linintf(fi - (float)(uint32_t)fi, s0, (float)tbl[i1] * 0.0078125f);
}

static inline float quantize(float v, uint8_t d)
{
    if (d == 0) return v;
    const float lv = (float)(1 << ((d < 7) ? (8 - d) : 1));
    return (float)((int32_t)(v * lv + 0.5f)) / lv;
}

// ============================================================================
//  Apply parameter to internal state
//  Called from both unit_init (for defaults) and unit_set_param_value
// ============================================================================

static void apply_param(uint8_t id, int32_t value)
{
    switch (id) {

    // params[0] = A knob (shape) -> Bank A select
    // Platform sends 0-1023. Scale to 0-63.
    case k_unit_osc_fixed_param_shape:
        load_stage_a((uint8_t)((value * 63) / 1023));
        break;

    // params[1] = B knob (altshape) -> Morph
    // Platform sends 0-1023. Scale to 0.0-1.0.
    case k_unit_osc_fixed_param_altshape:
        s.morph = param_val_to_f32(value);
        break;

    // params[2] = BnkB
    case k_num_unit_osc_fixed_param_id + 0:
        load_stage_b((uint8_t)(value & 0x3F));
        break;

    // params[3] = Ctrl
    case k_num_unit_osc_fixed_param_id + 1:
        s.bit_depth   = (uint8_t)(value & 0x07);
        s.interp_mode = (uint8_t)((value >> 3) & 0x01);
        break;

    // params[4] = DTun
    case k_num_unit_osc_fixed_param_id + 2:
        s.detune = (float)value * 0.5f;
        break;

    // params[5] = WrAd
    case k_num_unit_osc_fixed_param_id + 3:
        if (value < 32) {
            s.write_target = 0;  s.write_index = (uint8_t)value;
        } else if (value < 64) {
            s.write_target = 1;  s.write_index = (uint8_t)(value - 32);
        } else {
            s.write_target = 2;
            uint8_t off = (uint8_t)(value - 64);
            s.write_bank  = off / 32;
            s.write_index = off % 32;
        }
        s.write_count = 0;
        switch (s.write_target) {
        case 0: memcpy(s.write_buf, s.stage_a, WAVE_LEN); break;
        case 1: memcpy(s.write_buf, s.stage_b, WAVE_LEN); break;
        case 2:
            if (s.write_bank < NUM_USER_SLOTS)
                memcpy(s.write_buf, s.banks[NUM_PRESETS + s.write_bank], WAVE_LEN);
            break;
        }
        break;

    // params[6] = WrDt
    case k_num_unit_osc_fixed_param_id + 4:
    {
        int8_t sample = (int8_t)((value & 0xFF) - 128);
        if (s.write_index < WAVE_LEN) {
            s.write_buf[s.write_index] = sample;
            s.write_index = (s.write_index + 1) & (WAVE_LEN - 1);
            if (++s.write_count >= WAVE_LEN)
                commit_write();
        }
        break;
    }

    default: break;
    }
}

// ============================================================================
//  SDK v2 Callbacks (following dummy-osc template)
// ============================================================================

__unit_callback int8_t unit_init(const unit_runtime_desc_t *desc)
{
    if (!desc)
        return k_unit_err_undef;

    if (desc->target != unit_header.target)
        return k_unit_err_target;

    if (!UNIT_API_IS_COMPAT(desc->api))
        return k_unit_err_api_version;

    if (desc->samplerate != 48000)
        return k_unit_err_samplerate;

    if (desc->input_channels != 2 || desc->output_channels != 1)
        return k_unit_err_geometry;

    s_context = static_cast<const unit_runtime_osc_context_t *>(desc->hooks.runtime_context);

    // Initialize cached params from header defaults
    for (int id = 0; id < UNIT_OSC_MAX_PARAM_COUNT; ++id) {
        s_cached_values[id] = static_cast<int32_t>(unit_header.params[id].init);
    }

    // Initialize internal state
    memset(&s, 0, sizeof(s));

    // Load preset wave banks
    for (uint32_t i = 0; i < 16; i++)
        memcpy(s.banks[i], preset_waves[i], WAVE_LEN);
    gen_harmonic_presets();
    for (uint32_t i = 0; i < NUM_USER_SLOTS; i++)
        memcpy(s.banks[NUM_PRESETS + i], preset_waves[0], WAVE_LEN);

    // Apply all init values to internal state
    for (int id = 0; id < UNIT_OSC_MAX_PARAM_COUNT; ++id) {
        apply_param(static_cast<uint8_t>(id), s_cached_values[id]);
    }

    return k_unit_err_none;
}

__unit_callback void unit_teardown()
{
    s_context = nullptr;
}

__unit_callback void unit_reset()
{
    s.phase = 0.f;
    s.phase2 = 0.f;
}

__unit_callback void unit_resume() { }
__unit_callback void unit_suspend() { }

__unit_callback void unit_render(const float *in, float *out, uint32_t frames)
{
    (void)in;

    // Pitch from runtime context
    float w0 = s.w0;
    if (s_context) {
        w0 = osc_w0f_for_note((uint8_t)(s_context->pitch >> 8),
                               (uint8_t)(s_context->pitch & 0xFF));
        s.w0 = w0;
    }

    // Shape LFO modulation on morph
    float morph = s.morph;
    if (s_context) {
        const float lfo = q31_to_f32(s_context->shape_lfo);
        morph = clipminmaxf(0.f, morph + lfo, 1.f);
    }

    const bool detune_on = (s.detune > 0.001f);
    const float w0_2 = detune_on ? w0 * (1.f + s.detune * 0.000577789f) : w0;

    float ph  = s.phase;
    float ph2 = s.phase2;
    const uint8_t ip = s.interp_mode;
    const uint8_t bd = s.bit_depth;
    const int8_t *ta = s.stage_a;
    const int8_t *tb = s.stage_b;

    for (uint32_t i = 0; i < frames; i++) {
        float sig = linintf(morph, read_wt(ta, ph, ip), read_wt(tb, ph, ip));

        if (detune_on) {
            float sig2 = linintf(morph, read_wt(ta, ph2, ip), read_wt(tb, ph2, ip));
            sig = (sig + sig2) * 0.5f;
        }

        sig = quantize(sig, bd);
        out[i] = clipminmaxf(-1.f, sig, 1.f);

        ph += w0;
        ph -= (uint32_t)ph;
        if (detune_on) {
            ph2 += w0_2;
            ph2 -= (uint32_t)ph2;
        }
    }

    s.phase  = ph;
    s.phase2 = ph2;
}

__unit_callback void unit_set_param_value(uint8_t id, int32_t value)
{
    // Clip to valid range as defined in header
    value = clipminmaxi32(unit_header.params[id].min, value, unit_header.params[id].max);

    // Cache value for unit_get_param_value
    s_cached_values[id] = value;

    // Apply to internal state
    apply_param(id, value);
}

__unit_callback int32_t unit_get_param_value(uint8_t id)
{
    return s_cached_values[id];
}

__unit_callback const char *unit_get_param_str_value(uint8_t id, int32_t value)
{
    (void)id;
    (void)value;
    return nullptr;
}

__unit_callback void unit_note_on(uint8_t note, uint8_t velo)
{
    (void)note;
    (void)velo;
}

__unit_callback void unit_note_off(uint8_t note)
{
    (void)note;
}

__unit_callback void unit_all_note_off() { }

__unit_callback void unit_pitch_bend(uint16_t bend)
{
    (void)bend;
}

__unit_callback void unit_channel_pressure(uint8_t press)
{
    (void)press;
}

__unit_callback void unit_aftertouch(uint8_t note, uint8_t press)
{
    (void)note;
    (void)press;
}

__unit_callback void unit_set_tempo(uint32_t tempo)
{
    (void)tempo;
}

__unit_callback void unit_tempo_4ppqn_tick(uint32_t counter)
{
    (void)counter;
}