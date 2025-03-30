#include <iostream>
#include <stdlib.h>
#include "bfs.h"

int getInt(char *argv) {
  char c;
  int ret = 0;
  for(int i = 0; (c = argv[i]); i++)
    ret = 10 * ret + (c-'0');
  return ret;
}

/*
  Lemma 2:
  Let A' = A(X,Y,Z1,...,Zk,...,ZY) where Zk = 2 be a refinement where layer k has 2 bricks.
  Consider all placement of bricks in layer k and compute A' based the models that can be built on the two sides of the layer.

  ./run.o 6  903.65s user 89.97s system 98% cpu 16:49.61 total

*/
int main(int argc, char** argv) {
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

  return 0;
}
