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

#include "PluginProcessor.h"

#include "PluginEditor.h"

namespace
{
constexpr const char* editorWidthStateKey = "uiEditorWidth";
constexpr const char* editorHeightStateKey = "uiEditorHeight";
constexpr const char* stateVersionKey = "liveCutUltraStateVersion";
constexpr int currentStateVersion = 7;

float smoothStep (float x) noexcept
{
    x = juce::jlimit (0.f, 1.f, x);
    return x * x * (3.f - 2.f * x);
}
} // namespace

LiveCutAudioProcessor::LiveCutAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createParameterLayout())
{
    bypassParam = dynamic_cast<juce::AudioParameterBool*> (apvts.getParameter (ParamIDs::bypass));

    auto& k = kernel;
    bindParameter (ParamIDs::cutproc, [&k] (float v) { k.setCutProc (int (v)); });
    bindParameter (ParamIDs::subdiv, [&k] (float v)
    {
        static constexpr int subdivs[] = {2, 4, 6, 8, 12, 16, 18, 24, 32, 64};
        k.setSubDiv (subdivs[juce::jlimit (0, 9, int (v))]);
    });
    bindParameter (ParamIDs::seed, [&k] (float v) { k.setSeed (int (v)); });
    bindParameter (ParamIDs::fade, [&k] (float v) { k.setFade (v); });
    bindParameter (ParamIDs::minamp, [&k] (float v) { k.setMinAmp (v / 100.f); });
    bindParameter (ParamIDs::maxamp, [&k] (float v) { k.setMaxAmp (v / 100.f); });
    bindParameter (ParamIDs::minpan, [&k] (float v) { k.setMinPan (v / 100.f); });
    bindParameter (ParamIDs::maxpan, [&k] (float v) { k.setMaxPan (v / 100.f); });
    bindParameter (ParamIDs::minpitch, [&k] (float v) { k.setMinPitch (v); });
    bindParameter (ParamIDs::maxpitch, [&k] (float v) { k.setMaxPitch (v); });
    bindParameter (ParamIDs::duty, [&k] (float v) { k.setDuty (v / 100.f); });
    bindParameter (ParamIDs::fillduty, [&k] (float v) { k.setFillDuty (v / 100.f); });
    bindParameter (ParamIDs::minphrase, [&k] (float v) { k.setMinPhrase (int (v)); });
    bindParameter (ParamIDs::maxphrase, [&k] (float v) { k.setMaxPhrase (int (v)); });
    bindParameter (ParamIDs::minrepeat, [&k] (float v) { k.setMinRepeat (int (v)); });
    bindParameter (ParamIDs::maxrepeat, [&k] (float v) { k.setMaxRepeat (int (v)); });
    bindParameter (ParamIDs::stutter, [&k] (float v) { k.setStutter (v / 100.f); });
    bindParameter (ParamIDs::area, [&k] (float v) { k.setArea (v / 100.f); });
    bindParameter (ParamIDs::straight, [&k] (float v) { k.setStraight (v / 100.f); });
    bindParameter (ParamIDs::regular, [&k] (float v) { k.setRegular (v / 100.f); });
    bindParameter (ParamIDs::ritard, [&k] (float v) { k.setRitard (v / 100.f); });
    bindParameter (ParamIDs::speed, [&k] (float v) { k.setSpeed (v); });
    bindParameter (ParamIDs::activity, [&k] (float v) { k.setActivity (v / 100.f); });
    bindParameter (ParamIDs::crusher, [&k] (float v) { k.setBitcrusher (v > 0.5f); });
    bindParameter (ParamIDs::minbits, [&k] (float v) { k.setMinBits (int (v)); });
    bindParameter (ParamIDs::maxbits, [&k] (float v) { k.setMaxBits (int (v)); });
    bindParameter (ParamIDs::minfreq, [&k] (float v) { k.setMinFreq (v / 100.f); });
    bindParameter (ParamIDs::maxfreq, [&k] (float v) { k.setMaxFreq (v / 100.f); });
    bindParameter (ParamIDs::comb, [&k] (float v) { k.setComb (v > 0.5f); });
    bindParameter (ParamIDs::combtype, [&k] (float v) { k.setCombType (v > 0.5f); });
    bindParameter (ParamIDs::combfeedback, [&k] (float v) { k.setCombFeedback (v / 100.f); });
    bindParameter (ParamIDs::combmindelay, [&k] (float v) { k.setCombMinDelay (v); });
    bindParameter (ParamIDs::combmaxdelay, [&k] (float v) { k.setCombMaxDelay (v); });

    for (int i = 0; i < Modulation::numLfos; ++i)
    {
        auto& refs = lfoParams[size_t (i)];
        refs.wave = apvts.getRawParameterValue (ParamIDs::lfoWave (i));
        refs.rate = apvts.getRawParameterValue (ParamIDs::lfoRate (i));
        refs.sync = apvts.getRawParameterValue (ParamIDs::lfoSync (i));
        refs.division = apvts.getRawParameterValue (ParamIDs::lfoDivision (i));
        refs.phase = apvts.getRawParameterValue (ParamIDs::lfoPhase (i));
        refs.unipolar = apvts.getRawParameterValue (ParamIDs::lfoUnipolar (i));
        refs.retrigger = apvts.getRawParameterValue (ParamIDs::lfoRetrigger (i));
        refs.fade = apvts.getRawParameterValue (ParamIDs::lfoFade (i));
    }

    for (int i = 0; i < Modulation::numWanders; ++i)
    {
        auto& refs = wanderParams[size_t (i)];
        refs.rate = apvts.getRawParameterValue (ParamIDs::wanderRate (i));
        refs.sync = apvts.getRawParameterValue (ParamIDs::wanderSync (i));
        refs.division = apvts.getRawParameterValue (ParamIDs::wanderDivision (i));
        refs.feel = apvts.getRawParameterValue (ParamIDs::wanderFeel (i));
        refs.smooth = apvts.getRawParameterValue (ParamIDs::wanderSmooth (i));
        refs.range = apvts.getRawParameterValue (ParamIDs::wanderRange (i));
        refs.unipolar = apvts.getRawParameterValue (ParamIDs::wanderUnipolar (i));
        refs.retrigger = apvts.getRawParameterValue (ParamIDs::wanderRetrigger (i));
    }

    // Listen specifically for OFF -> ON retrigger parameter changes.  This creates
    // an explicit audio-thread retrigger request instead of relying on polling a
    // boolean edge at block boundaries.  It also works for host automation as well
    // as clicks in the editor.
    for (int i = 0; i < Modulation::numLfos; ++i)
        apvts.addParameterListener (ParamIDs::lfoRetrigger (i), this);
    for (int i = 0; i < Modulation::numWanders; ++i)
        apvts.addParameterListener (ParamIDs::wanderRetrigger (i), this);

    for (int i = 0; i < Modulation::numSlots; ++i)
    {
        auto& refs = modSlotParams[size_t (i)];
        refs.enabled = apvts.getRawParameterValue (ParamIDs::modEnabled (i));
        refs.source = apvts.getRawParameterValue (ParamIDs::modSource (i));
        refs.destination = apvts.getRawParameterValue (ParamIDs::modDestination (i));
        refs.depth = apvts.getRawParameterValue (ParamIDs::modDepth (i));
    }

    for (int destination = 1; destination < Modulation::numDestinations; ++destination)
        if (const auto* id = Modulation::destinationParameterID (destination))
            destinationParameters[size_t (destination)] = apvts.getParameter (id);

    for (auto& value : lfoDisplayValues)
        value.store (0.f, std::memory_order_relaxed);
    for (auto& value : wanderDisplayValues)
        value.store (0.f, std::memory_order_relaxed);
    for (auto& value : destinationDisplayOffsets)
        value.store (0.f, std::memory_order_relaxed);
}

LiveCutAudioProcessor::~LiveCutAudioProcessor()
{
    for (int i = 0; i < Modulation::numLfos; ++i)
        apvts.removeParameterListener (ParamIDs::lfoRetrigger (i), this);
    for (int i = 0; i < Modulation::numWanders; ++i)
        apvts.removeParameterListener (ParamIDs::wanderRetrigger (i), this);
}

void LiveCutAudioProcessor::parameterChanged (const juce::String& parameterID, float newValue)
{
    // Retrig is a latch for transport behaviour, but an OFF -> ON change is also
    // a manual reset gesture.  APVTS listener callbacks may arrive on different
    // threads, so only publish a bit to the audio thread here.
    if (newValue <= 0.5f)
        return;

    for (int i = 0; i < Modulation::numLfos; ++i)
    {
        if (parameterID == ParamIDs::lfoRetrigger (i))
        {
            pendingManualRetriggers.fetch_or (retriggerBitForLfo (i), std::memory_order_release);
            return;
        }
    }

    for (int i = 0; i < Modulation::numWanders; ++i)
    {
        if (parameterID == ParamIDs::wanderRetrigger (i))
        {
            pendingManualRetriggers.fetch_or (retriggerBitForWander (i), std::memory_order_release);
            return;
        }
    }
}

void LiveCutAudioProcessor::bindParameter (const char* paramID, std::function<void (float)> apply)
{
    auto* source = apvts.getRawParameterValue (paramID);
    jassert (source != nullptr);
    paramPushers.push_back ({source, std::move (apply)});
}

void LiveCutAudioProcessor::pushParameters()
{
    for (auto& p : paramPushers)
    {
        const auto value = p.source->load (std::memory_order_relaxed);
        if (value != p.lastPushed)
        {
            p.lastPushed = value;
            p.apply (value);
        }
    }
}

float LiveCutAudioProcessor::randomBipolar() noexcept
{
    return modulationRandom.nextFloat() * 2.f - 1.f;
}

void LiveCutAudioProcessor::prepareToPlay (double sampleRate, int)
{
    kernel.setSampleRate (sampleRate);

    for (auto& p : paramPushers)
        p.lastPushed = std::numeric_limits<float>::quiet_NaN();
    pushParameters();

    freeRunPpq = 0.0;
    wasPlaying = false;
    expectedModPpq = std::numeric_limits<double>::quiet_NaN();
    lastHostPpq = std::numeric_limits<double>::quiet_NaN();
    expectedHostSamplePosition = std::numeric_limits<std::int64_t>::min();
    pendingManualRetriggers.store (0, std::memory_order_relaxed);
    inputEnvelopeValue = 0.f;
    smoothedDestinationOffsets.fill (0.f);
    lastAppliedModulatedSeed = -1;
    activeMidiNotes.fill (false);

    for (auto& value : destinationDisplayOffsets)
        value.store (0.f, std::memory_order_relaxed);

    for (auto& lfo : lfoStates)
    {
        lfo.phase = 0.0;
        lfo.fadeAgeSeconds = 0.0;
        lfo.syncAnchorPpq = 0.0;
        lfo.lastSyncCycle = -1;
        lfo.syncAnchorValid = false;
        lfo.retriggerWasEnabled = false;
        lfo.sampleHold = randomBipolar();
        lfo.randomA = randomBipolar();
        lfo.randomB = randomBipolar();
    }

    for (auto& wander : wanderStates)
    {
        wander.phase = 0.0;
        wander.current = 0.f;
        wander.target = randomBipolar();
        wander.retriggerWasEnabled = false;
    }
}

bool LiveCutAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainInputChannelSet() == juce::AudioChannelSet::stereo()
        && layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void LiveCutAudioProcessor::handleMidi (const juce::MidiBuffer& midi)
{
    for (const auto metadata : midi)
    {
        const auto message = metadata.getMessage();

        if (message.isController() && message.getControllerNumber() == 1)
            modWheelValue = float (message.getControllerValue()) / 127.f;

        if (message.isNoteOn())
        {
            const auto note = juce::jlimit (0, 127, message.getNoteNumber());
            activeMidiNotes[size_t (note)] = true;
            velocityValue = message.getFloatVelocity();
        }
        else if (message.isNoteOff())
        {
            const auto note = juce::jlimit (0, 127, message.getNoteNumber());
            activeMidiNotes[size_t (note)] = false;

            if (std::none_of (activeMidiNotes.begin(), activeMidiNotes.end(), [] (bool active) { return active; }))
                velocityValue = 0.f;
        }

        if (message.isChannelPressure())
            aftertouchValue = float (message.getChannelPressureValue()) / 127.f;
        else if (message.isAftertouch())
            aftertouchValue = float (message.getAfterTouchValue()) / 127.f;
    }
}

void LiveCutAudioProcessor::updateInputEnvelope (const juce::AudioBuffer<float>& buffer,
                                                  int numSamples)
{
    float peak = 0.f;
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        peak = juce::jmax (peak, buffer.getMagnitude (ch, 0, numSamples));

    const auto target = juce::jlimit (0.f, 1.f, peak);
    const auto sr = juce::jmax (1.0, getSampleRate());
    const auto tau = target > inputEnvelopeValue ? 0.010 : 0.180;
    const auto coefficient = std::exp (-double (numSamples) / (sr * tau));
    inputEnvelopeValue = float (coefficient * inputEnvelopeValue + (1.0 - coefficient) * target);
}

double LiveCutAudioProcessor::lfoCyclesPerQuarter (int index) const noexcept
{
    static constexpr double cyclesPerQuarter[] = {
         0.25,  0.375, // 1/1,  1/1T
         0.5,   0.75,  // 1/2,  1/2T
         1.0,   1.5,   // 1/4,  1/4T
         2.0,   3.0,   // 1/8,  1/8T
         4.0,   6.0,   // 1/16, 1/16T
         8.0,  12.0,   // 1/32, 1/32T
        16.0,  24.0    // 1/64, 1/64T
    };
    const auto division = juce::jlimit (0, 13,
        juce::roundToInt (lfoParams[size_t (index)].division->load()));
    return cyclesPerQuarter[division];
}

double LiveCutAudioProcessor::lfoFrequencyHz (int index, double bpm) const noexcept
{
    const auto sync = lfoParams[size_t (index)].sync->load() > 0.5f;
    if (! sync)
        return lfoParams[size_t (index)].rate->load();

    return juce::jmax (1.0, bpm) / 60.0 * lfoCyclesPerQuarter (index);
}

double LiveCutAudioProcessor::wanderFrequencyHz (int index, double bpm, double numerator, double denominator) const noexcept
{
    const auto sync = wanderParams[size_t (index)].sync->load() > 0.5f;
    if (! sync)
        return wanderParams[size_t (index)].rate->load();

    const auto division = juce::jlimit (0, 20,
        juce::roundToInt (wanderParams[size_t (index)].division->load()));

    // Indices 0..13 are note-value divisions (straight then triplet);
    // 14..20 are the original 1/2/4/8/16/32/64-bar periods.
    if (division < 14)
    {
        static constexpr double cyclesPerQuarter[] = {
            16.0, 24.0,   // 1/64, 1/64T
             8.0, 12.0,   // 1/32, 1/32T
             4.0,  6.0,   // 1/16, 1/16T
             2.0,  3.0,   // 1/8,  1/8T
             1.0,  1.5,   // 1/4,  1/4T
             0.5,  0.75,  // 1/2,  1/2T
             0.25, 0.375  // 1/1,  1/1T
        };
        return juce::jmax (1.0, bpm) / 60.0 * cyclesPerQuarter[division];
    }

    static constexpr double bars[] = {1.0, 2.0, 4.0, 8.0, 16.0, 32.0, 64.0};
    const auto barIndex = division - 14;
    const auto quartersPerBar = juce::jmax (0.25, numerator * 4.0 / juce::jmax (1.0, denominator));
    const auto secondsPerBar = (60.0 / juce::jmax (1.0, bpm)) * quartersPerBar;
    return 1.0 / (bars[barIndex] * secondsPerBar);
}

float LiveCutAudioProcessor::evaluateLfo (int index, double phase) noexcept
{
    auto& state = lfoStates[size_t (index)];
    const auto wave = juce::jlimit (0, 6,
        juce::roundToInt (lfoParams[size_t (index)].wave->load()));
    const auto p = float (phase - std::floor (phase));

    switch (wave)
    {
        case 0: return std::sin (juce::MathConstants<float>::twoPi * p);
        case 1: return 1.f - 4.f * std::abs (p - 0.5f);
        case 2: return 2.f * p - 1.f;
        case 3: return 1.f - 2.f * p;
        case 4: return p < 0.5f ? 1.f : -1.f;
        case 5: return state.sampleHold;
        case 6:
        {
            const auto t = smoothStep (p);
            return state.randomA + (state.randomB - state.randomA) * t;
        }
        default: return 0.f;
    }
}

void LiveCutAudioProcessor::updateModulationSources (int numSamples, double bpm,
                                                        double numerator, double denominator,
                                                        double ppqPos, bool gotHostPosition,
                                                        bool hostIsPlaying, std::uint32_t retriggerMask)
{
    const auto sr = juce::jmax (1.0, getSampleRate());
    const auto blockSeconds = double (numSamples) / sr;

    auto resetLfoRandomState = [this] (LfoState& state)
    {
        state.sampleHold = randomBipolar();
        state.randomA = randomBipolar();
        state.randomB = randomBipolar();
        state.lastSyncCycle = -1;
    };

    for (int i = 0; i < Modulation::numLfos; ++i)
    {
        auto& state = lfoStates[size_t (i)];
        const auto sync = lfoParams[size_t (i)].sync->load() > 0.5f;
        const auto retriggerEnabled = lfoParams[size_t (i)].retrigger->load() > 0.5f;
        const auto retriggerJustEnabled = retriggerEnabled && ! state.retriggerWasEnabled;
        state.retriggerWasEnabled = retriggerEnabled;
        const auto requestedRetrigger = (retriggerMask & retriggerBitForLfo (i)) != 0;
        const auto effectiveRetrigger = requestedRetrigger || retriggerJustEnabled;
        const auto hostLocked = sync && retriggerEnabled && gotHostPosition && hostIsPlaying;
        const auto phaseOffset = double (lfoParams[size_t (i)].phase->load()) / 360.0;

        double phaseForWave = 0.0;

        if (hostLocked)
        {
            // Retriggered tempo-sync LFOs use the DAW timeline as their clock.
            // The anchor is reset at transport start and whenever the timeline
            // jumps backwards/forwards (e.g. an arrangement-loop wrap). This
            // avoids accumulated drift and avoids MIDI notes repeatedly resetting
            // slow LFOs on rhythmic material.
            if (effectiveRetrigger || ! state.syncAnchorValid)
            {
                state.syncAnchorPpq = ppqPos;
                state.syncAnchorValid = true;
                state.fadeAgeSeconds = 0.0;
                resetLfoRandomState (state);
            }

            const auto cycles = (ppqPos - state.syncAnchorPpq) * lfoCyclesPerQuarter (i);
            phaseForWave = cycles + phaseOffset;

            const auto cycle = static_cast<std::int64_t> (std::floor (phaseForWave));
            if (cycle != state.lastSyncCycle)
            {
                if (state.lastSyncCycle >= 0)
                {
                    state.sampleHold = randomBipolar();
                    state.randomA = state.randomB;
                    state.randomB = randomBipolar();
                }
                state.lastSyncCycle = cycle;
            }

            // Keep a sensible phase if Sync/Retrig is subsequently switched off.
            state.phase = cycles - std::floor (cycles);
        }
        else
        {
            // Force a fresh host anchor if Sync/Retrig is turned back on later.
            state.syncAnchorValid = false;

            if (effectiveRetrigger && retriggerEnabled)
            {
                state.phase = 0.0;
                state.fadeAgeSeconds = 0.0;
                resetLfoRandomState (state);
            }

            phaseForWave = state.phase + phaseOffset;
        }

        auto value = evaluateLfo (i, phaseForWave);

        const auto fadeSeconds = double (lfoParams[size_t (i)].fade->load());
        const auto fadeGain = fadeSeconds <= 0.0
            ? 1.f
            : juce::jlimit (0.f, 1.f, float (state.fadeAgeSeconds / fadeSeconds));

        const auto unipolar = lfoParams[size_t (i)].unipolar->load() > 0.5f;
        if (unipolar)
            value = (value * 0.5f + 0.5f) * fadeGain;
        else
            value *= fadeGain;

        lfoValues[size_t (i)] = value;
        lfoDisplayValues[size_t (i)].store (value, std::memory_order_relaxed);

        if (hostIsPlaying || ! gotHostPosition)
            state.fadeAgeSeconds += blockSeconds;

        if (! hostLocked)
        {
            const auto increment = lfoFrequencyHz (i, bpm) * blockSeconds;
            const auto oldPhase = state.phase;
            state.phase += increment;

            if (std::floor (state.phase) != std::floor (oldPhase))
            {
                state.sampleHold = randomBipolar();
                state.randomA = state.randomB;
                state.randomB = randomBipolar();
            }
            state.phase -= std::floor (state.phase);
        }
    }

    for (int i = 0; i < Modulation::numWanders; ++i)
    {
        auto& state = wanderStates[size_t (i)];
        const auto retriggerEnabled = wanderParams[size_t (i)].retrigger->load() > 0.5f;
        const auto retriggerJustEnabled = retriggerEnabled && ! state.retriggerWasEnabled;
        state.retriggerWasEnabled = retriggerEnabled;
        const auto requestedRetrigger = (retriggerMask & retriggerBitForWander (i)) != 0;

        if (requestedRetrigger || retriggerJustEnabled)
        {
            state.phase = 0.0;
            state.current = 0.f;
            state.target = randomBipolar();
        }

        const auto smooth = juce::jlimit (0.f, 1.f,
            wanderParams[size_t (i)].smooth->load() / 100.f);
        const auto p = float (state.phase);
        const auto fastShape = juce::jmin (1.f, p * 8.f);
        const auto shaped = fastShape + (smoothStep (p) - fastShape) * smooth;
        auto value = state.current + (state.target - state.current) * shaped;

        const auto range = juce::jlimit (0.f, 1.f,
            wanderParams[size_t (i)].range->load() / 100.f);
        const auto unipolar = wanderParams[size_t (i)].unipolar->load() > 0.5f;
        value = unipolar ? (value * 0.5f + 0.5f) * range : value * range;
        wanderValues[size_t (i)] = value;
        wanderDisplayValues[size_t (i)].store (value, std::memory_order_relaxed);

        const auto increment = wanderFrequencyHz (i, bpm, numerator, denominator) * blockSeconds;
        const auto oldPhase = state.phase;
        state.phase += increment;

        if (std::floor (state.phase) != std::floor (oldPhase))
        {
            state.current = state.target;
            const auto feel = juce::jlimit (0.f, 1.f,
                wanderParams[size_t (i)].feel->load() / 100.f);
            const auto step = 0.12f + 0.88f * feel;
            state.target = juce::jlimit (-1.f, 1.f, state.current + randomBipolar() * step);
        }
        state.phase -= std::floor (state.phase);
    }
}

float LiveCutAudioProcessor::getSourceValue (int sourceIndex) const noexcept
{
    if (sourceIndex >= 1 && sourceIndex <= 4)
        return lfoValues[size_t (sourceIndex - 1)];
    if (sourceIndex >= 5 && sourceIndex <= 8)
        return wanderValues[size_t (sourceIndex - 5)];

    switch (sourceIndex)
    {
        case 9:  return modWheelValue;
        case 10: return velocityValue;
        case 11: return aftertouchValue;
        case 12: return inputEnvelopeValue;
        default: return 0.f;
    }
}

void LiveCutAudioProcessor::applyDestination (int destination, float normalizedOffset)
{
    const auto* parameterID = Modulation::destinationParameterID (destination);
    if (parameterID == nullptr)
        return;

    auto* parameter = destinationParameters[size_t (destination)];
    if (parameter == nullptr)
        return;

    const auto normalized = juce::jlimit (0.f, 1.f, parameter->getValue() + normalizedOffset);
    const auto plain = parameter->convertFrom0to1 (normalized);

    switch (destination)
    {
        case Modulation::destSubdiv:
        {
            static constexpr int subdivs[] = {2, 4, 6, 8, 12, 16, 18, 24, 32, 64};
            const auto index = juce::jlimit (0, 9, juce::roundToInt (plain));
            kernel.setSubDiv (subdivs[index]);
            break;
        }
        case Modulation::destSeed:
        {
            const auto seed = juce::jlimit (1, 16, juce::roundToInt (plain));
            // Re-seeding every audio block would pin the random stream. Only
            // reseed when the quantised modulated Seed value actually changes.
            if (seed != lastAppliedModulatedSeed)
            {
                kernel.setSeed (seed);
                lastAppliedModulatedSeed = seed;
            }
            break;
        }
        case Modulation::destFade:          kernel.setFade (plain); break;
        case Modulation::destMinPhrase:     kernel.setMinPhrase (juce::roundToInt (plain)); break;
        case Modulation::destMaxPhrase:     kernel.setMaxPhrase (juce::roundToInt (plain)); break;
        case Modulation::destDuty:          kernel.setDuty (plain / 100.f); break;
        case Modulation::destFillDuty:      kernel.setFillDuty (plain / 100.f); break;
        case Modulation::destMinAmp:        kernel.setMinAmp (plain / 100.f); break;
        case Modulation::destMaxAmp:        kernel.setMaxAmp (plain / 100.f); break;
        case Modulation::destMinPan:        kernel.setMinPan (plain / 100.f); break;
        case Modulation::destMaxPan:        kernel.setMaxPan (plain / 100.f); break;
        case Modulation::destMinPitch:      kernel.setMinPitch (plain); break;
        case Modulation::destMaxPitch:      kernel.setMaxPitch (plain); break;
        case Modulation::destMinRepeat:     kernel.setMinRepeat (juce::roundToInt (plain)); break;
        case Modulation::destMaxRepeat:     kernel.setMaxRepeat (juce::roundToInt (plain)); break;
        case Modulation::destStutter:       kernel.setStutter (plain / 100.f); break;
        case Modulation::destArea:          kernel.setArea (plain / 100.f); break;
        case Modulation::destStraight:      kernel.setStraight (plain / 100.f); break;
        case Modulation::destRegular:       kernel.setRegular (plain / 100.f); break;
        case Modulation::destRitard:        kernel.setRitard (plain / 100.f); break;
        case Modulation::destSpeed:         kernel.setSpeed (plain); break;
        case Modulation::destActivity:      kernel.setActivity (plain / 100.f); break;
        case Modulation::destMinBits:       kernel.setMinBits (juce::roundToInt (plain)); break;
        case Modulation::destMaxBits:       kernel.setMaxBits (juce::roundToInt (plain)); break;
        case Modulation::destMinFreq:       kernel.setMinFreq (plain / 100.f); break;
        case Modulation::destMaxFreq:       kernel.setMaxFreq (plain / 100.f); break;
        case Modulation::destCombFeedback:  kernel.setCombFeedback (plain / 100.f); break;
        case Modulation::destCombMinDelay:  kernel.setCombMinDelay (plain); break;
        case Modulation::destCombMaxDelay:  kernel.setCombMaxDelay (plain); break;
        default: break;
    }
}

void LiveCutAudioProcessor::applyModulation (int numSamples)
{
    std::array<float, Modulation::numDestinations> destinationSums {};

    for (int i = 0; i < Modulation::numSlots; ++i)
    {
        if (modSlotParams[size_t (i)].enabled->load() <= 0.5f)
            continue;

        const auto source = juce::roundToInt (
            modSlotParams[size_t (i)].source->load());
        const auto destination = juce::roundToInt (
            modSlotParams[size_t (i)].destination->load());

        if (source <= 0 || destination <= 0 || destination >= Modulation::numDestinations)
            continue;

        const auto depth = modSlotParams[size_t (i)].depth->load() / 100.f;
        destinationSums[size_t (destination)] += getSourceValue (source) * depth;
    }

    const auto sr = juce::jmax (1.0, getSampleRate());
    const auto smoothingCoefficient = std::exp (-double (numSamples) / (sr * 0.025));

    for (int destination = 1; destination < Modulation::numDestinations; ++destination)
    {
        auto& smoothed = smoothedDestinationOffsets[size_t (destination)];
        smoothed = float (smoothingCoefficient * smoothed
                          + (1.0 - smoothingCoefficient) * destinationSums[size_t (destination)]);
        destinationDisplayOffsets[size_t (destination)].store (smoothed, std::memory_order_relaxed);
        applyDestination (destination, smoothed);
    }
}

void LiveCutAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;

    const auto numSamples = buffer.getNumSamples();
    if (numSamples == 0)
        return;

    handleMidi (midi);
    updateInputEnvelope (buffer, numSamples);

    Livecut::Kernel::TimeInfo timeInfo;
    bool hostIsPlaying = false;
    bool hostIsLooping = false;
    bool gotHostPosition = false;
    bool gotHostSamplePosition = false;
    bool gotLoopPoints = false;
    std::int64_t hostSamplePosition = 0;
    double loopStartPpq = 0.0;
    double loopEndPpq = 0.0;

    if (auto* playHead = getPlayHead())
    {
        if (auto position = playHead->getPosition())
        {
            if (auto bpm = position->getBpm())
                timeInfo.tempo = *bpm;
            if (auto sig = position->getTimeSignature())
            {
                timeInfo.numerator = sig->numerator;
                timeInfo.denominator = sig->denominator;
            }

            hostIsPlaying = position->getIsPlaying();
            hostIsLooping = position->getIsLooping();

            if (auto samples = position->getTimeInSamples())
            {
                hostSamplePosition = *samples;
                gotHostSamplePosition = true;
            }

            if (auto loop = position->getLoopPoints())
            {
                loopStartPpq = loop->ppqStart;
                loopEndPpq = loop->ppqEnd;
                gotLoopPoints = loopEndPpq > loopStartPpq;
            }

            if (auto ppq = position->getPpqPosition(); ppq && hostIsPlaying)
            {
                timeInfo.ppqPos = *ppq;
                gotHostPosition = true;
            }
        }
    }

    if (!gotHostPosition)
    {
        timeInfo.ppqPos = freeRunPpq;
        freeRunPpq += (numSamples / juce::jmax (1.0, getSampleRate())) * (timeInfo.tempo / 60.0);
    }
    else
    {
        freeRunPpq = timeInfo.ppqPos;
    }

    timeInfo.playing = true;
    timeInfo.transportChanged = (hostIsPlaying != wasPlaying);

    // Build a robust transport discontinuity detector from both sample position
    // and PPQ.  Some hosts expose one more reliably than the other, and JUCE
    // documents both as optional.  A backwards PPQ move is always a jump/loop;
    // forward jumps are detected against the position expected from the previous
    // audio callback.  Explicit loop points are used as an additional signal.
    const auto ppqBlockDuration =
        (double (numSamples) / juce::jmax (1.0, getSampleRate())) * (timeInfo.tempo / 60.0);
    const auto ppqTolerance = juce::jmax (0.002, ppqBlockDuration * 0.75);
    bool timelineJump = false;
    bool loopWrap = false;

    if (hostIsPlaying)
    {
        if (gotHostSamplePosition)
        {
            if (expectedHostSamplePosition != std::numeric_limits<std::int64_t>::min())
            {
                const auto sampleDelta = hostSamplePosition - expectedHostSamplePosition;
                if (sampleDelta > 8 || sampleDelta < -8)
                    timelineJump = true;
            }

            expectedHostSamplePosition = hostSamplePosition + std::int64_t (numSamples);
        }
        else
        {
            expectedHostSamplePosition = std::numeric_limits<std::int64_t>::min();
        }

        if (gotHostPosition)
        {
            if (std::isfinite (lastHostPpq)
                && timeInfo.ppqPos < lastHostPpq - ppqTolerance)
                timelineJump = true;

            if (std::isfinite (expectedModPpq)
                && std::abs (timeInfo.ppqPos - expectedModPpq) > ppqTolerance)
                timelineJump = true;

            if (hostIsLooping && gotLoopPoints && std::isfinite (lastHostPpq))
            {
                const auto loopTolerance = juce::jmax (0.01, ppqBlockDuration * 2.0);
                loopWrap = lastHostPpq >= loopEndPpq - loopTolerance
                        && timeInfo.ppqPos <= loopStartPpq + loopTolerance;
            }

            lastHostPpq = timeInfo.ppqPos;
            expectedModPpq = timeInfo.ppqPos + ppqBlockDuration;
        }
        else
        {
            lastHostPpq = std::numeric_limits<double>::quiet_NaN();
            expectedModPpq = std::numeric_limits<double>::quiet_NaN();
        }
    }
    else
    {
        expectedHostSamplePosition = std::numeric_limits<std::int64_t>::min();
        lastHostPpq = std::numeric_limits<double>::quiet_NaN();
        expectedModPpq = std::numeric_limits<double>::quiet_NaN();
    }

    const auto transportStarted = ! wasPlaying && hostIsPlaying;
    const auto transportRetrigger = transportStarted || timelineJump || loopWrap;

    // Manual OFF -> ON edges are published by the APVTS listener.  Merge those
    // with transport events here on the audio thread.  Transport events only
    // reset sources whose Retrig latch is enabled.
    auto retriggerMask = pendingManualRetriggers.exchange (0, std::memory_order_acq_rel);
    if (transportRetrigger)
    {
        for (int i = 0; i < Modulation::numLfos; ++i)
            if (lfoParams[size_t (i)].retrigger->load() > 0.5f)
                retriggerMask |= retriggerBitForLfo (i);

        for (int i = 0; i < Modulation::numWanders; ++i)
            if (wanderParams[size_t (i)].retrigger->load() > 0.5f)
                retriggerMask |= retriggerBitForWander (i);
    }

    wasPlaying = hostIsPlaying;

    pushParameters();
    updateModulationSources (numSamples, timeInfo.tempo, timeInfo.numerator, timeInfo.denominator,
                             timeInfo.ppqPos, gotHostPosition, hostIsPlaying, retriggerMask);
    applyModulation (numSamples);

    if (bypassParam != nullptr && bypassParam->get())
        return;

    const float* inL = buffer.getReadPointer (0);
    const float* inR = buffer.getReadPointer (buffer.getNumChannels() > 1 ? 1 : 0);
    float* outL = buffer.getWritePointer (0);
    float* outR = buffer.getWritePointer (buffer.getNumChannels() > 1 ? 1 : 0);

    kernel.process ({inL, inR}, {outL, outR}, uint32_t (numSamples), timeInfo);
}

void LiveCutAudioProcessor::setLastEditorSize (int width, int height) noexcept
{
    lastEditorWidth.store (width, std::memory_order_relaxed);
    lastEditorHeight.store (height, std::memory_order_relaxed);
}

bool LiveCutAudioProcessor::getLastEditorSize (int& width, int& height) const noexcept
{
    width = lastEditorWidth.load (std::memory_order_relaxed);
    height = lastEditorHeight.load (std::memory_order_relaxed);
    return width > 0 && height > 0;
}

float LiveCutAudioProcessor::getDestinationModulation (int destination) const noexcept
{
    if (destination <= 0 || destination >= Modulation::numDestinations)
        return 0.f;

    return destinationDisplayOffsets[size_t (destination)].load (std::memory_order_relaxed);
}

float LiveCutAudioProcessor::getModulationSourceDisplay (int sourceIndex) const noexcept
{
    if (sourceIndex >= 1 && sourceIndex <= Modulation::numLfos)
        return lfoDisplayValues[size_t (sourceIndex - 1)].load (std::memory_order_relaxed);

    const auto firstWander = Modulation::numLfos + 1;
    const auto lastWander = Modulation::numLfos + Modulation::numWanders;
    if (sourceIndex >= firstWander && sourceIndex <= lastWander)
        return wanderDisplayValues[size_t (sourceIndex - firstWander)].load (std::memory_order_relaxed);

    return 0.f;
}

void LiveCutAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty (stateVersionKey, currentStateVersion, nullptr);

    int width = 0, height = 0;
    if (getLastEditorSize (width, height))
    {
        state.setProperty (editorWidthStateKey, width, nullptr);
        state.setProperty (editorHeightStateKey, height, nullptr);
    }

    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void LiveCutAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
    {
        if (! xml->hasTagName (apvts.state.getType()))
            return;

        auto state = juce::ValueTree::fromXml (*xml);
        const auto version = static_cast<int> (state.getProperty (stateVersionKey, 0));

        // UI03 inserted 2 and 4 ahead of the original subdivision choices.
        // Only states older than UI03 need this migration.
        if (version < 3)
        {
            for (auto child : state)
            {
                if (child.getProperty ("id").toString() == ParamIDs::subdiv)
                {
                    const auto oldIndex = juce::roundToInt (float (child.getProperty ("value", 1.f)));
                    if (oldIndex >= 0 && oldIndex <= 6)
                        child.setProperty ("value", oldIndex + 2, nullptr);
                    break;
                }
            }
        }

        if (version < 4)
        {
            // UI04 adds triplet entries between all of the original straight
            // LFO divisions, so old index N becomes new index N * 2.
            for (int i = 0; i < Modulation::numLfos; ++i)
            {
                const auto id = ParamIDs::lfoDivision (i);
                for (auto child : state)
                {
                    if (child.getProperty ("id").toString() == id)
                    {
                        const auto oldIndex = juce::roundToInt (float (child.getProperty ("value", 2.f)));
                        if (oldIndex >= 0 && oldIndex <= 6)
                            child.setProperty ("value", oldIndex * 2, nullptr);
                        break;
                    }
                }
            }

            // UI04 adds fourteen straight/triplet note-value Wander periods
            // before the original 1/2/4/8/16/32/64-bar choices.
            for (int i = 0; i < Modulation::numWanders; ++i)
            {
                const auto id = ParamIDs::wanderDivision (i);
                for (auto child : state)
                {
                    if (child.getProperty ("id").toString() == id)
                    {
                        const auto oldIndex = juce::roundToInt (float (child.getProperty ("value", 1.f)));
                        if (oldIndex >= 0 && oldIndex <= 6)
                            child.setProperty ("value", oldIndex + 14, nullptr);
                        break;
                    }
                }
            }

            // Seed is inserted immediately after SubDiv in the matrix target
            // menu, so preserve all UI03 destination selections after SubDiv.
            for (int i = 0; i < Modulation::numSlots; ++i)
            {
                const auto id = ParamIDs::modDestination (i);
                for (auto child : state)
                {
                    if (child.getProperty ("id").toString() == id)
                    {
                        const auto oldDestination =
                            juce::roundToInt (float (child.getProperty ("value", 0.f)));
                        if (oldDestination >= 2)
                            child.setProperty ("value", oldDestination + 1, nullptr);
                        break;
                    }
                }
            }
        }

        if (version < currentStateVersion)
            state.setProperty (stateVersionKey, currentStateVersion, nullptr);

        const auto width = static_cast<int> (state.getProperty (editorWidthStateKey, 0));
        const auto height = static_cast<int> (state.getProperty (editorHeightStateKey, 0));

        if (width > 0 && height > 0)
            setLastEditorSize (width, height);

        apvts.replaceState (state);
    }
}

juce::AudioProcessorEditor* LiveCutAudioProcessor::createEditor()
{
    return new LiveCutEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new LiveCutAudioProcessor();
}
