#include "cnf.h"
#include "solvertypes.h"
#include "clauseallocator.h"

using namespace CMSat;

size_t CNF::print_mem_used_longclauses(const size_t totalMem) const
{
    size_t mem = 0;
    mem += clAllocator->memUsed();
    mem += longIrredCls.capacity()*sizeof(ClOffset);
    mem += longRedCls.capacity()*sizeof(ClOffset);
    printStatsLine("c Mem for longclauses"
        , mem/(1024UL*1024UL)
        , "MB"
        , (double)mem/(double)totalMem*100.0
        , "%"
    );

    return mem;
}

bool CNF::redundant(const Watched& ws) const
{
    return ((ws.isBinary() || ws.isTri()) && ws.red())
            || (ws.isClause() && clAllocator->getPointer(ws.getOffset())->red()
    );
}