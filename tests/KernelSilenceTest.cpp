/*
 This file is part of LiveCut Enhanced.

 Regression test for the "audio stops being generated" bug: BBCutter only
 restarts a phrase on a unit event with sd == 0 (bar start). If a bar
 boundary falls exactly on a process-block boundary it must still be
 reported to the cutter, otherwise no new phrase ever starts and the
 plugin outputs silence forever.

 Two real-world triggers are simulated:
   1. A DAW arrangement loop where the host splits buffers at the loop
      wrap (Ableton Live), so each post-wrap block starts exactly on a
      bar line.
   2. Linear playback where samplesPerBar is an exact multiple of the
      block size (e.g. 120 bpm @ 48 kHz with a 256-sample buffer).

 Livecut can be redistributed and/or modified under the terms of the
 GNU General Public License, as published by the Free Software Foundation;
 either version 2 of the License, or (at your option) any later version.
 */

#include "Kernel.h"

#include <cmath>
#include <cstdio>

namespace
{

    struct Result
    {
        double maxSilentGapBars;
        double silentTailBars;
        uint32_t phrases;
    };

    Result run(int cutproc, bool loop, double loopStartPpq, double loopLenPpq,
               double minutes, int blockSize)
    {
        Livecut::Kernel kernel;
        kernel.setSampleRate(48000.0);
        kernel.setSubDiv(8);
        kernel.setCutProc(cutproc);
        kernel.setSeed(12345);

        const double sr = 48000.0, tempo = 120.0;
        const double ppqPerSample = (tempo / 60.0) / sr;

        static float inL[8192], inR[8192], outL[8192], outR[8192];
        for (int i = 0; i < 8192; ++i)
            inL[i] = inR[i] = 1.0f;

        double ppq = loopStartPpq, absPpq = 0.0, lastAudible = 0.0, maxGap = 0.0;
        long samplesToRun = (long)(minutes * 60.0 * sr);
        bool first = true;

        while (samplesToRun > 0)
        {
            int n = blockSize;
            if (loop)
            {
                // exact split like a real host: the pre-wrap block ends exactly
                // at the loop end; the post-wrap block starts exactly at loop start
                double left = (loopStartPpq + loopLenPpq) - ppq;
                int samplesLeft = (int)std::floor(left / ppqPerSample + 1e-9);
                if (samplesLeft < n)
                    n = samplesLeft;
                if (n <= 0)
                {
                    ppq = loopStartPpq;
                    continue;
                }
            }

            Livecut::Kernel::TimeInfo ti;
            ti.tempo = tempo;
            ti.numerator = 4;
            ti.denominator = 4;
            ti.ppqPos = ppq;
            ti.playing = true;
            ti.transportChanged = first;
            first = false;

            auto peak = kernel.process({inL, inR}, {outL, outR}, (uint32_t)n, ti);
            if (peak.first > 0.f || peak.second > 0.f)
            {
                maxGap = std::max(maxGap, absPpq - lastAudible);
                lastAudible = absPpq;
            }

            ppq += n * ppqPerSample;
            absPpq += n * ppqPerSample;
            if (loop && ppq >= loopStartPpq + loopLenPpq - 1e-9)
                ppq = loopStartPpq;

            samplesToRun -= n;
        }

        return {maxGap / 4.0, (absPpq - lastAudible) / 4.0, kernel.getPhraseCount()};
    }

    int failures = 0;

    void expectAlive(const char *name, Result r)
    {
        // the cutter legitimately rests up to ~1 bar between phrases; anything
        // much longer means it stopped generating cuts
        const double limitBars = 4.0;
        const bool ok = r.maxSilentGapBars < limitBars && r.silentTailBars < limitBars;
        std::printf("%-52s maxGap=%7.1f bars  silentTail=%7.1f bars  phrases=%4u  %s\n",
                    name, r.maxSilentGapBars, r.silentTailBars, r.phrases,
                    ok ? "OK" : "FAIL");
        if (!ok)
            ++failures;
    }

} // namespace

int main()
{
    const char *procName[] = {"CutProc11", "WarpCut", "SQPusher"};
    char buf[128];

    for (int proc = 0; proc <= 2; ++proc)
    {
        std::snprintf(buf, sizeof buf, "[%s] 1-bar loop, host splits at wrap", procName[proc]);
        expectAlive(buf, run(proc, true, 0.0, 4.0, 10.0, 512));

        std::snprintf(buf, sizeof buf, "[%s] 4-bar loop, host splits at wrap", procName[proc]);
        expectAlive(buf, run(proc, true, 0.0, 16.0, 10.0, 512));

        std::snprintf(buf, sizeof buf, "[%s] linear, bar == multiple of block (256)", procName[proc]);
        expectAlive(buf, run(proc, false, 0.0, 0.0, 10.0, 256));

        std::snprintf(buf, sizeof buf, "[%s] linear, unaligned block (448)", procName[proc]);
        expectAlive(buf, run(proc, false, 0.0, 0.0, 10.0, 448));

        std::snprintf(buf, sizeof buf, "[%s] 1-beat loop at bar start", procName[proc]);
        expectAlive(buf, run(proc, true, 0.0, 1.0, 10.0, 512));

        std::snprintf(buf, sizeof buf, "[%s] 2-beat loop at beat 2 (no bar line inside)", procName[proc]);
        expectAlive(buf, run(proc, true, 1.0, 2.0, 10.0, 512));
    }

    std::printf(failures == 0 ? "\nALL PASS\n" : "\n%d FAILURE(S)\n", failures);
    return failures == 0 ? 0 : 1;
}
