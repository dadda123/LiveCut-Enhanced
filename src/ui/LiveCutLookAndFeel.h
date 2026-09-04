/*
 This file is part of LiveCut Enhanced.
 Based on Livecut, Copyright 2004 by Remy Muller.
 Expanded modulation/UI modifications, 2026-08-10.

 Livecut can be redistributed and/or modified under the terms of the
 GNU General Public License, as published by the Free Software Foundation;
 either version 2 of the License, or (at your option) any later version.

 Livecut is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 */

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace Colors
{
const juce::Colour background {0xff0f1115};
const juce::Colour panel {0xff171b21};
const juce::Colour panelOutline {0xff242a33};
const juce::Colour text {0xffe8eaed};
const juce::Colour textDim {0xff8a919e};
const juce::Colour knobTrack {0xff2a313c};
const juce::Colour accent {0xffff6b3d};
const juce::Colour accentProc {0xff8b7cff};
const juce::Colour accentFx {0xff2dd4bf};
const juce::Colour led {0xffffc24d};

// Negative modulation gets a complementary colour for each control family so
// it cannot disappear into the knob's normal value arc. In particular, the
// orange main-page knobs use a bright sky blue for negative travel.
const juce::Colour modNegativeOrange {0xff45c8ff};
const juce::Colour modNegativeProc   {0xffffd45a};
const juce::Colour modNegativeFx     {0xffff5aa7};

inline juce::Colour negativeModulationFor (juce::Colour positive)
{
    if (positive.getARGB() == accent.getARGB())
        return modNegativeOrange;
    if (positive.getARGB() == accentProc.getARGB())
        return modNegativeProc;
    if (positive.getARGB() == accentFx.getARGB())
        return modNegativeFx;
    return modNegativeOrange;
}
} // namespace Colors

class LiveCutLookAndFeel : public juce::LookAndFeel_V4
{
public:
    LiveCutLookAndFeel()
    {
        setColour (juce::ResizableWindow::backgroundColourId, Colors::background);
        setColour (juce::Slider::rotarySliderFillColourId, Colors::accent);
        setColour (juce::Slider::rotarySliderOutlineColourId, Colors::knobTrack);
        setColour (juce::Slider::textBoxTextColourId, Colors::text);
        setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        setColour (juce::Label::textColourId, Colors::textDim);
        setColour (juce::ComboBox::backgroundColourId, Colors::knobTrack.withAlpha (0.6f));
        setColour (juce::ComboBox::textColourId, Colors::text);
        setColour (juce::ComboBox::outlineColourId, Colors::panelOutline);
        setColour (juce::ComboBox::arrowColourId, Colors::textDim);
        setColour (juce::PopupMenu::backgroundColourId, Colors::panel);
        setColour (juce::PopupMenu::textColourId, Colors::text);
        setColour (juce::PopupMenu::highlightedBackgroundColourId,
                   Colors::accent.withAlpha (0.25f));
        setColour (juce::PopupMenu::highlightedTextColourId, Colors::text);
        setColour (juce::TextButton::buttonColourId, Colors::knobTrack.withAlpha (0.6f));
        setColour (juce::TextButton::buttonOnColourId, Colors::accent);
        setColour (juce::TextButton::textColourOffId, Colors::textDim);
        setColour (juce::TextButton::textColourOnId, juce::Colour (0xff15110e));
        setColour (juce::TooltipWindow::backgroundColourId, Colors::panel);
        setColour (juce::TooltipWindow::textColourId, Colors::text);
    }

    juce::Font getLabelFont (juce::Label&) override
    {
        // Upstream used 12.5 pt. 17 pt makes the parameter labels substantially
        // easier to read without changing the control grid itself.
        return juce::Font (juce::FontOptions (17.0f));
    }

    juce::Font getComboBoxFont (juce::ComboBox&) override
    {
        return juce::Font (juce::FontOptions (13.5f));
    }

    juce::Font getTextButtonFont (juce::TextButton& button, int) override
    {
        const auto id = button.getComponentID();
        const auto size = id == "pageTab" ? 16.0f
                        : id == "cutProcTab" ? 14.5f
                        : id == "modSubTab" ? 14.0f
                        : 12.5f;
        return juce::Font (juce::FontOptions (size, juce::Font::bold));
    }

    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                           juce::Slider& slider) override
    {
        auto bounds = juce::Rectangle<int> (x, y, width, height).toFloat().reduced (4.f);
        auto size = juce::jmin (bounds.getWidth(), bounds.getHeight());
        auto square = bounds.withSizeKeepingCentre (size, size);
        auto radius = size / 2.f;
        auto centre = square.getCentre();
        auto lineW = juce::jmax (2.4f, radius * 0.14f);
        auto arcRadius = radius - lineW / 2.f;

        auto fill = slider.findColour (juce::Slider::rotarySliderFillColourId);
        auto track = slider.findColour (juce::Slider::rotarySliderOutlineColourId);

        juce::Path backgroundArc;
        backgroundArc.addCentredArc (centre.x, centre.y, arcRadius, arcRadius, 0.f,
                                     rotaryStartAngle, rotaryEndAngle, true);
        g.setColour (track);
        g.strokePath (backgroundArc,
                      juce::PathStrokeType (lineW, juce::PathStrokeType::curved,
                                            juce::PathStrokeType::rounded));

        const auto toAngle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

        // Draw the underlying parameter value first. The modulation overlay is
        // deliberately drawn afterwards; UI05 drew it first, so negative travel
        // (which sits on top of the existing value arc) was largely painted over.
        const bool bipolar = slider.getMinimum() < 0.0 && slider.getMaximum() > 0.0;
        const auto fillStart = bipolar ? (rotaryStartAngle + rotaryEndAngle) / 2.f
                                       : rotaryStartAngle;

        if (std::abs (toAngle - fillStart) > 0.01f)
        {
            juce::Path valueArc;
            valueArc.addCentredArc (centre.x, centre.y, arcRadius, arcRadius, 0.f,
                                    juce::jmin (fillStart, toAngle),
                                    juce::jmax (fillStart, toAngle), true);
            g.setColour (fill);
            g.strokePath (valueArc,
                          juce::PathStrokeType (lineW, juce::PathStrokeType::curved,
                                                juce::PathStrokeType::rounded));
        }

        const auto thumbRadius = arcRadius - lineW * 1.4f;
        juce::Point<float> thumb (
            centre.x + thumbRadius * std::cos (toAngle - juce::MathConstants<float>::halfPi),
            centre.y + thumbRadius * std::sin (toAngle - juce::MathConstants<float>::halfPi));
        g.setColour (fill);
        g.fillEllipse (juce::Rectangle<float> (lineW * 1.1f, lineW * 1.1f).withCentre (thumb));

        const auto modAmount = float (double (
            slider.getProperties().getWithDefault ("modAmount", 0.0)));

        if (std::abs (modAmount) > 0.001f)
        {
            const auto modulatedPos = juce::jlimit (0.f, 1.f, sliderPos + modAmount);
            const auto modulatedAngle =
                rotaryStartAngle + modulatedPos * (rotaryEndAngle - rotaryStartAngle);
            const auto modulationColour = modAmount < 0.f
                ? Colors::negativeModulationFor (fill)
                : fill;

            // Use a separate inner modulation ring. This stays visible even when
            // negative modulation travels back across a brightly-coloured base arc.
            const auto modArcRadius = juce::jmax (4.f, arcRadius - lineW * 0.82f);
            juce::Path modulationArc;
            modulationArc.addCentredArc (centre.x, centre.y, modArcRadius, modArcRadius, 0.f,
                                         juce::jmin (toAngle, modulatedAngle),
                                         juce::jmax (toAngle, modulatedAngle), true);

            const auto magnitude = juce::jlimit (0.f, 1.f, std::abs (modAmount));
            g.setColour (modulationColour.withAlpha (0.10f + magnitude * 0.08f));
            g.strokePath (modulationArc,
                          juce::PathStrokeType (lineW * 1.45f, juce::PathStrokeType::curved,
                                                juce::PathStrokeType::rounded));
            g.setColour (modulationColour.withAlpha (0.26f + magnitude * 0.18f));
            g.strokePath (modulationArc,
                          juce::PathStrokeType (lineW * 0.90f, juce::PathStrokeType::curved,
                                                juce::PathStrokeType::rounded));
            g.setColour (modulationColour.withAlpha (0.96f));
            g.strokePath (modulationArc,
                          juce::PathStrokeType (juce::jmax (2.0f, lineW * 0.34f),
                                                juce::PathStrokeType::curved,
                                                juce::PathStrokeType::rounded));

            // The modulated-position marker is also drawn last. This means a
            // negative value remains visible even when the parameter itself is
            // clamped at its minimum and the travel arc collapses to zero length.
            const auto markerRadius = modArcRadius;
            juce::Point<float> modThumb (
                centre.x + markerRadius * std::cos (modulatedAngle - juce::MathConstants<float>::halfPi),
                centre.y + markerRadius * std::sin (modulatedAngle - juce::MathConstants<float>::halfPi));
            g.setColour (modulationColour.withAlpha (0.18f + magnitude * 0.16f));
            g.fillEllipse (juce::Rectangle<float> (juce::jmax (11.f, lineW * 1.65f),
                                                   juce::jmax (11.f, lineW * 1.65f))
                               .withCentre (modThumb));
            g.setColour (modulationColour.withAlpha (0.98f));
            g.fillEllipse (juce::Rectangle<float> (juce::jmax (5.f, lineW * 0.78f),
                                                   juce::jmax (5.f, lineW * 0.78f))
                               .withCentre (modThumb));
        }

        g.setColour (slider.isEnabled() ? Colors::text : Colors::textDim);
        g.setFont (juce::Font (juce::FontOptions (juce::jmax (11.f, radius * 0.42f))));
        g.drawText (slider.getTextFromValue (slider.getValue()),
                    square.toNearestInt().reduced (int (lineW * 2.f)),
                    juce::Justification::centred, false);
    }

    void drawLinearSlider (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPos, float minSliderPos, float maxSliderPos,
                           const juce::Slider::SliderStyle style, juce::Slider& slider) override
    {
        if (slider.getComponentID() != "modDepthSlider")
        {
            juce::LookAndFeel_V4::drawLinearSlider (g, x, y, width, height,
                                                    sliderPos, minSliderPos, maxSliderPos,
                                                    style, slider);
            return;
        }

        const auto centreY = float (y) + float (height) * 0.5f;
        const auto track = juce::Rectangle<float> (float (x), centreY - 2.f,
                                                   float (width), 4.f);
        const auto centreX = (minSliderPos + maxSliderPos) * 0.5f;
        const auto active = bool (
            slider.getProperties().getWithDefault ("modRouteActive", false));

        g.setColour (Colors::knobTrack.withAlpha (0.9f));
        g.fillRoundedRectangle (track, 2.f);

        const auto fillColour = slider.findColour (juce::Slider::trackColourId);
        const auto amountColour = sliderPos < centreX
            ? Colors::negativeModulationFor (fillColour)
            : fillColour;
        const auto left = juce::jmin (centreX, sliderPos);
        const auto right = juce::jmax (centreX, sliderPos);
        auto amountTrack = juce::Rectangle<float> (left, centreY - 2.f,
                                                   juce::jmax (1.f, right - left), 4.f);

        if (active && std::abs (sliderPos - centreX) > 0.5f)
        {
            g.setColour (amountColour.withAlpha (0.09f));
            g.fillRoundedRectangle (amountTrack.expanded (5.f, 5.f), 6.f);
            g.setColour (amountColour.withAlpha (0.20f));
            g.fillRoundedRectangle (amountTrack.expanded (3.f, 3.f), 4.f);
            g.setColour (amountColour.withAlpha (0.42f));
            g.fillRoundedRectangle (amountTrack.expanded (1.5f, 1.5f), 3.f);
        }

        g.setColour (amountColour.withAlpha (active ? 0.98f : 0.55f));
        g.fillRoundedRectangle (amountTrack, 2.f);

        g.setColour (Colors::textDim.withAlpha (0.55f));
        g.fillRect (juce::Rectangle<float> (1.f, 10.f).withCentre (juce::Point<float> {centreX, centreY}));

        const auto thumbColour = sliderPos < centreX
            ? Colors::negativeModulationFor (slider.findColour (juce::Slider::thumbColourId))
            : slider.findColour (juce::Slider::thumbColourId);
        if (active)
        {
            g.setColour (thumbColour.withAlpha (0.18f));
            g.fillEllipse (juce::Rectangle<float> (18.f, 18.f).withCentre (juce::Point<float> {sliderPos, centreY}));
            g.setColour (thumbColour.withAlpha (0.30f));
            g.fillEllipse (juce::Rectangle<float> (13.f, 13.f).withCentre (juce::Point<float> {sliderPos, centreY}));
        }

        g.setColour (thumbColour.withAlpha (active ? 1.f : 0.75f));
        g.fillEllipse (juce::Rectangle<float> (9.f, 9.f).withCentre (juce::Point<float> {sliderPos, centreY}));
    }

    void drawToggleButton (juce::Graphics& g, juce::ToggleButton& button,
                           bool shouldDrawButtonAsHighlighted, bool) override
    {
        auto bounds = button.getLocalBounds().toFloat();
        auto size = juce::jmin (bounds.getWidth(), bounds.getHeight()) - 4.f;
        auto circle = bounds.withSizeKeepingCentre (size, size);
        const bool on = button.getToggleState();
        auto accent = button.findColour (juce::TextButton::buttonOnColourId);

        g.setColour (on ? accent.withAlpha (0.18f) : Colors::knobTrack.withAlpha (0.5f));
        g.fillEllipse (circle);
        g.setColour (on ? accent : (shouldDrawButtonAsHighlighted ? Colors::text : Colors::textDim));

        auto symbol = circle.reduced (size * 0.28f);
        juce::Path p;
        p.addCentredArc (symbol.getCentreX(), symbol.getCentreY(), symbol.getWidth() / 2.f,
                         symbol.getHeight() / 2.f, 0.f, 0.6f, juce::MathConstants<float>::twoPi - 0.6f,
                         true);
        g.strokePath (p, juce::PathStrokeType (1.8f, juce::PathStrokeType::curved,
                                               juce::PathStrokeType::rounded));
        g.drawLine ({symbol.getCentreX(), symbol.getY() - size * 0.06f, symbol.getCentreX(),
                     symbol.getCentreY() - size * 0.02f},
                    1.8f);
    }

    void drawComboBox (juce::Graphics& g, int width, int height, bool, int, int, int, int,
                       juce::ComboBox& box) override
    {
        auto bounds = juce::Rectangle<int> (0, 0, width, height).toFloat().reduced (0.5f);
        g.setColour (box.findColour (juce::ComboBox::backgroundColourId));
        g.fillRoundedRectangle (bounds, 6.f);
        g.setColour (box.findColour (juce::ComboBox::outlineColourId));
        g.drawRoundedRectangle (bounds, 6.f, 1.f);

        juce::Path arrow;
        auto arrowZone = bounds.removeFromRight (24.f).reduced (7.f, float (height) * 0.38f);
        arrow.startNewSubPath (arrowZone.getX(), arrowZone.getY());
        arrow.lineTo (arrowZone.getCentreX(), arrowZone.getBottom());
        arrow.lineTo (arrowZone.getRight(), arrowZone.getY());
        g.setColour (box.findColour (juce::ComboBox::arrowColourId));
        g.strokePath (arrow, juce::PathStrokeType (1.6f));
    }
};
