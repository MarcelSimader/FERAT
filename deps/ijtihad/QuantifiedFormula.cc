//
// Created by vedad on 17/07/17.
//

#include "QuantifiedFormula.hh"
#include <algorithm>

/**
 * Introduces an order to the literals. A literal is smaller if it's variable index is smaller. If two literals have
 * the same variable, the negated one is smaller.
 * @param a first literal operand
 * @param b second literal operand
 * @return true if first operand is smaller than the second
 */
inline bool orderLits(Lit a, Lit b)
{ return (a != -b) ? (std::abs(a) < std::abs(b)) : (a < b); }

void QuantifiedFormula::addClause(vector<Lit>& phi_clause, vector<Lit>& ksi_clause, vector<Lit>& full_clause)
{
  std::sort(phi_clause.begin(), phi_clause.end(), orderLits);
  std::sort(ksi_clause.begin(), ksi_clause.end(), orderLits);
  int depth_phi = (phi_clause.empty()) ? -1 : variable_depth[var(phi_clause.back())];
  
  // do Kleine-Bunning forall-elimination
  if (phi_clause.size() != 0)
  {
    while (!ksi_clause.empty())
    {
      if (variable_depth[var(ksi_clause.back())] > depth_phi)
        ksi_clause.pop_back();
      else
        break;
    }
  }
  else
  {
    ksi_clause.clear();
  }
  
  int depth_ksi = (ksi_clause.empty()) ? -1 : variable_depth[var(ksi_clause.back())];
  
  full_clause.insert(full_clause.end(), phi_clause.begin(), phi_clause.end());
  full_clause.insert(full_clause.end(), ksi_clause.begin(), ksi_clause.end());
  std::sort(full_clause.begin(), full_clause.end());
  
  phi_part.push_back(phi_clause);
  phi_depths.push_back(depth_phi);
  size_phi += phi_clause.size();
  
  ksi_part.push_back(ksi_clause);
  ksi_depths.push_back(depth_ksi);
  size_ksi += ksi_clause.size();
  
  cnf.push_back(full_clause);
  size += full_clause.size();
}

ostream &operator<<(ostream &outs, const QuantifiedFormula &f)
{
  outs << "[" << f.pref << "]" << f.cnf;
  return outs;
}