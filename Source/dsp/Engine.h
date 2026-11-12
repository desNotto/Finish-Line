#pragma once
#include "../Params.h"
#include "FactorySamples.h"
#include "Fx.h"
#include "Layers.h"

namespace fl
{
enum LayerMask { kSub = 1, kImpact = 2, kSynth = 4, kRiser = 8, kAllLayers = 15 };

// The complete audio engine: 4 layers -> sends -> master FX -> limiter.
// It only sees a plain array of parameter values, so the same class drives both
// the realtime callback and the offline WAV render used for drag-and-drop.
class Engine
{
public:
    std::atomic<const SampleData*> userSample { nullptr };
    bool tapeEngage = false;
    float gainReduction = 1.0f;

    void prepare (double sampleRate)
    {
        sr = (float) sampleRate;
        Wavetables::get();
        bank = &factoryBank();

        sub.prepare (sr);
        impact.prepare (sr);
        synth.prepare (sr);
        riser.prepare (sr);
        dist.prepare (sr);
        filter.prepare (sr);
        delay.prepare (sr);
        reverb.prepare (sr);
        tape.prepare (sr);
        limiter.prepare (sr);

        duckAtt = 1.0f - std::exp (-1.0f / (0.005f * sr));
        duckRel = 1.0f - std::exp (-1.0f / (0.25f * sr));
        duckEnv = 0.0f;
        masterGain = 1.0f;
        for (auto& g : gain) g = 0.0f;
        for (auto& v : vel) v = 1.0f;
    }

    float riserProgress() const { return riser.progress; }

    void noteOn (int mask, float transpose, float velocity, const float* p, const HostInfo& h)
    {
        using namespace P;
        const float v = 0.35f + 0.65f * clampf (velocity, 0.0f, 1.0f);

        if ((mask & kSub) && p[sub_on] > 0.5f)
        {
            subTranspose = transpose;
            vel[0] = v;
            gain[0] = dbToGain (p[sub_level]) * v;
            sub.trigger();
        }

        if ((mask & kImpact) && p[imp_on] > 0.5f)
        {
            const int idx = (int) p[imp_sample];
            const SampleData* s = nullptr;
            if (idx >= 0 && idx < (int) bank->size())
                s = (*bank)[(size_t) idx].get();
            else
                s = userSample.load();
            if (s == nullptr)
                s = (*bank)[2].get();

            vel[1] = v;
            gain[1] = dbToGain (p[imp_level]) * v;
            impact.trigger (s, p[imp_pitch] + transpose, p[imp_start], p[imp_reverse] > 0.5f);
        }

        if ((mask & kSynth) && p[syn_on] > 0.5f)
        {
            synTranspose = transpose;
            vel[2] = v;
            if (! synth.active) gain[2] = dbToGain (p[syn_level]) * v;
            synth.trigger (synthFreq (p));
        }

        if ((mask & kRiser) && p[ris_on] > 0.5f)
        {
            static const double bars[] = { 1.0, 2.0, 4.0, 8.0 };
            const double barBeats = h.beatsPerBar();
            double beats = bars[juce::jlimit (0, 3, (int) p[ris_len])] * barBeats;

            if (p[ris_grid] > 0.5f && h.playing && barBeats > 0.0)
            {
                // End exactly on a bar line even when the note lands slightly early or late.
                double frac = (h.ppq - h.ppqBar) / barBeats;
                frac -= std::floor (frac);
                beats += frac > 0.5 ? (1.0 - frac) * barBeats : -frac * barBeats;
            }

            risTranspose = transpose;
            vel[3] = v;
            gain[3] = dbToGain (p[ris_level]) * v;
            riser.trigger (beats * 60.0 / juce::jlimit (20.0, 400.0, h.bpm) * (double) sr);
        }

        if (! h.playing)
            gate.resetPhase();
    }

    void noteOff (int mask)
    {
        if (mask & kSynth)
            synth.release();
    }

    void process (float* L, float* R, int n, const float* p, HostInfo h)
    {
        for (int o = 0; o < n; o += kChunk)
        {
            const int c = juce::jmin (kChunk, n - o);
            chunk (L + o, R + o, c, p, h);
            h.ppq += (double) c * h.bpm / (60.0 * (double) sr);
        }
    }

private:
    static constexpr int kChunk = 256;
    using Buf = std::array<float, kChunk>;

    float synthFreq (const float* p) const
    {
        const float midi = 24.0f + p[P::key] + p[P::syn_pitch] + synTranspose;
        return 440.0f * std::pow (2.0f, (midi - 69.0f) / 12.0f);
    }

    void mixLayer (int k, int n, float target, float rev, float dly)
    {
        float g = gain[k];
        const float step = (target - g) / (float) n;

        for (int i = 0; i < n; ++i)
        {
            g += step;
            const float l = layL[(size_t) i] * g, r = layR[(size_t) i] * g;
            dryL[(size_t) i] += l;
            dryR[(size_t) i] += r;
            revL[(size_t) i] += l * rev;
            revR[(size_t) i] += r * rev;
            dlyIn[(size_t) i] += (l + r) * 0.5f * dly;
        }

        gain[k] = target;
    }

    void chunk (float* L, float* R, int n, const float* p, const HostInfo& h)
    {
        using namespace P;
        const double bpm = juce::jlimit (20.0, 400.0, h.bpm);
        const auto bytes = sizeof (float) * (size_t) n;

        std::memset (dryL.data(), 0, bytes);
        std::memset (dryR.data(), 0, bytes);
        std::memset (revL.data(), 0, bytes);
        std::memset (revR.data(), 0, bytes);
        std::memset (dlyIn.data(), 0, bytes);

        auto clearLayer = [&] { std::memset (layL.data(), 0, bytes); std::memset (layR.data(), 0, bytes); };
        auto target = [&] (int k, int onId, int levelId) { return p[onId] > 0.5f ? dbToGain (p[levelId]) * vel[k] : 0.0f; };

        // ---- Layer 1: SUB
        if (sub.active)
        {
            float fEnd = p[sub_end];
            if (p[key_lock] > 0.5f) fEnd = quantizeToKey (fEnd, (int) p[key]);
            const float tr = semis (subTranspose);
            sub.setParams ((int) p[sub_wave], p[sub_start] * tr, fEnd * tr, p[sub_pdecay], p[sub_decay], p[sub_punch], p[sub_harm]);
            clearLayer();
            sub.render (layL.data(), layR.data(), n);
            mixLayer (0, n, target (0, sub_on, sub_level), p[sub_rev], p[sub_dly]);
        }
        else gain[0] = target (0, sub_on, sub_level);

        // ---- Layer 2: IMPACT
        if (impact.active)
        {
            impact.attackAmt = p[imp_attack];
            impact.sustainAmt = p[imp_sustain];
            clearLayer();
            impact.render (layL.data(), layR.data(), n);
            mixLayer (1, n, target (1, imp_on, imp_level), p[imp_rev], p[imp_dly]);
        }
        else gain[1] = target (1, imp_on, imp_level);

        // ---- Layer 3: SYNTH / TEXTURE
        if (synth.active)
        {
            synth.freq = synthFreq (p);
            synth.setEnvelope (p[syn_a], p[syn_d], p[syn_s], p[syn_r]);
            clearLayer();
            synth.render (layL.data(), layR.data(), n, p[syn_wt], (int) p[syn_uni], p[syn_det],
                          (int) p[syn_ntype], p[syn_noise], (int) p[syn_ftype], p[syn_cut], p[syn_res]);
            mixLayer (2, n, target (2, syn_on, syn_level), p[syn_rev], p[syn_dly]);
        }
        else gain[2] = target (2, syn_on, syn_level);

        // ---- Layer 4: RISER / TRANSITION
        if (riser.active)
        {
            const float midi = 60.0f + p[key] + risTranspose;
            const float fEnd = 440.0f * std::pow (2.0f, (midi - 69.0f) / 12.0f);
            const double gateStep = gateDivBeats ((int) p[ris_gate]) * 60.0 / bpm * (double) sr;
            clearLayer();
            riser.render (layL.data(), layR.data(), n, (int) p[ris_mode], (int) p[ris_curve],
                          p[ris_range], p[ris_tone], fEnd, gateStep);
            mixLayer (3, n, target (3, ris_on, ris_level), p[ris_rev], p[ris_dly]);
        }
        else
        {
            riser.progress = -1.0f;
            gain[3] = target (3, ris_on, ris_level);
        }

        // ---- Master FX: distortion -> filter -> gate
        if (p[dist_on] > 0.5f)
            dist.process (dryL.data(), dryR.data(), n, (int) p[dist_type], p[dist_drive], p[dist_mix]);

        if (p[flt_on] > 0.5f)
            filter.process (dryL.data(), dryR.data(), n, (int) p[flt_type], p[flt_slope] > 0.5f, p[flt_cut],
                            p[flt_res], p[flt_lfo_rate], p[flt_lfo_depth], p[flt_env]);

        gate.process (dryL.data(), dryR.data(), n, gateDivBeats ((int) p[gate_rate]), p[gate_depth], h, sr);

        // ---- Space: ping-pong delay + ducked reverb
        const float delaySamples = (float) (delayDivBeats ((int) p[dly_time]) * 60.0 / bpm * (double) sr);
        delay.process (dlyIn.data(), layL.data(), layR.data(), n, delaySamples, p[dly_fb], p[dly_locut], p[dly_hicut]);

        const float dMix = p[dly_mix];
        for (int i = 0; i < n; ++i)
        {
            L[i] = dryL[(size_t) i] + dMix * layL[(size_t) i];
            R[i] = dryR[(size_t) i] + dMix * layR[(size_t) i];
        }

        reverb.set ((int) p[rev_mode], p[rev_size]);
        reverb.process (revL.data(), revR.data(), layL.data(), layR.data(), n);

        const float rMix = p[rev_mix], duck = p[rev_duck];
        for (int i = 0; i < n; ++i)
        {
            const float a = juce::jmax (std::abs (dryL[(size_t) i]), std::abs (dryR[(size_t) i]));
            duckEnv += (a > duckEnv ? duckAtt : duckRel) * (a - duckEnv);
            const float g = rMix * (1.0f - duck * juce::jmin (1.0f, duckEnv * 3.0f));
            L[i] += g * layL[(size_t) i];
            R[i] += g * layR[(size_t) i];
        }

        // ---- Glitch, output gain, limiter
        tape.process (L, R, n, tapeEngage || p[tape_stop] > 0.5f, p[tape_time], sr);

        const float mgTarget = dbToGain (p[master_gain]);
        const float mgStep = (mgTarget - masterGain) / (float) n;
        for (int i = 0; i < n; ++i)
        {
            masterGain += mgStep;
            L[i] *= masterGain;
            R[i] *= masterGain;
        }
        masterGain = mgTarget;

        limiter.process (L, R, n, p[lim_mode] > 0.5f, dbToGain (p[lim_ceiling]));
        gainReduction = limiter.minGain;
    }

    float sr = 48000.0f;
    const SampleBank* bank = nullptr;

    SubLayer sub;
    ImpactLayer impact;
    SynthLayer synth;
    RiserLayer riser;

    Distortion dist;
    MasterFilter filter;
    Gate gate;
    PingPong delay;
    Reverb reverb;
    TapeStop tape;
    Limiter limiter;

    Buf layL {}, layR {}, dryL {}, dryR {}, revL {}, revR {}, dlyIn {};
    float gain[4] {}, vel[4] { 1.0f, 1.0f, 1.0f, 1.0f };
    float subTranspose = 0.0f, synTranspose = 0.0f, risTranspose = 0.0f;
    float duckEnv = 0.0f, duckAtt = 0.0f, duckRel = 0.0f;
    float masterGain = 1.0f;
};
} // namespace fl
