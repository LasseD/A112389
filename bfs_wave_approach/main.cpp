#include <iostream>
#include <stdlib.h>
#include "bfs.h"

#ifdef PROFILING
  InvocationMap *invocationCounts;
#endif

/*
  This code base uses an approach inspired by Eilers (2016) to compute all models of n bricks:
  For a given brick in the first (base) layer:
  Let the base brick be the first "BFS Wave" or "wave"
  For each wave:
    Pick 1..|wave| bricks from wave:
      Find next wave and recurse until model contains n bricks.
*/
int main(int argc, char** argv) {
  char c;
  int sum;

  uint8_t *maxLayerSizes = NULL;
  if(argc == 2) {
#ifdef REFINEMENT
    std::cerr << "-DREFINEMENT compilation flag used. Program must be run with MAX_TOKEN!" << std::endl;
    return 4;
#endif
  }
  else if(argc == 3) {
#ifndef REFINEMENT
    std::cerr << "-DREFINEMENT compilation flag missing when using MAX_TOKEN!" << std::endl;
    return 3;
#endif
    sum = 0;
    maxLayerSizes = new uint8_t(MAX_BRICKS);
    for(uint8_t i = 0; i < MAX_BRICKS; i++) {
      maxLayerSizes[i] = 0;
    }
    for(uint8_t i = 0; (c = argv[2][i]); i++) {
      maxLayerSizes[i] = c - '0';
      sum += maxLayerSizes[i];
    }
  }
  else {
    std::cerr << "Parameters: SIZE [MAx_TOKEN]" << std::endl;
    return 1;
  }

  int n = 0;
  for(int i = 0; (c = argv[1][i]); i++) {
    n += (c-'0');
  }

  if(argc == 3 && sum != n) {
    std::cerr << "MAX_TOKEN does not match SIZE!" << std::endl;
    return 2;
  }

  std::cout << "Building models for size " << n << std::endl;
  rectilinear::Combination combination;
  rectilinear::CombinationBuilder b(combination, 0, 1, n, maxLayerSizes);
  b.build(false);
  b.report();

#ifdef PROFILING
  Profiler::reportInvocations();
#endif
  
  return 0;
}
