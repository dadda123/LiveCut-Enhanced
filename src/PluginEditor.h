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
        name.setBounds (r.removeFromBottom (24));
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

    int getModulationDestination() const noexcept { return modulationDestination; }
    void setModulationAmount (float amount)
    {
        slider.getProperties().set ("modAmount", double (amount));
        slider.repaint();
    }

private:
    void layoutContent() override { slider.setBounds (contentArea); }
    void applyTooltip (const juce::String& tooltip) override { slider.setTooltip (tooltip); }

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    int modulationDestination = Modulation::destOff;
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
            juce::jmin (contentArea.getWidth(), 108), 30));
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
        : title (panelTitle), accent (panelAccent) {}

    void addCell (juce::Component& c)
    {
        cells.push_back (&c);
        addAndMakeVisible (c);
    }

    void setCompactCellLayout (bool shouldCompact) { compactCellLayout = shouldCompact; }

    void setActivityProvider (std::function<float()> provider)
    {
        activityProvider = std::move (provider);
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
        g.setFont (juce::Font (juce::FontOptions (13.5f, juce::Font::bold)));
        g.drawText (title.toUpperCase(), header, juce::Justification::centredLeft, false);

        if (activityProvider && ! activityIndicatorBounds.isEmpty())
        {
            const auto value = juce::jlimit (-1.f, 1.f, activityProvider());
            const auto magnitude = juce::jlimit (0.f, 1.f, std::abs (value));
            const auto colour = value < 0.f ? Colors::negativeModulationFor (accent) : accent;
            const auto box = activityIndicatorBounds.toFloat().reduced (0.5f);
            const auto inner = box.reduced (6.f);
            const auto centreY = inner.getCentreY();
            const auto markerY = juce::jmap (value, -1.f, 1.f,
                                             inner.getBottom(), inner.getY());

            g.setColour (Colors::background.withAlpha (0.64f));
            g.fillRoundedRectangle (box, 6.f);
            g.setColour (Colors::panelOutline.brighter (0.18f));
            g.drawRoundedRectangle (box, 6.f, 1.f);

            // A centre line makes bipolar polarity obvious at a glance.
            g.setColour (Colors::textDim.withAlpha (0.30f));
            g.drawHorizontalLine (juce::roundToInt (centreY),
                                  inner.getX(), inner.getRight());

            // Illuminate the travelled distance from zero to the current value.
            const auto top = juce::jmin (centreY, markerY);
            const auto bottom = juce::jmax (centreY, markerY);
            auto levelBar = juce::Rectangle<float> (8.f, juce::jmax (1.f, bottom - top))
                                .withCentre (juce::Point<float> {inner.getCentreX(), (top + bottom) * 0.5f});

            if (magnitude > 0.002f)
            {
                g.setColour (colour.withAlpha (0.09f + magnitude * 0.10f));
                g.fillRoundedRectangle (levelBar.expanded (10.f, 4.f), 6.f);
                g.setColour (colour.withAlpha (0.18f + magnitude * 0.18f));
                g.fillRoundedRectangle (levelBar.expanded (6.f, 2.f), 5.f);
                g.setColour (colour.withAlpha (0.50f + magnitude * 0.38f));
                g.fillRoundedRectangle (levelBar, 4.f);
            }

            auto marker = juce::Rectangle<float> (13.f, 13.f)
                              .withCentre (juce::Point<float> {inner.getCentreX(), markerY});
            g.setColour (colour.withAlpha (0.15f + magnitude * 0.15f));
            g.fillRoundedRectangle (marker.expanded (7.f), 5.f);
            g.setColour (colour.withAlpha (0.95f));
            g.fillRoundedRectangle (marker, 3.f);

            g.setFont (juce::Font (juce::FontOptions (10.f, juce::Font::bold)));
            g.setColour (Colors::textDim.withAlpha (0.65f));
            g.drawText ("+", activityIndicatorBounds.getX(), activityIndicatorBounds.getY() + 2,
                        activityIndicatorBounds.getWidth(), 11,
                        juce::Justification::centred, false);
            g.drawText ("-", activityIndicatorBounds.getX(),
                        activityIndicatorBounds.getBottom() - 13,
                        activityIndicatorBounds.getWidth(), 11,
                        juce::Justification::centred, false);
        }
    }

    void resized() override
    {
        auto r = getLocalBounds().reduced (10);
        r.removeFromTop (28);
        if (cells.empty())
            return;

        activityIndicatorBounds = {};

        if (activityProvider)
        {
            const auto indicatorSize = juce::jlimit (56, 78, r.getHeight() - 12);
            auto indicatorStrip = r.removeFromRight (indicatorSize + 18);
            activityIndicatorBounds =
                indicatorStrip.withSizeKeepingCentre (indicatorSize, indicatorSize);
            r.removeFromRight (6);
        }

        if (compactCellLayout)
        {
            // Keep the source controls grouped closely together, while leaving
            // a dedicated readable activity display at the right-hand side.
            const auto compactWidth = juce::roundToInt (float (r.getWidth()) * 0.82f);
            r = r.withSizeKeepingCentre (compactWidth, r.getHeight());
        }

        const auto w = r.getWidth() / int (cells.size());
        for (auto* c : cells)
            c->setBounds (r.removeFromLeft (w).reduced (2, 0));
    }

private:
    juce::String title;
    juce::Colour accent;
    std::vector<juce::Component*> cells;
    std::function<float()> activityProvider;
    juce::Rectangle<int> activityIndicatorBounds;
    bool compactCellLayout = false;
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
class ModMatrixRow : public juce::Component
{
public:
    ModMatrixRow (juce::AudioProcessorValueTreeState& apvts, int slotIndex);
    void resized() override;
    void paint (juce::Graphics&) override;
    void refreshVisualState();
    void setHelpMode (bool enabled);

private:
    int slot = 0;
    juce::ToggleButton enabled;
    juce::ComboBox source;
    juce::ComboBox destination;
    juce::Slider depth;
    juce::RangedAudioParameter* enabledParameter = nullptr;

    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> enabledAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> sourceAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> destinationAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> depthAttachment;
};

class ModMatrixPage : public juce::Component
{
public:
    explicit ModMatrixPage (juce::AudioProcessorValueTreeState& apvts);
    void paint (juce::Graphics&) override;
    void resized() override;
    void refreshVisuals();
    void setHelpMode (bool enabled);

private:
    juce::Viewport viewport;
    juce::Component content;
    std::vector<std::unique_ptr<ModMatrixRow>> rows;
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
    struct SourcePanelSet
    {
        std::unique_ptr<Panel> panel;
        std::vector<std::unique_ptr<Cell>> cells;
        Cell* rateCell = nullptr;
        Cell* divisionCell = nullptr;
        std::atomic<float>* syncValue = nullptr;
    };

    void timerCallback() override;
    void updateProcVisibility();
    void updatePageVisibility();
    void setMainPage (int page);
    void setModPage (int page);
    void createModulationSourcePanels();
    void setHints();
    void applyHints();
    void saveUiSize();

    LiveCutAudioProcessor& processor;
    LiveCutLookAndFeel lookAndFeel;
    juce::TooltipWindow tooltipWindow {this, 250};

    SegmentedControl procSelector;
    juce::TextButton mainPageButton {"MAIN"};
    juce::TextButton modulationPageButton {"MODULATION"};
    juce::TextButton lfoPageButton {"LFOs"};
    juce::TextButton wanderPageButton {"WANDER"};
    juce::TextButton matrixPageButton {"MATRIX"};
    juce::TextButton bypassButton {"BYPASS"};
    juce::TextButton helpButton {"?"};
    juce::TextButton aboutButton {"i"};
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAttachment;
    std::vector<Cell*> hintedCells;
    std::vector<Knob*> modulationKnobs;
    juce::Rectangle<int> titleArea, ledArea;
    float ledLevel = 0.f;
    juce::uint32 lastCutCount = 0;
    int currentMainPage = 0;
    int currentModPage = 0;

    bool uiSizeDirty = false;
    bool uiSizePersistenceActive = false;
    int preferredUiWidth = 1020;
    int preferredUiHeight = 640;
    juce::uint32 editorOpenTime = 0;
    juce::uint32 lastUiResizeTime = 0;

    Panel programPanel {"Program", Colors::accent};
    LabeledCombo subdiv;
    Knob seed, fade, minPhrase, maxPhrase, duty, fillDuty;

    Panel rangesPanel {"Ranges", Colors::accent};
    Knob minAmp, maxAmp, minPan, maxPan, minPitch, maxPitch;

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

    std::vector<SourcePanelSet> lfoPanels;
    std::vector<SourcePanelSet> wanderPanels;
    ModMatrixPage matrixPage;

    AboutOverlay aboutOverlay;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LiveCutEditor)
};
