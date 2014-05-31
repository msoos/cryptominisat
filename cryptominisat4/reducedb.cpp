#include "reducedb.h"
#include "solver.h"
#include "sqlstats.h"
#include "clausecleaner.h"
#include "completedetachreattacher.h"

using namespace CMSat;

struct MySorter
{
    virtual ~MySorter()
    {}

    virtual bool operator()(const ClOffset, const ClOffset) const
    {
        return false;
    }
};

struct SortRedClsGlue: public MySorter
{
    SortRedClsGlue(ClauseAllocator& _clAllocator) :
        clAllocator(_clAllocator)
    {}
    ClauseAllocator& clAllocator;

    bool operator () (const ClOffset xOff, const ClOffset yOff) const override
    {
        const Clause* x = clAllocator.getPointer(xOff);
        const Clause* y = clAllocator.getPointer(yOff);

        const uint32_t xsize = x->size();
        const uint32_t ysize = y->size();

        //No clause should be less than 3-long: 2&3-long are not removed
        assert(xsize > 2 && ysize > 2);

        //First tie: glue
        if (x->stats.glue > y->stats.glue) return 1;
        if (x->stats.glue < y->stats.glue) return 0;

        //Second tie: size
        return xsize > ysize;
    }
};

struct SortRedClsSize: public MySorter
{
    SortRedClsSize(ClauseAllocator& _clAllocator) :
        clAllocator(_clAllocator)
    {}
    ClauseAllocator& clAllocator;

    bool operator () (const ClOffset xOff, const ClOffset yOff) const override
    {
        const Clause* x = clAllocator.getPointer(xOff);
        const Clause* y = clAllocator.getPointer(yOff);

        const uint32_t xsize = x->size();
        const uint32_t ysize = y->size();

        //No clause should be less than 3-long: 2&3-long are not removed
        assert(xsize > 2 && ysize > 2);

        //First tie: size
        if (xsize > ysize) return 1;
        if (xsize < ysize) return 0;

        //Second tie: glue
        return x->stats.glue > y->stats.glue;
    }
};
struct SortRedClsAct: public MySorter
{
    SortRedClsAct(ClauseAllocator& _clAllocator) :
        clAllocator(_clAllocator)
    {}
    ClauseAllocator& clAllocator;

    bool operator () (const ClOffset xOff, const ClOffset yOff) const override
    {
        const Clause* x = clAllocator.getPointer(xOff);
        const Clause* y = clAllocator.getPointer(yOff);

        const uint32_t xsize = x->size();
        const uint32_t ysize = y->size();

        //No clause should be less than 3-long: 2&3-long are not removed
        assert(xsize > 2 && ysize > 2);

        //First tie: activity
        if (x->stats.activity < y->stats.activity) return 1;
        if (x->stats.activity > y->stats.activity) return 0;

        //Second tie: size
        return xsize > ysize;
    }
};
struct SortRedClsPropConfl: public MySorter
{
    SortRedClsPropConfl(
        ClauseAllocator& _clAllocator
        , uint64_t _confl_multiplier
    ) :
        clAllocator(_clAllocator)
        , confl_multiplier(_confl_multiplier)
    {}
    ClauseAllocator& clAllocator;
    uint64_t confl_multiplier;

    bool operator () (const ClOffset xOff, const ClOffset yOff) const override
    {
        const Clause* x = clAllocator.getPointer(xOff);
        const Clause* y = clAllocator.getPointer(yOff);

        const uint32_t xsize = x->size();
        const uint32_t ysize = y->size();

        //No clause should be less than 3-long: 2&3-long are not removed
        assert(xsize > 2 && ysize > 2);

        const uint64_t x_useful = x->stats.numPropAndConfl(confl_multiplier);
        const uint64_t y_useful = y->stats.numPropAndConfl(confl_multiplier);
        if (x_useful != y_useful)
            return x_useful < y_useful;

        //Second tie: UIP usage
        if (x->stats.used_for_uip_creation != y->stats.used_for_uip_creation)
            return x->stats.used_for_uip_creation < y->stats.used_for_uip_creation;

        return x->size() > y->size();
    }
};
struct SortRedClsConflDepth : public MySorter
{
    SortRedClsConflDepth(ClauseAllocator& _clAllocator) :
        clAllocator(_clAllocator)
    {}
    ClauseAllocator& clAllocator;

    bool operator () (const ClOffset xOff, const ClOffset yOff) const override
    {
        const Clause* x = clAllocator.getPointer(xOff);
        const Clause* y = clAllocator.getPointer(yOff);

        const uint32_t xsize = x->size();
        const uint32_t ysize = y->size();

        //No clause should be less than 3-long: 2&3-long are not removed
        assert(xsize > 2 && ysize > 2);

        if (x->stats.numPropAndConfl(1) == 0 && y->stats.numPropAndConfl(1) == 0)
            return false;

        if (x->stats.numPropAndConfl(1) == 0)
            return true;
        if (y->stats.numPropAndConfl(1) == 0)
            return false;

        const double x_useful = x->stats.confl_usefulness();
        const double y_useful = y->stats.confl_usefulness();
        if (x_useful != y_useful)
            return x_useful < y_useful;

        //Second tie: UIP usage
        if (x->stats.used_for_uip_creation != y->stats.used_for_uip_creation)
            return x->stats.used_for_uip_creation < y->stats.used_for_uip_creation;

        return x->size() > y->size();
    }
};

ReduceDB::ReduceDB(Solver* _solver) :
    solver(_solver)
{
}

void ReduceDB::sort_red_cls(CleaningStats& tmpStats, ClauseCleaningTypes clean_type)
{
    MySorter* my_sorter;
    switch (clean_type) {
        case clean_glue_based : {
            my_sorter = new SortRedClsGlue(solver->clAllocator);
            tmpStats.glueBasedClean = 1;
            break;
        }

        case clean_size_based : {
            my_sorter = new SortRedClsSize(solver->clAllocator);
            tmpStats.sizeBasedClean = 1;
            break;
        }

        case clean_sum_activity_based : {
            my_sorter = new SortRedClsAct(solver->clAllocator);
            tmpStats.actBasedClean = 1;
            break;
        }

        case clean_sum_prop_confl_based : {
            uint64_t multiplier = solver->conf.clean_confl_multiplier;
            my_sorter = new SortRedClsPropConfl(solver->clAllocator, multiplier);
            tmpStats.propConflBasedClean = 1;
            break;
        }

        case clean_sum_confl_depth_based : {
            my_sorter = new SortRedClsGlue(solver->clAllocator);
            tmpStats.propConflBasedClean = 1;
            break;
        }

        default: {
            assert(false);
        }
    }

    std::sort(solver->longRedCls.begin(), solver->longRedCls.end(), *my_sorter);
    delete my_sorter;
}

void ReduceDB::print_best_red_clauses_if_required() const
{
    if (solver->longRedCls.empty()
        || solver->conf.doPrintBestRedClauses == 0
    ) {
        return;
    }

    size_t at = 0;
    for(long i = ((long)solver->longRedCls.size())-1
        ; i > ((long)solver->longRedCls.size())-1-solver->conf.doPrintBestRedClauses && i >= 0
        ; i--
    ) {
        ClOffset offset = solver->longRedCls[i];
        const Clause* cl = solver->clAllocator.getPointer(offset);
        cout
        << "c [best-red-cl] Red " << nbReduceDB
        << " No. " << at << " > "
        << solver->clauseBackNumbered(*cl)
        << endl;

        at++;
    }
}

CleaningStats ReduceDB::reduceDB(bool lock_clauses_in)
{
    //Clean the clause database before doing cleaning
    //varReplacer->performReplace();
    solver->clauseCleaner->removeAndCleanAll();

    const double myTime = cpuTime();
    nbReduceDB++;
    CleaningStats tmpStats;
    tmpStats.origNumClauses = solver->longRedCls.size();
    tmpStats.origNumLits = solver->litStats.redLits;
    uint64_t removeNum = calc_how_many_to_remove();
    uint64_t sumConfl = solver->sumConflicts();

    //Complete detach&reattach of OK clauses will be *much* faster
    CompleteDetachReatacher detachReattach(solver);
    detachReattach.detachNonBinsNonTris();

    if (lock_clauses_in) {
        lock_most_UIP_used_clauses();
    }

    tmpStats.clauseCleaningType = solver->conf.clauseCleaningType;
    sort_red_cls(tmpStats, solver->conf.clauseCleaningType);
    print_best_red_clauses_if_required();
    real_clean_clause_db(tmpStats, sumConfl, removeNum);

    if (lock_clauses_in) {
        lock_in_top_N_uncleaned();
    }

    //Reattach what's left
    detachReattach.reattachLongs();

    tmpStats.cpu_time = cpuTime() - myTime;
    if (solver->conf.verbosity >= 3)
        tmpStats.print(1);
    else if (solver->conf.verbosity >= 1) {
        tmpStats.printShort();
    }
    cleaningStats += tmpStats;

    if (solver->conf.doSQL) {
        solver->sqlStats->time_passed_min(
            solver
            , "dbclean"
            , tmpStats.cpu_time
        );
    }

    last_reducedb_num_conflicts = solver->sumConflicts();
    return tmpStats;
}

void ReduceDB::lock_in_top_N_uncleaned()
{
    size_t locked = 0;
    size_t skipped = 0;

    long cutoff = (long)solver->longRedCls.size() - (long)solver->conf.lock_topclean_per_dbclean;
    for(long i = (long)solver->longRedCls.size()-1
        ; i >= 0 && i >= cutoff
        ; i--
    ) {
        const ClOffset offs = solver->longRedCls[i];
        Clause& cl = *solver->clAllocator.getPointer(offs);
        if (!cl.stats.locked) {
            cl.stats.locked = true;
            locked++;
        } else {
            skipped++;
        }
    }

    if (solver->conf.verbosity >= 2) {
        cout
        << "c [DBclean] TOP uncleaned"
        << " locked: " << locked << " skipped: " << skipped << endl;
    }
}

void ReduceDB::lock_most_UIP_used_clauses()
{
    if (solver->conf.lock_uip_per_dbclean == 0)
        return;

    std::function<bool (const ClOffset, const ClOffset)> uipsort
        = [&] (const ClOffset a, const ClOffset b) -> bool {
            const Clause& a_cl = *solver->clAllocator.getPointer(a);
            const Clause& b_cl = *solver->clAllocator.getPointer(b);

            return a_cl.stats.used_for_uip_creation > b_cl.stats.used_for_uip_creation;
    };
    std::sort(solver->longRedCls.begin(), solver->longRedCls.end(), uipsort);

    size_t locked = 0;
    size_t skipped = 0;
    for(size_t i = 0
        ; i < solver->longRedCls.size() && locked < solver->conf.lock_uip_per_dbclean
        ; i++
    ) {
        const ClOffset offs = solver->longRedCls[i];
        Clause& cl = *solver->clAllocator.getPointer(offs);
        if (!cl.stats.locked) {
            cl.stats.locked = true;
            locked++;
        } else {
            skipped++;
        }
    }

    if (solver->conf.verbosity >= 2) {
        cout << "c [DBclean] UIP"
        << " Locked: " << locked << " skipped: " << skipped << endl;
    }
}

bool ReduceDB::red_cl_too_young(const Clause* cl)
{
    return cl->stats.introduced_at_conflict + solver->conf.min_time_in_db_before_eligible_for_cleaning
            >= solver->sumConflicts();
}

bool ReduceDB::red_cl_introduced_since_last_reducedb(const Clause* cl)
{
    return cl->stats.introduced_at_conflict >= last_reducedb_num_conflicts;
}

void ReduceDB::real_clean_clause_db(
    CleaningStats& tmpStats
    , uint64_t sumConfl
    , uint64_t removeNum
) {
    size_t i, j;
    for (i = j = 0
        ; i < solver->longRedCls.size() && tmpStats.removed.num < removeNum
        ; i++
    ) {
        ClOffset offset = solver->longRedCls[i];
        Clause* cl = solver->clAllocator.getPointer(offset);
        assert(cl->size() > 3);

        //Don't delete if not aged long enough or locked
        if (red_cl_too_young(cl)
             || (solver->conf.dont_remove_fresh_glue2 && cl->stats.glue == 2 && red_cl_introduced_since_last_reducedb(cl))
             || cl->stats.locked
        ) {
            solver->longRedCls[j++] = offset;
            tmpStats.remain.incorporate(cl);
            tmpStats.remain.age += sumConfl - cl->stats.introduced_at_conflict;
            continue;
        }

        //Stats Update
        tmpStats.removed.incorporate(cl);
        tmpStats.removed.age += sumConfl - cl->stats.introduced_at_conflict;

        //free clause
        *solver->drup << del << *cl << fin;
        solver->clAllocator.clauseFree(offset);
    }

    //Count what is left
    for (; i < solver->longRedCls.size(); i++) {
        ClOffset offset = solver->longRedCls[i];
        Clause* cl = solver->clAllocator.getPointer(offset);

        //Stats Update
        tmpStats.remain.incorporate(cl);
        tmpStats.remain.age += sumConfl - cl->stats.introduced_at_conflict;

        if (cl->stats.introduced_at_conflict > sumConfl) {
            cout
            << "c DEBUG: solver->conflict introduction numbers are wrong."
            << " according to CL, introduction: " << cl->stats.introduced_at_conflict
            << " but we think max solver->confl: "  << sumConfl
            << endl;
        }
        assert(cl->stats.introduced_at_conflict <= sumConfl);

        solver->longRedCls[j++] = offset;
    }

    //Resize long redundant clause array
    solver->longRedCls.resize(solver->longRedCls.size() - (i - j));
}

uint64_t ReduceDB::calc_how_many_to_remove()
{
    //Calculate how many to remove
    uint64_t origRemoveNum = (double)solver->longRedCls.size() *solver->conf.ratioRemoveClauses;

    //If there is a ratio limit, and we are over it
    //then increase the removeNum accordingly
    double maxToHave = (double)(solver->longIrredCls.size() + solver->binTri.irredTris + solver->nVars() + 300ULL)
        * (double)nbReduceDB
        * solver->conf.maxNumRedsRatio;

    //To guard against infinity and undefined cast to integer
    if (maxToHave > 1000.0*1000.0*1000.0) {
        maxToHave = 1000.0*1000.0*1000.0;
    }
    uint64_t removeNum = std::max<long long>((long long)origRemoveNum, (long long)solver->longRedCls.size()-(long long)maxToHave);

    if (removeNum != origRemoveNum) {
        if (solver->conf.verbosity >= 2) {
            cout
            << "c [DBclean] Hard upper limit reached, removing more than normal: "
            << origRemoveNum << " --> " << removeNum
            << endl;
        }
    } else {
        if (solver->conf.verbosity >= 2) {
        cout
        << "c [DBclean] Hard long cls limit would be: " << maxToHave/1000 << "K"
        << endl;
        }
    }

    return removeNum;
}

void ReduceDB::reduce_db_and_update_reset_stats(bool lock_clauses_in)
{
    ClauseUsageStats irred_cl_usage_stats = sumClauseData(solver->longIrredCls, false);
    ClauseUsageStats red_cl_usage_stats = sumClauseData(solver->longRedCls, true);
    ClauseUsageStats sum_cl_usage_stats;
    sum_cl_usage_stats += irred_cl_usage_stats;
    sum_cl_usage_stats += red_cl_usage_stats;
    if (solver->conf.verbosity >= 1) {
        cout << "c irred";
        irred_cl_usage_stats.print();

        cout << "c red  ";
        red_cl_usage_stats.print();

        cout << "c sum  ";
        sum_cl_usage_stats.print();
    }

    CleaningStats iterCleanStat = reduceDB(lock_clauses_in);
    solver->consolidateMem();

    if (solver->conf.doSQL) {
        solver->sqlStats->reduceDB(irred_cl_usage_stats, red_cl_usage_stats, iterCleanStat, solver);
    }

    if (solver->conf.doClearStatEveryClauseCleaning) {
        solver->clear_clauses_stats();
    }

    nextCleanLimit += nextCleanLimitInc;
    nextCleanLimitInc *= solver->conf.increaseClean;
}

ClauseUsageStats ReduceDB::sumClauseData(
    const vector<ClOffset>& toprint
    , const bool red
) const {
    vector<ClauseUsageStats> perSizeStats;
    vector<ClauseUsageStats> perGlueStats;

    //Reset stats
    ClauseUsageStats stats;

    for(ClOffset offset: toprint) {
        Clause& cl = *solver->clAllocator.getPointer(offset);
        const uint32_t clause_size = cl.size();

        //We have stats on this clause
        if (cl.size() == 3)
            continue;

        stats.addStat(cl);

        //Update size statistics
        if (perSizeStats.size() < cl.size() + 1U)
            perSizeStats.resize(cl.size()+1);

        perSizeStats[clause_size].addStat(cl);

        //If redundant, sum up GLUE-based stats
        if (red) {
            const size_t glue = cl.stats.glue;
            assert(glue != std::numeric_limits<uint32_t>::max());
            if (perSizeStats.size() < glue + 1) {
                perSizeStats.resize(glue + 1);
            }

            perSizeStats[glue].addStat(cl);
        }

        if (solver->conf.verbosity >= 4)
            cl.print_extra_stats();
    }

    //Print more stats
    /*if (solver->conf.verbosity >= 4) {
        solver->printPropConflStats("clause-len", perSizeStats);

        if (red) {
            solver->printPropConflStats("clause-glue", perGlueStats);
        }
    }*/

    return stats;
}

void ReduceDB::increment_next_clean_limit()
{
    if (solver->conf.startClean < 100) {
        cout << "SolverConf::startclean must be at least 100. Option on command line is '--startclean'" << endl;
        exit(-1);
    }

    nextCleanLimitInc = solver->conf.startClean;
    nextCleanLimit = solver->sumConflicts() + nextCleanLimitInc;
}

void ReduceDB::reset_next_clean_limit()
{
    nextCleanLimitInc = 0;
    nextCleanLimit = 0;
}
