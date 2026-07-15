/*
 This file is part of LiveCut Enhanced.
 Based on Livecut, Copyright 2004 by Remy Muller.

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
        return juce::Font (juce::FontOptions (12.5f));
    }

    juce::Font getComboBoxFont (juce::ComboBox&) override
    {
        return juce::Font (juce::FontOptions (13.5f));
    }

    juce::Font getTextButtonFont (juce::TextButton&, int) override
    {
        return juce::Font (juce::FontOptions (12.5f, juce::Font::bold));
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

        auto toAngle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

        // bipolar parameters fill from 12 o'clock, others from the start of the arc
        const bool bipolar = slider.getMinimum() < 0.0 && slider.getMaximum() > 0.0;
        auto fillStart = bipolar ? (rotaryStartAngle + rotaryEndAngle) / 2.f : rotaryStartAngle;

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

        auto thumbRadius = arcRadius - lineW * 1.4f;
        juce::Point<float> thumb (centre.x
                                      + thumbRadius * std::cos (toAngle - juce::MathConstants<float>::halfPi),
                                  centre.y
                                      + thumbRadius * std::sin (toAngle - juce::MathConstants<float>::halfPi));
        g.setColour (fill);
        g.fillEllipse (juce::Rectangle<float> (lineW * 1.1f, lineW * 1.1f).withCentre (thumb));

        g.setColour (slider.isEnabled() ? Colors::text : Colors::textDim);
        g.setFont (juce::Font (juce::FontOptions (juce::jmax (11.f, radius * 0.42f))));
        g.drawText (slider.getTextFromValue (slider.getValue()),
                    square.toNearestInt().reduced (int (lineW * 2.f)),
                    juce::Justification::centred, false);
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
