#ifndef __CLAUSEDUMPER_H__
#define __CLAUSEDUMPER_H__

#include <ostream>
#include <vector>
#include <limits>
#include "solvertypesmini.h"
#include "cloffset.h"

using std::vector;

namespace CMSat {

class Solver;

class ClauseDumper
{
public:
    ClauseDumper(const Solver* _solver) :
        solver(_solver)
    {}

    void open_file_and_dump_red_clauses(const std::string redDumpFname);
    void open_file_and_dump_irred_clauses(const std::string irredDumpFname);

private:
    const Solver* solver;
    std::ostream* outfile;

    void open_dump_file(std::string filename);
    void dumpBinClauses(
        const bool dumpRed
        , const bool dumpIrred
    );
    void dumpTriClauses(
        const bool alsoRed
        , const bool alsoIrred
    );

    void dumpEquivalentLits();
    void dumpUnitaryClauses();
    void dumpRedClauses(const uint32_t maxSize);
    void dump_clauses(
        const vector<ClOffset>& cls
        , size_t max_size = std::numeric_limits<size_t>::max()
    );

    void dump_blocked_clauses();
    void dump_component_clauses();
    void dumpIrredClauses();

    vector<Lit> tmpCl;

};

}

#endif //__CLAUSEDUMPER_H__
