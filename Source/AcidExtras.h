#pragma once
// AcidLine extras: built-in pattern sequencer + guitar-amp style distortion (pure C++)
#include "AcidVoice.h"
#include <climits>

// ======================= Pattern sequencer =======================
struct AcidStep { bool on; int semis; bool accent; bool slide; };

namespace AcidPatterns
{
    constexpr int numSteps = 16;
    constexpr int numPatterns = 4;
    // shorthand: N(semis) note, A(semis) accent, S(semis) slide, AS(semis) accent+slide, R rest
    #define N(x)  { true,  x, false, false }
    #define A(x)  { true,  x, true,  false }
    #define S(x)  { true,  x, false, true  }
    #define AS(x) { true,  x, true,  true  }
    #define R     { false, 0, false, false }
    inline const AcidStep patterns[numPatterns][numSteps] = {
        // 1: Classic acid
        { A(0), N(0), S(12), N(0), R, A(3), N(0), S(7), N(0), A(0), S(10), N(12), N(0), R, AS(5), N(3) },
        // 2: Rolling psy (octave jumps)
        { N(0), N(12), N(0), A(0), S(12), N(0), N(3), N(0), N(0), N(12), A(0), S(7), N(0), N(10), N(0), A(12) },
        // 3: Squelchy
        { A(0), R, N(0), N(0), S(3), N(5), A(0), R, S(7), N(10), N(0), A(0), R, S(12), N(0), N(0) },
        // 4: Hypnotic minimal
        { N(0), N(0), A(0), S(0), N(1), N(0), N(0), A(0), N(0), N(0), S(0), N(3), N(0), N(0), A(0), R },
    };
    #undef N
    #undef A
    #undef S
    #undef AS
    #undef R
}

class AcidSequencer
{
public:
    // stepPos = position in 16th notes (ppq * 4). Call once per sample while playing.
    void tick (double stepPos, int pattern, int rootNote, AcidVoice& voice)
    {
        const auto& pat = AcidPatterns::patterns[pattern];
        const long long absStep = (long long) std::floor (stepPos);
        const int idx = (int) (((absStep % 16) + 16) % 16);
        const double frac = stepPos - (double) absStep;

        if (absStep != lastStep)
        {
            const auto& s = pat[idx];
            const auto& prev = pat[(idx + 15) % 16];
            const bool slideIn = gateOpen && prev.on && prev.slide && lastStep == absStep - 1;
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
            lastStep = absStep;
        }

        // release at ~55% of the step, unless this step slides into the next one
        if (gateOpen && frac >= 0.55 && ! pat[idx].slide)
        {
            voice.noteOff();
            gateOpen = false;
        }
    }

    int currentIndex() const
    {
        return lastStep == LLONG_MIN ? -1 : (int) (((lastStep % 16) + 16) % 16);
    }

    void stop (AcidVoice& voice)
    {
        if (gateOpen) voice.noteOff();
        gateOpen = false;
        lastStep = LLONG_MIN;
    }

private:
    long long lastStep = LLONG_MIN;
    bool gateOpen = false;
};

// ======================= Guitar-amp distortion =======================
// tighten (HPF) -> big gain -> asymmetric two-stage clipping -> cabinet-like low-pass
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

        // tighten low end before clipping (like a tube screamer into an amp)
        const float hp = hpCoef * (hpY + x - hpX);
        hpX = x; hpY = hp;
        const float tightened = hp + 0.35f * x;

        // gain stages with asymmetric clipping -> warm even harmonics
        const float gain = 2.0f + amount * 45.0f;
        float y = tightened * gain;
        y = y > 0.0f ? std::tanh (y) : std::tanh (y * 0.7f) / 0.7f * 0.85f;
        y = std::tanh (y * (1.0f + amount * 2.0f) + 0.1f) - std::tanh (0.1f);

        // cabinet: two one-pole low-passes take off the fizz
        lp1 = y   + cabCoef * (lp1 - y);
        lp2 = lp1 + cabCoef * (lp2 - lp1);

        const float wet = lp2 * 0.4f / (0.6f + amount * 0.6f);   // keep loudness roughly even
        const float mix = std::min (1.0f, amount * 2.5f);
        return x * (1.0f - mix) + wet * mix;
    }

private:
    float sr = 44100.0f, hpCoef = 0.0f, cabCoef = 0.0f;
    float hpX = 0.0f, hpY = 0.0f, lp1 = 0.0f, lp2 = 0.0f;
};
