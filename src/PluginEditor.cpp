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

#include "PluginEditor.h"

namespace
{
constexpr int minEditorWidth = 820;
constexpr int minEditorHeight = 540;
constexpr int maxEditorWidth = 1680;
constexpr int maxEditorHeight = 1100;
constexpr int defaultEditorWidth = 1020;
constexpr int defaultEditorHeight = 640;
constexpr juce::uint32 uiSizeSaveDelayMs = 500;
constexpr juce::uint32 uiPersistenceArmDelayMs = 250;
constexpr const char* uiWidthKey = "editorWidth";
constexpr const char* uiHeightKey = "editorHeight";

juce::PropertiesFile::Options makeUiPropertiesOptions()
{
    juce::PropertiesFile::Options options;
    options.applicationName = "LiveCutUltra";
    options.filenameSuffix = ".settings";
    options.folderName = "LiveCutUltra";
    options.osxLibrarySubFolder = "Application Support";
    options.commonToAllUsers = false;
    options.storageFormat = juce::PropertiesFile::storeAsXML;
    options.millisecondsBeforeSaving = 0;
    return options;
}

juce::Point<int> loadGlobalUiSize()
{
    juce::PropertiesFile properties (makeUiPropertiesOptions());
    if (! properties.isValidFile())
        return {defaultEditorWidth, defaultEditorHeight};

    const auto width = juce::jlimit (minEditorWidth, maxEditorWidth,
                                     properties.getIntValue (uiWidthKey, defaultEditorWidth));
    const auto height = juce::jlimit (minEditorHeight, maxEditorHeight,
                                      properties.getIntValue (uiHeightKey, defaultEditorHeight));
    return {width, height};
}

void saveGlobalUiSize (int width, int height)
{
    juce::PropertiesFile properties (makeUiPropertiesOptions());
    if (! properties.isValidFile())
        return;

    properties.setValue (uiWidthKey, width);
    properties.setValue (uiHeightKey, height);
    properties.saveIfNeeded();
}
} // namespace

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

    modulationDestination = Modulation::destinationForParameterID (paramID);
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
        button->setComponentID ("cutProcTab");
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
ModMatrixRow::ModMatrixRow (juce::AudioProcessorValueTreeState& apvts, int slotIndex)
    : slot (slotIndex)
{
    enabled.setButtonText ({});
    enabled.setColour (juce::TextButton::buttonOnColourId, Colors::accent);
    addAndMakeVisible (enabled);

    source.addItemList (Modulation::sourceChoices(), 1);
    source.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (source);

    destination.addItemList (Modulation::destinationChoices(), 1);
    destination.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (destination);

    depth.setSliderStyle (juce::Slider::LinearHorizontal);
    depth.setComponentID ("modDepthSlider");
    depth.setColour (juce::Slider::trackColourId, Colors::accent);
    depth.setColour (juce::Slider::thumbColourId, Colors::accent);
    depth.setTextBoxStyle (juce::Slider::TextBoxRight, false, 72, 24);
    depth.setTextValueSuffix (" %");
    addAndMakeVisible (depth);

    enabledAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
        apvts, ParamIDs::modEnabled (slot), enabled);
    sourceAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
        apvts, ParamIDs::modSource (slot), source);
    destinationAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
        apvts, ParamIDs::modDestination (slot), destination);
    depthAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        apvts, ParamIDs::modDepth (slot), depth);

    enabledParameter = apvts.getParameter (ParamIDs::modEnabled (slot));

    // Choosing a real source is an explicit routing action, so make the slot live
    // immediately. Returning the source to Off also turns the route back off.
    // The parameter write goes through the host so automation/state remain correct.
    source.onChange = [this]
    {
        const auto shouldEnable = source.getSelectedItemIndex() > 0;
        enabled.setToggleState (shouldEnable, juce::dontSendNotification);

        if (enabledParameter != nullptr)
        {
            const auto wanted = shouldEnable ? 1.f : 0.f;

            if (std::abs (enabledParameter->getValue() - wanted) > 0.001f)
            {
                enabledParameter->beginChangeGesture();
                enabledParameter->setValueNotifyingHost (wanted);
                enabledParameter->endChangeGesture();
            }
        }

        refreshVisualState();
    };

    destination.onChange = [this] { refreshVisualState(); };
    depth.onValueChange = [this] { refreshVisualState(); };
    enabled.onClick = [this] { refreshVisualState(); };

    refreshVisualState();
}

void ModMatrixRow::refreshVisualState()
{
    const auto active = enabled.getToggleState()
        && source.getSelectedItemIndex() > 0
        && destination.getSelectedItemIndex() > 0
        && std::abs (depth.getValue()) > 0.001;

    depth.getProperties().set ("modRouteActive", active);
    depth.repaint();
}

void ModMatrixRow::setHelpMode (bool helpEnabled)
{
    enabled.setTooltip (helpEnabled
        ? juce::String ("Enable modulation slot ") + juce::String (slot + 1)
        : juce::String());
    source.setTooltip (helpEnabled
        ? "Choose the modulation source. Selecting any source other than Off also enables this slot."
        : juce::String());
    destination.setTooltip (helpEnabled
        ? "Choose the LiveCut parameter controlled by this modulation route."
        : juce::String());
    depth.setTooltip (helpEnabled
        ? "Signed modulation depth. Positive values follow the source; negative values invert it."
        : juce::String());
}

void ModMatrixRow::paint (juce::Graphics& g)
{
    if ((slot & 1) != 0)
    {
        g.setColour (Colors::knobTrack.withAlpha (0.12f));
        g.fillRoundedRectangle (getLocalBounds().toFloat(), 4.f);
    }

    g.setColour (Colors::textDim);
    g.setFont (juce::Font (juce::FontOptions (13.f, juce::Font::bold)));
    g.drawText (juce::String (slot + 1).paddedLeft ('0', 2), 4, 0, 32, getHeight(),
                juce::Justification::centred, false);
}

void ModMatrixRow::resized()
{
    const auto compact = getHeight() < 38;
    const auto padY = compact ? 2 : 4;
    auto r = getLocalBounds().reduced (4, padY);
    r.removeFromLeft (compact ? 32 : 38);

    const auto toggleWidth = compact ? 30 : 38;
    const auto toggleSize = juce::jmin (compact ? 24 : 28, r.getHeight());
    enabled.setBounds (r.removeFromLeft (toggleWidth).withSizeKeepingCentre (toggleSize, toggleSize));
    r.removeFromLeft (compact ? 4 : 6);

    const auto depthWidth = juce::jlimit (160, 270, r.getWidth() / 3);
    auto depthArea = r.removeFromRight (depthWidth);
    r.removeFromRight (compact ? 6 : 8);

    const auto sourceWidth = juce::jlimit (116, 190, r.getWidth() / 3);
    source.setBounds (r.removeFromLeft (sourceWidth));
    r.removeFromLeft (compact ? 6 : 8);
    destination.setBounds (r);
    depth.setBounds (depthArea);
}

ModMatrixPage::ModMatrixPage (juce::AudioProcessorValueTreeState& apvts)
{
    viewport.setViewedComponent (&content, false);
    viewport.setScrollBarsShown (true, false);
    viewport.setScrollBarThickness (10);
    addAndMakeVisible (viewport);

    for (int i = 0; i < Modulation::numSlots; ++i)
    {
        auto row = std::make_unique<ModMatrixRow> (apvts, i);
        content.addAndMakeVisible (*row);
        rows.push_back (std::move (row));
    }
}

void ModMatrixPage::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat().reduced (0.5f);
    g.setColour (Colors::panel);
    g.fillRoundedRectangle (bounds, 10.f);
    g.setColour (Colors::panelOutline);
    g.drawRoundedRectangle (bounds, 10.f, 1.f);

    auto header = getLocalBounds().reduced (12, 0).removeFromTop (28);
    g.setColour (Colors::accent);
    g.fillRoundedRectangle (header.removeFromLeft (8).withSizeKeepingCentre (8, 8).toFloat(), 2.f);
    header.removeFromLeft (8);
    g.setColour (Colors::textDim);
    g.setFont (juce::Font (juce::FontOptions (13.5f, juce::Font::bold)));
    g.drawText ("16-SLOT MODULATION MATRIX", header, juce::Justification::centredLeft, false);
}

void ModMatrixPage::resized()
{
    auto r = getLocalBounds().reduced (8);
    r.removeFromTop (28);
    viewport.setBounds (r);

    const auto contentWidth = juce::jmax (720, viewport.getWidth() - 2);

    // Fit all sixteen rows when the host gives us enough vertical space.
    // This is especially useful on 1080p displays running 125% DPI scaling.
    const auto availablePerRow = juce::jmax (1, viewport.getHeight() / Modulation::numSlots);
    const auto rowHeight = juce::jlimit (30, 42, availablePerRow);
    const auto contentHeight = juce::jmax (viewport.getHeight(), rowHeight * Modulation::numSlots);
    content.setSize (contentWidth, contentHeight);

    auto rowArea = content.getLocalBounds();
    for (auto& row : rows)
        row->setBounds (rowArea.removeFromTop (rowHeight));
}

void ModMatrixPage::refreshVisuals()
{
    for (auto& row : rows)
        row->refreshVisualState();
}

void ModMatrixPage::setHelpMode (bool helpEnabled)
{
    for (auto& row : rows)
        row->setHelpMode (helpEnabled);
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
      combMaxDelay (p.apvts, ParamIDs::combmaxdelay, "Max Delay", " ms", Colors::accentFx, 1),
      matrixPage (p.apvts)
{
    setLookAndFeel (&lookAndFeel);

    addAndMakeVisible (procSelector);

    auto setupPageButton = [this] (juce::TextButton& button, const juce::String& componentID)
    {
        button.setComponentID (componentID);
        button.setClickingTogglesState (false);
        button.setColour (juce::TextButton::buttonOnColourId, Colors::accent);
        addAndMakeVisible (button);
    };

    setupPageButton (mainPageButton, "pageTab");
    setupPageButton (modulationPageButton, "pageTab");
    setupPageButton (lfoPageButton, "modSubTab");
    setupPageButton (wanderPageButton, "modSubTab");
    setupPageButton (matrixPageButton, "modSubTab");

    mainPageButton.onClick = [this] { setMainPage (0); };
    modulationPageButton.onClick = [this] { setMainPage (1); };
    lfoPageButton.onClick = [this] { setModPage (0); };
    wanderPageButton.onClick = [this] { setModPage (1); };
    matrixPageButton.onClick = [this] { setModPage (2); };


    bypassButton.setClickingTogglesState (true);
    addAndMakeVisible (bypassButton);
    bypassAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
        processor.apvts, ParamIDs::bypass, bypassButton);

    helpButton.setClickingTogglesState (true);
    helpButton.onClick = [this] { applyHints(); };
    addAndMakeVisible (helpButton);

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

    modulationKnobs = {
        &seed, &fade, &minPhrase, &maxPhrase, &duty, &fillDuty,
        &minAmp, &maxAmp, &minPan, &maxPan, &minPitch, &maxPitch,
        &minRepeat, &maxRepeat, &stutter, &area,
        &straight, &regular, &ritard, &speed, &activity,
        &minBits, &maxBits, &minFreq, &maxFreq,
        &combFeedback, &combMinDelay, &combMaxDelay
    };

    createModulationSourcePanels();
    addChildComponent (matrixPage);

    addChildComponent (aboutOverlay);

    setHints();
    setMainPage (0);
    setModPage (0);
    startTimerHz (30);

    setResizable (true, true);
    setResizeLimits (minEditorWidth, minEditorHeight, maxEditorWidth, maxEditorHeight);

    auto savedSize = loadGlobalUiSize();
    int instanceWidth = 0, instanceHeight = 0;
    if (processor.getLastEditorSize (instanceWidth, instanceHeight))
    {
        savedSize.x = juce::jlimit (minEditorWidth, maxEditorWidth, instanceWidth);
        savedSize.y = juce::jlimit (minEditorHeight, maxEditorHeight, instanceHeight);
    }

    preferredUiWidth = savedSize.x;
    preferredUiHeight = savedSize.y;
    editorOpenTime = juce::Time::getMillisecondCounter();

    setSize (preferredUiWidth, preferredUiHeight);
    processor.setLastEditorSize (preferredUiWidth, preferredUiHeight);
}

LiveCutEditor::~LiveCutEditor()
{
    if (uiSizeDirty)
        saveUiSize();

    setLookAndFeel (nullptr);
}

void LiveCutEditor::createModulationSourcePanels()
{
    auto addCell = [] (SourcePanelSet& set, std::unique_ptr<Cell> cell) -> Cell*
    {
        auto* raw = cell.get();
        set.panel->addCell (*raw);
        set.cells.push_back (std::move (cell));
        return raw;
    };

    for (int i = 0; i < Modulation::numLfos; ++i)
    {
        SourcePanelSet set;
        set.panel = std::make_unique<Panel> (juce::String ("LFO ") + juce::String (i + 1), Colors::accentProc);
        set.panel->setCompactCellLayout (true);
        set.panel->setActivityProvider ([this, i]
        {
            return processor.getModulationSourceDisplay (i + 1);
        });
        addCell (set, std::make_unique<LabeledCombo> (processor.apvts, ParamIDs::lfoWave (i), "Wave"));
        set.rateCell = addCell (set, std::make_unique<Knob> (processor.apvts, ParamIDs::lfoRate (i), "Rate", " Hz", Colors::accentProc, 2));
        addCell (set, std::make_unique<PowerCell> (processor.apvts, ParamIDs::lfoSync (i), "Sync", Colors::accentProc));
        set.divisionCell = addCell (set, std::make_unique<LabeledCombo> (processor.apvts, ParamIDs::lfoDivision (i), "Division"));
        set.syncValue = processor.apvts.getRawParameterValue (ParamIDs::lfoSync (i));
        addCell (set, std::make_unique<Knob> (processor.apvts, ParamIDs::lfoPhase (i), "Phase", " deg", Colors::accentProc));
        addCell (set, std::make_unique<PowerCell> (processor.apvts, ParamIDs::lfoUnipolar (i), "Uni", Colors::accentProc));
        addCell (set, std::make_unique<PowerCell> (
            processor.apvts, ParamIDs::lfoRetrigger (i), "Retrig", Colors::accentProc));
        addCell (set, std::make_unique<Knob> (processor.apvts, ParamIDs::lfoFade (i), "Fade In", " s", Colors::accentProc, 2));
        addChildComponent (*set.panel);
        lfoPanels.push_back (std::move (set));
    }

    for (int i = 0; i < Modulation::numWanders; ++i)
    {
        SourcePanelSet set;
        set.panel = std::make_unique<Panel> (juce::String ("Wander ") + juce::String (i + 1), Colors::accentFx);
        set.panel->setCompactCellLayout (true);
        set.panel->setActivityProvider ([this, i]
        {
            return processor.getModulationSourceDisplay (Modulation::numLfos + i + 1);
        });
        set.rateCell = addCell (set, std::make_unique<Knob> (processor.apvts, ParamIDs::wanderRate (i), "Rate", " Hz", Colors::accentFx, 3));
        addCell (set, std::make_unique<PowerCell> (processor.apvts, ParamIDs::wanderSync (i), "Sync", Colors::accentFx));
        set.divisionCell = addCell (set, std::make_unique<LabeledCombo> (processor.apvts, ParamIDs::wanderDivision (i), "Period"));
        set.syncValue = processor.apvts.getRawParameterValue (ParamIDs::wanderSync (i));
        addCell (set, std::make_unique<Knob> (processor.apvts, ParamIDs::wanderFeel (i), "Feel", "%", Colors::accentFx));
        addCell (set, std::make_unique<Knob> (processor.apvts, ParamIDs::wanderSmooth (i), "Smooth", "%", Colors::accentFx));
        addCell (set, std::make_unique<Knob> (processor.apvts, ParamIDs::wanderRange (i), "Range", "%", Colors::accentFx));
        addCell (set, std::make_unique<PowerCell> (processor.apvts, ParamIDs::wanderUnipolar (i), "Uni", Colors::accentFx));
        addCell (set, std::make_unique<PowerCell> (
            processor.apvts, ParamIDs::wanderRetrigger (i), "Retrig", Colors::accentFx));
        addChildComponent (*set.panel);
        wanderPanels.push_back (std::move (set));
    }
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
    auto titleWidth = juce::GlyphArrangement::getStringWidthInt (g.getCurrentFont(), "LIVECUT");
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

    const auto compact = getWidth() < 1000;
    titleArea = header.removeFromLeft (compact ? 150 : 180);

    helpButton.setBounds (header.removeFromRight (30).reduced (0, 9));
    header.removeFromRight (6);
    aboutButton.setBounds (header.removeFromRight (30).reduced (0, 9));
    header.removeFromRight (8);
    bypassButton.setBounds (header.removeFromRight (compact ? 76 : 86).reduced (0, 9));
    header.removeFromRight (8);
    ledArea = header.removeFromRight (16).withSizeKeepingCentre (14, 14);
    header.removeFromRight (10);

    const auto mainWidth = compact ? 72 : 90;
    const auto modWidth = compact ? 104 : 120;
    const auto pageGap = compact ? 6 : 8;
    const auto groupGap = compact ? 14 : 24;
    const auto pageGroupWidth = mainWidth + pageGap + modWidth;
    const auto procWidth = juce::jlimit (180, 340,
        header.getWidth() - groupGap - pageGroupWidth);

    auto navigation = header.withSizeKeepingCentre (
        procWidth + groupGap + pageGroupWidth, 30);

    mainPageButton.setBounds (navigation.removeFromLeft (mainWidth));
    navigation.removeFromLeft (pageGap);
    modulationPageButton.setBounds (navigation.removeFromLeft (modWidth));
    navigation.removeFromLeft (groupGap);
    procSelector.setBounds (navigation.removeFromLeft (procWidth));

    r.removeFromTop (12);
    const auto gap = 12;

    // Main page layout
    auto mainR = r;
    const auto rowHeight = (mainR.getHeight() - 2 * gap) / 3;
    auto rowA = mainR.removeFromTop (rowHeight);
    programPanel.setBounds (rowA);

    mainR.removeFromTop (gap);
    auto rowB = mainR.removeFromTop (rowHeight);
    rangesPanel.setBounds (rowB.removeFromLeft (juce::roundToInt (rowB.getWidth() * 0.62f)));
    rowB.removeFromLeft (gap);
    cutProc11Panel.setBounds (rowB);
    warpCutPanel.setBounds (rowB);
    sqPusherPanel.setBounds (rowB);

    mainR.removeFromTop (gap);
    auto rowC = mainR;
    crusherPanel.setBounds (rowC.removeFromLeft (juce::roundToInt (rowC.getWidth() * 0.52f)));
    rowC.removeFromLeft (gap);
    combPanel.setBounds (rowC);

    // Modulation page layout
    auto modR = r;
    auto subTabs = modR.removeFromTop (36).withSizeKeepingCentre (juce::jmin (420, modR.getWidth()), 32);
    const auto tabWidth = subTabs.getWidth() / 3;
    lfoPageButton.setBounds (subTabs.removeFromLeft (tabWidth));
    wanderPageButton.setBounds (subTabs.removeFromLeft (tabWidth));
    matrixPageButton.setBounds (subTabs);
    modR.removeFromTop (8);

    auto lfoR = modR;
    auto wanderR = modR;
    const auto sourceGap = 8;
    const auto sourceRowHeight = (modR.getHeight() - sourceGap * 3) / 4;

    for (auto& set : lfoPanels)
    {
        set.panel->setBounds (lfoR.removeFromTop (sourceRowHeight));
        lfoR.removeFromTop (sourceGap);
    }

    for (auto& set : wanderPanels)
    {
        set.panel->setBounds (wanderR.removeFromTop (sourceRowHeight));
        wanderR.removeFromTop (sourceGap);
    }

    matrixPage.setBounds (modR);

    if (uiSizePersistenceActive)
    {
        processor.setLastEditorSize (getWidth(), getHeight());
        uiSizeDirty = true;
        lastUiResizeTime = juce::Time::getMillisecondCounter();
    }
}

void LiveCutEditor::saveUiSize()
{
    const auto width = juce::jlimit (minEditorWidth, maxEditorWidth, getWidth());
    const auto height = juce::jlimit (minEditorHeight, maxEditorHeight, getHeight());
    processor.setLastEditorSize (width, height);
    saveGlobalUiSize (width, height);
}

void LiveCutEditor::setMainPage (int page)
{
    currentMainPage = juce::jlimit (0, 1, page);
    updatePageVisibility();
}

void LiveCutEditor::setModPage (int page)
{
    currentModPage = juce::jlimit (0, 2, page);
    updatePageVisibility();
}

void LiveCutEditor::updatePageVisibility()
{
    const auto main = currentMainPage == 0;

    mainPageButton.setToggleState (main, juce::dontSendNotification);
    modulationPageButton.setToggleState (! main, juce::dontSendNotification);

    programPanel.setVisible (main);
    rangesPanel.setVisible (main);
    crusherPanel.setVisible (main);
    combPanel.setVisible (main);

    lfoPageButton.setVisible (! main);
    wanderPageButton.setVisible (! main);
    matrixPageButton.setVisible (! main);

    lfoPageButton.setToggleState (currentModPage == 0, juce::dontSendNotification);
    wanderPageButton.setToggleState (currentModPage == 1, juce::dontSendNotification);
    matrixPageButton.setToggleState (currentModPage == 2, juce::dontSendNotification);

    for (auto& set : lfoPanels)
        set.panel->setVisible (! main && currentModPage == 0);
    for (auto& set : wanderPanels)
        set.panel->setVisible (! main && currentModPage == 1);
    matrixPage.setVisible (! main && currentModPage == 2);

    updateProcVisibility();
}

void LiveCutEditor::setHints()
{
    auto hint = [this] (Cell& cell, const juce::String& text)
    {
        cell.setHint (text);
        hintedCells.push_back (&cell);
    };

    hint (subdiv, "Number of subdivisions per bar - the rhythmic grid Livecut cuts to. "
                  "Available values now run from 2 through 64.");
    hint (seed, "Seed for the random generator. The same seed replays the same sequence of cutting decisions.");
    hint (fade, "Fade in/out applied to every cut, in milliseconds. Longer fades soften clicks at cut boundaries.");
    hint (minPhrase, "Shortest phrase length, in bars. A new cutting pattern is chosen for every phrase.");
    hint (maxPhrase, "Longest phrase length, in bars. A new cutting pattern is chosen for every phrase.");
    hint (duty, "How much of each repeat slot is actually played. Below 100% leaves gaps of silence between repeats.");
    hint (fillDuty, "Same as Duty, but applied to fills towards the end of a phrase.");
    hint (minAmp, "Lower bound for the random loudness applied to each cut.");
    hint (maxAmp, "Upper bound for the random loudness applied to each cut.");
    hint (minPan, "Lower bound for the random stereo position of each cut (-100 = left, +100 = right).");
    hint (maxPan, "Upper bound for the random stereo position of each cut (-100 = left, +100 = right).");
    hint (minPitch, "Lower bound for random pitch shift, in cents.");
    hint (maxPitch, "Upper bound for random pitch shift, in cents.");
    hint (minRepeat, "Fewest times a cut may be repeated within a block.");
    hint (maxRepeat, "Most times a cut may be repeated within a block.");
    hint (stutter, "Chance of a stutter - a burst of very fast repeats.");
    hint (area, "How much of the phrase is open to stutters. Larger values let stutters happen earlier.");
    hint (straight, "Chance of playing a block straight through, without warped repeats.");
    hint (regular, "Chance of regular, evenly spaced repeats.");
    hint (ritard, "Chance that warped repeats slow down instead of accelerating.");
    hint (speed, "Acceleration factor for warped repeats. Lower values give a more extreme speed-up or slow-down.");
    hint (activity, "Density of Squarepusher-style fills.");
    hint (crusherOn, "Enable the bit crusher. Its settings are re-randomised for every cut.");
    hint (minBits, "Lower bound for random bit depth per cut.");
    hint (maxBits, "Upper bound for random bit depth per cut.");
    hint (minFreq, "Lower bound for random sample-rate reduction per cut.");
    hint (maxFreq, "Upper bound for random sample-rate reduction per cut.");
    hint (combOn, "Enable the comb filter. Its delay time is re-randomised for every cut.");
    hint (combType, "FeedFwd mixes the delayed signal once; FeedBack recirculates it.");
    hint (combFeedback, "Amount of signal recirculated through the comb filter.");
    hint (combMinDelay, "Lower bound for random comb delay time per cut.");
    hint (combMaxDelay, "Upper bound for random comb delay time per cut.");

    // Modulation-source controls use the same help-mode gate as the main page.
    // Keep these descriptions on the Cell objects rather than setting JUCE
    // tooltips directly, so nothing appears while ? help mode is off.
    for (auto& set : lfoPanels)
    {
        if (set.cells.size() < 8)
            continue;

        hint (*set.cells[0], "LFO waveform: sine, triangle, rising/falling saw, square, sample-and-hold, or smooth random.");
        hint (*set.cells[1], "Free-running LFO rate in Hz. This control is active when Sync is off.");
        hint (*set.cells[2], "Tempo Sync. When enabled, Division sets the LFO speed from the DAW tempo instead of Rate.");
        hint (*set.cells[3], "Tempo-synchronised LFO division, including straight and triplet values from 1/1 to 1/64.");
        hint (*set.cells[4], "Starting phase in degrees. Retrigger returns the LFO to this phase.");
        hint (*set.cells[5], "Unipolar mode changes the LFO output from -1..+1 to 0..+1.");
        hint (*set.cells[6], "Retrigger latch. Turning it on manually resets this LFO immediately. While on, transport start, DAW loop wraps and playhead jumps reset it to the selected Phase.");
        hint (*set.cells[7], "Fade In ramps the LFO depth from zero to full strength after a retrigger. Zero means immediate full depth.");
    }

    for (auto& set : wanderPanels)
    {
        if (set.cells.size() < 8)
            continue;

        hint (*set.cells[0], "Free-running Wander speed in Hz. This control is active when Sync is off.");
        hint (*set.cells[1], "Tempo Sync. When enabled, Period follows the DAW tempo instead of the free Rate.");
        hint (*set.cells[2], "Tempo-synchronised Wander period, from short note divisions and triplets through long 1-64 bar movements.");
        hint (*set.cells[3], "Feel controls how far each new Wander target may move from the previous target. Higher values make larger excursions.");
        hint (*set.cells[4], "Smooth controls the interpolation between Wander targets. Higher values give gentler, more flowing movement.");
        hint (*set.cells[5], "Range scales the overall Wander output amount before it reaches the modulation matrix.");
        hint (*set.cells[6], "Unipolar mode changes the Wander output from -1..+1 to 0..+1.");
        hint (*set.cells[7], "Retrigger latch. Turning it on manually restarts this Wander source immediately. While on, transport start, DAW loop wraps and playhead jumps restart it.");
    }
}

void LiveCutEditor::applyHints()
{
    const bool on = helpButton.getToggleState();

    for (auto* cell : hintedCells)
        cell->setHintActive (on);

    procSelector.setButtonTooltips (
        on ? juce::StringArray {"Classic BBCut algorithm with repeats and stutters.",
                                "Warped cutting with accelerating or slowing repeat patterns.",
                                "Squarepusher-inspired fill patterns."}
           : juce::StringArray {});

    mainPageButton.setTooltip (on ? "Show the main LiveCut cutting and effects controls." : "");
    modulationPageButton.setTooltip (on ? "Show LFOs, Wander sources and the modulation matrix." : "");
    lfoPageButton.setTooltip (on ? "Four tempo-syncable LFO modulation sources." : "");
    wanderPageButton.setTooltip (on ? "Four stochastic Wander modulation sources." : "");
    matrixPageButton.setTooltip (on ? "Sixteen source/destination modulation routes." : "");
    bypassButton.setTooltip (on ? "Pass the input through unprocessed." : "");
    aboutButton.setTooltip (on ? "About LiveCut Enhanced." : "");
    helpButton.setTooltip (on ? "Help mode is ON. Hover over controls for descriptions; click ? again to hide them." : "");
    matrixPage.setHelpMode (on);
    aboutOverlay.setHelpMode (on);
}

void LiveCutEditor::updateProcVisibility()
{
    const auto proc = juce::roundToInt (
        processor.apvts.getRawParameterValue (ParamIDs::cutproc)->load());
    const auto main = currentMainPage == 0;
    cutProc11Panel.setVisible (main && proc == 0);
    warpCutPanel.setVisible (main && proc == 1);
    sqPusherPanel.setVisible (main && proc == 2);
}

void LiveCutEditor::timerCallback()
{
    updateProcVisibility();

    const auto now = juce::Time::getMillisecondCounter();

    if (! uiSizePersistenceActive && now - editorOpenTime >= uiPersistenceArmDelayMs)
    {
        if (getWidth() != preferredUiWidth || getHeight() != preferredUiHeight)
            setSize (preferredUiWidth, preferredUiHeight);

        processor.setLastEditorSize (preferredUiWidth, preferredUiHeight);
        uiSizePersistenceActive = true;
    }

    if (uiSizeDirty && now - lastUiResizeTime >= uiSizeSaveDelayMs)
    {
        saveUiSize();
        uiSizeDirty = false;
    }

    for (auto* knob : modulationKnobs)
        knob->setModulationAmount (
            processor.getDestinationModulation (knob->getModulationDestination()));

    for (auto& set : lfoPanels)
    {
        const auto synced = set.syncValue != nullptr && set.syncValue->load() > 0.5f;
        if (set.rateCell != nullptr)
            set.rateCell->setEnabled (! synced);
        if (set.divisionCell != nullptr)
            set.divisionCell->setEnabled (synced);
        set.panel->repaint();
    }

    for (auto& set : wanderPanels)
    {
        const auto synced = set.syncValue != nullptr && set.syncValue->load() > 0.5f;
        if (set.rateCell != nullptr)
            set.rateCell->setEnabled (! synced);
        if (set.divisionCell != nullptr)
            set.divisionCell->setEnabled (synced);
        set.panel->repaint();
    }

    matrixPage.refreshVisuals();

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
