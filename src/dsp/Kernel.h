/*
 This file is part of Livecut
 Copyright 2004 by Remy Muller.
 VST3 SDK Adaption by Arne Scheffler
 JUCE 8 adaption for LiveCut Enhanced, 2026.

 Livecut can be redistributed and/or modified under the terms of the
 GNU General Public License, as published by the Free Software Foundation;
 either version 2 of the License, or (at your option) any later version.

 Livecut is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with Livecut; if not, visit www.gnu.org/licenses or write to the
 Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 Boston, MA 02111-1307 USA
 */

#pragma once

#include "BBCutter.h"
#include "BitCrusher.h"
#include "Comb.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <limits>
#include <utility>

namespace Livecut {

template <typename T>
inline constexpr T normalizedToPlain (T min, T max, T normalizedValue) noexcept
{
	return normalizedValue * (max - min) + min;
}

//------------------------------------------------------------------------
struct Kernel : BBCutListener
{
	Kernel () : bbcutter (player)
	{
		bbcutter.RegisterListener (&crusher);
		bbcutter.RegisterListener (&comb);
		bbcutter.RegisterListener (this);
		bbcutter.SetSubdiv (subDiv);
	}

	void setCutProc (int32_t index) { bbcutter.SetCutProc (index); }
	void setSubDiv (int32_t value)
	{
		subDiv = value;
		bbcutter.SetSubdiv (value);
	}
	void setSeed (int32_t value) { Math::randomseed (value); }
	void setFade (double ms) { bbcutter.SetFade (float (ms)); }
	void setMinAmp (double norm) { bbcutter.SetMinAmp (float (norm)); }
	void setMaxAmp (double norm) { bbcutter.SetMaxAmp (float (norm)); }
	void setMinPan (double value) { bbcutter.SetMinPan (float (value)); }
	void setMaxPan (double value) { bbcutter.SetMaxPan (float (value)); }
	void setMinPitch (double value) { bbcutter.SetMinDetune (float (value)); }
	void setMaxPitch (double value) { bbcutter.SetMaxDetune (float (value)); }
	void setDuty (double value) { bbcutter.SetDutyCycle (float (value)); }
	void setFillDuty (double value) { bbcutter.SetFillDutyCycle (float (value)); }
	void setMaxPhrase (int32_t value) { bbcutter.SetMaxPhraseLength (value); }
	void setMinPhrase (int32_t value) { bbcutter.SetMinPhraseLength (value); }
	void setMaxRepeat (int32_t value) { bbcutter.SetMaxRepeats (value); }
	void setMinRepeat (int32_t value) { bbcutter.SetMinRepeats (value); }
	void setStutter (double value) { bbcutter.SetStutterChance (float (value)); }
	void setArea (double value) { bbcutter.SetStutterArea (float (value)); }
	void setStraight (double value) { bbcutter.SetStraightChance (float (value)); }
	void setRegular (double value) { bbcutter.SetRegularChance (float (value)); }
	void setRitard (double value) { bbcutter.SetRitardChance (float (value)); }
	void setSpeed (double value) { bbcutter.SetAccel (float (value)); }
	void setActivity (double value) { bbcutter.SetActivity (float (value)); }
	void setBitcrusher (bool state) { crusher.SetOn (state); }
	void setMinBits (int32_t bits) { crusher.SetMinBits (float (bits)); }
	void setMaxBits (int32_t bits) { crusher.SetMaxBits (float (bits)); }
	void setMinFreq (double norm)
	{
		auto freq = normalizedToPlain (sampleRate / 100., sampleRate, norm);
		crusher.SetMinFreq (float (freq));
	}
	void setMaxFreq (double norm)
	{
		auto freq = normalizedToPlain (sampleRate / 100., sampleRate, norm);
		crusher.SetMaxFreq (float (freq));
	}
	void setComb (bool state) { comb.SetOn (state); }
	void setCombType (bool type) { comb.SetType (type ? 1 : 0); }
	void setCombFeedback (double feedback) { comb.SetFeedBack (float (feedback)); }
	void setCombMinDelay (double ms) { comb.SetMinDelay (float (ms)); }
	void setCombMaxDelay (double ms) { comb.SetMaxDelay (float (ms)); }

	void setSampleRate (double rate)
	{
		sampleRate = rate;
		crusher.SetSampleRate (float (rate));
		comb.SetSampleRate (float (rate));
	}

	using StereoBuffer = std::array<const float*, 2>;
	using StereoOutBuffer = std::array<float*, 2>;
	struct TimeInfo
	{
		double tempo {120};
		double numerator {4};
		double denominator {4};
		double ppqPos {0};
		bool playing {false};
		bool transportChanged {false};
	};

	std::pair<float, float> process (StereoBuffer inputs, StereoOutBuffer outputs,
	                                 uint32_t numSamples, const TimeInfo& timeInfo) noexcept
	{
		auto numSamplesD = static_cast<double> (numSamples);
		auto subDivNumerator = static_cast<double> (subDiv) / timeInfo.numerator;
		auto ppqBlockDuration = (numSamplesD / sampleRate) * (timeInfo.tempo / 60.0);
		auto divPerSample =
		    subDivNumerator * ppqBlockDuration * (timeInfo.denominator / 4.0) / numSamplesD;
		auto position = subDivNumerator * timeInfo.ppqPos * (timeInfo.denominator / 4.0);
		auto ref = 0.0;

		bbcutter.SetTimeInfos (timeInfo.tempo, timeInfo.numerator, timeInfo.denominator,
		                       sampleRate);

		if (timeInfo.transportChanged && timeInfo.playing)
		{
			lastPositionInMeasure =
			    static_cast<int32_t> (std::floor (std::fmod (position - ref, subDiv)));
			bbcutter.SetPosition (static_cast<int32_t> (std::floor ((position - ref) / subDiv)),
			                      lastPositionInMeasure);
		}
		else if (std::abs (position - expectedPosition) > 0.5)
		{
			// the host timeline jumped (loop wrap, playhead relocate) without a
			// transport edge: restart phrasing from here, and force a position
			// event on the first sample so short loops keep producing cuts.
			// expectedPosition starts as NaN, so the first block never triggers
			bbcutter.Reset();
			lastPositionInMeasure = -1;
		}

		std::pair<float, float> peak = {0.f, 0.f};

		for (uint32_t i = 0; i < numSamples; i++)
		{
			int32_t positionInMeasure = int32_t (std::floor (std::fmod (position - ref, subDiv)));
			int32_t measure = int32_t (std::floor ((position - ref) / subDiv));

			// lastPositionInMeasure persists across process calls: a bar/unit
			// boundary landing exactly on a block boundary must still be
			// reported, or BBCutter never sees sd==0 and stops starting phrases
			if (positionInMeasure != lastPositionInMeasure)
			{
				bbcutter.SetPosition (measure, positionInMeasure);
				lastPositionInMeasure = positionInMeasure;
			}

			float l = 0.f;
			float r = 0.f;
			player.tick (l, r, inputs[0][i], inputs[1][i]);
			crusher.tick (l, r, l, r);
			comb.tick (l, r, l, r);
			outputs[0][i] = l;
			outputs[1][i] = r;
			auto absL = std::abs (l);
			auto absR = std::abs (r);
			if (absL > peak.first)
				peak.first = absL;
			if (absR > peak.second)
				peak.second = absR;
			position += divPerSample;
		}
		expectedPosition = position;
		return peak;
	}

	uint32_t getPhraseCount () const { return phraseCount.load (std::memory_order_relaxed); }
	uint32_t getBlockCount () const { return blockCount.load (std::memory_order_relaxed); }
	uint32_t getUnitCount () const { return unitCount.load (std::memory_order_relaxed); }
	uint32_t getCutCount () const { return cutCount.load (std::memory_order_relaxed); }

private:
	void OnPhrase (long, long) override { phraseCount.fetch_add (1, std::memory_order_relaxed); }
	void OnBlock (long, long) override { blockCount.fetch_add (1, std::memory_order_relaxed); }
	void OnUnit (long, long) override { unitCount.fetch_add (1, std::memory_order_relaxed); }
	void OnCut (long, long) override { cutCount.fetch_add (1, std::memory_order_relaxed); }

	LivePlayer player;
	BitCrusher crusher;
	Comb comb;
	BBCutter bbcutter;

	double sampleRate {44100.};
	uint32_t subDiv {8};
	int32_t lastPositionInMeasure {-1};
	double expectedPosition {std::numeric_limits<double>::quiet_NaN()};

	std::atomic<uint32_t> phraseCount {0};
	std::atomic<uint32_t> blockCount {0};
	std::atomic<uint32_t> unitCount {0};
	std::atomic<uint32_t> cutCount {0};
};

//------------------------------------------------------------------------
} // namespace Livecut
