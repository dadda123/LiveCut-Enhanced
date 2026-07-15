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

#include "PluginEditor.h"

#include "Params.h"

//==============================================================================
Knob::Knob (juce::AudioProcessorValueTreeState& apvts, const juce::String& paramID,
            const juce::String& displayName, const juce::String& suffix, juce::Colour accent,
            int numDecimals)
    : Cell (displayName)
{
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    slider.setColour (juce::Slider::rotarySliderFillColourId, accent);
    slider.textFromValueFunction = [suffix, numDecimals] (double value)
    {
        auto text = numDecimals > 0 ? juce::String (value, numDecimals)
                                    : juce::String (juce::roundToInt (value));
        return suffix.isEmpty() ? text : text + suffix;
    };
    addAndMakeVisible (slider);

    attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        apvts, paramID, slider);

    if (auto* param = apvts.getParameter (paramID))
        slider.setDoubleClickReturnValue (
            true, param->getNormalisableRange().convertFrom0to1 (param->getDefaultValue()));
    slider.updateText();
}

//==============================================================================
LabeledCombo::LabeledCombo (juce::AudioProcessorValueTreeState& apvts,
                            const juce::String& paramID, const juce::String& displayName)
    : Cell (displayName)
{
    if (auto* choice = dynamic_cast<juce::AudioParameterChoice*> (apvts.getParameter (paramID)))
        box.addItemList (choice->choices, 1);
    box.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (box);
    attachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
        apvts, paramID, box);
}

//==============================================================================
PowerCell::PowerCell (juce::AudioProcessorValueTreeState& apvts, const juce::String& paramID,
                      const juce::String& displayName, juce::Colour accent)
    : Cell (displayName)
{
    button.setButtonText ({});
    button.setColour (juce::TextButton::buttonOnColourId, accent);
    addAndMakeVisible (button);
    attachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
        apvts, paramID, button);
}

//==============================================================================
SegmentedControl::SegmentedControl (juce::RangedAudioParameter& param,
                                    const juce::StringArray& labels)
    : attachment (param,
                  [this] (float value)
                  {
                      const auto index = juce::roundToInt (value);
                      for (int i = 0; i < int (buttons.size()); ++i)
                          buttons[size_t (i)]->setToggleState (i == index,
                                                               juce::dontSendNotification);
                  })
{
    for (int i = 0; i < labels.size(); ++i)
    {
        auto button = std::make_unique<juce::TextButton> (labels[i]);
        button->setClickingTogglesState (false);
        button->setConnectedEdges (
            (i > 0 ? juce::Button::ConnectedOnLeft : 0)
            | (i < labels.size() - 1 ? juce::Button::ConnectedOnRight : 0));
        button->onClick = [this, i] { attachment.setValueAsCompleteGesture (float (i)); };
        addAndMakeVisible (*button);
        buttons.push_back (std::move (button));
    }
    attachment.sendInitialUpdate();
}

//==============================================================================
LiveCutEditor::LiveCutEditor (LiveCutAudioProcessor& p)
    : AudioProcessorEditor (p),
      processor (p),
      procSelector (*p.apvts.getParameter (ParamIDs::cutproc),
                    {"CutProc11", "WarpCut", "SQPusher"}),
      subdiv (p.apvts, ParamIDs::subdiv, "SubDiv"),
      seed (p.apvts, ParamIDs::seed, "Seed", "", Colors::accent),
      fade (p.apvts, ParamIDs::fade, "Fade", " ms", Colors::accent, 1),
      minPhrase (p.apvts, ParamIDs::minphrase, "Min Phrase", "", Colors::accent),
      maxPhrase (p.apvts, ParamIDs::maxphrase, "Max Phrase", "", Colors::accent),
      duty (p.apvts, ParamIDs::duty, "Duty", "%", Colors::accent),
      fillDuty (p.apvts, ParamIDs::fillduty, "Fill Duty", "%", Colors::accent),
      minAmp (p.apvts, ParamIDs::minamp, "Min Amp", "%", Colors::accent),
      maxAmp (p.apvts, ParamIDs::maxamp, "Max Amp", "%", Colors::accent),
      minPan (p.apvts, ParamIDs::minpan, "Min Pan", "", Colors::accent),
      maxPan (p.apvts, ParamIDs::maxpan, "Max Pan", "", Colors::accent),
      minPitch (p.apvts, ParamIDs::minpitch, "Min Pitch", "", Colors::accent),
      maxPitch (p.apvts, ParamIDs::maxpitch, "Max Pitch", "", Colors::accent),
      minRepeat (p.apvts, ParamIDs::minrepeat, "Min Repeat", "", Colors::accentProc),
      maxRepeat (p.apvts, ParamIDs::maxrepeat, "Max Repeat", "", Colors::accentProc),
      stutter (p.apvts, ParamIDs::stutter, "Stutter", "%", Colors::accentProc),
      area (p.apvts, ParamIDs::area, "Area", "%", Colors::accentProc),
      straight (p.apvts, ParamIDs::straight, "Straight", "%", Colors::accentProc),
      regular (p.apvts, ParamIDs::regular, "Regular", "%", Colors::accentProc),
      ritard (p.apvts, ParamIDs::ritard, "Ritard", "%", Colors::accentProc),
      speed (p.apvts, ParamIDs::speed, "Speed", "", Colors::accentProc, 3),
      activity (p.apvts, ParamIDs::activity, "Activity", "%", Colors::accentProc),
      crusherOn (p.apvts, ParamIDs::crusher, "On", Colors::accentFx),
      minBits (p.apvts, ParamIDs::minbits, "Min Bits", "", Colors::accentFx),
      maxBits (p.apvts, ParamIDs::maxbits, "Max Bits", "", Colors::accentFx),
      minFreq (p.apvts, ParamIDs::minfreq, "Min Freq", "%", Colors::accentFx),
      maxFreq (p.apvts, ParamIDs::maxfreq, "Max Freq", "%", Colors::accentFx),
      combOn (p.apvts, ParamIDs::comb, "On", Colors::accentFx),
      combType (p.apvts, ParamIDs::combtype, "Type"),
      combFeedback (p.apvts, ParamIDs::combfeedback, "Feedback", "%", Colors::accentFx),
      combMinDelay (p.apvts, ParamIDs::combmindelay, "Min Delay", " ms", Colors::accentFx, 1),
      combMaxDelay (p.apvts, ParamIDs::combmaxdelay, "Max Delay", " ms", Colors::accentFx, 1)
{
    setLookAndFeel (&lookAndFeel);

    addAndMakeVisible (procSelector);

    bypassButton.setClickingTogglesState (true);
    addAndMakeVisible (bypassButton);
    bypassAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
        processor.apvts, ParamIDs::bypass, bypassButton);

    helpButton.setClickingTogglesState (true);
    helpButton.setTooltip ("Help mode: hover over any control to see what it does.");
    helpButton.onClick = [this] { applyHints(); };
    addAndMakeVisible (helpButton);

    aboutButton.setTooltip ("About Livecut");
    aboutButton.onClick = [this] { aboutOverlay.show(); };
    addAndMakeVisible (aboutButton);

    for (auto* cell : {(Cell*) &subdiv, (Cell*) &seed, (Cell*) &fade, (Cell*) &minPhrase,
                       (Cell*) &maxPhrase, (Cell*) &duty, (Cell*) &fillDuty})
        programPanel.addCell (*cell);
    addAndMakeVisible (programPanel);

    for (auto* cell : {&minAmp, &maxAmp, &minPan, &maxPan, &minPitch, &maxPitch})
        rangesPanel.addCell (*cell);
    addAndMakeVisible (rangesPanel);

    for (auto* cell : {&minRepeat, &maxRepeat, &stutter, &area})
        cutProc11Panel.addCell (*cell);
    addAndMakeVisible (cutProc11Panel);

    for (auto* cell : {&straight, &regular, &ritard, &speed})
        warpCutPanel.addCell (*cell);
    addChildComponent (warpCutPanel);

    sqPusherPanel.addCell (activity);
    addChildComponent (sqPusherPanel);

    crusherPanel.addCell (crusherOn);
    for (auto* cell : {&minBits, &maxBits, &minFreq, &maxFreq})
        crusherPanel.addCell (*cell);
    addAndMakeVisible (crusherPanel);

    combPanel.addCell (combOn);
    combPanel.addCell (combType);
    for (auto* cell : {&combFeedback, &combMinDelay, &combMaxDelay})
        combPanel.addCell (*cell);
    addAndMakeVisible (combPanel);

    addChildComponent (aboutOverlay); // added last so it covers everything when shown

    setHints();
    updateProcVisibility();
    startTimerHz (30);

    setResizable (true, true);
    setResizeLimits (820, 540, 1680, 1100);
    setSize (1020, 640);
}

LiveCutEditor::~LiveCutEditor()
{
    setLookAndFeel (nullptr);
}

void LiveCutEditor::paint (juce::Graphics& g)
{
    g.fillAll (Colors::background);

    juce::ColourGradient glow (Colors::accent.withAlpha (0.06f), float (getWidth()) / 2.f, 0.f,
                               juce::Colours::transparentBlack, float (getWidth()) / 2.f,
                               float (getHeight()) * 0.5f, false);
    g.setGradientFill (glow);
    g.fillRect (getLocalBounds());

    auto title = titleArea;
    g.setColour (Colors::text);
    g.setFont (juce::Font (juce::FontOptions (27.f, juce::Font::bold)));
    auto titleWidth =
        juce::GlyphArrangement::getStringWidthInt (g.getCurrentFont(), "LIVECUT");
    g.drawText ("LIVECUT", title.removeFromLeft (titleWidth + 4),
                juce::Justification::centredLeft, false);
    title.removeFromLeft (8);
    g.setColour (Colors::accent);
    g.fillEllipse (title.removeFromLeft (8).withSizeKeepingCentre (7, 7).toFloat());

    auto led = ledArea.toFloat();
    g.setColour (Colors::knobTrack);
    g.fillEllipse (led);
    if (ledLevel > 0.01f)
    {
        g.setColour (Colors::led.withAlpha (ledLevel));
        g.fillEllipse (led.reduced (2.f));
    }
}

void LiveCutEditor::resized()
{
    aboutOverlay.setBounds (getLocalBounds());

    auto r = getLocalBounds().reduced (16);

    auto header = r.removeFromTop (48);
    titleArea = header.removeFromLeft (200);
    helpButton.setBounds (header.removeFromRight (30).reduced (0, 9));
    header.removeFromRight (6);
    aboutButton.setBounds (header.removeFromRight (30).reduced (0, 9));
    header.removeFromRight (8);
    bypassButton.setBounds (header.removeFromRight (86).reduced (0, 9));
    header.removeFromRight (10);
    ledArea = header.removeFromRight (16).withSizeKeepingCentre (14, 14);
    header.removeFromRight (14);
    procSelector.setBounds (
        header.withSizeKeepingCentre (juce::jmin (header.getWidth(), 340), 30));

    r.removeFromTop (12);
    const auto gap = 12;
    const auto rowHeight = (r.getHeight() - 2 * gap) / 3;

    auto rowA = r.removeFromTop (rowHeight);
    programPanel.setBounds (rowA);

    r.removeFromTop (gap);
    auto rowB = r.removeFromTop (rowHeight);
    rangesPanel.setBounds (rowB.removeFromLeft (juce::roundToInt (rowB.getWidth() * 0.62f)));
    rowB.removeFromLeft (gap);
    cutProc11Panel.setBounds (rowB);
    warpCutPanel.setBounds (rowB);
    sqPusherPanel.setBounds (rowB);

    r.removeFromTop (gap);
    auto rowC = r;
    crusherPanel.setBounds (rowC.removeFromLeft (juce::roundToInt (rowC.getWidth() * 0.52f)));
    rowC.removeFromLeft (gap);
    combPanel.setBounds (rowC);
}

void LiveCutEditor::setHints()
{
    auto hint = [this] (Cell& cell, const juce::String& text)
    {
        cell.setHint (text);
        hintedCells.push_back (&cell);
    };

    hint (subdiv, "Number of subdivisions per bar - the rhythmic grid Livecut cuts to. "
                  "Higher values slice the audio into finer pieces.");
    hint (seed, "Seed for the random generator. The same seed replays the same sequence "
                "of cutting decisions.");
    hint (fade, "Fade in/out applied to every cut, in milliseconds. Longer fades soften "
                "the clicks at cut boundaries.");
    hint (minPhrase, "Shortest phrase length, in bars. A new cutting pattern is chosen "
                     "for every phrase.");
    hint (maxPhrase, "Longest phrase length, in bars. A new cutting pattern is chosen "
                     "for every phrase.");
    hint (duty, "How much of each repeat slot is actually played. Below 100% leaves gaps "
                "of silence between repeats, like a gate.");
    hint (fillDuty, "Same as Duty, but applied to fills - the ornament cuts placed "
                    "towards the end of a phrase.");

    hint (minAmp, "Lower bound for the random loudness applied to each cut.");
    hint (maxAmp, "Upper bound for the random loudness applied to each cut.");
    hint (minPan, "Lower bound for the random stereo position of each cut "
                  "(-100 = left, +100 = right).");
    hint (maxPan, "Upper bound for the random stereo position of each cut "
                  "(-100 = left, +100 = right).");
    hint (minPitch, "Lower bound for the random pitch shift of each cut, in cents "
                    "(100 cents = 1 semitone).");
    hint (maxPitch, "Upper bound for the random pitch shift of each cut, in cents "
                    "(100 cents = 1 semitone).");

    hint (minRepeat, "Fewest times a cut may be repeated within a block.");
    hint (maxRepeat, "Most times a cut may be repeated within a block.");
    hint (stutter, "Chance of a stutter - a burst of very fast repeats.");
    hint (area, "How much of the phrase is open to stutters. Larger values let "
                "stutters happen earlier in the phrase.");

    hint (straight, "Chance of playing a block straight through, without warped repeats.");
    hint (regular, "Chance of regular, evenly spaced repeats.");
    hint (ritard, "Chance that warped repeats slow down (ritard) instead of accelerating.");
    hint (speed, "Acceleration factor for warped repeats. Lower values give a more "
                 "extreme speed-up or slow-down.");

    hint (activity, "Density of fills. Higher values trigger Squarepusher-style "
                    "fill patterns more often.");

    hint (crusherOn, "Enable the bit crusher. Its settings are re-randomised for every cut.");
    hint (minBits, "Lower bound for the random bit depth per cut. Fewer bits means "
                   "more distortion.");
    hint (maxBits, "Upper bound for the random bit depth per cut. Fewer bits means "
                   "more distortion.");
    hint (minFreq, "Lower bound for the random sample-rate reduction per cut, as a "
                   "portion of the full sample rate.");
    hint (maxFreq, "Upper bound for the random sample-rate reduction per cut, as a "
                   "portion of the full sample rate.");

    hint (combOn, "Enable the comb filter. Its delay time is re-randomised for every cut.");
    hint (combType, "FeedFwd mixes the delayed signal in once, giving a gentle phasing. "
                    "FeedBack recirculates it, giving a sharper metallic resonance.");
    hint (combFeedback, "Amount of signal recirculated through the comb filter "
                        "(FeedBack type only).");
    hint (combMinDelay, "Lower bound for the random comb delay time per cut, in milliseconds.");
    hint (combMaxDelay, "Upper bound for the random comb delay time per cut, in milliseconds.");
}

void LiveCutEditor::applyHints()
{
    const bool on = helpButton.getToggleState();

    for (auto* cell : hintedCells)
        cell->setHintActive (on);

    procSelector.setButtonTooltips (
        on ? juce::StringArray {"Classic BBCut algorithm: repeats short fragments with "
                                "occasional stutters, in the style of early jungle edits.",
                                "Warped cutting: repeat patterns that accelerate or slow "
                                "down across the block (rolls and ritards).",
                                "Cut patterns modelled on Squarepusher-style drum fills."}
           : juce::StringArray {});

    bypassButton.setTooltip (on ? "Pass the input through unprocessed." : "");
}

void LiveCutEditor::updateProcVisibility()
{
    const auto proc =
        juce::roundToInt (processor.apvts.getRawParameterValue (ParamIDs::cutproc)->load());
    cutProc11Panel.setVisible (proc == 0);
    warpCutPanel.setVisible (proc == 1);
    sqPusherPanel.setVisible (proc == 2);
}

void LiveCutEditor::timerCallback()
{
    updateProcVisibility();

    const auto cuts = processor.getKernel().getCutCount();
    if (cuts != lastCutCount)
    {
        lastCutCount = cuts;
        ledLevel = 1.f;
    }
    else
    {
        ledLevel *= 0.82f;
    }
    repaint (ledArea.expanded (2));
}
