#pragma once
#include "Util.h"
#include "Wavetables.h"

namespace fl
{
//==============================================================================
// Layer 1: SUB - analogue-modelled oscillator with a pitch drop envelope.
struct SubLayer
{
    bool active = false;
    float sr = 48000.0f;
    double phase = 0.0;
    float e = 1.0f, amp = 0.0f, att = 0.0f, click = 0.0f, over = 0.0f;
    Rng rng { 77 };
    OnePole clickLp;

    int wave = 0;
    float fStart = 180.0f, fEnd = 36.0f, pCoef = 0.999f, aCoef = 0.9999f, punch = 0.4f, harm = 0.25f;

    void prepare (float s)
    {
        sr = s;
        clickLp.setCutoff (3500.0f, sr);
        active = false;
    }

    void trigger()
    {
        active = true;
        phase = 0.0;
        e = 1.0f;
        amp = 1.0f;
        att = 0.0f;
        click = 1.0f;
        over = 1.0f;
    }

    void setParams (int w, float start, float end, float pitchDecay, float ampDecay, float pnch, float hrm)
    {
        wave = w;
        fStart = clampf (start, 10.0f, 4000.0f);
        fEnd = clampf (end, 10.0f, 2000.0f);
        pCoef = std::exp (-4.6f / (juce::jmax (0.005f, pitchDecay) * sr));
        aCoef = std::exp (-6.9f / (juce::jmax (0.05f, ampDecay) * sr));
        punch = pnch;
        harm = hrm;
    }

    void render (float* L, float* R, int n)
    {
        if (! active) return;

        const float logR = std::log (fStart / fEnd);
        const float attInc = 1.0f / (0.002f * sr);
        const float clickCoef = std::exp (-1.0f / (0.004f * sr));
        const float overCoef = std::exp (-1.0f / (0.025f * sr));
        const float d = 1.0f + harm * 9.0f;
        const float norm = 1.0f / std::tanh (d);

        for (int i = 0; i < n; ++i)
        {
            const float f = fEnd * std::exp (logR * e);
            e *= pCoef;
            phase += (double) f / sr;
            if (phase >= 1.0) phase -= 1.0;
            const float ph = (float) phase;

            float s;
            if (wave == 1)
                s = ph < 0.25f ? 4.0f * ph : (ph < 0.75f ? 2.0f - 4.0f * ph : 4.0f * ph - 4.0f);
            else
            {
                s = std::sin (kTwoPi * ph);
                if (wave == 2) s = std::tanh (3.0f * s) * 1.005f;
            }

            const float sat = std::tanh (s * d) * norm;
            s += harm * (sat - s);

            att = juce::jmin (1.0f, att + attInc);
            const float a = amp * att * (1.0f + 0.6f * punch * over);
            amp *= aCoef;
            over *= overCoef;

            const float c = punch * click * clickLp.lp (rng.bi()) * 1.5f;
            click *= clickCoef;

            const float out = s * a + c;
            L[i] += out;
            R[i] += out;
        }

        if (amp < 1.0e-4f) active = false;
    }
};

//==============================================================================
// Layer 2: IMPACT - one-shot sample player with a transient shaper.
struct ImpactLayer
{
    const SampleData* smp = nullptr;
    bool active = false, rev = false;
    double pos = 0.0, rate = 1.0;
    float sr = 48000.0f;
    int fadeIn = 0;
    float envF = 0.0f, envS = 0.0f;
    float fa = 0.0f, fr = 0.0f, sa = 0.0f, sr_ = 0.0f;
    float attackAmt = 0.0f, sustainAmt = 0.0f;

    void prepare (float s)
    {
        sr = s;
        fa = 1.0f - std::exp (-1.0f / (0.0008f * sr));
        fr = 1.0f - std::exp (-1.0f / (0.030f * sr));
        sa = 1.0f - std::exp (-1.0f / (0.020f * sr));
        sr_ = 1.0f - std::exp (-1.0f / (0.250f * sr));
        active = false;
    }

    void trigger (const SampleData* s, float pitchSemis, float startOffset, bool reverse)
    {
        if (s == nullptr || s->length() < 8)
        {
            active = false;
            return;
        }

        smp = s;
        rev = reverse;
        rate = (double) semis (pitchSemis) * s->sr / (double) sr;
        const double len = (double) s->length();
        pos = rev ? (len - 2.0) - (double) startOffset * len : (double) startOffset * len;
        fadeIn = 0;
        envF = envS = 0.0f;
        active = true;
    }

    void render (float* L, float* R, int n)
    {
        if (! active || smp == nullptr) return;

        const int len = smp->length();
        const float* sl = smp->l.data();
        const float* srr = smp->r.data();

        for (int i = 0; i < n; ++i)
        {
            const int idx = (int) pos;
            if (pos < 0.0 || idx >= len - 1)
            {
                active = false;
                break;
            }

            const float frac = (float) (pos - (double) idx);
            float l = sl[idx] + frac * (sl[idx + 1] - sl[idx]);
            float r = srr[idx] + frac * (srr[idx + 1] - srr[idx]);

            float g = juce::jmin (1.0f, (float) fadeIn * (1.0f / 64.0f));
            ++fadeIn;
            const float toEnd = rev ? (float) pos : (float) (len - 1) - (float) pos;
            g *= juce::jmin (1.0f, toEnd * (1.0f / 256.0f));

            const float a = juce::jmax (std::abs (l), std::abs (r));
            envF += (a > envF ? fa : fr) * (a - envF);
            envS += (a > envS ? sa : sr_) * (a - envS);
            const float dlt = (envF - envS) / (envF + envS + 1.0e-5f);
            const float sg = dlt > 0.0f ? 1.0f + attackAmt * dlt * 2.5f : 1.0f - sustainAmt * dlt * 2.5f;
            g *= clampf (sg, 0.05f, 4.0f);

            L[i] += l * g;
            R[i] += r * g;
            pos += rev ? -rate : rate;
        }
    }
};

//==============================================================================
// Layer 3: SYNTH / TEXTURE - wavetable oscillator with unison, noise and a filter.
struct SynthLayer
{
    static constexpr int kMaxUni = 8;

    bool active = false;
    float sr = 48000.0f;
    float freq = 55.0f;
    double ph[kMaxUni] {};
    juce::ADSR adsr;
    Svf fL, fR;
    Rng rngL { 0xA11CE }, rngR { 0xB0B };
    float pk[2][3] {};
    double mph[6] {};
    OnePole metalHp;

    void prepare (float s)
    {
        sr = s;
        adsr.setSampleRate ((double) s);
        adsr.reset();
        metalHp.setCutoff (2500.0f, sr);
        fL.reset();
        fR.reset();
        active = false;
    }

    void trigger (float f)
    {
        freq = f;
        if (! adsr.isActive())
        {
            for (int u = 0; u < kMaxUni; ++u)
                ph[u] = u == 0 ? 0.0 : (double) rngL.uni();
            fL.reset();
            fR.reset();
        }
        adsr.noteOn();
        active = true;
    }

    void release() { adsr.noteOff(); }

    void setEnvelope (float a, float d, float s, float r)
    {
        adsr.setParameters ({ a, d, s, r });
    }

    float pinkNoise (int ch, float w)
    {
        auto& b = pk[ch];
        b[0] = 0.99765f * b[0] + w * 0.0990460f;
        b[1] = 0.96300f * b[1] + w * 0.2965164f;
        b[2] = 0.57000f * b[2] + w * 1.0526913f;
        return (b[0] + b[1] + b[2] + w * 0.1848f) * 0.25f;
    }

    void render (float* L, float* R, int n, float wtPos, int uni, float detune,
                 int noiseType, float noiseLevel, int filterType, float cutoff, float res)
    {
        if (! active) return;

        const auto& wt = Wavetables::get();
        uni = juce::jlimit (1, kMaxUni, uni);

        float inc[kMaxUni], panL[kMaxUni], panR[kMaxUni];
        float maxF = freq;
        for (int u = 0; u < uni; ++u)
        {
            const float spread = uni > 1 ? ((float) u / (float) (uni - 1)) * 2.0f - 1.0f : 0.0f;
            const float f = freq * semis (spread * detune * 0.6f);
            maxF = juce::jmax (maxF, f);
            inc[u] = f / sr;
            panL[u] = std::sqrt (0.5f * (1.0f - 0.8f * spread));
            panR[u] = std::sqrt (0.5f * (1.0f + 0.8f * spread));
        }

        const int level = Wavetables::levelFor (maxF, sr);
        const float fpos = clampf (wtPos, 0.0f, 1.0f) * (float) (Wavetables::kFrames - 1);
        const int f0 = juce::jmin (Wavetables::kFrames - 2, (int) fpos);
        const float fmix = fpos - (float) f0;
        const float* t0 = wt.table (f0, level);
        const float* t1 = wt.table (f0 + 1, level);
        const float uniGain = 1.0f / std::sqrt ((float) uni);

        fL.set (cutoff, res, sr);
        fR.copyCoeffs (fL);

        static const float metalFreqs[6] = { 605.0f, 920.0f, 968.0f, 1090.0f, 1350.0f, 1943.0f };

        for (int i = 0; i < n; ++i)
        {
            float l = 0.0f, r = 0.0f;

            for (int u = 0; u < uni; ++u)
            {
                ph[u] += (double) inc[u];
                if (ph[u] >= 1.0) ph[u] -= 1.0;
                const float p = (float) ph[u] * (float) Wavetables::kSize;
                const int idx = (int) p;
                const float fr = p - (float) idx;
                const float a = t0[idx] + fr * (t0[idx + 1] - t0[idx]);
                const float b = t1[idx] + fr * (t1[idx + 1] - t1[idx]);
                const float s = a + fmix * (b - a);
                l += s * panL[u];
                r += s * panR[u];
            }

            l *= uniGain;
            r *= uniGain;

            if (noiseLevel > 0.0f)
            {
                float nl, nr;
                if (noiseType == 0)
                {
                    nl = rngL.bi() * 0.6f;
                    nr = rngR.bi() * 0.6f;
                }
                else if (noiseType == 1)
                {
                    nl = pinkNoise (0, rngL.bi());
                    nr = pinkNoise (1, rngR.bi());
                }
                else
                {
                    float m = 0.0f;
                    for (int k = 0; k < 6; ++k)
                    {
                        mph[k] += (double) (metalFreqs[k] * 2.3f / sr);
                        if (mph[k] >= 1.0) mph[k] -= 1.0;
                        m += mph[k] < 0.5 ? 1.0f : -1.0f;
                    }
                    nl = nr = metalHp.hp (m * (1.0f / 6.0f)) * 0.9f;
                }
                l += nl * noiseLevel;
                r += nr * noiseLevel;
            }

            fL.tick (l);
            fR.tick (r);
            const float env = adsr.getNextSample() * 0.7f;
            L[i] += fL.out (filterType) * env;
            R[i] += fR.out (filterType) * env;
        }

        if (! adsr.isActive()) active = false;
    }
};

//==============================================================================
// Layer 4: RISER / TRANSITION - tempo-locked noise + tone sweep generator.
struct RiserLayer
{
    bool active = false;
    float sr = 48000.0f;
    double pos = 0.0, len = 1.0;
    Svf fL, fR;
    Rng rL { 11 }, rR { 12 };
    double ph1 = 0.0, ph2 = 0.0, phs = 0.0;
    float gateSm = 1.0f;
    float progress = -1.0f;

    void prepare (float s)
    {
        sr = s;
        active = false;
        progress = -1.0f;
    }

    void trigger (double lengthSamples)
    {
        len = juce::jmax (64.0, lengthSamples);
        pos = 0.0;
        ph1 = ph2 = phs = 0.0;
        gateSm = 1.0f;
        fL.reset();
        fR.reset();
        active = true;
    }

    static float shape (float x, int curve)
    {
        x = clampf (x, 0.0f, 1.0f);
        return curve == 0 ? x : (std::exp (4.0f * x) - 1.0f) / (std::exp (4.0f) - 1.0f);
    }

    static float curveAt (float x, int mode, int curve)
    {
        if (mode == 0) return shape (x, curve);
        if (mode == 1) return shape (1.0f - x, curve);
        return shape (1.0f - std::abs (2.0f * x - 1.0f), curve);
    }

    void render (float* L, float* R, int n, int mode, int curve, float range, float tone,
                 float fEnd, double gateStepSamples)
    {
        if (! active)
        {
            progress = -1.0f;
            return;
        }

        const float fadeIn = 1.0f / (0.003f * sr);
        const float fadeOut = 1.0f / (0.004f * sr);
        const float gc = 1.0f - std::exp (-1.0f / (0.0015f * sr));
        float m = 0.0f, f = fEnd;

        for (int i = 0; i < n; ++i)
        {
            const float x = (float) (pos / len);
            if (x >= 1.0f)
            {
                active = false;
                break;
            }

            if ((i & 7) == 0)
            {
                m = curveAt (x, mode, curve);
                fL.set (180.0f * std::pow (2.0f, m * 6.4f), 0.5f, sr);
                fR.copyCoeffs (fL);
                f = fEnd * semis (-(1.0f - m) * range);
            }

            fL.tick (rL.bi());
            fR.tick (rR.bi());
            const float nl = fL.bp * 1.5f + fL.hp * 0.12f * m;
            const float nr = fR.bp * 1.5f + fR.hp * 0.12f * m;

            const float dt1 = f / sr, dt2 = f * 1.008f / sr;
            ph1 += (double) dt1; if (ph1 >= 1.0) ph1 -= 1.0;
            ph2 += (double) dt2; if (ph2 >= 1.0) ph2 -= 1.0;
            phs += (double) (dt1 * 0.5f); if (phs >= 1.0) phs -= 1.0;
            const float sub = 0.45f * std::sin (kTwoPi * (float) phs);
            const float tl = 0.5f * (2.0f * (float) ph1 - 1.0f - polyBlep ((float) ph1, dt1)) + sub;
            const float tr = 0.5f * (2.0f * (float) ph2 - 1.0f - polyBlep ((float) ph2, dt2)) + sub;

            float a = 0.08f + 0.92f * m;
            a *= juce::jmin (1.0f, (float) pos * fadeIn) * juce::jmin (1.0f, (float) (len - pos) * fadeOut);

            if (gateStepSamples > 1.0)
            {
                const double gp = std::fmod (pos, gateStepSamples) / gateStepSamples;
                gateSm += gc * ((gp < 0.5 ? 1.0f : 0.0f) - gateSm);
                a *= gateSm;
            }

            L[i] += (nl + tone * (tl - nl)) * a;
            R[i] += (nr + tone * (tr - nr)) * a;
            pos += 1.0;
        }

        progress = active ? (float) (pos / len) : -1.0f;
    }
};
} // namespace fl
