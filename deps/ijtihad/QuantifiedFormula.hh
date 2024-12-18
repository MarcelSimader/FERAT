//
// Created by vedad on 17/07/17.
//

#ifndef QUANTIFIEDFORMULA_H
#define QUANTIFIEDFORMULA_H

#include <vector>
#include "qtypes.hh"

using std::vector;

/**
 * Internal representation of a QBF, where clauses are also spli into the existential and universal part, in order to
 * speed up the acces when fomrulas PHI and KSI are expanded.
 * @see MySolver::extendPHI()
 * @see MySolver::extendKSI()
 */
class QuantifiedFormula
{
private:
  Prefix pref;                  ///< the prefix of the QBF
  Prefix pref_inc;              ///< the cumulative prefixes of the QBF
  CNF cnf;                      ///< the cnf part of the QBF
  CNF phi_part;                 ///< existential parts of the cnf
  CNF ksi_part;                 ///< universal parts of the cnf
  unsigned long size;           ///< total number of literals in the CNF
  unsigned long size_phi;       ///< total number of existential literals in the CNF
  unsigned long size_ksi;       ///< total number of universal literals in the CNF
  vector<int> variable_depth;   ///< stores quantification depth for each variable index
  vector<int> phi_depths;       ///< stores maximum existential literal depth of each clause
  vector<int> ksi_depths;       ///< stores maximum universal literal depth of each clause
public:
  inline void addQuant(const Quantification& q) { pref.push_back(q); };
  inline void addCumulativeQuant(const Quantification& q) { pref_inc.push_back(q); };
  void addClause(vector<Lit>& phi_clause, vector<Lit>& ksi_clause, vector<Lit>& full_clause);
  inline unsigned long getSize() const { return size; };
  inline unsigned long getSizePHI() const { return size_phi; };
  inline unsigned long getSizeKSI() const { return size_ksi; };
  inline int getDepthPHI(unsigned position) const { return phi_depths[position]; };
  inline int getDepthKSI(unsigned position) const { return ksi_depths[position]; };
  inline const Quantification& getQuant(unsigned position) const { return pref[position]; };
  inline const Quantification& getCumulativeQuant(unsigned position) const { return pref_inc[position]; };
  inline unsigned getNumQuants() const { return pref.size(); };
  inline const vector<Lit>& getClause(unsigned clause_num) const { return cnf[clause_num]; };
  inline const vector<Lit>& getClausePHI(unsigned clause_num) const { return phi_part[clause_num]; };
  inline const vector<Lit>& getClauseKSI(unsigned clause_num) const { return ksi_part[clause_num]; };
  inline unsigned getNumClauses() const { return phi_part.size(); };
  inline void setVariableDepthSize(unsigned size) { variable_depth.resize(size); };
  inline int getVariableDepth(unsigned position) const { return variable_depth[position]; };
  inline void setVariableDepth(unsigned position, int depth) { variable_depth[position] = depth; };
  
  friend ostream& operator<<(ostream& outs, const QuantifiedFormula& f);
};

#endif //QUANTIFIEDFORMULA_H
