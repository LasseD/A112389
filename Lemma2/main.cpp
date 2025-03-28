#include <iostream>
#include <stdlib.h>
#include "bfs.h"

/*
  Lemma 2:
  Let A' = A(X,Y,Z1,...,Zk,...,ZY) where Zk = 2 be a refinement where layer k has 2 bricks.
  Consider all placement of bricks in layer k and compute A' based the models that can be built on the two sides of the layer.

  ./run.o 6  903.65s user 89.97s system 98% cpu 16:49.61 total

*/
int main(int argc, char** argv) {
  char c;

  if(argc == 2) {
    int n = argv[1][0] - '0';

    if(n < 3 || n > 8) {
      std::cerr << "Invalid single parameter! n=" << n << ", should be between 3 and 8" << std::endl;
      return 1;
    }

    rectilinear::Lemma2 lemma2(n);
    lemma2.computeOnBase2();

    return 0;
  }
  else if(argc == 3) {
    int left = 0, n = 0;

    for(int i = 0; (c = argv[2][i]); i++)
      left = 10 * left + (c-'0');
    for(int i = 0; (c = argv[1][i]); i++)
      n = 10 * n + (c-'0');
    int right = n-2-left;

    if(n < 2 || left < 1 || right < 1 || left > right) {
      std::cerr << "Invalid parameter! n=" << n << ", left=" << left << ", right=" << right << std::endl;
      return 1;
    }

    std::cout << "Building models for size " << n << " with " << left << " bricks on one side and " << right << " on the other " << std::endl;

    rectilinear::Lemma2 lemma2(left, right, n);
    lemma2.computeAllLeftAndRight();
    lemma2.report();

    return 0;
  }
  else {
    std::cerr << "Provide either 1 or 2 parameters to run:" << std::endl;
    std::cerr << "1 Parameter: SIZE_TOTAL to build on a base of 2 bricks. Results are saved to files in folder /base_2_size_" << std::endl;
    std::cerr << "2 parameters: SIZE_TOTAL SIZE_LEFT, where SIZE_LEFT is excluding the 2 bricks in the layer of interest" << std::endl;
    return 1;
  }
}
