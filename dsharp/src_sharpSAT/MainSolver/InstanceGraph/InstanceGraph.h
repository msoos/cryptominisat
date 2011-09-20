#ifndef INSTANCE_GRAPH_H
#define INSTANCE_GRAPH_H

#include<vector>
#include<deque>

#ifdef DEBUG
#include <assert.h>
#endif

using namespace std;

#include <Interface/AnalyzerData.h>

#include "../../Basics.h"
#include "AtomsAndNodes.h"
#include "ComponentTypes.h"

typedef vector<CClauseVertex> DepositOfClauses;
typedef vector<CVariableVertex> DepositOfVars;

extern CRunAnalyzer theRunAn;

class CInstanceGraph
{
	/** theLitVector: the literals of all clauses are stored here
	 *   INVARIANT: first and last entries of theLitVector are a SENTINEL_LIT
	 *
	 */
	vector<LiteralIdT> theLitVector;

	/** theInClsVector:
	 * the Vector containing all links from variables
	 * to the clauses they appear in
	 * INVARIANT: for each Variable x the block of clause links looks as follows:
	 SENTINEL_CL nnnnnnnnppppppp SENTINEL_CL
	 where: n ... clause where -x appears; p.. clause where x appears
	 */
	vector<ClauseIdT> theInClsVector;

	DepositOfClauses theClauses;
	unsigned int iOfsBeginConflictClauses;

	DepositOfVars theVars;

	vector<AntecedentT> theConflicted;
	unsigned int numBinClauses;
	unsigned int numBinCCls;

	vector<int> varTranslation;
	vector<int> varUntranslation;
	vector<int> origTranslation;

protected:

	vector<LiteralIdT> theUnitClauses;
	vector<TriValue> theUClLookUp;

	/// THE function to clear all the data in this class
	void reset()
	{
		theLitVector.clear();
		theLitVector.push_back(SENTINEL_LIT);

		theInClsVector.clear();
		theInClsVector.push_back(SENTINEL_CL);

		theClauses.clear();
		theClauses.push_back(CClauseVertex(false));
		iOfsBeginConflictClauses = 0;

		theVars.clear();
		theVars.push_back(CVariableVertex(0, 0)); //initializing the Sentinel
		theConflicted.clear();

		numBinClauses = 0;
		numBinCCls = 0;

		theUnitClauses.clear();
		theUClLookUp.clear();

		varTranslation.clear();
		varUntranslation.clear();
		origTranslation.clear();
	}

	void doVSIDSScoreDiv()
	{
		DepositOfVars::iterator it;
		for (it = beginOfVars(); it != endOfVars(); it++)
		{
			it->scoreVSIDS[0] >>= 1;
			it->scoreVSIDS[1] >>= 1;
		}
	}

protected:
	void printCClstats()
	{
		toSTDOUT("CCls (all/bin/unit):\t");
		toSTDOUT(theClauses.size() - iOfsBeginConflictClauses);
		toSTDOUT("/"<< numBinCCls<< "/"<<theUnitClauses.size()<<endl);
	}
	bool isUnitCl(const LiteralIdT theLit)
	{
		return theUClLookUp[theLit.toVarIdx()] == theLit.polarityTriVal();
	}

	unsigned int countBinClauses() const
	{
		return numBinClauses;
	}
	unsigned int countBinCCls() const
	{
		return numBinCCls;
	}
	DepositOfClauses::iterator beginOfCCls()
	{
		return theClauses.begin() + iOfsBeginConflictClauses;
	}
	DepositOfClauses::iterator endOfCCls()
	{
		return theClauses.end();
	}
	DepositOfClauses::iterator beginOfClauses()
	{
		return theClauses.begin() + 1;
	}

	ClauseIdT toClauseIdT(DepositOfClauses::iterator it)
	{
		unsigned int idx = (unsigned int) (it - theClauses.begin());
		return ClauseIdT(idx);
	}

	DepositOfClauses::iterator endOfClauses()
	{
		return theClauses.begin() + iOfsBeginConflictClauses;
	}

	DepositOfVars::iterator beginOfVars()
	{
		return theVars.begin() + 1;
	}

	DepositOfVars::iterator endOfVars()
	{
		return theVars.end();
	}
	vector<LiteralIdT>::iterator begin(CClauseVertex &rCV)
	{
		return theLitVector.begin() + rCV.getLitOfs();
	}

	vector<LiteralIdT>::const_iterator begin(const CClauseVertex &rCV) const
	{
		return theLitVector.begin() + rCV.getLitOfs();
	}

	const LiteralIdT & ClauseEnd() const
	{
		return SENTINEL_LIT;
	}

	bool substituteLitsOf(CClauseVertex &rCl, const LiteralIdT &oldLit,
			const LiteralIdT &newLit);
	bool containsVar(const CClauseVertex &rCl, const VarIdT &theVar) const;
	bool containsLit(const CClauseVertex &rCl, const LiteralIdT &theLit) const;

public:

	CInstanceGraph();
	~CInstanceGraph();

	const vector<int> & getVarTranslation() const
	{
		return varTranslation;
	}

	const vector<int> & getVarUnTranslation() const
	{
		return varUntranslation;
	}

	const vector<int> & getOrigTranslation() const
	{
		return origTranslation;
	}

	/////////////////////////////////////////////////////////
	// BEGIN access to variables and clauses
	inline vector<ClauseIdT>::const_iterator var_InClsBegin(VarIdT VarIndex) const
	{
		return (theInClsVector.begin()
				+ theVars[VarIndex].getInClsVecOfs(false));
	}

	inline vector<ClauseIdT>::iterator var_InClsBegin(VarIdT VarIndex, bool pol)
	{
		return (theInClsVector.begin() + theVars[VarIndex].getInClsVecOfs(pol));
	}

	inline vector<ClauseIdT>::iterator var_InClsStart(VarIdT VarIndex, bool pol)
	{
		return (theInClsVector.begin() + theVars[VarIndex].getInClsVecOfs(true)
				- 1 + (int) pol);
	}

	inline int var_InClsStep(bool pol)
	{
		return pol ? 1 : -1;
	}

	inline bool var_EraseClsLink(VarIdT VarIndex, bool linkPol, ClauseIdT idCl)
	{
		CVariableVertex &rV = getVar(VarIndex);
		vector<ClauseIdT>::iterator it;

		if (!linkPol)
		{
			for (it = var_InClsBegin(VarIndex, true) - 1; *it != SENTINEL_CL; it--)
				if (*it == idCl)
				{
					while (*it != SENTINEL_CL)
					{
						*it = *(it - 1);
						it--;
					}
					rV.setInClsVecOfs(false, rV.getInClsVecOfs(false) + 1);
					return true;
				}
		}
		else
		{
			for (it = var_InClsBegin(VarIndex, true); *it != SENTINEL_CL; it++)
				if (*it == idCl)
				{
					while (*it != SENTINEL_CL)
					{
						*it = *(it + 1);
						it++;
					}
					return true;
				}
		}
		return false;
	}

	inline vector<ClauseIdT>::const_iterator var_InClsBegin(CVariableVertex &vv) const
	{
		return (theInClsVector.begin() + vv.getInClsVecOfs(false));
	}

	inline CVariableVertex &getVar(VarIdT VarIndex)
	{
		return theVars[VarIndex];
	}
	inline const CVariableVertex &getVar(VarIdT VarIndex) const
	{
		return theVars[VarIndex];
	}

	inline bool isSatisfied(const LiteralIdT &lit) const
	{
		return theVars[lit.toVarIdx()].getVal() == lit.polarityTriVal();
	}
	inline bool isSatisfied(const ClauseIdT &iCl) const
	{
		return isSatisfied(getClause(iCl).idLitA()) || isSatisfied(getClause(
				iCl).idLitB());
	}

	inline CVariableVertex &getVar(const LiteralIdT &rLitId)
	{
		return theVars[rLitId.toVarIdx()];
	}

	inline const CVariableVertex &getVar(const LiteralIdT &rLitId) const
	{
		return theVars[rLitId.toVarIdx()];
	}

	inline CClauseVertex &getClause(const ClauseIdT &iClauseId)
	{
		return theClauses[iClauseId];
	}

	inline const CClauseVertex &getClause(const ClauseIdT &iClauseId) const
	{
		return theClauses[iClauseId];
	}

	// END access to variables and clauses
	/////////////////////////////////////////////////////////

	inline const ClauseIdT getLastClauseId()
	{
		return ClauseIdT(theClauses.size() - 1);
	}

	vector<AntecedentT> &getConflicted()
	{
		return theConflicted;
	}

	// BEGIN count something
	inline unsigned int countAllClauses() const
	{
		return theClauses.size() - 1 + countBinClauses();
	}

	inline unsigned int countCCls() const
	{
		return theClauses.size() - iOfsBeginConflictClauses;
	}

	inline unsigned int countOriginalClauses() const
	{
		return theClauses.size();
	}

	inline unsigned int getMaxOriginalClIdx() const
	{
		return iOfsBeginConflictClauses;
	}

	inline unsigned int countAllVars()
	{
		return theVars.size() - 1;
	}

	unsigned int countActiveBinLinks(VarIdT theVar) const;
	
	unsigned int originalVarCount;

	// END count something

	bool createfromFile(const char* lpstrFileName);

	void convertComponent(CComponentId &oldComp, vector<int> * newComp);

	void print();
	void printCl(const CClauseVertex &rCl) const;
	void printClause(const ClauseIdT &idCl) const;
	void printActiveClause(const ClauseIdT &idCl) const;

protected:
	// cleans up the ClausePool
	/// NOTE: this does only work correctly if no CCls are present
	bool prep_CleanUpPool();

	// only valid if no conflict clauses are present
	bool prep_substituteClauses(unsigned int oldIdx, unsigned int newIdx);
	bool prep_substituteVars(CVariableVertex &rV, unsigned int newIdx);

	bool markCClDeleted(ClauseIdT idCl);

	///  only correct if theLit is conteined in idCl
	bool setCClImplyingLit(ClauseIdT idCl, const LiteralIdT &theLit);
	bool eraseLiteralFromCl(ClauseIdT idCl, LiteralIdT theLit);

	bool deleteConflictCls();
	bool cleanUp_deletedCCls();

	bool createConflictClause(const vector<LiteralIdT> &theCClause);

	unsigned int makeVariable(unsigned int VarNum)
	{
		theVars.push_back(CVariableVertex(VarNum, theVars.size()));
		return theVars.back().getVarIdT();
	}

	ClauseIdT makeClause()
	{
		theClauses.push_back(CClauseVertex(false));
		return ClauseIdT(theClauses.size() - 1);
	}

	ClauseIdT makeConflictClause()
	{
		theClauses.push_back(CClauseVertex(true));
		return ClauseIdT(theClauses.size() - 1);
	}

	bool createUnitCl(LiteralIdT Lit)
	{
		theUnitClauses.push_back(Lit);
		if (theUClLookUp[Lit.toVarIdx()] == Lit.oppositeLit().polarityTriVal())
			return false;
		theUClLookUp[Lit.toVarIdx()] = Lit.polarityTriVal();
		return true;
	}

	bool createBinCl(LiteralIdT LitA, LiteralIdT LitB)
	{
		if (getVar(LitA).hasBinLinkTo(LitB, LitA.polarity()))
		{
#ifdef DEBUG
			assert(getVar(LitB).hasBinLinkTo(LitA,LitB.polarity()));
			toDEBUGOUT("%");
#endif
			return false;
		}
		getVar(LitA).addBinLink(LitA.polarity(), LitB);
		getVar(LitB).addBinLink(LitB.polarity(), LitA);
		numBinClauses++;
		return true;
	}

	bool createBinCCl(LiteralIdT LitA, LiteralIdT LitB)
	{
		if (getVar(LitA).hasBinLinkTo(LitB, LitA.polarity()))
		{
#ifdef DEBUG
			assert(getVar(LitB).hasBinLinkTo(LitA,LitB.polarity()));
#endif
			return false;
		}
		getVar(LitA).addBinLinkCCl(LitA.polarity(), LitB);
		getVar(LitB).addBinLinkCCl(LitB.polarity(), LitA);
		numBinCCls++;
		return true;
	}

};

#endif // CLAUSEPOOL_H
