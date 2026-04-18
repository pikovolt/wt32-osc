/*
 * WT32-OSC: Unit Header
 *
 * params[0] = A knob (shape)    -> BnkA
 * params[1] = B knob (altshape) -> Mrph
 * params[2..6] = encoder params -> BnkB, Ctrl, DTun, WrAd, WrDt
 * params[7..10] = unused (empty descriptors required by SDK)
 */

#include "unit_osc.h"

#define WT32_DEV_ID  (0x57543332)  // "WT32"

const __attribute__((section(".unit_header"))) unit_header_t unit_header = {
    .header_size = sizeof(unit_header_t),
    .target      = UNIT_TARGET_PLATFORM | k_unit_module_osc,
    .api         = UNIT_API_VERSION,
    .dev_id      = WT32_DEV_ID,
    .unit_id     = 0x00010000,
    .version     = 0x00020000,  // [M-1] version bump: 1.0.0 -> 2.0.0
    .name        = "WT32",
    .reserved0   = 0x0,
    .reserved1   = 0x0,
    .num_params  = 7,
    .params = {
        /* [0] A knob (shape): BnkA - Stage A wave bank select
         * Platform sends 0-1023 (10bit). Internally scaled to 0-53. */
        { .min    = 0,
          .max    = 1023,
          .center = 0,
          .init   = 0,
          .type   = k_unit_param_type_none,
          .frac   = 0,
          .frac_mode = k_unit_param_frac_mode_fixed,
          .reserved  = 0,
          .name   = "BnkA" },

        /* [1] B knob (altshape): Mrph - A<>B morph position
         * Platform sends 0-1023 (10bit). Internally scaled to 0.0-1.0. */
        { .min    = 0,
          .max    = 1023,
          .center = 0,
          .init   = 0,
          .type   = k_unit_param_type_none,
          .frac   = 0,
          .frac_mode = k_unit_param_frac_mode_fixed,
          .reserved  = 0,
          .name   = "Mrph" },

        /* [2] encoder: BnkB - Stage B wave bank select */
        { .min    = 0,
          .max    = 53,   // [M-1] 63->53 (TOTAL_BANKS-1)
          .center = 0,
          .init   = 1,
          .type   = k_unit_param_type_enum,
          .frac   = 0,
          .frac_mode = k_unit_param_frac_mode_fixed,
          .reserved  = 0,
          .name   = "BnkB" },

        /* [3] encoder: Ctrl - bit[2:0]=bit-depth, bit[3]=interp */
        { .min    = 0,
          .max    = 15,
          .center = 0,
          .init   = 0,
          .type   = k_unit_param_type_enum,
          .frac   = 0,
          .frac_mode = k_unit_param_frac_mode_fixed,
          .reserved  = 0,
          .name   = "Ctrl" },

        /* [4] encoder: DTun - detune 0-50 cents */
        { .min    = 0,
          .max    = 100,
          .center = 0,
          .init   = 0,
          .type   = k_unit_param_type_cents,
          .frac   = 0,
          .frac_mode = k_unit_param_frac_mode_fixed,
          .reserved  = 0,
          .name   = "DTun" },

        /* [5] CC: WrAd - register write address */
        { .min    = 0,
          .max    = 255,
          .center = 0,
          .init   = 0,
          .type   = k_unit_param_type_enum,
          .frac   = 0,
          .frac_mode = k_unit_param_frac_mode_fixed,
          .reserved  = 0,
          .name   = "WrAd" },

        /* [6] CC: WrDt - register write data */
        { .min    = 0,
          .max    = 255,
          .center = 128,
          .init   = 128,
          .type   = k_unit_param_type_enum,
          .frac   = 0,
          .frac_mode = k_unit_param_frac_mode_fixed,
          .reserved  = 0,
          .name   = "WrDt" },

        /* [7] unused */
        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
        /* [8] unused */
        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
        /* [9] unused */
        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
        /* [10] unused */
        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
    }
};
