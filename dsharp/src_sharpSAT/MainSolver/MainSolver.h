#ifndef MAINSOLVER_H
#define MAINSOLVER_H

#include <set>
#include <queue>
#include <utility>

//shared files
#include <Interface/AnalyzerData.h>

#include "../Basics.h"
#include "InstanceGraph/InstanceGraph.h"

#include "FormulaCache.h"
#include "DecisionTree.h"

/** \addtogroup Interna Solver Interna
 * Dies sind alle Klassen, die ausschlie�ich vom Solver selbst verwendet werden
 * sollten.
 */

/*@{*/

typedef unsigned char viewStateT;
#define   NIL  0
#define   IN_SUP_COMP  1
#define   SEEN 2
#define   IN_OTHER_COMP  3

enum retStateT
{

	EXIT, RESOLVED, PROCESS_COMPONENT
};

enum retIBCPT
{
	NOTHING_FOUND, FOUND, CONTRADICTION,
};

class CMainSolver: public CInstanceGraph
{
	CDecisionStack decStack; // decision stack
	CStopWatch stopWatch;

	CFormulaCache xFormulaCache;
	vector<AntAndLit> bcpImplQueue;

	int lastTimeCClDeleted;
	int lastCClCleanUp;

	unsigned int remPoll;

	///////////////////
	// Decision Tree //
	///////////////////
	//DTNode * DT_current;
	DTNode * DT_original;
	int num_Nodes;
	bool enable_DT_recording;
	vector<DTNode *> litNodes;
	vector<pair<DTNode *, DTNode *> > dirtyLitNodes;

	DTNode * get_lit_node(int lit)
	{
		if (lit < 0)
			return litNodes[lit * -2];
		else
			return litNodes[lit * 2 + 1];
	}
	////-----------////

	vector<LiteralIdT> backbones;

	retStateT backTrack();

	// removes all cachePollutions that might be present in decedants of comnponents from the
	// present decision level
	void removeAllCachePollutions();

	bool findVSADSDecVar(LiteralIdT &theLit, const CComponentId & superComp);

	bool decide();

	bool bcp();

	void handleSolution()
	{
		int actCompVars = 0;
		static CRealNum rnCodedSols;

		// in fact the active component should only contain active vars
		if (decStack.TOS_countRemComps() != 0)
			actCompVars = decStack.TOS_NextComp().countVars();

		pow2(rnCodedSols, actCompVars);
		decStack.top().includeSol(rnCodedSols);
		theRunAn.addValue(SOLUTION, decStack.getDL());

		DTNode * newTop = new DTNode(DT_TOP, num_Nodes++);
		newTop->addParent(decStack.top().getCurrentDTNode());

	}

	retStateT resolveConflict();

	SOLVER_StateT countSAT();

	bool performPreProcessing();

	/**
	 *  passes a componentId to rComp that is made of all
	 *  active variables and clauses
	 *  !! used to initialize decStack at DL zero
	 **/
	void makeCompIdFromActGraph(CComponentId& rComp);

	// BEGIN component analysis
	vector<VarIdT> componentSearchStack;
	bool recordRemainingComps();
	bool getComp(const VarIdT &theVar, const CComponentId &superComp,
			viewStateT lookUpCls[], viewStateT lookUpVars[]);
	// END component analysis

	void printComponent(const CComponentId& rComp);

	inline bool assignVal(const LiteralIdT &rLitId, AntecedentT ant =
			AntecedentT(NOT_A_CLAUSE))
	{
		return getVar(rLitId).setVal(rLitId.polarity(), decStack.getDL(), ant);
	}

	bool BCP(vector<AntAndLit> &thePairsOfImpl);

	/////////////////////////////////////////////
	//  BEGIN conflict analysis
	/////////////////////////////////////////////

private:

	vector<LiteralIdT> theQueue;
	vector<LiteralIdT> ca_1UIPClause;
	vector<LiteralIdT> ca_lastUIPClause;

	int imaxDecLev;

	// includes the causes of a variable assignment via
	// breadth first search
	void caIncludeCauses(LiteralIdT theLit, bool viewedVars[]);
	void caIncorporateLit(const LiteralIdT &Lit, bool viewedVars[]);

	void caAddtoCauses(LiteralIdT theLit, bool viewedVars[]);
	// initializes all data structures for the analysis
	// especially the startin point of the analysis i plugged into the queue
	bool caInit(vector<AntecedentT> & theConflicted, bool viewedVars[]);

	bool caGetCauses_lastUIP(vector<AntecedentT> & theConflicted);
	bool caGetCauses_firstUIP(vector<AntecedentT> & theConflicted);

	// whenever a variable that is not implied causes a conflict
	// the conflict clause generated from this conflict implies a flip
	// of that variable:
	// creation of that clause and recording the implication is done by:

	const vector<LiteralIdT> &caGetPrev1UIPCCl()
	{
		return ca_1UIPClause;
	}
	const vector<LiteralIdT> &caGetPrevLastUIPCCl()
	{
		return ca_lastUIPClause;
	}

	/// ermittelt den maximalen Entscheidungslevel von
	/// Variablen die zu den Konfliktursachen geh�en.
	/// Ein Aufruf ist erst nach dem Auftruf von getCausesOf sinnvoll
	int getMaxDecLevFromAnalysis() const
	{
		return imaxDecLev;
	}

	/// gibt KonfliktKlausel zurck die durch die
	/// vorhergehende Analyse gewonnen wurde
	bool create1UIPCCl()
	{
		return createConflictClause(ca_1UIPClause);
	}

	bool createLastUIPCCl()
	{
		return createConflictClause(ca_lastUIPClause);
	}

	/////////////////////////////////////////////
	//  END conflict analysis
	/////////////////////////////////////////////

	/////////////////////////////////////////////
	//  BEGIN implicitBCP
	/////////////////////////////////////////////

	///  this method is used for heuristically testing variable assignments
	///  whether they incur a conflict. If so we can infer the opposite assignment of
	///  the variable in question
	///  Thus the method will then perform the assignment
	///  Furthermore it manages the necessary entries to the decision stack itself
	///  returned value: analogously to the bcp procedure: false iff a conflict is found
	bool implicitBCP();

	bool createAntClauseFor(const LiteralIdT &Lit);

	/////////////////////////////////////////////
	//  END implicitBCP
	/////////////////////////////////////////////

	/////////////////////////////////////////////
	// BEGIN PreProcessing methods
	////////////////////////////////////////////

	///NOTE: Preprocessing methods assume that no conflict clauses are present !

	bool prep_IBCP(vector<AntAndLit> &impls);
	/// Preprocessing
	/// i.e. the "hard" version of ImplicitBCP
	/// used for preprosessing ONLY
	bool prepFindHiddenBackBoneLits();

	/// Apply the BCP rule in a Preprocessing step:
	/// That is: if firstLit is a valid literal, propagation is started there
	/// firstLit should not be assigned already!
	/// furthermore ALL unit clauses are processed
	bool prepBCP(LiteralIdT firstLit = NOT_A_LIT);

	/////////////////////////////////////////////
	// END PreProcessing methods
	/////////////////////////////////////////////

	bool printUnitClauses();

public:

	// Variable to determine the mode we're running in
	bool compile_mode;

	// Variables for the nnf generation
	int bdg_edge_count;
	int bdg_var_count;

	// class constructor
	CMainSolver();

	// class destructor
	~CMainSolver();

	void solve(const char *lpstrFileName);

	void setTimeBound(long int i)
	{
		stopWatch.setTimeBound(i);
	}

	void writeBDG(const char *fileName)
	{
		set<int> nodesSeen;
		set<int> litsSeen;
		queue<DTNode *> openList;

		ofstream out(fileName);

		out << "digraph backdoorgraph {" << endl;

		// First add the root
		if (1 == decStack.top().getDTNode()->numChildren())
			openList.push(decStack.top().getDTNode()->onlyChild());
		else
			openList.push(decStack.top().getDTNode());

		int rootID = openList.front()->getID();

		DTNode *node;
		while (!openList.empty())
		{
			node = openList.front();
			openList.pop();

			int node_id = node->getID();

			if (DT_LIT == node->getType())
			{
				node_id = get_lit_node(node->getVal())->getID();
			}

			if (nodesSeen.find(node_id) == nodesSeen.end())
			{
				// Make sure we don't add this twice
				nodesSeen.insert(node_id);

				// Add the children to the open list
				set<DTNode *>::iterator it;
				for (it = node->getChildrenBegin(); it
						!= node->getChildrenEnd(); it++)
				{
					openList.push(*it);
				}

				// Print the node
				if (node->getID() == rootID)
				{
					out << "root";
				}
				else
				{
					out << node_id;
				}
				out << " [label=\"";
				switch (node->getType())
				{
				case DT_AND:
					out << "AND";
					break;
				case DT_OR:
					out << "OR";
					break;
				case DT_BOTTOM:
					out << "F";
					break;
				case DT_TOP:
					out << "T";
					break;
				case DT_LIT:
					out << node->getVal();
					break;
				default:
					break;
				}
				out << "\"];" << endl;

			}
		}

		// Now we insert all of the edges
		nodesSeen.clear();

		if (1 == decStack.top().getDTNode()->numChildren())
			openList.push(decStack.top().getDTNode()->onlyChild());
		else
			openList.push(decStack.top().getDTNode());

		while (!openList.empty())
		{
			node = openList.front();
			openList.pop();

			if (nodesSeen.find(node->getID()) == nodesSeen.end())
			{
				// Make sure we don't add this twice
				nodesSeen.insert(node->getID());

				// Add the children to the open list, and make the edges
				set<DTNode *>::iterator it;
				litsSeen.clear();

				for (it = node->getChildrenBegin(); it
						!= node->getChildrenEnd(); it++)
				{
					int node_id = (*it)->getID();

					if (DT_LIT == (*it)->getType())
					{
						node_id = get_lit_node((*it)->getVal())->getID();
					}

					if (litsSeen.find(node_id) == litsSeen.end())
					{

						litsSeen.insert(node_id);

						openList.push(*it);
						if (node->getID() == rootID)
						{
							out << "root -> " << node_id << ";" << endl;
						}
						else
						{
							out << node->getID() << " -> " << node_id << ";"
									<< endl;
						}
					}

				}
			}
		}

		out << "}" << endl;

	}

	void writeNNF(const char *fileName)
	{
		ofstream out(fileName);
		vector<DTNode*> *nodeList = new vector<DTNode*> ();

		DTNode* root;

		if (1 == decStack.top().getDTNode()->numChildren())
			root = decStack.top().getDTNode()->onlyChild();
		else
			root = decStack.top().getDTNode();

		root->prepNNF(nodeList);

		out << "nnf " << nodeList->size() << " " << bdg_edge_count << " "
				<< bdg_var_count << endl;

		for (int i = 0; i < nodeList->size(); i++)
		{
			nodeList->at(i)->genNNF(out);
		}
	}

	void print_translation(const vector<int> trans)
	{
		toSTDOUT("Translation:" << endl);
		for (int i = 0; i < trans.size(); ++i)
		{
			toSTDOUT(i << " -> " << trans[i] << endl);
		}
	}
};

/*@}*/
#endif // SOLVER_H
