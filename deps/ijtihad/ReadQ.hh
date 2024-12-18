/* 
 * File:   Read2Q.hh
 * Author: mikolas
 *
 * Created on Tue Jan 11 15:08:14
 */
#ifndef READ2Q_HH
#define  READ2Q_HH

#include <zlib.h>
#include <stdio.h>
#include "qtypes.hh"
#include "Reader.hh"
#include "ReadException.hh"

class ReadQ
{
public:
  ReadQ(Reader& r, bool qube_input = false);
  
  ~ReadQ();
  
  void read();
  
  Var get_max_id() const;
  
  vector<Quantification>& get_prefix();
  
  const vector<vector<Lit>>& get_clauses() const;
  
  bool get_header_read() const;
  
  int get_qube_output() const;
  
  typedef struct VarSet_
  {
  public:
    inline void add(Var v)
    {
      if (v <= 0)
        return;
      if ((unsigned) v >= inside.size())
        inside.resize((unsigned) v + 1, false);
      inside[v] = true;
    }
    
    inline bool get(Var v)
    {
      return ((unsigned) v < inside.size() && inside[v]);
    }
    
    void convert(vector<Var>& res)
    {
      res.clear();
      for (unsigned i = 0; i < inside.size(); i++)
      {
        if (inside[i])
          res.push_back((Var) i);
      }
    }
  
  private:
    vector<bool> inside;
  } VarSet;
private:
  Reader& r;
  bool qube_input;
  int qube_output;
  Var max_id;
  bool _header_read;
  vector<vector<Lit>> clause_vector;
  vector<Quantification> quantifications;
  VarSet quantified_variables;
  VarSet unquantified_variables;
  
  void read_header();
  
  void read_quantifiers();
  
  void read_clauses();
  
  void read_cnf_clause(Reader& in, vector<Lit>& lits);
  
  void read_quantification(Reader& in, Quantification& quantification);
  
  Var parse_variable(Reader& in);
  
  int parse_lit(Reader& in);
  
  void read_word(const char* word, size_t length);
};

#endif	/* READ2Q_HH */

