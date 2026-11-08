#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

// Custom neon look for knobs, buttons and menus
class AcidLookAndFeel : public juce::LookAndFeel_V4
{
public:
    AcidLookAndFeel();
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos,
                           float startAngle, float endAngle, juce::Slider&) override;
    void drawComboBox (juce::Graphics&, int w, int h, bool down, int, int, int, int, juce::ComboBox&) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool over, bool down) override;
    void drawButtonText (juce::Graphics&, juce::TextButton&, bool over, bool down) override;
};

// 16-step editor: click = on/off, drag up/down = pitch, A row = accent, S row = slide
class StepGrid : public juce::Component
{
public:
    explicit StepGrid (AcidLineProcessor& p) : processor (p) {}

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;

    std::function<void()> onEdit;

private:
    enum class Zone { none, pitch, accent, slide };
    Zone zoneAt (float y) const;
    int columnAt (float x) const;
    juce::Rectangle<float> pitchArea() const { return getLocalBounds().toFloat().withTrimmedBottom (54.0f); }
    void edited() { repaint(); if (onEdit) onEdit(); }

    AcidLineProcessor& processor;
    int dragCol = -1, dragStartSemis = 0;
    float dragStartY = 0.0f;
    bool dragged = false;
    Zone dragZone = Zone::none;
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
    void drawSection (juce::Graphics&, juce::Rectangle<float> r, const juce::String& title);

    struct Knob
    {
        juce::Slider slider;
        juce::Label label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };
    void setupKnob (Knob&, const char* id, const char* name, juce::Colour colour, float fontSize);

    AcidLineProcessor& processor;
    AcidLookAndFeel lnf;
    std::vector<std::unique_ptr<Knob>> filterKnobs, fxKnobs;
    Knob distKnob;

    juce::TextButton seqButton { "SEQ" }, holdButton { "HOLD" }, randomButton { "RANDOM" }, clearButton { "CLEAR" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> seqAttachment, holdAttachment;
    juce::ComboBox presetBox, delayTimeBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> delayTimeAttachment;
    StepGrid grid;

    int shownStep = -2;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AcidLineEditor)
};
