#include "AtomsAndNodes.h" // class's header file

CVariableVertex::~CVariableVertex()
{
}

bool CVariableVertex::substituteWatchCl(bool polarity, const ClauseIdT & oldId, const ClauseIdT &newId)
{
    for (vector<ClauseIdT>::iterator jt = watchCls[polarity].end()-1;*jt != SENTINEL_CL; jt--)
    {
        if (*jt == oldId) *jt = newId;
    }
    return true;
}


bool CVariableVertex::substituteBinLink(bool polarity, const LiteralIdT& oldLit, const LiteralIdT& newLit)
{
    vector<LiteralIdT>::iterator it;
    for (it = binClLinks[polarity].begin(); it != binClLinks[polarity].end(); it++)
    {
        if (*it == oldLit)
        {
            *it = newLit;
            return true;
        }
    }
    return false;
}


bool CVariableVertex::eraseWatchClause(ClauseIdT idCl, bool polarity)
{
    vector<ClauseIdT>& theWatchCls = watchCls[polarity];
    vector<ClauseIdT>::iterator it;

    for (it = theWatchCls.begin();it != theWatchCls.end();it++)
    {
        if (*it == idCl)
        {
            theWatchCls.erase(it);
            return true;
        }
    }
    return false;
}

bool CVariableVertex::eraseBinLinkTo(LiteralIdT idLit, bool Linkpolarity)
{
    vector<LiteralIdT> &theLinks = getBinLinks(Linkpolarity);
    vector<LiteralIdT>::iterator it;

    for (it = theLinks.begin();it != theLinks.end();it++)
    {
        if (*it == idLit)
        {
            theLinks.erase(it);
            return true;
        }
    }
    return false;
}


bool CVariableVertex::hasBinLinkTo(LiteralIdT idLit,bool pol) const
{
    vector<LiteralIdT>::const_iterator it;

    for (it = getBinLinks(pol).begin();*it != SENTINEL_LIT;it++)
    {
        if (*it == idLit) return true;
    }
    it++;
    for (;*it != SENTINEL_LIT;it++)
    {
        if (*it == idLit) return true;
    }
    return false;
}
