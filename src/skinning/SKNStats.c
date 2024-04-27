#include <dolphin.h>
#include "skinning.h"

sStats SKNStatistics[6];

char *StatNames[6] = {
    "SKN OUTER LOOP ",
    "SKN SK1 ",
    "SKN SK2 ",
    "SKN SKACC ",
    "SKN_BZERO ",
    "SKN_FLUSH "
};

void SKNPrintStats(void) {
    u32 i;
    u32 total = 0; // doesnt appear to be used?
    u32 totalxform = 0; // doesnt appear to be used?

    OSReport("PMC counts");
    for(i = 0; i < 6; i++) {
        OSReport("%s (%9d hits)\n", StatNames[i], SKNStatistics[i].hits);
        OSReport("   cycles	%9d	 avg %9d	 frame pct %2.2f\n", SKNStatistics[i].cycleTotal, SKNStatistics[i].cycleTotal / SKNStatistics[i].hits, 100.0f * SKNStatistics[i].cycleTotal / SKNStatistics[i].hits / 6750000);
        OSReport("   loadStores	%9d	 avg %9d\n", SKNStatistics[i].loadStoreTotal, SKNStatistics[i].loadStoreTotal / SKNStatistics[i].hits);
        OSReport("   missCycles	%9d	 avg %9d pct %2.2f\n", SKNStatistics[i].missCycleTotal, SKNStatistics[i].missCycleTotal / SKNStatistics[i].hits, 100.0f * SKNStatistics[i].missCycleTotal / SKNStatistics[i].cycleTotal);
        OSReport("   instructions	%9d	 avg %9d\n", SKNStatistics[i].instructionTotal, SKNStatistics[i].instructionTotal / SKNStatistics[i].hits);
    }
}
