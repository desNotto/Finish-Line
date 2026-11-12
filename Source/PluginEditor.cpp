#include "PluginEditor.h"

using namespace fl;
using namespace ui;

namespace
{
//==============================================================================
template <typename T, typename... Args>
T& make (juce::OwnedArray<juce::Component>& owned, juce::Component& parent, juce::Rectangle<int> bounds, Args&&... args)
{
    auto* c = new T (std::forward<Args> (args)...);
    owned.add (c);
    parent.addAndMakeVisible (c);
    c->setBounds (bounds);
    return *c;
}

//==============================================================================
class Knob : public juce::Component
{
public:
    Knob (FinishLineProcessor& p, int pid, juce::String labelText, juce::Colour accent)
        : label (std::move (labelText)), unit (kDefs[pid].unit)
    {
        slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        slider.setColour (juce::Slider::rotarySliderFillColourId, accent);
        slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.2f, juce::MathConstants<float>::pi * 2.8f, true);
        addAndMakeVisible (slider);
        attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (p.apvts, kDefs[pid].id, slider);
        slider.onValueChange = [this] { repaint(); };
        slider.addMouseListener (this, false);
    }

    void resized() override { slider.setBounds (getLocalBounds().withTrimmedBottom (13)); }

    void paint (juce::Graphics& g) override
    {
        const bool active = slider.isMouseOverOrDragging();
        auto text = label;
        if (active)
        {
            text = slider.getTextFromValue (slider.getValue());
            if (unit.isNotEmpty()) text << " " << unit;
        }

        g.setFont (font (10.5f));
        g.setColour (active ? col::amber : col::dim);
        g.drawText (text, getLocalBounds().removeFromBottom (13), juce::Justification::centred, false);
    }

    void mouseEnter (const juce::MouseEvent&) override { repaint(); }
    void mouseExit (const juce::MouseEvent&) override { repaint(); }

private:
    juce::Slider slider;
    juce::String label, unit;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
};

class Amount : public juce::Slider
{
public:
    Amount (FinishLineProcessor& p, int pid, juce::Colour accent)
    {
        setSliderStyle (juce::Slider::LinearHorizontal);
        setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        setColour (juce::Slider::trackColourId, accent);
        attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (p.apvts, kDefs[pid].id, *this);
    }

private:
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
};

class Choice : public juce::ComboBox
{
public:
    Choice (FinishLineProcessor& p, int pid, juce::Colour accent = col::orange)
    {
        addItemList (juce::StringArray::fromTokens (kDefs[pid].choices, "|", ""), 1);
        setColour (juce::ComboBox::arrowColourId, accent);
        attachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (p.apvts, kDefs[pid].id, *this);
    }

private:
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> attachment;
};

class Toggle : public juce::TextButton
{
public:
    Toggle (FinishLineProcessor& p, int pid, const juce::String& text, juce::Colour accent)
        : juce::TextButton (text)
    {
        setClickingTogglesState (true);
        setColour (juce::TextButton::buttonOnColourId, accent);
        attachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (p.apvts, kDefs[pid].id, *this);
    }

private:
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> attachment;
};

// Momentary audition button: note-on while held, note-off on release.
class TrigButton : public juce::TextButton
{
public:
    TrigButton (FinishLineProcessor& p, int layerMask, const juce::String& text, juce::Colour accent)
        : juce::TextButton (text), proc (p), mask (layerMask)
    {
        setColour (juce::TextButton::buttonOnColourId, accent);
    }

    void mouseDown (const juce::MouseEvent& e) override { juce::TextButton::mouseDown (e); proc.uiTrigger (mask, true); }
    void mouseUp (const juce::MouseEvent& e) override { juce::TextButton::mouseUp (e); proc.uiTrigger (mask, false); }

private:
    FinishLineProcessor& proc;
    int mask;
};

//==============================================================================
void scanlines (juce::Graphics& g, juce::Rectangle<float> r, float alpha = 0.22f)
{
    g.setColour (juce::Colours::black.withAlpha (alpha));
    for (float y = r.getY() + 1.0f; y < r.getBottom(); y += 3.0f)
        g.fillRect (juce::Rectangle<float> (r.getX(), y, r.getWidth(), 1.0f));
}

class Viz : public juce::Component
{
public:
    Viz() { setInterceptsMouseClicks (false, false); }

    std::function<void (juce::Graphics&, juce::Rectangle<float>)> draw;

    void paint (juce::Graphics& g) override
    {
        const auto r = getLocalBounds().toFloat();
        g.setColour (col::screen);
        g.fillRoundedRectangle (r, 3.0f);
        if (draw) draw (g, r.reduced (5.0f, 4.0f));
        scanlines (g, r);
        g.setColour (col::line);
        g.drawRoundedRectangle (r.reduced (0.5f), 3.0f, 1.0f);
    }
};

class Root : public juce::Component
{
public:
    std::function<void (juce::Graphics&)> draw;
    void paint (juce::Graphics& g) override { if (draw) draw (g); }
};

class Panel : public juce::Component
{
public:
    Panel (juce::String t, juce::String j, juce::Colour a, juce::String k = {})
        : title (std::move (t)), jpTitle (std::move (j)), kanji (std::move (k)), accent (a) {}

    void caption (const char* text, int x, int y) { caps.push_back ({ text, x, y }); }

    void paint (juce::Graphics& g) override
    {
        const auto r = getLocalBounds().toFloat();
        g.setColour (col::panel);
        g.fillRoundedRectangle (r, 5.0f);

        if (kanji.isNotEmpty())
        {
            g.setColour (accent.withAlpha (0.13f));
            g.setFont (jp (54.0f));
            g.drawText (kanji, getLocalBounds().removeFromBottom (66).removeFromRight (72), juce::Justification::centred, false);
        }

        g.setColour (col::panelHi);
        g.fillRoundedRectangle (r.withHeight (28.0f), 5.0f);
        g.fillRect (r.withHeight (28.0f).withTrimmedTop (14.0f));
        g.setColour (accent);
        g.fillRect (0.0f, 27.0f, r.getWidth(), 1.5f);
        g.fillRect (8.0f, 8.0f, 4.0f, 13.0f);

        const auto tf = font (15.5f);
        g.setFont (tf);
        g.setColour (col::cream);
        g.drawText (title, 18, 0, 200, 28, juce::Justification::centredLeft, false);
        g.setFont (jp (14.0f));
        g.setColour (accent);
        g.drawText (jpTitle, 18 + (int) textWidth (tf, title) + 8, 0, 140, 28, juce::Justification::centredLeft, false);

        g.setFont (font (9.5f));
        g.setColour (col::dim);
        for (const auto& c : caps)
            g.drawText (c.text, c.x, c.y, 120, 10, juce::Justification::centredLeft, false);

        g.setColour (col::line);
        g.drawRoundedRectangle (r.reduced (0.5f), 5.0f, 1.0f);
    }

private:
    struct Cap { juce::String text; int x, y; };
    juce::String title, jpTitle, kanji;
    std::vector<Cap> caps;

public:
    juce::Colour accent;
};

//==============================================================================
// Central CRT: oscilloscope / spectrum analyser (click to switch).
class Display : public juce::Component
{
public:
    explicit Display (FinishLineProcessor& p)
        : proc (p), fft (11), window (2048, juce::dsp::WindowingFunction<float>::hann)
    {
        setMouseCursor (juce::MouseCursor::PointingHandCursor);
    }

    void mouseDown (const juce::MouseEvent&) override
    {
        spectrum = ! spectrum;
        repaint();
    }

    void update()
    {
        const int w = proc.scopeWrite.load (std::memory_order_relaxed);
        for (int i = 0; i < 2048; ++i)
            wave[(size_t) i] = proc.scope[(size_t) ((w - 2048 + i) & (FinishLineProcessor::kScopeSize - 1))];

        if (spectrum)
        {
            std::fill (fftData.begin(), fftData.end(), 0.0f);
            std::copy (wave.begin(), wave.end(), fftData.begin());
            window.multiplyWithWindowingTable (fftData.data(), 2048);
            fft.performFrequencyOnlyForwardTransform (fftData.data());

            const double sr = proc.getSampleRate() > 0.0 ? proc.getSampleRate() : 48000.0;
            for (int b = 0; b < kBars; ++b)
            {
                const double f0 = 20.0 * std::pow (1000.0, (double) b / kBars);
                const double f1 = 20.0 * std::pow (1000.0, (double) (b + 1) / kBars);
                const int i0 = juce::jlimit (1, 1023, (int) (f0 / sr * 2048.0));
                const int i1 = juce::jlimit (i0 + 1, 1024, (int) (f1 / sr * 2048.0) + 1);
                float mag = 0.0f;
                for (int i = i0; i < i1; ++i) mag = juce::jmax (mag, fftData[(size_t) i]);
                const float db = juce::Decibels::gainToDecibels (mag / 512.0f, -90.0f);
                const float v = juce::jlimit (0.0f, 1.0f, juce::jmap (db, -78.0f, 0.0f, 0.0f, 1.0f));
                bars[(size_t) b] = juce::jmax (v, bars[(size_t) b] * 0.86f);
            }
        }

        repaint();
    }

    void paint (juce::Graphics& g) override
    {
        const auto r = getLocalBounds().toFloat();
        g.setColour (col::screen);
        g.fillRoundedRectangle (r, 6.0f);

        const auto area = r.reduced (12.0f, 14.0f).withTrimmedTop (14.0f);

        // grid
        g.setColour (col::amber.withAlpha (0.10f));
        for (int i = 0; i <= 12; ++i)
            g.fillRect (juce::Rectangle<float> (area.getX() + area.getWidth() * (float) i / 12.0f, area.getY(), 1.0f, area.getHeight()));
        for (int i = 0; i <= 4; ++i)
            g.fillRect (juce::Rectangle<float> (area.getX(), area.getY() + area.getHeight() * (float) i / 4.0f, area.getWidth(), 1.0f));

        if (spectrum)
        {
            const float bw = area.getWidth() / (float) kBars;
            for (int b = 0; b < kBars; ++b)
            {
                const float h = bars[(size_t) b] * area.getHeight();
                const juce::Rectangle<float> bar (area.getX() + bw * (float) b + 1.0f, area.getBottom() - h, bw - 2.0f, h);
                g.setColour (col::amber.interpolatedWith (col::orange, bars[(size_t) b]));
                g.fillRect (bar);
            }
        }
        else
        {
            int start = 0;
            for (int i = 1; i < 1000; ++i)
                if (wave[(size_t) (i - 1)] <= 0.0f && wave[(size_t) i] > 0.0f) { start = i; break; }

            juce::Path path;
            const int count = 1024;
            for (int i = 0; i < count; ++i)
            {
                const float x = area.getX() + area.getWidth() * (float) i / (float) (count - 1);
                const float y = area.getCentreY() - juce::jlimit (-1.0f, 1.0f, wave[(size_t) (start + i)]) * area.getHeight() * 0.5f;
                if (i == 0) path.startNewSubPath (x, y); else path.lineTo (x, y);
            }
            g.setColour (col::amber.withAlpha (0.22f));
            g.strokePath (path, juce::PathStrokeType (5.0f));
            g.setColour (col::amber);
            g.strokePath (path, juce::PathStrokeType (1.6f));
        }

        // readouts
        g.setFont (mono (12.0f));
        g.setColour (col::amber);
        g.drawText (spectrum ? "SPECTRUM" : "OSCILLOSCOPE", 14, 6, 120, 16, juce::Justification::centredLeft, false);
        g.setFont (jp (13.5f));
        g.setColour (col::orange);
        g.drawText (spectrum ? U ("スペクトル") : U ("波形"), 122, 5, 90, 18, juce::Justification::centredLeft, false);

        static const char* keys[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
        const bool playing = proc.hostPlaying.load();
        juce::String status;
        status << "BPM " << juce::String (proc.hostBpm.load(), 1)
               << "   KEY " << keys[juce::jlimit (0, 11, (int) proc.getParam (P::key))]
               << "   " << (playing ? "SYNC" : "FREE");
        g.setFont (mono (12.0f));
        g.setColour (playing ? col::teal : col::amber);
        g.drawText (status, getWidth() - 274, 6, 260, 16, juce::Justification::centredRight, false);

        scanlines (g, r, 0.2f);
        g.setColour (col::line);
        g.drawRoundedRectangle (r.reduced (0.5f), 6.0f, 1.0f);
    }

private:
    static constexpr int kBars = 72;
    FinishLineProcessor& proc;
    juce::dsp::FFT fft;
    juce::dsp::WindowingFunction<float> window;
    std::array<float, 2048> wave {};
    std::array<float, 4096> fftData {};
    std::array<float, kBars> bars {};
    bool spectrum = false;
};

//==============================================================================
// Drag this tile into the DAW: the current sound is bounced to a 24-bit WAV.
class DragTile : public juce::Component
{
public:
    explicit DragTile (FinishLineProcessor& p) : proc (p)
    {
        setMouseCursor (juce::MouseCursor::DraggingHandCursor);
    }

    void paint (juce::Graphics& g) override
    {
        const auto r = getLocalBounds().toFloat().reduced (0.5f);
        g.setColour (isMouseOverOrDragging() ? col::panelHi : col::ink);
        g.fillRoundedRectangle (r, 4.0f);

        juce::Path outline;
        outline.addRoundedRectangle (r, 4.0f);
        const float dashes[] = { 5.0f, 3.0f };
        juce::Path dashed;
        juce::PathStrokeType (1.2f).createDashedStroke (dashed, outline, dashes, 2);
        g.setColour (col::amber);
        g.fillPath (dashed);

        g.setFont (font (13.0f));
        g.drawText ("DRAG WAV", getLocalBounds().withTrimmedBottom (11), juce::Justification::centred, false);
        g.setFont (font (9.0f));
        g.setColour (col::dim);
        g.drawText ("24-BIT  >>  DAW TRACK", getLocalBounds().withTrimmedTop (15), juce::Justification::centred, false);
    }

    void mouseEnter (const juce::MouseEvent&) override { repaint(); }
    void mouseExit (const juce::MouseEvent&) override { repaint(); }
    void mouseUp (const juce::MouseEvent&) override { dragging = false; }

    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (dragging || e.getDistanceFromDragStart() < 6)
            return;

        dragging = true;
        const auto file = proc.renderToWav();
        if (file.existsAsFile())
            juce::DragAndDropContainer::performExternalDragDropOfFiles ({ file.getFullPathName() }, false, this,
                                                                         [this] { dragging = false; });
        else
            dragging = false;
    }

private:
    FinishLineProcessor& proc;
    bool dragging = false;
};

//==============================================================================
// Trigger keyboard: every key is tinted with the colour of the block it fires.
struct KeyRole { juce::Colour colour; const char* label; bool active; };

KeyRole keyRole (int note)
{
    switch (note)
    {
        case 36: return { col::cream, "ALL", true };
        case 38: return { col::orange, "SUB", true };
        case 40: return { col::amber, "IMP", true };
        case 41: return { col::teal, "SYN", true };
        case 43: return { col::pink, "RIS", true };
        case 45: return { col::red, "STOP", true };
        default: break;
    }

    if (note >= 48)
        return { juce::MidiMessage::isMidiNoteBlack (note) ? juce::Colour (0xff1b1a1e) : juce::Colour (0xffcbc4b1),
                 note == 60 ? "ROOT" : "", true };

    return { juce::MidiMessage::isMidiNoteBlack (note) ? juce::Colour (0xff232227) : juce::Colour (0xff3a3940), "", false };
}

class TriggerKeyboard : public juce::MidiKeyboardComponent
{
public:
    explicit TriggerKeyboard (juce::MidiKeyboardState& state)
        : juce::MidiKeyboardComponent (state, juce::MidiKeyboardComponent::horizontalKeyboard)
    {
        setAvailableRange (36, 96);
        setLowestVisibleKey (36);
        setKeyWidth ((float) (FinishLineEditor::kW - 24) / 36.0f);
        setScrollButtonsVisible (false);
        setBlackNoteLengthProportion (0.6f);
        setVelocity (1.0f, false);
        setMidiChannel (1);
        setWantsKeyboardFocus (false);
        setColour (juce::MidiKeyboardComponent::shadowColourId, juce::Colours::transparentBlack);
    }

    void drawWhiteNote (int note, juce::Graphics& g, juce::Rectangle<float> area, bool isDown, bool isOver,
                        juce::Colour, juce::Colour) override
    {
        const auto role = keyRole (note);
        auto c = role.colour;
        if (isDown) c = role.active ? c.interpolatedWith (juce::Colours::white, 0.55f) : c.brighter (0.2f);
        else if (isOver && role.active) c = c.brighter (0.25f);

        const auto key = area.reduced (0.75f, 0.0f).withTrimmedBottom (isDown ? 0.0f : 2.0f);
        g.setColour (c);
        g.fillRoundedRectangle (key, 2.5f);
        g.fillRect (key.withHeight (4.0f));

        if (! isDown)
        {
            g.setColour (juce::Colours::black.withAlpha (0.28f));
            g.fillRect (key.withTop (key.getBottom() - 5.0f).reduced (1.0f, 0.0f));
        }

        juce::String text (role.label);
        if (note >= 48 && note % 12 == 0 && note != 60)
            text = "C" + juce::String (note / 12 - 2);

        if (text.isNotEmpty())
        {
            g.setFont (font (10.0f));
            g.setColour (col::ink.withAlpha (role.label[0] != 0 ? 1.0f : 0.6f));
            g.drawText (text, key.withTrimmedBottom (8.0f).toNearestInt(), juce::Justification::centredBottom, false);
        }
    }

    void drawBlackNote (int note, juce::Graphics& g, juce::Rectangle<float> area, bool isDown, bool isOver, juce::Colour) override
    {
        const auto role = keyRole (note);
        auto c = role.colour;
        if (isDown) c = role.active ? col::amber : c.brighter (0.15f);
        else if (isOver && role.active) c = c.brighter (0.35f);

        g.setColour (col::bg);
        g.fillRect (area.expanded (1.0f, 0.0f).withTrimmedBottom (-1.0f));
        g.setColour (c);
        g.fillRoundedRectangle (area.withTrimmedBottom (isDown ? 0.0f : 2.0f), 2.0f);
        g.fillRect (area.withHeight (4.0f));

        if (role.active && ! isDown)
        {
            g.setColour (juce::Colours::white.withAlpha (0.08f));
            g.fillRect (area.reduced (2.5f, 0.0f).withTrimmedBottom (7.0f));
        }
    }
};

void strokeGlow (juce::Graphics& g, const juce::Path& p, juce::Colour c, float width = 1.6f)
{
    g.setColour (c.withAlpha (0.2f));
    g.strokePath (p, juce::PathStrokeType (width + 3.5f));
    g.setColour (c);
    g.strokePath (p, juce::PathStrokeType (width));
}

void vizLabel (juce::Graphics& g, juce::Rectangle<float> r, const juce::String& text, juce::Colour c,
               juce::Justification j = juce::Justification::topRight)
{
    g.setFont (mono (10.0f));
    g.setColour (c.withAlpha (0.85f));
    g.drawText (text, r.toNearestInt(), j, false);
}

} // namespace

//==============================================================================
FinishLineEditor::FinishLineEditor (FinishLineProcessor& p)
    : AudioProcessorEditor (&p), proc (p)
{
    auto root = std::make_unique<Root>();
    root->draw = [] (juce::Graphics& g)
    {
        g.fillAll (col::bg);

        // top bar plate
        const juce::Rectangle<float> bar (8.0f, 8.0f, (float) kW - 16.0f, 56.0f);
        g.setColour (col::panel);
        g.fillRoundedRectangle (bar, 5.0f);
        g.setColour (col::line);
        g.drawRoundedRectangle (bar.reduced (0.5f), 5.0f, 1.0f);

        // rising-sun mark
        g.setColour (col::red);
        g.fillEllipse (18.0f, 15.0f, 42.0f, 42.0f);
        g.setColour (col::panel);
        for (int i = 0; i < 3; ++i)
            g.fillRect (juce::Rectangle<float> (18.0f, 41.0f + (float) i * 5.0f, 42.0f, 2.0f));

        g.setColour (col::cream);
        g.setFont (font (31.0f));
        g.drawText ("FINISH LINE", 70, 9, 220, 34, juce::Justification::centredLeft, false);

        // chequered finish-line strip
        for (int row = 0; row < 2; ++row)
            for (int i = 0; i < 26; ++i)
            {
                g.setColour (((i + row) & 1) ? col::cream : col::ink);
                g.fillRect (72 + i * 5, 44 + row * 5, 5, 5);
            }

        g.setFont (jp (14.0f));
        g.setColour (col::orange);
        g.drawText (U ("フィニッシュ・ライン"), 208, 39, 140, 20, juce::Justification::centredLeft, false);

        g.setFont (mono (11.0f));
        g.setColour (col::dim);
        g.drawText ("by desNotto", 248, 11, 96, 13, juce::Justification::centredLeft, false);

        // keyboard plate + legend
        {
            const juce::Rectangle<float> plate (8.0f, 780.0f, (float) kW - 16.0f, (float) kH - 788.0f);
            g.setColour (col::panel);
            g.fillRoundedRectangle (plate, 5.0f);

            g.setFont (font (13.0f));
            g.setColour (col::cream);
            g.drawText ("TRIGGER KEYS", 18, 783, 100, 20, juce::Justification::centredLeft, false);
            g.setFont (jp (12.5f));
            g.setColour (col::orange);
            g.drawText (U ("鍵盤"), 110, 783, 40, 20, juce::Justification::centredLeft, false);

            struct Item { juce::Colour c; const char* text; int width; };
            const Item items[] = {
                { col::cream, "C1  ALL LAYERS", 100 }, { col::orange, "D1  SUB", 58 }, { col::amber, "E1  IMPACT", 76 },
                { col::teal, "F1  SYNTH (HOLD)", 112 }, { col::pink, "G1  RISER", 70 }, { col::red, "A1  TAPE STOP (HOLD)", 136 },
                { juce::Colour (0xffcbc4b1), "C2-C6  ALL LAYERS, PITCHED  (C3 = ROOT)", 250 }, { juce::Colour (0xff3a3940), "UNUSED", 60 }
            };

            int x = 156;
            g.setFont (font (10.5f));
            for (const auto& it : items)
            {
                g.setColour (it.c);
                g.fillRoundedRectangle ((float) x, 788.0f, 10.0f, 10.0f, 2.0f);
                g.setColour (col::dim.brighter (0.4f));
                g.drawText (it.text, x + 15, 783, it.width, 20, juce::Justification::centredLeft, false);
                x += it.width + 28;
            }

            g.setColour (col::line);
            g.drawRoundedRectangle (plate.reduced (0.5f), 5.0f, 1.0f);
        }
        g.setFont (jp (11.5f));
        g.setColour (col::dim);
        g.drawText (U ("FL-01 音響兵器"), 248, 24, 96, 14, juce::Justification::centredLeft, false);
    };
    content = std::move (root);
    content->setLookAndFeel (&look);
    addAndMakeVisible (*content);

    buildTopBar();
    buildMacros();
    buildDisplay();
    buildMaster();
    buildLayers();
    buildFx();
    buildKeyboard();

    setResizable (true, true);
    setResizeLimits (kW * 3 / 4, kH * 3 / 4, kW * 2, kH * 2);
    getConstrainer()->setFixedAspectRatio ((double) kW / (double) kH);
    const float s = juce::jlimit (0.75f, 2.0f, proc.uiScale);
    setSize (juce::roundToInt ((float) kW * s), juce::roundToInt ((float) kH * s));

    startTimerHz (30);
}

FinishLineEditor::~FinishLineEditor()
{
    stopTimer();
    owned.clear();
    content->setLookAndFeel (nullptr);
}

void FinishLineEditor::paint (juce::Graphics& g)
{
    g.fillAll (col::bg);
}

void FinishLineEditor::resized()
{
    const float s = (float) getWidth() / (float) kW;
    content->setTransform (juce::AffineTransform::scale (s));
    content->setBounds (0, 0, kW, kH);
    proc.uiScale = s;
}

//==============================================================================
void FinishLineEditor::buildTopBar()
{
    auto& c = *content;
    const int x = 352, y = 22, h = 28;

    auto& prev = make<juce::TextButton> (owned, c, { x, y, 28, h }, "<");
    prev.onClick = [this] { stepPreset (-1); };

    presetBox = &make<juce::ComboBox> (owned, c, { x + 32, y, 276, h });
    presetBox->setTextWhenNothingSelected ("INIT");
    presetBox->setColour (juce::ComboBox::arrowColourId, col::amber);
    refreshPresetBox();
    presetBox->onChange = [this]
    {
        const int id = presetBox->getSelectedId();
        if (id > 0)
        {
            proc.applyPreset (id - 1);
            shownPreset = proc.presetName;
        }
    };

    auto& next = make<juce::TextButton> (owned, c, { x + 312, y, 28, h }, ">");
    next.onClick = [this] { stepPreset (1); };

    auto& save = make<juce::TextButton> (owned, c, { x + 348, y, 54, h }, "SAVE");
    save.onClick = [this]
    {
        chooser = std::make_unique<juce::FileChooser> ("Save preset",
                                                       proc.getUserPresetDirectory().getChildFile (proc.presetName + ".flpreset"),
                                                       "*.flpreset");
        chooser->launchAsync (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles
                                  | juce::FileBrowserComponent::warnAboutOverwriting,
                              [this] (const juce::FileChooser& fc)
                              {
                                  const auto f = fc.getResult();
                                  if (f != juce::File())
                                      proc.saveUserPreset (f.withFileExtension ("flpreset"));
                              });
    };

    auto& load = make<juce::TextButton> (owned, c, { x + 406, y, 54, h }, "LOAD");
    load.onClick = [this]
    {
        chooser = std::make_unique<juce::FileChooser> ("Load preset", proc.getUserPresetDirectory(), "*.flpreset");
        chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                              [this] (const juce::FileChooser& fc)
                              {
                                  const auto f = fc.getResult();
                                  if (f.existsAsFile())
                                      proc.loadUserPreset (f);
                              });
    };

    auto& rnd = make<juce::TextButton> (owned, c, { x + 472, y, 124, h }, "RANDOMIZE");
    rnd.setColour (juce::TextButton::buttonOnColourId, col::amber);
    rnd.setColour (juce::TextButton::textColourOffId, col::amber);
    rnd.onClick = [this] { proc.randomize(); };

    auto& fire = make<TrigButton> (owned, c, { x + 604, y, 100, h }, proc, (int) kAllLayers, "FIRE ALL", col::red);
    fire.setColour (juce::TextButton::textColourOffId, col::orange);

    make<DragTile> (owned, c, { x + 712, 16, kW - 16 - (x + 712), 40 }, proc);
}

void FinishLineEditor::refreshPresetBox()
{
    presetBox->clear (juce::dontSendNotification);
    const auto& presets = factoryPresets();
    juce::String lastCategory;

    for (int i = 0; i < (int) presets.size(); ++i)
    {
        if (lastCategory != presets[(size_t) i].category)
        {
            lastCategory = presets[(size_t) i].category;
            presetBox->addSectionHeading (lastCategory);
        }
        presetBox->addItem (presets[(size_t) i].name, i + 1);
    }

    shownPreset = proc.presetName;
    if (proc.presetIndex >= 0)
        presetBox->setSelectedId (proc.presetIndex + 1, juce::dontSendNotification);
    else
        presetBox->setText (proc.presetName, juce::dontSendNotification);
}

void FinishLineEditor::stepPreset (int delta)
{
    const int count = (int) factoryPresets().size();
    const int current = proc.presetIndex < 0 ? (delta > 0 ? -1 : 0) : proc.presetIndex;
    presetBox->setSelectedId (((current + delta + count) % count) + 1, juce::sendNotificationSync);
}

//==============================================================================
void FinishLineEditor::buildMacros()
{
    auto& pn = make<Panel> (owned, *content, { 8, 72, 300, 218 }, "MACRO", U ("マクロ"), col::cream);

    struct M { int knob, firstSlot; const char* name; juce::Colour c; };
    const M macros[2] = { { P::macro1, P::m1_t1, "AGGRESSION", col::orange }, { P::macro2, P::m2_t1, "TENSION", col::teal } };

    for (int m = 0; m < 2; ++m)
    {
        const int y = 34 + m * 92;
        make<Knob> (owned, pn, { 4, y, 82, 84 }, proc, macros[m].knob, macros[m].name, macros[m].c);

        for (int s = 0; s < 4; ++s)
        {
            const int sx = 92 + (s % 2) * 102, sy = y + 4 + (s / 2) * 42;
            make<Choice> (owned, pn, { sx, sy, 98, 20 }, proc, macros[m].firstSlot + s * 2, macros[m].c);
            make<Amount> (owned, pn, { sx, sy + 22, 98, 12 }, proc, macros[m].firstSlot + s * 2 + 1, macros[m].c);
        }
    }
}

void FinishLineEditor::buildDisplay()
{
    auto& d = make<Display> (owned, *content, { 316, 72, 608, 218 }, proc);
    display = &d;
}

void FinishLineEditor::buildMaster()
{
    auto& pn = make<Panel> (owned, *content, { 932, 72, 300, 218 }, "MASTER", U ("マスター"), col::cream);

    // cassette window with spinning reels
    auto& reels = make<Viz> (owned, pn, { 8, 34, 150, 84 });
    reels.draw = [this] (juce::Graphics& g, juce::Rectangle<float> r)
    {
        const float cy = r.getCentreY() + 4.0f, rad = 24.0f;
        const float cx[2] = { r.getX() + r.getWidth() * 0.27f, r.getX() + r.getWidth() * 0.73f };
        const float tape[2] = { rad + 9.0f, rad + 2.0f };

        g.setFont (jp (10.5f));
        g.setColour (col::amber.withAlpha (0.8f));
        g.drawText (U ("SIDE A  TYPE II  録音"), r.withHeight (12.0f).toNearestInt(), juce::Justification::centred, false);

        for (int i = 0; i < 2; ++i)
        {
            g.setColour (juce::Colour (0xff3a2415));
            g.fillEllipse (cx[i] - tape[i], cy - tape[i], tape[i] * 2.0f, tape[i] * 2.0f);
            g.setColour (col::cream);
            g.fillEllipse (cx[i] - 13.0f, cy - 13.0f, 26.0f, 26.0f);
            g.setColour (col::screen);
            g.fillEllipse (cx[i] - 8.0f, cy - 8.0f, 16.0f, 16.0f);

            g.setColour (col::cream);
            for (int k = 0; k < 6; ++k)
            {
                const float a = reelAngle * (i == 0 ? 1.0f : 1.25f) + (float) k * juce::MathConstants<float>::pi / 3.0f;
                g.drawLine (cx[i] + std::cos (a) * 5.0f, cy + std::sin (a) * 5.0f,
                            cx[i] + std::cos (a) * 8.5f, cy + std::sin (a) * 8.5f, 2.0f);
            }
        }

        g.setColour (juce::Colour (0xff3a2415));
        g.drawLine (cx[0], cy + tape[0], cx[1], cy + tape[1], 1.5f);
    };
    animated.add (&reels);

    make<Knob> (owned, pn, { 166, 36, 62, 78 }, proc, P::master_gain, "OUTPUT", col::cream);
    make<Knob> (owned, pn, { 230, 36, 62, 78 }, proc, P::lim_ceiling, "CEILING", col::red);

    auto& meters = make<Viz> (owned, pn, { 8, 124, 284, 38 });
    meters.draw = [this] (juce::Graphics& g, juce::Rectangle<float> r)
    {
        const float lv[2] = { proc.meterL.load(), proc.meterR.load() };
        const float gr = proc.gainReduction.load();
        const char* names[3] = { "L", "R", "GR" };
        const float rowH = r.getHeight() / 3.0f;

        for (int i = 0; i < 3; ++i)
        {
            auto row = juce::Rectangle<float> (r.getX(), r.getY() + rowH * (float) i, r.getWidth(), rowH).reduced (0.0f, 1.5f);
            g.setFont (mono (9.0f));
            g.setColour (col::amber);
            g.drawText (names[i], row.removeFromLeft (18.0f).toNearestInt(), juce::Justification::centredLeft, false);
            g.setColour (col::amber.withAlpha (0.12f));
            g.fillRect (row);

            if (i < 2)
            {
                const float db = juce::Decibels::gainToDecibels (lv[i], -60.0f);
                const float v = juce::jlimit (0.0f, 1.0f, juce::jmap (db, -48.0f, 0.0f, 0.0f, 1.0f));
                g.setColour (db > -3.0f ? col::red : col::amber);
                g.fillRect (row.withWidth (row.getWidth() * v));
            }
            else
            {
                const float db = -juce::Decibels::gainToDecibels (gr, -60.0f);
                const float v = juce::jlimit (0.0f, 1.0f, db / 18.0f);
                g.setColour (col::orange);
                g.fillRect (row.withLeft (row.getRight() - row.getWidth() * v));
            }
        }
    };
    animated.add (&meters);

    pn.caption ("KEY", 8, 166);
    pn.caption ("OUTPUT STAGE", 158, 166);
    make<Choice> (owned, pn, { 8, 180, 62, 24 }, proc, P::key, col::amber);
    make<Toggle> (owned, pn, { 76, 180, 76, 24 }, proc, P::key_lock, "KEY LOCK", col::amber);
    make<Choice> (owned, pn, { 158, 180, 134, 24 }, proc, P::lim_mode, col::red);
}

//==============================================================================
void FinishLineEditor::buildLayers()
{
    const int py = 298, pw = 300, ph = 316;
    const int rowStrip = 250, row3 = 192, row2 = 134, row1 = 76;
    auto cell = [] (int i) { return 8 + i * 71; };

    auto header = [this] (Panel& pn, int onId, int lockId, int mask)
    {
        make<TrigButton> (owned, pn, { 170, 5, 40, 18 }, proc, mask, "TRIG", pn.accent);
        make<Toggle> (owned, pn, { 214, 5, 40, 18 }, proc, lockId, "LOCK", col::cream);
        make<Toggle> (owned, pn, { 258, 5, 34, 18 }, proc, onId, "ON", pn.accent);
    };

    auto strip = [this, rowStrip, cell] (Panel& pn, int levelId, int revId, int dlyId)
    {
        make<Knob> (owned, pn, { cell (0), rowStrip, 71, 58 }, proc, levelId, "LEVEL", col::cream);
        make<Knob> (owned, pn, { cell (1), rowStrip, 71, 58 }, proc, revId, "REVERB", col::cream);
        make<Knob> (owned, pn, { cell (2), rowStrip, 71, 58 }, proc, dlyId, "DELAY", col::cream);
    };

    // ------------------------------------------------------------------ SUB
    {
        auto& pn = make<Panel> (owned, *content, { 8, py, pw, ph }, "SUB", U ("サブ"), col::orange, U ("壱"));
        header (pn, P::sub_on, P::sub_lock, kSub);

        auto& viz = make<Viz> (owned, pn, { 8, 34, 284, 94 });
        viz.draw = [this] (juce::Graphics& g, juce::Rectangle<float> r)
        {
            const float fs = proc.getParam (P::sub_start), fe = proc.getParam (P::sub_end);
            const float pd = proc.getParam (P::sub_pdecay), ad = proc.getParam (P::sub_decay);
            const float span = juce::jlimit (0.3f, 6.0f, pd * 2.5f);
            auto yFor = [&r] (float hz) { return r.getBottom() - r.getHeight() * juce::jlimit (0.0f, 1.0f, std::log (hz / 18.0f) / std::log (900.0f / 18.0f)); };

            juce::Path amp, pitch;
            amp.startNewSubPath (r.getX(), r.getBottom());
            for (int i = 0; i <= 80; ++i)
            {
                const float t = span * (float) i / 80.0f;
                const float x = r.getX() + r.getWidth() * (float) i / 80.0f;
                amp.lineTo (x, r.getBottom() - r.getHeight() * 0.9f * std::exp (-6.9f * t / ad));
                const float f = fe * std::pow (fs / fe, std::exp (-4.6f * t / pd));
                if (i == 0) pitch.startNewSubPath (x, yFor (f)); else pitch.lineTo (x, yFor (f));
            }
            amp.lineTo (r.getRight(), r.getBottom());
            amp.closeSubPath();
            g.setColour (col::orange.withAlpha (0.14f));
            g.fillPath (amp);
            strokeGlow (g, pitch, col::orange);
            vizLabel (g, r, juce::String (juce::roundToInt (fs)) + " > " + juce::String (juce::roundToInt (fe)) + " Hz", col::orange);
        };
        animated.add (&viz);

        make<Knob> (owned, pn, { cell (0), row2, 71, 58 }, proc, P::sub_start, "START", pn.accent);
        make<Knob> (owned, pn, { cell (1), row2, 71, 58 }, proc, P::sub_end, "END", pn.accent);
        make<Knob> (owned, pn, { cell (2), row2, 71, 58 }, proc, P::sub_pdecay, "P.DECAY", pn.accent);
        make<Knob> (owned, pn, { cell (3), row2, 71, 58 }, proc, P::sub_decay, "DECAY", pn.accent);
        make<Knob> (owned, pn, { cell (0), row3, 71, 58 }, proc, P::sub_punch, "PUNCH", pn.accent);
        make<Knob> (owned, pn, { cell (1), row3, 71, 58 }, proc, P::sub_harm, "HARMONICS", pn.accent);
        pn.caption ("WAVE", 154, row3 + 6);
        make<Choice> (owned, pn, { 154, row3 + 20, 138, 22 }, proc, P::sub_wave, pn.accent);
        strip (pn, P::sub_level, P::sub_rev, P::sub_dly);
    }

    // ------------------------------------------------------------------ IMPACT
    {
        auto& pn = make<Panel> (owned, *content, { 316, py, pw, ph }, "IMPACT", U ("インパクト"), col::amber, U ("弐"));
        header (pn, P::imp_on, P::imp_lock, kImpact);

        auto& viz = make<Viz> (owned, pn, { 8, 34, 284, 94 });
        viz.draw = [this] (juce::Graphics& g, juce::Rectangle<float> r)
        {
            const auto smp = proc.getDisplaySample();
            const bool reverse = proc.getParam (P::imp_reverse) > 0.5f;
            const float start = proc.getParam (P::imp_start);

            if (smp != nullptr && ! smp->peaks.empty())
            {
                const int n = (int) smp->peaks.size();
                const float bw = r.getWidth() / (float) n;
                for (int i = 0; i < n; ++i)
                {
                    const int src = reverse ? n - 1 - i : i;
                    const float h = juce::jmax (1.0f, smp->peaks[(size_t) src] * r.getHeight());
                    const bool skipped = (float) i / (float) n < start;
                    g.setColour (col::amber.withAlpha (skipped ? 0.25f : 0.95f));
                    g.fillRect (juce::Rectangle<float> (r.getX() + bw * (float) i, r.getCentreY() - h * 0.5f, juce::jmax (1.0f, bw - 0.4f), h));
                }
                vizLabel (g, r, smp->name + (reverse ? "  << REV" : ""), col::cream);
            }
            else
            {
                vizLabel (g, r, "DROP A WAV HERE", col::amber, juce::Justification::centred);
            }
        };
        animated.add (&viz);

        make<Knob> (owned, pn, { cell (0), row2, 71, 58 }, proc, P::imp_pitch, "PITCH", pn.accent);
        make<Knob> (owned, pn, { cell (1), row2, 71, 58 }, proc, P::imp_start, "START", pn.accent);
        make<Knob> (owned, pn, { cell (2), row2, 71, 58 }, proc, P::imp_attack, "ATTACK", pn.accent);
        make<Knob> (owned, pn, { cell (3), row2, 71, 58 }, proc, P::imp_sustain, "SUSTAIN", pn.accent);
        pn.caption ("SAMPLE", 8, row3 + 6);
        make<Choice> (owned, pn, { 8, row3 + 20, 150, 22 }, proc, P::imp_sample, pn.accent);

        auto& loadBtn = make<juce::TextButton> (owned, pn, { 162, row3 + 20, 52, 22 }, "LOAD");
        loadBtn.onClick = [this]
        {
            chooser = std::make_unique<juce::FileChooser> ("Load impact sample", juce::File(), "*.wav;*.aif;*.aiff;*.flac;*.ogg");
            chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                                  [this] (const juce::FileChooser& fc)
                                  {
                                      const auto f = fc.getResult();
                                      if (f.existsAsFile())
                                          proc.loadUserSample (f);
                                  });
        };

        make<Toggle> (owned, pn, { 218, row3 + 20, 74, 22 }, proc, P::imp_reverse, "REVERSE", pn.accent);
        strip (pn, P::imp_level, P::imp_rev, P::imp_dly);
    }

    // ------------------------------------------------------------------ SYNTH / TEXTURE
    {
        auto& pn = make<Panel> (owned, *content, { 624, py, pw, ph }, "SYNTH", U ("シンセ"), col::teal, U ("参"));
        header (pn, P::syn_on, P::syn_lock, kSynth);

        auto& viz = make<Viz> (owned, pn, { 8, 34, 284, 38 });
        viz.draw = [this] (juce::Graphics& g, juce::Rectangle<float> r)
        {
            auto left = r.removeFromLeft (r.getWidth() * 0.48f);
            r.removeFromLeft (10.0f);

            const auto& wt = Wavetables::get();
            const float fpos = proc.getParam (P::syn_wt) * (float) (Wavetables::kFrames - 1);
            const int f0 = juce::jmin (Wavetables::kFrames - 2, (int) fpos);
            const float mix = fpos - (float) f0;
            const float* a = wt.table (f0, 3);
            const float* b = wt.table (f0 + 1, 3);

            juce::Path wave;
            for (int i = 0; i <= 128; ++i)
            {
                const int idx = juce::jmin (Wavetables::kSize, i * 16);
                const float v = a[idx] + mix * (b[idx] - a[idx]);
                const float x = left.getX() + left.getWidth() * (float) i / 128.0f;
                const float y = left.getCentreY() - v * left.getHeight() * 0.46f;
                if (i == 0) wave.startNewSubPath (x, y); else wave.lineTo (x, y);
            }
            strokeGlow (g, wave, col::teal, 1.4f);

            const float at = proc.getParam (P::syn_a), dc = proc.getParam (P::syn_d);
            const float su = proc.getParam (P::syn_s), rl = proc.getParam (P::syn_r);
            auto seg = [] (float seconds) { return 0.08f + 0.92f * std::sqrt (juce::jlimit (0.0f, 1.0f, seconds / 5.0f)); };
            const float unit = r.getWidth() / 4.0f;
            float x = r.getX();

            juce::Path env;
            env.startNewSubPath (x, r.getBottom());
            x += unit * seg (at);            env.lineTo (x, r.getY());
            x += unit * seg (dc);            env.lineTo (x, r.getBottom() - r.getHeight() * su);
            x += unit * 0.6f;                env.lineTo (x, r.getBottom() - r.getHeight() * su);
            x += unit * seg (rl * 0.625f);   env.lineTo (x, r.getBottom());
            strokeGlow (g, env, col::cream, 1.3f);
        };
        animated.add (&viz);

        make<Knob> (owned, pn, { cell (0), row1, 71, 58 }, proc, P::syn_wt, "WT POS", pn.accent);
        make<Knob> (owned, pn, { cell (1), row1, 71, 58 }, proc, P::syn_uni, "UNISON", pn.accent);
        make<Knob> (owned, pn, { cell (2), row1, 71, 58 }, proc, P::syn_det, "DETUNE", pn.accent);
        make<Knob> (owned, pn, { cell (3), row1, 71, 58 }, proc, P::syn_pitch, "PITCH", pn.accent);
        make<Knob> (owned, pn, { cell (0), row2, 71, 58 }, proc, P::syn_cut, "CUTOFF", pn.accent);
        make<Knob> (owned, pn, { cell (1), row2, 71, 58 }, proc, P::syn_res, "RESO", pn.accent);
        make<Knob> (owned, pn, { cell (2), row2, 71, 58 }, proc, P::syn_noise, "NOISE", pn.accent);
        make<Choice> (owned, pn, { cell (3) - 2, row2 + 4, 73, 20 }, proc, P::syn_ftype, pn.accent);
        make<Choice> (owned, pn, { cell (3) - 2, row2 + 30, 73, 20 }, proc, P::syn_ntype, pn.accent);
        make<Knob> (owned, pn, { cell (0), row3, 71, 58 }, proc, P::syn_a, "ATTACK", col::cream);
        make<Knob> (owned, pn, { cell (1), row3, 71, 58 }, proc, P::syn_d, "DECAY", col::cream);
        make<Knob> (owned, pn, { cell (2), row3, 71, 58 }, proc, P::syn_s, "SUSTAIN", col::cream);
        make<Knob> (owned, pn, { cell (3), row3, 71, 58 }, proc, P::syn_r, "RELEASE", col::cream);
        strip (pn, P::syn_level, P::syn_rev, P::syn_dly);
    }

    // ------------------------------------------------------------------ RISER / TRANSITION
    {
        auto& pn = make<Panel> (owned, *content, { 932, py, pw, ph }, "RISER", U ("ライザー"), col::pink, U ("肆"));
        header (pn, P::ris_on, P::ris_lock, kRiser);

        auto& viz = make<Viz> (owned, pn, { 8, 34, 284, 94 });
        viz.draw = [this] (juce::Graphics& g, juce::Rectangle<float> r)
        {
            const int mode = (int) proc.getParam (P::ris_mode), curve = (int) proc.getParam (P::ris_curve);
            const int gateIdx = (int) proc.getParam (P::ris_gate), lenIdx = (int) proc.getParam (P::ris_len);
            static const int barCount[] = { 1, 2, 4, 8 };
            const int bars = barCount[juce::jlimit (0, 3, lenIdx)];

            g.setColour (col::pink.withAlpha (0.18f));
            for (int i = 1; i < bars; ++i)
                g.fillRect (juce::Rectangle<float> (r.getX() + r.getWidth() * (float) i / (float) bars, r.getY(), 1.0f, r.getHeight()));

            juce::Path fill, path;
            fill.startNewSubPath (r.getX(), r.getBottom());
            for (int i = 0; i <= 100; ++i)
            {
                const float x01 = (float) i / 100.0f;
                const float x = r.getX() + r.getWidth() * x01;
                const float y = r.getBottom() - r.getHeight() * 0.95f * RiserLayer::curveAt (x01, mode, curve);
                fill.lineTo (x, y);
                if (i == 0) path.startNewSubPath (x, y); else path.lineTo (x, y);
            }
            fill.lineTo (r.getRight(), r.getBottom());
            fill.closeSubPath();
            g.setColour (col::pink.withAlpha (0.16f));
            g.fillPath (fill);
            strokeGlow (g, path, col::pink);

            if (gateIdx > 0)
            {
                const double steps = (double) bars * 4.0 / gateDivBeats (gateIdx);
                const float sw = r.getWidth() / (float) steps;
                g.setColour (col::screen.withAlpha (0.75f));
                for (int i = 0; i < (int) steps && sw > 1.5f; ++i)
                    g.fillRect (juce::Rectangle<float> (r.getX() + sw * ((float) i + 0.5f), r.getY(), sw * 0.5f, r.getHeight()));
            }

            const float prog = proc.riserProgress.load();
            if (prog >= 0.0f)
            {
                g.setColour (col::cream);
                g.fillRect (juce::Rectangle<float> (r.getX() + r.getWidth() * juce::jmin (1.0f, prog) - 1.0f, r.getY(), 2.0f, r.getHeight()));
            }

            vizLabel (g, r, juce::String (bars) + (bars == 1 ? " BAR" : " BARS"), col::pink, juce::Justification::topLeft);
        };
        animated.add (&viz);

        make<Knob> (owned, pn, { cell (0), row2, 71, 58 }, proc, P::ris_range, "RANGE", pn.accent);
        make<Knob> (owned, pn, { cell (1), row2, 71, 58 }, proc, P::ris_tone, "TONE", pn.accent);
        make<Choice> (owned, pn, { 154, row2 + 4, 138, 20 }, proc, P::ris_mode, pn.accent);
        make<Choice> (owned, pn, { 154, row2 + 30, 138, 20 }, proc, P::ris_len, pn.accent);
        pn.caption ("CURVE", 8, row3 + 6);
        make<Choice> (owned, pn, { 8, row3 + 20, 138, 22 }, proc, P::ris_curve, pn.accent);
        pn.caption ("STUTTER / GATE", 154, row3 + 6);
        make<Choice> (owned, pn, { 154, row3 + 20, 68, 22 }, proc, P::ris_gate, pn.accent);
        make<Toggle> (owned, pn, { 226, row3 + 20, 66, 22 }, proc, P::ris_grid, "BAR SYNC", pn.accent);
        strip (pn, P::ris_level, P::ris_rev, P::ris_dly);
    }
}

//==============================================================================
void FinishLineEditor::buildFx()
{
    const int y = 622, h = 150, ky = 80;

    {
        auto& pn = make<Panel> (owned, *content, { 8, y, 190, h }, "DISTORTION", U ("歪み"), col::orange);
        make<Toggle> (owned, pn, { 148, 5, 34, 18 }, proc, P::dist_on, "ON", pn.accent);
        make<Choice> (owned, pn, { 8, 38, 174, 22 }, proc, P::dist_type, pn.accent);
        make<Knob> (owned, pn, { 22, ky, 66, 60 }, proc, P::dist_drive, "DRIVE", pn.accent);
        make<Knob> (owned, pn, { 102, ky, 66, 60 }, proc, P::dist_mix, "MIX", col::cream);
    }
    {
        auto& pn = make<Panel> (owned, *content, { 206, y, 340, h }, "FILTER", U ("フィルター"), col::teal);
        make<Toggle> (owned, pn, { 298, 5, 34, 18 }, proc, P::flt_on, "ON", pn.accent);
        make<Choice> (owned, pn, { 8, 38, 90, 22 }, proc, P::flt_type, pn.accent);
        make<Choice> (owned, pn, { 104, 38, 84, 22 }, proc, P::flt_slope, pn.accent);
        make<Knob> (owned, pn, { 10, ky, 64, 60 }, proc, P::flt_cut, "CUTOFF", pn.accent);
        make<Knob> (owned, pn, { 74, ky, 64, 60 }, proc, P::flt_res, "RESO", pn.accent);
        make<Knob> (owned, pn, { 138, ky, 64, 60 }, proc, P::flt_lfo_rate, "LFO RATE", col::cream);
        make<Knob> (owned, pn, { 202, ky, 64, 60 }, proc, P::flt_lfo_depth, "LFO DEPTH", col::cream);
        make<Knob> (owned, pn, { 266, ky, 64, 60 }, proc, P::flt_env, "ENV", col::cream);
    }
    {
        auto& pn = make<Panel> (owned, *content, { 554, y, 220, h }, "REVERB", U ("残響"), col::pink);
        make<Choice> (owned, pn, { 8, 38, 204, 22 }, proc, P::rev_mode, pn.accent);
        make<Knob> (owned, pn, { 14, ky, 64, 60 }, proc, P::rev_size, "SIZE", pn.accent);
        make<Knob> (owned, pn, { 78, ky, 64, 60 }, proc, P::rev_mix, "MIX", col::cream);
        make<Knob> (owned, pn, { 142, ky, 64, 60 }, proc, P::rev_duck, "DUCK", col::cream);
    }
    {
        auto& pn = make<Panel> (owned, *content, { 782, y, 250, h }, "DELAY", U ("ディレイ"), col::amber);
        pn.caption ("PING-PONG / BPM SYNC", 104, 44);
        make<Choice> (owned, pn, { 8, 38, 90, 22 }, proc, P::dly_time, pn.accent);
        make<Knob> (owned, pn, { 5, ky, 60, 60 }, proc, P::dly_fb, "FEEDBACK", pn.accent);
        make<Knob> (owned, pn, { 65, ky, 60, 60 }, proc, P::dly_locut, "LO-CUT", col::cream);
        make<Knob> (owned, pn, { 125, ky, 60, 60 }, proc, P::dly_hicut, "HI-CUT", col::cream);
        make<Knob> (owned, pn, { 185, ky, 60, 60 }, proc, P::dly_mix, "MIX", col::cream);
    }
    {
        auto& pn = make<Panel> (owned, *content, { 1040, y, 192, h }, "GLITCH", U ("グリッチ"), col::red);
        make<Toggle> (owned, pn, { 130, 5, 54, 18 }, proc, P::fx_lock, "FX LOCK", col::cream);
        pn.caption ("GATE", 98, 44);
        make<Choice> (owned, pn, { 8, 38, 84, 22 }, proc, P::gate_rate, pn.accent);
        make<Knob> (owned, pn, { 2, ky, 56, 60 }, proc, P::gate_depth, "DEPTH", pn.accent);
        make<Knob> (owned, pn, { 58, ky, 56, 60 }, proc, P::tape_time, "STOP TIME", col::cream);
        make<Toggle> (owned, pn, { 116, ky + 2, 70, 44 }, proc, P::tape_stop, "TAPE STOP", col::red);
    }
}

void FinishLineEditor::buildKeyboard()
{
    make<TriggerKeyboard> (owned, *content, { 12, 806, kW - 24, kH - 806 - 12 }, proc.keyState);
}

//==============================================================================
void FinishLineEditor::timerCallback()
{
    if (auto* d = dynamic_cast<Display*> (display))
        d->update();

    const float level = juce::jmax (proc.meterL.load(), proc.meterR.load());
    reelAngle += 0.02f + juce::jmin (1.0f, level) * 0.25f;
    if (reelAngle > juce::MathConstants<float>::twoPi * 100.0f)
        reelAngle = 0.0f;

    for (auto* c : animated)
        c->repaint();

    if (proc.presetName != shownPreset)
    {
        shownPreset = proc.presetName;
        if (proc.presetIndex >= 0)
            presetBox->setSelectedId (proc.presetIndex + 1, juce::dontSendNotification);
        else
        {
            presetBox->setSelectedId (0, juce::dontSendNotification);
            presetBox->setText (shownPreset, juce::dontSendNotification);
        }
    }
}

bool FinishLineEditor::isInterestedInFileDrag (const juce::StringArray& files)
{
    for (const auto& f : files)
        if (juce::File (f).hasFileExtension ("wav;aif;aiff;flac;ogg"))
            return true;
    return false;
}

void FinishLineEditor::filesDropped (const juce::StringArray& files, int, int)
{
    for (const auto& f : files)
        if (juce::File (f).hasFileExtension ("wav;aif;aiff;flac;ogg"))
        {
            proc.loadUserSample (juce::File (f));
            break;
        }
}
