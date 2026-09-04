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

#include <juce_audio_processors/juce_audio_processors.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <functional>
#include <limits>
#include <memory>
#include <vector>

#include "Params.h"
#include "dsp/Kernel.h"

class LiveCutAudioProcessor : public juce::AudioProcessor,
                              private juce::AudioProcessorValueTreeState::Listener
{
public:
    LiveCutAudioProcessor();
    ~LiveCutAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported(const BusesLayout &layouts) const override;
    void processBlock(juce::AudioBuffer<float> &, juce::MidiBuffer &) override;

    juce::AudioProcessorEditor *createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String &) override {}

    void getStateInformation(juce::MemoryBlock &destData) override;
    void setStateInformation(const void *data, int sizeInBytes) override;

    juce::AudioProcessorParameter *getBypassParameter() const override { return bypassParam; }

    const Livecut::Kernel &getKernel() const { return kernel; }

    void setLastEditorSize (int width, int height) noexcept;
    bool getLastEditorSize (int& width, int& height) const noexcept;

    float getDestinationModulation (int destination) const noexcept;
    float getModulationSourceDisplay (int sourceIndex) const noexcept;

    juce::AudioProcessorValueTreeState apvts;

private:
    struct ParamPush
    {
        std::atomic<float> *source = nullptr;
        std::function<void(float)> apply;
        float lastPushed = std::numeric_limits<float>::quiet_NaN();
    };

    struct LfoState
    {
        double phase = 0.0;
        double fadeAgeSeconds = 0.0;
        double syncAnchorPpq = 0.0;
        std::int64_t lastSyncCycle = -1;
        bool syncAnchorValid = false;
        bool retriggerWasEnabled = false;
        float sampleHold = 0.f;
        float randomA = 0.f;
        float randomB = 0.f;
    };

    struct WanderState
    {
        double phase = 0.0;
        float current = 0.f;
        float target = 0.f;
        bool retriggerWasEnabled = false;
    };

    struct LfoParamRefs
    {
        std::atomic<float>* wave = nullptr;
        std::atomic<float>* rate = nullptr;
        std::atomic<float>* sync = nullptr;
        std::atomic<float>* division = nullptr;
        std::atomic<float>* phase = nullptr;
        std::atomic<float>* unipolar = nullptr;
        std::atomic<float>* retrigger = nullptr;
        std::atomic<float>* fade = nullptr;
    };

    struct WanderParamRefs
    {
        std::atomic<float>* rate = nullptr;
        std::atomic<float>* sync = nullptr;
        std::atomic<float>* division = nullptr;
        std::atomic<float>* feel = nullptr;
        std::atomic<float>* smooth = nullptr;
        std::atomic<float>* range = nullptr;
        std::atomic<float>* unipolar = nullptr;
        std::atomic<float>* retrigger = nullptr;
    };

    struct ModSlotParamRefs
    {
        std::atomic<float>* enabled = nullptr;
        std::atomic<float>* source = nullptr;
        std::atomic<float>* destination = nullptr;
        std::atomic<float>* depth = nullptr;
    };

    void bindParameter(const char *paramID, std::function<void(float)> apply);
    void pushParameters();

    void handleMidi (const juce::MidiBuffer& midi);
    void updateInputEnvelope (const juce::AudioBuffer<float>& buffer, int numSamples);
    void updateModulationSources (int numSamples, double bpm, double numerator, double denominator,
                                  double ppqPos, bool gotHostPosition, bool hostIsPlaying,
                                  std::uint32_t retriggerMask);
    void applyModulation (int numSamples);
    float getSourceValue (int sourceIndex) const noexcept;
    float evaluateLfo (int index, double phase) noexcept;
    double lfoFrequencyHz (int index, double bpm) const noexcept;
    double lfoCyclesPerQuarter (int index) const noexcept;
    double wanderFrequencyHz (int index, double bpm, double numerator, double denominator) const noexcept;
    float randomBipolar() noexcept;
    void applyDestination (int destination, float normalizedOffset);
    void parameterChanged (const juce::String& parameterID, float newValue) override;
    static constexpr std::uint32_t retriggerBitForLfo (int index) noexcept
    {
        return std::uint32_t (1) << std::uint32_t (index);
    }
    static constexpr std::uint32_t retriggerBitForWander (int index) noexcept
    {
        return std::uint32_t (1) << std::uint32_t (Modulation::numLfos + index);
    }

    Livecut::Kernel kernel;
    std::vector<ParamPush> paramPushers;
    juce::AudioParameterBool *bypassParam = nullptr;

    bool wasPlaying = false;
    double freeRunPpq = 0.0;
    double expectedModPpq = std::numeric_limits<double>::quiet_NaN();
    double lastHostPpq = std::numeric_limits<double>::quiet_NaN();
    std::int64_t expectedHostSamplePosition = std::numeric_limits<std::int64_t>::min();
    std::atomic<std::uint32_t> pendingManualRetriggers {0};

    std::atomic<int> lastEditorWidth {0};
    std::atomic<int> lastEditorHeight {0};

    std::array<LfoState, Modulation::numLfos> lfoStates;
    std::array<WanderState, Modulation::numWanders> wanderStates;
    std::array<LfoParamRefs, Modulation::numLfos> lfoParams;
    std::array<WanderParamRefs, Modulation::numWanders> wanderParams;
    std::array<ModSlotParamRefs, Modulation::numSlots> modSlotParams;
    std::array<juce::RangedAudioParameter*, Modulation::numDestinations> destinationParameters {};
    std::array<float, Modulation::numLfos> lfoValues {};
    std::array<float, Modulation::numWanders> wanderValues {};
    std::array<float, Modulation::numDestinations> smoothedDestinationOffsets {};

    // UI-facing snapshots are atomic because the audio thread writes them while
    // the message thread paints modulation indicators.
    std::array<std::atomic<float>, Modulation::numLfos> lfoDisplayValues;
    std::array<std::atomic<float>, Modulation::numWanders> wanderDisplayValues;
    std::array<std::atomic<float>, Modulation::numDestinations> destinationDisplayOffsets;

    juce::Random modulationRandom {0x4c697665437574ULL};
    int lastAppliedModulatedSeed = -1;

    float modWheelValue = 0.f;
    float velocityValue = 0.f;
    float aftertouchValue = 0.f;
    float inputEnvelopeValue = 0.f;
    std::array<bool, 128> activeMidiNotes {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LiveCutAudioProcessor)
};
