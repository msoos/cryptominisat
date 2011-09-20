/*
 *      DecisionTree.h
 *
 *      Copyright 2008 Christian Muise <christian.muise@gmail.com>
 *
 *      This program is free software; you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published by
 *      the Free Software Foundation; either version 2 of the License, or
 *      (at your option) any later version.
 *
 *      This program is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *      GNU General Public License for more details.
 *
 *      You should have received a copy of the GNU General Public License
 *      along with this program; if not, write to the Free Software
 *      Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 *      MA 02110-1301, USA.
 */

#ifndef DECISIONTREE_H
#define DECISIONTREE_H

#include <set>
#include <vector>
#include <iostream>

#include <SomeTime.h>
#include "InstanceGraph/InstanceGraph.h"
#include "InstanceGraph/ComponentTypes.h"

#include "../Basics.h"
using namespace std;

class DTNode
{
	DT_NodeType type;
	set<DTNode *> children;
	set<DTNode *> parents;

	int val;
	int id;
	int uncheckID;

public:

	int choiceVar;
	int nnfID;

	bool checked;

	DTNode * firstNode; // These are used to keep track of the children
	DTNode * secondNode;//  in the case of OR nodes (decision points).

	// Constructor mainly for leaf nodes
	DTNode(int literal, bool lit_constructor, int CURRENT_ID) :
		type(DT_LIT), val(literal), firstNode(NULL), secondNode(NULL)
	{
		id = CURRENT_ID;
		nnfID = -1;
		CSolverConf::nodeCount++;
		uncheckID = 0;
	}

	DTNode(DT_NodeType newType, int CURRENT_ID) :
		type(newType), firstNode(NULL), secondNode(NULL)
	{
		id = CURRENT_ID;
		nnfID = -1;
		CSolverConf::nodeCount++;
		uncheckID = 0;
	}

	// Default destructor
	~DTNode()
	{
		for (set<DTNode *>::iterator it = parents.begin(); it != parents.end(); it++)
		{
			(*it)->childDeleted(this);
		}

		// We delete the children recursively
		//  if they don't have another parent.
		//  Obviously, if there are cycles in
		//  the decision tree then bad things
		//  will happen.
		for (set<DTNode *>::iterator it = children.begin(); it
				!= children.end(); it++)
		{
			(*it)->parentDeleted(this);

			// Only delete the child if this was the only parent.
			if (0 == (*it)->numParents())
			{
				// Protect against deleting a literal
				if (DT_LIT != (*it)->getType())
					delete (*it);
			}
		}

		children.clear();

		CSolverConf::nodeCount--;
	}

	set<DTNode *>::iterator getChildrenBegin();
	set<DTNode *>::iterator getChildrenEnd();

	DTNode * onlyChild();

	DT_NodeType getType();
	int getID();
	int getVal();

	bool isBottom();
	bool isTop();
	bool isValid();

	void topIfy();

	// Parent methods
	void addParent(DTNode * newParent, bool link = false);
	bool parentDeleted(DTNode *oldParent);
	int numParents();

	// Add a child
	void addChild(DTNode * newChild, bool link = false);
	bool childDeleted(DTNode * oldChild);
	int numChildren();
	bool hasChild(DTNode * child);

	void compressNode();
	int count(bool isRoot);
	void uncheck(int unID);

	void reset();
	bool validate();
	void translateLiterals(const vector<int> varTranslation);

	void prepNNF(vector<DTNode*> * nodeList);
	void genNNF(ostream & out);

	// Printout
	void print(int depth = -1);

	// Sanity checks
	bool checkCycle(int sourceID, bool first = true);
};

#endif
