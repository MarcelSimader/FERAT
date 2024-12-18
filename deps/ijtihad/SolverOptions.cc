//
// Created by vedad on 17/07/17.
//

#include "SolverOptions.hh"
#include "debug.hh"
#include <iostream>
#include <string>
#include <sstream>
#include <unistd.h>

using std::cout;
using std::endl;

SolverOptions::SolverOptions() :
    cex_per_call(DEFAULT_CEX_PER_CALL),
    wit_per_call(DEFAULT_WIT_PER_CALL),
    logging_phi(false),
    logging_ksi(false),
    tmp_dir("/tmp/"),
    tseitin_optimisation(DEFAULT_TSEITIN_OPTIMISATION),
    bumping(DEFAULT_BUMPING),
    warmup(DEFAULT_WARMUP),
    warmup_randoms(DEFAULT_WARMUP_RANDOMS),
    trimming_phi(DEFAULT_TRIMMING_PHI),
    trimming_ksi(DEFAULT_TRIMMING_KSI),
    trimming_interval(DEFAULT_TRIMMING_INTERVAL),
    memory_limit(DEFAULT_MEMORY_LIMIT)
{ }

bool SolverOptions::parse(int argc, char** argv)
{
  debugn("Parsing options:");
  
  if(argc >= 2 && std::string(argv[1]) == "--help") return false;
  
  long vall = 0;
  std::string vals = "";

  unsigned long cpc = DEFAULT_CEX_PER_CALL;
  unsigned long wpc = DEFAULT_WIT_PER_CALL;
  bool wu = DEFAULT_WARMUP;
  long wr = DEFAULT_WARMUP_RANDOMS;
  
  TrimmingMode trim_phi = DEFAULT_TRIMMING_PHI;
  TrimmingMode trim_ksi = DEFAULT_TRIMMING_KSI;
  unsigned long trim_int = DEFAULT_TRIMMING_INTERVAL;
  unsigned long ml = DEFAULT_MEMORY_LIMIT;
  
  for (int position = 1; position < argc - 1; position++)
  {
    std::string option(argv[position]);
    std::stringstream value("", std::ios::in);
    unsigned long eq;
    debugn("option " << position << " : " << option);
    if (option.find("--") == 0)
    {
      eq = option.find("=");
      if (eq != std::string::npos && eq + 1 < option.size())
      {
        value.str(option.substr(eq + 1));
        value >> vall;
        value.clear();
        value.str(option.substr(eq + 1));
        value >> vals;
        value.clear();
        value.str(option.substr(eq + 1));
        
        option = option.substr(0, eq);
        debugn("After splitting: " << option << " " << value.str() << " " << vall << " " << vals);
      }
      
      if (option == "--warmup")
      {
        if (eq == std::string::npos || (bool) vall)
          wu = true;
        else
          wu = false;
      }
      else if (option == "--cex_per_call" && eq != std::string::npos)
      {
        if (vall < 0)
        {
          debugn("cex_per_call has a negative value, defaulting to all");
          vall = -1;
        }
        cpc = (unsigned long) vall;
      }
      else if (option == "--wit_per_call" && eq != std::string::npos)
      {
        if (vall < 0)
        {
          debugn("wit_per_call has a negative value, defaulting to all");
          vall = -1;
        }
        wpc = (unsigned long) vall;
      }
      else if (option == "--warmup_randoms" && eq != std::string::npos)
      {
        if (vall >= 0 && vall <= 100)
        {
          wr = vall;
        }
        else
        {
          debugn("warmup_random cannot have such a value, default value used instead");
        }
      }
      else if (option == "--mem_limit" && eq != std::string::npos)
      {
        if (vall >= 100)
        {
          ml = vall * 750;
        }
        else
        {
          debugn("--mem_limit cannot be so low, try more than 100 MB");
        }
      }
      else if (option == "--trimming_phi" && eq != std::string::npos)
      {
        if(vals == "periodic")
        {
          trim_phi = PERIODIC_TRIMMING;
        }
        else if(vals == "dynamic")
        {
          trim_phi = DYNAMIC_TRIMMING;
        }
        else if(vals == "branch")
        {
          trim_phi = BRANCH_TRIMMING;
        }
        else if(vals == "none")
        {
          trim_phi = NO_TRIMMING;
        }
        else
        {
          debugn("Trimming option is not valid");
        }
      }
      else if (option == "--trimming_ksi" && eq != std::string::npos)
      {
        if(vals == "branch")
        {
          trim_ksi = BRANCH_TRIMMING;
        }
        else if(vals == "none")
        {
          trim_ksi = NO_TRIMMING;
        }
        else
        {
          debugn("Trimming option is not valid");
        }
      }
      else if(option == "--tmp_dir" && eq != std::string::npos)
      {
        tmp_dir = vals;
      }
      else if(option == "--log_phi" && eq != std::string::npos)
      {
        phi_log = vals;
        tmp_phi_log = "ijtihad-phi-";
        tmp_phi_log.append(std::to_string(::getpid())).append(".txt");
        logging_phi = true;
      }
      else if(option == "--log_ksi" && eq != std::string::npos)
      {
        ksi_log = vals;
        tmp_phi_log = "ijtihad-ksi-";
        tmp_phi_log.append(std::to_string(::getpid())).append(".txt");
        logging_ksi = true;
      }
      else if (option == "--trimming_interval"  && eq != std::string::npos)
      {
        if(vall > 5)
        {
          trim_int = (unsigned long)vall;
        }
        else
        {
          debugn("Cannot set the trimming interval to less than 5 calls");
        }
      }
      else
      {
        debugn("Unknown opiton or no argument provided: " << option << " , aborting.");
        return false;
      }
    }
    else
    {
      debugn("option " << position << " has wrong format, aborting.");
      return false;
    }
  }

  if(logging_phi) tmp_phi_log = tmp_dir + tmp_phi_log;
  if(logging_ksi) tmp_ksi_log = tmp_dir + tmp_ksi_log;

  cex_per_call = cpc;
  wit_per_call = wpc;
  warmup = wu;
  warmup_randoms = wr;
  trimming_phi = trim_phi;
  trimming_ksi = trim_ksi;
  trimming_interval = trim_int;
  memory_limit = ml;
  
  cout  << "c Options after parsing: "
        << "\nc cex_per_call = " << cex_per_call
        << "\nc wit_per_call = " << wit_per_call
        << "\nc warmup = " << warmup
        << "\nc warmup_randoms = " << warmup_randoms
        << "\nc trimming_phi = " << trimming_phi
        << "\nc trimming_ksi = " << trimming_ksi
        << "\nc trimming_interval = " << trimming_interval
        << "\nc memory_limit = " << memory_limit
        << endl;
  return true;
}