#include "PluginProcessor.h"
#include "PluginEditor.h"

AcidLineProcessor::AcidLineProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createLayout())
{
    heldNotes.reserve (32);
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

    // Only the 4 knobs that shape the acid sound the most.
    // Everything else is fixed inside readParams() with tuned values.
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "cutoff", 1 }, "Cutoff",    range (40.0f, 8000.0f, 600.0f), 400.0f));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "reso", 1 },   "Resonance", 0.0f, 1.0f, 0.75f));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "envmod", 1 }, "Env Mod",   0.0f, 1.0f, 0.6f));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "decay", 1 },  "Decay",     range (0.05f, 3.0f, 0.4f), 0.35f));

    // extras
    p.push_back (std::make_unique<AudioParameterFloat>  (ParameterID { "dist", 1 },    "Guitar Dist", 0.0f, 1.0f, 0.0f));
    p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { "pattern", 1 }, "Pattern",
        StringArray { "Off (play MIDI)", "Classic Acid", "Rolling Psy", "Squelchy", "Hypnotic" }, 0));

    return { p.begin(), p.end() };
}

void AcidLineProcessor::prepareToPlay (double sampleRate, int)
{
    sampleRateHz = sampleRate;
    voice.prepare (sampleRate);
    guitarDist.prepare (sampleRate);
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
    // fixed values for the rest
    p.waveform  = 0.0f;    // saw
    p.accent    = 0.6f;
    p.glideSec  = 0.06f;
    p.drive     = 0.3f;
    p.volume    = 0.5f;    // about -6 dB
    return p;
}

void AcidLineProcessor::handleMidi (const juce::MidiMessage& m)
{
    const bool patternOn = apvts.getRawParameterValue ("pattern")->load() > 0.5f;
    if (patternOn)
    {
        if (m.isNoteOn()) seqRoot = m.getNoteNumber();   // key = transpose, stays latched
        return;
    }

    if (m.isNoteOn())
    {
        const bool legato = ! heldNotes.empty();   // overlapping notes = slide
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
    auto* left = buffer.getWritePointer (0);

    const int pattern = juce::roundToInt (apvts.getRawParameterValue ("pattern")->load()) - 1; // -1 = off
    const float distAmount = apvts.getRawParameterValue ("dist")->load();

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

    const bool seqRunning = pattern >= 0 && (haveHost ? hostPlaying : true);
    if (! haveHost) stepPos = internalStepPos;               // standalone: free-running clock
    const double stepInc = bpm / 60.0 * 4.0 / sampleRateHz;  // 16th notes per sample

    if (seqWasRunning && ! seqRunning) sequencer.stop (voice);
    if (pattern >= 0 && ! seqWasRunning) { heldNotes.clear(); voice.noteOff(); }
    seqWasRunning = seqRunning;

    auto renderSample = [&] (int i)
    {
        if (seqRunning) sequencer.tick (stepPos + i * stepInc, pattern, seqRoot, voice);
        left[i] = guitarDist.process (voice.process (params), distAmount);
    };

    int pos = 0;
    for (const auto meta : midi)   // sample-accurate MIDI
    {
        const int when = juce::jlimit (0, numSamples, meta.samplePosition);
        for (; pos < when; ++pos) renderSample (pos);
        handleMidi (meta.getMessage());
    }
    for (; pos < numSamples; ++pos) renderSample (pos);

    if (! haveHost) internalStepPos += numSamples * stepInc;

    for (int ch = 1; ch < buffer.getNumChannels(); ++ch)
        buffer.copyFrom (ch, 0, buffer, 0, 0, numSamples);
}

juce::AudioProcessorEditor* AcidLineProcessor::createEditor() { return new AcidLineEditor (*this); }

void AcidLineProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void AcidLineProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new AcidLineProcessor(); }
