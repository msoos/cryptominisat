#include "InstanceGraph.h"
#include <math.h>
#include<fstream>
#include<stdio.h>

CRunAnalyzer theRunAn;

// class constructor
CInstanceGraph::CInstanceGraph()
{

}

// class destructor
CInstanceGraph::~CInstanceGraph()
{
	toDEBUGOUT("lv sz:"<< theLitVector.size() << endl);toDEBUGOUT("nUcls:"<< theUnitClauses.size() << endl);
}

////////////////////////////////////////////////////////////////////////
//
//  BEGIN Methods for Clauses
//
///////////////////////////////////////////////////////////////////////

bool CInstanceGraph::substituteLitsOf(CClauseVertex &rCl,
		const LiteralIdT &oldLit, const LiteralIdT &newLit)
{
	vector<LiteralIdT>::iterator it;

	if (oldLit == rCl.idLitA())
		rCl.setLitA(newLit);
	else if (oldLit == rCl.idLitB())
		rCl.setLitB(newLit);

	if (oldLit.oppositeLit() == rCl.idLitA())
		rCl.setLitA(newLit.oppositeLit());
	else if (oldLit.oppositeLit() == rCl.idLitB())
		rCl.setLitB(newLit.oppositeLit());

	for (it = begin(rCl); *it != ClauseEnd(); it++)
	{
		if (*it == oldLit)
		{
			*it = newLit;
			return true;
		}
		else if (*it == oldLit.oppositeLit())
		{
			*it = newLit.oppositeLit();
			return true;
		}
	}
	return false;
}

bool CInstanceGraph::containsVar(const CClauseVertex &rCl, const VarIdT &theVar) const
{
	vector<LiteralIdT>::const_iterator it;

	for (it = begin(rCl); *it != ClauseEnd(); it++)
	{
		if (it->toVarIdx() == theVar)
			return true;
	}
	return false;
}

bool CInstanceGraph::containsLit(const CClauseVertex &rCl,
		const LiteralIdT &theLit) const
{
	vector<LiteralIdT>::const_iterator it;

	for (it = begin(rCl); *it != ClauseEnd(); it++)
	{
		if (*it == theLit)
			return true;
	}
	return false;
}

void CInstanceGraph::printCl(const CClauseVertex &rCl) const
{
	vector<LiteralIdT>::const_iterator it;

	for (it = begin(rCl); *it != ClauseEnd(); it++)
	{
		if (!(it)->polarity())
			toSTDOUT("-");
		toSTDOUT(it->toVarIdx()+1 << " ");
	}
	toSTDOUT("  0\n");
}

bool CInstanceGraph::createConflictClause(const vector<LiteralIdT> &theCClause)
{
	ClauseIdT cclId;
	vector<LiteralIdT>::const_iterator it;

#ifdef DEBUG
	assert(theCClause.size()> 0);
#endif

	if (theCClause.size() == 1)
	{
		createUnitCl(theCClause.front());
		if (theUnitClauses.size() == 1 || theUnitClauses.size() % 5 == 0)
			printCClstats();
		getVar(theCClause.front()).scoreVSIDS[theCClause.front().polarity()]++;
		getVar(theCClause.front()).scoreVSIDS[theCClause.front().oppositeLit().polarity()]++;
		theRunAn.addClause();
		return true;
	}

	if (theCClause.size() == 2)
	{
		if (!createBinCCl(theCClause[0], theCClause[1]))
			return false;

		getVar(theCClause[0]).scoreVSIDS[theCClause[0].polarity()]++;
		getVar(theCClause[1]).scoreVSIDS[theCClause[1].polarity()]++;
		getVar(theCClause[0]).scoreVSIDS[theCClause[0].oppositeLit().polarity()]++;
		getVar(theCClause[1]).scoreVSIDS[theCClause[1].oppositeLit().polarity()]++;

		if (numBinCCls % 100 == 0)
			printCClstats();
		theRunAn.addClause();
		return true;
	}

	// create the ClauseVertex

	cclId = makeConflictClause();

	CClauseVertex *pCCl = &getClause(cclId);

	pCCl->setLitOfs(theLitVector.size());
	pCCl->setLength(theCClause.size());

	int score = 0;
	LiteralIdT aLit = NOT_A_LIT;
	LiteralIdT bLit = NOT_A_LIT;

	theLitVector.reserve(theLitVector.size() + theCClause.size());
	for (it = theCClause.begin(); it != theCClause.end(); it++)
	{
		// add literal *it to the litvector
		theLitVector.push_back(*it);

		if (getVar(*it).getDLOD() >= score)
		// determine the most recently set literals to become watched
		{
			bLit = aLit;
			aLit = *it;
			score = getVar(*it).getDLOD();
		}
		getVar(*it).scoreVSIDS[it->polarity()]++;
		getVar(*it).scoreVSIDS[it->oppositeLit().polarity()]++;
	}
	score = 0;
	if (bLit == NOT_A_LIT)
		for (it = theCClause.begin(); it != theCClause.end(); it++)
		{
			if (*it != aLit && getVar(*it).getDLOD() >= score)
			// determine the most recently set literals to become watched
			{
				bLit = *it;
				score = getVar(*it).getDLOD();
			}
		}

#ifdef DEBUG
	assert(aLit != NOT_A_LIT);
	assert(bLit != NOT_A_LIT);
#endif

	// close the clause with a SENTINEL_LIT
	theLitVector.push_back(SENTINEL_LIT);

	// set watch for litA
	if (aLit != NOT_A_LIT)
	{
		pCCl->setLitA(aLit);
		getVar(aLit).addWatchClause(cclId, aLit.polarity());
	}
	// set watch for litB
	if (bLit != NOT_A_LIT)
	{
		pCCl->setLitB(bLit);
		getVar(bLit).addWatchClause(cclId, bLit.polarity());
	}

	if (countCCls() % 10000 == 0)
		printCClstats();
	theRunAn.addClause();
	return true;
}

bool CInstanceGraph::setCClImplyingLit(ClauseIdT idCl, const LiteralIdT &theLit)
{
	CClauseVertex &rCV = getClause(idCl);
	vector<LiteralIdT>::const_iterator it;

	getVar(rCV.idLitA()).eraseWatchClause(idCl, rCV.idLitA().polarity());
	getVar(rCV.idLitB()).eraseWatchClause(idCl, rCV.idLitB().polarity());

	int score = -1;
	LiteralIdT aLit = NOT_A_LIT;

#ifdef DEBUG
	bool ex = false;
	for (it = begin(rCV); *it != ClauseEnd(); it++)
	if (*it == theLit)
	{
		ex = true;
		break;
	}
	assert(ex);
#endif

	rCV.setLitA(theLit);
	getVar(rCV.idLitA()).addWatchClause(idCl, rCV.idLitA().polarity());
	// set watch for litB

	aLit = NOT_A_LIT;
	score = -1;

	for (it = begin(rCV); *it != ClauseEnd(); it++)
		if (getVar(*it).getDLOD() > score)
		{
			if (*it == theLit)
				continue;
			aLit = *it;
			score = getVar(*it).getDLOD();
		}

	if (aLit != NOT_A_LIT)
	{
		rCV.setLitB(aLit);
		getVar(aLit).addWatchClause(idCl, aLit.polarity());
	}

	return true;
}

bool CInstanceGraph::cleanUp_deletedCCls()
{
	DepositOfClauses::iterator ct;

	///////////////////
	// clean up LitVector
	///////////////////

	vector<LiteralIdT>::iterator writeLit = theLitVector.begin()
			+ (beginOfCCls())->getLitOfs();
	ct = beginOfCCls();

	for (vector<LiteralIdT>::iterator xt = writeLit; xt != theLitVector.end(); xt++)
		if (*xt != NOT_A_LIT)
		{
			if (!ct->isDeleted())
			{
				ct->setLitOfs((unsigned int) (writeLit - theLitVector.begin()));

				while (*xt != NOT_A_LIT)
				{
					if (writeLit != xt)
						*writeLit = *xt;
					xt++;
					writeLit++;
				}
				*(writeLit++) = NOT_A_LIT;
			}
			else
			{ // *ct is deleted, hence, omit all its literals from consideration
				while (*xt != NOT_A_LIT)
					xt++;//*(xt++) = NOT_A_LIT;
			}
			ct++;
		}

	theLitVector.resize((unsigned int) (writeLit - theLitVector.begin()));

	DepositOfClauses::iterator itWrite = beginOfCCls();
	///////////////////
	// clean up clauses
	///////////////////
	ClauseIdT oldId, newId;

	for (ct = beginOfCCls(); ct != endOfCCls(); ct++)
		if (!ct->isDeleted())
		{
			if (itWrite != ct)
			{
				*itWrite = *ct;
				//BEGIN substitute CCLs

				oldId = toClauseIdT(ct);
				newId = toClauseIdT(itWrite);

				if (getVar(itWrite->idLitB()).isImpliedBy(oldId))
					getVar(itWrite->idLitB()).adjustAntecedent(AntecedentT(
							newId));
				if (getVar(itWrite->idLitA()).isImpliedBy(oldId))
					getVar(itWrite->idLitA()).adjustAntecedent(AntecedentT(
							newId));

				getVar(itWrite->idLitA()).substituteWatchCl(
						itWrite->idLitA().polarity(), oldId, newId);
				getVar(itWrite->idLitB()).substituteWatchCl(
						itWrite->idLitB().polarity(), oldId, newId);
				//END substitute CCLs

				ct->setDeleted();
			}
			itWrite++;
		}

	theClauses.erase(itWrite, endOfCCls());

	return true;
}

bool CInstanceGraph::deleteConflictCls()
{
	DepositOfClauses::iterator it;

	double vgl = 0;

	for (it = beginOfCCls(); it != endOfCCls(); it++)
	{
		vgl = 11000.0;
		if (it->length() != 0)
			vgl /= pow((double) it->length(), 3);

		if (CStepTime::getTime() - it->getLastTouchTime() > vgl)
		{
			markCClDeleted(toClauseIdT(it));
		}
	}
	return true;
}

bool CInstanceGraph::markCClDeleted(ClauseIdT idCl)
{
	CClauseVertex & rCV = getClause(idCl);
	if (rCV.isDeleted())
		return false;
	/////
	// a clause may not be deleted if it causes an implication:
	///
	if (getVar(rCV.idLitB()).isImpliedBy(idCl)
			|| getVar(rCV.idLitA()).isImpliedBy(idCl))
	{
		return false;
	}

	getVar(rCV.idLitB()).eraseWatchClause(idCl, rCV.idLitB().polarity());
	getVar(rCV.idLitA()).eraseWatchClause(idCl, rCV.idLitA().polarity());
	rCV.setDeleted();
	return true;
}

////////////////////////////////////////////////////////////////////////
//
//  END Methods for Clauses
//
///////////////////////////////////////////////////////////////////////

bool CInstanceGraph::prep_substituteClauses(unsigned int oldIdx,
		unsigned int newIdx)
{
	CClauseVertex &rCl = getClause(newIdx);
	vector<LiteralIdT>::const_iterator it;
	vector<ClauseIdT>::iterator jt;
	ClauseIdT oldId(oldIdx), newId(newIdx);

	if (getVar(rCl.idLitB()).isImpliedBy(oldId))
		getVar(rCl.idLitB()).adjustAntecedent(AntecedentT(newId));

	if (getVar(rCl.idLitA()).isImpliedBy(oldId))
		getVar(rCl.idLitA()).adjustAntecedent(AntecedentT(newId));

	getVar(rCl.idLitA()).substituteWatchCl(rCl.idLitA().polarity(), oldId,
			newId);
	getVar(rCl.idLitB()).substituteWatchCl(rCl.idLitB().polarity(), oldId,
			newId);

	for (it = begin(rCl); *it != ClauseEnd(); it++)
	{
		// substitute ClEdges in theInClsVector
		for (vector<ClauseIdT>::iterator jt = var_InClsBegin(it->toVarIdx(),
				false); *jt != SENTINEL_CL; jt++)
		{
			if (*jt == oldId)
				*jt = newId;
		}
		//
	}

	return true;
}

bool CInstanceGraph::prep_substituteVars(CVariableVertex &rV,
		unsigned int newIdx)
// only valid if no conflict clauses are present
{
	vector<ClauseIdT>::const_iterator it;
	vector<LiteralIdT>::iterator kt, vt;

	vector<LiteralIdT>::iterator jt;
	unsigned int oldIdx = rV.getVarIdT();
	rV.newtecIndex(newIdx);

	LiteralIdT oldLit, newLit;

	oldLit = LiteralIdT(oldIdx, true);
	newLit = LiteralIdT(newIdx, true);

	varTranslation[newIdx] = oldIdx;
	varUntranslation[oldIdx] = newIdx;

	for (it = var_InClsBegin(rV.getVarIdT(), true); *it != SENTINEL_CL; it++)
	{
		substituteLitsOf(getClause(*it), oldLit, newLit);
	}

	for (kt = rV.getBinLinks(true).begin(); *kt != SENTINEL_LIT; kt++)
	{
		getVar(*kt).substituteBinLink(kt->polarity(), oldLit, newLit);
	}

	oldLit = LiteralIdT(oldIdx, false);
	newLit = LiteralIdT(newIdx, false);

	for (it = var_InClsBegin(rV.getVarIdT(), true) - 1; *it != SENTINEL_CL; it--)
	{
		substituteLitsOf(getClause(*it), oldLit, newLit);
	}

	for (kt = rV.getBinLinks(false).begin(); *kt != SENTINEL_LIT; kt++)
	{
		getVar(*kt).substituteBinLink(kt->polarity(), oldLit, newLit);
	}
	return true;
}

bool CInstanceGraph::eraseLiteralFromCl(ClauseIdT idCl, LiteralIdT theLit)
{
	bool retV = false;
	CClauseVertex & rCV = getClause(idCl);
	vector<LiteralIdT>::iterator it;
	vector<LiteralIdT>::iterator endCl = begin(rCV) + rCV.length();
	if (rCV.isDeleted())
		return false;

	if (getVar(rCV.idLitA()).isImpliedBy(idCl)
			|| getVar(rCV.idLitB()).isImpliedBy(idCl))
		return false;

	getVar(rCV.idLitA()).eraseWatchClause(idCl, rCV.idLitA().polarity());
	if (rCV.length() >= 2)
		getVar(rCV.idLitB()).eraseWatchClause(idCl, rCV.idLitB().polarity());

	for (it = begin(rCV); *it != ClauseEnd(); it++)
	{
		if ((*it) == theLit)
		{
			if (it != endCl - 1)
				*it = *(endCl - 1);
			*(endCl - 1) = NOT_A_LIT;
			rCV.setLength(rCV.length() - 1);
			retV = true;
			break;
		}
	}

	rCV.setLitA(NOT_A_LIT);
	rCV.setLitB(NOT_A_LIT);

	if (rCV.length() >= 1)
	{
		rCV.setLitA(*begin(rCV));
		getVar(rCV.idLitA()).addWatchClause(idCl, rCV.idLitA().polarity());
	}
	if (rCV.length() >= 2)
	{
		rCV.setLitB(*(begin(rCV) + 1));
		getVar(rCV.idLitB()).addWatchClause(idCl, rCV.idLitB().polarity());
	}

	return retV;
}

bool CInstanceGraph::prep_CleanUpPool()
{
	///////////////////
	// clean up clauses
	///////////////////
	DepositOfClauses::iterator ct, ctWrite = theClauses.begin() + 1;

	for (ct = theClauses.begin() + 1; ct != theClauses.end(); ct++)
		if (!ct->isDeleted())
		{
			if (ctWrite != ct)
			{
				*ctWrite = *ct;
				prep_substituteClauses(
						(unsigned int) (ct - theClauses.begin()),
						(unsigned int) (ctWrite - theClauses.begin()));
			}
			ctWrite++;
		}

	theClauses.erase(ctWrite, theClauses.end());
	iOfsBeginConflictClauses = theClauses.size();
	///////////////////
	// clean up LitVector
	///////////////////

	vector<LiteralIdT>::iterator writeLit = theLitVector.begin();

	ct = theClauses.begin() + 1;

	for (vector<LiteralIdT>::iterator xt = writeLit; xt != theLitVector.end(); xt++)
		if (*xt != SENTINEL_LIT) // start of the next clause found
		{
			ct->setLitOfs((unsigned int) (writeLit - theLitVector.begin()));
			ct++;
			while (*xt != SENTINEL_LIT)
			{
				if (writeLit != xt)
					*writeLit = *xt;
				xt++;
				writeLit++;
			}
			*writeLit = NOT_A_LIT;
			writeLit++;
		}

	theLitVector.resize((unsigned int) (writeLit - theLitVector.begin()));

	///////////////////
	// clean up vars
	///////////////////

	DepositOfVars::iterator it, itWriteVar = theVars.begin() + 1;

	for (it = theVars.begin() + 1; it != theVars.end(); it++)
	{
		if (!it->isolated() || it->isActive())
		{
			if (itWriteVar != it)
			{
				*itWriteVar = *it;
				prep_substituteVars(*itWriteVar, itWriteVar - theVars.begin());
			}
			itWriteVar++;
		}
	}

	theVars.erase(itWriteVar, theVars.end());

	it = theVars.begin() + 1;
	///////////////////
	// clean up inCls
	//////////////////

	vector<ClauseIdT>::iterator clt, cltWrite = theInClsVector.begin() + 2;
	for (clt = theInClsVector.begin() + 2; clt != theInClsVector.end(); clt++)
		if (*clt != SENTINEL_CL)
		{
			while (theInClsVector[it->getInClsVecOfs(false)] == SENTINEL_CL
					&& theInClsVector[it->getInClsVecOfs(true)] == SENTINEL_CL)
			{
				it->setInClsVecOfs(false, 0);
				it->setInClsVecOfs(true, 1);
				it++;
			}

			{
				it->setInClsVecOfs((unsigned int) (cltWrite
						- theInClsVector.begin()));

				while (*clt != SENTINEL_CL)
				{
					if (cltWrite != clt)
						*cltWrite = *clt;
					clt++;
					cltWrite++;
				}
				*(cltWrite++) = SENTINEL_CL;
			}
			it++;
		}

	theInClsVector.resize((unsigned int) (cltWrite - theInClsVector.begin()));

	theUnitClauses.clear();
	theUClLookUp.clear();
	theUClLookUp.resize(theVars.size(), X);

	unsigned int countBinCl = 0;
	// as the number of binary clauses might have changed,
	// we have to update the numBinClauses, which keeps track of the # of bin Clauses
	for (it = theVars.begin(); it != theVars.end(); it++)
	{
		countBinCl += it->countBinLinks();
	}
	numBinClauses = countBinCl >> 1;

	toDEBUGOUT("inCls sz:"<<theInClsVector.size()*sizeof(ClauseIdT)<<" "<<endl);
	return true;
}

bool CInstanceGraph::createfromFile(const char* lpstrFileName)
{

	const int BUF_SZ = 65536;
	const int TOK_SZ = 255;

	char buf[BUF_SZ];
	char token[TOK_SZ];
	unsigned int line = 0;
	unsigned int nVars, nCls;
	int lit;
	vector<int> litVec;
	vector<TriValue> seenV;
	int clauseLen = 0;
	TriValue pol;

	vector<int> varPosMap;

	// BEGIN INIT
	reset(); // clear everything
	// END INIT

	///BEGIN File input
	FILE *filedesc;
	filedesc = fopen(lpstrFileName, "r");
	if (filedesc == NULL)
	{
		toERROUT(" Error opening file "<< lpstrFileName<<endl);
		exit(3);
	}
	fclose(filedesc);

	ifstream inFile(lpstrFileName, ios::in);

	// read the preamble of the cnf file
	while (inFile.getline(buf, BUF_SZ))
	{
		line++;
		if (buf[0] == 'c')
			continue;
		if (buf[0] == 'p')
		{
			if (sscanf(buf, "p cnf %d %d", &nVars, &nCls) < 2)
			{
				toERROUT("line "<<line<<": failed reading problem line \n");
				exit(3);
			}
			break;
		}
		else
		{
			toERROUT("line"<<line<<": problem line expected "<<endl);
		}
	}
	originalVarCount = nVars;
	int i, j;
	// now read the data
	while (inFile.getline(buf, BUF_SZ))
	{
		line++;
		i = 0;
		j = 0;
		if (buf[0] == 'c')
			continue;
		while (buf[i] != 0x0)
		{

			while (buf[i] != 0x0 && buf[i] != '-' && (buf[i] < '0' || buf[i]
					> '9'))
				i++;
			while (buf[i] == '-' || buf[i] >= '0' && buf[i] <= '9')
			{
				token[j] = buf[i];
				i++;
				j++;
			}
			token[j] = 0x0;
			lit = atoi(token);
			j = 0;
			if (lit == 0) // end of clause
			{
				if (clauseLen > 0)
					litVec.push_back(0);
				clauseLen = 0;
			}
			else
			{
				clauseLen++;
				litVec.push_back(lit);
			}
		}
	}

	if (!inFile.eof())
	{
		toERROUT(" CNF input: line too long");
	}
	inFile.close();
	/// END FILE input


	vector<int>::iterator it, jt, itEndCl;

	int actVar;
	bool istaut = true;
	int imultipleLits = 0;
	int ilitA, ilitB, lengthCl;
	LiteralIdT LitA, LitB;
	ClauseIdT idCl;

	seenV.resize(nVars + 1, X);
	varPosMap.resize(nVars + 1, -1);
	theVars.reserve(nVars + 1);
	theLitVector.reserve(litVec.size());
	theClauses.reserve(nCls + 10000);

	varTranslation.reserve(nVars + 1);
	varUntranslation.reserve(nVars + 1);
	origTranslation.reserve(nVars + 1);

	theRunAn.init(nVars, nCls);

	vector<vector<ClauseIdT> > _inClLinks[2];

	_inClLinks[0].resize(nVars + 1);
	_inClLinks[1].resize(nVars + 1);

	it = litVec.begin();

	while (it != litVec.end())
	{
		jt = it;
		istaut = false;
		imultipleLits = 0;
		ilitA = 0;
		ilitB = 0; // we pick two literals from each clause for watch- or bin-creation
		lengthCl = 0;
		while (*jt != 0) // jt passes through the clause to determine if it is valid
		{
			actVar = abs(*jt);
			if (seenV[actVar] == X) // literal not seen
			{
				seenV[actVar] = (*jt > 0) ? W : F;
				if (ilitA == 0)
					ilitA = *jt;
				else if (ilitB == 0)
					ilitB = *jt;
				jt++;
			}
			else if (seenV[actVar] == (*jt > 0) ? W : F)
			{ // literal occurs twice: omit it
				*jt = 0;
				imultipleLits++;
				jt++;
			}
			else
			{ // literal in two opposing polarities -> don't include this clause (INVALID)
				istaut = true;
				while (*jt != 0)
					jt++;
				//cout <<"X";
				break;
			}
		}

		itEndCl = jt;
		lengthCl = (int) (itEndCl - it) - imultipleLits;

		if (!istaut && lengthCl > 0) // if the clause is not tautological, add it
		{
#ifdef DEBUG
			if (ilitA == 0)
			{
				toERROUT("ERR");
				exit(3);
			}
#endif

			actVar = abs(ilitA);
			if (varPosMap[actVar] == -1) // create new Var if not present yet
				varPosMap[actVar] = makeVariable(actVar);

			LitA = LiteralIdT(varPosMap[actVar], (ilitA > 0) ? W : F);

			if (ilitB != 0)// determine LiteralIdT for ilitB
			{
				actVar = abs(ilitB);
				if (varPosMap[actVar] == -1) // create new Var if not present yet
					varPosMap[actVar] = makeVariable(actVar);

				LitB = LiteralIdT(varPosMap[actVar], (ilitB > 0) ? W : F);
			}

			if (lengthCl == 1)
			{
				theUnitClauses.push_back(LitA);
			}
			else if (lengthCl == 2)
			{
#ifdef DEBUG
				if (ilitB == 0)
				{
					toERROUT("ERR BIN CL");
					exit(3);
				}
#endif

				if (!getVar(LitA).hasBinLinkTo(LitB, LitA.polarity()))
				{
					getVar(LitA).addBinLink(LitA.polarity(), LitB);
					getVar(LitB).addBinLink(LitB.polarity(), LitA);
					numBinClauses++;
				}
			}
			else
			{
#ifdef DEBUG
				if (ilitB == 0)
				{
					toERROUT("ERR CL");
					exit(3);
				}
#endif
				idCl = makeClause();
				getClause(idCl).setLitOfs(theLitVector.size());

				theLitVector.push_back(LitA);

				/// new
				_inClLinks[LitA.polarity()][LitA.toVarIdx()].push_back(idCl);
				getVar(LitA).scoreDLIS[LitA.polarity()]++;
				///
				theLitVector.push_back(LitB);

				/// new
				_inClLinks[LitB.polarity()][LitB.toVarIdx()].push_back(idCl);
				getVar(LitB).scoreDLIS[LitB.polarity()]++;
				///

				for (jt = it + 2; jt != itEndCl; jt++)
					if (*jt != 0 && *jt != ilitB) // add all nonzero literals
					{
						actVar = abs(*jt);
						pol = (*jt > 0) ? W : F;
						if (varPosMap[actVar] == -1) // create new Var
							varPosMap[actVar] = makeVariable(actVar);

						// add lit to litvector
						theLitVector.push_back(LiteralIdT(varPosMap[actVar],
								pol));
						/// new
						_inClLinks[pol][varPosMap[actVar]].push_back(idCl);
						getVar(varPosMap[actVar]).scoreDLIS[pol]++;
						///
					}
				// make an end: SENTINEL_LIT
				theLitVector.push_back(SENTINEL_LIT);

				getClause(idCl).setLitA(LitA);
				getClause(idCl).setLitB(LitB);
				getClause(idCl).setLength(lengthCl);

				getVar(LitA).addWatchClause(idCl, LitA.polarity());
				getVar(LitB).addWatchClause(idCl, LitB.polarity());
			}

		}

		// undo the entries in seenV
		for (jt = it; jt != itEndCl; jt++)
			seenV[abs(*jt)] = X;

		it = itEndCl;
		it++;
	}

	//BEGIN initialize theInClsVector
	theInClsVector.clear();
	theInClsVector.reserve(theLitVector.size() + nVars);
	theInClsVector.push_back(SENTINEL_CL);
	vector<ClauseIdT>::iterator clt;
	for (unsigned int i = 0; i <= nVars; i++)
	{
		getVar(i).setInClsVecOfs(false, theInClsVector.size());
		for (clt = _inClLinks[0][i].begin(); clt != _inClLinks[0][i].end(); clt++)
		{
			theInClsVector.push_back(*clt);
		}

		getVar(i).setInClsVecOfs(true, theInClsVector.size());
		for (clt = _inClLinks[1][i].begin(); clt != _inClLinks[1][i].end(); clt++)
		{
			theInClsVector.push_back(*clt);
		}
		theInClsVector.push_back(SENTINEL_CL);
	}
	//END initialize theInClsVector

#ifdef DEBUG
	assert(theInClsVector.size() <= theLitVector.size() + nVars + 1);
	toDEBUGOUT("inCls sz:"<<theInClsVector.size()*sizeof(ClauseIdT)<<" "<<endl);
	toDEBUGOUT("lsz: "<< theLitVector.size()*sizeof(unsigned int)<< " bytes"<<endl);
#endif

	theUClLookUp.resize(theVars.size() + 1, X);
	iOfsBeginConflictClauses = theClauses.size();

	theRunAn.setUsedVars(countAllVars());

	// Store the original translation
	origTranslation.clear();
	origTranslation.resize(varPosMap.size(), -1);
	for (int i = 0; i < varPosMap.size(); i++)
	{
		if (-1 != varPosMap[i])
			origTranslation[varPosMap[i]] = i;
	}

	//----- This is a good place to set up the var(Un)Translation
	//--- Clear it out
	varTranslation.clear();
	varUntranslation.clear();

	varTranslation.resize(nVars + 1);
	varUntranslation.resize(nVars + 1);

	//--- Put in the initial 0 (since variables start at 1)
	//---  and add the default values for all variables
	for (unsigned int i = 0; i <= countAllVars(); i++)
	{
		varTranslation[(int) i] = (int) i;
		varUntranslation[(int) i] = (int) i;
	}

	return true;
}

unsigned int CInstanceGraph::countActiveBinLinks(VarIdT theVar) const
{
	unsigned int n = 0;

	const CVariableVertex &rVar = getVar(theVar);
	vector<LiteralIdT>::const_iterator bt;

	for (bt = rVar.getBinLinks(true).begin(); bt
			!= rVar.getBinLinks(true).end(); bt++)
	{
		if (*bt != SENTINEL_LIT)
			n += (unsigned int) getVar(*bt).isActive();
	}

	for (bt = rVar.getBinLinks(false).begin(); bt
			!= rVar.getBinLinks(false).end(); bt++)
	{
		if (*bt != SENTINEL_LIT)
			n += (unsigned int) getVar(*bt).isActive();
	}

	return n;
}

void CInstanceGraph::convertComponent(CComponentId &oldComp,
		vector<int> * newComp)
{
	vector<ClauseIdT>::const_iterator it;
	for (it = oldComp.clsBegin(); *it != clsSENTINEL; it++)
	{
		vector<LiteralIdT>::const_iterator lit;

		for (lit = begin(getClause(*it)); *lit != ClauseEnd(); lit++)
		{
			int sign = ((*lit).polarity()) ? 1 : -1;
			int val = (*lit).toVarIdx();
			newComp->push_back(sign * val);
		}
		newComp->push_back(0);
	}
}

void CInstanceGraph::print()
{
	DepositOfClauses::iterator it;
	for (it = theClauses.begin() + 1; it != theClauses.end(); it++)
	{
		printCl(*it);
	}
}

void CInstanceGraph::printClause(const ClauseIdT &idCl) const
{
	vector<LiteralIdT>::const_iterator it;
	toSTDOUT("(");

	for (it = begin(getClause(idCl)); *it != ClauseEnd(); it++)
	{
		(*it).print();
	}
	toSTDOUT(")");
}

void CInstanceGraph::printActiveClause(const ClauseIdT &idCl) const
{
	vector<LiteralIdT>::const_iterator it;
	toSTDOUT("(");

	for (it = begin(getClause(idCl)); *it != ClauseEnd(); it++)
	{
		if (getVar(*it).isActive())
		{
			(*it).print();
		}
	}
	toSTDOUT(")");
}
