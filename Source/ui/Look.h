#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

// Cassette-futurism palette and widget drawing for Finish Line.
namespace ui
{
namespace col
{
const juce::Colour bg      { 0xff141315 };
const juce::Colour panel   { 0xff212025 };
const juce::Colour panelHi { 0xff2b2a30 };
const juce::Colour line    { 0xff3b3a42 };
const juce::Colour screen  { 0xff0c0a08 };
const juce::Colour cream   { 0xffe9e2cf };
const juce::Colour dim     { 0xff8d887b };
const juce::Colour orange  { 0xffff4f1f };
const juce::Colour amber   { 0xffffb000 };
const juce::Colour teal    { 0xff2fc4b2 };
const juce::Colour pink    { 0xffff5c8a };
const juce::Colour red     { 0xffe5262b };
const juce::Colour ink     { 0xff111012 };
}

inline juce::String U (const char* utf8) { return juce::String::fromUTF8 (utf8); }

inline juce::Font font (float h, bool bold = true)
{
    return juce::Font (juce::FontOptions ("Bahnschrift", h, bold ? juce::Font::bold : juce::Font::plain));
}

inline juce::Font jp (float h)
{
    return juce::Font (juce::FontOptions ("Yu Gothic UI", h, juce::Font::bold));
}

inline juce::Font mono (float h)
{
    return juce::Font (juce::FontOptions ("Consolas", h, juce::Font::bold));
}

inline float textWidth (const juce::Font& f, const juce::String& s)
{
    juce::GlyphArrangement ga;
    ga.addLineOfText (f, s, 0.0f, 0.0f);
    return ga.getBoundingBox (0, -1, true).getWidth();
}

class Look : public juce::LookAndFeel_V4
{
public:
    Look()
    {
        setColour (juce::PopupMenu::backgroundColourId, col::panel);
        setColour (juce::PopupMenu::textColourId, col::cream);
        setColour (juce::PopupMenu::headerTextColourId, col::amber);
        setColour (juce::PopupMenu::highlightedBackgroundColourId, col::orange);
        setColour (juce::PopupMenu::highlightedTextColourId, col::ink);
        setColour (juce::ComboBox::textColourId, col::cream);
        setColour (juce::ComboBox::arrowColourId, col::orange);
        setColour (juce::TextButton::textColourOffId, col::cream);
        setColour (juce::TextButton::textColourOnId, col::ink);
        setColour (juce::TextButton::buttonColourId, col::ink);
        setColour (juce::TextButton::buttonOnColourId, col::orange);
        setColour (juce::Slider::rotarySliderFillColourId, col::orange);
        setColour (juce::Slider::trackColourId, col::orange);
    }

    void drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                           float startAngle, float endAngle, juce::Slider& s) override
    {
        const auto accent = s.findColour (juce::Slider::rotarySliderFillColourId);
        const auto area = juce::Rectangle<int> (x, y, w, h).toFloat();
        const float r = juce::jmin (area.getWidth(), area.getHeight()) * 0.5f - 2.0f;
        const auto c = area.getCentre();
        const float angle = startAngle + pos * (endAngle - startAngle);
        const float arcR = r - 1.5f;
        const bool bipolar = s.getMinimum() < 0.0 && s.getMaximum() > 0.0;

        juce::Path track;
        track.addCentredArc (c.x, c.y, arcR, arcR, 0.0f, startAngle, endAngle, true);
        g.setColour (col::ink);
        g.strokePath (track, juce::PathStrokeType (3.0f));

        const float from = bipolar ? startAngle + (float) s.valueToProportionOfLength (0.0) * (endAngle - startAngle) : startAngle;
        juce::Path value;
        value.addCentredArc (c.x, c.y, arcR, arcR, 0.0f, juce::jmin (from, angle), juce::jmax (from, angle), true);
        g.setColour (accent.withAlpha (s.isEnabled() ? 1.0f : 0.3f));
        g.strokePath (value, juce::PathStrokeType (3.0f));

        const float capR = r * 0.72f;
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff3a3940), c.x, c.y - capR,
                                                 juce::Colour (0xff19181c), c.x, c.y + capR, false));
        g.fillEllipse (c.x - capR, c.y - capR, capR * 2.0f, capR * 2.0f);
        g.setColour (juce::Colour (0xff09090a));
        g.drawEllipse (c.x - capR, c.y - capR, capR * 2.0f, capR * 2.0f, 1.2f);

        juce::Path pointer;
        pointer.addRoundedRectangle (-1.3f, -capR + 2.0f, 2.6f, capR * 0.62f, 1.0f);
        g.setColour (col::cream);
        g.fillPath (pointer, juce::AffineTransform::rotation (angle).translated (c.x, c.y));
    }

    void drawLinearSlider (juce::Graphics& g, int x, int y, int w, int h, float sliderPos, float, float,
                           juce::Slider::SliderStyle, juce::Slider& s) override
    {
        const auto r = juce::Rectangle<int> (x, y, w, h).toFloat().reduced (0.0f, juce::jmax (0.0f, (float) h * 0.5f - 3.0f));
        g.setColour (col::ink);
        g.fillRoundedRectangle (r, 2.0f);

        const float zero = (float) x + (float) s.valueToProportionOfLength (0.0) * (float) w;
        const float a = juce::jmin (zero, sliderPos), b = juce::jmax (zero, sliderPos);
        g.setColour (s.findColour (juce::Slider::trackColourId));
        g.fillRect (juce::Rectangle<float> (a, r.getY(), b - a, r.getHeight()));
        g.setColour (col::cream);
        g.fillRect (juce::Rectangle<float> (sliderPos - 1.0f, r.getY() - 2.0f, 2.0f, r.getHeight() + 4.0f));
    }

    void drawComboBox (juce::Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox& box) override
    {
        const auto r = juce::Rectangle<int> (0, 0, w, h).toFloat().reduced (0.5f);
        g.setColour (col::ink);
        g.fillRoundedRectangle (r, 3.0f);
        g.setColour (box.hasKeyboardFocus (true) ? col::dim : col::line);
        g.drawRoundedRectangle (r, 3.0f, 1.0f);

        juce::Path arrow;
        const float ax = (float) w - 11.0f, ay = (float) h * 0.5f;
        arrow.addTriangle (ax - 4.0f, ay - 2.5f, ax + 4.0f, ay - 2.5f, ax, ay + 3.5f);
        g.setColour (box.findColour (juce::ComboBox::arrowColourId));
        g.fillPath (arrow);
    }

    juce::Font getComboBoxFont (juce::ComboBox& box) override { return font (juce::jmin (12.5f, (float) box.getHeight() * 0.62f)); }

    void positionComboBoxText (juce::ComboBox& box, juce::Label& label) override
    {
        label.setBounds (5, 0, box.getWidth() - 20, box.getHeight());
        label.setFont (getComboBoxFont (box));
    }

    juce::Font getPopupMenuFont() override { return font (13.5f); }

    void drawPopupMenuBackground (juce::Graphics& g, int w, int h) override
    {
        g.fillAll (col::panel);
        g.setColour (col::line);
        g.drawRect (0, 0, w, h, 1);
    }

    void drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool over, bool down) override
    {
        const auto r = b.getLocalBounds().toFloat().reduced (0.5f);
        const auto on = b.findColour (juce::TextButton::buttonOnColourId);
        const bool lit = b.getToggleState() || down;

        g.setColour (lit ? on : (over ? col::panelHi : col::ink));
        g.fillRoundedRectangle (r, 3.0f);
        g.setColour (lit ? on.brighter (0.3f) : (over ? on.withAlpha (0.8f) : col::line));
        g.drawRoundedRectangle (r, 3.0f, 1.0f);
    }

    juce::Font getTextButtonFont (juce::TextButton&, int h) override { return font (juce::jmin (12.5f, (float) h * 0.6f)); }

    void drawButtonText (juce::Graphics& g, juce::TextButton& b, bool, bool down) override
    {
        const bool lit = b.getToggleState() || down;
        g.setFont (getTextButtonFont (b, b.getHeight()));
        g.setColour (lit ? col::ink : b.findColour (juce::TextButton::textColourOffId).withAlpha (b.isEnabled() ? 1.0f : 0.4f));
        g.drawFittedText (b.getButtonText(), b.getLocalBounds().reduced (2, 0), juce::Justification::centred, 1, 0.75f);
    }
};
} // namespace ui
