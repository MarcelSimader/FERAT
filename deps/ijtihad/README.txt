Ijtihad is a solver for Quantified Boolean Formulas (QBFs). The solver tackles 
the a formula iteratively, using counterexample-guided expansion. 

////////////////////////////////////Notation////////////////////////////////////

In the following description, α will denote a full assignment to the universal
variables U of a QBF Π.φ formula with prefix Π and matrix φ. Similarly, σ will
denote a full assignment of the existential variables E. The are therefore 
functions mapping variables from U (respectively E) to the the set {T, ⊥}. Any
α is part of the set ΣA, which contains all possible full assignments. Same 
holds for σ and ΣS.

Since ijtihad is an expansion based solver, we will also need a notation for a
reduction of formula φ by a full assignement like α, which we will write as φ_α. 
When reducing φ by α, we mean replacing the variables U with the values they are 
mapped to and performing simple logic rules to the resulting formula. At the same 
time, all variables in E are replaced with annotated versions E^α. This means 
that each variable x in E is annotated with values α(y) for all y in U that 
appear before x in the prefix Π. When we want to get the original variables E 
from E^α, we will write that as (E^α)^-α.

During the solving of the QBF we will need to call the sat solver for formulas
that are concatenations of different φ_α, where α is in some set. For the set A,
we would write AND_{α∈A}(φ_α) to denote the conjunction of φ_α for all α in A.
Similarly we would write OR_{σ∈S}(φ_σ) to denote the disjunction of φ_σ for all
σ in S.

The results of such calls to a sat solver will usually be assignments like
τ : E^α ∪ E^α' ∪ E^α" ... → {T, ⊥}. We will use the notation τ|E^α to mean the
domain restriction of τ to only variables in E^α. This will in turn be the same
as σ : E → {T, ⊥}, with the difference that the variables are annotated. The 
annotations can be removed with the same notation as before, so 
(τ|E^α)^-α : E → {T, ⊥}.

///////////////////////////////////Algorithm////////////////////////////////////

The pseudo-code below summarises the basic idea of our approach for solving the 
QBF Π.φ with universal variables U and existential variables E. First, an 
arbitrary assignment α for the universal variables is selected in Line 1. The 
instantiation φ_α is handed over to a SAT solver. If φ α is unsatisfiable,
denoted by τ = ∅ in Line 5, then Π.φ is false and the algorithm returns. If
φ_α is satisfiable, then τ : E^α → {T, ⊥} is a satisfying assignment of φ_α . 
Let σ = τ^−α . Then ασ is a satisfying assignment for φ. Next, the propositional 
formula ¬φ_σ is handed over to a SAT solver for checking the validity of φ_σ . 
If ¬φ_σ is unsatisfiable, then Π.φ is true and the algorithm returns. If ¬φ_σ is 
satisfiable, then ρ : U^σ → {T, ⊥} is a satisfying assignment of ¬φ_σ . 
Let α' = ρ^−σ . Then α'σ is a satisfying assignment for ¬φ.

In the next round of the algorithm, the propositional formula φ_α ∧ φ_α' is
handed over to a SAT solver. If this formula is satisfiable under some 
assignment τ : E^α ∪ E^α' → {T, ⊥}, then a new assignment σ' : E → {T, ⊥} is 
obtained. Note that σ =/= σ'. This assignment is then used for obtaining a new 
propositional formula φ_σ ∨ φ_σ'. To show the validity of this formula, its 
negation is passed to a SAT solver. The formula is satisfiable under the 
assignment ρ : U^σ ∪ U^σ → {T, ⊥}, from which α" : U → {T, ⊥} is obtained. This 
assignment is then used in the next round of the algorithm. In this way, the 
propositional formulas AND_{α∈ΣU}(φ_α) and OR_{σ∈ΣA}(φ_σ) are generated. If
AND_{α∈A}(φ_α) is satisfiable for some A ⊆ ΣU , then Π.φ is false. Dually, if 
OR_{σ∈S}(φ_σ) is valid for some S ⊆ ΣE , Π.φ is true. The algorithm iteratively 
extends the sets A and S by adding parts of satisfying assignments of φ to S and 
parts of falsifying assignments to A. In particular, A is extended by assignments 
of the universal variables and S is extended by assignments of the existential 
variables. The order in which assignments are considered is nondeterministic, 
because they are given by an arbitrary SAT solver. In the following we argue that 
the algorithm terminates.

///////////////////////////////////Pseudocode///////////////////////////////////
input : QBF Π.φ with universal variables U and existential variables E
output: truth value of Π.φ
01.  A := {α}, where α : U → {T, ⊥} is an arbitrary assignment
02.  S := ∅
03.  while true do
04.      τ := SAT( AND_{α∈A}(φ_α) )
05.      if τ = ∅ then return false;
06.      S := S ∪ {(τ|E^α)^−α | α ∈ A}
07.      ρ := SAT( AND_{σ∈S}(¬φ_σ) )
08.      if ρ = ∅ then return true;
09.      A := A ∪ {(ρ|U^σ)^−σ | σ ∈ S}
10.  end
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////Options/////////////////////////////////////

The solver supports several options. First of all, compilation determines which
SAT solver will be used in the backend, and can dramatically influence the 
performance of the ijtihad. In our experience Ijtihad works best with the SAT
solver glucose [3], so we have configured the standard make target to use it
in the backend. Other supported solvers are cryptominisat, abcdsat, lingeling, 
and picosat. The solver that is linked can be chosen by 
setting the flag BACKEND, when calling make. For example, compiling and linking 
with cryptominisat can be done with:

make BACKEND=cryptominisat  

DepQBF is also included as a library and used for calculating dependencies
between existential and universal variables. The usage of dependencies can be
turned on with one of the runtime command line options.

The solver is usually run with a preprocessor like bloqqer, which simplifies
the formula and makes it easier to solve. In the competitive evaluations, 
ijtihad is usually ran with bloqqer as a preprocessor. However, it can be used
with other preprocessors as well.

Some available runtime options are:

--reorder               Indicates whether variables should be reordered inside a 
                        quantifier. Can sometimes be useful for random QBF.

--variable_elimination  Indicates whether variable elimination through unit
                        propagation should be applied. Can sometimes reduce the
                        space consumption of the solver.

--parallel              Indicates whether the sat solvers producing witnesses 
                        and counterexamples should be run in parallel. Doing
                        this can make the solver faster in cases where proving
                        and disproving the QBF is equally as hard. In our 
                        experience verifying is much harder and in most cases
                        takes as much as 95% of the solving time. Expanding at
                        the same time does however reduce the overhead a bit.

--deduplication         Indicates whether clauses that are generated multiple    
                        times should be only added once. This can in some cases  
                        lead to a reduced strain on the internally used sat      
                        solver, but can also result in a signifficant increase 
                        in space consumption for rapidly expanding QBF.

--dependencies          Indicates whether dependencies provided by DepQBF should
                        be used to reduce the search space. Improves the number
                        of solved test cases in many industrial QBF families.

--dep_timeout=<double>  Used for setting the timeout given to the dependency
                        analysis by DepQBF. When the timeout is exceeded the
                        solver returns to the standard behaviour and ignores
                        any dependency information generated up to that point.

--cex_per_call=<int>    Used for setting the maximum number of counterexamples
                        used for expanding the formula for the verifier. Giving
                        a negative number indicates that all counterexamples
                        generated in one call should be used for the expansion.

--wit_per_call=<int>    Used for setting the maximum number of witnesses used 
                        for expanding the formula for the refuter. Giving a 
                        negative number indicates that all witnesses generated 
                        in one call should be used for the expansion.

--cex_order=<int>       Used for determining the order in which the counter-
                        examples are used for the expansion of the verifier's
                        formula. Can impact the route the solver takes when
                        solving a QBF.
                        1   indicates that the counterexample from the oldest 
                            expansion in the refuter should be used first
                        -1  indicates that the counterexample from the newest 
                            expansion in the refuter should be used first

--wit_order=<int>       Used for determining the order in which the witnesses
                        are used for the expansion of the refuter's formula. Can
                        impact the route the solver takes when solving a QBF.
                        1   indicates that the witness from the oldest expansion 
                            in the verifier should be used first
                        -1  indicates that the witness from the newest expansion 
                            in the verifier should be used first

The solver is called in this format:

./<SOLVER> <OPTIONS> <FILENAME>

--------------------------------------------------------------------------------
