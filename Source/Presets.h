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
        { "SUB DROPS & BOOMS", "Earthquake",
          { { imp_on, 0 }, { sub_wave, 2 }, { sub_start, 90 }, { sub_end, 26 }, { sub_pdecay, 2.5f }, { sub_decay, 9 }, { sub_harm, 0.7f }, { dist_type, 1 }, { dist_drive, 0.4f } } },
        { "SUB DROPS & BOOMS", "Tri Sub Dive",
          { { imp_on, 0 }, { sub_wave, 1 }, { sub_start, 600 }, { sub_end, 36 }, { sub_pdecay, 0.9f }, { sub_decay, 4 }, { sub_harm, 0.15f } } },
        { "SUB DROPS & BOOMS", "Nuke",
          { { sub_start, 300 }, { sub_end, 28 }, { sub_pdecay, 1.8f }, { sub_decay, 8 }, { sub_harm, 0.5f }, { imp_sample, 4 }, { imp_rev, 0.6f },
            { rev_mode, 2 }, { rev_size, 0.7f }, { dist_drive, 0.35f } } },
        { "SUB DROPS & BOOMS", "Low Tuned Thud",
          { { imp_on, 0 }, { sub_start, 110 }, { sub_end, 41 }, { sub_pdecay, 0.05f }, { sub_decay, 0.7f }, { sub_punch, 1.0f }, { sub_harm, 0.55f }, { dist_type, 2 }, { dist_drive, 0.3f } } },

        // ------------------------------------------------------------ Breakdown Impacts
        { "BREAKDOWN IMPACTS", "Cinema Slam",
          { { imp_sample, 2 }, { sub_start, 180 }, { sub_end, 36 }, { sub_decay, 3 }, { imp_rev, 0.3f } } },
        { "BREAKDOWN IMPACTS", "Anvil Strike",
          { { imp_sample, 1 }, { imp_pitch, -5 }, { imp_attack, 0.5f }, { imp_rev, 0.4f }, { sub_start, 160 }, { sub_end, 40 }, { sub_pdecay, 0.15f },
            { sub_decay, 1.5f }, { rev_mode, 1 }, { rev_size, 0.5f } } },
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
        { "BREAKDOWN IMPACTS", "Industrial Clank",
          { { imp_sample, 5 }, { imp_pitch, -7 }, { imp_dly, 0.3f }, { imp_rev, 0.3f }, { dist_type, 3 }, { dist_drive, 0.25f }, { rev_mode, 1 }, { sub_decay, 1.6f } } },
        { "BREAKDOWN IMPACTS", "Door To Hell",
          { { imp_sample, 9 }, { imp_pitch, -9 }, { imp_sustain, 0.6f }, { imp_rev, 0.65f }, { rev_mode, 2 }, { rev_size, 0.45f }, { sub_start, 130 },
            { sub_end, 30 }, { sub_pdecay, 0.8f }, { sub_decay, 5 }, { sub_harm, 0.4f } } },

        // ------------------------------------------------------------ Risers & Sweeps
        { "RISERS & SWEEPS", "Noise Riser - 1 Bar",
          { { sub_on, 0 }, { imp_on, 0 }, { ris_on, 1 }, { ris_len, 0 }, { ris_tone, 0.1f }, { ris_level, -6 } } },
        { "RISERS & SWEEPS", "Tension Riser - 2 Bars",
          { { sub_on, 0 }, { imp_on, 0 }, { ris_on, 1 }, { ris_len, 1 }, { ris_tone, 0.5f }, { ris_range, 24 }, { ris_rev, 0.4f }, { ris_level, -6 } } },
        { "RISERS & SWEEPS", "Epic Riser - 4 Bars",
          { { sub_on, 0 }, { imp_on, 0 }, { ris_on, 1 }, { ris_len, 2 }, { ris_tone, 0.4f }, { ris_range, 36 }, { ris_rev, 0.5f }, { ris_level, -6 },
            { rev_size, 0.75f }, { dist_type, 1 }, { dist_drive, 0.3f } } },
        { "RISERS & SWEEPS", "Mega Build - 8 Bars",
          { { sub_on, 0 }, { imp_on, 0 }, { ris_on, 1 }, { ris_len, 3 }, { ris_tone, 0.55f }, { ris_range, 48 }, { ris_rev, 0.45f }, { ris_level, -6 },
            { rev_mode, 2 }, { rev_size, 0.4f } } },
        { "RISERS & SWEEPS", "Stutter Riser - 2 Bars",
          { { sub_on, 0 }, { imp_on, 0 }, { ris_on, 1 }, { ris_len, 1 }, { ris_gate, 2 }, { ris_tone, 0.45f }, { ris_level, -5 }, { ris_dly, 0.25f } } },
        { "RISERS & SWEEPS", "Triplet Gate Riser - 4 Bars",
          { { sub_on, 0 }, { imp_on, 0 }, { ris_on, 1 }, { ris_len, 2 }, { ris_gate, 4 }, { ris_tone, 0.6f }, { ris_range, 31 }, { ris_level, -5 } } },
        { "RISERS & SWEEPS", "Downlifter - 2 Bars",
          { { sub_on, 0 }, { imp_on, 0 }, { ris_on, 1 }, { ris_mode, 1 }, { ris_len, 1 }, { ris_tone, 0.3f }, { ris_rev, 0.5f }, { ris_level, -6 } } },
        { "RISERS & SWEEPS", "Whoosh Sweep - 1 Bar",
          { { sub_on, 0 }, { imp_on, 0 }, { ris_on, 1 }, { ris_mode, 2 }, { ris_len, 0 }, { ris_curve, 0 }, { ris_tone, 0.05f }, { ris_rev, 0.35f }, { ris_level, -6 } } },
        { "RISERS & SWEEPS", "Reverse Hit Riser - 1 Bar",
          { { sub_on, 0 }, { imp_sample, 2 }, { imp_reverse, 1 }, { imp_rev, 0.4f }, { ris_on, 1 }, { ris_len, 0 }, { ris_tone, 0.25f }, { ris_level, -9 } } },

        // ------------------------------------------------------------ Reese Basses & Drones
        { "REESE BASSES & DRONES", "Reese Classic",
          { { sub_on, 0 }, { imp_on, 0 }, { syn_on, 1 }, { syn_level, -6 }, { syn_wt, 0.2857f }, { syn_uni, 2 }, { syn_det, 0.35f }, { syn_cut, 900 },
            { syn_res, 0.25f }, { syn_s, 1 }, { syn_r, 0.3f }, { syn_rev, 0.1f }, { dist_drive, 0.3f } } },
        { "REESE BASSES & DRONES", "Reese Wide 8",
          { { sub_on, 0 }, { imp_on, 0 }, { syn_on, 1 }, { syn_level, -6 }, { syn_uni, 8 }, { syn_det, 0.5f }, { syn_cut, 1500 }, { syn_s, 1 }, { syn_rev, 0.15f } } },
        { "REESE BASSES & DRONES", "Djent Growl",
          { { sub_on, 0 }, { imp_on, 0 }, { syn_on, 1 }, { syn_level, -7 }, { syn_wt, 1.0f }, { syn_uni, 3 }, { syn_det, 0.2f }, { syn_cut, 2200 },
            { syn_res, 0.4f }, { syn_s, 1 }, { syn_rev, 0.05f }, { dist_type, 2 }, { dist_drive, 0.5f } } },
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

        // ------------------------------------------------------------ Glitches & Tape Stops
        { "GLITCHES & TAPE STOPS", "Gated Reese 1/16",
          { { sub_on, 0 }, { imp_on, 0 }, { syn_on, 1 }, { syn_level, -6 }, { syn_uni, 3 }, { syn_det, 0.4f }, { syn_cut, 1800 }, { syn_s, 1 },
            { gate_rate, 2 }, { gate_depth, 1 }, { dist_drive, 0.35f } } },
        { "GLITCHES & TAPE STOPS", "Triplet Chop",
          { { sub_on, 0 }, { imp_sample, 10 }, { imp_sustain, 0.8f }, { imp_rev, 0.5f }, { rev_size, 0.7f }, { rev_duck, 0 }, { gate_rate, 5 }, { gate_depth, 1 } } },
        { "GLITCHES & TAPE STOPS", "Bitcrush Static",
          { { sub_on, 0 }, { imp_sample, 11 }, { imp_dly, 0.4f }, { imp_level, -3 }, { dist_type, 3 }, { dist_drive, 0.6f }, { dly_time, 6 }, { dly_fb, 0.6f } } },
        { "GLITCHES & TAPE STOPS", "Tape Stop Reese",
          { { sub_on, 0 }, { imp_on, 0 }, { syn_on, 1 }, { syn_level, -6 }, { syn_uni, 4 }, { syn_det, 0.4f }, { syn_cut, 2500 }, { syn_s, 1 }, { syn_r, 1 },
            { tape_time, 1.2f }, { dist_type, 1 }, { dist_drive, 0.4f } } },
        { "GLITCHES & TAPE STOPS", "Glitch Riser 1/32",
          { { sub_on, 0 }, { imp_on, 0 }, { ris_on, 1 }, { ris_len, 0 }, { ris_gate, 3 }, { ris_tone, 0.7f }, { ris_level, -5 }, { dist_type, 3 }, { dist_drive, 0.35f } } },
        { "GLITCHES & TAPE STOPS", "Delay Shards",
          { { sub_on, 0 }, { imp_sample, 7 }, { imp_dly, 0.7f }, { imp_rev, 0.3f }, { imp_level, -3 }, { dly_time, 6 }, { dly_fb, 0.7f }, { dly_hicut, 9000 } } },
        { "GLITCHES & TAPE STOPS", "Scrape Stutter",
          { { sub_on, 0 }, { imp_sample, 3 }, { imp_level, -3 }, { imp_rev, 0.35f }, { gate_rate, 1 }, { gate_depth, 0.9f }, { rev_mode, 1 }, { rev_size, 0.6f },
            { dist_type, 2 }, { dist_drive, 0.3f } } },
    };

    return presets;
}
} // namespace fl
