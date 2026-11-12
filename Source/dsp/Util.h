#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <memory>
#include <vector>

namespace fl
{
constexpr float kPi = 3.14159265358979f;
constexpr float kTwoPi = 6.28318530717959f;

inline float clampf (float x, float a, float b) { return x < a ? a : (x > b ? b : x); }
inline float dbToGain (float db) { return std::pow (10.0f, db * 0.05f); }
inline float semis (float st) { return std::pow (2.0f, st / 12.0f); }

struct Rng
{
    uint32_t s;
    explicit Rng (uint32_t seed = 0x12345678u) : s (seed ? seed : 1u) {}
    uint32_t u() { s ^= s << 13; s ^= s >> 17; s ^= s << 5; return s; }
    float uni() { return (float) (u() >> 8) * (1.0f / 16777216.0f); } // 0..1
    float bi() { return uni() * 2.0f - 1.0f; }                         // -1..1
};

struct OnePole
{
    float z = 0.0f, a = 1.0f;
    void setCutoff (float hz, float sr) { a = 1.0f - std::exp (-kTwoPi * clampf (hz, 1.0f, sr * 0.49f) / sr); }
    void reset() { z = 0.0f; }
    float lp (float x) { z += a * (x - z); return z; }
    float hp (float x) { return x - lp (x); }
};

// Topology-preserving state variable filter (Zavalishin / Cytomic form).
struct Svf
{
    float ic1 = 0.0f, ic2 = 0.0f;
    float a1 = 0.0f, a2 = 0.0f, a3 = 0.0f, k = 1.0f;
    float lp = 0.0f, bp = 0.0f, hp = 0.0f;

    void reset() { ic1 = ic2 = lp = bp = hp = 0.0f; }

    void set (float fc, float res, float sr)
    {
        fc = clampf (fc, 16.0f, sr * 0.45f);
        const float g = std::tan (kPi * fc / sr);
        const float q = 0.707f * std::pow (17.0f, clampf (res, 0.0f, 1.0f));
        k = 1.0f / q;
        a1 = 1.0f / (1.0f + g * (g + k));
        a2 = g * a1;
        a3 = g * a2;
    }

    void copyCoeffs (const Svf& o) { a1 = o.a1; a2 = o.a2; a3 = o.a3; k = o.k; }

    void tick (float x)
    {
        const float v3 = x - ic2;
        const float v1 = a1 * ic1 + a2 * v3;
        const float v2 = ic2 + a2 * ic1 + a3 * v3;
        ic1 = 2.0f * v1 - ic1;
        ic2 = 2.0f * v2 - ic2;
        lp = v2;
        bp = v1;
        hp = x - k * v1 - v2;
    }

    float out (int type) const { return type == 0 ? lp : (type == 1 ? hp : bp); }
};

inline float polyBlep (float t, float dt)
{
    if (t < dt) { t /= dt; return t + t - t * t - 1.0f; }
    if (t > 1.0f - dt) { t = (t - 1.0f) / dt; return t * t + t + t + 1.0f; }
    return 0.0f;
}

// A mono/stereo sample held fully in memory.
struct SampleData
{
    std::vector<float> l, r;
    double sr = 48000.0;
    juce::String name;
    std::vector<float> peaks; // coarse |peak| overview for the UI

    int length() const { return (int) l.size(); }

    void computePeaks (int points = 240)
    {
        peaks.assign ((size_t) points, 0.0f);
        const int n = length();
        if (n == 0) return;
        for (int i = 0; i < n; ++i)
        {
            const auto idx = (size_t) juce::jmin (points - 1, (int) ((int64_t) i * points / n));
            peaks[idx] = juce::jmax (peaks[idx], std::abs (l[(size_t) i]), std::abs (r[(size_t) i]));
        }
    }
};

struct HostInfo
{
    double bpm = 120.0;
    bool playing = false;
    double ppq = 0.0;     // position at the start of the block, quarter notes
    double ppqBar = 0.0;  // position of the last bar start
    int num = 4, den = 4;

    double beatsPerBar() const { return (double) num * 4.0 / (double) juce::jmax (1, den); }
};

// Beat length (in quarter notes) of the shared gate division list: OFF|1/8|1/16|1/32|1/8T|1/16T
inline double gateDivBeats (int idx)
{
    static const double d[] = { 0.0, 0.5, 0.25, 0.125, 1.0 / 3.0, 1.0 / 6.0 };
    return d[juce::jlimit (0, 5, idx)];
}

// 1/2|1/4|1/4D|1/8|1/8D|1/8T|1/16
inline double delayDivBeats (int idx)
{
    static const double d[] = { 2.0, 1.0, 1.5, 0.5, 0.75, 1.0 / 3.0, 0.25 };
    return d[juce::jlimit (0, 6, idx)];
}

// Snap a frequency to the nearest pitch of the given pitch class (0 = C).
inline float quantizeToKey (float hz, int key)
{
    const float m = 69.0f + 12.0f * std::log2 (juce::jmax (1.0f, hz) / 440.0f);
    float d = std::fmod (m - (float) key, 12.0f);
    if (d < 0.0f) d += 12.0f;
    float n = m - d;
    if (d > 6.0f) n += 12.0f;
    return 440.0f * std::pow (2.0f, (n - 69.0f) / 12.0f);
}
} // namespace fl
