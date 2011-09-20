#include "FormulaCache.h"

unsigned int CFormulaCache::oldestEntryAllowed = (unsigned int) -1;


CFormulaCache::CFormulaCache()
{
    iBuckets = 900001;// =299999;
    theData.resize(iBuckets,NULL);
    theBucketBase.reserve(iBuckets);
    theEntryBase.reserve(iBuckets*10);
    scoresDivTime = 50000;
    lastDivTime = 0;
#ifdef SUN_OS
    if (CSolverConf::maxCacheSize == 0)
        CSolverConf::maxCacheSize = 100*1024*1024;
#endif
#ifndef SUN_OS
    if (CSolverConf::maxCacheSize == 0)
    {
        int pgs = getpagesize();
        long int pages = get_avphys_pages();
        CSolverConf::maxCacheSize = (pgs*pages)/2;
    }
#endif
}



bool CFormulaCache::include(CComponentId &rComp, const CRealNum &val, DTNode * dtNode)
{
#ifdef DEBUG
    // if everything is correct, a new value to be cached
    // should not already be stored in the cache
    assert(rComp.cachedAs == NIL_ENTRY);
#endif

    if (rComp.empty()) return false;
    iCacheTries++;

    if (memUsage >= CSolverConf::maxCacheSize)  return false;

    long unsigned int hV = rComp.getHashKey();

    CCacheBucket &rBucket = at(clip(hV));


    CacheEntryId eId = newEntry();
    CCacheEntry & rEntry = entry(eId);
    rBucket.push_back(eId);
    rEntry.createFrom(rComp);
    rEntry.hashKey = hV;
    rEntry.theVal = val;
    rEntry.theDTNode = dtNode;

    rComp.cachedAs = eId; // save in the Comp, wwhere it was saved
    rEntry.theDescendants = rComp.cachedChildren; // save the cache ids of its children

    rComp.cachedChildren.clear();

    adjustDescendantsFather(eId);


    //BEGIN satistics

    unsigned int memU = memUsage/(10*1024*1024);

    memUsage += rEntry.memSize();

    if (memU < memUsage/(10*1024*1024))
    {
        toSTDOUT("Cache: usedMem "<< memUsage<<"Bytes\n");
    }
    iSumCachedCompSize += rComp.countVars();

    iCachedComponents++;
    if (iCachedComponents % 50000 == 0)
    {
        double d = iSumCachedCompSize;
        d /= (double) iCachedComponents;
        toSTDOUT("cachedComponents:"<< iCachedComponents<<" avg. size:"<<d<<endl);
    }
    //END satistics
    return true;
}

bool CFormulaCache::extract(CComponentId &rComp, CRealNum &val, DTNode * dtNode)
{
    long unsigned int hV = rComp.getHashKey();

    unsigned int v = clip(hV);

    if (!isBucketAt(v)) return false;
    CCacheBucket &rBucket = *theData[v]; // the location of the considered bucket

    CCacheEntry *pComp;

    for (CCacheBucket::iterator it = rBucket.begin(); it != rBucket.end();it++)
    {
        pComp = &entry(*it);
        if (hV == pComp->getHashKey() &&  pComp->equals(rComp))
        {
            val = pComp->theVal;
            pComp->score++;
            pComp->score+= (unsigned int)pComp->sizeVarVec();

            iCacheRetrievals++;
            iSumRetrieveSize += rComp.countVars();

            if (iCacheRetrievals % 50000 == 0)
            {
                double d = iSumRetrieveSize;
                d /= (double) iCacheRetrievals;
                toSTDOUT("cache hits:"<< iCacheRetrievals<<" avg size:"<< d<<endl);
            }
            
            pComp->theDTNode->addParent(dtNode, true);
            
            return true;

        }
    }
    return false;
}

int CFormulaCache::removePollutedEntries(CacheEntryId root)
{
    vector<CacheEntryId>::iterator it;// theDescendants;
    CCacheBucket &rBuck = at(clip(entry(root).hashKey));
    unsigned int n = 0;


    for (CCacheBucket::iterator jt= rBuck.begin(); jt != rBuck.end(); jt++)
    {
        if (*jt == root)
        {
            rBuck.erase(jt);
            n++;
            break;
        }
    }

    for (it = entry(root).theDescendants.begin(); it != entry(root).theDescendants.end(); it++)
    {
        n += removePollutedEntries(*it);
    }

    entry(root).clear();

    return n;
}

bool CFormulaCache::deleteEntries(CDecisionStack & rDecStack)
{
    vector<CCacheBucket>::iterator jt;
    vector<CCacheEntry>::iterator it,itWrite;
    CCacheBucket::iterator bt;

    if (memUsage < (unsigned int) ((double) 0.85* (double)CSolverConf::maxCacheSize)) return false;

    // first : go through the EntryBase and mark the entries to be deleted as deleted (i.e. EMPTY
    for (it = beginEntries(); it != endEntries(); it++)
    {
        if (it->score <= minScoreBound)
        {
            deleteFromDescendantsTree(toCacheEntryId(it));
            it->clear();
        }
    }

    // then go through the BucketBase and rease all Links to empty entries
    for (jt = theBucketBase.begin(); jt != theBucketBase.end(); jt++)
    {
        for (bt = jt->end()-1; bt != jt->begin()-1; bt--)
        {
            if (entry(*bt).empty()) bt = jt->erase(bt);
        }
    }

    // now: go through the decisionStack. and delete all Links to empty entries
    revalidateCacheLinksIn(rDecStack.getAllCompStack());

    // finally: truly erase the empty entries, but keep the descendants tree consistent
    long int newSZ = 0;
    long int SumNumOfVars= 0;
    CacheEntryId idOld,idNew;

    itWrite = beginEntries();
    for (it = beginEntries(); it != endEntries(); it++)
    {
        if (!it->empty())
        {
            if (it != itWrite)
            {
                *itWrite = *it;
                idNew = toCacheEntryId(itWrite);
                idOld = toCacheEntryId(it);
                at(clip(itWrite->getHashKey())).substituteIds(idOld,idNew);
                substituteInDescTree(idOld,idNew);
                substituteCacheLinksIn(rDecStack.getAllCompStack(),idOld,idNew);
            }
            itWrite++;
            //theEntryBase.pop_back();
            newSZ += itWrite->memSize();
            SumNumOfVars += itWrite->sizeVarVec();
        }
    }

    theEntryBase.erase(itWrite,theEntryBase.end());

    iCachedComponents = theEntryBase.size();
    iSumCachedCompSize = SumNumOfVars*sizeof(unsigned int)*8 / CCacheEntry::bitsPerVar();

    memUsage = newSZ;

    toSTDOUT("Cache cleaned: "<<iCachedComponents<<" Components ("<< (memUsage>>10)<< " KB remain"<<endl);

    if (scoresDivTime == 0) scoresDivTime = 1;
    double dbound = (double) 0.5* (double)CSolverConf::maxCacheSize;
    if (memUsage < (unsigned int) dbound)
    {
        minScoreBound/= 2;
        if (memUsage < 0.5*dbound) scoresDivTime *= 2;
    }
    else if (memUsage > (unsigned int) dbound)
    {
        minScoreBound <<= 1;
        minScoreBound++;
        scoresDivTime /= 2;
        if (scoresDivTime < 50000) scoresDivTime = 50000;
    }
    toDEBUGOUT("setting scoresDivTime: "<<scoresDivTime<<endl);
    toDEBUGOUT("setting minScoreBound: "<<minScoreBound<<endl);

    return true;
}


void CFormulaCache::revalidateCacheLinksIn(const vector<CComponentId *> &rComps)
{
    vector<CComponentId *>::const_iterator it;
    vector<unsigned int>::iterator jt;
    for (it = rComps.begin(); it !=rComps.end(); it++)
    {
        if (!isEntry((*it)->cachedAs)) (*it)->cachedAs = 0;

        if (isEntry((*it)->cachedAs) && entry((*it)->cachedAs).empty()) (*it)->cachedAs = 0;

        for (jt = (*it)->cachedChildren.end()-1; jt != (*it)->cachedChildren.begin()-1; jt--)
        {
            if ((!isEntry(*jt)) || entry(*jt).empty())
            {
                toDEBUGOUT("_E");
                jt =(*it)->cachedChildren.erase(jt);
            }
        }
    }
}


void CFormulaCache::substituteCacheLinksIn(const vector<CComponentId *> &rComps, CacheEntryId idOld, CacheEntryId idNew)
{
    vector<CComponentId *>::const_iterator it;
    vector<unsigned int>::iterator jt;
    for (it = rComps.begin(); it !=rComps.end(); it++)
    {
        if ((*it)->cachedAs == idOld) (*it)->cachedAs = idNew;
        for (jt = (*it)->cachedChildren.begin(); jt != (*it)->cachedChildren.end(); jt++)
        {
            if (*jt == idOld)
            {
                toDEBUGOUT("_D");
                *jt = idNew;
            }
        }
    }
}
