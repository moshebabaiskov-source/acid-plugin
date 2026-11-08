#pragma once
// AcidLine - monophonic acid bass voice (no JUCE dependency, pure C++)
#include <cmath>
#include <algorithm>

struct AcidParams
{
    float waveform  = 0.0f;    // 0 = saw, 1 = square (blend in between)
    float cutoffHz  = 500.0f;  // base filter cutoff
    float resonance = 0.7f;    // 0..1
    float envMod    = 0.6f;    // 0..1 (filter envelope depth)
    float decaySec  = 0.4f;    // filter envelope decay
    float accent    = 0.6f;    // 0..1 accent strength
    float glideSec  = 0.06f;   // slide time
    float drive     = 0.3f;    // 0..1 distortion
    float volume    = 0.7f;    // linear gain
};

class AcidVoice
{
public:
    void prepare (double sampleRate)
    {
        sr = (float) sampleRate;
        reset();
    }

    void reset()
    {
        for (auto& s : z) s = 0.0f;
        phase = 0.0f; ampEnv = 0.0f; filtEnv = 0.0f; accEnv = 0.0f;
        gate = false;
    }

    // slide = true -> legato: glide pitch, don't retrigger envelopes
    void noteOn (int midiNote, float velocity, bool slide)
    {
        targetFreq = 440.0f * std::pow (2.0f, (midiNote - 69) / 12.0f);
        if (! slide || ! gate)
        {
            if (! gate) currentFreq = targetFreq;     // fresh note: jump to pitch
            filtEnv = 1.0f;
            accented = velocity >= 0.78f;             // velocity >= ~100
            if (accented) accEnv = 1.0f;
        }
        sliding = slide && gate;
        gate = true;
    }

    void slideTo (int midiNote)
    {
        targetFreq = 440.0f * std::pow (2.0f, (midiNote - 69) / 12.0f);
        sliding = true;
    }

    void noteOff() { gate = false; }

    bool isActive() const { return gate || ampEnv > 1.0e-5f; }

    float process (const AcidParams& p)
    {
        // ---- pitch / glide ----
        if (sliding && p.glideSec > 0.001f)
        {
            const float coef = std::exp (-1.0f / (p.glideSec * 0.33f * sr));
            currentFreq = targetFreq + (currentFreq - targetFreq) * coef;
        }
        else
            currentFreq = targetFreq;

        // ---- envelopes ----
        const float ampTarget = gate ? 1.0f : 0.0f;
        const float ampTime   = gate ? 0.003f : 0.010f;   // quick attack / release
        ampEnv = ampTarget + (ampEnv - ampTarget) * std::exp (-1.0f / (ampTime * sr));

        const float decay = accented ? 0.2f : p.decaySec; // accent = short, punchy env
        filtEnv *= std::exp (-4.6f / (decay * sr));
        accEnv  *= std::exp (-4.6f / (0.25f * sr));

        // ---- oscillator (PolyBLEP saw + square) ----
        const float dt = std::min (currentFreq / sr, 0.45f);
        phase += dt;
        if (phase >= 1.0f) phase -= 1.0f;

        float saw = 2.0f * phase - 1.0f;
        saw -= polyBlep (phase, dt);

        float sq = phase < 0.5f ? 1.0f : -1.0f;
        sq += polyBlep (phase, dt);
        float p2 = phase + 0.5f; if (p2 >= 1.0f) p2 -= 1.0f;
        sq -= polyBlep (p2, dt);

        float osc = saw * (1.0f - p.waveform) + sq * p.waveform;

        // ---- cutoff modulation ----
        const float acc   = p.accent * accEnv;
        const float octs  = p.envMod * 5.0f * filtEnv + acc * 3.0f;
        float fc = p.cutoffHz * std::pow (2.0f, octs);
        fc = std::clamp (fc, 20.0f, 0.45f * sr);

        // ---- 4-pole ZDF ladder filter with saturation ----
        const float g = std::tan (3.14159265f * fc / sr);
        const float G = g / (1.0f + g);
        const float k = p.resonance * (3.9f + acc * 0.3f);    // ~4 = self-oscillation

        const float S = G * G * G * (z[0] / (1.0f + g))
                      + G * G * (z[1] / (1.0f + g))
                      + G * (z[2] / (1.0f + g))
                      + (z[3] / (1.0f + g));
        const float G4 = G * G * G * G;
        float u = (osc - k * S) / (1.0f + k * G4);
        u = std::tanh (u * 1.2f);

        float x = u;
        for (int i = 0; i < 4; ++i)
        {
            const float v = G * (x - z[i]);
            const float y = v + z[i];
            z[i] = y + v;
            x = y;
        }
        float out = x * (1.0f + 0.6f * p.resonance);   // make up bass lost to resonance

        // ---- drive ----
        const float d = 1.0f + p.drive * 12.0f;
        out = std::tanh (out * d) / std::tanh (d);

        const float accGain = accented ? (1.0f + p.accent * 0.8f) : 1.0f;
        return out * ampEnv * accGain * p.volume * 0.5f;
    }

private:
    static float polyBlep (float t, float dt)
    {
        if (t < dt)          { t /= dt;               return t + t - t * t - 1.0f; }
        if (t > 1.0f - dt)   { t = (t - 1.0f) / dt;   return t * t + t + t + 1.0f; }
        return 0.0f;
    }

    float sr = 44100.0f;
    float z[4] {};
    float phase = 0.0f, currentFreq = 110.0f, targetFreq = 110.0f;
    float ampEnv = 0.0f, filtEnv = 0.0f, accEnv = 0.0f;
    bool gate = false, sliding = false, accented = false;
};
