#pragma once
#include "Util.h"

// Built-in impact library. Every sample is synthesised procedurally at start-up,
// so the plugin ships as a single file with no external content to install.
namespace fl
{
namespace gen
{
constexpr float kSr = 48000.0f;

inline std::shared_ptr<SampleData> blank (const char* name, float seconds)
{
    auto s = std::make_shared<SampleData>();
    s->name = name;
    s->sr = kSr;
    const auto n = (size_t) (seconds * kSr);
    s->l.assign (n, 0.0f);
    s->r.assign (n, 0.0f);
    return s;
}

inline void finish (SampleData& s, float fadeSeconds = 0.06f)
{
    const int n = s.length();
    float pk = 0.0f;
    for (int i = 0; i < n; ++i)
        pk = juce::jmax (pk, std::abs (s.l[(size_t) i]), std::abs (s.r[(size_t) i]));

    const float g = pk > 0.0f ? 0.89f / pk : 1.0f;
    const int fade = juce::jmin (n, (int) (fadeSeconds * kSr));

    for (int i = 0; i < n; ++i)
    {
        float f = g;
        if (i >= n - fade) f *= (float) (n - 1 - i) / (float) fade;
        s.l[(size_t) i] *= f;
        s.r[(size_t) i] *= f;
    }

    s.computePeaks();
}

struct Mode { float freq, amp, decay; };

// Sum of exponentially decaying partials (struck metal, membranes, pipes).
inline void addModes (SampleData& s, std::initializer_list<Mode> modes, Rng& rng, float start = 0.0f, float spread = 0.004f)
{
    const int n = s.length();
    const int i0 = juce::jlimit (0, n, (int) (start * kSr));

    for (const auto& m : modes)
    {
        const float wL = kTwoPi * m.freq / kSr;
        const float wR = wL * (1.0f + spread * rng.bi());
        const float aL = m.amp * (0.75f + 0.25f * rng.uni());
        const float aR = m.amp * (0.75f + 0.25f * rng.uni());
        const float dec = std::exp (-6.9f / (m.decay * kSr));
        float e = 1.0f;

        for (int i = i0; i < n && e > 1.0e-5f; ++i)
        {
            const float t = (float) (i - i0);
            s.l[(size_t) i] += aL * e * std::sin (wL * t);
            s.r[(size_t) i] += aR * e * std::sin (wR * t);
            e *= dec;
        }
    }
}

// Sine with an exponential pitch drop (kicks, booms).
inline void addSweep (SampleData& s, float f0, float f1, float pitchTau, float ampDecay, float amp, float start = 0.0f)
{
    const int n = s.length();
    const int i0 = juce::jlimit (0, n, (int) (start * kSr));
    double ph = 0.0;

    for (int i = i0; i < n; ++i)
    {
        const float t = (float) (i - i0) / kSr;
        const float f = f1 + (f0 - f1) * std::exp (-t / pitchTau);
        ph += (double) f / kSr;
        const float e = std::exp (-6.9f * t / ampDecay) * juce::jmin (1.0f, t * 800.0f);
        const float v = amp * e * std::sin (kTwoPi * (float) ph);
        s.l[(size_t) i] += v;
        s.r[(size_t) i] += v;
    }
}

// Enveloped noise through a swept low-pass and optional high-pass.
inline void addNoise (SampleData& s, Rng& rng, float amp, float attack, float decay,
                      float lpStart, float lpEnd, float lpTau, float hpHz = 0.0f, float start = 0.0f)
{
    const int n = s.length();
    const int i0 = juce::jlimit (0, n, (int) (start * kSr));
    Svf fl_, fr_;
    OnePole hl, hr;
    hl.setCutoff (juce::jmax (1.0f, hpHz), kSr);
    hr.setCutoff (juce::jmax (1.0f, hpHz), kSr);

    for (int i = i0; i < n; ++i)
    {
        const float t = (float) (i - i0) / kSr;

        if (((i - i0) & 15) == 0)
        {
            const float fc = lpEnd + (lpStart - lpEnd) * std::exp (-t / lpTau);
            fl_.set (fc, 0.1f, kSr);
            fr_.copyCoeffs (fl_);
        }

        float e = t < attack ? t / attack : std::exp (-6.9f * (t - attack) / decay);
        if (e < 1.0e-5f && t > attack) break;

        fl_.tick (rng.bi());
        fr_.tick (rng.bi());
        float a = fl_.lp, b = fr_.lp;
        if (hpHz > 0.0f) { a = hl.hp (a); b = hr.hp (b); }
        s.l[(size_t) i] += amp * e * a;
        s.r[(size_t) i] += amp * e * b;
    }
}

inline void drive (SampleData& s, float d)
{
    for (auto& v : s.l) v = std::tanh (v * d);
    for (auto& v : s.r) v = std::tanh (v * d);
}
} // namespace gen

using SampleBank = std::vector<std::shared_ptr<SampleData>>;

inline SampleBank buildFactoryBank()
{
    using namespace gen;
    SampleBank bank;

    { // 0 TAIKO
        Rng rng (101);
        auto s = blank ("TAIKO", 1.7f);
        addSweep (*s, 150.0f, 58.0f, 0.045f, 1.2f, 1.0f);
        addModes (*s, { { 92.0f, 0.35f, 0.9f }, { 124.0f, 0.22f, 0.6f }, { 171.0f, 0.12f, 0.4f } }, rng);
        addNoise (*s, rng, 0.9f, 0.001f, 0.05f, 5000.0f, 600.0f, 0.02f);
        addNoise (*s, rng, 0.12f, 0.01f, 0.9f, 900.0f, 200.0f, 0.3f);
        drive (*s, 1.6f);
        finish (*s);
        bank.push_back (s);
    }
    { // 1 ANVIL
        Rng rng (202);
        auto s = blank ("ANVIL", 2.2f);
        addModes (*s, { { 523.0f, 0.5f, 1.6f }, { 1187.0f, 0.6f, 1.3f }, { 1873.0f, 0.5f, 1.0f },
                        { 2741.0f, 0.45f, 0.8f }, { 3562.0f, 0.35f, 0.6f }, { 4790.0f, 0.3f, 0.45f },
                        { 6120.0f, 0.2f, 0.3f }, { 7930.0f, 0.12f, 0.2f } }, rng);
        addNoise (*s, rng, 1.0f, 0.0005f, 0.012f, 16000.0f, 4000.0f, 0.01f, 1500.0f);
        addSweep (*s, 120.0f, 70.0f, 0.02f, 0.18f, 0.5f);
        finish (*s);
        bank.push_back (s);
    }
    { // 2 CINEMA SLAM
        Rng rng (303);
        auto s = blank ("CINEMA SLAM", 3.2f);
        addSweep (*s, 95.0f, 31.0f, 0.12f, 2.6f, 1.0f);
        addNoise (*s, rng, 1.1f, 0.001f, 1.3f, 9000.0f, 180.0f, 0.22f);
        addNoise (*s, rng, 0.6f, 0.0005f, 0.03f, 14000.0f, 3000.0f, 0.02f, 800.0f);
        addModes (*s, { { 61.0f, 0.3f, 2.0f }, { 233.0f, 0.1f, 0.5f } }, rng);
        drive (*s, 2.2f);
        finish (*s, 0.2f);
        bank.push_back (s);
    }
    { // 3 METAL SCRAPE
        Rng rng (404);
        auto s = blank ("METAL SCRAPE", 1.9f);
        const int n = s->length();
        Svf f[4];
        const float base[4] = { 830.0f, 1940.0f, 3310.0f, 5270.0f };
        for (int i = 0; i < n; ++i)
        {
            const float t = (float) i / kSr;
            if ((i & 15) == 0)
                for (int k = 0; k < 4; ++k)
                    f[k].set (base[k] * (1.0f + 0.22f * std::sin (t * (5.0f + 3.1f * (float) k)) + 0.35f * t), 0.82f, kSr);

            const float env = juce::jmin (1.0f, t / 0.25f) * std::exp (-1.4f * juce::jmax (0.0f, t - 0.5f));
            const float grit = (rng.uni() > 0.55f + 0.3f * std::sin (t * 37.0f)) ? 1.0f : 0.15f;
            const float x = rng.bi() * grit;
            float a = 0.0f, b = 0.0f;
            for (int k = 0; k < 4; ++k)
            {
                f[k].tick (x);
                ((k & 1) ? b : a) += f[k].bp;
                ((k & 1) ? a : b) += f[k].bp * 0.4f;
            }
            s->l[(size_t) i] = env * a;
            s->r[(size_t) i] = env * b;
        }
        drive (*s, 0.6f);
        finish (*s, 0.15f);
        bank.push_back (s);
    }
    { // 4 GUNSHOT
        Rng rng (505);
        auto s = blank ("GUNSHOT", 1.5f);
        addNoise (*s, rng, 1.6f, 0.0003f, 0.09f, 18000.0f, 2500.0f, 0.03f, 300.0f);
        addSweep (*s, 190.0f, 55.0f, 0.03f, 0.3f, 1.0f);
        addNoise (*s, rng, 0.35f, 0.02f, 1.1f, 4000.0f, 500.0f, 0.25f, 150.0f);
        drive (*s, 3.0f);
        finish (*s);
        bank.push_back (s);
    }
    { // 5 IND CLANK
        Rng rng (606);
        auto s = blank ("IND CLANK", 1.3f);
        addModes (*s, { { 211.0f, 0.6f, 0.5f }, { 487.0f, 0.55f, 0.45f }, { 733.0f, 0.5f, 0.35f },
                        { 1310.0f, 0.45f, 0.3f }, { 2093.0f, 0.35f, 0.22f }, { 3177.0f, 0.2f, 0.15f } }, rng, 0.0f, 0.01f);
        addNoise (*s, rng, 1.0f, 0.0005f, 0.04f, 9000.0f, 1500.0f, 0.02f, 400.0f);
        addModes (*s, { { 266.0f, 0.3f, 0.3f }, { 912.0f, 0.25f, 0.2f } }, rng, 0.085f);
        drive (*s, 1.8f);
        finish (*s);
        bank.push_back (s);
    }
    { // 6 BREATH
        Rng rng (707);
        auto s = blank ("BREATH", 1.7f);
        const int n = s->length();
        Svf f[3], g[3];
        const float fm[3] = { 720.0f, 1250.0f, 2700.0f };
        const float am[3] = { 1.0f, 0.7f, 0.45f };
        OnePole pinkL, pinkR;
        pinkL.setCutoff (2500.0f, kSr);
        pinkR.setCutoff (2500.0f, kSr);
        for (int i = 0; i < n; ++i)
        {
            const float t = (float) i / kSr;
            if ((i & 31) == 0)
                for (int k = 0; k < 3; ++k)
                {
                    f[k].set (fm[k] * (1.0f - 0.18f * t), 0.55f, kSr);
                    g[k].copyCoeffs (f[k]);
                }
            const float env = (t < 0.35f ? (t / 0.35f) * (t / 0.35f) : std::exp (-4.0f * (t - 0.35f)));
            const float xl = pinkL.lp (rng.bi()) * 3.0f, xr = pinkR.lp (rng.bi()) * 3.0f;
            float a = 0.0f, b = 0.0f;
            for (int k = 0; k < 3; ++k)
            {
                f[k].tick (xl);
                g[k].tick (xr);
                a += am[k] * f[k].bp;
                b += am[k] * g[k].bp;
            }
            s->l[(size_t) i] = env * (a + 0.1f * xl);
            s->r[(size_t) i] = env * (b + 0.1f * xr);
        }
        finish (*s, 0.2f);
        bank.push_back (s);
    }
    { // 7 GLASS DEBRIS
        Rng rng (808);
        auto s = blank ("GLASS DEBRIS", 1.8f);
        addNoise (*s, rng, 0.8f, 0.0005f, 0.03f, 18000.0f, 6000.0f, 0.02f, 2500.0f);
        const int n = s->length();
        for (int k = 0; k < 70; ++k)
        {
            const float u = rng.uni();
            const float t0 = u * u * 0.95f;
            const float fr = 1800.0f + 9000.0f * rng.uni();
            const float dc = 0.02f + 0.22f * rng.uni() * rng.uni();
            const float pan = rng.uni();
            const float amp = 0.25f + 0.5f * rng.uni();
            const float w = kTwoPi * fr / kSr;
            const float dec = std::exp (-6.9f / (dc * kSr));
            float e = 1.0f;
            for (int i = (int) (t0 * kSr), j = 0; i < n && e > 1.0e-4f; ++i, ++j)
            {
                const float v = amp * e * std::sin (w * (float) j);
                s->l[(size_t) i] += v * (1.0f - pan);
                s->r[(size_t) i] += v * pan;
                e *= dec;
            }
        }
        finish (*s);
        bank.push_back (s);
    }
    { // 8 PIPE HIT
        Rng rng (909);
        auto s = blank ("PIPE HIT", 2.4f);
        addModes (*s, { { 180.0f, 0.7f, 1.9f }, { 496.0f, 0.55f, 1.5f }, { 972.0f, 0.45f, 1.1f },
                        { 1607.0f, 0.35f, 0.8f }, { 2400.0f, 0.25f, 0.5f }, { 3350.0f, 0.15f, 0.35f } }, rng, 0.0f, 0.006f);
        addNoise (*s, rng, 0.9f, 0.0005f, 0.02f, 7000.0f, 1200.0f, 0.015f, 200.0f);
        drive (*s, 1.3f);
        finish (*s);
        bank.push_back (s);
    }
    { // 9 DOOR SLAM
        Rng rng (1010);
        auto s = blank ("DOOR SLAM", 1.3f);
        addSweep (*s, 110.0f, 52.0f, 0.02f, 0.28f, 1.0f);
        addNoise (*s, rng, 1.2f, 0.0005f, 0.07f, 3500.0f, 400.0f, 0.03f);
        addNoise (*s, rng, 0.25f, 0.015f, 0.7f, 1800.0f, 300.0f, 0.2f, 120.0f);
        addModes (*s, { { 143.0f, 0.25f, 0.25f }, { 318.0f, 0.15f, 0.18f } }, rng);
        drive (*s, 2.0f);
        finish (*s);
        bank.push_back (s);
    }
    { // 10 SNARE BOMB
        Rng rng (1111);
        auto s = blank ("SNARE BOMB", 1.9f);
        addSweep (*s, 330.0f, 175.0f, 0.02f, 0.22f, 0.9f);
        addNoise (*s, rng, 1.3f, 0.0005f, 0.25f, 15000.0f, 3500.0f, 0.1f, 350.0f);
        addNoise (*s, rng, 0.4f, 0.01f, 1.4f, 6000.0f, 800.0f, 0.4f, 250.0f);
        addModes (*s, { { 187.0f, 0.3f, 0.2f }, { 412.0f, 0.2f, 0.15f } }, rng);
        drive (*s, 2.6f);
        finish (*s, 0.15f);
        bank.push_back (s);
    }
    { // 11 STATIC BURST
        Rng rng (1212);
        auto s = blank ("STATIC BURST", 0.9f);
        const int n = s->length();
        float hl = 0.0f, hr = 0.0f;
        int hold = 0;
        for (int i = 0; i < n; ++i)
        {
            const float t = (float) i / kSr;
            if (--hold <= 0)
            {
                hold = 2 + (int) (rng.uni() * 14.0f);
                hl = std::round (rng.bi() * 6.0f) / 6.0f;
                hr = std::round (rng.bi() * 6.0f) / 6.0f;
            }
            const float chop = (std::fmod (t, 0.0625f) < 0.04f - 0.02f * t) ? 1.0f : 0.0f;
            const float env = std::exp (-2.5f * t);
            const float buzz = 0.4f * (std::fmod (t * 55.0f, 1.0f) < 0.5f ? 1.0f : -1.0f);
            s->l[(size_t) i] = env * chop * (hl + buzz);
            s->r[(size_t) i] = env * chop * (hr + buzz);
        }
        finish (*s, 0.02f);
        bank.push_back (s);
    }

    jassert ((int) bank.size() == 12);
    return bank;
}

inline const SampleBank& factoryBank()
{
    static const SampleBank bank = buildFactoryBank();
    return bank;
}
} // namespace fl
