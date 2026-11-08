#pragma once
// AcidLine extras: pattern bank + sequencer, guitar-amp distortion, ping-pong delay (pure C++)
#include "AcidVoice.h"
#include <climits>
#include <random>
#include <vector>

// ======================= Patterns =======================
struct AcidStep
{
    bool on = false; int semis = 0; bool accent = false; bool slide = false;

    // pack into one int so the UI and audio thread can share it through std::atomic<int>
    int pack() const { return (on ? 1 : 0) | (accent ? 2 : 0) | (slide ? 4 : 0) | ((semis + 64) << 8); }
    static AcidStep unpack (int v) { return { (v & 1) != 0, ((v >> 8) & 0xff) - 64, (v & 2) != 0, (v & 4) != 0 }; }
};

namespace AcidPatterns
{
    constexpr int numSteps = 16;
    constexpr int numPatterns = 10;
    constexpr int minSemis = -12, maxSemis = 24;

    inline const char* names[numPatterns] = {
        "Classic Acid", "Rolling Psy", "Squelchy", "Hypnotic", "Full On",
        "Goa", "Offbeat Stabs", "Dark Driver", "Octave Bounce", "Slide Madness"
    };

    #define N(x)  AcidStep { true,  x, false, false }
    #define A(x)  AcidStep { true,  x, true,  false }
    #define S(x)  AcidStep { true,  x, false, true  }
    #define AS(x) AcidStep { true,  x, true,  true  }
    #define R     AcidStep { false, 0, false, false }
    inline const AcidStep patterns[numPatterns][numSteps] = {
        { A(0), N(0), S(12), N(0), R, A(3), N(0), S(7), N(0), A(0), S(10), N(12), N(0), R, AS(5), N(3) },
        { N(0), N(12), N(0), A(0), S(12), N(0), N(3), N(0), N(0), N(12), A(0), S(7), N(0), N(10), N(0), A(12) },
        { A(0), R, N(0), N(0), S(3), N(5), A(0), R, S(7), N(10), N(0), A(0), R, S(12), N(0), N(0) },
        { N(0), N(0), A(0), S(0), N(1), N(0), N(0), A(0), N(0), N(0), S(0), N(3), N(0), N(0), A(0), R },
        { N(0), N(0), N(12), N(0), N(0), N(0), A(12), N(0), N(0), N(0), N(12), N(0), N(0), S(7), N(10), A(12) },
        { A(0), S(7), N(12), N(10), N(7), N(3), A(0), R, N(0), S(5), N(7), N(3), A(12), N(10), S(7), N(5) },
        { R, R, A(0), R, R, R, A(12), R, R, R, A(3), R, R, R, AS(7), N(5) },
        { N(0), N(1), N(0), A(0), N(0), N(1), S(0), N(-2), N(0), N(1), N(0), A(0), N(0), S(6), N(1), N(0) },
        { N(0), N(12), N(0), N(12), A(0), N(12), N(0), S(12), N(0), N(12), N(3), N(15), A(0), N(12), S(7), N(19) },
        { AS(0), S(12), S(3), N(15), AS(0), S(7), S(10), N(12), AS(0), S(12), S(5), N(17), AS(0), S(10), S(7), N(3) },
    };
    #undef N
    #undef A
    #undef S
    #undef AS
    #undef R

    // random acid line in a minor scale, weighted toward the root
    inline void randomise (AcidStep out[numSteps], unsigned seed)
    {
        static const int notes[] = { 0, 0, 0, 0, 3, 5, 7, 10, 12, 12, 15, -2 };
        std::mt19937 rng (seed);
        std::uniform_real_distribution<float> u (0.0f, 1.0f);
        for (int i = 0; i < numSteps; ++i)
        {
            auto& s = out[i];
            s.on = u (rng) < (i % 4 == 0 ? 0.95f : 0.75f);
            s.semis = notes[rng() % (sizeof (notes) / sizeof (int))];
            s.accent = s.on && u (rng) < 0.22f;
            s.slide = s.on && u (rng) < 0.2f;
        }
    }
}

// ======================= Sequencer =======================
class AcidSequencer
{
public:
    // stepPos = position in 16th notes (ppq * 4). Call once per sample while playing.
    void tick (double stepPos, const AcidStep* pat, int rootNote, AcidVoice& voice)
    {
        const long long absStep = (long long) std::floor (stepPos);
        const int idx = (int) (((absStep % 16) + 16) % 16);
        const double frac = stepPos - (double) absStep;

        if (absStep != lastStep)
        {
            const auto& s = pat[idx];
            const bool slideIn = gateOpen && prevSlide && lastStep == absStep - 1;
            if (s.on)
            {
                voice.noteOn (rootNote + s.semis, s.accent ? 1.0f : 0.6f, slideIn);
                gateOpen = true;
            }
            else if (gateOpen)
            {
                voice.noteOff();
                gateOpen = false;
            }
            curSlide = s.on && s.slide;
            prevSlide = curSlide;
            lastStep = absStep;
        }

        // release at ~55% of the step, unless this step slides into the next one
        if (gateOpen && frac >= 0.55 && ! curSlide)
        {
            voice.noteOff();
            gateOpen = false;
        }
    }

    int currentIndex() const { return lastStep == LLONG_MIN ? -1 : (int) (((lastStep % 16) + 16) % 16); }

    void stop (AcidVoice& voice)
    {
        if (gateOpen) voice.noteOff();
        gateOpen = false;
        prevSlide = curSlide = false;
        lastStep = LLONG_MIN;
    }

private:
    long long lastStep = LLONG_MIN;
    bool gateOpen = false, curSlide = false, prevSlide = false;
};

// ======================= Guitar-amp distortion =======================
class GuitarDist
{
public:
    void prepare (double sampleRate)
    {
        sr = (float) sampleRate;
        hpCoef  = std::exp (-2.0f * 3.14159265f * 120.0f / sr);
        cabCoef = std::exp (-2.0f * 3.14159265f * 4200.0f / sr);
        hpX = hpY = lp1 = lp2 = 0.0f;
    }

    float process (float x, float amount)
    {
        if (amount <= 0.001f) return x;
        const float hp = hpCoef * (hpY + x - hpX);
        hpX = x; hpY = hp;
        const float tightened = hp + 0.35f * x;

        const float gain = 2.0f + amount * 45.0f;
        float y = tightened * gain;
        y = y > 0.0f ? std::tanh (y) : std::tanh (y * 0.7f) / 0.7f * 0.85f;
        y = std::tanh (y * (1.0f + amount * 2.0f) + 0.1f) - std::tanh (0.1f);

        lp1 = y   + cabCoef * (lp1 - y);
        lp2 = lp1 + cabCoef * (lp2 - lp1);

        const float wet = lp2 * 0.4f / (0.6f + amount * 0.6f);
        const float mix = std::min (1.0f, amount * 2.5f);
        return x * (1.0f - mix) + wet * mix;
    }

private:
    float sr = 44100.0f, hpCoef = 0.0f, cabCoef = 0.0f;
    float hpX = 0.0f, hpY = 0.0f, lp1 = 0.0f, lp2 = 0.0f;
};

// ======================= Ping-pong delay =======================
// Bass is filtered out of the echoes (HPF ~250 Hz) so the low end stays tight,
// and the repeats get darker each time (LPF ~3.5 kHz) for a dub feel.
class PingPongDelay
{
public:
    void prepare (double sampleRate, float maxSeconds = 2.5f)
    {
        sr = (float) sampleRate;
        size = (int) (maxSeconds * sr) + 4;
        bufL.assign ((size_t) size, 0.0f);
        bufR.assign ((size_t) size, 0.0f);
        writePos = 0;
        hpC = std::exp (-2.0f * 3.14159265f * 250.0f / sr);
        lpC = std::exp (-2.0f * 3.14159265f * 3500.0f / sr);
        smoothC = std::exp (-1.0f / (0.05f * sr));
        for (auto& f : st) f = {};
        curDelay = -1.0f;
    }

    void process (float in, float delaySamples, float feedback, float mix, float& outL, float& outR)
    {
        delaySamples = std::clamp (delaySamples, 1.0f, (float) size - 3.0f);
        if (curDelay < 0.0f) curDelay = delaySamples;
        curDelay = delaySamples + (curDelay - delaySamples) * smoothC;   // glide on tempo changes

        const float dL = read (bufL, curDelay);
        const float dR = read (bufR, curDelay);

        bufL[(size_t) writePos] = filt (st[0], in + feedback * dR);
        bufR[(size_t) writePos] = filt (st[1], feedback * dL);
        if (++writePos >= size) writePos = 0;

        outL = in + mix * dL;
        outR = in + mix * dR;
    }

private:
    struct FiltState { float hpX = 0, hpY = 0, lp = 0; };

    float filt (FiltState& s, float x)
    {
        const float hp = hpC * (s.hpY + x - s.hpX);
        s.hpX = x; s.hpY = hp;
        s.lp = hp + lpC * (s.lp - hp);
        return s.lp;
    }

    float read (const std::vector<float>& b, float d) const
    {
        float rp = (float) writePos - d;
        while (rp < 0.0f) rp += (float) size;
        const int i0 = (int) rp;
        const int i1 = (i0 + 1) % size;
        const float fr = rp - (float) i0;
        return b[(size_t) i0] + fr * (b[(size_t) i1] - b[(size_t) i0]);
    }

    float sr = 44100.0f, hpC = 0.0f, lpC = 0.0f, smoothC = 0.0f, curDelay = -1.0f;
    int size = 1, writePos = 0;
    std::vector<float> bufL, bufR;
    FiltState st[2];
};
