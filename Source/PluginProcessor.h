#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "AcidExtras.h"
#include <array>
#include <atomic>
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
    double getTailLengthSeconds() const override { return 3.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // ---- editable pattern (shared safely between UI and audio) ----
    AcidStep getStep (int i) const       { return AcidStep::unpack (steps[(size_t) i].load()); }
    void setStep (int i, AcidStep s)     { steps[(size_t) i].store (s.pack()); }
    void loadPreset (int index);
    void randomisePattern();

    juce::AudioProcessorValueTreeState apvts;
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    std::atomic<int> currentStep { -1 };   // for the step display (-1 = not running)

private:
    void handleMidi (const juce::MidiMessage& m);
    AcidParams readParams() const;

    AcidVoice voice;
    AcidSequencer sequencer;
    GuitarDist guitarDist;
    PingPongDelay delay;
    juce::Reverb reverb;
    std::vector<float> revL, revR;
    float revHpX = 0.0f, revHpY = 0.0f, revHpC = 0.0f;

    std::array<std::atomic<int>, AcidPatterns::numSteps> steps;
    std::vector<int> heldNotes;
    int seqRoot = 36;
    double internalStepPos = 0.0;
    bool seqWasRunning = false;
    double sampleRateHz = 44100.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AcidLineProcessor)
};
