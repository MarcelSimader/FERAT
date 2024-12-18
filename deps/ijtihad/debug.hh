//
// This file is part of the ijtihad QBF solver
// Copyright (c) 2016, Graz University of Technology
// Authored by Vedad Hadzic
//

#ifndef DEBUG_H
#define DEBUG_H

// #define DEBUG

#include <iostream>
using std::cout;

#ifndef DEBUG
#define debug( x )
#define debugn( x )
#else
#define debug( x ) \
    x

#define debugn( x )  \
  cout << x << "\n" << std::flush
#endif



#endif //DEBUG_H
