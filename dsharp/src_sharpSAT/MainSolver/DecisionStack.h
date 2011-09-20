#ifndef DECISIONSTACK_H
#define DECISIONSTACK_H

#include<vector>

#ifdef DEBUG
#include <assert.h>
#endif

#include <RealNumberTypes.h>

#include "../Basics.h"

#include "InstanceGraph/ComponentTypes.h"
#include "InstanceGraph/AtomsAndNodes.h"
#include "InstanceGraph/InstanceGraph.h"

#include "DecisionTree.h"

using namespace std;

/** \addtogroup Interna*/
/*@{*/


class CDecision
{
    ////////////////////
    /// active Component
    ////////////////////
    unsigned int refCompId;

    // branch
    bool flipped;

    //  bcp
    unsigned int iImpLitOfs;

    //  Solutioncount
    CRealNum  rnNumSols[2];
    
    ////////////////////
    /// decision tree node
    ////////////////////
    DTNode * flipNode;
    

    ////////////////////
    /// remaining Components
    ////////////////////

    /**
    * [iRemCompOfs,iEndRemComps) defines the interval in allComponentsStack
    * where the child components of the [refCompId] component are stored
    */
    unsigned int iRemCompOfs;
    unsigned int iEndRemComps;

    //this function only to be used by decisionStack
    unsigned int countCompsToProcess()
    {
#ifdef DEBUG
        assert(iEndRemComps >= iRemCompOfs);
#endif
        return iEndRemComps - iRemCompOfs;
    }
    void popRemComp()
    {
#ifdef DEBUG
        assert(iEndRemComps >= iRemCompOfs);
#endif
        iEndRemComps--;
    }

public:

    CDecision(DTNode * other)
    {
        flipped = false;
        rnNumSols[0] = 0.0;
        rnNumSols[1] = 0.0;
        refCompId = 0;
        iImpLitOfs = (unsigned int) -1;
        iRemCompOfs = (unsigned int) -1;
        iEndRemComps = (unsigned int) -1;
        
        flipNode = other;
    }

    ~CDecision() {}
    
    DTNode * getDTNode() { return flipNode; }
    DTNode * getOrDTNode() { return flipNode; }
    DTNode * getCurrentDTNode()
    {
        if (isFlipped()) return flipNode->secondNode;
        else return flipNode->firstNode;
    }

    bool isFlipped()
    {
        return flipped;
    }
    bool anotherCompProcessible()
    {
        return getBranchSols() !=0.0 && countCompsToProcess() > 0;
    }

    void includeSol(const CRealNum &rnCodedSols)
    {
        if (rnNumSols[flipped] == 0.0)   rnNumSols[flipped] = rnCodedSols;
        else
            rnNumSols[flipped] *= rnCodedSols;
    }

    const CRealNum &getBranchSols() const
    {
        return rnNumSols[flipped];
    }

    const CRealNum getOverallSols() const
    {
        return rnNumSols[0] + rnNumSols[1];
    }

    friend class CDecisionStack;
};


class CDecisionStack : vector<CDecision>
{
    CInstanceGraph &theClPool;

    vector<LiteralIdT> allImpliedLits;

    vector<CComponentId *> allComponentsStack;
    
    void reactivateTOS();

    // store each cacheEntry where the children of top().refComp are stored
    bool storeTOSCachedChildren();

    unsigned int addToDecLev;
public:

	DTNode * dtMain;
	
    ///
    const vector<CComponentId *> & getAllCompStack()
    {
        return allComponentsStack;
    }


    vector<CComponentId *>::iterator TOSRemComps_begin()
    {
        return allComponentsStack.begin() + top().iRemCompOfs;
    }

    CDecisionStack(CInstanceGraph &pool):theClPool(pool)
    {
        addToDecLev = 0;
    }

    ~CDecisionStack() {}

    //begin for implicit BCP
    void beginTentative()
    {
        addToDecLev = 1;
    }
    void endTentative()
    {
        addToDecLev = 0;
    }
    //end for implicit BCP

    bool pop();
    void push(DTNode * other);

    inline CDecision &top()
    {
        return back();
    }

    bool flipTOS();

    unsigned int countAllImplLits()
    {
        return allImpliedLits.size();
    }

    void shrinkImplLitsTo(unsigned int sz)
    {
        if (sz >= allImpliedLits.size()) return;
        allImpliedLits.resize(sz);
    }

    int TOS_siblingsProcessed()
    {
#ifdef DEBUG
        assert(allComponentsStack.size() >= top().iEndRemComps);
#endif
        return (int) allComponentsStack.size() - (int) top().iEndRemComps;
    }

    unsigned int TOS_countImplLits()
    {
        return allImpliedLits.size() - top().iImpLitOfs;
    }


    const LiteralIdT &TOS_decLit() const
    {
        return  allImpliedLits[back().iImpLitOfs];
    }

    void TOS_addImpliedLit(LiteralIdT aLit)
    {
        allImpliedLits.push_back(aLit);
    }

    void TOS_popImpliedLit()
    {
        allImpliedLits.pop_back();
    }

    vector<LiteralIdT>::const_iterator TOS_ImpliedLits_begin()
    {
        return allImpliedLits.begin() + top().iImpLitOfs;
    }

    vector<LiteralIdT>::const_iterator TOS_ImpliedLits_end()
    {
        return allImpliedLits.end();
    }


    CComponentId & TOSRefComp()
    {
#ifdef DEBUG
        assert(top().refCompId < allComponentsStack.size());
#endif
        return *allComponentsStack[top().refCompId];
    }

    bool TOS_hasAnyRemComp()
    {
        return allComponentsStack.size() > top().iRemCompOfs;
    }

    void TOS_popRemComp()
    {
        top().popRemComp();
    }

    unsigned int TOS_countRemComps()
    {
        return top().countCompsToProcess();
    }

    void TOS_addRemComp()
    {
        allComponentsStack.push_back(new CComponentId());
        top().iEndRemComps = allComponentsStack.size();
    }

    CComponentId & TOS_NextComp()
    {
#ifdef DEBUG
        assert(top().iEndRemComps <= allComponentsStack.size());
        assert(allComponentsStack[top().iEndRemComps -1] != NULL);
#endif
        return *allComponentsStack[top().iEndRemComps - 1];
    }

    void TOS_sortRemComps();

    CComponentId & lastComp()
    {
        return *allComponentsStack.back();
    }

    void printStats();

    inline int getDL() const
    {
        return size()-1+addToDecLev;    // 0 means pre-1st-decision
    }

    void init(unsigned int resSize = 1);
};

/*@}*/

#endif

