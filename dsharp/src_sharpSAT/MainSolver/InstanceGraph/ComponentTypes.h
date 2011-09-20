#ifndef COMPONENTTYPES_H
#define COMPONENTTYPES_H

#ifdef DEBUG
#include <assert.h>
#endif


#include <vector>
#include <math.h>

#include <RealNumberTypes.h>

#include "AtomsAndNodes.h"

using namespace std;

//
//  the identifier of the components
//  identifier for (truly) binary clauses are not stored, therefore we need
//  trueClauseCount to kepp track of the number of binary clauses still active in the
//  component, otherwise if theClauses is empty one might suspect that the whole component is
//  empty which would not be true if there where binary clauses in the original formula
//
class CComponentId
{
    /// INVARIANT: content: vvvvv varsSENTINEL
    vector<VarIdT> theVars;
    /// INVARIANT: content: cccccc clsSENTINEL
    vector<ClauseIdT> theClauses;

    unsigned int trueClauseCount;

    long unsigned int hashKeyVars;
    long unsigned int hashKeyCls;

public:

    /// INVARIANT:
    ///  cachedChildren may be nonempty only if cachedAs == NIL_ENTRY
    ///  if cachedAS != NIL_ENTRY then this component has been cached and
    ///  its descendants can be found in xFormulaCache.entry(cachedAs).theDescendants
    unsigned int cachedAs;
    vector<unsigned int> cachedChildren;

    void addCachedChildren(const vector<unsigned int> & cc)
    {
        cachedChildren.insert(cachedChildren.end(),cc.begin(),cc.end());
    }


#define varsSENTINEL  0
#define clsSENTINEL   NOT_A_CLAUSE

    void reserveSpace(unsigned int nVars, unsigned int nCls)
    {
        theVars.reserve(nVars + 1);
        theClauses.reserve(nCls + 1);
    }

    inline void addCl(const ClauseIdT & cl)
    {
        theClauses.push_back(cl);

        //Compute HashKey
        if (cl != clsSENTINEL) hashKeyCls = hashKeyCls*7 + cl;
    }

    inline void addVar(const VarIdT & var)
    {
        theVars.push_back(var);

        //Compute HashKey
        if (var != varsSENTINEL) hashKeyVars = hashKeyVars*5 + var;

    }

    long unsigned int getHashKey() const
    {
        return hashKeyVars + (hashKeyCls<<9);
    }


    void clear()
    {
        theVars.clear();
        theClauses.clear();
        trueClauseCount = 0;
        cachedAs = 0;
    }

    CComponentId()
    {
        clear();
        trueClauseCount = 0;
        cachedAs = 0;
        hashKeyVars = 0;
        hashKeyCls = 0;
    }


    vector<VarIdT>::iterator varsBegin()
    {
        return theVars.begin();
    }


    vector<ClauseIdT>::iterator clsBegin()
    {
        return theClauses.begin();
    }

    inline vector<VarIdT>::const_iterator  varsBegin()  const
    {
        return theVars.begin();
    }

    inline vector<ClauseIdT>::const_iterator  clsBegin()  const
    {
        return theClauses.begin();
    }


    unsigned int countVars()  const
    {
#ifdef DEBUG
        assert(theVars.size() >= 1);
#endif
        return theVars.size() - 1;
    }

    unsigned int countCls() const
    {
#ifdef DEBUG
        assert(theClauses.size() >= 1);
#endif
        return theClauses.size() - 1;
    }

    bool empty() const
    {
        return theVars.empty();// || theClauses.empty();
    }

    int memSize() const
    {
        return theVars.capacity()*sizeof(VarIdT) +
               theClauses.capacity()*sizeof(ClauseIdT);
    }

    void setTrueClauseCount(unsigned int count)
    {
        trueClauseCount = count;
    }

    unsigned int getClauseCount()
    {
#ifdef DEBUG
        assert(trueClauseCount >= countCls());
#endif
        return trueClauseCount;
    }

    bool containsBinCls() const
    {
#ifdef DEBUG
        assert(trueClauseCount >= countCls());
#endif
        return (trueClauseCount > countCls());
    }

};

template <class _T, unsigned int _bitsPerBlock = (sizeof(_T)<<3)>
class CPackedCompId
{
    static unsigned int bpeCls, bpeVars; // bitsperentry
    static unsigned int maskVars,maskCls;
protected:
    vector<_T> theVars;
    vector<_T> theClauses;

public:
    static unsigned int bitsPerVar()
    {
        return bpeVars;
    }
    static unsigned int bitsPerCl()
    {
        return bpeCls;
    }

    unsigned int sizeVarVec()
    {
        return theVars.size();
    }
    unsigned int sizeClVec()
    {
        return theClauses.size();
    }

    static void adjustPackSize(unsigned int maxVarId, unsigned int maxClId);

    CPackedCompId()
    {
    }

    void createFrom(const CComponentId &rComp);


    bool equals(const CPackedCompId<_T> &rComp) const
    {
        return theVars == rComp.theVars
               && theClauses == rComp.theClauses;
    }

    bool equals(const CComponentId &rComp) const;

    void clear()
    {
        theVars.clear();
        theClauses.clear();
    }

    bool empty() const
    {
        return theVars.empty() && theClauses.empty();
    }

    int memSize() const
    {
        return (theVars.capacity() + theClauses.capacity())*sizeof(_T);
    }

};

/////////////////////////////////////////////////////////////////////////////
//BEGIN Implementation CPackedCompId
/////////////////////////////////////////////////////////////////////////////

template <class _T, unsigned int _bitsPerBlock>
unsigned int CPackedCompId<_T,_bitsPerBlock>::bpeCls = 0;
template <class _T, unsigned int _bitsPerBlock>
unsigned int CPackedCompId<_T,_bitsPerBlock>::bpeVars = 0; // bitsperentry
template <class _T, unsigned int _bitsPerBlock>
unsigned int CPackedCompId<_T,_bitsPerBlock>::maskVars = 0;
template <class _T, unsigned int _bitsPerBlock>
unsigned int CPackedCompId<_T,_bitsPerBlock>::maskCls = 0; // bitsperentry

template <class _T, unsigned int _bitsPerBlock>
void CPackedCompId<_T,_bitsPerBlock>::adjustPackSize(unsigned int maxVarId, unsigned int maxClId)
{
    bpeVars = (unsigned int)ceil(log((double)maxVarId+1)/log(2.0));
    bpeCls = (unsigned int)ceil(log((double)maxClId+1)/log(2.0));

    maskVars = maskCls = 0;
    for (unsigned int i=0; i < bpeVars;i++) maskVars = (maskVars<<1) +1;
    for (unsigned int i=0; i < bpeCls;i++) maskCls = (maskCls<<1) +1;
}

template <class _T, unsigned int _bitsPerBlock>
void CPackedCompId<_T,_bitsPerBlock>::createFrom(const CComponentId &rComp)
{
    vector<VarIdT>::const_iterator it;
    vector<ClauseIdT>::const_iterator jt;

    if (!theVars.empty()) theVars.clear();
    if (!theClauses.empty()) theClauses.clear();

    theVars.reserve( rComp.countVars()*bpeVars/_bitsPerBlock +1);
    theClauses.reserve(rComp.countCls()*bpeCls/_bitsPerBlock +1);

    unsigned int bitpos = 0;
    unsigned int h = 0;
    _T * pBack;

    pBack = &theVars.front();

    for (it = rComp.varsBegin(); *it != varsSENTINEL;it++)
    {
        h |= ((*it)<< (bitpos));
        bitpos+= bpeVars;

        if (bitpos >= _bitsPerBlock)
        {
            bitpos -= _bitsPerBlock;
#ifdef DEBUG
            assert(theVars.size() < theVars.capacity());
#endif
            theVars.push_back(h);
            h = ((*it)>> (bpeVars - bitpos));
        }
    }
    if (bitpos > 0) theVars.push_back(h);

    bitpos = 0;
    h = 0;
    pBack = &theClauses.front();
    for (jt = rComp.clsBegin(); *jt != clsSENTINEL;jt++)
    {
        h |= ((*jt)<< (bitpos));
        bitpos += bpeCls;

        if (bitpos >= _bitsPerBlock)
        {
            bitpos -= _bitsPerBlock;
#ifdef DEBUG
            assert(theClauses.size() < theClauses.capacity());
#endif
            theClauses.push_back(h);
            h = ((*jt)>> (bpeCls - bitpos));
        }
    }
    if (bitpos > 0) theClauses.push_back(h);
}


template <class _T, unsigned int _bitsPerBlock>
bool CPackedCompId<_T,_bitsPerBlock>::equals(const CComponentId &rComp) const
{

    if ( (theVars.capacity() !=  (uint) rComp.countVars()*bpeVars/_bitsPerBlock +1)
            || theClauses.capacity() != (uint) rComp.countCls()*bpeCls/_bitsPerBlock +1) return false;

    unsigned int bitpos = 0;
    unsigned int h = 0;
    const _T * pItA;

    pItA = &theVars.front();

    for (vector<VarIdT>::const_iterator it = rComp.varsBegin(); *it != varsSENTINEL;it++)
    {
        h = ((*pItA)>> (bitpos));
        bitpos+= bpeVars;
        if (bitpos >= _bitsPerBlock)
        {
            bitpos -= _bitsPerBlock;
            pItA++;
            h |= ((*pItA)<< (bpeVars - bitpos));
        }
        if (*it != (maskVars & h)) return false;
    }

    bitpos = 0;
    pItA = &theClauses.front();
    for (vector<ClauseIdT>::const_iterator jt = rComp.clsBegin(); *jt != clsSENTINEL;jt++)
    {
        h = (*pItA)>> (bitpos);
        bitpos += bpeCls;

        if (bitpos >= _bitsPerBlock)
        {
            bitpos -= _bitsPerBlock;
            pItA++;
            h |= ((*pItA)<< (bpeCls - bitpos));
        }
        if (*jt != (maskCls & h)) return false;
    }

    return true;


}

/////////////////////////////////////////////////////////////////////////////
//END Implementation CPackedCompId
/////////////////////////////////////////////////////////////////////////////
#endif
