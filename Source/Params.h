#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

// Single source of truth for every automatable parameter of Finish Line.
namespace fl
{
enum Kind { kF, kC, kB, kI };

#define FL_SAMPLE_NAMES "TAIKO|ANVIL|CINEMA SLAM|METAL SCRAPE|GUNSHOT|IND CLANK|BREATH|GLASS DEBRIS|PIPE HIT|DOOR SLAM|SNARE BOMB|STATIC BURST|USER WAV"
#define FL_TARGET_NAMES "OFF|DIST DRIVE|DIST MIX|FLT CUTOFF|FLT RESO|LFO RATE|LFO DEPTH|REV SIZE|REV MIX|DLY FEEDBK|DLY MIX|GATE DEPTH|SUB HARM|SUB PUNCH|SUB START|SYN WT POS|SYN DETUNE|SYN CUTOFF|SYN RESO|SYN NOISE|RIS RANGE|RIS TONE|IMP PITCH"
#define FL_DIVS "OFF|1/8|1/16|1/32|1/8T|1/16T"

// X(id, name, kind, min, max, default, skewCentre (0 = linear), choices, unit)
#define FL_LAYER(X, pre, nm, on, lvl, rv) \
    X(pre##_on,    nm " On",       kB, 0, 1, on, 0, "", "") \
    X(pre##_lock,  nm " Lock",     kB, 0, 1, 0, 0, "", "") \
    X(pre##_level, nm " Level",    kF, -48, 6, lvl, 0, "", "dB") \
    X(pre##_rev,   nm " Reverb",   kF, 0, 1, rv, 0, "", "") \
    X(pre##_dly,   nm " Delay",    kF, 0, 1, 0, 0, "", "")

#define FL_MACRO(X, m, nm, t1, a1, t2, a2, t3, a3, t4, a4) \
    X(m##_t1, nm " Target 1", kC, 0, 22, t1, 0, FL_TARGET_NAMES, "") \
    X(m##_a1, nm " Amount 1", kF, -1, 1, a1, 0, "", "") \
    X(m##_t2, nm " Target 2", kC, 0, 22, t2, 0, FL_TARGET_NAMES, "") \
    X(m##_a2, nm " Amount 2", kF, -1, 1, a2, 0, "", "") \
    X(m##_t3, nm " Target 3", kC, 0, 22, t3, 0, FL_TARGET_NAMES, "") \
    X(m##_a3, nm " Amount 3", kF, -1, 1, a3, 0, "", "") \
    X(m##_t4, nm " Target 4", kC, 0, 22, t4, 0, FL_TARGET_NAMES, "") \
    X(m##_a4, nm " Amount 4", kF, -1, 1, a4, 0, "", "")

#define FL_PARAMS(X) \
    X(master_gain, "Output", kF, -24, 6, 0, 0, "", "dB") \
    X(key, "Key", kC, 0, 11, 0, 0, "C|C#|D|D#|E|F|F#|G|G#|A|A#|B", "") \
    X(key_lock, "Key Lock", kB, 0, 1, 1, 0, "", "") \
    X(macro1, "Aggression", kF, 0, 1, 0, 0, "", "") \
    X(macro2, "Tension", kF, 0, 1, 0, 0, "", "") \
    FL_MACRO(X, m1, "Aggression", 1, 0.6, 12, 0.6, 15, 0.35, 4, 0.2) \
    FL_MACRO(X, m2, "Tension", 8, 0.4, 5, 0.5, 20, 0.4, 9, 0.3) \
    /* ---- Layer 1: SUB ---- */ \
    FL_LAYER(X, sub, "Sub", 1, -3, 0) \
    X(sub_wave, "Sub Wave", kC, 0, 2, 0, 0, "SINE|TRIANGLE|SAT SINE", "") \
    X(sub_start, "Sub Start Pitch", kF, 30, 800, 180, 150, "", "Hz") \
    X(sub_end, "Sub End Pitch", kF, 20, 150, 36, 50, "", "Hz") \
    X(sub_pdecay, "Sub Pitch Decay", kF, 0.01, 4, 0.35, 0.4, "", "s") \
    X(sub_decay, "Sub Amp Decay", kF, 0.1, 10, 2.5, 2, "", "s") \
    X(sub_punch, "Sub Punch", kF, 0, 1, 0.4, 0, "", "") \
    X(sub_harm, "Sub Harmonics", kF, 0, 1, 0.25, 0, "", "") \
    /* ---- Layer 2: IMPACT ---- */ \
    FL_LAYER(X, imp, "Impact", 1, -6, 0.25) \
    X(imp_sample, "Impact Sample", kC, 0, 12, 2, 0, FL_SAMPLE_NAMES, "") \
    X(imp_pitch, "Impact Pitch", kF, -24, 24, 0, 0, "", "st") \
    X(imp_start, "Impact Start Offset", kF, 0, 0.5, 0, 0, "", "") \
    X(imp_reverse, "Impact Reverse", kB, 0, 1, 0, 0, "", "") \
    X(imp_attack, "Impact Attack", kF, -1, 1, 0, 0, "", "") \
    X(imp_sustain, "Impact Sustain", kF, -1, 1, 0, 0, "", "") \
    /* ---- Layer 3: SYNTH / TEXTURE ---- */ \
    FL_LAYER(X, syn, "Synth", 0, -9, 0.2) \
    X(syn_wt, "Synth WT Position", kF, 0, 1, 0.2857, 0, "", "") \
    X(syn_uni, "Synth Unison", kI, 1, 8, 2, 0, "", "") \
    X(syn_det, "Synth Detune", kF, 0, 1, 0.3, 0, "", "") \
    X(syn_pitch, "Synth Pitch", kI, -24, 24, 0, 0, "", "st") \
    X(syn_ntype, "Synth Noise Type", kC, 0, 2, 0, 0, "WHITE|PINK|METALLIC", "") \
    X(syn_noise, "Synth Noise", kF, 0, 1, 0, 0, "", "") \
    X(syn_ftype, "Synth Filter Type", kC, 0, 2, 0, 0, "LP|HP|BP", "") \
    X(syn_cut, "Synth Cutoff", kF, 20, 20000, 1200, 1000, "", "Hz") \
    X(syn_res, "Synth Resonance", kF, 0, 1, 0.2, 0, "", "") \
    X(syn_a, "Synth Attack", kF, 0.001, 5, 0.01, 0.3, "", "s") \
    X(syn_d, "Synth Decay", kF, 0.01, 5, 0.3, 0.5, "", "s") \
    X(syn_s, "Synth Sustain", kF, 0, 1, 0.8, 0, "", "") \
    X(syn_r, "Synth Release", kF, 0.01, 8, 0.4, 0.8, "", "s") \
    /* ---- Layer 4: RISER / TRANSITION ---- */ \
    FL_LAYER(X, ris, "Riser", 0, -9, 0.3) \
    X(ris_mode, "Riser Mode", kC, 0, 2, 0, 0, "RISER|DOWNLIFTER|SWEEP", "") \
    X(ris_len, "Riser Length", kC, 0, 3, 1, 0, "1 BAR|2 BARS|4 BARS|8 BARS", "") \
    X(ris_curve, "Riser Curve", kC, 0, 1, 1, 0, "LINEAR|EXPONENTIAL", "") \
    X(ris_range, "Riser Pitch Range", kF, 0, 48, 24, 0, "", "st") \
    X(ris_tone, "Riser Tone", kF, 0, 1, 0.35, 0, "", "") \
    X(ris_gate, "Riser Gate", kC, 0, 5, 0, 0, FL_DIVS, "") \
    X(ris_grid, "Riser Grid Align", kB, 0, 1, 1, 0, "", "") \
    /* ---- Master FX ---- */ \
    X(fx_lock, "FX Lock", kB, 0, 1, 0, 0, "", "") \
    X(dist_on, "Dist On", kB, 0, 1, 1, 0, "", "") \
    X(dist_type, "Dist Type", kC, 0, 3, 0, 0, "TUBE|TAPE|HARD CLIP|BITCRUSH", "") \
    X(dist_drive, "Dist Drive", kF, 0, 1, 0.25, 0, "", "") \
    X(dist_mix, "Dist Mix", kF, 0, 1, 1, 0, "", "") \
    X(flt_on, "Filter On", kB, 0, 1, 0, 0, "", "") \
    X(flt_type, "Filter Type", kC, 0, 2, 0, 0, "LP|HP|BP", "") \
    X(flt_slope, "Filter Slope", kC, 0, 1, 1, 0, "12 dB|24 dB", "") \
    X(flt_cut, "Filter Cutoff", kF, 20, 20000, 8000, 1000, "", "Hz") \
    X(flt_res, "Filter Resonance", kF, 0, 1, 0.15, 0, "", "") \
    X(flt_lfo_rate, "Filter LFO Rate", kF, 0.05, 20, 2, 2, "", "Hz") \
    X(flt_lfo_depth, "Filter LFO Depth", kF, -1, 1, 0, 0, "", "") \
    X(flt_env, "Filter Env Depth", kF, -1, 1, 0, 0, "", "") \
    X(rev_mode, "Reverb Mode", kC, 0, 2, 0, 0, "DARK HALL|METALLIC ROOM|INFINITE CANYON", "") \
    X(rev_size, "Reverb Size", kF, 0, 1, 0.5, 0, "", "") \
    X(rev_mix, "Reverb Mix", kF, 0, 1, 0.7, 0, "", "") \
    X(rev_duck, "Reverb Duck", kF, 0, 1, 0.4, 0, "", "") \
    X(dly_time, "Delay Time", kC, 0, 6, 4, 0, "1/2|1/4|1/4D|1/8|1/8D|1/8T|1/16", "") \
    X(dly_fb, "Delay Feedback", kF, 0, 0.95, 0.4, 0, "", "") \
    X(dly_locut, "Delay Lo-Cut", kF, 20, 2000, 200, 300, "", "Hz") \
    X(dly_hicut, "Delay Hi-Cut", kF, 500, 20000, 5000, 4000, "", "Hz") \
    X(dly_mix, "Delay Mix", kF, 0, 1, 0.7, 0, "", "") \
    X(gate_rate, "Gate Rate", kC, 0, 5, 0, 0, FL_DIVS, "") \
    X(gate_depth, "Gate Depth", kF, 0, 1, 1, 0, "", "") \
    X(tape_stop, "Tape Stop", kB, 0, 1, 0, 0, "", "") \
    X(tape_time, "Tape Stop Time", kF, 0.05, 4, 0.8, 0.8, "", "s") \
    X(lim_mode, "Limiter Mode", kC, 0, 1, 0, 0, "LIMITER|CLIPPER", "") \
    X(lim_ceiling, "Ceiling", kF, -12, 0, -0.3, 0, "", "dB")

namespace P
{
enum : int
{
#define X(id, name, kind, mn, mx, df, ctr, ch, unit) id,
    FL_PARAMS(X)
#undef X
    NumParams
};
}

struct Def
{
    const char* id;
    const char* name;
    Kind kind;
    float mn, mx, df, centre;
    const char* choices;
    const char* unit;
};

inline const Def kDefs[P::NumParams] = {
#define X(id, name, kind, mn, mx, df, ctr, ch, unit) { #id, name, kind, (float) (mn), (float) (mx), (float) (df), (float) (ctr), ch, unit },
    FL_PARAMS(X)
#undef X
};

// Macro target index (see FL_TARGET_NAMES) -> parameter index.
inline const int kMacroTargets[] = {
    -1, P::dist_drive, P::dist_mix, P::flt_cut, P::flt_res, P::flt_lfo_rate, P::flt_lfo_depth,
    P::rev_size, P::rev_mix, P::dly_fb, P::dly_mix, P::gate_depth, P::sub_harm, P::sub_punch,
    P::sub_start, P::syn_wt, P::syn_det, P::syn_cut, P::syn_res, P::syn_noise, P::ris_range,
    P::ris_tone, P::imp_pitch
};

constexpr int kNumFactorySamples = 12;
constexpr int kUserSampleIndex = 12;

inline juce::String formatValue (float v)
{
    const float a = std::abs (v);
    if (a >= 100.0f) return juce::String (juce::roundToInt (v));
    return juce::String (v, a >= 10.0f ? 1 : 2);
}

inline juce::AudioProcessorValueTreeState::ParameterLayout createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    for (const auto& d : kDefs)
    {
        const juce::ParameterID pid { d.id, 1 };

        switch (d.kind)
        {
            case kF:
            {
                juce::NormalisableRange<float> range (d.mn, d.mx);
                if (d.centre > 0.0f)
                    range.setSkewForCentre (d.centre);

                layout.add (std::make_unique<juce::AudioParameterFloat> (
                    pid, d.name, range, d.df,
                    juce::AudioParameterFloatAttributes()
                        .withLabel (d.unit)
                        .withStringFromValueFunction ([] (float v, int) { return formatValue (v); })));
                break;
            }
            case kC:
                layout.add (std::make_unique<juce::AudioParameterChoice> (
                    pid, d.name, juce::StringArray::fromTokens (d.choices, "|", ""), (int) d.df));
                break;
            case kB:
                layout.add (std::make_unique<juce::AudioParameterBool> (pid, d.name, d.df > 0.5f));
                break;
            case kI:
                layout.add (std::make_unique<juce::AudioParameterInt> (
                    pid, d.name, (int) d.mn, (int) d.mx, (int) d.df,
                    juce::AudioParameterIntAttributes().withLabel (d.unit)));
                break;
        }
    }

    return layout;
}
} // namespace fl
