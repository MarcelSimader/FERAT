//
// This file is part of the ijtihad QBF solver
// Copyright (c) 2016, Graz University of Technology
// Authored by Vedad Hadzic
//

#include "LitTuple.hh"
#include <algorithm>
#include "debug.hh"

using std::vector;
using std::sort;

LitTuple::LitTuple(vector<Var>* base) :
hash_(SEED),
variables_(base)
{
}

LitTuple::LitTuple(vector<Var>* base, vector<bool> s) :
hash_(SEED),
variables_(base),
signs_(s)
{
  assert(s.size() == variables_->size());
  calculate_hash();
}

LitTuple::LitTuple(const LitTuple& source) :
hash_(source.hash_),
variables_(source.variables_),
signs_(source.signs_)
{
}

LitTuple& LitTuple::operator= (const LitTuple& source)
{
  hash_ = source.hash_;
  variables_ = source.variables_;
  signs_.assign(source.signs_.begin(), source.signs_.end());
  
  return *this;
}

void LitTuple::calculate_hash()
{
  unsigned int n_hash = SEED; // avoid dereferencing
  assert(signs_.size() == variables_->size());
  for(unsigned i = 0; i < variables_->size(); i++)
  {
    const Var& v = (*variables_)[i];
    const bool s = signs_[i];
    // mapping -1 -> 2, 1 -> 3, -2 -> 4, 2 -> 5, ...
    n_hash ^= (unsigned)(2 * v + (s ? 0 : 1)) + 0x9e3779b9U + (n_hash << 6) + (n_hash >> 2);
  }
  hash_ = n_hash;
}

void LitTuple::clear()
{
  hash_ = SEED;
  variables_ = nullptr;
  signs_.clear();
}

void LitTuple::rebase(const std::vector<Var>* base)
{
  clear();
  variables_ = base;
}

void LitTuple::assign(const std::vector<bool>& source)
{
  assert(source.size() == variables_->size());
  signs_.assign(source.begin(), source.end());
  calculate_hash();
}

void LitTuple::unassign()
{
  signs_.clear();
  hash_ = SEED;
}

bool operator== (const LitTuple& first, const LitTuple& second)
{
  if(first.variables_->size() != second.variables_->size()) // same dimension of the base
    return false;
  if(first.signs_.size() != second.signs_.size()) // same number of assigned literals
    return false;
  if(first.variables_ != second.variables_)
    for(unsigned i = 0; i < first.variables_->size(); i++)
    {
      if(first.variables_->at(i) != second.variables_->at(i))
        return false;
    }
  for(unsigned i = 0; i < first.signs_.size(); i++)
  {
    if(first.signs_.at(i) != second.signs_.at(i))
      return false;
  }
  
  return true;
}

LitTupleScore::LitTupleScore(const LitTuple* l, unsigned s) :
    lit_tuple(l), score(s)
{}

bool operator<(const LitTupleScore& a, const LitTupleScore& b)
{
  return (a.score < b.score);
}
