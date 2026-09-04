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

namespace ParamIDs
{
inline constexpr const char* cutproc = "cutproc";
inline constexpr const char* subdiv = "subdiv";
inline constexpr const char* seed = "seed";
inline constexpr const char* fade = "fade";
inline constexpr const char* minamp = "minamp";
inline constexpr const char* maxamp = "maxamp";
inline constexpr const char* minpan = "minpan";
inline constexpr const char* maxpan = "maxpan";
inline constexpr const char* minpitch = "minpitch";
inline constexpr const char* maxpitch = "maxpitch";
inline constexpr const char* duty = "duty";
inline constexpr const char* fillduty = "fillduty";
inline constexpr const char* minphrase = "minphrase";
inline constexpr const char* maxphrase = "maxphrase";
inline constexpr const char* minrepeat = "minrepeat";
inline constexpr const char* maxrepeat = "maxrepeat";
inline constexpr const char* stutter = "stutter";
inline constexpr const char* area = "area";
inline constexpr const char* straight = "straight";
inline constexpr const char* regular = "regular";
inline constexpr const char* ritard = "ritard";
inline constexpr const char* speed = "speed";
inline constexpr const char* activity = "activity";
inline constexpr const char* crusher = "crusher";
inline constexpr const char* minbits = "minbits";
inline constexpr const char* maxbits = "maxbits";
inline constexpr const char* minfreq = "minfreq";
inline constexpr const char* maxfreq = "maxfreq";
inline constexpr const char* comb = "comb";
inline constexpr const char* combtype = "combtype";
inline constexpr const char* combfeedback = "combfeedback";
inline constexpr const char* combmindelay = "combmindelay";
inline constexpr const char* combmaxdelay = "combmaxdelay";
inline constexpr const char* bypass = "bypass";

inline juce::String lfoWave (int i)       { return juce::String ("lfo") + juce::String (i + 1) + "Wave"; }
inline juce::String lfoRate (int i)       { return juce::String ("lfo") + juce::String (i + 1) + "Rate"; }
inline juce::String lfoSync (int i)       { return juce::String ("lfo") + juce::String (i + 1) + "Sync"; }
inline juce::String lfoDivision (int i)   { return juce::String ("lfo") + juce::String (i + 1) + "Division"; }
inline juce::String lfoPhase (int i)      { return juce::String ("lfo") + juce::String (i + 1) + "Phase"; }
inline juce::String lfoUnipolar (int i)   { return juce::String ("lfo") + juce::String (i + 1) + "Unipolar"; }
inline juce::String lfoRetrigger (int i)  { return juce::String ("lfo") + juce::String (i + 1) + "Retrigger"; }
inline juce::String lfoFade (int i)       { return juce::String ("lfo") + juce::String (i + 1) + "Fade"; }

inline juce::String wanderRate (int i)      { return juce::String ("wander") + juce::String (i + 1) + "Rate"; }
inline juce::String wanderSync (int i)      { return juce::String ("wander") + juce::String (i + 1) + "Sync"; }
inline juce::String wanderDivision (int i)  { return juce::String ("wander") + juce::String (i + 1) + "Division"; }
inline juce::String wanderFeel (int i)      { return juce::String ("wander") + juce::String (i + 1) + "Feel"; }
inline juce::String wanderSmooth (int i)    { return juce::String ("wander") + juce::String (i + 1) + "Smooth"; }
inline juce::String wanderRange (int i)     { return juce::String ("wander") + juce::String (i + 1) + "Range"; }
inline juce::String wanderUnipolar (int i)  { return juce::String ("wander") + juce::String (i + 1) + "Unipolar"; }
inline juce::String wanderRetrigger (int i) { return juce::String ("wander") + juce::String (i + 1) + "Retrigger"; }

inline juce::String modEnabled (int i)     { return juce::String ("mod") + juce::String (i + 1) + "Enabled"; }
inline juce::String modSource (int i)      { return juce::String ("mod") + juce::String (i + 1) + "Source"; }
inline juce::String modDestination (int i) { return juce::String ("mod") + juce::String (i + 1) + "Destination"; }
inline juce::String modDepth (int i)       { return juce::String ("mod") + juce::String (i + 1) + "Depth"; }
} // namespace ParamIDs

namespace Modulation
{
inline constexpr int numLfos = 4;
inline constexpr int numWanders = 4;
inline constexpr int numSlots = 16;

inline juce::StringArray lfoWaveChoices()
{
    return {"Sine", "Triangle", "Saw Up", "Saw Down", "Square", "S&H", "Smooth Random"};
}

inline juce::StringArray lfoSyncChoices()
{
    return {"1/1", "1/1T", "1/2", "1/2T", "1/4", "1/4T", "1/8", "1/8T",
            "1/16", "1/16T", "1/32", "1/32T", "1/64", "1/64T"};
}

inline juce::StringArray wanderSyncChoices()
{
    // Short rhythmic divisions first, followed by the original long bar periods.
    // Triplets sit immediately after their straight division for quick scanning.
    return {"1/64", "1/64T", "1/32", "1/32T", "1/16", "1/16T",
            "1/8", "1/8T", "1/4", "1/4T", "1/2", "1/2T",
            "1/1", "1/1T",
            "1 bar", "2 bars", "4 bars", "8 bars", "16 bars", "32 bars", "64 bars"};
}

inline juce::StringArray sourceChoices()
{
    return {"Off", "LFO 1", "LFO 2", "LFO 3", "LFO 4",
            "Wander 1", "Wander 2", "Wander 3", "Wander 4",
            "Mod Wheel", "Velocity", "Aftertouch", "Input Env"};
}

enum Destination
{
    destOff = 0,
    destSubdiv,
    destSeed,
    destFade,
    destMinPhrase,
    destMaxPhrase,
    destDuty,
    destFillDuty,
    destMinAmp,
    destMaxAmp,
    destMinPan,
    destMaxPan,
    destMinPitch,
    destMaxPitch,
    destMinRepeat,
    destMaxRepeat,
    destStutter,
    destArea,
    destStraight,
    destRegular,
    destRitard,
    destSpeed,
    destActivity,
    destMinBits,
    destMaxBits,
    destMinFreq,
    destMaxFreq,
    destCombFeedback,
    destCombMinDelay,
    destCombMaxDelay,
    numDestinations
};

inline juce::StringArray destinationChoices()
{
    return {"Off", "SubDiv", "Seed", "Fade", "Min Phrase", "Max Phrase", "Duty", "Fill Duty",
            "Min Amp", "Max Amp", "Min Pan", "Max Pan", "Min Pitch", "Max Pitch",
            "Min Repeat", "Max Repeat", "Stutter", "Area", "Straight", "Regular",
            "Ritard", "Speed", "Activity", "Min Bits", "Max Bits", "Min Freq",
            "Max Freq", "Comb Feedback", "Comb Min Delay", "Comb Max Delay"};
}

inline const char* destinationParameterID (int destination)
{
    switch (destination)
    {
        case destSubdiv:        return ParamIDs::subdiv;
        case destSeed:          return ParamIDs::seed;
        case destFade:          return ParamIDs::fade;
        case destMinPhrase:     return ParamIDs::minphrase;
        case destMaxPhrase:     return ParamIDs::maxphrase;
        case destDuty:          return ParamIDs::duty;
        case destFillDuty:      return ParamIDs::fillduty;
        case destMinAmp:        return ParamIDs::minamp;
        case destMaxAmp:        return ParamIDs::maxamp;
        case destMinPan:        return ParamIDs::minpan;
        case destMaxPan:        return ParamIDs::maxpan;
        case destMinPitch:      return ParamIDs::minpitch;
        case destMaxPitch:      return ParamIDs::maxpitch;
        case destMinRepeat:     return ParamIDs::minrepeat;
        case destMaxRepeat:     return ParamIDs::maxrepeat;
        case destStutter:       return ParamIDs::stutter;
        case destArea:          return ParamIDs::area;
        case destStraight:      return ParamIDs::straight;
        case destRegular:       return ParamIDs::regular;
        case destRitard:        return ParamIDs::ritard;
        case destSpeed:         return ParamIDs::speed;
        case destActivity:      return ParamIDs::activity;
        case destMinBits:       return ParamIDs::minbits;
        case destMaxBits:       return ParamIDs::maxbits;
        case destMinFreq:       return ParamIDs::minfreq;
        case destMaxFreq:       return ParamIDs::maxfreq;
        case destCombFeedback:  return ParamIDs::combfeedback;
        case destCombMinDelay:  return ParamIDs::combmindelay;
        case destCombMaxDelay:  return ParamIDs::combmaxdelay;
        default:                return nullptr;
    }
}

inline int destinationForParameterID (const juce::String& parameterID)
{
    for (int destination = 1; destination < numDestinations; ++destination)
        if (const auto* id = destinationParameterID (destination))
            if (parameterID == id)
                return destination;

    return destOff;
}
} // namespace Modulation

inline juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    using namespace juce;
    using FloatParam = AudioParameterFloat;
    using IntParam = AudioParameterInt;
    using ChoiceParam = AudioParameterChoice;
    using BoolParam = AudioParameterBool;

    auto percent = AudioParameterFloatAttributes().withLabel ("%");
    auto ms = AudioParameterFloatAttributes().withLabel ("ms");
    auto seconds = AudioParameterFloatAttributes().withLabel ("s");
    auto hz = AudioParameterFloatAttributes().withLabel ("Hz");
    auto degrees = AudioParameterFloatAttributes().withLabel ("deg");
    auto cents = AudioParameterFloatAttributes().withLabel ("ct");

    AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add (
        std::make_unique<ChoiceParam> (ParameterID {ParamIDs::cutproc, 1}, "Cut Proc",
                                       StringArray {"CutProc11", "WarpCut", "SQPusher"}, 0),
        std::make_unique<ChoiceParam> (ParameterID {ParamIDs::subdiv, 1}, "SubDiv",
                                       StringArray {"2", "4", "6", "8", "12", "16", "18", "24", "32", "64"}, 3),
        std::make_unique<IntParam> (ParameterID {ParamIDs::seed, 1}, "Seed", 1, 16, 1),
        std::make_unique<FloatParam> (ParameterID {ParamIDs::fade, 1}, "Fade",
                                      NormalisableRange<float> (0.f, 100.f, 0.f, 0.5f), 0.f, ms),
        std::make_unique<FloatParam> (ParameterID {ParamIDs::minamp, 1}, "Min Amp",
                                      NormalisableRange<float> (0.f, 100.f), 100.f, percent),
        std::make_unique<FloatParam> (ParameterID {ParamIDs::maxamp, 1}, "Max Amp",
                                      NormalisableRange<float> (0.f, 100.f), 100.f, percent),
        std::make_unique<FloatParam> (ParameterID {ParamIDs::minpan, 1}, "Min Pan",
                                      NormalisableRange<float> (-100.f, 100.f), -20.f, percent),
        std::make_unique<FloatParam> (ParameterID {ParamIDs::maxpan, 1}, "Max Pan",
                                      NormalisableRange<float> (-100.f, 100.f), 20.f, percent),
        std::make_unique<FloatParam> (ParameterID {ParamIDs::minpitch, 1}, "Min Pitch",
                                      NormalisableRange<float> (-2400.f, 2400.f, 1.f), 0.f, cents),
        std::make_unique<FloatParam> (ParameterID {ParamIDs::maxpitch, 1}, "Max Pitch",
                                      NormalisableRange<float> (-2400.f, 2400.f, 1.f), 0.f, cents),
        std::make_unique<FloatParam> (ParameterID {ParamIDs::duty, 1}, "Duty",
                                      NormalisableRange<float> (0.f, 100.f), 100.f, percent),
        std::make_unique<FloatParam> (ParameterID {ParamIDs::fillduty, 1}, "Fill Duty",
                                      NormalisableRange<float> (0.f, 100.f), 100.f, percent),
        std::make_unique<IntParam> (ParameterID {ParamIDs::minphrase, 1}, "Min Phrase", 1, 8, 1),
        std::make_unique<IntParam> (ParameterID {ParamIDs::maxphrase, 1}, "Max Phrase", 1, 8, 4));

    layout.add (
        std::make_unique<IntParam> (ParameterID {ParamIDs::minrepeat, 1}, "Min Repeat", 0, 4, 0),
        std::make_unique<IntParam> (ParameterID {ParamIDs::maxrepeat, 1}, "Max Repeat", 0, 4, 1),
        std::make_unique<FloatParam> (ParameterID {ParamIDs::stutter, 1}, "Stutter",
                                      NormalisableRange<float> (0.f, 100.f), 80.f, percent),
        std::make_unique<FloatParam> (ParameterID {ParamIDs::area, 1}, "Area",
                                      NormalisableRange<float> (0.f, 100.f), 50.f, percent),
        std::make_unique<FloatParam> (ParameterID {ParamIDs::straight, 1}, "Straight",
                                      NormalisableRange<float> (0.f, 100.f), 30.f, percent),
        std::make_unique<FloatParam> (ParameterID {ParamIDs::regular, 1}, "Regular",
                                      NormalisableRange<float> (0.f, 100.f), 50.f, percent),
        std::make_unique<FloatParam> (ParameterID {ParamIDs::ritard, 1}, "Ritard",
                                      NormalisableRange<float> (0.f, 100.f), 50.f, percent),
        std::make_unique<FloatParam> (ParameterID {ParamIDs::speed, 1}, "Speed",
                                      NormalisableRange<float> (0.5f, 0.999f, 0.001f), 0.949f),
        std::make_unique<FloatParam> (ParameterID {ParamIDs::activity, 1}, "Activity",
                                      NormalisableRange<float> (0.f, 100.f), 50.f, percent));

    layout.add (
        std::make_unique<BoolParam> (ParameterID {ParamIDs::crusher, 1}, "Crusher", false),
        std::make_unique<IntParam> (ParameterID {ParamIDs::minbits, 1}, "Min Bits", 1, 32, 32),
        std::make_unique<IntParam> (ParameterID {ParamIDs::maxbits, 1}, "Max Bits", 1, 32, 32),
        std::make_unique<FloatParam> (ParameterID {ParamIDs::minfreq, 1}, "Min Freq",
                                      NormalisableRange<float> (0.f, 100.f), 50.f, percent),
        std::make_unique<FloatParam> (ParameterID {ParamIDs::maxfreq, 1}, "Max Freq",
                                      NormalisableRange<float> (0.f, 100.f), 50.f, percent),
        std::make_unique<BoolParam> (ParameterID {ParamIDs::comb, 1}, "Comb", false),
        std::make_unique<ChoiceParam> (ParameterID {ParamIDs::combtype, 1}, "Comb Type",
                                       StringArray {"FeedFwd", "FeedBack"}, 0),
        std::make_unique<FloatParam> (ParameterID {ParamIDs::combfeedback, 1}, "Feedback",
                                      NormalisableRange<float> (0.f, 90.f), 45.f, percent),
        std::make_unique<FloatParam> (ParameterID {ParamIDs::combmindelay, 1}, "Min Delay",
                                      NormalisableRange<float> (1.f, 50.f), 10.8f, ms),
        std::make_unique<FloatParam> (ParameterID {ParamIDs::combmaxdelay, 1}, "Max Delay",
                                      NormalisableRange<float> (1.f, 50.f), 10.8f, ms),
        std::make_unique<BoolParam> (ParameterID {ParamIDs::bypass, 1}, "Bypass", false));

    for (int i = 0; i < Modulation::numLfos; ++i)
    {
        const auto n = juce::String (i + 1);
        layout.add (
            std::make_unique<ChoiceParam> (ParameterID {ParamIDs::lfoWave (i), 1}, juce::String ("LFO ") + n + " Wave",
                                           Modulation::lfoWaveChoices(), 0),
            std::make_unique<FloatParam> (ParameterID {ParamIDs::lfoRate (i), 1}, juce::String ("LFO ") + n + " Rate",
                                          NormalisableRange<float> (0.01f, 20.f, 0.001f, 0.35f), 0.5f, hz),
            std::make_unique<BoolParam> (ParameterID {ParamIDs::lfoSync (i), 1}, juce::String ("LFO ") + n + " Sync", false),
            std::make_unique<ChoiceParam> (ParameterID {ParamIDs::lfoDivision (i), 1}, juce::String ("LFO ") + n + " Division",
                                           Modulation::lfoSyncChoices(), 4),
            std::make_unique<FloatParam> (ParameterID {ParamIDs::lfoPhase (i), 1}, juce::String ("LFO ") + n + " Phase",
                                          NormalisableRange<float> (0.f, 360.f, 1.f), 0.f, degrees),
            std::make_unique<BoolParam> (ParameterID {ParamIDs::lfoUnipolar (i), 1}, juce::String ("LFO ") + n + " Unipolar", false),
            std::make_unique<BoolParam> (ParameterID {ParamIDs::lfoRetrigger (i), 1}, juce::String ("LFO ") + n + " Retrigger", true),
            std::make_unique<FloatParam> (ParameterID {ParamIDs::lfoFade (i), 1}, juce::String ("LFO ") + n + " Fade In",
                                          NormalisableRange<float> (0.f, 10.f, 0.01f, 0.5f), 0.f, seconds));
    }

    for (int i = 0; i < Modulation::numWanders; ++i)
    {
        const auto n = juce::String (i + 1);
        layout.add (
            std::make_unique<FloatParam> (ParameterID {ParamIDs::wanderRate (i), 1}, juce::String ("Wander ") + n + " Rate",
                                          NormalisableRange<float> (0.005f, 2.f, 0.001f, 0.35f), 0.08f, hz),
            std::make_unique<BoolParam> (ParameterID {ParamIDs::wanderSync (i), 1}, juce::String ("Wander ") + n + " Sync", false),
            std::make_unique<ChoiceParam> (ParameterID {ParamIDs::wanderDivision (i), 1}, juce::String ("Wander ") + n + " Period",
                                           Modulation::wanderSyncChoices(), 15),
            std::make_unique<FloatParam> (ParameterID {ParamIDs::wanderFeel (i), 1}, juce::String ("Wander ") + n + " Feel",
                                          NormalisableRange<float> (0.f, 100.f), 50.f, percent),
            std::make_unique<FloatParam> (ParameterID {ParamIDs::wanderSmooth (i), 1}, juce::String ("Wander ") + n + " Smooth",
                                          NormalisableRange<float> (0.f, 100.f), 80.f, percent),
            std::make_unique<FloatParam> (ParameterID {ParamIDs::wanderRange (i), 1}, juce::String ("Wander ") + n + " Range",
                                          NormalisableRange<float> (0.f, 100.f), 100.f, percent),
            std::make_unique<BoolParam> (ParameterID {ParamIDs::wanderUnipolar (i), 1}, juce::String ("Wander ") + n + " Unipolar", false),
            std::make_unique<BoolParam> (ParameterID {ParamIDs::wanderRetrigger (i), 1}, juce::String ("Wander ") + n + " Retrigger", true));
    }

    for (int i = 0; i < Modulation::numSlots; ++i)
    {
        const auto n = juce::String (i + 1);
        layout.add (
            std::make_unique<BoolParam> (ParameterID {ParamIDs::modEnabled (i), 1}, juce::String ("Mod ") + n + " Enabled", false),
            std::make_unique<ChoiceParam> (ParameterID {ParamIDs::modSource (i), 1}, juce::String ("Mod ") + n + " Source",
                                           Modulation::sourceChoices(), 0),
            std::make_unique<ChoiceParam> (ParameterID {ParamIDs::modDestination (i), 1}, juce::String ("Mod ") + n + " Destination",
                                           Modulation::destinationChoices(), 0),
            std::make_unique<FloatParam> (ParameterID {ParamIDs::modDepth (i), 1}, juce::String ("Mod ") + n + " Depth",
                                          NormalisableRange<float> (-100.f, 100.f, 0.1f), 0.f, percent));
    }

    return layout;
}
