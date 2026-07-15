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

#include "PluginProcessor.h"
#include "ui/AboutOverlay.h"
#include "ui/LiveCutLookAndFeel.h"

//==============================================================================
class Cell : public juce::Component
{
public:
    explicit Cell (const juce::String& cellName)
    {
        name.setText (cellName, juce::dontSendNotification);
        name.setJustificationType (juce::Justification::centred);
        name.setInterceptsMouseClicks (false, false);
        addAndMakeVisible (name);
    }

    void resized() override
    {
        auto r = getLocalBounds();
        name.setBounds (r.removeFromBottom (18));
        contentArea = r;
        layoutContent();
    }

    void setHint (const juce::String& text) { hint = text; }
    void setHintActive (bool active) { applyTooltip (active ? hint : juce::String()); }

protected:
    virtual void layoutContent() = 0;
    virtual void applyTooltip (const juce::String& tooltip) = 0;

    juce::Label name;
    juce::Rectangle<int> contentArea;
    juce::String hint;
};

//==============================================================================
class Knob : public Cell
{
public:
    Knob (juce::AudioProcessorValueTreeState& apvts, const juce::String& paramID,
          const juce::String& displayName, const juce::String& suffix,
          juce::Colour accent, int numDecimals = 0);

    juce::Slider slider;

private:
    void layoutContent() override { slider.setBounds (contentArea); }
    void applyTooltip (const juce::String& tooltip) override { slider.setTooltip (tooltip); }

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
};

//==============================================================================
class LabeledCombo : public Cell
{
public:
    LabeledCombo (juce::AudioProcessorValueTreeState& apvts, const juce::String& paramID,
                  const juce::String& displayName);

    juce::ComboBox box;

private:
    void layoutContent() override
    {
        box.setBounds (contentArea.withSizeKeepingCentre (
            juce::jmin (contentArea.getWidth(), 86), 28));
    }
    void applyTooltip (const juce::String& tooltip) override { box.setTooltip (tooltip); }

    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> attachment;
};

//==============================================================================
class PowerCell : public Cell
{
public:
    PowerCell (juce::AudioProcessorValueTreeState& apvts, const juce::String& paramID,
               const juce::String& displayName, juce::Colour accent);

    juce::ToggleButton button;

private:
    void layoutContent() override
    {
        auto size = juce::jmin (contentArea.getWidth(), contentArea.getHeight(), 44);
        button.setBounds (contentArea.withSizeKeepingCentre (size, size));
    }
    void applyTooltip (const juce::String& tooltip) override { button.setTooltip (tooltip); }

    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> attachment;
};

//==============================================================================
class Panel : public juce::Component
{
public:
    Panel (const juce::String& panelTitle, juce::Colour panelAccent)
        : title (panelTitle), accent (panelAccent)
    {
    }

    void addCell (juce::Component& c)
    {
        cells.push_back (&c);
        addAndMakeVisible (c);
    }

    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat().reduced (0.5f);
        g.setColour (Colors::panel);
        g.fillRoundedRectangle (r, 10.f);
        g.setColour (Colors::panelOutline);
        g.drawRoundedRectangle (r, 10.f, 1.f);

        auto header = getLocalBounds().reduced (14, 0).removeFromTop (30);
        g.setColour (accent);
        g.fillRoundedRectangle (header.removeFromLeft (16).withSizeKeepingCentre (8, 8).toFloat(),
                                2.f);
        header.removeFromLeft (6);
        g.setColour (Colors::textDim);
        g.setFont (juce::Font (juce::FontOptions (12.f, juce::Font::bold)));
        g.drawText (title.toUpperCase(), header, juce::Justification::centredLeft, false);
    }

    void resized() override
    {
        auto r = getLocalBounds().reduced (10);
        r.removeFromTop (28);
        if (cells.empty())
            return;
        const auto w = r.getWidth() / int (cells.size());
        for (auto* c : cells)
            c->setBounds (r.removeFromLeft (w).reduced (2, 0));
    }

private:
    juce::String title;
    juce::Colour accent;
    std::vector<juce::Component*> cells;
};

//==============================================================================
class SegmentedControl : public juce::Component
{
public:
    SegmentedControl (juce::RangedAudioParameter& param, const juce::StringArray& labels);

    void setButtonTooltips (const juce::StringArray& tips)
    {
        for (size_t i = 0; i < buttons.size(); ++i)
            buttons[i]->setTooltip (int (i) < tips.size() ? tips[int (i)] : juce::String());
    }

    void resized() override
    {
        auto r = getLocalBounds();
        const auto w = r.getWidth() / int (buttons.size());
        for (auto& b : buttons)
            b->setBounds (r.removeFromLeft (w));
    }

private:
    std::vector<std::unique_ptr<juce::TextButton>> buttons;
    juce::ParameterAttachment attachment;
};

//==============================================================================
class LiveCutEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit LiveCutEditor (LiveCutAudioProcessor&);
    ~LiveCutEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void updateProcVisibility();
    void setHints();
    void applyHints();

    LiveCutAudioProcessor& processor;
    LiveCutLookAndFeel lookAndFeel;
    juce::TooltipWindow tooltipWindow {this, 250};

    SegmentedControl procSelector;
    juce::TextButton bypassButton {"BYPASS"};
    juce::TextButton helpButton {"?"};
    juce::TextButton aboutButton {"i"};
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAttachment;
    std::vector<Cell*> hintedCells;
    juce::Rectangle<int> titleArea, ledArea;
    float ledLevel = 0.f;
    juce::uint32 lastCutCount = 0;

    Panel programPanel {"Program", Colors::accent};
    LabeledCombo subdiv;
    Knob seed, fade, minPhrase, maxPhrase, duty, fillDuty;

    Panel rangesPanel {"Ranges", Colors::accent};
    Knob minAmp, maxAmp, minPan, maxPan, minPitch, maxPitch;

    // only the panel for the currently selected cut procedure is visible
    Panel cutProc11Panel {"CutProc11", Colors::accentProc};
    Knob minRepeat, maxRepeat, stutter, area;
    Panel warpCutPanel {"WarpCut", Colors::accentProc};
    Knob straight, regular, ritard, speed;
    Panel sqPusherPanel {"SQPusher", Colors::accentProc};
    Knob activity;

    Panel crusherPanel {"Crusher", Colors::accentFx};
    PowerCell crusherOn;
    Knob minBits, maxBits, minFreq, maxFreq;
    Panel combPanel {"Comb", Colors::accentFx};
    PowerCell combOn;
    LabeledCombo combType;
    Knob combFeedback, combMinDelay, combMaxDelay;

    AboutOverlay aboutOverlay;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LiveCutEditor)
};
