#ifndef CONGLOMERATE_H
#define CONGLOMERATE_H

#include <vector>
#include <map>
#include <set>
#include "Clause.h"
#include "VarReplacer.h"

using std::vector;
using std::pair;
using std::map;
using std::set;

class Solver;

class Conglomerate
{
public:
    Conglomerate(Solver *S);
    ~Conglomerate();
    uint conglomerateXors(); ///<Conglomerate XOR-s that are attached using a variable
    void addRemovedClauses(); ///<Add clauses that have been removed. Used if solve() is called multiple times
    void doCalcAtFinish(); ///<Calculate variables removed during conglomeration
    const vec<XorClause*>& getCalcAtFinish() const;
    vec<XorClause*>& getCalcAtFinish();
    const vector<bool>& getRemovedVars() const;
    void newVar();
    
private:
    
    void process_clause(XorClause& x, const uint num, uint var, vec<Lit>& vars);
    void fillVarToXor();
    void clearDouble(vec<Lit>& ps) const;
    void clearToRemove();
    void clearLearntsFromToRemove();
    bool dealWithNewClause(vec<Lit>& ps, const bool inverted, const uint old_group);
    
    typedef map<uint, vector<pair<XorClause*, uint> > > varToXorMap;
    varToXorMap varToXor; 
    vector<bool> blocked;
    vector<bool> toRemove;
    vector<bool> removedVars;
    
    vec<XorClause*> calcAtFinish;
    
    Solver* S;
};

#endif //CONGLOMERATE_H
