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
    
    typedef map<uint, vector<pair<XorClause*, uint> > > varToXorMap;
    varToXorMap varToXor; 
    vector<bool> blocked;
    
    Solver* S;
};

#endif //__CONGLOMERATE_H__
