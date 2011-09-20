#include "MainSolver.h" // class's header file
// class constructor
CMainSolver::CMainSolver() :
	decStack(*this)
{
	stopWatch.setTimeBound(CSolverConf::secsTimeBound);
	remPoll = 0;
}

// class destructor
CMainSolver::~CMainSolver()
{
	toDEBUGOUT("removed. Poll: " << remPoll << endl);
}

bool CMainSolver::performPreProcessing()
{
	if (!prepBCP())
	{
		theRunAn.setExitState(SUCCESS);
		stopWatch.markStopTime();
		return false;
	}
	if (CSolverConf::allowPreProcessing)
	{
		toSTDOUT("BEGIN preprocessing" << endl);
		if (!prepFindHiddenBackBoneLits())
		{
			theRunAn.setExitState(SUCCESS);
			toSTDOUT("ERR: UNSAT Formula" << endl);
			stopWatch.markStopTime();
			return false;
		}
		toSTDOUT(endl << "END preprocessing" << endl);
	}

	prep_CleanUpPool();

	toSTDOUT("#Vars remaining:" << countAllVars() << endl);
	toSTDOUT("#Clauses remaining:" << countAllClauses() << endl);
	toSTDOUT("#bin Cls remaining:" << countBinClauses() << endl);

	return true;
}

void CMainSolver::solve(const char *lpstrFileName)
{
	SOLVER_StateT exSt;

	stopWatch.markStartTime();

	num_Nodes = 3;

	createfromFile(lpstrFileName);

	decStack.init(countAllVars());

	CStepTime::makeStart();

	xFormulaCache.init();

	bdg_var_count = originalVarCount;

	// Create the initial LIT nodes for quick caching
	for (int i = 0; i <= originalVarCount; ++i)
	{
		litNodes.push_back(new DTNode(i, true, num_Nodes++));
		litNodes.push_back(new DTNode(-1 * i, true, num_Nodes++));
	}

	toSTDOUT("#Vars:" << countAllVars() << endl);
	toSTDOUT("#Clauses:" << countAllClauses() << endl);
	toSTDOUT("#bin Cls:" << countBinClauses() << endl);

	bool needToCount = performPreProcessing();

	theRunAn.setRemovedClauses(theRunAn.getData().nRemovedClauses
			+ theRunAn.getData().nOriginalClauses - getMaxOriginalClIdx() + 1
			- countBinClauses());

	if (needToCount)
	{
		// the following call only correct if bin clauses not used for caching
		CCacheEntry::adjustPackSize(countAllVars(), getMaxOriginalClIdx());

		lastTimeCClDeleted = CStepTime::getTime();
		lastCClCleanUp = CStepTime::getTime();
		makeCompIdFromActGraph(decStack.TOSRefComp());
		bcpImplQueue.clear();
		bcpImplQueue.reserve(countAllVars());

		// the size of componentSearchStack has to be reserved, otherwise getComp(...) might
		// become erroneous due to realloc
		componentSearchStack.reserve(countAllVars() + 2);

		exSt = countSAT();

		theRunAn.setExitState(exSt);
		theRunAn.setSatCount(decStack.top().getOverallSols());
	}
	else
	{

		theRunAn.setExitState(SUCCESS);
		theRunAn.setSatCount(0.0);
	}

	stopWatch.markStopTime();

	theRunAn.setElapsedTime(stopWatch.getElapsedTime());

	xFormulaCache.printStatistics(theRunAn);

	// There may have been some translation done during the preprocessing
	//  phase, so we translate the bdg literals back.
	decStack.top().getDTNode()->uncheck(1);
	decStack.top().getDTNode()->translateLiterals(getVarTranslation());

	// We also need to add all of the backbones found in preprocessing, but
	//  we only do this if there are solutions.
	if (decStack.top().getOverallSols() != 0)
	{
		vector<LiteralIdT>::iterator bbit;
		for (bbit = backbones.begin(); bbit != backbones.end(); bbit++)
		{
			//toSTDOUT("Adding backbone lit: " << bbit->toSignedInt() << endl);
			decStack.top().getCurrentDTNode()->addChild(get_lit_node(
					bbit->toSignedInt()));
		}
	}

	// We compress the tree now that the search is finished
	decStack.top().getDTNode()->uncheck(2);
	cout << "Uncompressed Edges: " << decStack.top().getDTNode()->count(true)
			<< endl;

	decStack.top().getDTNode()->uncheck(3);
	decStack.top().getDTNode()->compressNode();

	decStack.top().getDTNode()->uncheck(4);
	bdg_edge_count = decStack.top().getDTNode()->count(true);
	cout << "Compressed Edges: " << bdg_edge_count << endl;

	// There may have been some translation done during the file parsing
	//  phase, so we translate the bdg literals back.
	decStack.top().getDTNode()->uncheck(5);
	decStack.top().getDTNode()->translateLiterals(getOrigTranslation());
}

SOLVER_StateT CMainSolver::countSAT()
{
	retStateT res;

	while (true)
	{
		while (decide())
		{
			if (stopWatch.timeBoundBroken())
				return TIMEOUT;

			while (!bcp())
			{
				res = resolveConflict();
				if (res == EXIT)
					return SUCCESS;
				if (res == PROCESS_COMPONENT)
					break; //force calling decide()
			}
		}

		res = backTrack();
		if (res == EXIT)
			return SUCCESS;

		while (res != PROCESS_COMPONENT && !bcp())
		{
			res = resolveConflict();
			if (res == EXIT)
				return SUCCESS;
		}
	}
}

bool CMainSolver::findVSADSDecVar(LiteralIdT &theLit,
		const CComponentId & superComp)
{
	vector<VarIdT>::const_iterator it;

	int score = -1;
	int bo;
	PVARV pVV = NULL;

	for (it = superComp.varsBegin(); *it != varsSENTINEL; it++)
		if (getVar(*it).isActive())
		{
			bo = (int) getVar(*it).getVSIDSScore() + getVar(*it).getDLCSScore();

			if (bo > score)
			{
				score = bo;
				pVV = &getVar(*it);
			}
		}

	if (pVV == NULL)
		return false;

	bool pol = pVV->scoreDLIS[true] + pVV->scoreVSIDS[true]
			> pVV->scoreDLIS[false] + pVV->scoreVSIDS[false];

	theLit = pVV->getLitIdT(pol);

	return true;
}

bool CMainSolver::decide()
{
	LiteralIdT theLit(NOT_A_LIT);

	CStepTime::stepTime(); // Solver-Zeit


	if (CStepTime::getTime() - xFormulaCache.getLastDivTime()
			> xFormulaCache.getScoresDivTime())
	{
		xFormulaCache.divCacheScores();
		xFormulaCache.setLastDivTime(CStepTime::getTime());
	}

	if (!bcpImplQueue.empty())
		return true;

	if (!decStack.TOS_hasAnyRemComp())
	{
		recordRemainingComps();
		if (decStack.TOS_countRemComps() == 0)
		{
			handleSolution();
			return false;
		}
	}

	if (!findVSADSDecVar(theLit, decStack.TOS_NextComp())
			|| decStack.TOS_NextComp().getClauseCount() == 0) // i.e. component satisfied
	{
		handleSolution();
		decStack.TOS_popRemComp();
		return false;
	}

	//checkCachedCompVal:
	static CRealNum cacheVal;

	//decStack.TOS_NextComp();

	if (CSolverConf::allowComponentCaching && xFormulaCache.extract(
			decStack.TOS_NextComp(), cacheVal,
			decStack.top().getCurrentDTNode()))
	{
		decStack.top().includeSol(cacheVal);
		theRunAn.addValue(SOLUTION, decStack.getDL());
		decStack.TOS_popRemComp();
		return false;
	}
	/////////////////////////////

	// Create the nodes
	DTNode * newNode = new DTNode(DT_OR, num_Nodes++);
	DTNode * left = new DTNode(DT_AND, num_Nodes++);
	DTNode * right = new DTNode(DT_AND, num_Nodes++);
	DTNode * leftLit = get_lit_node(theLit.toSignedInt());
	DTNode * rightLit = get_lit_node(-1 * theLit.toSignedInt());

	newNode->choiceVar = theLit.toSignedInt();

	// Set the parents
	left->addParent(newNode, true);
	leftLit->addParent(left, true);
	right->addParent(newNode, true);
	rightLit->addParent(right, true);
	newNode->addParent(decStack.top().getCurrentDTNode(), true);

	decStack.push(newNode);
	bcpImplQueue.clear();
	bcpImplQueue.push_back(AntAndLit(NOT_A_CLAUSE, theLit));

	theRunAn.addValue(DECISION, decStack.getDL());

	if (theRunAn.getData().nDecisions % 255 == 0)
	{
		doVSIDSScoreDiv();
	}

	return true;
}

void CMainSolver::removeAllCachePollutions()
{
	// We are removing the pollutions because a conflict was found, so the
	//  current decision level must be reset to only contain the literal.

	// TODO: Confirm that this is not required for the graph
	//decStack.top().getCurrentDTNode()->reset();

	// check if one sibling has modelcount 0
	// if so, the other siblings may have polluted the cache --> remove pollutions
	// cout << decStack.getDL()<<" "<<endl;
	for (vector<CComponentId *>::iterator it = decStack.TOSRemComps_begin(); it
			!= decStack.getAllCompStack().end(); it++)
		// if component *it is cached remove it and all of its descendants
		if ((*it)->cachedAs != 0)
		{
			remPoll += xFormulaCache.removePollutedEntries((*it)->cachedAs);
			(*it)->cachedAs = 0;
			(*it)->cachedChildren.clear();
		}
		// it might occur that *it is not yet cached, but has descedants that are cached
		// thus we have to delete them
		else if (!(*it)->cachedChildren.empty())
		{
			for (vector<unsigned int>::iterator ct =
					(*it)->cachedChildren.begin(); ct
					!= (*it)->cachedChildren.end(); ct++)
			{
				remPoll += xFormulaCache.removePollutedEntries(*ct);
			}
		}
	xFormulaCache.revalidateCacheLinksIn(decStack.getAllCompStack());

}

retStateT CMainSolver::backTrack()
{
	unsigned int refTime = theRunAn.getData().nAddedClauses;
	LiteralIdT aLit;

	if (refTime - lastTimeCClDeleted > 1000 && countCCls() > 3000)
	{
		deleteConflictCls();
		lastTimeCClDeleted = refTime;
	}

	if (refTime - lastCClCleanUp > 35000)
	{
		cleanUp_deletedCCls();
		toSTDOUT("cleaned:");
		printCClstats();
		lastCClCleanUp = refTime;
	}

	//component cache delete old entries
	if (CSolverConf::allowComponentCaching)
		xFormulaCache.deleteEntries(decStack);

	do
	{
		if (decStack.top().getBranchSols() == 0.0)
		{
			removeAllCachePollutions();
		}

		if (decStack.top().anotherCompProcessible())
		{
			return PROCESS_COMPONENT;
		}

		if (!decStack.top().isFlipped())
		{
			aLit = decStack.TOS_decLit();
			decStack.flipTOS();
			bcpImplQueue.push_back(AntAndLit(NOT_A_CLAUSE, aLit.oppositeLit()));

			return RESOLVED;
		}

		//include the component value of the current component into the Cache
		if (CSolverConf::allowComponentCaching)
		{
			xFormulaCache.include(decStack.TOSRefComp(),
					decStack.top().getOverallSols(),
					decStack.top().getOrDTNode());
		}

	} while (decStack.pop());

	return EXIT;
}

bool CMainSolver::bcp()
{
	bool bSucceeded;

	vector<LiteralIdT>::iterator it;

	//BEGIN process unit clauses
	for (it = theUnitClauses.begin(); it != theUnitClauses.end(); it++)
	{
		if (getVar(*it).isActive())
			bcpImplQueue.push_back(AntAndLit(NOT_A_CLAUSE, *it));

		if (isUnitCl(it->oppositeLit()))
		{
			// dealing with opposing unit clauses is handed over to
			// resolveConflict
			return false;
		}
	}
	//END process unit clauses

	bSucceeded = BCP(bcpImplQueue);
	bcpImplQueue.clear();

	if (CSolverConf::allowImplicitBCP && bSucceeded)
	{
		bSucceeded = implicitBCP();
	}

	theRunAn.addValue(IMPLICATION, decStack.getDL(),
			decStack.TOS_countImplLits());

	return bSucceeded;
}

retStateT CMainSolver::resolveConflict()
{
	int backtrackDecLev;

	// Since we have a conflict, we should add a bottom to the current DTNode
	DTNode * newBot = new DTNode(DT_BOTTOM, num_Nodes++);
	newBot->addParent(decStack.top().getCurrentDTNode(), true);

	for (vector<LiteralIdT>::iterator it = theUnitClauses.begin(); it
			!= theUnitClauses.end(); it++)
	{
		if (isUnitCl(it->oppositeLit()))
		{
			toSTDOUT("\nOPPOSING UNIT CLAUSES - INSTANCE UNSAT\n");
			return EXIT;
		}
	}

	theRunAn.addValue(CONFLICT, decStack.getDL(), 1);

#ifdef DEBUG
	assert(!getConflicted().empty());
#endif

	if (!CSolverConf::analyzeConflicts || getConflicted().empty())
		return backTrack();

	caGetCauses_firstUIP(getConflicted());
	backtrackDecLev = getMaxDecLevFromAnalysis();

	if (ca_1UIPClause.size() < ca_lastUIPClause.size())
		create1UIPCCl();
	//BEGIN Backtracking

	if (backtrackDecLev < decStack.getDL())
	{
		if (CSolverConf::doNonChronBackTracking)
			while (decStack.getDL() > backtrackDecLev)
			{
				/// check for polluted cache Entries
				removeAllCachePollutions();
				decStack.pop();

				newBot = new DTNode(DT_BOTTOM, num_Nodes++);
				newBot->addParent(decStack.top().getCurrentDTNode(), true);

			}
		return backTrack();

	}

	// maybe the other branch had some solutions
	if (decStack.top().isFlipped())
	{
		return backTrack();
	}

	// now: if the other branch has to be visited:

	createAntClauseFor(decStack.TOS_decLit().oppositeLit());

	LiteralIdT aLit = decStack.TOS_decLit();
	AntecedentT ant = getVar(aLit).getAntecedent();
	decStack.flipTOS();

	bcpImplQueue.push_back(AntAndLit(ant, aLit.oppositeLit()));
	//END Backtracking
	return RESOLVED;
}

/////////////////////////////////////////////////////
// BEGIN component analysis
/////////////////////////////////////////////////////


bool CMainSolver::recordRemainingComps()
{
	// the refComp has to be copied!! because in case that the vector
	// containing it has to realloc (which might happen here)
	// copying the refcomp is not necessary anymore, as the allCompsStack is a vector
	// of pointers - realloc does not invalidate these
	CComponentId & refSupComp = decStack.TOSRefComp();

	viewStateT lookUpCls[getMaxOriginalClIdx() + 2];
	viewStateT lookUpVars[countAllVars() + 2];

	memset(lookUpCls, NIL, sizeof(viewStateT) * (getMaxOriginalClIdx() + 2));
	memset(lookUpVars, NIL, sizeof(viewStateT) * (countAllVars() + 2));

	vector<VarIdT>::const_iterator vt;
	vector<ClauseIdT>::const_iterator itCl;

	for (itCl = refSupComp.clsBegin(); *itCl != clsSENTINEL; itCl++)
	{
		lookUpCls[*itCl] = IN_SUP_COMP;
	}

	for (vt = refSupComp.varsBegin(); *vt != varsSENTINEL; vt++)
	{
		if (getVar(*vt).isActive())
		{
			lookUpVars[*vt] = IN_SUP_COMP;
			getVar(*vt).scoreDLIS[true] = 0;
			getVar(*vt).scoreDLIS[false] = 0;
		}
	}

	vector<LiteralIdT>::const_iterator itL;

	for (vt = refSupComp.varsBegin(); *vt != varsSENTINEL; vt++)
		if (lookUpVars[*vt] == IN_SUP_COMP)
		{
			decStack.TOS_addRemComp();

			getComp(*vt, decStack.TOSRefComp(), lookUpCls, lookUpVars);
		}

	decStack.TOS_sortRemComps();
	return true;
}

bool CMainSolver::getComp(const VarIdT &theVar, const CComponentId &superComp,
		viewStateT lookUpCls[], viewStateT lookUpVars[])
{
	componentSearchStack.clear();

	lookUpVars[theVar] = SEEN;
	componentSearchStack.push_back(theVar);
	vector<VarIdT>::const_iterator vt, itVEnd;

	vector<LiteralIdT>::const_iterator itL;
	vector<ClauseIdT>::const_iterator itCl;

	CVariableVertex * pActVar;

	unsigned int nClausesSeen = 0, nBinClsSeen = 0;

	for (vt = componentSearchStack.begin(); vt != componentSearchStack.end(); vt++)
	// the for-loop is applicable here because componentSearchStack.capacity() == countAllVars()
	{
		pActVar = &getVar(*vt);
		//BEGIN traverse binary clauses
		for (itL = pActVar->getBinLinks(true).begin(); *itL != SENTINEL_LIT; itL++)
			if (lookUpVars[itL->toVarIdx()] == IN_SUP_COMP)
			{
				nBinClsSeen++;
				lookUpVars[itL->toVarIdx()] = SEEN;
				componentSearchStack.push_back(itL->toVarIdx());
				getVar(*itL).scoreDLIS[itL->polarity()]++;
				pActVar->scoreDLIS[true]++;
			}
		for (itL = pActVar->getBinLinks(false).begin(); *itL != SENTINEL_LIT; itL++)
			if (lookUpVars[itL->toVarIdx()] == IN_SUP_COMP)
			{
				nBinClsSeen++;
				lookUpVars[itL->toVarIdx()] = SEEN;
				componentSearchStack.push_back(itL->toVarIdx());
				getVar(*itL).scoreDLIS[itL->polarity()]++;
				pActVar->scoreDLIS[false]++;
			}
		//END traverse binary clauses


		for (itCl = var_InClsBegin(*pActVar); *itCl != SENTINEL_CL; itCl++)
			if (lookUpCls[*itCl] == IN_SUP_COMP)
			{
				itVEnd = componentSearchStack.end();
				for (itL = begin(getClause(*itCl)); *itL != ClauseEnd(); itL++)
					if (lookUpVars[itL->toVarIdx()] == NIL) //i.e. the var is not active
					{
						if (!isSatisfied(*itL))
							continue;
						//BEGIN accidentally entered a satisfied clause: undo the search process
						while (componentSearchStack.end() != itVEnd)
						{
							lookUpVars[componentSearchStack.back()]
									= IN_SUP_COMP;
							componentSearchStack.pop_back();
						}
						lookUpCls[*itCl] = NIL;
						for (vector<LiteralIdT>::iterator itX = begin(
								getClause(*itCl)); itX != itL; itX++)
						{
							if (getVar(*itX).scoreDLIS[itX->polarity()] > 0)
								getVar(*itX).scoreDLIS[itX->polarity()]--;
						}
						//END accidentally entered a satisfied clause: undo the search process
						break;

					}
					else
					{

						getVar(*itL).scoreDLIS[itL->polarity()]++;
						if (lookUpVars[itL->toVarIdx()] == IN_SUP_COMP)
						{
							lookUpVars[itL->toVarIdx()] = SEEN;
							componentSearchStack.push_back(itL->toVarIdx());
						}
					}

				if (lookUpCls[*itCl] == NIL)
					continue;
				nClausesSeen++;
				lookUpCls[*itCl] = SEEN;
			}
	}

	/////////////////////////////////////////////////
	// BEGIN store variables in resComp
	/////////////////////////////////////////////////

	decStack.lastComp().reserveSpace(componentSearchStack.size(), nClausesSeen);

	for (vt = superComp.varsBegin(); *vt != varsSENTINEL; vt++)
		if (lookUpVars[*vt] == SEEN) //we have to put a var into our component
		{
			decStack.lastComp().addVar(*vt);
			lookUpVars[*vt] = IN_OTHER_COMP;
		}

	decStack.lastComp().addVar(varsSENTINEL);

	/////////////////////////////////////////////////
	// END store variables in resComp
	/////////////////////////////////////////////////

	for (itCl = superComp.clsBegin(); *itCl != clsSENTINEL; itCl++)
		if (lookUpCls[*itCl] == SEEN)
		{
			decStack.lastComp().addCl(*itCl);
			lookUpCls[*itCl] = IN_OTHER_COMP;
		}
	decStack.lastComp().addCl(clsSENTINEL);
	decStack.lastComp().setTrueClauseCount(nClausesSeen + nBinClsSeen);

	return true;
}

/////////////////////////////////////////////////////
// END component analysis
/////////////////////////////////////////////////////


void CMainSolver::makeCompIdFromActGraph(CComponentId& rComp)
{
	DepositOfClauses::iterator it;
	DepositOfVars::iterator jt;

	rComp.clear();
	unsigned int nBinClauses = 0;
	unsigned int idx = 1;
	for (it = beginOfClauses(); it != endOfClauses(); it++, idx++)
	{
#ifdef DEBUG
		assert(!it->isDeleted()); // deleted Cls should be cleaned by prepCleanPool
#endif
		rComp.addCl(ClauseIdT(idx));
	}
	rComp.addCl(clsSENTINEL);
	idx = 1;
	for (jt = beginOfVars(); jt != endOfVars(); jt++, idx++)
	{
#ifdef DEBUG
		assert(jt->isActive()); // deleted Vars should be cleaned by prepCleanPool
#endif
		rComp.addVar(idx);
		nBinClauses += countActiveBinLinks(idx);

	}
	rComp.addVar(varsSENTINEL);

	nBinClauses >>= 1;
	rComp.setTrueClauseCount(nBinClauses + rComp.countCls());
}

void CMainSolver::printComponent(const CComponentId& rComp)
{
	vector<ClauseIdT>::const_iterator it;
	for (it = rComp.clsBegin(); *it != clsSENTINEL; it++)
	{
		printActiveClause(*it);
	}
}

///////////////////////////////////////////////

bool CMainSolver::BCP(vector<AntAndLit> &thePairsOfImpl)
{
	extd_vector<ClauseIdT>* pUnWatchCls;
	extd_vector<ClauseIdT>::iterator it;
	bool retV;
	ClauseIdT actCl;
	PCLV pCl;
	vector<LiteralIdT>::const_iterator bt;
	vector<LiteralIdT>::iterator itL;
	LiteralIdT satLit, unLit;

	getConflicted().clear();

	for (unsigned int i = 0; i < thePairsOfImpl.size(); i++)
		if (assignVal(thePairsOfImpl[i].getLit(), thePairsOfImpl[i].getAnt()))
		{
			satLit = thePairsOfImpl[i].getLit();
			unLit = thePairsOfImpl[i].getLit().oppositeLit();
			decStack.TOS_addImpliedLit(satLit);

#ifdef FULL_DDNNF
			if (enable_DT_recording)
			{
				DTNode * satLitDTNode = get_lit_node(satLit.toSignedInt());
				satLitDTNode->addParent(decStack.top().getCurrentDTNode(), true);
				dirtyLitNodes.push_back(pair<DTNode *, DTNode *> (
						decStack.top().getCurrentDTNode(), satLitDTNode));
			}
#endif

			pUnWatchCls = &getVar(unLit).getWatchClauses(unLit.polarity());

			//BEGIN Propagate Bin Clauses
			/* First propagate the original binary clauses */
			for (bt = getVar(unLit).getBinLinks(unLit.polarity()).begin(); *bt
					!= SENTINEL_LIT; bt++)
			{
				if (getVar(*bt).isActive())
				{
					thePairsOfImpl.push_back(AntAndLit(unLit, *bt));

#ifdef FULL_DDNNF
					if (enable_DT_recording)
					{
						DTNode * ccLit = get_lit_node((*bt).toSignedInt());
						ccLit->addParent(decStack.top().getCurrentDTNode(), true);
						dirtyLitNodes.push_back(pair<DTNode *, DTNode*> (
								decStack.top().getCurrentDTNode(), ccLit));
					}
#endif
				}
				else if (!isSatisfied(*bt))
				{
					getConflicted().push_back(unLit);
					getConflicted().push_back(*bt);
					return false;
				}
			}
			/* Next propagate the conflict generated binary clauses */
			for (bt = bt + 1; *bt != SENTINEL_LIT; bt++)
			{
				if (getVar(*bt).isActive())
				{
					thePairsOfImpl.push_back(AntAndLit(unLit, *bt));

					if (enable_DT_recording)
					{
						// Add the implied literal due to a conflict clause
						DTNode * ccLit = get_lit_node((*bt).toSignedInt());
						ccLit->addParent(decStack.top().getCurrentDTNode(), true);
						dirtyLitNodes.push_back(pair<DTNode *, DTNode*> (
								decStack.top().getCurrentDTNode(), ccLit));
					}
				}
				else if (!isSatisfied(*bt))
				{
					getConflicted().push_back(unLit);
					getConflicted().push_back(*bt);
					return false;
				}
			}
			//END Propagate Bin Clauses

			for (it = pUnWatchCls->end() - 1; *it != SENTINEL_CL; it--)
			{
				actCl = *it;
				pCl = &getClause(actCl);
				// all clauses treated as conflict clauses, because no subsumption
				// is being performed
				if (isSatisfied(pCl->idLitA()) || isSatisfied(pCl->idLitB()))
					continue;
				///////////////////////////////////////
				//BEGIN update watched Lits
				///////////////////////////////////////
				retV = false;
				for (itL = begin(*pCl); *itL != ClauseEnd(); itL++)
					if (getVar(*itL).isActive() || isSatisfied(*itL))
					{
						if (*itL == pCl->idLitA() || *itL == pCl->idLitB())
							continue;
						retV = isSatisfied(*itL);
						getVar(*itL).addWatchClause(actCl, itL->polarity());
						if (pCl->idLitA() == unLit)
							pCl->setLitA(*itL);
						else
							pCl->setLitB(*itL);
						pUnWatchCls->quickErase(it);
						break;
					}
				if (retV)
					continue; // i.e. clause satisfied
				///////////////////////////////////////
				//END update watched Lits
				///////////////////////////////////////

				if (getVar(pCl->idLitA()).isActive())
				{
					if (!getVar(pCl->idLitB()).isActive()) //IMPLIES_LITA;
					{
						thePairsOfImpl.push_back(
								AntAndLit(actCl, pCl->idLitA()));
						pCl->setTouched();

						// Add the implied literal due to a conflict clause
#ifndef FULL_DDNNF
						if (pCl->isCC())
						{
#endif
						if (enable_DT_recording)
						{
							DTNode * ccLit = get_lit_node(
									pCl->idLitA().toSignedInt());
							ccLit->addParent(decStack.top().getCurrentDTNode(),
									true);
							dirtyLitNodes.push_back(pair<DTNode *, DTNode*> (
									decStack.top().getCurrentDTNode(), ccLit));
						}
#ifndef FULL_DDNNF
					}
#endif
					}
				}
				else
				{
					pCl->setTouched();
					if (getVar(pCl->idLitB()).isActive())
					{ //IMPLIES_LITB;
						thePairsOfImpl.push_back(
								AntAndLit(actCl, pCl->idLitB()));

						// Add the implied literal due to a conflict clause
#ifndef FULL_DDNNF
						if (pCl->isCC())
						{
#endif
						if (enable_DT_recording)
						{
							DTNode * ccLit = get_lit_node(
									pCl->idLitB().toSignedInt());
							ccLit->addParent(decStack.top().getCurrentDTNode(),
									true);
							dirtyLitNodes.push_back(pair<DTNode *, DTNode*> (
									decStack.top().getCurrentDTNode(), ccLit));
						}
#ifndef FULL_DDNNF
					}
#endif
					}
					else //provided that SATISFIED has been tested
					{
						getConflicted().push_back(actCl);
						return false;
					}
				}
			}
		}
	return true;
}

bool CMainSolver::createAntClauseFor(const LiteralIdT &Lit)
{
	const vector<LiteralIdT> &actCCl = caGetPrevLastUIPCCl();

	bool created = createLastUIPCCl();

	if (!created && actCCl.size() != 2)
		return false;

	if (actCCl.size() == 1)
	{
		getVar(Lit).adjustAntecedent(NOT_A_CLAUSE);
		return true; // theLit has become a unit clause: nothing to be done
	}
	if (actCCl.size() == 2)
	{
#ifdef DEBUG
		assert(actCCl.front() != Lit.oppositeLit());
		assert(actCCl.back() != Lit.oppositeLit());
#endif

		if (actCCl.front() == Lit)
			getVar(Lit).adjustAntecedent(AntecedentT(actCCl.back()));
		else if (actCCl.back() == Lit)
			getVar(Lit).adjustAntecedent(AntecedentT(actCCl.front()));

		return true;
	}
	setCClImplyingLit(getLastClauseId(), Lit);
	getVar(Lit).adjustAntecedent(AntecedentT(getLastClauseId()));
	return true;
}

bool CMainSolver::implicitBCP()
{
	vector<LiteralIdT>::iterator jt;
	vector<LiteralIdT>::const_iterator it, lt;
	vector<ClauseIdT>::const_iterator ct;

	static vector<AntAndLit> implPairs;
	implPairs.clear();
	vector<LiteralIdT> nextStep;
	bool viewedLits[(countAllVars() + 1) * 2 + 1];
	LiteralIdT newLit;
	AntecedentT theAnt, nant;
	LiteralIdT theLit, nlit;
	implPairs.reserve(decStack.TOSRefComp().countVars());
	nextStep.reserve(decStack.TOSRefComp().countVars());
	int allFound = 0;
	bool bsat;

	int impOfs = 0;
	int step;
	do /*  while (decStack.TOS_countImplLits() - impOfs > 0);  */
	{
		if (decStack.TOS_countImplLits() - impOfs > 0)
		{
			memset(viewedLits, false, sizeof(bool) * ((countAllVars() + 1) * 2
					+ 1));
			nextStep.clear();
			for (it = decStack.TOS_ImpliedLits_begin() + impOfs; it
					!= decStack.TOS_ImpliedLits_end(); it++)
			{
				step = var_InClsStep(!it->polarity());
				for (ct = var_InClsStart(it->toVarIdx(), !it->polarity()); *ct
						!= SENTINEL_CL; ct += step)
				{
					bsat = false;
					for (lt = begin(getClause(*ct)); *lt != ClauseEnd(); lt++)
					{
						if (isSatisfied(*lt))
						{
							bsat = true;
							break;
						}
					}

					if (bsat)
						continue;
					for (lt = begin(getClause(*ct)); *lt != ClauseEnd(); lt++)
						if (getVar(*lt).isActive() && !viewedLits[lt->toUInt()])
						{
							nextStep.push_back(*lt);
							viewedLits[lt->toUInt()] = true;
						}
				}
			}
		}
		impOfs = decStack.TOS_countImplLits();

		for (jt = nextStep.begin(); jt != nextStep.end(); jt++)
			if (getVar(*jt).isActive())
			{
				implPairs.push_back(AntAndLit(NOT_A_CLAUSE, jt->oppositeLit()));
				// BEGIN BCPROPAGATIONNI
				unsigned int sz = decStack.countAllImplLits();

				// we increase the decLev artificially
				// s.t. after the tentative BCP call, we can learn a conflict clause
				// relative to the assignment of *jt
				decStack.beginTentative();

				dirtyLitNodes.clear();

				bool bSucceeded = BCP(implPairs);

				/* Regardless of what happens, we don't want to store the lits added
				 * to the decision tree during the first BCP. If it succeeded, we aren't
				 * keeping anything around anyways, and if it failed, we'll only want
				 * to keep what happens with the opposite setting.
				 */
				for (int i = 0; i < dirtyLitNodes.size(); ++i)
				{
					dirtyLitNodes[i].first->childDeleted(
							dirtyLitNodes[i].second);
					dirtyLitNodes[i].second->parentDeleted(
							dirtyLitNodes[i].first);
				}

				decStack.shrinkImplLitsTo(sz);
				theAnt = NOT_A_CLAUSE;
				theLit = implPairs[0].getLit();

				if (!bSucceeded && CSolverConf::analyzeConflicts
						&& !getConflicted().empty())
				{

					caGetCauses_lastUIP(getConflicted());

					createAntClauseFor(theLit.oppositeLit());
					//createLastUIPCCl();

					theRunAn.addValue(IBCPIMPL, decStack.getDL(), 1);
					theAnt = getVar(theLit).getAntecedent();

				}
				decStack.endTentative();

				for (vector<AntAndLit>::iterator at = implPairs.begin(); at
						!= implPairs.end(); at++)
					getVar(at->theLit).unsetVal();
				implPairs.clear();

				if (!bSucceeded)
				{
					implPairs.push_back(AntAndLit(theAnt, theLit.oppositeLit()));

					dirtyLitNodes.clear();

					if (!BCP(implPairs))
					{
						implPairs.clear();

						/* We may have added litNodes to the decision tree because of
						 * conflict clause unit propagations. If this is the case, then
						 * they will be located in the dirtyLitNodes vector, and we turn
						 * them true to invalidate them (since they're under an AND node).
						 */
						for (int i = 0; i < dirtyLitNodes.size(); ++i)
						{
							dirtyLitNodes[i].first->childDeleted(
									dirtyLitNodes[i].second);
							dirtyLitNodes[i].second->parentDeleted(
									dirtyLitNodes[i].first);
						}

						return false;
					}

					// Add the successful ibcp lit to the graph
					DTNode * ibcpLit = get_lit_node(
							theLit.oppositeLit().toSignedInt());
					ibcpLit->addParent(decStack.top().getCurrentDTNode(), true);

					implPairs.clear();
				}
				// END BCPROPAGATIONNI
			}
		allFound += decStack.TOS_countImplLits() - impOfs;

	} while (decStack.TOS_countImplLits() - impOfs > 0);
	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////
// BEGIN module conflictAnalyzer
///////////////////////////////////////////////////////////////////////////////////////////////


void CMainSolver::caIncludeCauses(LiteralIdT theLit, bool viewedVars[])
{
	vector<LiteralIdT>::const_iterator it;

	ClauseIdT implCl;

	if (!getVar(theLit).getAntecedent().isAClause())
	{
		LiteralIdT hlit;
		hlit = getVar(theLit).getAntecedent().toLit();
		caIncorporateLit(hlit, viewedVars);
#ifdef DEBUG
		assert(hlit != NOT_A_LIT);
#endif
		return;
	}

	implCl = getVar(theLit).getAntecedent().toCl();

	if (implCl == NOT_A_CLAUSE)
	// theLit has not been implied, in fact this means that
	// either theLit is the decided variable, a unit clause or
	// a variable that has been tentatively assigned by ImplicitBCP
	// other cases may not occur !
	{
		caAddtoCauses(theLit, viewedVars);
		return;
	}

	for (it = begin(getClause(implCl)); *it != ClauseEnd(); it++)
	{
		caIncorporateLit(*it, viewedVars);
	}
}
///////////////////////////////////////////////////////////////////
//
//  BEGIN public
//
///////////////////////////////////////////////////////////////////

void CMainSolver::caAddtoCauses(LiteralIdT theLit, bool viewedVars[])
{
	viewedVars[theLit.toVarIdx()] = true;

	ca_lastUIPClause.push_back(theLit.oppositeLit());

	if (getVar(theLit).getDLOD() > imaxDecLev)
		imaxDecLev = getVar(theLit).getDLOD();
}

void CMainSolver::caIncorporateLit(const LiteralIdT &Lit, bool viewedVars[])
{
	if (Lit == NOT_A_LIT)
		return;

	if (!viewedVars[Lit.toVarIdx()])
	{
		viewedVars[Lit.toVarIdx()] = true;
		////
		getVar(Lit).scoreVSIDS[Lit.polarity()]++;
		getVar(Lit).scoreVSIDS[Lit.oppositeLit().polarity()]++;
		////
		if (getVar(Lit).getDLOD() == decStack.getDL())
		{
			theQueue.push_back(Lit.oppositeLit());
		}
		else
			caAddtoCauses(Lit.oppositeLit(), viewedVars);
	}
}

bool CMainSolver::caInit(vector<AntecedentT> & theConflicted, bool viewedVars[])
{
	vector<LiteralIdT>::const_iterator it;

	imaxDecLev = -1;

	ca_1UIPClause.clear();
	ca_lastUIPClause.clear();

	theQueue.clear();
	theQueue.reserve(countAllVars() + 1);

	ClauseIdT idConflCl;
	LiteralIdT idConflLit;

	if (theConflicted[0].isAClause())
	{
		idConflCl = theConflicted[0].toCl();

		for (it = begin(getClause(idConflCl)); *it != ClauseEnd(); it++)
		{
			caIncorporateLit(*it, viewedVars);
		}
	}
	else
	{
		if (theConflicted.size() < 2)
		{
			toSTDOUT("error in getcauses bincl" << endl);
			return false;
		}

		idConflLit = theConflicted[0].toLit();
		caIncorporateLit(idConflLit, viewedVars);
		idConflLit = theConflicted[1].toLit();
		caIncorporateLit(idConflLit, viewedVars);
	}

	return true;
}

bool CMainSolver::caGetCauses_lastUIP(vector<AntecedentT> & theConflicted)
{
	vector<LiteralIdT>::const_iterator it;
	bool viewedVars[countAllVars() + 1];

	memset(viewedVars, false, sizeof(bool) * (countAllVars() + 1));

	if (!caInit(theConflicted, viewedVars))
		return false;

	for (unsigned int i = 0; i < theQueue.size(); i++)
	{
		viewedVars[theQueue[i].toVarIdx()] = true;
		caIncludeCauses(theQueue[i], viewedVars);
	}

	// analyzer data
	theRunAn.addValue(CCL_lastUIP, decStack.getDL(), ca_lastUIPClause.size());
	return true;
}

bool CMainSolver::caGetCauses_firstUIP(vector<AntecedentT> & theConflicted)
{
	vector<LiteralIdT>::const_iterator it;
	bool viewedVars[countAllVars() + 1];

	memset(viewedVars, false, sizeof(bool) * (countAllVars() + 1));

	if (!caInit(theConflicted, viewedVars))
		return false;

	bool pastfirstUIP = false;

	for (unsigned int i = 0; i < theQueue.size(); i++)
	{

		if (i == theQueue.size() - 1 && pastfirstUIP == false)
		{
			pastfirstUIP = true;

			ca_1UIPClause = ca_lastUIPClause;

			ca_1UIPClause.push_back(theQueue.back().oppositeLit());
		}
		viewedVars[theQueue[i].toVarIdx()] = true;
		caIncludeCauses(theQueue[i], viewedVars);
	}
	// analyzer data
	theRunAn.addValue(CCL_1stUIP, decStack.getDL(), ca_1UIPClause.size());
	theRunAn.addValue(CCL_lastUIP, decStack.getDL(), ca_lastUIPClause.size());

	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////
// END module conflictAnalyzer
///////////////////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////
//
//  BEGIN Methods for Preprocessing
//
///////////////////////////////////////////////////////////////////////

bool CMainSolver::prep_IBCP(vector<AntAndLit> &impls)
{
	bool bSucceeded = false;

	getConflicted().clear();

	unsigned int sz = decStack.countAllImplLits();

	enable_DT_recording = false;
	bSucceeded = BCP(impls);
	enable_DT_recording = true;

	decStack.shrinkImplLitsTo(sz);

	vector<AntAndLit>::iterator it;

	for (it = impls.begin(); it != impls.end(); it++)
	{
		getVar(it->theLit).unsetVal();
	}

	impls.clear();
	return bSucceeded;
}

bool CMainSolver::prepFindHiddenBackBoneLits()
{
	DepositOfVars::iterator jt;
	vector<AntAndLit> implPairs;
	implPairs.reserve(countAllVars());

	int foundInLoop = 0;
	int allFound = 0;

	do
	{
		foundInLoop = 0;

		for (jt = beginOfVars(); jt != endOfVars(); jt++)
			if (jt->isActive() && jt->countBinLinks() > 0)
			{
				if (jt->countBinLinks(false) > 0)
					implPairs.push_back(AntAndLit(NOT_A_CLAUSE, jt->getLitIdT(
							true)));
				if ((jt->countBinLinks(false) > 0) && !prep_IBCP(implPairs))
				{
					foundInLoop++;
					if (!prepBCP(jt->getLitIdT(false)))
						return false;
				}
				else if (jt->countBinLinks(true) > 0)
				{
					implPairs.push_back(AntAndLit(NOT_A_CLAUSE, jt->getLitIdT(
							false)));
					if (!prep_IBCP(implPairs))
					{
						foundInLoop++;
						if (!prepBCP(jt->getLitIdT(true)))
							return false;
					}
				}
			}
		allFound += foundInLoop;

	} while (foundInLoop != 0);
	if (allFound != 0)
		toSTDOUT("found UCCL" << allFound << endl);
	return true;
}

bool CMainSolver::prepBCP(LiteralIdT firstLit)
{
	vector<LiteralIdT> impls;
	int step = 0;
	LiteralIdT unLit, satLit;

	impls.reserve(countAllVars());

	if (firstLit != NOT_A_LIT)
		impls.push_back(firstLit);

	// prepare UnitClauses for BCP
	if (!theUnitClauses.empty())
	{
		impls.insert(impls.end(), theUnitClauses.begin(), theUnitClauses.end());
		theUnitClauses.clear();
		theUClLookUp.clear();
	}

	vector<ClauseIdT>::iterator it;
	vector<ClauseIdT>::iterator itBegin;
	vector<LiteralIdT>::iterator lt;
	CVariableVertex *pV;

	for (unsigned int i = 0; i < impls.size(); i++)
		if (getVar(impls[i]).setVal(impls[i].polarity(), 0))
		{
			unLit = impls[i].oppositeLit();
			satLit = impls[i];
			pV = &getVar(satLit);

			backbones.push_back(satLit);

			// BEGIN propagation
			for (lt = pV->getBinLinks(unLit.polarity()).begin(); *lt
					!= SENTINEL_LIT; lt++)
			{
				if (getVar(*lt).isActive())
					impls.push_back(*lt);
				else if (getVar(*lt).getboolVal() != lt->polarity())
					return false;
			}
			CClauseVertex *pCl;

			itBegin = var_InClsStart(unLit.toVarIdx(), unLit.polarity());
			step = var_InClsStep(unLit.polarity());

			for (it = itBegin; *it != SENTINEL_CL; it += step)
				if (!getClause(*it).isDeleted())
				{
					pCl = &getClause(*it);
					eraseLiteralFromCl(*it, unLit);
					switch (getClause(*it).length())
					{
					case 0:
						return false;
					case 1:
						impls.push_back(*begin(*pCl));
						break;
					case 2:
						createBinCl((*begin(*pCl)), *(begin(*pCl) + 1));
						/// mark clause *it as deleted

						getVar(pCl->idLitB()).eraseWatchClause(*it,
								pCl->idLitB().polarity());
						getVar(pCl->idLitA()).eraseWatchClause(*it,
								pCl->idLitA().polarity());

						for (vector<LiteralIdT>::iterator litt = begin(*pCl); *litt
								!= ClauseEnd(); litt++)
						{
							if (*litt != unLit)
								var_EraseClsLink(litt->toVarIdx(),
										litt->polarity(), *it);
							*litt = NOT_A_LIT;
						}

						pCl->setDeleted();

						///
						break;
					default:
						break;
					};
				}
			//END propagation

			//BEGIN subsumption
			for (lt = pV->getBinLinks(satLit.polarity()).begin(); *lt
					!= SENTINEL_LIT; lt++)
				getVar(*lt).eraseBinLinkTo(satLit, lt->polarity());

			itBegin = var_InClsStart(satLit.toVarIdx(), satLit.polarity());
			step = var_InClsStep(satLit.polarity());

			for (it = itBegin; *it != SENTINEL_CL; it += step)

				if (!getClause(*it).isDeleted())
				{
					pCl = &getClause(*it);
					/// mark clause *it as deleted

					getVar(pCl->idLitB()).eraseWatchClause(*it,
							pCl->idLitB().polarity());
					getVar(pCl->idLitA()).eraseWatchClause(*it,
							pCl->idLitA().polarity());

					vector<LiteralIdT>::iterator litt;
					for (litt = begin(*pCl); *litt != ClauseEnd(); litt++)
					{
						if (*litt != satLit)
							// deleting ClauseEdges from satLit would be erroneous as it affects satPCl
							var_EraseClsLink(litt->toVarIdx(),
									litt->polarity(), *it);
						*litt = NOT_A_LIT;
					}

					pCl->setDeleted();
					///
				}
			// END subsumption

			for (it = var_InClsBegin(unLit.toVarIdx(), false); *it
					!= SENTINEL_CL; it++)
			{
				*it = SENTINEL_CL;
			}

			pV->eraseAllEdges();
		}
	return true;
}

////////////////////////////////////////////////////////////////////////
//
//  END Methods for Preprocessing
//
///////////////////////////////////////////////////////////////////////


bool CMainSolver::printUnitClauses()
{
	vector<LiteralIdT>::iterator it, jt;

	toSTDOUT("UCCL:\n");
	for (it = theUnitClauses.begin(); it != theUnitClauses.end(); it++)
	{
		toSTDOUT((it->polarity() ? " " : "-") << getVar(*it).getVarIdT()
				<< " 0\n");
	}
	toSTDOUT(endl);

	return true;
}
