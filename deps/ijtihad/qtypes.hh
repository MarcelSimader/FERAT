/* 
 * File:   qtypes.hh
 * Author: mikolas
 *
 * Created on January 12, 2011, 5:32 PM
 */

/** @file */

#ifndef QTYPES_HH
#define  QTYPES_HH

#include <vector>
#include <map>
#include <iostream>
#include <assert.h>

using std::vector;
using std::ostream;

typedef unsigned Var;
typedef signed Lit;

/**
 * Type of a formula used in the QBF solver
 */
enum FormulaType
{
  PHI, KSI
};

/**
 * Type of a Quantification
 */
enum QuantifierType
{
  UNIVERSAL, EXISTENTIAL
};

/**
 * Structure representing a quantified vector of boolean variables, represented by their ids.
 */
struct Quantification
{
  /**
   * Simple assignment constructor for WorkerArgument objects.
   * @param q QuantifierType of the quantified bitvector
   * @param vars variables which are quantified in this Quantification
   */
  Quantification(const QuantifierType q, const vector<Var>& vars);
  /**
   * Default Constructor
   */
  Quantification();
  
  QuantifierType first; ///< type of the quantified bitvector
  vector<Var> second;   ///< variables which are quantified
};

/**
 * Vector of Quantification objects.
 */
typedef vector<Quantification> Prefix;

/**
 * Matrix of Lit objects.
 */
typedef vector<vector<Lit>> CNF;

/**
 * Structure used for ranking importance of variables. Used in MySolver::reorderQuantifiersInternally()
 */
struct VarScore
{
  /**
   * Simple assignment constructor for VarScore objects.
   * @param v a variable
   * @param s score of the variable
   */
  VarScore(Var v, unsigned s);
  
  Var v;          ///< the variable that is scored
  unsigned score; ///< the score of the scored variable
};

/**
 * Less-Than operator for VarScore objects
 * @param a first VarScore operand
 * @param b second VarScore operand
 * @return true if variable in a has a lower score than the variable in b, false otherwise
 */
bool operator<(const VarScore& a, const VarScore& b);

/**
 * Structure representing a dependency relation between two vectors of variables. In this relation bottom is dependent
 * upon top. Dependency in this case means that a value to one variable cannot be assigned before a value fo the other
 * is known. A variable can only be dependant upon variables taht come before it in the prefix of the CNF.
 */
struct DependencyPair
{
  /**
   * Simple assignment constructor for DependencyPair objects.
   * @param t vector of upstream variables
   * @param b vector of downsream variables
   */
  DependencyPair(const vector<Var>& t, const vector<Var>& b);
  
  vector<Var> top;    ///< variables in top are depended upon by variables in bottom
  vector<Var> bottom; ///< variables in bottom depend on variables in top
};

/**
 * Equivalence operator for Var vectors.
 * @param a fisrt operand
 * @param b second operand
 * @return true if all elements are equivalent
 */
bool operator==(const vector<Var>& a, const vector<Var>& b);

class Occurrence
{
public:
  Occurrence();
  Lit get(Var v);
  void set(Lit l);
  void clear();
private:
  vector<bool> lit_occurs;
};

/**
 * Retrieves the variable associated with the given literal.
 * @param lit literal of the desired variable
 * @return variable of the literal
 */
inline Var var(Lit lit)
{ return (lit < 0) ? (Var) (-lit) : (Var) lit; }

/**
 * Retrieves the sign of the given literal.
 * @param lit literal whose sign is desired
 * @return true if the variable was negated, false otherwise
 */
inline bool sign(Lit lit)
{ return (lit < 0); }

/**
 * Creates a literal from a variable and a sign
 * @param v a variable
 * @param sign whether the variable is to be negated
 * @return negated variable if sign is true, simple variable if sign is false
 */
inline Lit mkLit(Var v, bool sign = false)
{ return ((sign) ? -1 : 1) * (Lit) v; }

/**
 * Changes the sign of the input literal.
 * @param lit a literal
 * @return negated literal
 */
inline Lit negateLit(Lit lit)
{ return -lit; }

ostream& operator<<(ostream& outs, QuantifierType q);

ostream& operator<<(ostream& outs, const CNF& f);

ostream& operator<<(ostream& outs, const Prefix& ls);

ostream& operator<<(ostream& outs, const vector<Var>& vs);

ostream& operator<<(ostream& outs, const vector<Lit>& ls);

#endif	/* QTYPES_HH */

