#pragma once
#include "Util.h"

namespace fl
{
//==============================================================================
// TUBE | TAPE | HARD CLIP | BITCRUSH
struct Distortion
{
    float sr = 48000.0f;
    OnePole dcL, dcR, tpL, tpR;
    float holdL = 0.0f, holdR = 0.0f, holdCount = 0.0f;

    void prepare (float s)
    {
        sr = s;
        dcL.setCutoff (12.0f, s);
        dcR.setCutoff (12.0f, s);
        dcL.reset(); dcR.reset(); tpL.reset(); tpR.reset();
        holdL = holdR = holdCount = 0.0f;
    }

    void process (float* L, float* R, int n, int type, float drive, float mix)
    {
        const float pre = 1.0f + drive * drive * 30.0f;
        const float post = 1.0f / std::pow (pre, 0.55f);
        tpL.setCutoff (18000.0f - drive * 13000.0f, sr);
        tpR.a = tpL.a;
        const float steps = std::pow (2.0f, 15.0f - drive * 12.0f);
        const float hold = 1.0f + drive * drive * 48.0f * (sr / 48000.0f);

        for (int i = 0; i < n; ++i)
        {
            const float xl = L[i], xr = R[i];
            float yl, yr;

            switch (type)
            {
                case 0:
                    yl = dcL.hp (std::tanh (pre * xl + 0.15f)) * post * 1.3f;
                    yr = dcR.hp (std::tanh (pre * xr + 0.15f)) * post * 1.3f;
                    break;
                case 1:
                    yl = tpL.lp ((2.0f / kPi) * std::atan (pre * 1.3f * xl)) * post * 1.4f;
                    yr = tpR.lp ((2.0f / kPi) * std::atan (pre * 1.3f * xr)) * post * 1.4f;
                    break;
                case 2:
                    yl = clampf (pre * xl, -1.0f, 1.0f) * post;
                    yr = clampf (pre * xr, -1.0f, 1.0f) * post;
                    break;
                default:
                    holdCount += 1.0f;
                    if (holdCount >= hold)
                    {
                        holdCount -= hold;
                        holdL = std::round (xl * steps) / steps;
                        holdR = std::round (xr * steps) / steps;
                    }
                    yl = holdL;
                    yr = holdR;
                    break;
            }

            L[i] = xl + mix * (yl - xl);
            R[i] = xr + mix * (yr - xr);
        }
    }
};

//==============================================================================
// Multimode 12/24 dB filter with LFO and envelope-follower modulation.
struct MasterFilter
{
    float sr = 48000.0f;
    Svf a[2], b[2];
    float env = 0.0f;
    double lfoPh = 0.0;
    float att = 0.0f, rel = 0.0f;

    void prepare (float s)
    {
        sr = s;
        att = 1.0f - std::exp (-1.0f / (0.005f * sr));
        rel = 1.0f - std::exp (-1.0f / (0.150f * sr));
        for (auto& f : a) f.reset();
        for (auto& f : b) f.reset();
        env = 0.0f;
    }

    void process (float* L, float* R, int n, int type, bool slope24, float cut, float res,
                  float lfoRate, float lfoDepth, float envDepth)
    {
        const double lfoInc = (double) lfoRate / sr;

        for (int i = 0; i < n; ++i)
        {
            const float mono = std::abs (L[i] + R[i]) * 0.5f;
            env += (mono > env ? att : rel) * (mono - env);

            if ((i & 15) == 0)
            {
                const float lfo = std::sin (kTwoPi * (float) lfoPh);
                const float oct = 4.0f * (lfoDepth * lfo + envDepth * juce::jmin (1.0f, env * 2.0f));
                a[0].set (cut * std::pow (2.0f, oct), res, sr);
                a[1].copyCoeffs (a[0]);
                b[0].copyCoeffs (a[0]);
                b[1].copyCoeffs (a[0]);
            }

            lfoPh += lfoInc;
            if (lfoPh >= 1.0) lfoPh -= 1.0;

            a[0].tick (L[i]);
            a[1].tick (R[i]);
            float l = a[0].out (type), r = a[1].out (type);

            if (slope24)
            {
                b[0].tick (l);
                b[1].tick (r);
                l = b[0].out (type);
                r = b[1].out (type);
            }

            L[i] = l;
            R[i] = r;
        }
    }
};

//==============================================================================
// Rhythmic gate locked to the host grid (or free-running when the transport is stopped).
struct Gate
{
    float sm = 1.0f;
    double counter = 0.0;

    void resetPhase() { counter = 0.0; }

    void process (float* L, float* R, int n, double stepBeats, float depth, const HostInfo& h, float sr)
    {
        const float c = 1.0f - std::exp (-1.0f / (0.0015f * sr));
        const double bps = h.bpm / (60.0 * (double) sr);
        double pos = h.playing ? h.ppq : counter;

        for (int i = 0; i < n; ++i)
        {
            float target = 1.0f;
            if (stepBeats > 0.0)
            {
                double ph = pos / stepBeats;
                ph -= std::floor (ph);
                target = ph < 0.5 ? 1.0f : 1.0f - depth;
            }
            sm += c * (target - sm);
            L[i] *= sm;
            R[i] *= sm;
            pos += bps;
        }

        if (! h.playing) counter = pos;
    }
};

//==============================================================================
// Tempo-synced ping-pong delay with lo/hi cut in the feedback path.
struct PingPong
{
    float sr = 48000.0f;
    std::vector<float> bL, bR;
    int mask = 0, w = 0;
    float cur = 12000.0f;
    OnePole lcL, lcR, hcL, hcR;

    void prepare (float s)
    {
        sr = s;
        int size = 1;
        while (size < (int) (s * 4.0f) + 8) size <<= 1;
        bL.assign ((size_t) size, 0.0f);
        bR.assign ((size_t) size, 0.0f);
        mask = size - 1;
        w = 0;
        lcL.reset(); lcR.reset(); hcL.reset(); hcR.reset();
    }

    float read (const std::vector<float>& b, float d) const
    {
        const float rp = (float) w - d;
        const int i0 = (int) std::floor (rp);
        const float fr = rp - (float) i0;
        const float x0 = b[(size_t) (i0 & mask)];
        const float x1 = b[(size_t) ((i0 + 1) & mask)];
        return x0 + fr * (x1 - x0);
    }

    void process (const float* in, float* outL, float* outR, int n,
                  float delaySamples, float fb, float loCut, float hiCut)
    {
        const float tgt = clampf (delaySamples, 16.0f, (float) mask - 8.0f);
        const float slew = 1.0f - std::exp (-1.0f / (0.08f * sr));
        lcL.setCutoff (loCut, sr); lcR.a = lcL.a;
        hcL.setCutoff (hiCut, sr); hcR.a = hcL.a;

        for (int i = 0; i < n; ++i)
        {
            cur += slew * (tgt - cur);
            const float dl = read (bL, cur);
            const float dr = read (bR, cur);
            float wl = in[i] + fb * dr;
            float wr = fb * dl;
            wl = lcL.hp (hcL.lp (wl));
            wr = lcR.hp (hcR.lp (wr));
            bL[(size_t) w] = wl;
            bR[(size_t) w] = wr;
            outL[i] = dl;
            outR[i] = dr;
            w = (w + 1) & mask;
        }
    }
};

//==============================================================================
// 8-line feedback delay network: DARK HALL | METALLIC ROOM | INFINITE CANYON
struct Reverb
{
    static constexpr int N = 8, kLen = 1 << 16;

    struct AP
    {
        std::vector<float> b;
        int w = 0, len = 1;
        float tick (float x, float g)
        {
            const float d = b[(size_t) w];
            const float y = -g * x + d;
            b[(size_t) w] = x + g * y;
            if (++w >= len) w = 0;
            return y;
        }
    };

    float sr = 48000.0f;
    std::vector<float> buf[N];
    int w = 0;
    float cur[N] {}, tgt[N] {}, g[N] {};
    OnePole damp[N], inHp;
    AP ap[4];
    float apG = 0.6f;

    void prepare (float s)
    {
        sr = s;
        for (auto& b : buf) b.assign ((size_t) kLen, 0.0f);
        static const float apMs[4] = { 4.7f, 3.6f, 12.7f, 9.3f };
        for (int i = 0; i < 4; ++i)
        {
            ap[i].len = juce::jmax (2, (int) (apMs[i] * 0.001f * s));
            ap[i].b.assign ((size_t) ap[i].len, 0.0f);
            ap[i].w = 0;
        }
        for (auto& d : damp) d.reset();
        inHp.setCutoff (90.0f, s);
        inHp.reset();
        w = 0;
        set (0, 0.5f);
        for (int i = 0; i < N; ++i) cur[i] = tgt[i];
    }

    void set (int mode, float size)
    {
        static const float ms[3][N] = {
            { 31.7f, 37.1f, 41.3f, 47.9f, 53.3f, 61.1f, 67.9f, 73.3f },
            { 5.1f, 6.3f, 7.9f, 9.7f, 11.3f, 12.7f, 14.9f, 16.1f },
            { 59.3f, 71.9f, 83.7f, 97.1f, 109.3f, 127.1f, 139.9f, 151.7f }
        };

        mode = juce::jlimit (0, 2, mode);
        float scale, rt60, dampHz;

        if (mode == 0)      { scale = 0.6f + size;        rt60 = 1.5f + size * 6.0f;          dampHz = 2600.0f; apG = 0.62f; }
        else if (mode == 1) { scale = 0.6f + size;        rt60 = 0.4f + size * 2.6f;          dampHz = 9500.0f; apG = 0.25f; }
        else                { scale = 0.7f + size * 0.6f; rt60 = 6.0f + size * size * 80.0f;  dampHz = 5200.0f; apG = 0.68f; }

        for (int i = 0; i < N; ++i)
        {
            tgt[i] = clampf (ms[mode][i] * scale * 0.001f * sr, 8.0f, (float) kLen - 8.0f);
            g[i] = std::pow (10.0f, -3.0f * tgt[i] / (rt60 * sr));
            damp[i].setCutoff (dampHz, sr);
        }
    }

    void process (const float* inL, const float* inR, float* outL, float* outR, int n)
    {
        const int mask = kLen - 1;
        const float slew = 1.0f - std::exp (-1.0f / (0.06f * sr));

        for (int s = 0; s < n; ++s)
        {
            float x = inHp.hp ((inL[s] + inR[s]) * 0.5f);
            for (auto& a : ap) x = a.tick (x, apG);

            float v[N];
            float sum = 0.0f;

            for (int i = 0; i < N; ++i)
            {
                cur[i] += slew * (tgt[i] - cur[i]);
                const float rp = (float) w - cur[i];
                const int i0 = (int) std::floor (rp);
                const float fr = rp - (float) i0;
                const float x0 = buf[i][(size_t) (i0 & mask)];
                const float x1 = buf[i][(size_t) ((i0 + 1) & mask)];
                v[i] = damp[i].lp (x0 + fr * (x1 - x0)) * g[i];
                sum += v[i];
            }

            sum *= 2.0f / (float) N;

            for (int i = 0; i < N; ++i)
                buf[i][(size_t) w] = ((i & 1) ? -x : x) * 0.5f + (v[i] - sum);

            outL[s] = (v[0] + v[2] + v[4] + v[6]) * 0.35f;
            outR[s] = (v[1] + v[3] + v[5] + v[7]) * 0.35f;
            w = (w + 1) & mask;
        }
    }
};

//==============================================================================
// Tape stop: playback speed ramps down to zero while engaged.
struct TapeStop
{
    std::vector<float> bL, bR;
    int mask = 0, w = 0;
    bool stopping = false;
    double rp = 0.0;
    float speed = 1.0f, xf = 1.0f;

    void prepare (float sr)
    {
        int size = 1;
        while (size < (int) (sr * 2.3f)) size <<= 1;
        bL.assign ((size_t) size, 0.0f);
        bR.assign ((size_t) size, 0.0f);
        mask = size - 1;
        w = 0;
        stopping = false;
        xf = 1.0f;
    }

    void process (float* L, float* R, int n, bool engage, float timeSec, float sr)
    {
        if (! engage && ! stopping && xf >= 1.0f)
        {
            // Idle: only keep the history buffer up to date.
            for (int i = 0; i < n; ++i)
            {
                bL[(size_t) w] = L[i];
                bR[(size_t) w] = R[i];
                w = (w + 1) & mask;
            }
            return;
        }

        const float dec = 1.0f / (juce::jmax (0.02f, timeSec) * sr);
        const float xfInc = 1.0f / (0.01f * sr);
        const double size = (double) (mask + 1);

        for (int i = 0; i < n; ++i)
        {
            bL[(size_t) w] = L[i];
            bR[(size_t) w] = R[i];

            if (engage && ! stopping) { stopping = true; rp = (double) w; speed = 1.0f; }
            if (! engage && stopping) { stopping = false; xf = 0.0f; }

            if (stopping)
            {
                const int i0 = (int) rp;
                const float fr = (float) (rp - (double) i0);
                const float g = juce::jmin (1.0f, speed * 6.0f);
                L[i] = (bL[(size_t) (i0 & mask)] + fr * (bL[(size_t) ((i0 + 1) & mask)] - bL[(size_t) (i0 & mask)])) * g;
                R[i] = (bR[(size_t) (i0 & mask)] + fr * (bR[(size_t) ((i0 + 1) & mask)] - bR[(size_t) (i0 & mask)])) * g;
                speed = juce::jmax (0.0f, speed - dec);
                rp += (double) speed;
                if (rp >= size) rp -= size;
            }
            else if (xf < 1.0f)
            {
                xf = juce::jmin (1.0f, xf + xfInc);
                L[i] *= xf;
                R[i] *= xf;
            }

            w = (w + 1) & mask;
        }
    }
};

//==============================================================================
// Output brickwall limiter / soft clipper.
struct Limiter
{
    float g = 1.0f, rel = 0.001f;
    float minGain = 1.0f; // lowest gain in the last block, for metering

    void prepare (float sr)
    {
        rel = 1.0f - std::exp (-1.0f / (0.08f * sr));
        g = 1.0f;
    }

    void process (float* L, float* R, int n, bool clipper, float ceiling)
    {
        minGain = 1.0f;

        if (clipper)
        {
            const float inv = 1.0f / ceiling;
            for (int i = 0; i < n; ++i)
            {
                const float pk = juce::jmax (std::abs (L[i]), std::abs (R[i]));
                L[i] = ceiling * std::tanh (L[i] * inv);
                R[i] = ceiling * std::tanh (R[i] * inv);
                if (pk > 1.0e-6f)
                    minGain = juce::jmin (minGain, juce::jmax (std::abs (L[i]), std::abs (R[i])) / pk);
            }
            return;
        }

        for (int i = 0; i < n; ++i)
        {
            const float pk = juce::jmax (std::abs (L[i]), std::abs (R[i]));
            const float target = pk > ceiling ? ceiling / pk : 1.0f;
            if (target < g) g = target;
            else g += rel * (target - g);
            L[i] = clampf (L[i] * g, -ceiling, ceiling);
            R[i] = clampf (R[i] * g, -ceiling, ceiling);
            minGain = juce::jmin (minGain, g);
        }
    }
};
} // namespace fl
