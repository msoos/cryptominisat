#ifndef CONGLOMERATE_H
#define CONGLOMERATE_H

#include <vector>
#include <map>
#include "Clause.h"
#include "VarReplacer.h"

using std::vector;
using std::pair;
using std::map;

class Solver;

class Conglomerate
{
public:
    Conglomerate(Solver *S);
    uint conglomerateXors(); ///<Conglomerate XOR-s that are attached using a variable
    void doCalcAtFinish(); ///<Calculate variables removed during conglomeration
    const vec<XorClause*>& getCalcAtFinish() const;
    vec<XorClause*>& getCalcAtFinish();
    
private:
    
    void process_clause(XorClause& x, const uint num, uint var, vector<Lit>& vars);
    void fillVarToXor();
    void clearDouble(vector<Lit>& ps) const;
    void clearToRemove();
    bool dealWithNewClause(vector<Lit>& ps, const bool inverted, const uint old_group);
    
    typedef map<uint, vector<pair<XorClause*, uint> > > varToXorMap;
    varToXorMap varToXor; 
    vector<bool> blocked;
    vector<bool> toRemove;
    
    vec<XorClause*> calcAtFinish;
    
    Solver* S;
};

#endif //CONGLOMERATE_H
