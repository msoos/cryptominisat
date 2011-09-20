#include "DecisionStack.h"



void CDecisionStack::reactivateTOS()
{
    for (vector<LiteralIdT>::const_iterator it = TOS_ImpliedLits_begin();it != TOS_ImpliedLits_end();it++)
    {
        theClPool.getVar(*it).unsetVal();
    }
}

bool CDecisionStack::flipTOS()
{
    if (getDL() <= 0) return false;
    if (top().isFlipped()) return false; // es wurde schonmal geflippt

    top().flipped = true;

    reactivateTOS();
    allImpliedLits.resize(top().iImpLitOfs);
    storeTOSCachedChildren();
    top().iEndRemComps = top().iRemCompOfs;

    while (allComponentsStack.size() > top().iRemCompOfs)
    {
        delete allComponentsStack.back();
        allComponentsStack.pop_back();
    }
    return true;
}

bool CDecisionStack::storeTOSCachedChildren()
{
    vector<CComponentId *>::iterator it = allComponentsStack.begin() +top().iRemCompOfs;
    for (;it != allComponentsStack.end(); it++)
    {
        if ((*it)->cachedAs != 0) allComponentsStack[top().refCompId]->cachedChildren.push_back((*it)->cachedAs);

    }
    return true;
}

bool CDecisionStack::pop()
{
    if (getDL() <= 0) return false;

    reactivateTOS();
    allImpliedLits.resize(top().iImpLitOfs);
    storeTOSCachedChildren();

    while (allComponentsStack.size() > top().iRemCompOfs)
    {
        delete allComponentsStack.back();
        allComponentsStack.pop_back();
    }

    (end()-2)->includeSol(top().getOverallSols());

    pop_back();
    return true;
}


void CDecisionStack::push(DTNode * other)
{
    push_back(CDecision(other));

    top().refCompId = (end()-2)->iEndRemComps-1;
    (end()-2)->popRemComp();

    top().iImpLitOfs = allImpliedLits.size();
    top().iRemCompOfs = allComponentsStack.size();
    top().iEndRemComps = allComponentsStack.size();
}


void CDecisionStack::init(unsigned int resSize)
{
    clear();
    reserve(resSize);
    allImpliedLits.clear();
    allImpliedLits.reserve(theClPool.countAllVars());
    allComponentsStack.clear();
    allComponentsStack.reserve(theClPool.countAllVars());
    allComponentsStack.push_back(new CComponentId());

    // initialize the stack to contain at least level zero
    DTNode * dummyLeft = new DTNode(DT_AND, 2);
    DTNode * dummyRight = new DTNode(DT_AND, 1);
    dtMain = new DTNode(DT_AND, 0);

    dummyLeft->addParent(dtMain, true);
    dummyRight->addParent(dtMain, true);

    push_back(CDecision(dtMain));
    back().flipped = true;
    top().iRemCompOfs = 1;
    top().iEndRemComps = 1;
    addToDecLev = 0;
}

void CDecisionStack::TOS_sortRemComps()
{
    CComponentId * vBuf;
    vector<CComponentId *> &keys = allComponentsStack;

    int uLower = top().iRemCompOfs;
    int uUpper = top().iEndRemComps-1;


    for (int i = uLower; i <=uUpper;i++)
        for (int j = i+1; j <=uUpper;j++)
        {
            if (keys[i]->countVars() < keys[j]->countVars())
            {
                vBuf = keys[i];
                keys[i] = keys[j];
                keys[j] = vBuf;
            }
        }
}
