#include "PluginEditor.h"

namespace
{
    const juce::Colour bgTop    { 0xff141418 };
    const juce::Colour bgBottom { 0xff0a0a0c };
    const juce::Colour panel    { 0xff1c1c22 };
    const juce::Colour edge     { 0xff2c2c36 };
    const juce::Colour acid     { 0xffc8ff2e };   // neon acid green
    const juce::Colour hot      { 0xffff5a1f };   // distortion / accent orange
    const juce::Colour fxCol    { 0xffb36bff };   // fx purple
    const juce::Colour textCol  { 0xffd9d9e0 };
    const juce::Colour dimText  { 0xff70707c };

    constexpr int W = 780, H = 600;

    juce::String noteName (int semis)
    {
        static const char* n[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
        const int pc = ((semis % 12) + 12) % 12;
        juce::String s (n[pc]);
        if (semis >= 24)      s << "++";
        else if (semis >= 12) s << "+";
        else if (semis < 0)   s << "-";
        return s;
    }
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

    juce::Path track;
    track.addCentredArc (c.x, c.y, arcR, arcR, 0.0f, startAngle, endAngle, true);
    g.setColour (juce::Colour (0xff2a2a33));
    g.strokePath (track, juce::PathStrokeType (trackW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

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

    const float bodyR = arcR - trackW * 1.6f;
    auto body = juce::Rectangle<float> (bodyR * 2.0f, bodyR * 2.0f).withCentre (c);
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.fillEllipse (body.translated (0.0f, 3.0f));
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff3a3a46), c.x, body.getY(),
                                             juce::Colour (0xff15151a), c.x, body.getBottom(), false));
    g.fillEllipse (body);
    g.setColour (juce::Colour (0xff4a4a58));
    g.drawEllipse (body.reduced (0.5f), 1.0f);

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
    const auto col = box.findColour (juce::ComboBox::textColourId);
    g.setColour (box.isMouseOver() ? col.withAlpha (0.7f) : edge);
    g.drawRoundedRectangle (r, 6.0f, 1.5f);

    juce::Path arrow;
    const float ax = (float) w - 16.0f, ay = (float) h * 0.5f;
    arrow.addTriangle (ax - 5.0f, ay - 3.0f, ax + 5.0f, ay - 3.0f, ax, ay + 4.0f);
    g.setColour (col);
    g.fillPath (arrow);
}

juce::Font AcidLookAndFeel::getComboBoxFont (juce::ComboBox& box)
{
    return juce::Font (juce::FontOptions (box.getHeight() > 30 ? 16.0f : 14.0f, juce::Font::bold));
}

void AcidLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool over, bool down)
{
    auto r = b.getLocalBounds().toFloat().reduced (1.0f);
    const auto col = b.findColour (juce::TextButton::buttonOnColourId);
    if (b.getToggleState())
    {
        g.setColour (col.withAlpha (0.25f));
        g.fillRoundedRectangle (r.expanded (1.0f), 7.0f);
        g.setColour (col);
        g.fillRoundedRectangle (r, 6.0f);
    }
    else
    {
        g.setColour (down ? juce::Colour (0xff26262e) : juce::Colour (0xff101014));
        g.fillRoundedRectangle (r, 6.0f);
        g.setColour (over ? col.withAlpha (0.7f) : edge);
        g.drawRoundedRectangle (r, 6.0f, 1.5f);
    }
}

void AcidLookAndFeel::drawButtonText (juce::Graphics& g, juce::TextButton& b, bool, bool)
{
    const auto col = b.findColour (juce::TextButton::buttonOnColourId);
    g.setColour (b.getToggleState() ? juce::Colour (0xff101014) : col);
    g.setFont (juce::FontOptions (13.0f, juce::Font::bold));
    g.drawText (b.getButtonText(), b.getLocalBounds(), juce::Justification::centred);
}

// ============================ Step grid ============================
int StepGrid::columnAt (float x) const
{
    return juce::jlimit (0, AcidPatterns::numSteps - 1, (int) (x / ((float) getWidth() / AcidPatterns::numSteps)));
}

StepGrid::Zone StepGrid::zoneAt (float y) const
{
    const float ph = pitchArea().getHeight();
    if (y < ph)          return Zone::pitch;
    if (y < ph + 27.0f)  return Zone::accent;
    return Zone::slide;
}

void StepGrid::paint (juce::Graphics& g)
{
    const int cur = processor.currentStep.load();
    const float cellW = (float) getWidth() / AcidPatterns::numSteps;
    const auto pa = pitchArea();
    const float range = (float) (AcidPatterns::maxSemis - AcidPatterns::minSemis);

    for (int i = 0; i < AcidPatterns::numSteps; ++i)
    {
        const auto s = processor.getStep (i);
        auto col = juce::Rectangle<float> (i * cellW, 0.0f, cellW, pa.getHeight()).reduced (2.0f, 0.0f);
        const bool isCur = i == cur;
        const auto lit = s.accent ? hot : acid;

        // column background
        g.setColour (i % 4 == 0 ? juce::Colour (0xff22222a) : juce::Colour (0xff1a1a20));
        g.fillRoundedRectangle (col, 4.0f);

        if (s.on)
        {
            const float frac = (float) (s.semis - AcidPatterns::minSemis + 2) / (range + 2.0f);
            auto bar = col.withTop (col.getBottom() - col.getHeight() * frac);
            if (isCur)
            {
                g.setColour (lit.withAlpha (0.3f));
                g.fillRoundedRectangle (bar.expanded (3.0f), 6.0f);
            }
            g.setColour (isCur ? lit : lit.withAlpha (0.45f));
            g.fillRoundedRectangle (bar, 4.0f);
            g.setColour (isCur ? juce::Colour (0xff101014) : textCol);
            g.setFont (juce::FontOptions (10.0f, juce::Font::bold));
            g.drawText (noteName (s.semis), bar.withHeight (16.0f), juce::Justification::centred);
        }
        else if (isCur)
        {
            g.setColour (dimText.withAlpha (0.4f));
            g.drawRoundedRectangle (col, 4.0f, 1.0f);
        }

        // root line (semis = 0)
        const float zeroY = col.getBottom() - col.getHeight() * (float) (-AcidPatterns::minSemis + 2) / (range + 2.0f);
        g.setColour (juce::Colours::white.withAlpha (0.06f));
        g.drawHorizontalLine ((int) zeroY, col.getX(), col.getRight());

        // accent + slide toggles
        auto aBox = juce::Rectangle<float> (i * cellW, pa.getBottom() + 5.0f, cellW, 22.0f).reduced (3.0f, 0.0f);
        auto sBox = aBox.translated (0.0f, 27.0f);
        g.setFont (juce::FontOptions (11.0f, juce::Font::bold));

        g.setColour (s.accent ? hot : juce::Colour (0xff1a1a20));
        g.fillRoundedRectangle (aBox, 4.0f);
        g.setColour (s.accent ? juce::Colour (0xff101014) : dimText);
        g.drawText ("A", aBox, juce::Justification::centred);

        g.setColour (s.slide ? acid : juce::Colour (0xff1a1a20));
        g.fillRoundedRectangle (sBox, 4.0f);
        g.setColour (s.slide ? juce::Colour (0xff101014) : dimText);
        g.drawText ("S", sBox, juce::Justification::centred);
    }
}

void StepGrid::mouseDown (const juce::MouseEvent& e)
{
    dragCol = columnAt ((float) e.x);
    dragZone = zoneAt ((float) e.y);
    dragStartY = (float) e.y;
    dragStartSemis = processor.getStep (dragCol).semis;
    dragged = false;

    auto s = processor.getStep (dragCol);
    if (dragZone == Zone::accent) { s.accent = ! s.accent; if (s.accent) s.on = true; processor.setStep (dragCol, s); edited(); }
    if (dragZone == Zone::slide)  { s.slide  = ! s.slide;  if (s.slide)  s.on = true; processor.setStep (dragCol, s); edited(); }
}

void StepGrid::mouseDrag (const juce::MouseEvent& e)
{
    if (dragZone != Zone::pitch || dragCol < 0) return;
    const float dy = dragStartY - (float) e.y;
    if (! dragged && std::abs (dy) < 4.0f) return;
    dragged = true;

    auto s = processor.getStep (dragCol);
    s.on = true;
    s.semis = juce::jlimit (AcidPatterns::minSemis, AcidPatterns::maxSemis, dragStartSemis + juce::roundToInt (dy / 5.0f));
    processor.setStep (dragCol, s);
    edited();
}

void StepGrid::mouseUp (const juce::MouseEvent&)
{
    if (dragZone == Zone::pitch && ! dragged && dragCol >= 0)
    {
        auto s = processor.getStep (dragCol);
        s.on = ! s.on;
        processor.setStep (dragCol, s);
        edited();
    }
    dragCol = -1;
    dragZone = Zone::none;
}

void StepGrid::mouseDoubleClick (const juce::MouseEvent& e)
{
    if (zoneAt ((float) e.y) != Zone::pitch) return;
    const int c = columnAt ((float) e.x);
    auto s = processor.getStep (c);
    s.on = true;
    s.semis = 0;
    processor.setStep (c, s);
    edited();
}

void StepGrid::mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& w)
{
    const int c = columnAt ((float) e.x);
    auto s = processor.getStep (c);
    if (! s.on) return;
    s.semis = juce::jlimit (AcidPatterns::minSemis, AcidPatterns::maxSemis, s.semis + (w.deltaY > 0 ? 1 : -1));
    processor.setStep (c, s);
    edited();
}

// ============================ Editor ============================
void AcidLineEditor::setupKnob (Knob& k, const char* id, const char* name, juce::Colour colour, float fontSize)
{
    k.slider.setLookAndFeel (&lnf);
    k.slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    k.slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 80, 16);
    k.slider.setColour (juce::Slider::rotarySliderFillColourId, colour);
    k.label.setText (name, juce::dontSendNotification);
    k.label.setJustificationType (juce::Justification::centred);
    k.label.setColour (juce::Label::textColourId, textCol);
    k.label.setFont (juce::FontOptions (fontSize, juce::Font::bold));
    addAndMakeVisible (k.slider);
    addAndMakeVisible (k.label);
    k.attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, id, k.slider);
}

AcidLineEditor::AcidLineEditor (AcidLineProcessor& p)
    : AudioProcessorEditor (&p), processor (p), grid (p)
{
    const char* filterIds[]   = { "cutoff", "reso", "envmod", "decay" };
    const char* filterNames[] = { "CUTOFF", "RESONANCE", "ENV MOD", "DECAY" };
    for (int i = 0; i < 4; ++i)
    {
        filterKnobs.push_back (std::make_unique<Knob>());
        setupKnob (*filterKnobs.back(), filterIds[i], filterNames[i], acid, 13.0f);
    }
    setupKnob (distKnob, "dist", "GUITAR DIST", hot, 13.0f);

    const char* fxIds[]   = { "dmix", "dfb", "rmix", "rsize" };
    const char* fxNames[] = { "DELAY", "FEEDBACK", "REVERB", "SIZE" };
    for (int i = 0; i < 4; ++i)
    {
        fxKnobs.push_back (std::make_unique<Knob>());
        setupKnob (*fxKnobs.back(), fxIds[i], fxNames[i], fxCol, 11.0f);
    }

    // sequencer controls
    for (auto* b : { &seqButton, &randomButton, &clearButton })
    {
        b->setLookAndFeel (&lnf);
        addAndMakeVisible (*b);
    }
    seqButton.setClickingTogglesState (true);
    seqButton.setColour (juce::TextButton::buttonOnColourId, acid);
    randomButton.setColour (juce::TextButton::buttonOnColourId, acid);
    clearButton.setColour (juce::TextButton::buttonOnColourId, dimText);
    seqAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (processor.apvts, "seq", seqButton);

    presetBox.setLookAndFeel (&lnf);
    for (int i = 0; i < AcidPatterns::numPatterns; ++i)
        presetBox.addItem (AcidPatterns::names[i], i + 1);
    presetBox.setTextWhenNothingSelected ("Load pattern...");
    presetBox.onChange = [this]
    {
        const int id = presetBox.getSelectedId();
        if (id > 0) { processor.loadPreset (id - 1); grid.repaint(); }
    };
    addAndMakeVisible (presetBox);

    randomButton.onClick = [this]
    {
        processor.randomisePattern();
        presetBox.setText ("Random", juce::dontSendNotification);
        grid.repaint();
    };
    clearButton.onClick = [this]
    {
        for (int i = 0; i < AcidPatterns::numSteps; ++i) processor.setStep (i, AcidStep {});
        presetBox.setText ("Empty", juce::dontSendNotification);
        grid.repaint();
    };

    grid.onEdit = [this] { presetBox.setText ("Custom", juce::dontSendNotification); };
    addAndMakeVisible (grid);

    // delay time
    delayTimeBox.setLookAndFeel (&lnf);
    delayTimeBox.setColour (juce::ComboBox::textColourId, fxCol);
    delayTimeBox.addItemList (processor.apvts.getParameter ("dtime")->getAllValueStrings(), 1);
    addAndMakeVisible (delayTimeBox);
    delayTimeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (processor.apvts, "dtime", delayTimeBox);

    setSize (W, H);
    startTimerHz (30);
}

AcidLineEditor::~AcidLineEditor()
{
    stopTimer();
    for (auto* v : { &filterKnobs, &fxKnobs })
        for (auto& k : *v) k->slider.setLookAndFeel (nullptr);
    distKnob.slider.setLookAndFeel (nullptr);
    for (auto* b : { &seqButton, &randomButton, &clearButton }) b->setLookAndFeel (nullptr);
    presetBox.setLookAndFeel (nullptr);
    delayTimeBox.setLookAndFeel (nullptr);
}

void AcidLineEditor::timerCallback()
{
    const int step = processor.currentStep.load();
    if (step != shownStep)
    {
        shownStep = step;
        grid.repaint();
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
    g.setGradientFill (juce::ColourGradient (bgTop, 0.0f, 0.0f, bgBottom, 0.0f, (float) H, false));
    g.fillAll();
    g.setColour (juce::Colours::white.withAlpha (0.025f));
    for (int x = 0; x < W; x += 20) g.drawVerticalLine (x, 0.0f, (float) H);
    for (int y = 0; y < H; y += 20) g.drawHorizontalLine (y, 0.0f, (float) W);

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
    g.drawText ("MONO ACID BASS SYNTH", juce::Rectangle<float> (W - 300.0f, 18.0f, 278.0f, 16.0f), juce::Justification::centredRight);
    g.setColour (dimText.withAlpha (0.7f));
    g.drawText ("SEQ on: press a key to transpose", juce::Rectangle<float> (W - 300.0f, 34.0f, 278.0f, 16.0f), juce::Justification::centredRight);

    drawSection (g, { 12.0f, 64.0f, 520.0f, 190.0f }, "FILTER");
    drawSection (g, { 540.0f, 64.0f, 228.0f, 190.0f }, "DRIVE");
    drawSection (g, { 12.0f, 262.0f, 520.0f, 326.0f }, "SEQUENCER");
    drawSection (g, { 540.0f, 262.0f, 228.0f, 326.0f }, "FX");

    g.setColour (dimText);
    g.setFont (juce::FontOptions (10.0f));
    g.drawText ("click = on/off   |   drag up/down = pitch   |   double-click = root   |   A = accent   S = slide",
                juce::Rectangle<float> (28.0f, 566.0f, 488.0f, 14.0f), juce::Justification::centred);
}

void AcidLineEditor::resized()
{
    // filter
    auto area = juce::Rectangle<int> (24, 84, 496, 162);
    const int w = area.getWidth() / 4;
    for (auto& k : filterKnobs)
    {
        auto col = area.removeFromLeft (w);
        k->label.setBounds (col.removeFromTop (20));
        k->slider.setBounds (col.reduced (4, 0));
    }

    // drive
    distKnob.label.setBounds (552, 84, 204, 20);
    distKnob.slider.setBounds (584, 104, 140, 142);

    // sequencer
    seqButton.setBounds (28, 288, 62, 32);
    presetBox.setBounds (98, 288, 238, 32);
    randomButton.setBounds (344, 288, 92, 32);
    clearButton.setBounds (444, 288, 72, 32);
    grid.setBounds (28, 334, 488, 226);

    // fx
    delayTimeBox.setBounds (556, 288, 196, 28);
    const int cw = 98, ch = 128;
    for (int i = 0; i < 4; ++i)
    {
        auto cell = juce::Rectangle<int> (556 + (i % 2) * cw, 326 + (i / 2) * ch, cw, ch);
        fxKnobs[(size_t) i]->label.setBounds (cell.removeFromTop (18));
        fxKnobs[(size_t) i]->slider.setBounds (cell.reduced (2, 0));
    }
}
