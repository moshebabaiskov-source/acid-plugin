#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "AcidExtras.h"
#include <vector>

class AcidLineProcessor : public juce::AudioProcessor
{
public:
    AcidLineProcessor();
    ~AcidLineProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "AcidLine"; }
    bool acceptsMidi() const override  { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.05; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

private:
    void handleMidi (const juce::MidiMessage& m);
    AcidParams readParams() const;

    AcidVoice voice;
    AcidSequencer sequencer;
    GuitarDist guitarDist;
    std::vector<int> heldNotes;   // for legato / slide handling
    int seqRoot = 36;             // pattern root note (C1); a played key transposes it
    double internalStepPos = 0.0; // fallback clock when there is no host (standalone)
    bool seqWasRunning = false;
    double sampleRateHz = 44100.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AcidLineProcessor)
};
