#ifndef FORMULACACHE_H
#define FORMULACACHE_H


#ifdef DEBUG
#include<assert.h>
#endif

#include <sys/sysinfo.h>
#include <vector>
#include <iostream>


#include <SomeTime.h>
#include <RealNumberTypes.h>
#include <Interface/AnalyzerData.h>


#include "../Basics.h"
#include "InstanceGraph/ComponentTypes.h"
#include "DecisionStack.h"
#include "DecisionTree.h"
using namespace std;

typedef unsigned int CacheEntryId;

class CCacheEntry : public CPackedCompId<unsigned int>
{
    long unsigned int hashKey;
    friend class CFormulaCache;

    // theFather and theDescendants:
    // each CCacheEntry is a Node in a tree which represents the relationship
    // of the components stored
    CacheEntryId theFather;
    vector<CacheEntryId> theDescendants;

public:

    CRealNum   theVal;
    
    DTNode  *  theDTNode;

    unsigned int score;

    CCacheEntry()
    {
        score = 0;
        theFather = 0;
    }


    void clear()
    {
        theVars.clear();
        theClauses.clear();
        theDescendants.clear();
    }


    ~CCacheEntry()
    {
        theDescendants.clear();
        theFather = 0;
        clear();
    }


    long unsigned int getHashKey() const
    {
        return hashKey;
    }

    void setHashKey(long unsigned int &val)
    {
        hashKey = val;
    }

    unsigned int memSize()
    {
        return  CPackedCompId<unsigned int>::memSize() + theDescendants.capacity()*sizeof(CacheEntryId);
    }

    void substituteDescendant(CacheEntryId iold, CacheEntryId inew)
    {
        vector<CacheEntryId>::iterator it;
        for (it = theDescendants.begin();it != theDescendants.end();it++)
        {
            if (*it == iold) *it = inew;
        }
    }

    void removeDescendant(CacheEntryId d)
    {
        vector<CacheEntryId>::iterator it;
        for (it = theDescendants.end()-1;it != theDescendants.begin()-1;it--)
        {
            if (*it == d)
            {
                it = theDescendants.erase(it);
            }
        }
    }


    void addDescendants(vector<CacheEntryId> &dVec)
    {
        theDescendants.insert(theDescendants.end(),dVec.begin(),dVec.end());
    }

    CacheEntryId getFather()
    {
        return theFather;
    }
    void substituteFather(CacheEntryId newFather)
    {
        theFather = newFather;
    }


};


class CCacheBucket : protected vector<CacheEntryId>
{
    friend class CFormulaCache;


public:

    CCacheBucket()
    {
    }

    ~CCacheBucket()
    {
        clear();
    }

    bool substituteIds(CacheEntryId oldId, CacheEntryId newId)
    {
        for (CCacheBucket::iterator it = begin(); it != end(); it++)
        {
            if (*it == oldId)
            {
                *it = newId;
                return true;
            }
        }
        return false;
    }

    using vector<CacheEntryId>::size;
};

class CFormulaCache
{
    vector<CCacheBucket> theBucketBase;

    vector<CCacheEntry> theEntryBase;

#define NIL_ENTRY 0

    vector<CCacheBucket *> theData;

    vector<CCacheEntry>::iterator beginEntries()
    {
        return theEntryBase.begin()+1;
    }
    vector<CCacheEntry>::iterator endEntries()
    {
        return theEntryBase.end();
    }

    CacheEntryId toCacheEntryId(vector<CCacheEntry>::iterator &it)
    {
        return (CacheEntryId)(it - theEntryBase.begin());
    }


    unsigned int iBuckets;
    static unsigned int oldestEntryAllowed;
    /* statistics */
    unsigned int iCacheRetrievals;
    unsigned int iSumRetrieveSize;
    unsigned int iCacheTries;

    unsigned int iCachedComponents;
    unsigned int iSumCachedCompSize;
    unsigned int iSumCachedMemSize;
    unsigned int iUsedBuckets;
    unsigned int scoresDivTime;
    unsigned int minScoreBound;

    unsigned int lastDivTime;

    /*end statistics */
    unsigned int memUsage;
    //unsigned int maxMemUsage;

    double avgCachedSize()
    {
        if (iCacheRetrievals == 0) return 0.0;
        return (double) iSumCachedCompSize / (double) iCachedComponents;
    }

    double avgCachedMemSize()
    {
        if (iCacheRetrievals == 0) return 0.0;
        return (double) memUsage / (double) iCachedComponents;
    }

    double avgHitSize()
    {
        if (iCacheRetrievals == 0) return 0.0;
        return (double) iSumRetrieveSize / (double) iCacheRetrievals;
    }

    unsigned int clip(unsigned int ofs)
    {
        return ofs % iBuckets;
    }

    unsigned int clip(long unsigned int ofs)
    {
        return ofs % iBuckets;
    }

    bool isEntry(CacheEntryId theId)
    {
        return (theId != 0) & (theId < theEntryBase.size());
    }

    CCacheEntry &entry(CacheEntryId theId)
    {
#ifdef DEBUG
        assert(theId < theEntryBase.size());
#endif
        return theEntryBase[theId];
    }

    CacheEntryId newEntry()
    {
        theEntryBase.push_back(CCacheEntry());
        return theEntryBase.size()-1;
    }

    unsigned int memSizeOf(CCacheBucket &rBuck)
    {
        unsigned int n = 0;
        for (CCacheBucket::iterator it = rBuck.begin();it!= rBuck.end();it++)
        {
            n += entry(*it).memSize();
        }
        return n;
    }

    CCacheBucket *createNewCacheBucket()
    {
        theBucketBase.push_back(CCacheBucket());
        iUsedBuckets++;
        return &theBucketBase.back();
    }

    CCacheBucket &at(unsigned int ofs)
    {
        if (theData[ofs] == NULL) theData[ofs] = createNewCacheBucket();
        return *theData[ofs];
    }

    bool isBucketAt(unsigned int ofs)
    {
#ifdef DEBUG
        assert(theData.size() > ofs);
#endif
        return theData[ofs] != NULL;
    }

public:

    unsigned int getScoresDivTime()
    {
        return scoresDivTime;
    }
    unsigned int getLastDivTime()
    {
        return lastDivTime;
    }
    void setLastDivTime(unsigned int d)
    {
        lastDivTime = d;
    }

    CFormulaCache();

    ~CFormulaCache()
    {
        reset();
    }

    void init()
    {
        theBucketBase.clear();
        theData.clear();
        theEntryBase.clear();
        theEntryBase.push_back(CCacheEntry()); // dummy Element
        theData.resize(iBuckets,NULL);
        theBucketBase.reserve(iBuckets);
        iUsedBuckets = 0;
        memUsage = 0;
        iCachedComponents = 0;
        iCacheRetrievals = 0;
        iSumRetrieveSize = 0;
        iSumCachedCompSize = 0;
        iSumCachedMemSize = 0;
        iCacheTries = 0;
        minScoreBound = 0;
    }

    void reset()
    {
        theBucketBase.clear();
        theEntryBase.clear();
        theData.clear();
    }

    void printStatistics(CRunAnalyzer & rAn)
    {
        rAn.setValue(FCACHE_MAXMEM,CSolverConf::maxCacheSize);  // Formula Cache Memory Bound
        rAn.setValue(FCACHE_MEMUSE,memUsage);  // Formula Cache memory usage
        rAn.setValue(FCACHE_USEDBUCKETS,iUsedBuckets); // number of hashbuckets used
        rAn.setValue(FCACHE_CACHEDCOMPS,iCachedComponents);
        rAn.setValue(FCACHE_RETRIEVALS,iCacheRetrievals); // number of retrieved components
        rAn.setValue(FCACHE_INCLUDETRIES,iCacheTries); // number of times it was tried to put a component into the cache

    }

    void divCacheScores()
    {
        for (vector<CCacheEntry>::iterator it =  theEntryBase.begin(); it != theEntryBase.end(); it++)
        {
            (it->score) >>= 1;
        }
    }

    long unsigned int computeHashVal(const CComponentId &rComp);

    bool include(CComponentId &rComp, const CRealNum &val, DTNode * dtNode);

    bool extract(CComponentId &rComp, CRealNum &val, DTNode * dtNode);

    bool deleteEntries(CDecisionStack & rDecStack);

    void revalidateCacheLinksIn(const vector<CComponentId*> &rComps);
    void substituteCacheLinksIn(const vector<CComponentId*> &rComps, CacheEntryId idOld, CacheEntryId idNew);


    int removePollutedEntries(CacheEntryId root); // remove the whole tree below root


    bool cacheCompaction(CDecisionStack & rDecStack);

    void adjustDescendantsFather(CacheEntryId father)
    {
        vector<CacheEntryId>::iterator it;
        for (it = entry(father).theDescendants.begin();
                it != entry(father).theDescendants.end();it++)
        {
            entry(*it).theFather = father;
        }
    }

    // delete entries, keeping the descendants tree consistent
    void deleteFromDescendantsTree(CacheEntryId rEnt)
    {
        // let ME be the entry to be deleted
        CacheEntryId father = entry(rEnt).getFather();
        // first remove link from father to ME
        if (father != NIL_ENTRY)
        {
            entry(father).removeDescendant(rEnt);
            // then add all my descendants to descendants of father
            entry(father).addDescendants(entry(rEnt).theDescendants);
        }
        // next all MY descendants have now father father
        vector<CacheEntryId>::iterator it;
        for (it = entry(rEnt).theDescendants.begin();
                it != entry(rEnt).theDescendants.end();
                it++)
        {
            entry(*it).substituteFather(father);
        }
    }

    void substituteInDescTree(CacheEntryId rOld, CacheEntryId rNew)
    {
        // let ME be the entry to be deleted
        CacheEntryId father = entry(rOld).getFather();
        // first subst link from father to ME

        if (father != NIL_ENTRY)
        {
#ifdef DEBUG
            assert(isEntry(father));
#endif
            entry(father).substituteDescendant(rOld, rNew);
        }
        // next all MY descendants get father rNew
        vector<CacheEntryId>::iterator it;
        for (it = entry(rOld).theDescendants.begin();
                it != entry(rOld).theDescendants.end();
                it++)
        {
#ifdef DEBUG
            assert(isEntry(*it));
#endif
            entry(*it).substituteFather(rNew);
        }
    }

};



#endif
