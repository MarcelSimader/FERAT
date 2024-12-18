//
// Created by vedad on 17/07/17.
//

#ifndef SOLVEROPTIONS_H
#define SOLVEROPTIONS_H

#include <fstream>

/**
 * Struct containing options, which are passed to the MySolver Constructor.
 * @see MySolver::MySolver()
 */
struct SolverOptions
{
public:
  
  /**
   * Different trimming modes
   */
  enum TrimmingMode
  {
    NO_TRIMMING,
    PERIODIC_TRIMMING,
    BRANCH_TRIMMING,
    DYNAMIC_TRIMMING
  };
  
  /**
   * Initialises all member variables to default values.
   */
  SolverOptions();
  
  /**
   * Member function for parsing options given as a command line input.
   * @param argc Number of arguments
   * @param argv Null terminated vector of arguments
   * @return true if the parsing was successful, false otherwise
   */
  bool parse(int argc, char** argv);
  
  static const unsigned long DEFAULT_CEX_PER_CALL = 1;    ///< Default value for SolverOptions::cex_per_call
  static const unsigned long DEFAULT_WIT_PER_CALL = 1;    ///< Default value for SolverOptions::wit_per_call
 static const bool DEFAULT_WARMUP = true;                ///< Default value for SolverOptions::
  static const long DEFAULT_WARMUP_RANDOMS = 0;           ///< Default value for SolverOptions::warmup_randoms
  
  static const bool DEFAULT_TSEITIN_OPTIMISATION = true; ///< Default value for SolverOptions::tseitin_optimisation
  static const bool DEFAULT_BUMPING = true;              ///< Default value for SolverOptions::bumping
  
  static const TrimmingMode DEFAULT_TRIMMING_PHI = NO_TRIMMING;     ///< Default value for SolverOptions::trimming_phi
  static const TrimmingMode DEFAULT_TRIMMING_KSI = BRANCH_TRIMMING;     ///< Default value for SolverOptions::trimming_ksi
  
  static const unsigned long DEFAULT_TRIMMING_INTERVAL = 200;   ///< Default value for SolverOptions::trimming_interval

  static const unsigned long DEFAULT_MEMORY_LIMIT = 3000000;    ///< Default value for SolverOptions::sat_memory_limit

  unsigned long cex_per_call; ///< Number of counter examples to be added per call
  unsigned long wit_per_call; ///< Number of witness examples to be added per call

  bool logging_phi;
  std::string phi_log;
  std::string tmp_phi_log;

  bool logging_ksi;
  std::string ksi_log;
  std::string tmp_ksi_log;

  std::string tmp_dir;

  bool tseitin_optimisation;
  bool bumping;

  bool warmup;                      ///< Whether warmup pattern should be used
  long warmup_randoms;              ///< Number of random counterexamples addet at initialisation
  TrimmingMode trimming_phi;        ///< Specifies the trimming mode used when trimming the formula
  TrimmingMode trimming_ksi;
  unsigned long trimming_interval;  ///< If trimming mode is periodic, do trimming every so many calls
  unsigned long memory_limit;       ///< Number of literals that can be in the sat solvers at once
};

#endif //SOLVEROPTIONS_H
