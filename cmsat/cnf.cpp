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

vector<Lit> CNF::get_lits(const Watched& ws, Lit origLit) const
{
    vector<Lit> lits;
    switch(ws.getType()) {
        case watch_binary_t:
            lits.push_back(origLit);
            lits.push_back(ws.lit2());
            break;

        case CMSat::watch_tertiary_t:
            lits.push_back(origLit);
            lits.push_back(ws.lit2());
            lits.push_back(ws.lit3());
            break;

        case watch_clause_t: {
            const Clause* cl = clAllocator->getPointer(ws.getOffset());
            for(const Lit lit: *cl) {
                lits.push_back(lit);
            }
            break;
        }

        default:
            assert(false);
            break;
    }

    return lits;
}

size_t CNF::cl_size(const Watched& ws) const
{
    switch(ws.getType()) {
        case watch_binary_t:
            return 2;
            break;

        case CMSat::watch_tertiary_t:
            return 3;
            break;

        case watch_clause_t: {
            const Clause* cl = clAllocator->getPointer(ws.getOffset());
            return cl->size();
            break;
        }

        default:
            assert(false);
            return 0;
            break;
    }
}

string CNF::watched_to_string(Lit otherLit, const Watched& ws) const
{
    std::stringstream ss;
    switch(ws.getType()) {
        case watch_binary_t:
            ss << otherLit << ", " << ws.lit2();
            break;

        case CMSat::watch_tertiary_t:
            ss << otherLit << ", " << ws.lit2() << ", " << ws.lit3();
            break;

        case watch_clause_t: {
            const Clause* cl = clAllocator->getPointer(ws.getOffset());
            for(size_t i = 0; i < cl->size(); i++) {
                ss << (*cl)[i];
                if (i < cl->size()-1)
                    ss << ", ";
            }
            break;
        }

        default:
            assert(false);
            break;
    }

    return ss.str();
}


bool ClauseSizeSorter::operator () (const ClOffset x, const ClOffset y)
{
    Clause* cl1 = clAllocator->getPointer(x);
    Clause* cl2 = clAllocator->getPointer(y);
    return (cl1->size() < cl2->size());
}
