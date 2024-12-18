//
// This file is part of the ijtihad QBF solver
// Copyright (c) 2016, Graz University of Technology
// Authored by Vedad Hadzic
//

/* Editor: Marcel Simader (marcel.simader@jku.at) */
/* Modified version of `Ijtihad` as part of the `FERAT` toolchain.  */
/* Date: 29.09.2024 */

#include <stdio.h>
#include <stdlib.h>

#include <csignal>
#include <fstream>
#include <iostream>

#include "MySolver.hh"
#include "ReadQ.hh"
#include "debug.hh"

using std::cerr;
using std::cin;
using std::cout;
using std::endl;
using std::string;

void handle_sigint(int sig_num);
void print_usage(char* instance);
void print_exit(bool sat);

SolverOptions* _sigint_opt = nullptr;

int main(int argc, char** argv) {
  SolverOptions opt;
  _sigint_opt = &opt;
  if (!opt.parse(argc, argv)) {
    print_usage(argv[0]);
    exit(100);
  }

  const string file_name = argv[argc - 1];
  gzFile file = Z_NULL;

  file = gzopen(file_name.c_str(), "rb");
  if (file == Z_NULL) {
    cerr << "Unable to open file: " << file_name << endl;
    exit(100);
  }

  Reader* file_reader = new Reader(file);
  ReadQ qbf_reader(*file_reader);

  try {
    qbf_reader.read();
  } catch (ReadException& rex) {
    cerr << rex.what() << endl;
    delete file_reader;
    exit(100);
  }

  if (!qbf_reader.get_header_read()) {
    if (qbf_reader.get_prefix().size() == 0) {
      cerr << "ERROR: Formula has empty prefix and no problem line." << endl;
      delete file_reader;
      exit(100);
    }
    cerr << "WARNING: Missing problem line in the input file." << endl;
  }

  if (qbf_reader.get_prefix().size() == 0) {
    delete file_reader;
    bool const sat = qbf_reader.get_clauses().empty();
    // Marcel: Trivial case, we just hard-code the output for log_phi
    if (opt.logging_phi) {
      try {
        std::ofstream phi_log_file(opt.phi_log, std::ofstream::out);
        phi_log_file << "c This file was generated by Ijtihad." << endl;
        if (sat) {
          // Marcel: Simply the empty formula
          phi_log_file << "p cnf 0 0" << endl;
        } else {
          // Marcel: Simply the empty clause
          phi_log_file << "p cnf 0 1" << endl;
          phi_log_file << "0" << endl;
        }
        phi_log_file.close();
      } catch (std::exception& e) {
        cout << "c " << e.what();
      }
    }
    print_exit(sat);
  }

  delete file_reader;
  // Marcel: We register SIGINT here, so that we can delete temporary files
  //         created when the solver is instantiated.
  std::signal(SIGINT, handle_sigint);

  MySolver s(qbf_reader.get_prefix(), qbf_reader.get_clauses(), &opt);

  debugn("Calling the solver.");
  const bool sat = s.solve();
  s.printStats();
  /*
  if(sat && qbf_reader.get_prefix()[0].first == EXISTENTIAL)
  {
    for(const Lit& l : s.getSolution())
      cout << l << " ";
    cout << endl;
  }
  */

  if (opt.logging_phi)
    if (std::remove(opt.tmp_phi_log.c_str()) != 0)
      cout << "c removing tmp file failed" << endl;

  print_exit(sat);
}

void handle_sigint(int sig_num) {
  if (_sigint_opt != nullptr)
    if (_sigint_opt->logging_phi)
      if (std::remove(_sigint_opt->tmp_phi_log.c_str()) != 0)
        cout << "c removing tmp file failed" << endl;
  exit(sig_num);
}

void print_exit(bool sat) {
  cout << "s cnf " << (sat ? '1' : '0') << endl;
  exit(sat ? 10 : 20);
}

void print_usage(char* instance) {
  cerr << "Usage:  " << instance << " <OPTIONS> <FILENAME>" << endl;
  cerr << "<OPTIONS> may contain the following:\n" << USAGE_TEXT;
}
