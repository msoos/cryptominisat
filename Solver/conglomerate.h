#ifndef __CONGLOMERATE_H__
#define __CONGLOMERATE_H__

#include <vector>
#include <map>
#include "clause.h"

using std::vector;
using std::pair;
using std::map;

class Solver;

class Conglomerate
{
public:
    uint conglomerateXors(Solver* S); ///<Conglomerate XOR-s that are attached using a variable
    static void doCalcAtFinish(Solver* S); ///<Calculate variables removed during conglomeration
    
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
    map<Var, Lit> toReplace;
    
    Solver* S;
};

#endif //__CONGLOMERATE_H__
