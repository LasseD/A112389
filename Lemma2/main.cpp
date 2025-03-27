#include <iostream>
#include <stdlib.h>
#include "bfs.h"

/*
  Lemma 2:
  Let A' = A(X,Y,Z1,...,Zk,...,ZY) where Zk = 2 be a refinement where layer k has 2 bricks.
  Consider all placement of bricks in layer k and compute A' based the models that can be built on the two sides of the layer.
*/
int main(int argc, char** argv) {
  if(argc != 3) {
    std::cerr << "Parameters: SIZE_TOTAL SIZE_LEFT, where SIZE_LEFT is excluding the 2 bricks in the layer of interest" << std::endl;
    return 1;
  }

  char c;
  int left = 0, n = 0;

  for(int i = 0; (c = argv[2][i]); i++) {
    left = 10 * left + (c-'0');
  }
  for(int i = 0; (c = argv[1][i]); i++) {
    n = 10 * n + (c-'0');
  }
  int right = n-2-left;

  if(n < 2 || left < 1 || right < 1 || left > right) {
    std::cerr << "Invalid parameter! n=" << n << ", left=" << left << ", right=" << right << std::endl;
    return 2;
  }

  std::cout << "Building models for size " << n << " with " << left << " bricks on one side and " << right << " on the other " << std::endl;

  rectilinear::Lemma2 lemma2(left, right, n);
  lemma2.execute();
  lemma2.report();

  return 0;
}
