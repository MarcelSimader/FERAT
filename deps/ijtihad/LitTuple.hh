//
// This file is part of the ijtihad QBF solver
// Copyright (c) 2016, Graz University of Technology
// Authored by Vedad Hadzic
//

#ifndef LITTUPLE_H
#define LITTUPLE_H

#include <vector>
#include <unordered_set>
#include <unordered_map>

#include "qtypes.hh"

using std::unordered_set;
using std::unordered_map;

class LitTuple
{
public:
  
  /**
   * Initialises the variable vector and sets the base to empty.
   */
  LitTuple(vector<Var>* base);

  /**
   * Initialises the members to empty and default values.
   */
  LitTuple(vector<Var>* base, vector<bool> s);
  
  /**
   * Creates a smart shallow copy of the given LitTuple.
   * @param source a LitTuple from which elements are copied
   */
  LitTuple(const LitTuple& source);
  
  /**
   * Creates a smart shallow copy of the given LitTuple.
   * @param source a LitTuple from which elements are copied
   * @see LitTuple(const LitTuple&)
   * @return self reference
   */
  LitTuple& operator= (const LitTuple& source);
  
  /**
   * Clears contents of the LitTuple and assigns the variable vector to given base.
   * @param source a vector of variables for initialising the data of the LitTuple
   */
  void rebase(const std::vector<Var>* base);
  
  /**
   * Clears contents of the LitTuple and assigns the internals to the given source vector.
   * @param source a vector of signs for the variable vector of the LitTuple
   */
  void assign(const std::vector<bool>& source);
  
  void unassign();
  
  /**
   * Gives Access tot the data vector.
   * @param index index of the element to be accessed.
   * @return a lit at position index in the data
   */
  inline const Lit operator[](const unsigned int index) const { return mkLit((*variables_)[index], signs_[index]); }
  
  /**
   * Checks whether two LitTuples are equal. Includes a few short-circuiting methods.
   * @param first  first LitTuple to be checked
   * @param second second LitTuple to be checked
   * @return true if the data matches, false otherwise
   */
  friend bool operator== (const LitTuple& first, const LitTuple& second);

  /**
   * @return size of the data vector.
   */
  inline unsigned long size() const { return variables_->size(); }
  
  /**
   * @return calculated hash of the data.
   */
  inline unsigned int getHash() const { return hash_; }

  /**
   * @return whether the lit tuple is properly assigned
   */
  inline bool isAssigned() const
  { return variables_ != nullptr && !variables_->empty() && signs_.size() == variables_->size(); }
  
  const static unsigned int SEED = 24111995; ///< initialisation seed given to each empty LitTuple
private:
  unsigned hash_;                        ///< Computed hash of the data.
  const std::vector<Var>* variables_;    ///< Pointer to a vector holding the data of the LitTuple
  std::vector<bool> signs_;              ///< Vector holding the signs for each variable when interpreting a literal
  
  /**
   * Calculates the hash of the LitTuple::literals_ and saves it in the LitTuple::hash_ member.
   */
  void calculate_hash();
  
  /**
   * Removes the reference to the currently referenced data. If no other LitTuple shares the data, the contents are
   * cleared and the LitTuple::hash_ is set to LitTuple::SEED.
   */
  void clear();
};

/**
 * Dummy class which is used as a function returning the hash of a given LitTuple, via a LitTuple::getHash() call.
 */
class LitTupleHash
{
public:
  inline unsigned int operator() (const LitTuple& lt) const {return lt.getHash();}
};

struct LitTupleScore
{
  /**
   * Simple assignment constructor for VarScore objects.
   * @param l a LitTuple
   * @param s score of the LitTuple
   */
  LitTupleScore(const LitTuple* l, unsigned s);
  
  const LitTuple* lit_tuple; ///< the LitTuple that is scored
  unsigned score;      ///< the score of the scored LitTuple
  
};

bool operator<(const LitTupleScore& a, const LitTupleScore& b);


/**
 * LitTuple2Lit
 * Hashmap which maps a LitTuple to a Lit. Used for MySolver::tseitin_cache_.
 * @see MySolver::tseitin_cache_
 */
typedef std::unordered_map<LitTuple, Lit, LitTupleHash> LitTuple2Lit;

/**
 * LitTuple2Uint
 * Hashmap which maps a LitTuple to a unsigned integer.
 */
typedef std::unordered_map<LitTuple, unsigned int, LitTupleHash> LitTuple2Uint;



/**
 * LitTupleSet
 * Hashset of LitTuple objects. Used for MySolver::counterexample_cache_ and MySolver::witness_cache_.
 * @see  MySolver::counterexample_cache_
 * @see  MySolver::witness_cache_
 */
typedef std::unordered_set<LitTuple, LitTupleHash> LitTupleSet;

/**
 * LitTuple2VariableVector
 * Hashmap from LitTuple objects to vector<Var> objects. Used for MySolver::vars_cache_phi_ and
 * MySolver::vars_cache_ksi_.
 * @see  MySolver::vars_cache_phi_
 * @see  MySolver::vars_cache_ksi_
 */
typedef std::unordered_map<LitTuple, std::vector<Var>, LitTupleHash> LitTuple2VariableVector;

/**
 * Int2LitTuple
 * Hashmap from integers to LitTuple objects. Used for MySolver::wit_to_add_ and
 * MySolver::cex_to_add_.
 * @see  MySolver::wit_to_add_
 * @see  MySolver::cex_to_add_
 */
typedef std::map<unsigned, LitTuple> Int2LitTuple;


#endif //LITTUPLE_H
