/*
 * WT32-OSC: 32-Byte Programmable Wavetable Oscillator
 * Target: KORG NTS-1 digital kit mkII (logue SDK v2)
 *
 * unit.cc: SDK v2 glue layer.
 * DSP implementation lives in osc.h (Osc class).
 */

#include "unit_osc.h"
#include "utils/int_math.h"
#include "osc.h"

#include <cstdint>
#include <cstring>

// ============================================================================
//  State (glue-side only)
// ============================================================================

static const unit_runtime_osc_context_t *s_context = nullptr;
static int32_t s_cached_values[UNIT_OSC_MAX_PARAM_COUNT];
static Osc     s_processor;

// ============================================================================
//  SDK v2 Callbacks
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

    // Initialize DSP processor (loads preset banks, resets state)
    s_processor.init(nullptr);

    // Apply all init values to processor
    for (int id = 0; id < UNIT_OSC_MAX_PARAM_COUNT; ++id) {
        s_processor.setParameter(static_cast<uint8_t>(id), s_cached_values[id]);
    }

    return k_unit_err_none;
}

__unit_callback void unit_teardown()
{
    s_context = nullptr;
}

__unit_callback void unit_reset()
{
    s_processor.reset();
}

__unit_callback void unit_resume() { }
__unit_callback void unit_suspend() { }

__unit_callback void unit_render(const float *in, float *out, uint32_t frames)
{
    (void)in;

    if (s_context) {
        const float w0 = osc_w0f_for_note(
            (uint8_t)(s_context->pitch >> 8),
            (uint8_t)(s_context->pitch & 0xFF));
        s_processor.setPitch(w0);
        s_processor.setShapeLfo(q31_to_f32(s_context->shape_lfo));
    } else {
        s_processor.setShapeLfo(0.f);
    }

    s_processor.process(nullptr, out, frames);
}

__unit_callback void unit_set_param_value(uint8_t id, int32_t value)
{
    if (id >= unit_header.num_params) return;  // [L-2] id範囲チェック

    value = clipminmaxi32(unit_header.params[id].min, value, unit_header.params[id].max);
    s_cached_values[id] = value;
    s_processor.setParameter(id, value);
}

__unit_callback int32_t unit_get_param_value(uint8_t id)
{
    if (id >= unit_header.num_params) return 0;  // [L-3] id範囲チェック
    return s_cached_values[id];
}

__unit_callback const char *unit_get_param_str_value(uint8_t id, int32_t value)
{
    return s_processor.getParameterStrValue(id, value);
}

__unit_callback void unit_note_on(uint8_t note, uint8_t velo)
{
    s_processor.noteOn(note, velo);
}

__unit_callback void unit_note_off(uint8_t note)
{
    s_processor.noteOff(note);
}

__unit_callback void unit_all_note_off() { }
__unit_callback void unit_pitch_bend(uint16_t /*bend*/) { }
__unit_callback void unit_channel_pressure(uint8_t /*press*/) { }
__unit_callback void unit_aftertouch(uint8_t /*note*/, uint8_t /*press*/) { }
__unit_callback void unit_set_tempo(uint32_t /*tempo*/) { }
__unit_callback void unit_tempo_4ppqn_tick(uint32_t /*counter*/) { }
