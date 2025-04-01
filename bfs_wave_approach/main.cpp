#include <iostream>
#include <stdlib.h>
#include "bfs.h"

#ifdef PROFILING
  InvocationMap *invocationCounts;
#endif

int getInt(char *argv) {
  char c;
  int ret = 0;
  for(int i = 0; (c = argv[i]); i++)
    ret = 10 * ret + (c-'0');
  return ret;
}

/*
  This code base uses an approach inspired by Eilers (2016) to compute all models of n bricks:
  For a given brick in the first (base) layer:
  Let the base brick be the first "BFS Wave" or "wave"
  For each wave:
    Pick 1..|wave| bricks from wave:
      Find next wave and recurse until model contains n bricks.
*/
int main(int argc, char** argv) {
#ifdef LEMMA2

  if(argc == 2) {
    int n = getInt(argv[1]);

    if(n < 3 || n > 8) {
      std::cerr << "Invalid single parameter! n=" << n << ", should be between 3 and 8" << std::endl;
      return 1;
    }

    rectilinear::Lemma2 lemma2(n);
    lemma2.computeOnBase2();
  }
  else if(argc == 5) {
    int n = getInt(argv[1]);
    bool vertical = argv[2][0] != '-';
    int dx = getInt(argv[3]);
    int dy = getInt(argv[4]);

    rectilinear::Lemma2 lemma2(n);
    rectilinear::Counts c, d;
    lemma2.computeOnBase2(vertical, dx, dy, c, d);
    std::cout << "Computed " << c << " connected and " << d << " disconnected models on " << rectilinear::Brick(vertical,dx,dy) << std::endl;
  }
  else {
    std::cerr << "Provide either 1 or 4 parameters to run:" << std::endl;
    std::cerr << "1 Parameter: SIZE_TOTAL to build on a base of 2 bricks. Results are saved to files in folder /base_2_size_" << std::endl;
    std::cerr << "4 parameters: SIZE_TOTAL ORIENTATION DX DY, where ORIENTATION is - or I, and dx and dy are 0 or greater" << std::endl;
    return 1;
  }

#else

  char c;
  int sum = 0;

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

  int n = getInt(argv[1]);

  if(argc == 3 && sum != n) {
    std::cerr << "MAX_TOKEN does not match SIZE!" << std::endl;
    return 2;
  }

  std::cout << "Building models for size " << n << std::endl;
  rectilinear::Combination combination;
  rectilinear::CombinationBuilder b(combination, 0, 1, n, maxLayerSizes);
  b.buildSplit();
  b.report();

#ifdef PROFILING
  Profiler::reportInvocations();
#endif

#endif
  
  return 0;
}
