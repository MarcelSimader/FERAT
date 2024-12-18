/***************************************************************************************[Solver.cc]
abcdSAT -- Copyright (c) 2016, Jingchao Chen, Donghua University   

abcdSAT sources are obtained by modifying Glucose (see below Glucose copyrights). Permissions and copyrights of
abcdSAT are exactly the same as Glucose.

----------------------------------------------------------

 Glucose -- Copyright (c) 2009, Gilles Audemard, Laurent Simon
				CRIL - Univ. Artois, France
				LRI  - Univ. Paris Sud, France
 
Glucose sources are based on MiniSat (see below MiniSat copyrights). Permissions and copyrights of
Glucose are exactly the same as Minisat on which it is based on. (see below).

---------------

Copyright (c) 2003-2006, Niklas Een, Niklas Sorensson
Copyright (c) 2007-2010, Niklas Sorensson

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
associated documentation files (the "Software"), to deal in the Software without restriction,
including without limitation the rights to use, copy, modify, merge, publish, distribute,
sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or
substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT
OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
**************************************************************************************************/

#include <math.h>

#include "abcdsat/mtl/Sort.h"
#include "abcdsat/core/Solver.h"
#include "abcdsat/core/Constants.h"
#include "abcdsat/utils/System.h"

#define LOWER_BOUND_FOR_BLOCKING_RESTART 10000
#define  SELECT_WIDTH  9    

int mod_k=4;

unsigned int all_conflicts=0;
unsigned int unsatCnt=0;
unsigned int unsat9Cnt=0;
int priorBegin[150];
int outvars=0;
  
int cancel_subsolve=0;
extern int size30;

extern int **Bclauses;
extern int nBlocked2;
extern int BCDfalses;
extern int nClausesB;
int  *varRange=0;
float *size_diff=0;

using namespace abcdSAT;

//static bool promoted2 = false;
int Maxlbd=0;
//=================================================================================================
// Options:

static const char* _cat = "CORE";
static const char* _cr = "CORE -- RESTART";
static const char* _cred = "CORE -- REDUCE";
static const char* _cm = "CORE -- MINIMIZE";

static DoubleOption opt_K                 (_cr, "K",           "The constant used to force restart",            0.8,     DoubleRange(0, false, 1, false));           
static DoubleOption opt_R                 (_cr, "R",           "The constant used to block restart",            1.4,     DoubleRange(1, false, 5, false));           
static IntOption     opt_size_lbd_queue     (_cr, "szLBDQueue",      "The size of moving average for LBD (restarts)", 50, IntRange(10, INT32_MAX)); //50 new idea
static IntOption     opt_size_trail_queue     (_cr, "szTrailQueue",      "The size of moving average for trail (block restarts)", 5000, IntRange(10, INT32_MAX));//5000, new idea

static IntOption     opt_first_reduce_db     (_cred, "firstReduceDB",      "The number of conflicts before the first reduce DB", 2000, IntRange(0, INT32_MAX));
static IntOption     opt_inc_reduce_db     (_cred, "incReduceDB",      "Increment for reduce DB", 300, IntRange(0, INT32_MAX));
static IntOption     opt_spec_inc_reduce_db     (_cred, "specialIncReduceDB",      "Special increment for reduce DB", 1000, IntRange(0, INT32_MAX));
static IntOption    opt_lb_lbd_frozen_clause      (_cred, "minLBDFrozenClause",        "Protect clauses if their LBD decrease and is lower than (for one turn)", 30, IntRange(0, INT32_MAX));

static IntOption     opt_lb_size_minimzing_clause     (_cm, "minSizeMinimizingClause",      "The min size required to minimize clause", 30, IntRange(3, INT32_MAX));
static IntOption     opt_lb_lbd_minimzing_clause     (_cm, "minLBDMinimizingClause",      "The min LBD required to minimize clause", 6, IntRange(3, INT32_MAX));


static DoubleOption  opt_var_decay         (_cat, "var-decay",   "The variable activity decay factor",            0.8,    DoubleRange(0, false, 1, false));
static DoubleOption  opt_clause_decay      (_cat, "cla-decay",   "The clause activity decay factor",              0.999,    DoubleRange(0, false, 1, false));
static DoubleOption  opt_random_var_freq   (_cat, "rnd-freq",    "The frequency with which the decision heuristic tries to choose a random variable", 0, DoubleRange(0, true, 1, true));
static DoubleOption  opt_random_seed       (_cat, "rnd-seed",    "Used by the random variable selection",         91648253, DoubleRange(0, false, HUGE_VAL, false));
static IntOption     opt_ccmin_mode        (_cat, "ccmin-mode",  "Controls conflict clause minimization (0=none, 1=basic, 2=deep)", 2, IntRange(0, 2));
static IntOption     opt_phase_saving      (_cat, "phase-saving", "Controls the level of phase saving (0=none, 1=limited, 2=full)", 2, IntRange(0, 2));
static BoolOption    opt_rnd_init_act      (_cat, "rnd-init",    "Randomize the initial activity", false);
static DoubleOption  opt_garbage_frac      (_cat, "gc-frac",     "The fraction of wasted memory allowed before a garbage collection is triggered",  0.20, DoubleRange(0, false, HUGE_VAL, false));
static IntOption     opt_restart_first     (_cat, "rfirst",      "The base restart interval", 100, IntRange(1, INT32_MAX));
static DoubleOption  opt_restart_inc       (_cat, "rinc",        "Restart interval increase factor", 2, DoubleRange(1, false, HUGE_VAL, false));


//=================================================================================================
// Constructor/Destructor:


Solver::Solver() :

    // Parameters (user settable):
    //
    verbosity        (0)
    , K              (opt_K)
    , R              (opt_R)
    , sizeLBDQueue   (opt_size_lbd_queue)
    , sizeTrailQueue   (opt_size_trail_queue)
    , firstReduceDB  (opt_first_reduce_db)
    , incReduceDB    (opt_inc_reduce_db)
    , specialIncReduceDB    (opt_spec_inc_reduce_db)
    , lbLBDFrozenClause (opt_lb_lbd_frozen_clause)
    , lbSizeMinimizingClause (opt_lb_size_minimzing_clause)
    , lbLBDMinimizingClause (opt_lb_lbd_minimzing_clause)
  , var_decay        (opt_var_decay)
  , clause_decay     (opt_clause_decay)
  , random_var_freq  (opt_random_var_freq)
  , random_seed      (opt_random_seed)
  , ccmin_mode       (opt_ccmin_mode)
  , phase_saving     (opt_phase_saving)
  , rnd_pol          (false)
  , rnd_init_act     (opt_rnd_init_act)
  , garbage_frac     (opt_garbage_frac)
  , restart_first    (opt_restart_first)
  , restart_inc      (opt_restart_inc)


    // Statistics: (formerly in 'SolverStats')
    //
  ,  nbRemovedClauses(0),nbReducedClauses(0), nbDL2(0),nbBin(0),nbUn(0) , nbReduceDB(0)
    , solves(0), starts(0), decisions(0), rnd_decisions(0), propagations(0), conflicts(0),nbstopsrestarts(0),nbstopsrestartssame(0),lastblockatrestart(0)
  , dec_vars(0), clauses_literals(0), learnts_literals(0), max_literals(0), tot_literals(0)
    //, curRestart(1)

  , ok                 (true)
  , cla_inc            (1)
  , var_inc            (1)
  , watches            (WatcherDeleted(ca))
  , watchesBin            (WatcherDeleted(ca))
  , qhead              (0)
  , simpDB_assigns     (-1)
  , simpDB_props       (0)
  , order_heap         (VarOrderLt(activity))
  , progress_estimate  (0)
  , remove_satisfied   (true)

    // Resource constraints:
    //
  , conflict_budget    (-1)
  , propagation_budget (-1)
  , asynch_interrupt   (false)
{  MYFLAG=0; 
   output=NULL;

   MYFLAG=0;
   g_pps=(struct PPS *) calloc (1, sizeof (PPS));
   recursiveDepth=0;
   conflict_limit=-1;
   completesolve=0;
   originVars=0;
   equhead=equlink=0;
   hbrsum=equsum=unitsum=probeSum=0;
   varRange=0;
   longcls=probe_unsat=0;
   promoted2 = false;
   lbd_cut=2;
   luby_restart=0;
   DBpre_conf=0;

   termCallback=0;
   incremental=0;
}

Solver::~Solver()
{
   additional_del();
   ClearClause(clauses);
   ClearClause(corelearnts);
   if(probe_unsat==0) garbageCollect();
 
   if(g_pps->unit) free(g_pps->unit);
   if(equhead) free(equhead);
   if(equlink) free(equlink);
   equhead = equlink=0;
   delete g_pps;
   g_pps=0;
}

//=================================================================================================
// Minor methods:


// Creates a new SAT variable in the solver. If 'decision' is cleared, variable will not be
// used as a decision variable (NOTE! This has effects on the meaning of a SATISFIABLE result).
//
Var Solver::newVar(bool sign, bool dvar)
{
    int v = nVars();
    watches  .init(mkLit(v, false));
    watches  .init(mkLit(v, true ));
    watchesBin  .init(mkLit(v, false));
    watchesBin  .init(mkLit(v, true ));
    assigns  .push(l_Undef);
    vardata  .push(mkVarData(CRef_Undef, 0));
    activity .push(0);
   // activity .push(rnd_init_act ? drand(random_seed) * 0.00001 : 0);
    seen     .push(0);
    permDiff  .push(0);
    polarity .push(sign);
    decision .push();
    trail    .capacity(v+1);
    setDecisionVar(v, dvar);
    return v;
}


bool Solver::addClause_(vec<Lit>& ps)
{
    assert(decisionLevel() == 0);
    if (!ok) return false;

    // Check if clause is satisfied and remove false/duplicate literals:
    sort(ps);

    vec<Lit>    oc;
    oc.clear();

    Lit p; int i, j;
    //flag = 0;
    for (i = j = 0, p = lit_Undef; i < ps.size(); i++) {
        oc.push(ps[i]);
      //  if (value(ps[i]) == l_True || ps[i] == ~p || value(ps[i]) == l_False) flag = 1;
    }

    for (i = j = 0, p = lit_Undef; i < ps.size(); i++)
        if (value(ps[i]) == l_True || ps[i] == ~p)
            return true;
        else if (value(ps[i]) != l_False && ps[i] != p)
            ps[j++] = p = ps[i];
    ps.shrink(i - j);

    if (ps.size() == 0)
        return ok = false;
    else if (ps.size() == 1){
        uncheckedEnqueue(ps[0]);
        return ok = (propagate() == CRef_Undef);
    }else{
        CRef cr = ca.alloc(ps, false);
        clauses.push(cr);
        attachClause(cr);
    }
    return true;
}


void Solver::attachClause(CRef cr) {
    const Clause& c = ca[cr];
    assert(c.size() > 1);
    if(c.size()==2) {
      watchesBin[~c[0]].push(Watcher(cr, c[1]));
      watchesBin[~c[1]].push(Watcher(cr, c[0]));
    } else {
      watches[~c[0]].push(Watcher(cr, c[1]));
      watches[~c[1]].push(Watcher(cr, c[0]));
    }
    if (c.learnt()) learnts_literals += c.size();
    else            clauses_literals += c.size(); }




void Solver::detachClause(CRef cr, bool strict) {
    const Clause& c = ca[cr];
    
    assert(c.size() > 1);
    if(c.size()==2) {
      if (strict){
        remove(watchesBin[~c[0]], Watcher(cr, c[1]));
        remove(watchesBin[~c[1]], Watcher(cr, c[0]));
      }else{
        // Lazy detaching: (NOTE! Must clean all watcher lists before garbage collecting this clause)
        watchesBin.smudge(~c[0]);
        watchesBin.smudge(~c[1]);
      }
    } else {
      if (strict){
        remove(watches[~c[0]], Watcher(cr, c[1]));
        remove(watches[~c[1]], Watcher(cr, c[0]));
      }else{
        // Lazy detaching: (NOTE! Must clean all watcher lists before garbage collecting this clause)
        watches.smudge(~c[0]);
        watches.smudge(~c[1]);
      }
    }
    if (c.learnt()) learnts_literals -= c.size();
    else            clauses_literals -= c.size(); }


void Solver::removeClause(CRef cr) {
  
  Clause& c = ca[cr];

  if (output != NULL) {
    fprintf(output, "d ");
    for (int i = 0; i < c.size(); i++)
      fprintf(output, "%i ", (var(c[i]) + 1) * (-2 * sign(c[i]) + 1));
    fprintf(output, "0\n");
  }

  detachClause(cr);
  // Don't leave pointers to free'd memory!
  if (locked(c)) vardata[var(c[0])].reason = CRef_Undef;
  c.mark(1); 
  ca.free(cr);
}


bool Solver::satisfied(const Clause& c) const {
    for (int i = 0; i < c.size(); i++)
        if (value(c[i]) == l_True)
            return true;
    return false; }


// Revert to the state at given level (keeping all assignment at 'level' but not beyond).
//
void Solver::cancelUntil(int level) {
    if (decisionLevel() > level){
        for (int c = trail.size()-1; c >= trail_lim[level]; c--){
            Var      x  = var(trail[c]);
            assigns [x] = l_Undef;
            if (phase_saving > 1 || (phase_saving == 1) && c > trail_lim.last())
                polarity[x] = sign(trail[c]);
            insertVarOrder(x); }
        qhead = trail_lim[level];
        trail.shrink(trail.size() - trail_lim[level]);
        trail_lim.shrink(trail_lim.size() - level);
    } }


//=================================================================================================
// Major methods:

Lit Solver::pickBranchLit()
{
    Var next = var_Undef;
    int clevel=decisionLevel();
    if(clevel>0 && clevel < 4){
          if(varRange){
            //  if(conflicts > 6000000 || recursiveDepth>0) goto nofound;
              if( (int)conflicts >= BCD_conf || recursiveDepth>0) goto nofound;
              Lit lit=trail[trail_lim[0]];
              int v = var(lit);
              if(varRange[v]==0 || v >= originVars) goto nofound;
              int ed = varRange[v];
              double maxAct=-10000;
              int vec[20];
              for (int i = varRange[v]+1; i >=ed-5; i--){//-5, -10
                        int *plit = Bclauses[i];
                        int j=0;
                        while(*plit){
                            int cv=ABS(*plit)-1;
                            if(value(cv) == l_Undef){
                                if(decision[cv] && j<20) vec[j++]=cv;  
                            }
                            else {
                                if(value(cv) == l_True) { if(*plit>0) goto nexti;}
                                else   if(*plit<0) goto nexti;
                            }
                            plit++;
                        }
                        for(j--; j>=0; j--){
                              if(activity[vec[j]]>maxAct){
                                   next=vec[j];
                                   maxAct=activity[next];
                              }
                        }
                        nexti: ;
              }
              if(next != var_Undef ) goto found;
//             next = var_Undef;
         }
    }
//
nofound:
   if (next == var_Undef){
        if (order_heap.empty()) return lit_Undef;
        else next = order_heap.removeMin();
    }
   
   // Activity based decision:
    while (value(next) != l_Undef || !decision[next])
        if (order_heap.empty()) return lit_Undef;
        else next = order_heap.removeMin();

found:
    if(bitVecNo>=0){
            if(clevel<12){
                   polarity[next]=(bitVecNo>>(clevel% mod_k)) & 1;
            }
     }
     return mkLit(next, rnd_pol ? drand(random_seed) < 0.5 : polarity[next]);
}


/*_________________________________________________________________________________________________
|
|  analyze : (confl : Clause*) (out_learnt : vec<Lit>&) (out_btlevel : int&)  ->  [void]
|  
|  Description:
|    Analyze conflict and produce a reason clause.
|  
|    Pre-conditions:
|      * 'out_learnt' is assumed to be cleared.
|      * Current decision level must be greater than root level.
|  
|    Post-conditions:
|      * 'out_learnt[0]' is the asserting literal at level 'out_btlevel'.
|      * If out_learnt.size() > 1 then 'out_learnt[1]' has the greatest decision level of the 
|        rest of literals. There may be others from the same level though.
|  
|________________________________________________________________________________________________@*/
void Solver::analyze(CRef confl, vec<Lit>& out_learnt, int& out_btlevel,unsigned int &lbd)
{
    int pathC = 0;
    Lit p     = lit_Undef;

    // Generate conflict clause:
    //
    out_learnt.push();      // (leave room for the asserting literal)
    int index   = trail.size() - 1;

    do{
        assert(confl != CRef_Undef); // (otherwise should be UIP)
        Clause& c = ca[confl];

	// Special case for binary clauses
	// The first one has to be SAT
	if( p != lit_Undef && c.size()==2 && value(c[0])==l_False) {
	  
	  assert(value(c[1])==l_True);
	  Lit tmp = c[0];
	  c[0] =  c[1], c[1] = tmp;
	}
	
       if (c.learnt() && c.mark() == 0)
            claBumpActivity(c);

#ifdef DYNAMICNBLEVEL		    
       // DYNAMIC NBLEVEL trick (see competition'09 companion paper)
       if(c.learnt()  && c.lbd()>2) { 
	 MYFLAG++;
	 unsigned  int nblevels =0;
	 for(int i=0;i<c.size();i++) {
	   int l = level(var(c[i]));
	   if (l != 0 && permDiff[l] != MYFLAG) {
	     permDiff[l] = MYFLAG;
	     nblevels++;
	   }
	 }
         if (c.mark() == 3) c.mark(2); //3->2 
    
         if(nblevels+1 <  c.lbd() ) { // improve the LBD
//	 if(nblevels+1 <= c.lbd() ) {
	   if(c.lbd()<=lbLBDFrozenClause) c.setCanBeDel(false); 
	   // seems to be interesting : keep it for the next round
	   c.setLBD(nblevels); // Update it
           if (nblevels <= lbd_cut && c.mark() == 0){
               corelearnts.push(confl); //core learnt clauses
               promoted2 = true;
               c.mark(3); 
           }
	 }
       }
#endif
       

        for (int j = (p == lit_Undef) ? 0 : 1; j < c.size(); j++){
            Lit q = c[j];

            if (!seen[var(q)] && level(var(q)) > 0){
                varBumpActivity(var(q));
                seen[var(q)] = 1;
                if (level(var(q)) >= decisionLevel()) {
                    pathC++;
#ifdef UPDATEVARACTIVITY
		    // UPDATEVARACTIVITY trick (see competition'09 companion paper)
		    if((reason(var(q))!= CRef_Undef)  && ca[reason(var(q))].learnt()) 
		      lastDecisionLevel.push(q);
#endif

		} else {
                    out_learnt.push(q);
		}
	    }
        }
        
        // Select next clause to look at:
        while (!seen[var(trail[index--])]);
        p     = trail[index+1];
        confl = reason(var(p));
        seen[var(p)] = 0;
        pathC--;

    }while (pathC > 0);
    out_learnt[0] = ~p;

    // Simplify conflict clause:
    //
    int i, j;
    out_learnt.copyTo(analyze_toclear);
    if (ccmin_mode == 2){
        uint32_t abstract_level = 0;
        for (i = 1; i < out_learnt.size(); i++)
            abstract_level |= abstractLevel(var(out_learnt[i])); // (maintain an abstraction of levels involved in conflict)

        for (i = j = 1; i < out_learnt.size(); i++)
            if (reason(var(out_learnt[i])) == CRef_Undef || !litRedundant(out_learnt[i], abstract_level))
                out_learnt[j++] = out_learnt[i];
        
    }else if (ccmin_mode == 1){
        for (i = j = 1; i < out_learnt.size(); i++){
            Var x = var(out_learnt[i]);

            if (reason(x) == CRef_Undef)
                out_learnt[j++] = out_learnt[i];
            else{
                Clause& c = ca[reason(var(out_learnt[i]))];
                for (int k = c.size() == 2 ? 0 : 1; k < c.size(); k++)
                    if (!seen[var(c[k])] && level(var(c[k])) > 0){
                        out_learnt[j++] = out_learnt[i];
                        break; }
            }
        }
    }else
        i = j = out_learnt.size();

    max_literals += out_learnt.size();
    out_learnt.shrink(i - j);
    tot_literals += out_learnt.size();


    /* ***************************************
      Minimisation with binary clauses of the asserting clause
      First of all : we look for small clauses
      Then, we reduce clauses with small LBD.
      Otherwise, this can be useless
     */
    if(out_learnt.size()<=lbSizeMinimizingClause) {
      // Find the LBD measure                                                                                                         
      lbd = 0;
      MYFLAG++;
      for(int i=0;i<out_learnt.size();i++) {

	int l = level(var(out_learnt[i]));
	if (permDiff[l] != MYFLAG) {
	  permDiff[l] = MYFLAG;
	  lbd++;
	}
      }


      if(lbd<=lbLBDMinimizingClause){
      MYFLAG++;
      
      for(int i = 1;i<out_learnt.size();i++) {
	permDiff[var(out_learnt[i])] = MYFLAG;
      }

      vec<Watcher>&  wbin  = watchesBin[p];
      int nb = 0;
      for(int k = 0;k<wbin.size();k++) {
	Lit imp = wbin[k].blocker;
	if(permDiff[var(imp)]==MYFLAG && value(imp)==l_True) {
	  /*      printf("---\n");
		  printClause(out_learnt);
		  printf("\n");
		  
		  printClause(*(wbin[k].clause));printf("\n");
	  */
	  nb++;
	  permDiff[var(imp)]= MYFLAG-1;
	}
      }
      int l = out_learnt.size()-1;
      if(nb>0) {
	nbReducedClauses++;
	for(int i = 1;i<out_learnt.size()-nb;i++) {
	  if(permDiff[var(out_learnt[i])]!=MYFLAG) {
	    Lit p = out_learnt[l];
	    out_learnt[l] = out_learnt[i];
	    out_learnt[i] = p;
	    l--;i--;
	  }
	}
	
	//    printClause(out_learnt);
	//printf("\n");
	out_learnt.shrink(nb);
      
	/*printf("nb=%d\n",nb);
	  printClause(out_learnt);
	  printf("\n");
	*/
      }
    }
    }
    // Find correct backtrack level:
    //
    if (out_learnt.size() == 1)
        out_btlevel = 0;
    else{
        int max_i = 1;
        // Find the first literal assigned at the next-highest level:
        for (int i = 2; i < out_learnt.size(); i++)
            if (level(var(out_learnt[i])) > level(var(out_learnt[max_i])))
                max_i = i;
        // Swap-in this literal at index 1:
        Lit p             = out_learnt[max_i];
        out_learnt[max_i] = out_learnt[1];
        out_learnt[1]     = p;
        out_btlevel       = level(var(p));
    }


  // Find the LBD measure 
  lbd = 0;
  MYFLAG++;
  for(int i=0;i<out_learnt.size();i++) {
    
    int l = level(var(out_learnt[i]));
    if (permDiff[l] != MYFLAG) {
      permDiff[l] = MYFLAG;
      lbd++;
    }
  }


  
#ifdef UPDATEVARACTIVITY
  // UPDATEVARACTIVITY trick (see competition'09 companion paper)
  if(lastDecisionLevel.size()>0) {
    for(int i = 0;i<lastDecisionLevel.size();i++) {
      if(ca[reason(var(lastDecisionLevel[i]))].lbd()<lbd)
	varBumpActivity(var(lastDecisionLevel[i]));
    }
    lastDecisionLevel.clear();
  } 
#endif	    



    for (int j = 0; j < analyze_toclear.size(); j++) seen[var(analyze_toclear[j])] = 0;    // ('seen[]' is now cleared)
}


// Check if 'p' can be removed. 'abstract_levels' is used to abort early if the algorithm is
// visiting literals at levels that cannot be removed later.
bool Solver::litRedundant(Lit p, uint32_t abstract_levels)
{
    analyze_stack.clear(); analyze_stack.push(p);
    int top = analyze_toclear.size();
    while (analyze_stack.size() > 0){
        assert(reason(var(analyze_stack.last())) != CRef_Undef);
        Clause& c = ca[reason(var(analyze_stack.last()))]; analyze_stack.pop();
	if(c.size()==2 && value(c[0])==l_False) {
	  assert(value(c[1])==l_True);
	  Lit tmp = c[0];
	  c[0] =  c[1], c[1] = tmp;
	}

        for (int i = 1; i < c.size(); i++){
            Lit p  = c[i];
            if (!seen[var(p)] && level(var(p)) > 0){
                if (reason(var(p)) != CRef_Undef && (abstractLevel(var(p)) & abstract_levels) != 0){
                    seen[var(p)] = 1;
                    analyze_stack.push(p);
                    analyze_toclear.push(p);
                }else{
                    for (int j = top; j < analyze_toclear.size(); j++)
                        seen[var(analyze_toclear[j])] = 0;
                    analyze_toclear.shrink(analyze_toclear.size() - top);
                    return false;
                }
            }
        }
    }

    return true;
}


/*_________________________________________________________________________________________________
|
|  analyzeFinal : (p : Lit)  ->  [void]
|  
|  Description:
|    Specialized analysis procedure to express the final conflict in terms of assumptions.
|    Calculates the (possibly empty) set of assumptions that led to the assignment of 'p', and
|    stores the result in 'out_conflict'.
|________________________________________________________________________________________________@*/
void Solver::analyzeFinal(Lit p, vec<Lit>& out_conflict)
{
    out_conflict.clear();
    out_conflict.push(p);

    if (decisionLevel() == 0)  return;

    seen[var(p)] = 1;

    for (int i = trail.size()-1; i >= trail_lim[0]; i--){
        Var x = var(trail[i]);
        if (seen[x]){
            if (reason(x) == CRef_Undef){
                assert(level(x) > 0);
                out_conflict.push(~trail[i]);
            }else{
                Clause& c = ca[reason(x)];
		//                for (int j = 1; j < c.size(); j++) Minisat (glucose 2.0) loop 
		// Bug in case of assumptions due to special data structures for Binary.
		// Many thanks to Sam Bayless (sbayless@cs.ubc.ca) for discover this bug.
		for (int j = ((c.size()==2) ? 0:1); j < c.size(); j++)
                    if (level(var(c[j])) > 0)
                        seen[var(c[j])] = 1;
            }  

            seen[x] = 0;
        }
    }

    seen[var(p)] = 0;
}


void Solver::uncheckedEnqueue(Lit p, CRef from)
{
    assert(value(p) == l_Undef);
    assigns[var(p)] = lbool(!sign(p));
    vardata[var(p)] = mkVarData(from, decisionLevel());
    trail.push_(p);
}


/*_________________________________________________________________________________________________
|
|  propagate : [void]  ->  [Clause*]
|  
|  Description:
|    Propagates all enqueued facts. If a conflict arises, the conflicting clause is returned,
|    otherwise CRef_Undef.
|  
|    Post-conditions:
|      * the propagation queue is empty, even if there was a conflict.
|________________________________________________________________________________________________@*/
CRef Solver::propagate()
{
    CRef    confl     = CRef_Undef;
    int     num_props = 0;
    watches.cleanAll();
    watchesBin.cleanAll();
    while (qhead < trail.size()){
        Lit            p   = trail[qhead++];     // 'p' is enqueued fact to propagate.
        vec<Watcher>&  ws  = watches[p];
        Watcher        *i, *j, *end;
        num_props++;

	
	    // First, Propagate binary clauses 
	vec<Watcher>&  wbin  = watchesBin[p];
	
	for(int k = 0;k<wbin.size();k++) {
	  
	  Lit imp = wbin[k].blocker;
	  
	  if(value(imp) == l_False) {
	    return wbin[k].cref;
	  }
	  
	  if(value(imp) == l_Undef) {
	    //printLit(p);printf(" ");printClause(wbin[k].cref);printf("->  ");printLit(imp);printf("\n");
	    uncheckedEnqueue(imp,wbin[k].cref);
	  }
	}
    


        for (i = j = (Watcher*)ws, end = i + ws.size();  i != end;){
            // Try to avoid inspecting the clause:
            Lit blocker = i->blocker;
            if (value(blocker) == l_True){
                *j++ = *i++; continue; }

            // Make sure the false literal is data[1]:
            CRef     cr        = i->cref;
            Clause&  c         = ca[cr];
            Lit      false_lit = ~p;
            if (c[0] == false_lit)
                c[0] = c[1], c[1] = false_lit;
            assert(c[1] == false_lit);
            i++;

            // If 0th watch is true, then clause is already satisfied.
            Lit     first = c[0];
            Watcher w     = Watcher(cr, first);
            if (first != blocker && value(first) == l_True){
	      
	      *j++ = w; continue; }

            // Look for new watch:
            for (int k = 2; k < c.size(); k++)
                if (value(c[k]) != l_False){
                    c[1] = c[k]; c[k] = false_lit;
                    watches[~c[1]].push(w);
                    goto NextClause; }

            // Did not find watch -- clause is unit under assignment:
            *j++ = w;
            if (value(first) == l_False){
                confl = cr;
                qhead = trail.size();
                // Copy the remaining watches:
                while (i < end)
                    *j++ = *i++;
            }else {
                uncheckedEnqueue(first, cr);
	  
		
	    }
        NextClause:;
        }
        ws.shrink(i - j);
    }
    propagations += num_props;
    simpDB_props -= num_props;
    
    return confl;
}


/*_________________________________________________________________________________________________
|
|  reduceDB : ()  ->  [void]
|  
|  Description:
|    Remove half of the learnt clauses, minus the clauses locked by the current assignment. Locked
|    clauses are clauses that are reason to some assignment. Binary clauses are never removed.
|________________________________________________________________________________________________@*/
struct reduceDB_lt { 
    ClauseAllocator& ca;
    reduceDB_lt(ClauseAllocator& ca_) : ca(ca_) {}
    bool operator () (CRef x, CRef y) const { return ca[x].activity() < ca[y].activity(); }    
#if 0
    bool operator () (CRef x, CRef y) const {
    // Main criteria... Like in MiniSat we keep all binary clauses              
    if(ca[x].size()> 2 && ca[y].size()==2) return 1;                            
                                                                                
    if(ca[y].size()>2 && ca[x].size()==2) return 0;                             
    if(ca[x].size()==2 && ca[y].size()==2) return 0;                            
                                                                                
    // Second one  based on literal block distance                              
    if(ca[x].lbd()> ca[y].lbd()) return 1;                                      
    if(ca[x].lbd()< ca[y].lbd()) return 0;                                      
                                                                                
    // Finally we can use old activity or size, we choose the last one          
    return ca[x].activity() < ca[y].activity();
    }
#endif
};

void Solver::reduceDB()
{
      int     i, j;
      nbReduceDB++;

      if(corelearnts.size()>100000){
          int k=corelearnts.size()-100000;
          int m=corelearnts.size()-5000;
          for (i = j = 0; i < corelearnts.size(); i++){
               Clause& c = ca[corelearnts[i]];
               if (c.canBeDel() && !locked(c) && k > 0 && i<m && c.lbd() >= lbd_cut ){
                      removeClause(corelearnts[i]);
                      nbRemovedClauses++;
                      k--;
               }
               else{
                     c.setCanBeDel(true);
                     corelearnts[j++] = corelearnts[i];
               }
           }
           corelearnts.shrink(i - j);
       } 

      sort(learnts2, reduceDB_lt(ca));
   

      int limit = learnts2.size() / 2;
      for (i = j = 0; i < learnts2.size(); i++){
        Clause& c = ca[learnts2[i]];
        if (c.mark() == 0)
            if (c.canBeDel() && !locked(c) && i < limit){
              removeClause(learnts2[i]);
              nbRemovedClauses++;
            }
            else {
              if(!c.canBeDel()) limit++; //we keep c, so we can delete an other clause
              c.setCanBeDel(true);       // At the next step, c can be delete
              learnts2[j++] = learnts2[i];
            }
      }
      learnts2.shrink(i - j);
      promoted2 = false;

//new idea
      unsigned int l_cut=3;
      if(recursiveDepth) l_cut=4; 
      if(lbd_cut>2){
        //int m=5000;
        for (i = j = 0; i < corelearnts.size(); i++){
            Clause& c = ca[corelearnts[i]];
            if (c.mark() == 3 && c.lbd() >= l_cut){
               learnts2.push(corelearnts[i]); 
               c.mark(0); 
	       c.setLBD(100);
             }
             else corelearnts[j++] = corelearnts[i];
         } 
         corelearnts.shrink(i - j);
      }

      checkGarbage();
}

void Solver::clearDB()
{
  int     i;
 
  for (i =0; i < learnts2.size(); i++){
        Clause& c = ca[learnts2[i]];
        if (c.mark() == 0) removeClause(learnts2[i]);
  }
  learnts2.shrink(i);
  for (i =0; i < corelearnts.size(); i++) removeClause(corelearnts[i]);
  corelearnts.shrink(i);

  checkGarbage();
}


void Solver::removeSatisfied(vec<CRef>& cs)
{
  
    int i, j;
    for (i = j = 0; i < cs.size(); i++){
        Clause& c = ca[cs[i]];


        if (c.size()>=2 && satisfied(c)) // A bug if we remove size ==2, We need to correct it, but later.
            removeClause(cs[i]);
        else
            cs[j++] = cs[i];
    }
    cs.shrink(i - j);
}


void Solver::rebuildOrderHeap()
{
    vec<Var> vs;
    for (Var v = 0; v < nVars(); v++)
        if (decision[v] && value(v) == l_Undef)
            vs.push(v);
    order_heap.build(vs);
}

void Solver::additional_del()
{
   int i, j;
   if (promoted2){
        for (i = j = 0; i < learnts2.size(); i++)
            if (ca[learnts2[i]].mark() == 0)  learnts2[j++] = learnts2[i];
        learnts2.shrink(i - j);
        promoted2 = false;
    }
}  

/*_________________________________________________________________________________________________
|
|  simplify : [void]  ->  [bool]
|  
|  Description:
|    Simplify the clause database according to the current top-level assigment. Currently, the only
|    thing done here is the removal of satisfied clauses, but more things can be put here.
|________________________________________________________________________________________________@*/
bool Solver::simplify()
{
    assert(decisionLevel() == 0);

    if (!ok || propagate() != CRef_Undef)
        return ok = false;

    if (nAssigns() == simpDB_assigns || (simpDB_props > 0))
        return true;

    additional_del();

    // Remove satisfied clauses:
    removeSatisfied(corelearnts);
    removeSatisfied(learnts2);
    if (remove_satisfied){        // Can be turned off.

          static int pre_conf=0;
          if((nVars() < 500000 || longcls<300000) && (conflicts>200000 || recursiveDepth>3) && (nClauses()<300000 || ((int)conflicts-pre_conf)>120000)){        pre_conf=(int)conflicts;
                  lbool ret=probe();
                  if(ret == l_False){
                        probe_unsat=1;//AProVE07-03
                        return ok = false;
                  }
          }
          else  removeSatisfied(clauses);
    }
    checkGarbage();
    rebuildOrderHeap();

    simpDB_assigns = nAssigns();
    simpDB_props   = clauses_literals + learnts_literals;   // (shouldn't depend on stats really, but it will do for now)

    return true;
}


/*_________________________________________________________________________________________________
|
|  search : (nof_conflicts : int) (params : const SearchParams&)  ->  [lbool]
|  
|  Description:
|    Search for a model the specified number of conflicts. 
|    NOTE! Use negative value for 'nof_conflicts' indicate infinity.
|  
|  Output:
|    'l_True' if a partial assigment that is consistent with respect to the clauseset is found. If
|    all variables are decision variables, this means that the clause set is satisfiable. 'l_False'
|    if the clause set is unsatisfiable. 'l_Undef' if the bound on number of conflicts is reached.
|________________________________________________________________________________________________@*/
lbool Solver::search(int nof_conflicts)
{
    assert(ok);
    int         backtrack_level;
    int         conflictC = 0;
    vec<Lit>    learnt_clause;
    unsigned int nblevels = 0;
    bool blocked=false;
    int reduceDBsize=20000;
    if(recursiveDepth>0 && recursiveDepth<13) reduceDBsize=10000;
    starts++;

    if(conflicts>2000000 || recursiveDepth) lbd_cut=5;
    int Vn=nVars();
    if(recursiveDepth==0 && Vn>4000 && Vn<11000 && nClauses() > 20*Vn && conflicts>7000000) lbd_cut=0;
   
    if(recursiveDepth==0) verbEveryConflicts=20000;
    else verbEveryConflicts=40000;

    for (;;){
        CRef confl = propagate();
        if (confl != CRef_Undef){
            conflicts++; conflictC++;
           if(conflicts%5000==0 && var_decay<0.95) var_decay += 0.01;
         
           if (verbosity >= 1 && conflicts%verbEveryConflicts==0){
             printf("c | %g core=%d ", cpuTime(),corelearnts.size());
             printf("c | %8d   %7d    %5d | %7d %8d %8d | %5d %8d | %6d %8d | %6.3f %% | LBD=%d %d\n",
                    (int)starts,(int)nbstopsrestarts, (int)(conflicts/starts), 
                    (int)dec_vars - (trail_lim.size() == 0 ? trail.size() : trail_lim[0]), nClauses(), (int)clauses_literals, 
                    (int)nbReduceDB, learnts2.size(), (int)nbDL2,(int)nbRemovedClauses, progressEstimate()*100,
                    int(sumLBD / conflicts),(int)conflicts);
            }
            if (decisionLevel() == 0) return l_False;
	  
            trailQueue.push(trail.size());
            if( conflicts>LOWER_BOUND_FOR_BLOCKING_RESTART && lbdQueue.isvalid()  && trail.size()>R*trailQueue.getavg()) {
                lbdQueue.fastclear();
                nbstopsrestarts++;
                if(!blocked) {lastblockatrestart=starts;nbstopsrestartssame++;blocked=true;}
            }

            learnt_clause.clear();
            analyze(confl, learnt_clause, backtrack_level,nblevels);

            lbdQueue.push(nblevels);
            sumLBD += nblevels;

            cancelUntil(backtrack_level);

            if (learnt_clause.size() == 1){
                uncheckedEnqueue(learnt_clause[0]);nbUn++;
            }else{
                CRef cr = ca.alloc(learnt_clause, true);
                if(lbd_cut<=3 && recursiveDepth==0){
                    ca[cr].setLBD(nblevels); 
                    if (nblevels <= lbd_cut || (clauses.size()>2000000 && nblevels <= (lbd_cut+1))){
                        corelearnts.push(cr);
                        ca[cr].mark(3);
                    }else{
                        learnts2.push(cr);
                        claBumpActivity(ca[cr]); 
                    }
               }
               else{
                   ca[cr].setLBD(0x7fffffff); 
                   learnts2.push(cr);
                   claBumpActivity(ca[cr]); 
               }
     
               attachClause(cr);

                uncheckedEnqueue(learnt_clause[0], cr);

                if(nblevels<=2) nbDL2++; // stats
                if(ca[cr].size()==2) nbBin++; // stats
            }
            varDecayActivity();
            claDecayActivity();
        }else{
            if((luby_restart && conflictC >= nof_conflicts) || 
                (nof_conflicts != -1 && conflicts > (unsigned)nof_conflicts)) goto backt;
        
            // Our dynamic restart, see the SAT09 competition compagnion paper 
            if (lbdQueue.isvalid() && ((lbdQueue.getavg()*K) > (sumLBD / conflicts)) || (nVars() <= 2000 && conflicts/500>starts)){//new idea
                  lbdQueue.fastclear();
                  progress_estimate = progressEstimate();
backt:          
                  int lev = 0;
	          if(incremental) { // do not back to 0
	               lev = (decisionLevel()<assumptions.size()) ? decisionLevel() : assumptions.size();
	          }
                  cancelUntil(lev);
                  return l_Undef; 
            }
           // Simplify the set of problem clauses:
            if (decisionLevel() == 0 && !simplify()){
              //  printf("c last restart ## conflicts  :  %d %d \n",conflictC,decisionLevel());
                return l_False;
            }

              if (learnts2.size() > reduceDBsize) reduceDB();
	    
            Lit next = lit_Undef;
            if(incremental) { 
	         while (decisionLevel() < assumptions.size()){
                    // Perform user provided assumption:
                    Lit p = assumptions[decisionLevel()];
                    if (value(p) == l_True){
                       // Dummy decision level:
                       newDecisionLevel();
                    }else if (value(p) == l_False){
                          analyzeFinal(~p, conflict);
                          return l_False;
                    }else{
                          next = p;
                          break;
                     }
                 }
            }

            if (next == lit_Undef){
                // New variable decision:
                decisions++;
                next = pickBranchLit();
                if (next == lit_Undef) return l_True;
            }

            // Increase decision level and enqueue 'next'
            newDecisionLevel();
            uncheckedEnqueue(next);
        }
    }
}


double Solver::progressEstimate() const
{
    double  progress = 0;
    double  F = 1.0 / nVars();

    for (int i = 0; i <= decisionLevel(); i++){
        int beg = i == 0 ? 0 : trail_lim[i - 1];
        int end = i == decisionLevel() ? trail.size() : trail_lim[i];
        progress += pow(F, i) * (end - beg);
    }

    return progress / nVars();
}

/*
  Finite subsequences of the Luby-sequence:
  0: 1
  1: 1 1 2
  2: 1 1 2 1 1 2 4
  3: 1 1 2 1 1 2 4 1 1 2 1 1 2 4 8
  ...
 */

static double luby(double y, int x){

    // Find the finite subsequence that contains index 'x', and the
    // size of that subsequence:
    int size, seq;
    for (size = 1, seq = 0; size < x+1; seq++, size = 2*size+1);
    while (size-1 != x){
        size = (size-1)>>1;
        seq--;
        x = x % size;
    }
    return pow(y, seq);
}

void SameReplaceSolve(int *Fsolution, PPS *pps);
void gaussTriSolve(int *Fsolution, PPS *pps);

void Solver:: buildEquAssume()
{   vec<Lit> lits;
    if(equhead==0) return;
    for (int i=1; i <= nVars(); i++){
         if(equhead[i]==0 || equhead[i]==i) continue;
         Lit p=mkLit(i-1);
         int lit=equhead[i];
         Lit q = (lit>0) ? mkLit(lit-1) : ~mkLit(-lit-1);
         for(int j=0; j< assumptions.size(); j++){
                 Lit r = assumptions[j];
                 if(r==p || r==~p || r==q || r==~q){
                     if(!hasBin(p, ~q)){
                         lits.clear();
                         lits.push(p);
                         lits.push(~q);
                         simpAddClause(lits);
                     }
                     if(!hasBin(~p, q)){
                         lits.clear();
                         lits.push(~p);
                         lits.push(q);
                         simpAddClause(lits);
                    }
                    goto nextbin;
                }
         }
nextbin: ;
    }
}

lbool Solver::addassume()
{
      if(assumptions.size() == 0) return l_False;
      vec<Lit> lits;
      lits.clear();
      for(int i=0; i < assumptions.size(); i++){
             Lit p = assumptions[i];
             lits.push(~p);
      }
      bool ret=addClause(lits);
      if(ret==false) return l_False;
      return l_Undef;
}

// NOTE: assumptions passed in member-variable 'assumptions'.
lbool Solver::solve_()
{
    outvars=originVars=nVars();
    model.clear();
    conflict.clear();
    if (!ok) return l_False;

    longcls=longClauses();
    int bincls=nClauses()-longcls;
   
    incremental=1;
   // if(longcls < 300000 && conflicts < 10){
   //       if(probe() == l_False) return l_False;
   // }
    rebuildOrderHeap();
   
    lbdQueue.initSize(sizeLBDQueue);
    trailQueue.initSize(sizeTrailQueue);
    sumLBD = 0;
    solves++;
    lbool   status        = l_Undef;
    nbclausesbeforereduce = firstReduceDB;
    verbEveryConflicts=10000;
  
    // Search:
    int ct=originVars>210000? 120000 :10000;
    bool solvecond= (nClauses() > 1500000) || (originVars>100000 && longcls<10000) || 
           (originVars>50000 && originVars<250000 && longcls>50000 && nClauses()<2*originVars+ct) ;

    solvecond=false;
    bool Thisolver=false;

    int curr_restarts = 0;
    int assume_flag=0;

    bitVecNo=-1;
  //  uint64_t  no_inc_limit =conflicts+20000;// 800;
    uint64_t  yes_inc_limit=conflicts+40000;
nextfind:    
    while (status == l_Undef){
         int climit=-1;
         if(originVars>100000 && longcls<10000) bitVecNo++;
         else{
            if(conflicts > 2000000 || conflicts > 300000 && originVars > 1000000) bitVecNo=-1;
            else  bitVecNo++;
         }

         if(5000000 < conflicts && conflicts<6500000){
              luby_restart=1;
              double rest_base = luby(restart_inc, curr_restarts);
              climit=rest_base * restart_first;
              curr_restarts++;
         }
         else luby_restart=0;
     
        // if(no_inc_limit<conflicts){
        //     if(incremental==0) buildEquAssume();
        //     incremental=1;
        // }
         status = search(climit);
         if (!withinBudget()) break;
         if(Thisolver || incremental==0) continue;
         if(yes_inc_limit<conflicts) continue;

         bool newcond=longcls < 1900000 && int(sumLBD / conflicts) < 5 && bincls>800000; 
         if(!solvecond || newcond){
            if(status == l_Undef){
                printf("c mid_simplification,then solve....conflicts=%d \n",(int)conflicts);
                cancelUntil(0);
                int rc=midsimp_solve();
                if(rc==SAT){
                       status=l_True;
                       solveEqu(equhead,model);
                       verify();
                       return l_True;
                }
                if(rc==UNSAT) {
                    if(assumptions.size() == 0) {
                          status = l_False;
                          break;
                    }
                    assume_flag=1;
	            status=addassume();
                    if(status == l_False) return l_False;
               }
            }
            Thisolver=true;
       }  
    }

    if (verbosity >= 1)
        printf("c abcdsat2016: conflicts=%d \n",(int)conflicts);
               
    if (status == l_True){
        if(assumptions.size()>0 && incremental==0){
            buildEquAssume();
            incremental=1;
            cancelUntil(0);
            status = l_Undef;
            goto nextfind;
        }
        // Extend & copy model:
        model.growTo(nVars());
        for (int i = 0; i < nVars(); i++) model[i] = value(i);
        solveEqu(equhead,model);
        verify();
    }
    else {
           if(assume_flag==0){
              if(decisionLevel() && incremental) addassume();
              if (status == l_False && conflict.size() == 0) ok = false;
           }
    }
    cancelUntil(0);
    return status;
}
//---------------------------------------------------
lbool Solver::solve2_()
{
    originVars=nVars();
    size30=0;
  
    BCDfalses=1000;
    BCD_conf=0;
    int mp= (originVars<10000)? 30: 21; 
    if(recursiveDepth==0 && nClauses()< mp*originVars){//7000
         if(originVars > 1600 && originVars < 15000)  BCD_conf=6000001;
         else                                         BCD_conf=500001;
    }
    if(BCD_conf){
          if(originVars<4400 || originVars>4700 || nClauses()< 14*originVars) fast_bcd();
    }

    model.clear();
    conflict.clear();
    if (!ok) return l_False;
 
    lbdQueue.initSize(sizeLBDQueue);

    trailQueue.initSize(sizeTrailQueue);
    sumLBD = 0;
    
    solves++;
    
    lbool   status        = l_Undef;
    nbclausesbeforereduce = firstReduceDB;
    verbEveryConflicts=10000;
   
    bitVecNo=-1;
    verbosity=1;
    if(verbosity>0)
      printf("c D=%d hard#=%d unsat#=%d unsat9#=%d \n",recursiveDepth,all_conflicts,unsatCnt, unsat9Cnt);
  
    int force=0;
    if(unsatCnt>2*unsat9Cnt && completesolve==0 && (recursiveDepth>=9 && recursiveDepth<=11) && all_conflicts <100){
          completesolve=1;
          force=1;
    }

    int oldfreeVars=nUnfixedVars();
    bitVecNo=-1;
    int curr_restarts = 0;
    int sLBD=0;
    while (status == l_Undef){
        int climit=conflict_limit;
        if(recursiveDepth==0){
            if((size30>6 && size30 <20) || outvars<500) bitVecNo=-1;
            else{
              if(BCDfalses>30){
                 if(conflicts > 3000000) bitVecNo=-1;
                 else  {
                    if(conflicts || outvars>1500) bitVecNo++;
                 }
               }
            }
         }
         else{
             if(recursiveDepth<=9 && conflicts < 30000) bitVecNo++;
             else bitVecNo=-1;

             if(completesolve==1){
                   if(recursiveDepth < 11){
                        if(nVars() < 1400 && progressEstimate()>0.081 && (force || recursiveDepth !=9)) climit=-1;
                        else climit=force ? 600000:50000;
                   }
                   else  climit=-1;
             }
             if(completesolve==0 || cancel_subsolve){
                  if(recursiveDepth < 11) climit=500;
                  else climit=10000;
             }
             if(recursiveDepth > 100) climit=-1;
         }
         if(climit != -1) {
                 if(conflicts >= (unsigned) climit) goto done; 
         }
//luby    
         if(5000000 < conflicts && conflicts<6500000 && recursiveDepth==0){
              luby_restart=1;
              double rest_base = luby(restart_inc, curr_restarts);
              climit=rest_base * restart_first;
              curr_restarts++;
         }
         else luby_restart=0;
  
         status = search(climit);
         if (!withinBudget()) break;

         if(BCD_conf>500001){
            if(conflicts>5000 && conflicts<300000 & sumLBD < 6*conflicts && sLBD==0) sLBD=int(sumLBD/conflicts);
            if(conflicts>300000 && sumLBD < 8*conflicts && (sLBD==0 || int(sumLBD/conflicts) <= sLBD)){
                if(BCD_conf!=500001) printf("c BCD_conf=500000 ---------------------\n");
                BCD_conf=500001;//new idea
            }
         }
//new idea
         if(BCD_conf){
              if(nClauses()>20*originVars && sumLBD < 10*conflicts && conflicts<1500000) continue;//52bits_12.dimacs
         }   

         if(recursiveDepth > 1900){
               if(conflicts > 20000){
                   if(status == l_Undef) goto done; 
               } 
               continue;
         }   
         if(recursiveDepth > 100){
               if(conflicts > 1500000){
                   if(status == l_Undef) goto done;//return l_Undef;
               } 
               continue;
         }   
         if(recursiveDepth == 0){
              if(conflicts>200000) {
                    if((oldfreeVars==nUnfixedVars() && sumLBD /7 < conflicts && oldfreeVars<2000) ||
                      40*nUnfixedVars() < outvars || nClauses()>600000 || ((hbrsum>110000 || progressEstimate()>0.011) && conflicts<300000)){
                          if(varRange){
                                free(varRange);
                                printf("c remove BCD pick \n");
                          }
                          varRange=0;
                    } 
              }
              if(nVars() < 1400 && ((nVars()>=nUnfixedVars()+2 && conflicts<1500000) ||conflicts<500000)) continue;
              if(nVars() > 2500 && conflicts<6000000) continue;
              int aLBD=sumLBD/conflicts;
              if(aLBD > 16 || nVars() > 10000 || outvars<500) continue;
              if(nVars() > 1500){
                  if(sumLBD /13 < conflicts){
                        if(conflicts<500000) continue;
                        if(outvars>3000 && conflicts<2000000) continue;
                  }
                  if(conflicts>7000000 || conflicts>3500000 && (aLBD<=10 || nVars() > 3500 && aLBD<=11)) {
                         cancel_subsolve=1; continue;
                  }
               }
         }

         if(status==l_False){
                  if(recursiveDepth <= 9) unsat9Cnt++;
                  break;
         }

         if(recursiveDepth >= 11 && conflicts > 10000){
               if(all_conflicts> unsatCnt+1300) cancel_subsolve=1;
               if(conflicts > 3500000) cancel_subsolve=1;
         }

         if(recursiveDepth == 0 && cancel_subsolve) continue;
       
         if(completesolve==0){
             if(recursiveDepth==0 && conflicts >= 100000 || recursiveDepth>0 && conflicts >= (unsigned)climit && recursiveDepth < 9){//10
                if(status == l_Undef) {
                       status=subsolve();
                       if(recursiveDepth==0 && status == l_Undef){
                             completesolve=1;
                             if(!cancel_subsolve) {
                                status=subsolve();
                             }
                       }
                 }
                 if(recursiveDepth) break;
                 cancel_subsolve=1;
                 continue;
             }
             if(conflicts >= (unsigned)climit) goto done;
             continue; 
         }
         if(recursiveDepth >=11 || climit==-1){
              if(conflicts < 3500000) continue;
              cancel_subsolve=1;
         }
         if(cancel_subsolve) goto done;
         if(conflicts >= (unsigned)climit){
               if(status == l_Undef)  status=subsolve();
         }
    }

    if (verbosity >= 1 && recursiveDepth==0)
      printf("c =========================================================================================================\n");
      if(conflicts > 650000) all_conflicts += 60;//100
      else if(conflicts > 230000){
            int m=conflicts/65000; 
            all_conflicts += (m*m);
      }
      else all_conflicts += ((int)conflicts/80000);
done:
     if(recursiveDepth) {
           additional_del();
           ClearClause(learnts2);
     }   
    if (status == l_Undef){
          cancelUntil(0);
          buildEquClause();
          return l_Undef;
    }
    if (status == l_True){
        // Extend & copy model:
        printf("c solution found by s2 rd=%d \n",recursiveDepth);
        solveEqu(equhead,assigns);
        if(recursiveDepth) return status;
        model.growTo(nVars());
        for (int i = 0; i < nVars(); i++) model[i] = value(i);
        return status;
      //  check(solution, g_pps->clause);
    }
    if(conflict.size() == 0) ok = false;
    unsatCnt++;
    return l_False;
}

//=================================================================================================
// Writing CNF to DIMACS:
// 
// FIXME: this needs to be rewritten completely.

static Var mapVar(Var x, vec<Var>& map, Var& max)
{
    if (map.size() <= x || map[x] == -1){
        map.growTo(x+1, -1);
        map[x] = max++;
    }
    return map[x];
}


void Solver::toDimacs(FILE* f, Clause& c, vec<Var>& map, Var& max)
{
    if (satisfied(c)) return;

    for (int i = 0; i < c.size(); i++)
        if (value(c[i]) != l_False)
            fprintf(f, "%s%d ", sign(c[i]) ? "-" : "", mapVar(var(c[i]), map, max)+1);
    fprintf(f, "0\n");
}


void Solver::toDimacs(const char *file, const vec<Lit>& assumps)
{
    FILE* f = fopen(file, "wr");
    if (f == NULL)
        fprintf(stderr, "could not open file %s\n", file), exit(1);
//    toDimacs(f, assumps);
    (void)assumps;
    toDimacs(f);
    fclose(f);
}


void Solver::toDimacs(FILE* f)
{
    // Handle case when solver is in contradictory state:
    if (!ok){
        fprintf(f, "p cnf 1 2\n1 0\n-1 0\n");
        return; }

    vec<Var> map; Var max = 0;

    // Cannot use removeClauses here because it is not safe
    // to deallocate them at this point. Could be improved.
    int cnt = 0;
    for (int i = 0; i < clauses.size(); i++)
        if (!satisfied(ca[clauses[i]]))
            cnt++;
        
    for (int i = 0; i < clauses.size(); i++)
        if (!satisfied(ca[clauses[i]])){
            Clause& c = ca[clauses[i]];
            for (int j = 0; j < c.size(); j++)
                if (value(c[j]) != l_False)
                    mapVar(var(c[j]), map, max);
        }

    // Assumptions are added as unit clauses:
    cnt += assumptions.size();

    fprintf(f, "p cnf %d %d\n", max, cnt);

    for (int i = 0; i < assumptions.size(); i++){
        assert(value(assumptions[i]) != l_False);
        fprintf(f, "%s%d 0\n", sign(assumptions[i]) ? "-" : "", mapVar(var(assumptions[i]), map, max)+1);
    }

    for (int i = 0; i < clauses.size(); i++)
        toDimacs(f, ca[clauses[i]], map, max);

    if (verbosity > 0)
        printf("c Wrote %d clauses with %d variables.\n", cnt, max);
}


//=================================================================================================
// Garbage Collection methods:

void Solver::relocAll(ClauseAllocator& to)
{
    // All watchers:
    //
    // for (int i = 0; i < watches.size(); i++)
    watches.cleanAll();
    watchesBin.cleanAll();
    for (int v = 0; v < nVars(); v++)
        for (int s = 0; s < 2; s++){
            Lit p = mkLit(v, s);
            // printf(" >>> RELOCING: %s%d\n", sign(p)?"-":"", var(p)+1);
            vec<Watcher>& ws = watches[p];
            for (int j = 0; j < ws.size(); j++)
                ca.reloc(ws[j].cref, to);
            vec<Watcher>& ws2 = watchesBin[p];
            for (int j = 0; j < ws2.size(); j++)
                ca.reloc(ws2[j].cref, to);
        }

    // All reasons:
    //
    for (int i = 0; i < trail.size(); i++){
        Var v = var(trail[i]);

        if (reason(v) != CRef_Undef && (ca[reason(v)].reloced() || locked(ca[reason(v)])))
            ca.reloc(vardata[v].reason, to);
    }

    // All learnt:
    //
    for (int i = 0; i < corelearnts.size(); i++)
        ca.reloc(corelearnts[i], to);
    for (int i = 0; i < learnts2.size(); i++)
        ca.reloc(learnts2[i], to);

    // All original:
    //
    for (int i = 0; i < clauses.size(); i++)
        ca.reloc(clauses[i], to);
}


void Solver::garbageCollect()
{
    // Initialize the next region to a size corresponding to the estimated utilization degree. This
    // is not precise but should avoid some unnecessary reallocations for the new region:
    ClauseAllocator to(ca.size() - ca.wasted()); 

    relocAll(to);
    if (verbosity >= 2)
        printf("c|  Garbage collection:   %12d bytes => %12d bytes             |\n", 
               ca.size()*ClauseAllocator::Unit_Size, to.size()*ClauseAllocator::Unit_Size);
    to.moveTo(ca);
}
//----------------------------------------------
// a bin clasue exists?
inline bool Solver::hasBin(Lit p, Lit q)
{
    vec<Watcher>&  wbin  = watchesBin[~p];
    for(int k = 0;k<wbin.size();k++) {
	  Lit imp = wbin[k].blocker;
	  if( imp == q) return true;
    }
    return false;
}	  

inline void Solver :: simpAddClause(const vec<Lit>& lits)
{
     CRef cr = ca.alloc(lits, false);
     clauses.push(cr);
     attachClause(cr);
}

CRef Solver::trypropagate(Lit p)//new idea
{
    newDecisionLevel();
    uncheckedEnqueue(p);
    return propagate();
}    

inline void Solver::setmark(vec<int> & liftlit)
{   liftlit.clear();
    for (int c = trail.size()-1; c >= trail_lim[0]; c--) {
            int lit=toInt(trail[c]);
            mark[lit]=1;
            liftlit.push(lit);
    }
    cancelUntil(0);
}

inline void Solver::clearmark(vec<int> & liftlit)
{
     for (int i =0; i < liftlit.size(); i++) mark[liftlit[i]]=0;
}

lbool Solver:: addequ(Lit p, Lit q)
{ 
    if(var(p) < var(q)){
         Lit t=p;  p=q;  q=t;
    }
    int pl=toIntLit(p);
    int ql=toIntLit(q);
    if(pl == ql)  return l_Undef;
    if(pl == -ql) return l_False;

    touchVar.push(var(p));
    touchVar.push(var(q));
 //   printf(" %d=%d ",toIntLit(p),toIntLit(q));
 
    if(pl<0){ pl=-pl; ql=-ql; }
    int qv=ABS(ql);
    if(equhead[pl] == equhead[qv]){
        if(equhead[pl] == 0){
           equhead[pl]=ql; equhead[qv]=qv;
           equlink[qv]=pl;
           equsum++;
           return l_Undef;
        }
        if(ql < 0) return l_False;
        return l_Undef;
    }
    if(equhead[pl] == -equhead[qv]){
        if(ql < 0) return l_Undef;
        return l_False;
    }
    equsum++;
    if(equhead[pl] == 0 && equhead[qv]){
        if(ql < 0) equhead[pl]=-equhead[qv];
        else equhead[pl]=equhead[qv];
        int h=ABS(equhead[pl]);
        int t = equlink[h];
        equlink[h]=pl;
        equlink[pl]=t;
        return l_Undef;
    }
    if(equhead[pl] && equhead[qv]==0){
        if(ql < 0) equhead[qv]=-equhead[pl];
        else equhead[qv]=equhead[pl];
        int h=ABS(equhead[qv]);
        int t = equlink[h];
        equlink[h]=qv;
        equlink[qv]=t;
        return l_Undef;
    }
//merge
    int x=equhead[pl];
    int y=equhead[qv];
    if(ql < 0) y=-y;
    int next=ABS(y);
    while(1){
         if(equhead[next]==y) equhead[next]=x;
         else  equhead[next]=-x;
         if(equlink[next]==0) break;
         next=equlink[next];
    }    
    int xh=ABS(x);
    int t=equlink[xh];
    equlink[xh]=ABS(y);
    equlink[next]=t;
    return l_Undef;
}

void Solver:: buildEquClause()
{   vec<Lit> lits;
    if(equhead==0) return;
    for (int i=1; i <= nVars(); i++){
         if(equhead[i]==0 || equhead[i]==i) continue;
         Lit p=mkLit(i-1);
         int lit=equhead[i];

         Lit q = (lit>0) ? mkLit(lit-1) : ~mkLit(-lit-1);
         lits.clear();
         lits.push(p);
         lits.push(~q);
         simpAddClause(lits);

         lits.clear();
         lits.push(~p);
         lits.push(q);
          simpAddClause(lits);
    }
    free(equhead);
    free(equlink);
    equhead = equlink=0;
}

void Solver:: solveEqu(int *equRepr, vec<lbool> & Amodel)
{   
   if(equRepr==0) return;
   printf("c solveEqu originVars=%d \n",originVars);
   for (int i=1; i <= originVars; i++){
         if(equRepr[i]==0 || equRepr[i]==i) continue;
         int v=equRepr[i];
         v=ABS(v)-1;
         Amodel[i-1] = l_False;
	// printf("e[%d]=%d ",i,equRepr[i]);
         if (Amodel[v] == l_Undef) Amodel[v] = l_True;
         if (Amodel[v] == l_True){
                   if(equRepr[i] > 0) Amodel[i-1] = l_True;
         }
         else      if(equRepr[i] < 0) Amodel[i-1] = l_True;
    }
}

//hyper bin resol
lbool Solver:: simpleprobehbr (Clause & c)
{   vec<int> ostack;
    vec<Lit> lits;
    ostack.clear();
    CRef confl= CRef_Undef;
    int sum=0,cnt=0;
    int maxcount=0;
    lbool ret=l_Undef;

    for (int i =0; i < 3; i++){
        Lit lit=c[i];
        vec<Watcher>&  bin  = watchesBin[lit];
        if(bin.size()) cnt++;
    }
    if(cnt <= 1) goto done;
    for (int i =0; i < 3; i++){
        Lit lit=c[i];
        int litint=toInt(lit);
        sum += litint;
        vec<Watcher>&  bin  = watchesBin[lit];
        for(int k = 0;k<bin.size();k++) {
	      Lit other = bin[k].blocker;
             int oint = toInt(other);
              if(mark[oint]) continue;
              int nother = oint^1;
              if(mark[nother]) {//unit
                    uncheckedEnqueue(~lit);//bug
                    confl=propagate();
                 //   printf("c u=%d <u -A><u A>",toIntLit(~lit));
                    goto done;
              }
              count[oint]++;
              if(maxcount < count[oint]) maxcount = count[oint];
              litsum[oint] += litint;
              mark[oint] =1;
              ostack.push(oint);
       }
       for(int k = 0;k<bin.size();k++) {
 	    Lit other = bin[k].blocker;
            mark[toInt(other)]= 0;
       }
   }
   if(maxcount < 2 ) goto done;
   for (int i =0; i < ostack.size(); i++){
           int oint=ostack[i];
           if(count[oint]<=1) continue;
           Lit other = toLit(oint);
           if(count[oint]==3){//unit
                  if(value(other) != l_Undef) continue; //bug 2016.3.18
                  uncheckedEnqueue(other);
                  confl=propagate();
                  if (confl != CRef_Undef) goto done;
                  continue;
           }
           int litint = sum - litsum[oint];
           Lit lit = toLit(litint);
           if(other == lit) {//unit
                  if(value(other) != l_Undef) continue; 
                  uncheckedEnqueue(other);
                  confl=propagate();
                  if (confl != CRef_Undef) goto done;
                  continue;
           }
           if(other == (~lit)) continue;
           if(hasBin(other, lit)) continue;
           if(hasBin(~other, ~lit)){//other=~lit
                   ret=addequ(other, ~lit);
                   if(ret == l_False) goto done;
           }
           else{
                   touchVar.push(var(other));
                   touchVar.push(var(lit));
           }               
           lits.clear();
           lits.push(other);
           lits.push(lit);

           hbrsum++;
           simpAddClause(lits);
  }              
done:
  for (int i =0; i < ostack.size(); i++){
        int oint=ostack[i];
        count[oint]=litsum[oint]=0;
        mark[oint]=0;
  }     
  if (confl != CRef_Undef) return l_False;
  return ret;
}

lbool Solver::removeEqVar(vec<CRef>& cls, bool learntflag)
{
    vec<Lit> newCls;
    int i, j,k,n;

    for (i = j = 0; i < cls.size(); i++){
        Clause& c = ca[cls[i]];
        newCls.clear();
        int T=0;
        int del=0;
        for (k = 0; k < c.size(); k++){
             Lit p=c[k];
             if (value(p) == l_True) {del=T=1; break; }
             if (value(p) == l_False){ del=1;  continue;}
             int v = var(p)+1;
             Lit q;
             if(equhead[v]==0 || equhead[v]==v) q=p;
             else{ int lit;
                 if(sign(p)) lit = -equhead[v];
                 else        lit = equhead[v];
                 q = (lit>0) ? mkLit(lit-1) : ~mkLit(-lit-1);
                 del=1;
             }
             if(del){
                for(n=newCls.size()-1; n>=0; n--){
                   if( q  == newCls[n]) break;
                   if( ~q == newCls[n]) {T=1; break;}
                }
             }
             else n=-1;
             if(T) break;
             if(n<0) newCls.push(q);
        }
       if(del==0){
            cls[j++] = cls[i];
            continue;
       }
       removeClause(cls[i]);
       if(T)  continue;
       if(newCls.size()<3){
          for (k = 0; k < newCls.size(); k++) touchMark[var(newCls[k])]=1;
       }
       if(newCls.size()==0) return l_False;
       if(newCls.size()==1){
              uncheckedEnqueue(newCls[0]);
              if(propagate() != CRef_Undef) return l_False;
              unitsum++;
         //     printf("c new clause unit %d \n",toIntLit(newCls[0]));
              continue;
       }
        CRef cr = ca.alloc(newCls, learntflag);
        attachClause(cr);
        cls[j++] = cr;
    }
    cls.shrink(i - j);
    checkGarbage();
    return l_Undef;
}

lbool Solver::probeaux()
{    
     int old_equsum = equsum;
     for (int i = 0; i<clauses.size() && i<400000; i++){
           Clause & c = ca[clauses[i]];
 	   int sz=c.size();
           if(sz!=3) continue;
	   int tcnt=0;
           for (int j = 0; j < sz; j++) {
                tcnt += touchMark[var(c[j])];
                if(value(c[j]) != l_Undef) goto next;
           }
           if(tcnt < 2) continue;
           if(simpleprobehbr (c) == l_False) {
              //  printf("c simpleprobehbr unsat \n");
                return l_False;
           }
next:      ;
     }
//lift
    vec<int> liftlit;
    vec<int> unit;
    int liftcnt;
    if(probeSum) liftcnt = nVars()/2;
    else liftcnt=10000;

    for(int vv=0; vv<nVars() && liftcnt > 0; vv++){
	     if(touchMark[vv]==0) continue;
             if(assigns[vv] != l_Undef) continue;
             if(equhead[vv+1] && ABS(equhead[vv+1]) <= vv) continue;

            Lit p = mkLit(vv);
            int pos=watchesBin[p].size();
            int neg=watchesBin[~p].size();

            if(pos==0 || neg==0) continue;
            liftcnt--;
            if(pos < neg) p=~p;
            CRef confl=trypropagate(p);
            if (confl != CRef_Undef){
                    cancelUntil(0);
                    uncheckedEnqueue(~p);
                    confl=propagate();
                    if (confl != CRef_Undef) return l_False;
                  //  printf("c lift u1=%d \n",toIntLit(~p));
                    unitsum++;
                    continue;
            }
        
            setmark(liftlit);
            confl=trypropagate(~p);
            if (confl != CRef_Undef){
                    cancelUntil(0);
                    uncheckedEnqueue(p);
                    propagate();
                    unitsum++;
                //    printf("c u2=%d \n",toIntLit(p));
                    clearmark(liftlit);
                    continue;
            }
            unit.clear();

            for (int c = trail.size()-1; c > trail_lim[0]; c--) {//not contain p
                 int lit=toInt(trail[c]);
                 if(mark[lit]) unit.push(lit);
                 else{
                      if(mark[lit^1]) {//equ p=~lit
                       lbool ret=addequ(~toLit(lit),p);
                       if(ret == l_False) return l_False;
                    }
                 }
            }

            clearmark(liftlit);
            cancelUntil(0);
           
          //  if(unit.size()){
         //       printf("< u.size=%d unitsum=%d ",unit.size(),unitsum);
         //       fflush(stdout);
          //  }
 
            for (int i =0; i < unit.size(); i++){
                  int lit=unit[i];
              	  Lit uLit=toLit(lit);
	          if(value(uLit) == l_Undef){
                     uncheckedEnqueue(uLit);
                     unitsum++;
                     propagate();
                  }
            }
     }
  
   for (int i =0; i < nVars(); i++) {
         touchMark[i]=0;
//new idea
         int exv=i+1;
         if(equhead[exv]==0 || equhead[exv]==exv) continue;
         decision[i]=1;
   }            
 
   for (int i =0; i < touchVar.size(); i++) touchMark[touchVar[i]]=1;

   additional_del();
   lbool ret=removeEqVar(clauses, false);
   if(ret == l_False) return l_False;
   if(old_equsum != equsum){
        ret=removeEqVar(learnts2, true);
        if(ret == l_False) return l_False;
        ret=removeEqVar(corelearnts, true);
   }
   return ret;
}

lbool Solver::probe()
{    
     if(incremental) return l_Undef;
	
     additional_del();
     lbool ret=l_Undef;
     count = (int* ) calloc (2*nVars()+1, sizeof(int));
     litsum = (int* ) calloc (2*nVars()+1, sizeof(int));
     mark = (char * ) calloc (2*nVars()+1, sizeof(char));
     if(equhead == 0) equhead = (int * ) calloc (nVars()+1, sizeof(int));
     if(equlink == 0) equlink = (int * ) calloc (nVars()+1, sizeof(int));
     touchMark = (char *) malloc(sizeof(char) * (nVars()+1));

     for (int i =0; i < nVars(); i++)  touchMark[i]=1;
     int nC=nClauses();
     nC += (nC/2);
     int m=0;
     while(hbrsum<nC){
          m++;
          touchVar.clear();
          int oldhbr=hbrsum;
          int oldequ=equsum;
          int olduni=unitsum;
          ret=probeaux();
          if(ret == l_False) break;
          if(recursiveDepth || probeSum>3 || conflicts>100000 && m>1500) break;        
          if(oldhbr==hbrsum && oldequ==equsum && olduni==unitsum) break;
     }
     if(verbosity>0) printf("c probe hbr=%d equ=%d unit=%d \n", hbrsum,equsum,unitsum);
     touchVar.clear();
     free(count);
     free(litsum);
     free(mark);
     free(touchMark);
     probeSum++;
     return ret;
}

//-------------------------------------------------------------
extern int nVarsB;
extern int sumLit;
void MixBCD(); 

void Solver :: varOrderbyClause()
{  
   for (Var v = 0; v < originVars; v++) varRange[v]=0;    
   for (int i = 0; i < nClausesB-5; i++){
      if(Bclauses[i][2]==0) continue;
      int *plit = Bclauses[i];
      int v=ABS(*plit)-1;
      if(varRange[v]==0){
          varRange[v]=i;
          if(varRange[v]<5) varRange[v]=5;
      }
   }
   for (int i = 0; i < nClausesB-5; i++){
      if(Bclauses[i][2]) continue;
      int *plit = Bclauses[i];
      int v=ABS(*plit)-1;
      if(varRange[v]==0){
          varRange[v]=i;
          if(varRange[v]<5) varRange[v]=5;
      }
   }
}

void Solver :: fast_bcd()
{  
   nVarsB = nVars();
   if( nVarsB < 60 || varRange) return;
   nClausesB=clauses.size();
   sumLit=0;   
   for (int i = 0; i < nClausesB; i++){
      Clause& c = ca[clauses[i]];
      sumLit += c.size() + 1;
   }
   Bclauses = (int**) malloc(sizeof(int*) * (nClausesB+1));
   Bclauses[0] = (int*) malloc (sizeof(int)*sumLit);
   int j; 
   for (int i = 0; i < nClausesB; i++){
      Clause& c = ca[clauses[i]];
      for (j = 0; j < c.size(); j++){
           Bclauses[i][j] = toIntLit(c[j]);
      }
      Bclauses[i][j]=0;
      Bclauses[i+1] = Bclauses[i] +j+1;
  }
  MixBCD();
   varRange = (int* )calloc (nVars(), sizeof(int));
   varOrderbyClause();
}
//---------------------------------------------
void Solver :: verify()
{   int j;
    for (int i = 0; i < clauses.size(); i++){
          Clause& c = ca[clauses[i]];
          for (j = 0; j < c.size(); j++){
                if (modelValue(c[j]) == l_True) break;
          }
          if (j >=c.size()){
            printf("c {");
            for (j = 0; j < c.size(); j++){
               if(sign(c[j])) printf("-");
               printf("%d", var(c[j])+1);
               if(modelValue(c[i]) == l_True) printf(":1 ");
               else printf(":0 ");
            }
            printf(" }\n");
            exit(0);
        }
    }
    printf("c verified by abcdSAT \n");
}

void Solver::removeAllLearnt(vec<CRef>& cs) 
{  
      for (int i =0; i < cs.size(); i++) removeClause(cs[i]);
}

bool parity (unsigned x);
bool Solver :: addxorclause (int *xorlit, int xorlen, int & exVar)
{
 	int xbuf[10];
	int size;
        vec<Lit> lits;

        if(xorlen > 4){
            int n=xorlen/2-1;
            while(n) {n--; newVar();}
        }    
	
        for(int pos=0; pos<xorlen; pos+=size){
	     if(xorlen<=4) size=xorlen;
	     else{
		     if(pos+3>=xorlen) size=xorlen-pos;
		     else size=2;
	      }
	      int xs;
	      for(xs=0;xs<size; xs++) xbuf[xs]=xorlit[pos+xs];
	      if(pos) xbuf[xs++]=exVar++;
              if(size!=xorlen-pos) xbuf[xs++]=-exVar;
	      unsigned pow2=1<<xs;
      	      unsigned int j;
	      for(j=0; j<pow2; j++){ // XOR -> CNF
		    if(parity(j)) continue;
           	    unsigned bits=j;
		    lits.clear();
       		    for(int k=0; k<xs; k++) {
			     int lit=xbuf[k];
			     if(bits & 1) lit=-lit;
			     bits=bits>>1;
                             lits.push( (lit > 0) ? mkLit(lit-1) : ~mkLit(-lit-1));
	  	   }
     		   bool ret=addClause(lits); //close a clause
	           if(ret==false) return ret;
	      }
	}
	return true;
}

typedef struct SPF SPF;
SPF * s_init (void);
void s_release (SPF *);
int s_maxVar (SPF *);
void s_add (SPF *, int lit);
int s_ideref (SPF * spf, int elit);
void  spf_set_ext_terminate(SPF * spf_solver, void* state, int (*termCallback)(void*));
int s_midsimp_solve(SPF * spf);

int Solver :: midsimp_solve()
{    int i,j;

     SPF * spf_solver = s_init ();
     spf_set_ext_terminate(spf_solver, termCallbackState, termCallback);
//assume
     for(i=0; i<assumptions.size(); i++){
          Lit p = assumptions[i];
          if (value(p) == l_True) continue;
          if (value(p) == l_False){
                s_release (spf_solver);
                return UNSAT;//res=20
          }
          s_add (spf_solver, toIntLit(p));
          s_add (spf_solver, 0); //close a clause 
     }
    
     for (i =0; i < clauses.size(); i++){
           Clause & c = ca[clauses[i]];
 	   int sz=c.size();
	   for (j = 0; j < sz; j++) {
		if (value(c[j]) == l_True) break;
		if (value(c[j]) == l_False) continue;
	   } 
	   if(j<sz) continue;
           for (j = 0; j < sz; j++) {
		Lit p=c[j];
                if (value(p) == l_False) continue;
                s_add(spf_solver, toIntLit(p));
	   }
           s_add(spf_solver, 0);
     }

     for (i =0; i < corelearnts.size(); i++){
           Clause & c = ca[corelearnts[i]];
 	   int sz=c.size();
           int k=0;
	   for (j = 0; j < sz; j++) {
		if (value(c[j]) == l_True) break;
		if (value(c[j]) == l_False) continue;
                k++;
	   } 
	   if(j<sz || k>2) continue;
           for (j = 0; j < sz; j++) {
		Lit p=c[j];
                if (value(p) == l_False) continue;
                s_add(spf_solver, toIntLit(p));
	   }
           s_add(spf_solver, 0); //close a clause 
      }
      int res = s_midsimp_solve(spf_solver);
//result:
      if(res==10) {
             model.growTo(nVars());
             for (i = 0; i < nVars(); i++) model[i] = value(i);
   
             int  maxvar = s_maxVar (spf_solver);
             for (i = 1; i <= maxvar; i++) {
                 if(model[i-1]!=l_Undef) continue;
	         if(s_ideref (spf_solver, i) > 0) model[i-1]=l_True;
                 else                             model[i-1]=l_False;
             }
             s_release (spf_solver);
             return SAT;
      }
      s_release (spf_solver);
      if(res==0) return _UNKNOWN;
      return UNSAT;//res=20
}

//---------------------------------------------------------
float *fweight;
inline float Solver::dynamicWeight() //new idea 
{   float w=0.01;
    for (int c = trail.size()-1; c >= trail_lim[0]; c--) w+=fweight[trail[c].x];
    cancelUntil(0);
    return w;
}

void Solver::LitWeight(vec<CRef>& cs) //new idea
{
   int size=2*nVars()+2;
   fweight=(float *) malloc(sizeof(float) *(size));
   int i, j;
   for( i = 0; i <size; i++ ) fweight[i]=0;
   for (i = 0; i < cs.size(); i++){
            Clause& c = ca[cs[i]];
 	    int sz=c.size()-1;
   	    if(sz>80) continue; 
            for (j = 0; j <= sz; j++) fweight[c[j].x]+=size_diff[sz];
    }
}
//----------------------------------------
//look both by an iterative uint propagation
int Solver :: IterativeUnitPropagation(int lit0, int * Queue, float *diff,int *unit,int *pre_unit)
{   int i,ForcedLit=0;
    int lt,var,sp,sp1;
    int rc=_UNKNOWN;
    PPS *pps = g_pps;
    Stack<int> * clause=pps->clause;
    Stack<int> * occCls=pps->occCls;
    Queue[0]=unit[ABS(lit0)]=lit0;
    sp1=0; sp=1;*diff=1;
    while(sp1<sp){
	    lt=Queue[sp1++];
	    int numocc = occCls[-lt];
	    for (i = 0; i < numocc; i++) {
		 int cpos=occCls[-lt][i];
                 int len=(*clause)[cpos];
		 int mark=len & MARKBIT;
                 if(mark!=CNF_CLS) continue;
	   	 len=len>>FLAGBIT;
 		 int *clsp=&(*clause)[cpos];
		 int *lits=clsp+1;
                 clsp+=len;
		 int m=0;
                 for (; lits<clsp; lits++){
			 var=ABS(*lits); 
			 if(unit[var]==*lits) goto nextC;
                      
                         int iVar=var-1;
                         if(assigns[iVar] != l_Undef){
                                if( (*lits > 0) && (assigns[iVar] == l_True)) goto nextC;  
                                if( (*lits < 0) && (assigns[iVar] == l_False)) goto nextC;  
                                continue;
			 }
                         
                         if(unit[var]==0) {m++; ForcedLit=*lits;}
		  }
		  if(m==0) {rc=UNSAT; goto ret;}; //conflict
		  if(m==1) {
                          int fvar=ABS(ForcedLit);
			  Queue[sp++]=unit[fvar]=ForcedLit;
                         
                           if(pre_unit){
                               if(unit[fvar]==pre_unit[fvar]){
	                          int iVar=fvar-1;
                                  if(assigns[iVar] != l_Undef) continue;
                  	          Lit ulit=(unit[fvar] > 0) ? mkLit(iVar) : ~mkLit(iVar);
                                  uncheckedEnqueue(ulit);
                                  CRef confl = propagate();
                                  if (confl != CRef_Undef) {Queue[sp]=0; return UNSAT;} 
                               }
                          }
                        
		  }
		  else{
			 if(m<100) (*diff)+=size_diff[m];
		  }
nextC:          ;
		}
      }
ret:
//     for(i=0; i<sp; i++) unit[ABS(Queue[i])]=0;
     Queue[sp]=0;
     return rc;
}

void setDoubleDataIndex(PPS *pps, bool add_delcls);
void clear_unit(int *Queue,int *unit)
{    
     for(int i=0; Queue[i]; i++) unit[ABS(Queue[i])]=0;
}
extern int *extVarRank,numVarRk;

int Solver :: findmaxVar(int & Mvar)
{   	
    setDoubleDataIndex(g_pps,false);
    if(g_pps->unit){
           free(g_pps->unit);
           g_pps->unit=0;
    } 
    int *Queue0=(int *)calloc (4*g_pps->numVar+4, sizeof (int));
    int *Queue1=Queue0 + g_pps->numVar+1;
    int *unit0=Queue1 + g_pps->numVar+1;
    int *unit1=unit0 + g_pps->numVar+1;
    float max=-1,diff1,diff2;
    Mvar=-1;
    for(int i=0; i<numVarRk && i < 100; i++){
 	     int eVar=extVarRank[i];
	     if(assigns[eVar-1] != l_Undef) continue;
             int rc1=IterativeUnitPropagation(eVar,  Queue0, &diff1, unit0, (int *)0);
	     int rc2=IterativeUnitPropagation(-eVar, Queue1, &diff2, unit1, unit0);
             clear_unit(Queue0,unit0);
             clear_unit(Queue1,unit1);
             if(rc1==UNSAT){
		  if(rc2==UNSAT) {
                        free(Queue0); 
                      //  printf("c i=%d double UNSAT \n",i); 
                        return UNSAT;
                  }
                  uncheckedEnqueue(~mkLit(eVar-1));// ~eVar
             }
	     else{
        	  if(rc2==UNSAT) uncheckedEnqueue(mkLit(eVar-1));//eVar
             }
             CRef confl = propagate();
             if (confl != CRef_Undef) {
                     free(Queue0); 
                    // printf("c propagate UNSAT \n"); 
                     return UNSAT;
             }
             if(rc1==UNSAT || rc2==UNSAT) continue;
	     float diff=diff1*diff2;
	     if(diff>max){
		       max=diff;
		       Mvar=eVar-1;
	      }
	}
	free(Queue0);
	return _UNKNOWN;
}

lbool Solver::dumpClause(vec<CRef>& cs,Solver* fromSolver,Solver* toSolver,int sizelimit) 
{    int i,j;
     vec<Lit> lits;
     for (i =0; i < cs.size(); i++){
                Clause & c = fromSolver->ca[cs[i]];
 		int sz=c.size();
           	if(sz > 6 && sizelimit==2) continue;
		lits.clear();
		for (j = 0; j < sz; j++) {
                        Lit lit=c[j];
			if (toSolver->value(lit) == l_True) break;
   			if (toSolver->value(lit) == l_False) continue;
                        lits.push(lit);	
		}
                if(j<sz) continue;
            	if(sizelimit==2 && lits.size() > 2) continue;
 //add clause   
                if(lits.size()==0) return l_False;
                if(lits.size() == 1){
                       toSolver->uncheckedEnqueue(lits[0]);
                       if(toSolver->propagate() != CRef_Undef ) return l_False;
                }
                else{
                    CRef cr = toSolver->ca.alloc(lits, false);
                    toSolver->clauses.push(cr);
                    toSolver->attachClause(cr);
                }
      }
      return l_Undef;
}

lbool Solver::dumpClause(Solver* fromSolver, Solver* toSolver)
{    
     fromSolver->additional_del();
     toSolver->additional_del();
     for(int i=0; i<nVars(); i++){
	    if(toSolver->assigns[i] != l_Undef) continue;
	    if(fromSolver->assigns[i] != l_Undef){
		       Lit lit = (fromSolver->assigns[i] == l_True) ? mkLit(i) : ~mkLit(i);
                       toSolver->uncheckedEnqueue(lit);
            }
      }
  
     lbool ret=dumpClause(fromSolver->clauses, fromSolver, toSolver, 10000000);
     if(ret == l_False) return l_False; 

     ret=dumpClause(fromSolver->corelearnts, fromSolver, toSolver, 2);
     if(ret == l_False) return l_False; 
     return dumpClause(fromSolver->learnts2, fromSolver, toSolver, 2);
}      

void Solver::copyAssign(Solver* fromSolver)
{
    for(int i=0; i<nVars(); i++){
	    if(assigns[i] != l_Undef) continue;
	    assigns[i]=fromSolver->assigns[i];
    }    
}

// 0 <= var < n
void Solver :: reconstructClause(int var, Solver * & solver1, Solver * & solver2,int NumCls,int **clsPtr)
{  
    solver1->additional_del();
    solver2->additional_del();
    vec<Lit> lits;
    Lit p0=mkLit(var);
    Lit p1=~mkLit(var);
    for(int i=0; i < NumCls;){
         int *pcls=clsPtr[i];
         int len1=*pcls;
	 int mark1=len1 & MARKBIT;
	 len1=len1>>FLAGBIT;
         if(mark1==DELETED) {i++; continue;}
         int share=0;
	 for(i++; i < NumCls; i++){
                int *pcls2=clsPtr[i];
                int len2=*pcls2;
        	int mark2=len2 & MARKBIT;
	        len2=len2>>FLAGBIT;
                if(len1 != len2) break;
                int j;
                for (j=1; j<len1; j++)  if(pcls2[j] != pcls[j]) break;
                if(j < len2 ) break;
                if(mark1==CNF_CLS && mark2==CNF_C_B || mark2==CNF_CLS && mark1==CNF_C_B) share=1;
         }
         lits.clear();
         if(share){
          //      printf("A=B:");
         }
         else{
               if(mark1==CNF_CLS){
                   lits.push(p0);
               }
               else{
                   lits.push(p1);
               }
         }
         for (int j=1; j<len1; j++){
	           int lit=pcls[j];
		   lits.push( (lit > 0) ? mkLit(lit-1) : ~mkLit(-lit-1));
         }
         simpAddClause(lits);
     }
}

lbool Solver :: run_subsolver(Solver* & newsolver, Lit plit, int conf_limit,int fullsolve)
{
        int nV=nVars();
        newsolver=new Solver();
        while (nV > newsolver->nVars()) newsolver->newVar();
        newsolver->recursiveDepth=recursiveDepth+1;
       
        for(int i=0; i<nV; i++){
	      if(assigns[i] != l_Undef){
		       Lit lt = (assigns[i] == l_True) ? mkLit(i) : ~mkLit(i);
                       newsolver->uncheckedEnqueue(lt);
               }
        }
        if(value(var(plit)) == l_Undef) newsolver->uncheckedEnqueue(plit);
       
        lbool ret=dumpClause(clauses, this, newsolver, 10000000);
        if(ret == l_False) return ret; 
        ret=dumpClause(corelearnts, this, newsolver, 2);
        if(ret == l_False) return ret; 
        if(newsolver->propagate() != CRef_Undef ) return l_False;
        newsolver->conflict_limit=conf_limit;
        newsolver->completesolve=fullsolve;
       
        newsolver->setTermCallback(termCallbackState, termCallback);

        ret=newsolver->solveNoAssump();
        return ret;
}      

void Solver :: ClearClause(vec<CRef>& cs)
{
    for (int i =0; i < cs.size(); i++) removeClause(cs[i]);
    cs.clear();
}

void Solver :: ClearbigClause(vec<CRef>& cs)
{   int i,j=0;
    for (i=0; i < cs.size(); i++){
           Clause & cl = ca[cs[i]];
           if( cl.size() > 2 ) removeClause(cs[i]);
           else cs[j++] = cs[i];
    }
    cs.shrink(i - j);
}

int Solver::Out_oneClause(Solver* solver,vec<CRef>& cs,Stack<int> * outClause, int lenLimit,int clause_type) 
{       int i,j,num=0;
	for (i =0; i < cs.size(); i++){
                Clause & c = solver->ca[cs[i]];
 		int sz=c.size();
	   	if(lenLimit==2 && sz>6) continue;
		int k=0;
		for (j = 0; j < sz; j++) {
			if (solver->value(c[j]) == l_True) break;
			if (solver->value(c[j]) == l_False) continue;
			k++;
		}
		if(j<sz) continue;
         	if(lenLimit==2 && k>2) continue;
		outClause->push(((k+1)<<FLAGBIT) | clause_type);
          	for (j = 0; j < sz; j++) {
			Lit p=c[j];
                        if (solver->value(p) == l_False) continue;
	                outClause->push(toIntLit(p));
         	}
                num++;
        }
        return num;
}

int Solver :: Output_All_Clause(Solver* solver)
{   int clause_type;
    if(g_pps->clause==0) {
            clause_type=CNF_CLS; 
            g_pps->clause=new Stack<int>;
    }
    else clause_type=CNF_C_B; 

    int Numclause = Out_oneClause(solver,solver->clauses, g_pps->clause, 1000000, clause_type);
    Numclause += Out_oneClause(solver,solver->corelearnts, g_pps->clause, 2,       clause_type);
    return Numclause;
}

CRef Solver :: copyTwoAssign(int var, Solver * & solver1, Solver * & solver2)
{   
    vec<Lit> lits;
    Lit p0=mkLit(var);
    Lit p1=~mkLit(var);
    for(int i=0; i<nVars(); i++){
	    if(assigns[i] != l_Undef) continue;
	    if(solver1->assigns[i]==solver2->assigns[i]){
                     if(solver1->assigns[i] != l_Undef){
                              Lit lit = (solver1->assigns[i] == l_True) ? mkLit(i) : ~mkLit(i);
                              uncheckedEnqueue(lit);
                              CRef confl=propagate();
                              if (confl != CRef_Undef){
                                     delete solver1;
                                     delete solver2;
                                     return confl;
                              }
                      }
                      continue;
             }
    }
    CRef confl=trypropagate(p1);
    if (confl != CRef_Undef){
            delete solver1;
            delete solver2;
            cancelUntil(0);
            uncheckedEnqueue(p0);
            return propagate();
    }       
    for(int i=0; i<nVars(); i++){
	    if(assigns[i] != l_Undef) continue;
	    if(i == var) continue;
            if(solver1->assigns[i] != l_Undef){
                      Lit lit = (solver1->assigns[i] == l_True) ? mkLit(i) : ~mkLit(i);
                      lits.clear();
                      lits.push(p0);
                      lits.push(lit);
                      simpAddClause(lits);
             }
    }    	
    delete solver1;
    cancelUntil(0);
    confl=trypropagate(p0);
    if (confl != CRef_Undef){
            delete solver2;
            cancelUntil(0);
            uncheckedEnqueue(p1);
            return propagate();
    }       
    for(int i=0; i<nVars(); i++){
	    if(assigns[i] != l_Undef) continue;
	    if(i == var) continue;
            if(solver2->assigns[i] != l_Undef){
                      Lit lit = (solver2->assigns[i] == l_True) ? mkLit(i) : ~mkLit(i);
                      lits.clear();
                      lits.push(p1);
                      lits.push(lit);
                      simpAddClause(lits);
             }
    }
    cancelUntil(0);
    delete solver2;
    return CRef_Undef;
}

void XOR_Preprocess(PPS *pps, int way);
void SortLiteral(Stack<int> * clause);
void sortClause(PPS *pps,int **clsPtr);
void release_pps (PPS * pps);

void init_weight()
{
	if(size_diff) return;
	int i, longest_clause = 100; 
        size_diff = (float *) malloc(sizeof(float) * longest_clause );
	size_diff[0]=size_diff[1] = 0.0;
	for( i = 2; i < longest_clause; i++ )  size_diff[ i ] = (float)pow(5.0, 2.0-i);
}

lbool Solver :: subsolve()
{  
        cancelUntil(0);
     
        if(size_diff==0) init_weight();

        additional_del();
        ClearClause(learnts2);
           
        g_pps->numVar=nVars();
	OutputClause(g_pps->clause, (int *)0, g_pps->numClause,0);

        int way=1;
        XOR_Preprocess(g_pps,way);

        int Mvar0;
        int rc0=findmaxVar(Mvar0);
        release_pps (g_pps);
        if(rc0 == UNSAT) return l_False;
     
        lbool ret;
        int mvar;
        float maxWeight;
        if(Mvar0 >=0 ){
             mvar=Mvar0;
             goto done;
        }
retry:	
        ret=l_Undef;
        mvar=-1;
        maxWeight=-1;
        LitWeight(clauses);
        for(int i=0; i<numVarRk; i++){
	      int var=extVarRank[i]-1;
	      if(assigns[var] != l_Undef) continue;
         
              Lit p0=~mkLit(var);
              Lit p1=mkLit(var);
              CRef confl0=trypropagate(p0);
              float w0=dynamicWeight();
              CRef confl1=trypropagate(p1);
              float w1=dynamicWeight();
              if (confl0 != CRef_Undef){
                    if (confl1 != CRef_Undef) {ret=l_False; break;}
                    uncheckedEnqueue(p1);
                    propagate();
                    continue;
              }
              if (confl1 != CRef_Undef){
                    uncheckedEnqueue(p0);
                    propagate();
                    continue;
              }
              if(maxWeight < w0*w1 && i<15 || mvar==-1){//15
                     maxWeight = w0*w1;
                     mvar=var;
              }
              if(i>30) break;//30
        }
        free(fweight); 
        if(ret==l_False) return l_False;
        if(mvar==-1) return l_True;
done:
        if(assigns[mvar] != l_Undef) goto retry;
        
        additional_del();
        Solver* subsolver1;
        lbool ret1=run_subsolver(subsolver1, ~mkLit(mvar),-1,completesolve);
        if(ret1==l_True) {
                  copyAssign(subsolver1);
                  delete subsolver1;
                  return l_True;
        }
        if(cancel_subsolve){
                  delete subsolver1;
                  return l_Undef;
        }
        Solver* subsolver2;
        lbool ret2=run_subsolver(subsolver2, mkLit(mvar),-1,completesolve);
        if(ret2==l_True){
               copyAssign(subsolver2);//bug
               delete subsolver1;
               delete subsolver2;
               return l_True;
        }
        int clearflag=0;
        if(!cancel_subsolve){
              ClearbigClause(clauses);
              ClearClause(learnts2);
              ClearbigClause(corelearnts);
              garbageCollect();
              clearflag=1;
       }
       if(recursiveDepth != 2){
               if(ret1==l_Undef && ret2==l_Undef && cancel_subsolve==0){
                         subsolver1->completesolve=1;
                         ret1=subsolver1->solve2_();
                         if(ret1==l_True){
                             copyAssign(subsolver1);
                             delete subsolver1;
                             delete subsolver2;
                             return l_True;
                         }
                }
        }
        
        if(ret1==l_False){
               if(ret2==l_False) {
                       delete subsolver1;
                       delete subsolver2;
                       return l_False;
               }
               if(!clearflag){
                    ClearClause(clauses);
                    ClearClause(learnts2);
                    ClearClause(corelearnts);
                    garbageCollect();
               }

               ret2=dumpClause(subsolver2, this);//one side copy
               delete subsolver1;
               delete subsolver2;
               if(ret2==l_False) return l_False;
               return l_Undef;
       }
       if(ret2==l_False){
              if(!clearflag){
                    ClearClause(clauses);
                    ClearClause(learnts2);
                    ClearClause(corelearnts);
                    garbageCollect();
              }
              ret1=dumpClause(subsolver1, this);
              delete subsolver1;
              delete subsolver2;
              if(ret1==l_False) return l_False;
              return l_Undef;
        }
        if(!clearflag){
               CRef confl=copyTwoAssign(mvar, subsolver1, subsolver2);
               if (confl != CRef_Undef) return l_False;
               return l_Undef;
        }

     //   printf("c reconstructClause \n");
     //   fflush(stdout);
     
        g_pps->numClause =  Output_All_Clause(subsolver1);
        g_pps->numClause += Output_All_Clause(subsolver2);
        SortLiteral(g_pps->clause);
        int **clsPos=(int **) malloc(sizeof(int *)*(g_pps->numClause));
        sortClause(g_pps,clsPos);
        reconstructClause(mvar, subsolver1, subsolver2, g_pps->numClause,clsPos);
        free(clsPos);
        delete g_pps->clause;
        g_pps->clause=0;

        CRef confl=copyTwoAssign(mvar, subsolver1, subsolver2);
        if (confl != CRef_Undef) return l_False;
        conflict.clear();   
        return l_Undef;
}
void Solver::Out_removeClause(vec<CRef>& cs,Stack<int> * & outClause, int & numCls,int lenLimit,int Del) //new idea
{       int i,j,m=0;
	for (i =0; i < cs.size(); i++){
                Clause & c = ca[cs[i]];
 		int sz=c.size();
		int k=0;
		for (j = 0; j < sz; j++) {
			if (value(c[j]) == l_True) break;
			if(g_pps->unit){
                             int vv=var(c[j])+1;
		             if((vv+1)==g_pps->unit[vv]) break;
                        }
			if (value(c[j]) == l_False) continue;
			k++;
		}
		if(j<sz){
			 if(Del) removeClause(cs[i]);
			 continue;
		}
         	if(lenLimit==2 && k>2){
                        if(Del) cs[m++] = cs[i];
  			continue;
		}
		outClause->push(((k+1)<<FLAGBIT) | CNF_CLS);
          	for (j = 0; j < sz; j++) {
			Lit p=c[j];
                        if (value(p) == l_False) continue;
	                outClause->push(toIntLit(p));
         	}
	        if(Del) removeClause(cs[i]);
                numCls++;
	}
        if(Del) cs.shrink(i - m);
}

void Solver :: OutputClause(Stack<int> * & clauseCNF, int *solution, int & numCls,int Del)
{
    if(clauseCNF) delete clauseCNF;
    cancelUntil(0);
	
    if(solution){
       	 for (int i = nVars()-1; i >=0 ; i--){
		 if(solution[i+1]) continue;
		 if (assigns[i] != l_Undef){
                        if(assigns[i]==l_True) solution[i+1]=i+1;
			else                   solution[i+1]=-(i+1);
		 }
	 }
    }

    clauseCNF=new Stack<int>; 
    numCls=0;

    Out_removeClause(clauses,     clauseCNF, numCls,10000000,Del); //new idea
    Out_removeClause(corelearnts, clauseCNF, numCls,2,Del);
    Out_removeClause(learnts2,    clauseCNF, numCls,2,Del);
}

//----------------------------------------
int Solver :: longClauses()
{  int cnt=0;
   for (int i = clauses.size()-1; i >= 0; i--){
      Clause& c = ca[clauses[i]];
      if(c.size()>2) cnt++;
   }
   return cnt;
}
//---------------------------
void Solver::deepropagate(vec<int> & deep, vec<int> & prvPos, Lit q)
{
    newDecisionLevel();
    uncheckedEnqueue(q);

    watches.cleanAll();
    watchesBin.cleanAll();
    int dhead=0;
    deep.push(0);
    prvPos.push(-1);
    while (qhead < trail.size()){
        Lit            p   = trail[qhead++];   
        int          depth = deep[dhead++]+1;
      	vec<Watcher>&  wbin  = watchesBin[p];
	for(int k = 0;k<wbin.size();k++) {
	  Lit imp = wbin[k].blocker;
	  if(value(imp) == l_Undef) {
	    uncheckedEnqueue(imp,wbin[k].cref);
            deep.push(depth);
            prvPos.push(qhead-1);
	  }
	}
    
        vec<Watcher>&  ws  = watches[p];
        Watcher        *i, *end;
        for (i = (Watcher*)ws, end = i + ws.size();  i != end; i++){
            CRef     cr        = i->cref;
            Clause&  c         = ca[cr];
            int j=0;    
            Lit lit;
            for (int k = 0; k < c.size(); k++){
                  if(value(c[k]) == l_True) goto NextClause;
                  if(value(c[k]) == l_False) continue;
                  if(j) goto NextClause;
                  j++;
                  lit=c[k];
            }
            if(j){
                  uncheckedEnqueue(lit, cr);
                  deep.push(depth);
                  prvPos.push(qhead-1);
	    }
            NextClause:;
        }
    }
}


