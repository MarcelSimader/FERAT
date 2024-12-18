#include "../ipasir.hh"
#include "stdio.h"
// #include "picosat.h"

extern "C" void* picosat_init ();
extern "C" void picosat_set_prefix (void* res, char* prefix);
extern "C" void picosat_set_verbosity (void* res, int v);
extern "C" void picosat_set_output (void* res, FILE* out);
extern "C" void picosat_stats (void* solver);
extern "C" void picosat_reset (void* solver);
extern "C" void picosat_add (void* solver, int lit);
extern "C" void picosat_assume (void* solver, int lit);
extern "C" int picosat_sat (void* solver, int a);
extern "C" int picosat_failed_assumption (void* solver, int lit);
extern "C" int picosat_deref (void* solver, int var);
extern "C" void picosat_set_interrupt (void* solver, void* state, int (*terminate)(void * state));


static const char * sig = "picosat";

const char * ipasir_signature () { return sig; }

void * ipasir_init () {
  char prefix[80];
  void * res = picosat_init ();
  sprintf (prefix, "c [%s] ", sig);
  picosat_set_prefix (res, prefix);
  picosat_set_verbosity (res, 0);
  picosat_set_output (res, stdout);
  return res;
}

void ipasir_release (void * solver) {
  picosat_stats (solver);
  picosat_reset (solver);
}

void ipasir_add (void * solver, int lit) { picosat_add (solver, lit); }

void ipasir_assume (void * solver, int lit) { picosat_assume (solver, lit); }

int ipasir_solve (void * solver) { return picosat_sat (solver, -1); }

int ipasir_failed (void * solver, int lit) {
  return picosat_failed_assumption (solver, lit);
}

int ipasir_val (void * solver, int var) {
  int val = picosat_deref (solver, var);
  if (!val) return -var;
  return val < 0 ? -var : var;
}

void
ipasir_set_terminate (
  void * solver,
  void * state, int (*terminate)(void * state)) {
  picosat_set_interrupt (solver, state, terminate);
}
