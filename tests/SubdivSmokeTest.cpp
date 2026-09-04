/*
 This file is part of LiveCut Enhanced.
 GPL v2 or later.

 Smoke-test every exposed subdivision against all three cut procedures.
 */

#include "Kernel.h"

#include <algorithm>
#include <cstdio>

int main()
{
    static constexpr int subdivisions[] = {2, 4, 6, 8, 12, 16, 18, 24, 32, 64};
    static constexpr const char* procNames[] = {"CutProc11", "WarpCut", "SQPusher"};

    static float inL[512], inR[512], outL[512], outR[512];
    for (int i = 0; i < 512; ++i)
        inL[i] = inR[i] = 1.f;

    int failures = 0;

    for (int proc = 0; proc < 3; ++proc)
    {
        for (auto subdiv : subdivisions)
        {
            Livecut::Kernel kernel;
            kernel.setSampleRate (48000.0);
            kernel.setSubDiv (subdiv);
            kernel.setCutProc (proc);
            kernel.setSeed (12345);

            double ppq = 0.0;
            long samplesLeft = 48000L * 8L;
            int audibleBlocks = 0;
            bool first = true;

            while (samplesLeft > 0)
            {
                const auto n = int (std::min<long> (512, samplesLeft));
                Livecut::Kernel::TimeInfo ti;
                ti.tempo = 120.0;
                ti.numerator = 4.0;
                ti.denominator = 4.0;
                ti.ppqPos = ppq;
                ti.playing = true;
                ti.transportChanged = first;
                first = false;

                const auto peak = kernel.process ({inL, inR}, {outL, outR}, uint32_t (n), ti);
                if (peak.first > 0.f || peak.second > 0.f)
                    ++audibleBlocks;

                ppq += double (n) * (120.0 / 60.0) / 48000.0;
                samplesLeft -= n;
            }

            const auto ok = kernel.getPhraseCount() > 0 && audibleBlocks > 4;
            std::printf ("%-10s subdiv=%2d phrases=%4u audibleBlocks=%4d %s\n",
                         procNames[proc], subdiv, kernel.getPhraseCount(), audibleBlocks,
                         ok ? "OK" : "FAIL");
            if (! ok)
                ++failures;
        }
    }

    std::printf (failures == 0 ? "\nALL SUBDIVISIONS PASS\n" : "\n%d FAILURE(S)\n", failures);
    return failures == 0 ? 0 : 1;
}
