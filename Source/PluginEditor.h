#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

// Custom neon look for knobs and the pattern menu
class AcidLookAndFeel : public juce::LookAndFeel_V4
{
public:
    AcidLookAndFeel();
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos,
                           float startAngle, float endAngle, juce::Slider&) override;
    void drawComboBox (juce::Graphics&, int w, int h, bool down, int, int, int, int, juce::ComboBox&) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override;
};

class AcidLineEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit AcidLineEditor (AcidLineProcessor&);
    ~AcidLineEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void drawSteps (juce::Graphics&, juce::Rectangle<float> area);
    void drawSection (juce::Graphics&, juce::Rectangle<float> r, const juce::String& title);

    struct Knob
    {
        juce::Slider slider;
        juce::Label label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };

    AcidLineProcessor& processor;
    AcidLookAndFeel lnf;
    std::vector<std::unique_ptr<Knob>> knobs;
    Knob distKnob;
    juce::ComboBox patternBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> patternAttachment;

    juce::Rectangle<float> stepArea;
    int shownStep = -2, shownPattern = -2;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AcidLineEditor)
};
