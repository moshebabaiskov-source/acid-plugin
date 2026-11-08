#include "PluginEditor.h"

namespace
{
    const juce::Colour bg     { 0xff1a1a1a };
    const juce::Colour panel  { 0xff262626 };
    const juce::Colour acid   { 0xffd8ff3a };   // acid green-yellow
    const juce::Colour text   { 0xffe8e8e8 };

    struct KnobDef { const char* id; const char* name; };
    const KnobDef defs[] = {
        { "cutoff", "CUTOFF" }, { "reso", "RESONANCE" }, { "envmod", "ENV MOD" }, { "decay", "DECAY" }
    };
}

AcidLineEditor::AcidLineEditor (AcidLineProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    lnf.setColour (juce::Slider::rotarySliderFillColourId, acid);
    lnf.setColour (juce::Slider::rotarySliderOutlineColourId, juce::Colour (0xff3a3a3a));
    lnf.setColour (juce::Slider::thumbColourId, juce::Colours::white);
    lnf.setColour (juce::Slider::textBoxTextColourId, text);
    lnf.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);

    for (auto& d : defs)
    {
        auto k = std::make_unique<Knob>();
        k->slider.setLookAndFeel (&lnf);
        k->slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        k->slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 80, 18);
        k->label.setText (d.name, juce::dontSendNotification);
        k->label.setJustificationType (juce::Justification::centred);
        k->label.setColour (juce::Label::textColourId, text);
        k->label.setFont (juce::FontOptions (15.0f, juce::Font::bold));
        addAndMakeVisible (k->slider);
        addAndMakeVisible (k->label);
        k->attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, d.id, k->slider);
        knobs.push_back (std::move (k));
    }

    // ---- extras row: pattern selector + guitar distortion ----
    auto setupLabel = [] (juce::Label& l, const juce::String& t)
    {
        l.setText (t, juce::dontSendNotification);
        l.setJustificationType (juce::Justification::centred);
        l.setColour (juce::Label::textColourId, text);
        l.setFont (juce::FontOptions (15.0f, juce::Font::bold));
    };

    setupLabel (patternLabel, "PATTERN");
    addAndMakeVisible (patternLabel);
    patternBox.addItemList (processor.apvts.getParameter ("pattern")->getAllValueStrings(), 1);
    patternBox.setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xff1a1a1a));
    patternBox.setColour (juce::ComboBox::textColourId, acid);
    patternBox.setColour (juce::ComboBox::outlineColourId, juce::Colour (0xff3a3a3a));
    patternBox.setColour (juce::ComboBox::arrowColourId, acid);
    addAndMakeVisible (patternBox);
    patternAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (processor.apvts, "pattern", patternBox);

    distKnob.slider.setLookAndFeel (&lnf);
    distKnob.slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    distKnob.slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 80, 18);
    distKnob.slider.setColour (juce::Slider::rotarySliderFillColourId, juce::Colour (0xffff6a2b)); // orange = dirt
    setupLabel (distKnob.label, "GUITAR DIST");
    addAndMakeVisible (distKnob.slider);
    addAndMakeVisible (distKnob.label);
    distKnob.attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, "dist", distKnob.slider);

    setSize (560, 420);
}

AcidLineEditor::~AcidLineEditor()
{
    for (auto& k : knobs) k->slider.setLookAndFeel (nullptr);
    distKnob.slider.setLookAndFeel (nullptr);
}

void AcidLineEditor::paint (juce::Graphics& g)
{
    g.fillAll (bg);
    g.setColour (panel);
    g.fillRoundedRectangle (juce::Rectangle<float> (10.0f, 50.0f, getWidth() - 20.0f, 220.0f), 10.0f);
    g.fillRoundedRectangle (juce::Rectangle<float> (10.0f, 280.0f, getWidth() - 20.0f, 130.0f), 10.0f);

    g.setColour (acid);
    g.setFont (juce::FontOptions (24.0f, juce::Font::bold));
    g.drawText ("ACIDLINE", 20, 8, 300, 30, juce::Justification::centredLeft);
    g.setColour (text.withAlpha (0.5f));
    g.setFont (juce::FontOptions (12.0f));
    g.drawText ("pattern on: press a key to transpose",
                200, 8, getWidth() - 220, 30, juce::Justification::centredRight);
}

void AcidLineEditor::resized()
{
    auto area = juce::Rectangle<int> (20, 60, getWidth() - 40, 200);
    const int w = area.getWidth() / (int) knobs.size();
    for (auto& k : knobs)
    {
        auto col = area.removeFromLeft (w);
        k->label.setBounds (col.removeFromTop (24));
        k->slider.setBounds (col.reduced (4));
    }

    auto extras = juce::Rectangle<int> (20, 290, getWidth() - 40, 110);
    auto left = extras.removeFromLeft (extras.getWidth() / 2).reduced (10, 0);
    patternLabel.setBounds (left.removeFromTop (24));
    patternBox.setBounds (left.withSizeKeepingCentre (left.getWidth(), 32));

    distKnob.label.setBounds (extras.removeFromTop (24));
    distKnob.slider.setBounds (extras.reduced (4));
}
