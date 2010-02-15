/**************************************************************************************************

VarOrder.h -- (C) Niklas Een, Niklas Sörensson, 2004

ADT for maintaining the variable ordering. It will keep a list of all decision
variables sorted on the current activity.

**************************************************************************************************/

#ifndef VarOrder_h
#define VarOrder_h

#include "SolverTypes.h"
#include "Heap.h"

//=================================================================================================


struct VarOrder_lt {
    const vec<double>&  activity;
    bool operator () (Var x, Var y) { return activity[x] > activity[y]; }
    VarOrder_lt(const vec<double>&  act) : activity(act) { }
};

struct VarOrder {
    const vec<int>&     assigns;     // var->val. Pointer to external assignment table.
    const vec<double>&  activity;    // var->act. Pointer to external activity table.
    Heap<VarOrder_lt>   heap;
    double              random_seed; // For the internal random number generator

//public:
    VarOrder(const vec<int>& ass, const vec<double>& act) :
        assigns(ass), activity(act), heap(VarOrder_lt(act)), random_seed(91648253) {}

    inline void newVar(void);
    inline void update(Var x);                  // Called when variable increased in activity.
    inline void undo(Var x);                    // Called when variable is unassigned and may be selected again.
    inline Var  select(double random_freq =.0); // Selects a new, unassigned variable (or 'var_Undef' if none exists).
    inline Var  peekSelect(void);               // Return next variable that will be selected if 'random_freq' is set to 0.
};


void VarOrder::newVar(void)
{
    heap.setBounds(assigns.size());
    heap.insert(assigns.size()-1);
}


void VarOrder::update(Var x)
{
    if (heap.inHeap(x))
        heap.increase(x);
}


void VarOrder::undo(Var x)
{
    if (!heap.inHeap(x))
        heap.insert(x);
}


Var VarOrder::select(double random_var_freq)
{
    // Random decision:
    if (drand(random_seed) < random_var_freq){
        Var next = irand(random_seed,assigns.size());
        if (lbool(assigns[next]) == l_Undef)
            return next;
    }

    // Activity based decision:
    while (!heap.empty()){
        Var next = heap.getmin();
        if (lbool(assigns[next]) == l_Undef)
            return next;
    }

    return var_Undef;
}

Var VarOrder::peekSelect(void) { return heap.peekmin(); }


//=================================================================================================
// A hack!


class VarOrder2 {
    VarOrder      st, lt;     // Short term, long term.
    const double& act_inc;
public:
    VarOrder2(const vec<int>& ass, const vec<double>& st_act, const vec<double>& lt_act, double& act_inc_) : st(ass, st_act), lt(ass, lt_act), act_inc(act_inc_) {}

    void newVar(void)         { st.newVar() ; lt.newVar() ; }
    void update(Var x)        { st.update(x); lt.update(x); }
    void undo(Var x)          { st.undo(x)  ; lt.undo(x)  ; }
    Var  select(double r = 0) {
        Var x = st.peekSelect();
        return (st.activity[x] / act_inc < 0.2) ? lt.select(r) : st.select();
    }
};


//=================================================================================================
#endif
