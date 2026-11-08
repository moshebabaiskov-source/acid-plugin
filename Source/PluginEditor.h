#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

class AcidLineEditor : public juce::AudioProcessorEditor
{
public:
    explicit AcidLineEditor (AcidLineProcessor&);
    ~AcidLineEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    struct Knob
    {
        juce::Slider slider;
        juce::Label label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };

    AcidLineProcessor& processor;
    juce::LookAndFeel_V4 lnf;
    std::vector<std::unique_ptr<Knob>> knobs;
    Knob distKnob;
    juce::Label patternLabel;
    juce::ComboBox patternBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> patternAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AcidLineEditor)
};
