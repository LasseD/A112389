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

  Lemma 2:
   Let A' = A(X,Y,Z1,...,Zk,...,ZY) where Zk = 2 be a refinement where layer k has 2 bricks.
   Consider all placement of bricks in layer k and compute A' based the models that can be built on the two sides of the layer.

   Performance:
   ./run.o 6  1075.85s user 13.63s system 561% cpu 3:14.05 total
   ./run.o 7  175385.56s user 384.71s system 358% cpu 13:36:34.43 total
   ./run.o 8 runs for a very long time:
   - =0,3= time: 14.5 hours
   - =0,4= time:  7.7 hours
   - Number of files:
    - Size 3:  77
    - Size 4: 163
    - Size 5: 277
    - Size 6: 419
    - Size 7: 589
    - Size 8: 787 (Expected if growth is polynomial 19 + 44x + 14x^2)
   - Estimated time to compute 787 files: 787 * 7.7 = 253 days or 8.3 months.

  Lemma 3:
   Let A' = A(X,Y,Z1,...,Zk,...,ZY) where Zk = 3 be a refinement where layer k has 3 bricks.
   Consider all placement of bricks in layer k and compute |A'| based the models that can be built on the two sides of the layer.

  Encode connectivity as additional digit in token:
  1 = All connected
  2 = Connect b0, b1
  3 = Connect b0, b2
  4 = Connect b1, b2
  5 = None connected
*/
int main(int argc, char** argv) {
#ifdef LEMMAS

  if(argc == 3 || argc == 4) {
    int base = getInt(argv[1]);
    if(!(base == 2 || base == 3)) {
      std::cerr << "Invalid parameter! BASE=" << base << ", should be either 2 or 3" << std::endl;
      return 1;
    }
    int n = getInt(argv[2]);
    if(n < 3 || n > 8) {
      std::cerr << "Invalid parameter! SIZE_TOTAL=" << n << ", should be between 3 and 8" << std::endl;
      return 1;
    }

    if(base == 2) {
      if(argc == 4) {
	std::cerr << "Parameter MAX_DIST should be used for base 3!" << std::endl;
	return 1;
      }
      rectilinear::Lemma2 lemma2(n);
      lemma2.computeOnBase2();
    }
    else {
      if(argc == 3) {
	std::cerr << "Parameter MAX_DIST should be used for base 3!" << std::endl;
	return 1;
      }
      rectilinear::Lemma3 lemma3(n, getInt(argv[3]));
      lemma3.computeOnBase3();
    }
  }
  else {
    std::cerr << "Usage: BASE SIZE_TOTAL [MAX_DIST] to build on BASE bricks. Results are saved to files in folder /base_<BASE>_size_<SIZE_TOTAL>" << std::endl;
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
