#include "PluginEditor.h"

namespace
{
    const juce::Colour bgTop    { 0xff141418 };
    const juce::Colour bgBottom { 0xff0a0a0c };
    const juce::Colour panel    { 0xff1c1c22 };
    const juce::Colour edge     { 0xff2c2c36 };
    const juce::Colour acid     { 0xffc8ff2e };   // neon acid green
    const juce::Colour hot      { 0xffff5a1f };   // distortion orange
    const juce::Colour textCol  { 0xffd9d9e0 };
    const juce::Colour dimText  { 0xff70707c };

    struct KnobDef { const char* id; const char* name; };
    const KnobDef defs[] = {
        { "cutoff", "CUTOFF" }, { "reso", "RESONANCE" }, { "envmod", "ENV MOD" }, { "decay", "DECAY" }
    };

    constexpr int W = 600, H = 470;
}

// ============================ Look and feel ============================
AcidLookAndFeel::AcidLookAndFeel()
{
    setColour (juce::Slider::textBoxTextColourId, textCol);
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    setColour (juce::ComboBox::textColourId, acid);
    setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xff16161b));
    setColour (juce::PopupMenu::textColourId, textCol);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, acid.withAlpha (0.25f));
    setColour (juce::PopupMenu::highlightedTextColourId, acid);
}

void AcidLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                                        float startAngle, float endAngle, juce::Slider& s)
{
    const auto accent = s.findColour (juce::Slider::rotarySliderFillColourId);
    auto bounds = juce::Rectangle<int> (x, y, w, h).toFloat().reduced (6.0f);
    const float radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) / 2.0f;
    const auto c = bounds.getCentre();
    const float angle = startAngle + pos * (endAngle - startAngle);
    const float trackW = juce::jmax (3.0f, radius * 0.12f);
    const float arcR = radius - trackW;

    // track
    juce::Path track;
    track.addCentredArc (c.x, c.y, arcR, arcR, 0.0f, startAngle, endAngle, true);
    g.setColour (juce::Colour (0xff2a2a33));
    g.strokePath (track, juce::PathStrokeType (trackW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // value arc with glow
    if (pos > 0.001f)
    {
        juce::Path val;
        val.addCentredArc (c.x, c.y, arcR, arcR, 0.0f, startAngle, angle, true);
        for (int i = 3; i >= 1; --i)
        {
            g.setColour (accent.withAlpha (0.07f * (float) i));
            g.strokePath (val, juce::PathStrokeType (trackW + 4.0f * (float) i, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }
        g.setColour (accent);
        g.strokePath (val, juce::PathStrokeType (trackW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    // knob body
    const float bodyR = arcR - trackW * 1.6f;
    auto body = juce::Rectangle<float> (bodyR * 2.0f, bodyR * 2.0f).withCentre (c);
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.fillEllipse (body.translated (0.0f, 3.0f));
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff3a3a46), c.x, body.getY(),
                                             juce::Colour (0xff15151a), c.x, body.getBottom(), false));
    g.fillEllipse (body);
    g.setColour (juce::Colour (0xff4a4a58));
    g.drawEllipse (body.reduced (0.5f), 1.0f);

    // pointer
    juce::Path ptr;
    const float pl = bodyR * 0.75f;
    ptr.addRoundedRectangle (-1.75f, -pl, 3.5f, pl * 0.55f, 1.75f);
    g.setColour (accent);
    g.fillPath (ptr, juce::AffineTransform::rotation (angle).translated (c.x, c.y));
}

void AcidLookAndFeel::drawComboBox (juce::Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox& box)
{
    auto r = juce::Rectangle<float> (0.0f, 0.0f, (float) w, (float) h).reduced (1.0f);
    g.setColour (juce::Colour (0xff101014));
    g.fillRoundedRectangle (r, 6.0f);
    g.setColour (box.isMouseOver() ? acid.withAlpha (0.7f) : edge);
    g.drawRoundedRectangle (r, 6.0f, 1.5f);

    juce::Path arrow;
    const float ax = (float) w - 20.0f, ay = (float) h * 0.5f;
    arrow.addTriangle (ax - 5.0f, ay - 3.0f, ax + 5.0f, ay - 3.0f, ax, ay + 4.0f);
    g.setColour (acid);
    g.fillPath (arrow);
}

juce::Font AcidLookAndFeel::getComboBoxFont (juce::ComboBox&)
{
    return juce::Font (juce::FontOptions (16.0f, juce::Font::bold));
}

// ============================ Editor ============================
AcidLineEditor::AcidLineEditor (AcidLineProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    auto setupKnob = [this] (Knob& k, const char* id, const char* name, juce::Colour colour)
    {
        k.slider.setLookAndFeel (&lnf);
        k.slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        k.slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 80, 18);
        k.slider.setColour (juce::Slider::rotarySliderFillColourId, colour);
        k.label.setText (name, juce::dontSendNotification);
        k.label.setJustificationType (juce::Justification::centred);
        k.label.setColour (juce::Label::textColourId, textCol);
        k.label.setFont (juce::FontOptions (13.0f, juce::Font::bold));
        addAndMakeVisible (k.slider);
        addAndMakeVisible (k.label);
        k.attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, id, k.slider);
    };

    for (auto& d : defs)
    {
        knobs.push_back (std::make_unique<Knob>());
        setupKnob (*knobs.back(), d.id, d.name, acid);
    }
    setupKnob (distKnob, "dist", "GUITAR DIST", hot);

    patternBox.setLookAndFeel (&lnf);
    patternBox.addItemList (processor.apvts.getParameter ("pattern")->getAllValueStrings(), 1);
    addAndMakeVisible (patternBox);
    patternAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (processor.apvts, "pattern", patternBox);

    setSize (W, H);
    startTimerHz (30);
}

AcidLineEditor::~AcidLineEditor()
{
    stopTimer();
    for (auto& k : knobs) k->slider.setLookAndFeel (nullptr);
    distKnob.slider.setLookAndFeel (nullptr);
    patternBox.setLookAndFeel (nullptr);
}

void AcidLineEditor::timerCallback()
{
    const int step = processor.currentStep.load();
    const int pat  = juce::roundToInt (processor.apvts.getRawParameterValue ("pattern")->load());
    if (step != shownStep || pat != shownPattern)
    {
        shownStep = step; shownPattern = pat;
        repaint (stepArea.toNearestInt().expanded (8));
    }
}

void AcidLineEditor::drawSection (juce::Graphics& g, juce::Rectangle<float> r, const juce::String& title)
{
    g.setColour (panel);
    g.fillRoundedRectangle (r, 12.0f);
    g.setColour (edge);
    g.drawRoundedRectangle (r.reduced (0.5f), 12.0f, 1.0f);
    g.setColour (dimText);
    g.setFont (juce::FontOptions (10.5f, juce::Font::bold));
    g.drawText (title, r.reduced (14.0f, 8.0f).removeFromTop (14.0f), juce::Justification::centredLeft);
}

void AcidLineEditor::paint (juce::Graphics& g)
{
    // background + subtle grid
    g.setGradientFill (juce::ColourGradient (bgTop, 0.0f, 0.0f, bgBottom, 0.0f, (float) H, false));
    g.fillAll();
    g.setColour (juce::Colours::white.withAlpha (0.025f));
    for (int x = 0; x < W; x += 20) g.drawVerticalLine (x, 0.0f, (float) H);
    for (int y = 0; y < H; y += 20) g.drawHorizontalLine (y, 0.0f, (float) W);

    // header: glowing title
    const juce::Rectangle<float> titleR (22.0f, 14.0f, 300.0f, 40.0f);
    g.setFont (juce::FontOptions (30.0f, juce::Font::bold));
    for (int i = 3; i >= 1; --i)
    {
        g.setColour (acid.withAlpha (0.06f * (float) i));
        g.drawText ("ACIDLINE", titleR.expanded ((float) i), juce::Justification::centredLeft);
    }
    g.setColour (acid);
    g.drawText ("ACIDLINE", titleR, juce::Justification::centredLeft);

    g.setColour (dimText);
    g.setFont (juce::FontOptions (11.0f));
    g.drawText ("MONO ACID BASS SYNTH", juce::Rectangle<float> (W - 260.0f, 18.0f, 238.0f, 16.0f), juce::Justification::centredRight);
    g.setColour (dimText.withAlpha (0.7f));
    g.drawText ("vel 100+ = accent  |  overlap = slide", juce::Rectangle<float> (W - 260.0f, 34.0f, 238.0f, 16.0f), juce::Justification::centredRight);

    // sections
    drawSection (g, { 12.0f, 64.0f, W - 24.0f, 196.0f }, "FILTER");
    drawSection (g, { 12.0f, 270.0f, 384.0f, 188.0f }, "SEQUENCER");
    drawSection (g, { 404.0f, 270.0f, 184.0f, 188.0f }, "AMP");

    drawSteps (g, stepArea);
}

void AcidLineEditor::drawSteps (juce::Graphics& g, juce::Rectangle<float> area)
{
    const int pat = juce::roundToInt (processor.apvts.getRawParameterValue ("pattern")->load()) - 1;
    const int cur = processor.currentStep.load();
    const float cellW = area.getWidth() / 16.0f;

    for (int i = 0; i < 16; ++i)
    {
        auto cell = juce::Rectangle<float> (area.getX() + i * cellW, area.getY(), cellW, area.getHeight()).reduced (2.5f, 0.0f);
        auto led = cell.withHeight (area.getHeight() - 34.0f);
        const bool beat = i % 4 == 0;

        juce::Colour col = juce::Colour (0xff24242c);
        bool accent = false, slide = false;
        if (pat >= 0)
        {
            const auto& s = AcidPatterns::patterns[pat][i];
            accent = s.on && s.accent;
            slide = s.on && s.slide;
            if (s.on) col = (accent ? hot : acid).withAlpha (0.35f);
        }
        if (i == cur)
        {
            const auto lit = accent ? hot : acid;
            g.setColour (lit.withAlpha (0.25f));
            g.fillRoundedRectangle (led.expanded (4.0f), 6.0f);
            col = lit;
        }

        g.setColour (col);
        g.fillRoundedRectangle (led, 3.0f);
        g.setColour (beat ? juce::Colour (0xff50505e) : edge);
        g.drawRoundedRectangle (led, 3.0f, 1.0f);

        // slide marker: a small line linking to the next step
        if (slide)
        {
            g.setColour (acid.withAlpha (0.8f));
            g.fillRect (led.getRight() - 1.0f, led.getBottom() + 4.0f, cellW * 0.6f + 2.0f, 2.0f);
        }
    }

    // legend
    auto legend = area.withY (area.getY() + area.getHeight() - 12.0f).withHeight (12.0f);
    g.setFont (juce::FontOptions (10.0f));
    g.setColour (acid);   g.fillRoundedRectangle (legend.getX(), legend.getY() + 2.0f, 8.0f, 8.0f, 2.0f);
    g.setColour (dimText); g.drawText ("note", legend.withTrimmedLeft (12.0f).withWidth (40.0f), juce::Justification::centredLeft);
    g.setColour (hot);    g.fillRoundedRectangle (legend.getX() + 50.0f, legend.getY() + 2.0f, 8.0f, 8.0f, 2.0f);
    g.setColour (dimText); g.drawText ("accent", legend.withTrimmedLeft (62.0f).withWidth (50.0f), juce::Justification::centredLeft);
    g.setColour (acid);   g.fillRect (legend.getX() + 115.0f, legend.getY() + 5.0f, 10.0f, 2.0f);
    g.setColour (dimText); g.drawText ("slide", legend.withTrimmedLeft (130.0f).withWidth (40.0f), juce::Justification::centredLeft);
    if (pat >= 0)
        g.drawText ("press a key to transpose", legend, juce::Justification::centredRight);
}

void AcidLineEditor::resized()
{
    // filter knobs
    auto area = juce::Rectangle<int> (24, 86, W - 48, 166);
    const int w = area.getWidth() / (int) knobs.size();
    for (auto& k : knobs)
    {
        auto col = area.removeFromLeft (w);
        k->label.setBounds (col.removeFromTop (20));
        k->slider.setBounds (col.reduced (6, 0));
    }

    // sequencer
    patternBox.setBounds (28, 296, 352, 36);
    stepArea = { 28.0f, 352.0f, 352.0f, 92.0f };

    // amp
    distKnob.label.setBounds (414, 292, 164, 20);
    distKnob.slider.setBounds (430, 314, 132, 136);
}
