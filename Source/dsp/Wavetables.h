#pragma once
#include "Util.h"

namespace fl
{
// 8 morphable wavetable frames, each band-limited into 7 mip levels (256 .. 4 harmonics).
// Frames: SINE, TRIANGLE, SAW, SQUARE, PULSE, ORGAN, METAL, GROWL
struct Wavetables
{
    static constexpr int kFrames = 8, kLevels = 7, kSize = 2048;

    std::vector<float> data;

    const float* table (int frame, int level) const
    {
        return data.data() + (size_t) (frame * kLevels + level) * (size_t) (kSize + 1);
    }

    static int levelFor (float freq, float sr)
    {
        const float maxH = sr * 0.5f / juce::jmax (1.0f, freq);
        int level = 0;
        while (level < kLevels - 1 && (float) (256 >> level) > maxH)
            ++level;
        return level;
    }

    static float harmonicAmp (int frame, int h)
    {
        const float fh = (float) h;
        switch (frame)
        {
            case 0: return h == 1 ? 1.0f : 0.0f;
            case 1: return (h & 1) ? (((h / 2) & 1) ? -1.0f : 1.0f) / (fh * fh) : 0.0f;
            case 2: return 1.0f / fh;
            case 3: return (h & 1) ? 1.0f / fh : 0.0f;
            case 4: return 2.0f * std::sin (kPi * fh * 0.22f) / fh;
            case 5:
            {
                static const int draw[] = { 1, 2, 3, 4, 6, 8, 10, 12, 16 };
                for (int d : draw)
                    if (d == h) return 1.0f / std::sqrt (fh);
                return 0.0f;
            }
            case 6:
            {
                static const int primes[] = { 1, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 47, 53 };
                for (int p : primes)
                    if (p == h) return 1.0f / std::sqrt (fh);
                return 0.04f / fh;
            }
            default:
            {
                const float f1 = std::exp (-(fh - 9.0f) * (fh - 9.0f) / 8.0f);
                const float f2 = std::exp (-(fh - 23.0f) * (fh - 23.0f) / 18.0f);
                return (1.0f / fh) * (1.0f + 3.0f * f1 + 2.0f * f2);
            }
        }
    }

    Wavetables()
    {
        data.assign ((size_t) (kFrames * kLevels) * (size_t) (kSize + 1), 0.0f);

        std::vector<float> sine ((size_t) kSize);
        for (int i = 0; i < kSize; ++i)
            sine[(size_t) i] = std::sin (kTwoPi * (float) i / (float) kSize);

        for (int f = 0; f < kFrames; ++f)
        {
            float norm = 1.0f;

            for (int lv = 0; lv < kLevels; ++lv)
            {
                auto* t = data.data() + (size_t) (f * kLevels + lv) * (size_t) (kSize + 1);
                const int maxH = 256 >> lv;

                for (int h = 1; h <= maxH; ++h)
                {
                    const float a = harmonicAmp (f, h);
                    if (a == 0.0f) continue;
                    for (int i = 0; i < kSize; ++i)
                        t[i] += a * sine[(size_t) ((h * i) & (kSize - 1))];
                }

                if (lv == 0)
                {
                    float pk = 0.0f;
                    for (int i = 0; i < kSize; ++i) pk = juce::jmax (pk, std::abs (t[i]));
                    norm = pk > 0.0f ? 1.0f / pk : 1.0f;
                }

                for (int i = 0; i < kSize; ++i) t[i] *= norm;
                t[kSize] = t[0]; // guard sample for interpolation
            }
        }
    }

    static const Wavetables& get()
    {
        static const Wavetables instance;
        return instance;
    }
};
} // namespace fl
