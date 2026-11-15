#pragma once
#include "Params.h"

namespace fl
{
struct Preset
{
    const char* category;
    const char* name;
    std::vector<std::pair<int, float>> values; // overrides on top of the defaults
};

inline const std::vector<Preset>& factoryPresets();

inline int findPreset (const char* name)
{
    const auto& presets = factoryPresets();
    for (int i = 0; i < (int) presets.size(); ++i)
        if (juce::String (presets[(size_t) i].name) == name)
            return i;
    return -1;
}

inline const std::vector<Preset>& factoryPresets()
{
    using namespace P;

    static const std::vector<Preset> presets = {
        // ------------------------------------------------------------ Sub Drops & Booms
        { "SUB DROPS & BOOMS", "808 Abyss",
          { { imp_on, 0 }, { sub_start, 220 }, { sub_end, 32 }, { sub_pdecay, 0.5f }, { sub_decay, 5 }, { sub_punch, 0.5f }, { sub_harm, 0.3f } } },
        { "SUB DROPS & BOOMS", "Breakdown Drop",
          { { imp_on, 0 }, { sub_start, 400 }, { sub_end, 30 }, { sub_pdecay, 1.2f }, { sub_decay, 6 }, { sub_punch, 0.3f }, { sub_harm, 0.45f }, { sub_rev, 0.15f } } },
        { "SUB DROPS & BOOMS", "Short Boom",
          { { imp_on, 0 }, { sub_start, 140 }, { sub_end, 45 }, { sub_pdecay, 0.08f }, { sub_decay, 1.2f }, { sub_punch, 0.8f } } },
        { "SUB DROPS & BOOMS", "Drop Tuned Sub Hit",
          { { imp_on, 0 }, { sub_start, 260 }, { sub_end, 33 }, { sub_pdecay, 0.25f }, { sub_decay, 3.5f }, { sub_punch, 0.6f }, { sub_harm, 0.35f } } },
        { "SUB DROPS & BOOMS", "Sub Swell Up",
          { { imp_on, 0 }, { sub_start, 30 }, { sub_end, 62 }, { sub_pdecay, 1.5f }, { sub_decay, 3 }, { sub_punch, 0 }, { sub_harm, 0.3f } } },
        { "SUB DROPS & BOOMS", "Blackened Boom",
          { { sub_start, 200 }, { sub_end, 29 }, { sub_pdecay, 0.9f }, { sub_decay, 6 }, { sub_rev, 0.1f }, { imp_sample, 9 }, { imp_pitch, -12 }, { imp_rev, 0.5f },
            { rev_size, 0.85f } } },
        { "SUB DROPS & BOOMS", "Tight Kick Thump",
          { { imp_on, 0 }, { sub_start, 150 }, { sub_end, 55 }, { sub_pdecay, 0.06f }, { sub_decay, 0.35f }, { sub_punch, 0.9f }, { sub_harm, 0.2f } } },

        // ------------------------------------------------------------ Breakdown Impacts
        { "BREAKDOWN IMPACTS", "Cinema Slam",
          { { imp_sample, 2 }, { sub_start, 180 }, { sub_end, 36 }, { sub_decay, 3 }, { imp_rev, 0.3f } } },
        { "BREAKDOWN IMPACTS", "Taiko Wall",
          { { imp_sample, 0 }, { imp_pitch, -3 }, { imp_sustain, 0.5f }, { imp_rev, 0.5f }, { imp_level, -3 }, { rev_size, 0.7f }, { dist_type, 1 },
            { dist_drive, 0.3f }, { sub_decay, 1.8f } } },
        { "BREAKDOWN IMPACTS", "Gun Breakdown",
          { { imp_sample, 4 }, { imp_attack, 0.7f }, { imp_level, -4 }, { sub_start, 240 }, { sub_end, 38 }, { sub_pdecay, 0.12f }, { sub_decay, 1.4f },
            { sub_punch, 0.7f }, { rev_duck, 0.6f } } },
        { "BREAKDOWN IMPACTS", "Pipe Bomb",
          { { imp_sample, 8 }, { imp_pitch, -12 }, { imp_rev, 0.35f }, { dist_type, 2 }, { dist_drive, 0.35f }, { sub_start, 200 }, { sub_end, 33 }, { sub_decay, 4 } } },
        { "BREAKDOWN IMPACTS", "Reverse Slam",
          { { sub_on, 0 }, { imp_sample, 2 }, { imp_reverse, 1 }, { imp_rev, 0.5f }, { imp_level, -3 } } },
        { "BREAKDOWN IMPACTS", "Snare Bomb Hall",
          { { imp_sample, 10 }, { imp_rev, 0.7f }, { imp_level, -4 }, { rev_size, 0.8f }, { rev_duck, 0.55f }, { sub_start, 150 }, { sub_end, 44 }, { sub_decay, 1.2f } } },
        { "BREAKDOWN IMPACTS", "Trash Smash",
          { { sub_on, 0 }, { imp_sample, 10 }, { imp_pitch, 5 }, { imp_attack, 0.6f }, { imp_rev, 0.45f }, { imp_level, -3 }, { dist_type, 1 }, { dist_drive, 0.4f } } },
        { "BREAKDOWN IMPACTS", "Bell Of Doom",
          { { imp_sample, 1 }, { imp_pitch, -17 }, { imp_sustain, 0.7f }, { imp_rev, 0.7f }, { rev_size, 0.9f }, { sub_start, 110 }, { sub_end, 31 },
            { sub_pdecay, 0.5f }, { sub_decay, 6 } } },
        { "BREAKDOWN IMPACTS", "Wall Slam Wide",
          { { imp_sample, 9 }, { imp_pitch, -4 }, { imp_dly, 0.25f }, { imp_rev, 0.45f }, { dly_time, 6 }, { rev_mode, 1 }, { rev_size, 0.7f }, { sub_decay, 1.4f } } },
        { "BREAKDOWN IMPACTS", "Clank And Sub",
          { { imp_sample, 5 }, { imp_pitch, -12 }, { imp_attack, 0.4f }, { imp_rev, 0.3f }, { sub_start, 300 }, { sub_end, 34 }, { sub_pdecay, 0.4f }, { sub_decay, 4 } } },
        { "BREAKDOWN IMPACTS", "Taiko Low Tune",
          { { imp_sample, 0 }, { imp_pitch, -12 }, { imp_sustain, 0.6f }, { imp_rev, 0.5f }, { imp_level, -3 }, { sub_start, 130 }, { sub_end, 38 }, { sub_decay, 2.5f } } },
        { "BREAKDOWN IMPACTS", "Hybrid Trailer Hit",
          { { imp_sample, 2 }, { imp_rev, 0.4f }, { sub_start, 250 }, { sub_end, 30 }, { sub_pdecay, 0.7f }, { sub_decay, 6 }, { syn_on, 1 }, { syn_level, -9 },
            { syn_wt, 1 }, { syn_uni, 6 }, { syn_det, 0.5f }, { syn_pitch, 12 }, { syn_cut, 3000 }, { syn_a, 0.001f }, { syn_d, 0.6f }, { syn_s, 0 }, { syn_r, 0.6f },
            { syn_rev, 0.5f }, { rev_size, 0.8f } } },
        { "BREAKDOWN IMPACTS", "Breath Before Drop",
          { { sub_on, 0 }, { imp_sample, 6 }, { imp_rev, 0.7f }, { imp_level, -3 }, { rev_mode, 2 }, { rev_size, 0.5f }, { rev_duck, 0.2f } } },

        // ------------------------------------------------------------ Risers & Sweeps
        { "RISERS & SWEEPS", "Epic Riser - 4 Bars",
          { { sub_on, 0 }, { imp_on, 0 }, { ris_on, 1 }, { ris_len, 2 }, { ris_tone, 0.4f }, { ris_range, 36 }, { ris_rev, 0.5f }, { ris_level, -6 },
            { rev_size, 0.75f }, { dist_type, 1 }, { dist_drive, 0.3f } } },
        { "RISERS & SWEEPS", "Stutter Riser - 2 Bars",
          { { sub_on, 0 }, { imp_on, 0 }, { ris_on, 1 }, { ris_len, 1 }, { ris_gate, 2 }, { ris_tone, 0.45f }, { ris_level, -5 }, { ris_dly, 0.25f } } },
        { "RISERS & SWEEPS", "Triplet Gate Riser - 4 Bars",
          { { sub_on, 0 }, { imp_on, 0 }, { ris_on, 1 }, { ris_len, 2 }, { ris_gate, 4 }, { ris_tone, 0.6f }, { ris_range, 31 }, { ris_level, -5 } } },
        { "RISERS & SWEEPS", "Reverse Hit Riser - 1 Bar",
          { { sub_on, 0 }, { imp_sample, 2 }, { imp_reverse, 1 }, { imp_rev, 0.4f }, { ris_on, 1 }, { ris_len, 0 }, { ris_tone, 0.25f }, { ris_level, -9 } } },
        { "RISERS & SWEEPS", "Dark Riser - 2 Bars",
          { { sub_on, 0 }, { imp_on, 0 }, { ris_on, 1 }, { ris_len, 1 }, { ris_tone, 0.2f }, { ris_range, 12 }, { ris_rev, 0.6f }, { ris_level, -6 },
            { rev_size, 0.8f }, { dist_type, 1 }, { dist_drive, 0.35f } } },
        { "RISERS & SWEEPS", "Pump Riser 1/8 - 4 Bars",
          { { sub_on, 0 }, { imp_on, 0 }, { ris_on, 1 }, { ris_len, 2 }, { ris_gate, 1 }, { ris_tone, 0.4f }, { ris_rev, 0.3f }, { ris_level, -5 } } },
        { "RISERS & SWEEPS", "Bitcrushed Riser - 2 Bars",
          { { sub_on, 0 }, { imp_on, 0 }, { ris_on, 1 }, { ris_len, 1 }, { ris_tone, 0.6f }, { ris_level, -6 }, { dist_type, 3 }, { dist_drive, 0.45f } } },

        // ------------------------------------------------------------ Reese Basses & Drones
        { "REESE BASSES & DRONES", "Reese Classic",
          { { sub_on, 0 }, { imp_on, 0 }, { syn_on, 1 }, { syn_level, -6 }, { syn_wt, 0.2857f }, { syn_uni, 2 }, { syn_det, 0.35f }, { syn_cut, 900 },
            { syn_res, 0.25f }, { syn_s, 1 }, { syn_r, 0.3f }, { syn_rev, 0.1f }, { dist_drive, 0.3f } } },
        { "REESE BASSES & DRONES", "Reese Wide 8",
          { { sub_on, 0 }, { imp_on, 0 }, { syn_on, 1 }, { syn_level, -6 }, { syn_uni, 8 }, { syn_det, 0.5f }, { syn_cut, 1500 }, { syn_s, 1 }, { syn_rev, 0.15f } } },
        { "REESE BASSES & DRONES", "Dark Drone",
          { { sub_on, 0 }, { imp_on, 0 }, { syn_on, 1 }, { syn_level, -6 }, { syn_wt, 0.43f }, { syn_uni, 6 }, { syn_det, 0.6f }, { syn_cut, 400 },
            { syn_a, 1.5f }, { syn_s, 1 }, { syn_r, 3 }, { syn_ntype, 1 }, { syn_noise, 0.25f }, { syn_rev, 0.6f }, { rev_size, 0.8f },
            { flt_on, 1 }, { flt_cut, 2500 }, { flt_lfo_rate, 0.15f }, { flt_lfo_depth, 0.3f } } },
        { "REESE BASSES & DRONES", "Metal Drone",
          { { sub_on, 0 }, { imp_on, 0 }, { syn_on, 1 }, { syn_level, -6 }, { syn_wt, 0.857f }, { syn_uni, 4 }, { syn_det, 0.4f }, { syn_pitch, 12 },
            { syn_ntype, 2 }, { syn_noise, 0.3f }, { syn_ftype, 2 }, { syn_cut, 1200 }, { syn_res, 0.5f }, { syn_a, 2 }, { syn_s, 1 }, { syn_r, 4 },
            { syn_rev, 0.5f }, { rev_mode, 1 }, { rev_size, 0.8f } } },
        { "REESE BASSES & DRONES", "Noise Bed",
          { { sub_on, 0 }, { imp_on, 0 }, { syn_on, 1 }, { syn_level, -6 }, { syn_wt, 0 }, { syn_uni, 1 }, { syn_noise, 1 }, { syn_ftype, 2 }, { syn_cut, 3000 },
            { syn_res, 0.45f }, { syn_a, 1 }, { syn_s, 1 }, { syn_r, 2 }, { syn_rev, 0.5f }, { syn_pitch, 24 }, { rev_mode, 2 } } },
        { "REESE BASSES & DRONES", "Sub Reese Stack",
          { { imp_on, 0 }, { sub_start, 60 }, { sub_end, 33 }, { sub_pdecay, 0.1f }, { sub_decay, 8 }, { sub_punch, 0.1f }, { syn_on, 1 }, { syn_level, -9 },
            { syn_uni, 4 }, { syn_det, 0.45f }, { syn_pitch, 12 }, { syn_cut, 1100 }, { syn_s, 1 }, { dist_type, 1 }, { dist_drive, 0.35f } } },
        { "REESE BASSES & DRONES", "Horror Strings",
          { { sub_on, 0 }, { imp_on, 0 }, { syn_on, 1 }, { syn_level, -7 }, { syn_uni, 8 }, { syn_det, 0.7f }, { syn_pitch, 24 }, { syn_cut, 2500 }, { syn_a, 2.5f },
            { syn_s, 1 }, { syn_r, 4 }, { syn_rev, 0.7f }, { rev_mode, 2 }, { rev_size, 0.4f }, { rev_duck, 0.1f } } },
        { "REESE BASSES & DRONES", "Pink Wind",
          { { sub_on, 0 }, { imp_on, 0 }, { syn_on, 1 }, { syn_level, -4 }, { syn_wt, 0 }, { syn_uni, 1 }, { syn_ntype, 1 }, { syn_noise, 1 }, { syn_cut, 800 },
            { syn_res, 0.5f }, { syn_a, 2 }, { syn_s, 1 }, { syn_r, 3 }, { syn_rev, 0.5f }, { flt_on, 1 }, { flt_cut, 1500 }, { flt_lfo_rate, 0.1f }, { flt_lfo_depth, 0.4f } } },
        { "REESE BASSES & DRONES", "Industrial Hum",
          { { sub_on, 0 }, { imp_on, 0 }, { syn_on, 1 }, { syn_level, -5 }, { syn_wt, 0.57f }, { syn_uni, 2 }, { syn_det, 0.05f }, { syn_ntype, 2 }, { syn_noise, 0.15f },
            { syn_cut, 350 }, { syn_res, 0.6f }, { syn_a, 0.3f }, { syn_s, 1 }, { syn_r, 1.5f }, { syn_rev, 0.25f }, { dist_type, 1 }, { dist_drive, 0.5f } } },

        // ------------------------------------------------------------ Glitches & Tape Stops
        { "GLITCHES & TAPE STOPS", "Tape Stop Drone",
          { { sub_on, 0 }, { imp_on, 0 }, { syn_on, 1 }, { syn_level, -6 }, { syn_wt, 0.43f }, { syn_uni, 6 }, { syn_det, 0.6f }, { syn_cut, 900 }, { syn_a, 0.5f },
            { syn_s, 1 }, { syn_r, 2 }, { syn_rev, 0.4f }, { tape_time, 2.5f } } },
        { "GLITCHES & TAPE STOPS", "Half-Gate Riser - 2 Bars",
          { { sub_on, 0 }, { imp_on, 0 }, { ris_on, 1 }, { ris_len, 1 }, { ris_tone, 0.5f }, { ris_level, -5 }, { ris_rev, 0.3f }, { gate_rate, 2 }, { gate_depth, 0.6f } } },
        { "GLITCHES & TAPE STOPS", "Broken Radio",
          { { sub_on, 0 }, { imp_sample, 11 }, { imp_reverse, 1 }, { imp_dly, 0.4f }, { imp_level, -3 }, { dist_type, 3 }, { dist_drive, 0.7f }, { flt_on, 1 },
            { flt_type, 2 }, { flt_cut, 1800 }, { flt_res, 0.5f } } },
        { "GLITCHES & TAPE STOPS", "Machine Gun Gate",
          { { sub_on, 0 }, { imp_sample, 4 }, { imp_sustain, 1 }, { imp_level, -3 }, { imp_rev, 0.25f }, { gate_rate, 5 }, { dist_type, 2 }, { dist_drive, 0.3f } } },
    };

    return presets;
}
} // namespace fl
