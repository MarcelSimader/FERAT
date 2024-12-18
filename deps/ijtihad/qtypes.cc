#include "qtypes.hh"

VarScore::VarScore(Var vi, unsigned si) :
    v(vi), score(si)
{}

bool operator<(const VarScore& a, const VarScore& b)
{
  return (a.score < b.score);
}

DependencyPair::DependencyPair(const vector<Var>& t, const vector<Var>& b) :
    top(t), bottom(b)
{}

bool operator==(const vector<Var>& a, const vector<Var>& b)
{
  if (a.size() != b.size())
    return false;
  for (unsigned i = 0; i < a.size(); i++)
  {
    if (a[i] != b[i])
      return false;
  }
  
  return true;
}

Occurrence::Occurrence()
{ }

Lit Occurrence::get(Var v)
{
  unsigned v1 = (v << 1);
  if( lit_occurs.empty() || v1 > lit_occurs.size() - 1)
    return 0;
  if(lit_occurs[v1] && !lit_occurs[v1 - 1])
    return v;
  if(!lit_occurs[v1] && lit_occurs[v1 - 1])
    return -v;
  return 0;
}

void Occurrence::set(Lit l)
{
  if((var(l) << 1) > lit_occurs.size())
    lit_occurs.resize((var(l) << 1) + 1, false);
  lit_occurs[(var(l) << 1) - sign(l)] = true;
}

void Occurrence::clear()
{
  lit_occurs.clear();
}

Quantification::Quantification(const QuantifierType q, const vector<Var>& vars) :
    first(q), second(vars)
{ }

Quantification::Quantification() :
    first(EXISTENTIAL)
{}

ostream& operator<<(ostream& outs, const CNF& f)
{
  outs << '[';
  for (const vector<Lit>& c : f)
  {
    outs << "[" << c << " ]";
  }
  outs << "]";
  return outs;
}

ostream& operator<<(ostream& outs, QuantifierType q)
{
  switch (q)
  {
    case UNIVERSAL:
      return outs << "A";
      break;
    case EXISTENTIAL:
      return outs << "E";
      break;
    default:
      assert(0);
      return outs << "?";
  }
}

ostream& operator<<(ostream& outs, const Prefix& p)
{
  for (const Quantification& qi : p)
  {
    outs << '[' << (qi.first == EXISTENTIAL ? 'e' : 'a');
    outs << " ";
    outs << qi.second;
    outs << ']' << std::endl;
  }
  return outs;
}

ostream& operator<<(ostream& outs, const vector<Var>& vs)
{
  for (const Var& v: vs) outs << " " << v;
  return outs;
}

ostream& operator<<(ostream& outs, const vector<Lit>& ls)
{
  for (const Lit& l: ls) outs << " " << l;
  return outs;
}