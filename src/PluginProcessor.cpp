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

#include "PluginProcessor.h"

#include "Params.h"
#include "PluginEditor.h"

LiveCutAudioProcessor::LiveCutAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createParameterLayout())
{
    bypassParam = dynamic_cast<juce::AudioParameterBool*> (apvts.getParameter (ParamIDs::bypass));

    auto& k = kernel;
    bindParameter (ParamIDs::cutproc, [&k] (float v) { k.setCutProc (int (v)); });
    bindParameter (ParamIDs::subdiv, [&k] (float v) {
        static constexpr int subdivs[] = {6, 8, 12, 16, 18, 24, 32};
        k.setSubDiv (subdivs[juce::jlimit (0, 6, int (v))]);
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

void LiveCutAudioProcessor::prepareToPlay (double sampleRate, int)
{
    kernel.setSampleRate (sampleRate);
    // force a full re-push so sample-rate dependent values (crusher freq) update
    for (auto& p : paramPushers)
        p.lastPushed = std::numeric_limits<float>::quiet_NaN();
    pushParameters();
    freeRunPpq = 0.0;
    wasPlaying = false;
}

bool LiveCutAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainInputChannelSet() == juce::AudioChannelSet::stereo()
        && layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void LiveCutAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const auto numSamples = buffer.getNumSamples();
    if (numSamples == 0)
        return;

    if (bypassParam != nullptr && bypassParam->get())
        return;

    pushParameters();

    Livecut::Kernel::TimeInfo timeInfo;
    bool hostIsPlaying = false;
    bool gotHostPosition = false;

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
            if (auto ppq = position->getPpqPosition(); ppq && hostIsPlaying)
            {
                timeInfo.ppqPos = *ppq;
                gotHostPosition = true;
            }
        }
    }

    if (!gotHostPosition)
    {
        // free-run: keep slicing against an internal clock (standalone, or host stopped)
        timeInfo.ppqPos = freeRunPpq;
        freeRunPpq += (numSamples / getSampleRate()) * (timeInfo.tempo / 60.0);
    }
    else
    {
        freeRunPpq = timeInfo.ppqPos;
    }

    timeInfo.playing = true;
    timeInfo.transportChanged = (hostIsPlaying != wasPlaying);
    wasPlaying = hostIsPlaying;

    const float* inL = buffer.getReadPointer (0);
    const float* inR = buffer.getReadPointer (buffer.getNumChannels() > 1 ? 1 : 0);
    float* outL = buffer.getWritePointer (0);
    float* outR = buffer.getWritePointer (buffer.getNumChannels() > 1 ? 1 : 0);

    kernel.process ({inL, inR}, {outL, outR}, uint32_t (numSamples), timeInfo);
}

void LiveCutAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void LiveCutAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessorEditor* LiveCutAudioProcessor::createEditor()
{
    return new LiveCutEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new LiveCutAudioProcessor();
}
