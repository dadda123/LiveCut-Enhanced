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
} // namespace ParamIDs

inline juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    using namespace juce;
    using FloatParam = AudioParameterFloat;
    using IntParam = AudioParameterInt;
    using ChoiceParam = AudioParameterChoice;
    using BoolParam = AudioParameterBool;

    auto percent = AudioParameterFloatAttributes().withLabel ("%");
    auto ms = AudioParameterFloatAttributes().withLabel ("ms");
    auto cents = AudioParameterFloatAttributes().withLabel ("ct");

    AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add (
        std::make_unique<ChoiceParam> (ParameterID {ParamIDs::cutproc, 1}, "Cut Proc",
                                       StringArray {"CutProc11", "WarpCut", "SQPusher"}, 0),
        std::make_unique<ChoiceParam> (ParameterID {ParamIDs::subdiv, 1}, "SubDiv",
                                       StringArray {"6", "8", "12", "16", "18", "24", "32"}, 1),
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

    return layout;
}
