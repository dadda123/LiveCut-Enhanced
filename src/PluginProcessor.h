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

#include <juce_audio_processors/juce_audio_processors.h>

#include "dsp/Kernel.h"

class LiveCutAudioProcessor : public juce::AudioProcessor
{
public:
    LiveCutAudioProcessor();

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported(const BusesLayout &layouts) const override;
    void processBlock(juce::AudioBuffer<float> &, juce::MidiBuffer &) override;

    juce::AudioProcessorEditor *createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
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

    juce::AudioProcessorValueTreeState apvts;

private:
    struct ParamPush
    {
        std::atomic<float> *source = nullptr;
        std::function<void(float)> apply;
        float lastPushed = std::numeric_limits<float>::quiet_NaN();
    };

    void bindParameter(const char *paramID, std::function<void(float)> apply);
    void pushParameters();

    Livecut::Kernel kernel;
    std::vector<ParamPush> paramPushers;
    juce::AudioParameterBool *bypassParam = nullptr;

    bool wasPlaying = false;
    double freeRunPpq = 0.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LiveCutAudioProcessor)
};
