#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    const float delayBeats[] = { 0.25f, 0.5f, 0.75f, 1.0f };   // 1/16, 1/8, 1/8 dotted, 1/4
}

AcidLineProcessor::AcidLineProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createLayout())
{
    heldNotes.reserve (32);
    loadPreset (0);
}

juce::AudioProcessorValueTreeState::ParameterLayout AcidLineProcessor::createLayout()
{
    using namespace juce;
    std::vector<std::unique_ptr<RangedAudioParameter>> p;

    auto range = [] (float lo, float hi, float centre)
    {
        NormalisableRange<float> r (lo, hi);
        r.setSkewForCentre (centre);
        return r;
    };

    // filter (the 4 main knobs)
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "cutoff", 1 }, "Cutoff",    range (40.0f, 8000.0f, 600.0f), 400.0f));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "reso", 1 },   "Resonance", 0.0f, 1.0f, 0.75f));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "envmod", 1 }, "Env Mod",   0.0f, 1.0f, 0.6f));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "decay", 1 },  "Decay",     range (0.05f, 3.0f, 0.4f), 0.35f));

    // drive
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "dist", 1 }, "Guitar Dist", 0.0f, 1.0f, 0.0f));

    // sequencer
    p.push_back (std::make_unique<AudioParameterBool> (ParameterID { "seq", 1 }, "Sequencer On", false));

    // fx
    p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { "dtime", 1 }, "Delay Time",
                                                         StringArray { "1/16", "1/8", "1/8 dot", "1/4" }, 2));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "dmix", 1 },  "Delay",       0.0f, 1.0f, 0.0f));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "dfb", 1 },   "Feedback",    0.0f, 0.9f, 0.45f));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "rmix", 1 },  "Reverb",      0.0f, 1.0f, 0.0f));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "rsize", 1 }, "Reverb Size", 0.0f, 1.0f, 0.6f));

    return { p.begin(), p.end() };
}

void AcidLineProcessor::loadPreset (int index)
{
    index = juce::jlimit (0, AcidPatterns::numPatterns - 1, index);
    for (int i = 0; i < AcidPatterns::numSteps; ++i)
        setStep (i, AcidPatterns::patterns[index][i]);
}

void AcidLineProcessor::randomisePattern()
{
    AcidStep tmp[AcidPatterns::numSteps];
    AcidPatterns::randomise (tmp, (unsigned) juce::Random::getSystemRandom().nextInt());
    for (int i = 0; i < AcidPatterns::numSteps; ++i)
        setStep (i, tmp[i]);
}

void AcidLineProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    sampleRateHz = sampleRate;
    voice.prepare (sampleRate);
    guitarDist.prepare (sampleRate);
    delay.prepare (sampleRate);
    reverb.setSampleRate (sampleRate);
    reverb.reset();
    revL.assign ((size_t) juce::jmax (1, samplesPerBlock), 0.0f);
    revR.assign ((size_t) juce::jmax (1, samplesPerBlock), 0.0f);
    revHpC = std::exp (-2.0f * juce::MathConstants<float>::pi * 200.0f / (float) sampleRate);
    revHpX = revHpY = 0.0f;
    sequencer.stop (voice);
    heldNotes.clear();
    internalStepPos = 0.0;
}

bool AcidLineProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo();
}

AcidParams AcidLineProcessor::readParams() const
{
    AcidParams p;
    p.cutoffHz  = apvts.getRawParameterValue ("cutoff")->load();
    p.resonance = apvts.getRawParameterValue ("reso")->load();
    p.envMod    = apvts.getRawParameterValue ("envmod")->load();
    p.decaySec  = apvts.getRawParameterValue ("decay")->load();
    p.waveform  = 0.0f;    // saw
    p.accent    = 0.6f;
    p.glideSec  = 0.06f;
    p.drive     = 0.3f;
    p.volume    = 0.5f;
    return p;
}

void AcidLineProcessor::handleMidi (const juce::MidiMessage& m)
{
    const bool seqOn = apvts.getRawParameterValue ("seq")->load() > 0.5f;
    if (seqOn)
    {
        if (m.isNoteOn()) seqRoot = m.getNoteNumber();   // key = transpose, stays latched
        return;
    }

    if (m.isNoteOn())
    {
        const bool legato = ! heldNotes.empty();
        heldNotes.push_back (m.getNoteNumber());
        voice.noteOn (m.getNoteNumber(), m.getFloatVelocity(), legato);
    }
    else if (m.isNoteOff())
    {
        heldNotes.erase (std::remove (heldNotes.begin(), heldNotes.end(), m.getNoteNumber()), heldNotes.end());
        if (heldNotes.empty()) voice.noteOff();
        else                   voice.slideTo (heldNotes.back());
    }
    else if (m.isAllNotesOff() || m.isAllSoundOff())
    {
        heldNotes.clear();
        voice.noteOff();
    }
}

void AcidLineProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    const auto params = readParams();
    const int numSamples = buffer.getNumSamples();
    const int numCh = buffer.getNumChannels();
    auto* outL = buffer.getWritePointer (0);
    auto* outR = numCh > 1 ? buffer.getWritePointer (1) : nullptr;

    const bool seqOn      = apvts.getRawParameterValue ("seq")->load() > 0.5f;
    const float distAmt   = apvts.getRawParameterValue ("dist")->load();
    const int dTimeIdx    = juce::jlimit (0, 3, juce::roundToInt (apvts.getRawParameterValue ("dtime")->load()));
    const float dMix      = apvts.getRawParameterValue ("dmix")->load();
    const float dFb       = apvts.getRawParameterValue ("dfb")->load();
    const float rMix      = apvts.getRawParameterValue ("rmix")->load();
    const float rSize     = apvts.getRawParameterValue ("rsize")->load();

    // pattern snapshot for this block
    AcidStep pat[AcidPatterns::numSteps];
    for (int i = 0; i < AcidPatterns::numSteps; ++i) pat[i] = getStep (i);

    // ---- transport / tempo ----
    bool hostPlaying = false, haveHost = false;
    double bpm = 140.0, stepPos = 0.0;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
        {
            haveHost = true;
            hostPlaying = pos->getIsPlaying();
            if (auto b = pos->getBpm()) bpm = *b;
            if (auto ppq = pos->getPpqPosition()) stepPos = *ppq * 4.0;
        }

    const bool seqRunning = seqOn && (haveHost ? hostPlaying : true);
    if (! haveHost) stepPos = internalStepPos;
    const double stepInc = bpm / 60.0 * 4.0 / sampleRateHz;
    const float delaySamples = (float) (delayBeats[dTimeIdx] * 60.0 / bpm * sampleRateHz);

    if (seqWasRunning && ! seqRunning) sequencer.stop (voice);
    if (seqOn && ! seqWasRunning) { heldNotes.clear(); voice.noteOff(); }
    seqWasRunning = seqRunning;

    auto renderSample = [&] (int i)
    {
        if (seqRunning) sequencer.tick (stepPos + i * stepInc, pat, seqRoot, voice);
        const float x = guitarDist.process (voice.process (params), distAmt);
        float l, r;
        delay.process (x, delaySamples, dFb, dMix, l, r);
        outL[i] = l;
        if (outR != nullptr) outR[i] = r;
    };

    int pos = 0;
    for (const auto meta : midi)
    {
        const int when = juce::jlimit (0, numSamples, meta.samplePosition);
        for (; pos < when; ++pos) renderSample (pos);
        handleMidi (meta.getMessage());
    }
    for (; pos < numSamples; ++pos) renderSample (pos);

    if (! haveHost) internalStepPos += numSamples * stepInc;
    currentStep.store (seqRunning ? sequencer.currentIndex() : -1);

    // ---- reverb (bass filtered out of the send so the low end stays clean) ----
    if (rMix > 0.001f)
    {
        if ((int) revL.size() < numSamples) { revL.resize ((size_t) numSamples); revR.resize ((size_t) numSamples); }

        juce::Reverb::Parameters rp;
        rp.roomSize = 0.3f + rSize * 0.68f;
        rp.damping = 0.45f;
        rp.wetLevel = 1.0f;
        rp.dryLevel = 0.0f;
        rp.width = 1.0f;
        reverb.setParameters (rp);

        for (int i = 0; i < numSamples; ++i)
        {
            const float m = outR != nullptr ? 0.5f * (outL[i] + outR[i]) : outL[i];
            const float hp = revHpC * (revHpY + m - revHpX);
            revHpX = m; revHpY = hp;
            revL[(size_t) i] = revR[(size_t) i] = hp;
        }
        reverb.processStereo (revL.data(), revR.data(), numSamples);

        const float g = rMix * 0.5f;
        for (int i = 0; i < numSamples; ++i)
        {
            outL[i] += g * revL[(size_t) i];
            if (outR != nullptr) outR[i] += g * revR[(size_t) i];
        }
    }
}

juce::AudioProcessorEditor* AcidLineProcessor::createEditor() { return new AcidLineEditor (*this); }

void AcidLineProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    juce::StringArray s;
    for (int i = 0; i < AcidPatterns::numSteps; ++i) s.add (juce::String (steps[(size_t) i].load()));
    state.setProperty ("steps", s.joinIntoString (","), nullptr);
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void AcidLineProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
        {
            auto tree = juce::ValueTree::fromXml (*xml);
            const auto s = juce::StringArray::fromTokens (tree.getProperty ("steps").toString(), ",", "");
            if (s.size() == AcidPatterns::numSteps)
                for (int i = 0; i < AcidPatterns::numSteps; ++i)
                    steps[(size_t) i].store (s[i].getIntValue());
            apvts.replaceState (tree);
        }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new AcidLineProcessor(); }
