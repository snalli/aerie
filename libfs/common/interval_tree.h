#ifndef __INTERVAL_TREE_H_JKA901
#define __INTERVAL_TREE_H_JKA901


#include <math.h>
#include <limits.h>
#include <iostream>
#include "common/TemplateStack.H"
#include "common/debug.h"

//  The interval_tree.h and interval_tree.cc files contain code for 
//  interval trees implemented using red-black-trees as described in 
//  the book _Introduction_To_Algorithms_ by Cormen, Leisserson, 
//  and Rivest.  

//  CONVENTIONS:  
//                Function names: Each word in a function name begins with 
//                a capital letter.  An example funcntion name is  
//                CreateRedTree(a,b,c). Furthermore, each function name 
//                should begin with a capital letter to easily distinguish 
//                them from variables. 
//                                                                     
//                Variable names: Each word in a variable name begins with 
//                a capital letter EXCEPT the first letter of the variable 
//                name.  For example, int newLongInt.  Global variables have 
//                names beginning with "g".  An example of a global 
//                variable name is gNewtonsConstant. 


#ifndef MAX_INT
#define MAX_INT INT_MAX // some architectures define INT_MAX not MAX_INT
#endif

// The Interval class is an Abstract Base Class.  This means that no
// instance of the Interval class can exist.  Only classes which
// inherit from the Interval class can exist.  Furthermore any class
// which inherits from the Interval class must define the member
// functions GetLowPoint and GetHighPoint.
//
// The GetLowPoint should return the lowest point of the interval and
// the GetHighPoint should return the highest point of the interval.  

class Interval {
public:
	Interval();
	virtual ~Interval();
	virtual int GetLowPoint() const = 0;
	virtual int GetHighPoint() const = 0;
	virtual void Print() const;
};

class IntervalTreeNode {
	friend class IntervalTree;
public:
	void Print(IntervalTreeNode*, IntervalTreeNode*) const;
	IntervalTreeNode();
	IntervalTreeNode(Interval *);
	~IntervalTreeNode();
protected:
	Interval*         storedInterval;
	int               key;
	int               high;
	int               maxHigh;
	int               red; // if red=0 then the node is black
	IntervalTreeNode* left;
	IntervalTreeNode* right;
	IntervalTreeNode* parent;
};


static inline int Overlap(int a1, int a2, int b1, int b2) {
  if (a1 <= b1) {
    return( (b1 <= a2) );
  } else {
    return( (a1 <= b2) );
  }
}


class IntervalTree {
public:
	IntervalTree();
	~IntervalTree();
	void Print() const;
	Interval * DeleteNode(IntervalTreeNode *);
	IntervalTreeNode * Insert(Interval *);
	IntervalTreeNode * GetPredecessorOf(IntervalTreeNode *) const;
	IntervalTreeNode * GetSuccessorOf(IntervalTreeNode *) const;
	void CheckAssumptions() const;
	Interval* LeftmostOverlap(int low, int high);


protected:
	/*  A sentinel is used for root and for nil.  These sentinels are */
	/*  created when ITTreeCreate is caled.  root->left should always */
	/*  point to the node which is the root of the tree.  nil points to a */
	/*  node which should always be black but has aribtrary children and */
	/*  parent and no key or info.  The point of using these sentinels is so */
	/*  that the root and nil nodes do not require special cases in the code */
	IntervalTreeNode* root;
	IntervalTreeNode* nil;

	void LeftRotate(IntervalTreeNode *);
	void RightRotate(IntervalTreeNode *);
	void TreeInsertHelp(IntervalTreeNode *);
	void TreePrintHelper(IntervalTreeNode *) const;
	void FixUpMaxHigh(IntervalTreeNode *);
	void DeleteFixUp(IntervalTreeNode *);
	void CheckMaxHighFields(IntervalTreeNode *) const;
	int CheckMaxHighFieldsHelper(IntervalTreeNode * y, 
	                             const int currentHigh,
	                             int match) const;
};


inline Interval* IntervalTree::LeftmostOverlap(int low, int high)
{
	IntervalTreeNode* x=root->left;
	Interval*         result_interval = NULL;
  
#ifdef DEBUG_ASSERT
	Assert((recursionNodeStackTop == 1),
	       "recursionStack not empty when entering IntervalTree::Enumerate");
#endif
	while (x != nil) {
		if (Overlap(low,high,x->key,x->high) ) {
			result_interval = x->storedInterval;
		}
		if(x->left->maxHigh >= low) {
			x = x->left;
		} else if (!result_interval) { // we are looking for the leftmost 
		    x = x->right;              // overlap so if we already have one 
		                               // we should not go right
		} else {
			break;
		}
	}
	return result_interval;
}


#endif // __INTERVAL_TREE_H_JKA901
