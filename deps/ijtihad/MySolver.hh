//
// This file is part of the ijtihad QBF solver
// Copyright (c) 2016, Graz University of Technology
// Authored by Vedad Hadzic
//

/* Editor: Marcel Simader (marcel.simader@jku.at) */
/* Modified version of `Ijtihad` as part of the `FERAT` toolchain.  */
/* Date: 29.09.2024 */

#ifndef MYSOLVER_H
#define MYSOLVER_H

#include "LitTuple.hh"
#include "SolverOptions.hh"
#include "qtypes.hh"

class QuantifiedFormula;
class QDPLL;

class MySolver {
   public:
    /**
     * Takes the prefix and cnf of the input QBF formula. After adjustments like
     * reordering, renaming, enforcement of Kleine-Bünning forall reduction and
     * dependency calculations, the prefix and cnf are saved in a form that is
     * usable by the solver later on. What optimisations are carried out is
     * determined by the given options, which can drastically change the
     * behaviour of the solver itself.
     * @param pref the prefix of the QBF.
     * @param cnf the CNF part of the QBF.
     * @param options options which control how the solver works and which
     * optimisations are used.
     * @see SolverOptions
     */
    MySolver(Prefix& pref, const CNF& cnf, const SolverOptions* options = NULL);

    /**
     * Calls for the destruction of the speciffic formula solver pointers.
     */
    ~MySolver();

    /**
     * Main solving method which implements the core idea of counter example
     * guided expansion used in ijtihad. The two used sat solvers are called
     * succesively and their results are used to expand the opposing solvers
     * formula. This is done untill one of the solvers returns UNSAT for their
     * formula, at which point the method returns. There are two possible modes
     * of operation implemented, one of which is chosen depending on the value
     * of options_.parallel.
     * @see SolverOptions
     * @return true if the given QBF is satisfiable, false otherwise
     */
    bool solve();

    /**
     * Prints information about the number of added variable layers for each
     * formula, as well as the time taken for the solving of each formula. The
     * total time used by the method solve() is also reported. Should aonly be
     * called after solve() was called.
     */
    void printStats();

    /**
     * Method which creates a new variable to be used in the formula PHI.
     * @return a fresh variable in formula PHI
     */
    inline Var newVarPHI() {
        num_vars_phi_++;
        return num_vars_phi_;
    }

    /**
     * Method which creates a new variable to be used in the formula KSI.
     * @return a fresh variable in formula KSI
     */
    inline Var newVarKSI() {
        num_vars_ksi_++;
        return num_vars_ksi_;
    }

   private:
    Var max_id_;  ///< maximum variable id occuring in the internally used QBF
                  ///< representation
    SolverOptions options_;  ///< stores options which determine the behaviour
                             ///< of the solver

    void* sat_solver_phi_;  ///< pointer to the sat solver used for PHI
    void* sat_solver_ksi_;  ///< pointer to the sat solver used for KSI
    // Marcel: I added this, so that we can still export PHI traces for
    //         trivially correct formulas.
    void* sat_solver_trivial_;  ///< pointer to the sat solver used for initial
                                ///< trivial check
    QDPLL* depqbf_;  ///< pointer to the depqbf solver used for dependencies and
                     ///< heretic mode

    vector<LitTuple> wit_to_add_;  ///< temporary storage for witnesses upon
                                   ///< which KSI will be expanded
    vector<LitTuple> cex_to_add_;  ///< temporary storage for counterexamples
                                   ///< upon which KSI will be expanded

    vector<Lit> current_assumptions_;  ///< represents the current interesting
                                       ///< top level existential assignment

    LitTuple current_top_ksi_;
    vector<Var> top_ksi_base_;

    int sat_calls_;          ///< number of calls to each sat solver
    unsigned num_vars_phi_;  ///< number of currently used variables in the
                             ///< formula PHI
    unsigned num_vars_ksi_;  ///< number of currently used variables in the
                             ///< formula KSI

    unsigned num_clauses_phi_;
    unsigned num_clauses_ksi_;

    std::vector<unsigned> origins_phi_;
    std::vector<unsigned> origins_ksi_;

    LitTuple2VariableVector
        vars_cache_phi_; /**< hashmap which stores the labels for each newly
                            introduced variable in PHI. Each label is an
                            assignent to the universial variables in the counter
                            example over which PHI was expanded. The labels are
                            mapped to a vector of variables which were
                            introduced by the expansion. */

    LitTuple2VariableVector
        vars_cache_ksi_; /**< hashmap which stores the labels for each newly
                            introduced variable in PHI. Each label is an
                            assignent to the existential variables in the
                              counter example over which KSI was expanded. The
                            labels are mapped to a vector of variables which
                            were introduced by the expansion. */

    LitTuple2Lit tseitin_cache_;  ///< hashmap storing which Tseitin variable
                                  ///< was used for which clause
    vector<vector<Var>*>
        layer_caluese_bases_;  ///< vector storing all variable vectors used as
                               ///< bases for tseitin_cache_

    LitTuple current_top_phi_;
    vector<Var> independent_phi_; /**< all independent variables in PHI. If
                                     dependencies provided by depqbf are used,
                                     this can include variables other than the
                                     ones in the topmost quantifier. */

    vector<Var>
        eliminated_vars_phi_;  ///< all variables that were eliminated from the
                               ///< formula PHI, used only for debug.
    vector<Var>
        eliminated_vars_ksi_;  ///< all variables that were eliminated from the
                               ///< formula KSI, used only for debug.

    vector<bool>
        elimination_status_phi_;  ///< stores whether a given variable was
                                  ///< eliminated in the formula PHI
    vector<bool>
        elimination_status_ksi_;  ///< stores whether a given variable was
                                  ///< eliminated in the formula KSI
    vector<bool>
        elimination_value_phi_;  ///< if a variable was eliminated from PHI, its
                                 ///< assigned value is stored here
    vector<bool>
        elimination_value_ksi_;  ///< if a variable was eliminated from KSI, its
                                 ///< assigned value is stored here

    LitTupleSet witness_cache_;  ///< contains all previously encountered and
                                 ///< expanded upon solutions to PHI
    LitTupleSet counterexample_cache_;  ///< contains all previously encountered
                                        ///< and expanded upon solutions to KSI

    QuantifiedFormula*
        formula_;  ///< internal representation of the given QBF formula

    vector<Var> names_phi_;  ///< stores the new name of a variable at the
                             ///< position of old name for existentials
    vector<Var> names_ksi_;  ///< stores the new name of a variable at the
                             ///< position of old name for universials
    vector<Var>
        renaming_scheme_;  ///< inverse to names_phi_ and names_ksi_ together

    vector<vector<Var>>
        layers_phi_;  ///< vector containing layers of variables in PHI which
                      ///< were introduced by expansion
    vector<vector<Var>>
        layers_ksi_;  ///< vector containing layers of variables in KSI which
                      ///< were introduced by expansion
    vector<Var> empty_layer_phi_;  ///< used to avoid constant allocating and
                                   ///< deallocating when adding layers into PHI
    vector<Var> empty_layer_ksi_;  ///< used to avoid constant allocating and
                                   ///< deallocating when adding layers into KSI

    vector<Var> tuple_base_phi_;
    vector<Var> tuple_base_ksi_;

    unsigned int layer_size_phi_;  ///< size of each entry in layers_phi_
    unsigned int layer_size_ksi_;  ///< size of each entry in layers_ksi_

    Occurrence pure_phi_;
    Occurrence pure_ksi_;

    std::ofstream tmp_phi_clauses_;
    std::ofstream tmp_ksi_clauses_;

    std::ofstream phi_log_;
    std::ofstream ksi_log_;

    double time_phi_;  ///< time consumed while calling the sat solver for PHI
    double time_ksi_;  ///< time consumed while calling the sat solver for KSI
    double time_;      ///< total time consumed by method solve()

    long unsigned int current_luby_counter_;
    long unsigned int luby_u_;
    long unsigned int luby_v_;

    /**
     * Prints the version of the solver to stdout.
     */
    void printVersion();

    /**
     * Expands the formula KSI over the witnesses in wit_to_add_. More details
     * on that found in the ijtihad paper and implementation in MySolver.cc.
     */
    void extendKSI();

    /**
     * Expands the formula PHI over the witnesses in cex_to_add_. More details
     * on that found in the ijtihad paper and implementation in MySolver.cc.
     */
    void extendPHI();

    /**
     * Updates the values MySolver::luby_u_ and MySolver::luby_v_ according to
     * the reluctant doubling production.
     */
    void updateLuby();

    /**
     * Method which checks whether the fiven variable is eliminated in the given
     * formula. If it was eliminated, the assigned value is stored in
     * elimination_value, and true is returned. Calling this method only make
     * sense if options_.variable_elimination is set to true.
     * @param type specifies for which formula the elimination value should be
     * checked
     * @param v variable, whose elimination status in the given formula should
     * be checked
     * @param elimination_value the assigned value to the eliminated variable
     * @return true if the variable was eliminated before, false if it was not
     */
    bool isEliminated(FormulaType type, Var v, bool& elimination_value);

    /**
     * Marks a given variable eliminated in the specified formula, with an
     * assigned value.
     * @param type formula in which the variable is to be eliminated
     * @param v the variable which should be eliminated in the formula
     * @param value the value assigned to the variable upon elimination
     */
    void eliminateVar(FormulaType type, Var v, bool value);

    /**
     * Performs unit clause propagation in a given set of clauses for the
     * specified formula. The resulting clause set is stored in the same clause
     * vector.
     * @param type formula which will be expanded by the clauses
     * @param clauses vector of clauses in which the unit propagation is
     * performed
     */
    void performVariableElimination(FormulaType type,
                                    vector<vector<Lit>*>& clauses);

    /**
     * Adds a clause into a specified formulas sat solver
     * @param solver the solver for the given formula
     * @param clause the clause to be added
     */
    void addClause(void* solver, const vector<Lit>& clause);

    /**
     * Adds an assumption vector into a specified formulas sat solver
     * @param solver the solver for the given formula
     * @param clause the assumption to be made
     */
    void addAssumption(void* solver, vector<Lit>& assumption);

    /**
     * Reorders all variables according to theri appearance in the original
     * input formula. This is optionally called in the constructor if
     * options_.reorder is set to true.
     * @param pref prefix of the input QBF formula
     * @param cnf cnf of the input QBF formula
     */
    void reorderQuantifiersInternally(Prefix& pref, const CNF& cnf);

    /**
     * Renames variables in the input QBF in sucha a way that existentially
     * quantified variabes are uneven, and universally quantified variables are
     * even. This is done to reduce the ammount of useless variables in both
     * formulas, as well as avoid unnecessary dereferencing when replacement
     * variables are looked up in the layer vector.
     * @param pref prefix of the input QBF formula
     * @param cnf cnf of the input QBF formula
     */
    void renameVariables(Prefix& pref, const CNF& cnf);

    /**
     * Method for calculating dependencies among the variables. DepQBF is called
     * as a library in order to obtain a simple dependency scheme. The simple
     * dependency structure is then merged for efficiency reasons, and stored in
     * deps_phi_ and deps_ksi_ respectively. Note here that we use the inverse
     * notation to what DepQBF uses, because we need it in that format. Also
     * note that this is only called if the flag options_.dependencies is set to
     * true. A configurable timeout keeps the dependency calculation from taking
     * to long, in order to avoid substantial performance loss.
     */
    void retrieveDependencies();

    /**
     * Merges the simple dependency structures in retrieveDependencies for the
     * given formula. It basically creates a set of variables which depend upon
     * another set of variables by successively creating unions from variables
     * which depend upon the same variables in the other quantifier type.
     * @param type formula for which the simple dependencies should be merged
     * @param source vector of simple dependencies by index
     */
    void mergeDependencies(FormulaType type, const vector<vector<Var>>& source);

    /**
     * Method for computing the witnesses and counter examples upon which a
     * given fromula will be expanded. This is done by going through all layers
     * of expansion variables and looking whether their assignment was ever
     * encountered before. If it was not, they are saved in the wit_to_add_ or
     * cex_to_add_ respectively. This is done until a configurable desired
     * number of witnesses or counter examples is reached or all layers were
     * checked.
     * @param type formula for whose extension to calculate the counter examples
     * / witnesses
     */
    void computeExtension(FormulaType type);

    /**
     * Try to find a meaningful first extension for PHI. Doing this will
     * hopefully push the solver in a better direction at the beginning instead
     * of cluelessly starting anywhere in the searchspace.
     */
    int initPHI();

    /**
     * Instantiates MySolver::depqbf_ with the QBF formula in MySolver::formula_
     */
    void instantiateDepQBF();

    /**
     * Learns clauses from DepQBF, starting from clause with index first up to
     * clause with index last
     * @param first index of first clause to be learned
     * @param last index of last clause to be learned
     */
    void learnClausesFromDepQBF(unsigned first, unsigned last);

    void generateDimacsPHI();
    void generateDimacsKSI();

    /**
     * Removes parts of PHI which are no longer relevant for solving the QBF
     */
    unsigned long trimPHI();

    /**
     * Removes parts of KSI which are no longer relevant for solving the QBF
     */
    unsigned long trimKSI();

    /**
     * Returns index of variable v in a layer for phi
     */
    inline unsigned layerIndexPHI(Var v) const { return (v >> 1); }

    /**
     * Returns index of variable v in a layer for ksi
     */
    inline unsigned layerIndexKSI(Var v) const { return (v >> 1) - 1; }
};

#define USAGE_TEXT \
    "\
--reorder               Indicates whether variables should be reordered inside a \n\
                        quantifier. Can sometimes be useful for random QBF.      \n\
\n\
--variable_elimination  Indicates whether variable elimination through unit      \n\
                        propagation should be applied. Can sometimes reduce the  \n\
                        space consumption of the solver.                         \n\
\n\
--deduplication         Indicates whether clauses that are generated multiple    \n\
                        times should be only added once. This can in some cases  \n\
                        lead to a reduced strain on the internally used sat      \n\
                        solver, but can also result in a signifficant increase   \n\
                        in space consumption for rapidly expanding QBF.          \n\
\n\
--dependencies          Indicates whether dependencies provided by DepQBF should \n\
                        be used to reduce the search space. Improves the number  \n\
                        of solved test cases in many industrial QBF families.    \n\
\n\
--dep_timeout=<double>  Used for setting the timeout given to the dependency     \n\
                        analysis by DepQBF. When the timeout is exceeded the     \n\
                        solver returns to the standard behaviour and ignores     \n\
                        any dependency information generated up to that point.   \n\
\n\
--cex_per_call=<int>    Used for setting the maximum number of counterexamples   \n\
                        used for expanding the formula for the verifier. Giving  \n\
                        a negative number indicates that all counterexamples     \n\
                        generated in one call should be used for the expansion.  \n\
\n\
--wit_per_call=<int>    Used for setting the maximum number of witnesses used    \n\
                        for expanding the formula for the refuter. Giving a      \n\
                        negative number indicates that all witnesses generated   \n\
                        in one call should be used for the expansion.            \n\
\n\
--cex_order=<int>       Used for determining the order in which the counter-     \n\
                        examples are used for the expansion of the verifier's    \n\
                        formula. Can impact the route the solver takes when      \n\
                        solving a QBF.                                           \n\
                        1   indicates that the counterexample from the oldest    \n\
                            expansion in the refuter should be used first        \n\
                        -1  indicates that the counterexample from the newest    \n\
                            expansion in the refuter should be used first        \n\
\n\
--wit_order=<int>       Used for determining the order in which the witnesses    \n\
                        are used for the expansion of the refuter\'s formula. Can\n\
                        impact the route the solver takes when solving a QBF.    \n\
                        1   indicates that the witness from the oldest expansion \n\
                            in the verifier should be used first                 \n\
                        -1  indicates that the witness from the newest expansion \n\
                            in the verifier should be used first                 \n\
\n"

#endif  // MYSOLVER_H
