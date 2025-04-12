#include <iostream>
#include <stdlib.h>
#include <assert.h>
#include "bfs.h"

using namespace rectilinear;

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
  Optimization:
  - Move precomputeForPlacements and up into thread handler:
  - Have ThreadEnablingBuilder handle last part of iteration:
   - MultiLayerBrickPicker -> BasePicker
    - Have own CombinationResultsMap resultsMap
    - next() => nextBase()
     - Clear counts on builder
     - Check with L3 if base ok
     - Mark as "extra from base" if using base that is not ready yet
     - Use "extra from base" when summing up
      - "report_mutex" in picker
  
  This code base uses an approach inspired by Eilers (2016) to compute all LEGO models of n 2x4 bricks:
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
*/
int runLemma2or3(int argc, char** argv) {
  if(argc < 3) {
    std::cerr << "Usage: BASE SIZE_TOTAL [REFINEMENT] [MAX_DIST] to build on BASE bricks. Results are saved to files in folder /base_<BASE>_size_<SIZE_TOTAL>[_refinement_REFINEMENT]" << std::endl;
    return 1;
  }

  int base = getInt(argv[1]);
  int n = getInt(argv[2]);
  if(n <= base || n > MAX_BRICKS || base > MAX_LAYER_SIZE) {
    std::cerr << "Invalid parameter! SIZE_TOTAL=" << n << ", should be between BASE+1 and " << MAX_BRICKS << std::endl;
    return 3;
  }

  if(base == 2) {
    if(argc != 3) {
      std::cerr << "For base 2, please only provide SIZE_TOTAL as additional parameter." << std::endl;
      return 4;
    }
    Lemma2 lemma2(n);
    lemma2.computeOnBase2();
  }
  else {
    Combination maxCombination;
    bool useMaxCombination = false;
    int requiredArgs = 4;
#ifdef REFINEMENT
    useMaxCombination = true;
    requiredArgs = 5;
#endif
    if(argc != requiredArgs) {
      std::cerr << "Invalid number of parameters for base 3!" << std::endl;
      return 4;
    }
    if(!Combination::createMaxCombination(n, argv[3], maxCombination)) {
      std::cerr << "Unable to interpret parameter REFINEMENT" << std::endl;
      return 5;
    }

    int maxDist = getInt(argv[requiredArgs-1]);
    std::cout << "Precomputing for base " << base << " up to size " << n << " and up to distance of " << maxDist << std::endl;
    Lemma3 lemma3(n, base, useMaxCombination ? &maxCombination : NULL);
    lemma3.precompute(maxDist);
  }
  return 0;
}

int runStandardConstruction(int argc, char** argv) {
  char c;
  int sum = 0;

  Combination maxCombination;
  bool useMaxCombination = false;
  if(argc <= 2) {
#ifdef REFINEMENT
    std::cerr << "-DREFINEMENT compilation flag used. Program must be run with MAX_TOKEN!" << std::endl;
    return 4;
#endif
  }
  int n = getInt(argv[1]);

#ifndef REFINEMENT
  if(argc != 3) {
    std::cerr << "-DREFINEMENT compilation flag missing when using MAX_TOKEN!" << std::endl;
    return 3;
  }

  if(!Combination::createMaxCombination(n, argv[2], maxCombination)) {
    std::cerr << "Unable to interpret parameter MAX_TOKEN" << std::endl;
    return 5;
  }
#endif

  BrickPlane neighbours[MAX_BRICKS];
  for(uint8_t i = 0; i < MAX_BRICKS; i++)
    neighbours[i].unsetAll();

  Combination combination;
  CombinationBuilder b(combination, 0, 1, n, neighbours, &maxCombination, true);
  b.buildSplit();
  b.report();

#ifdef PROFILING
  Profiler::reportInvocations();
#endif
  return 0;
}

bool connected(const Report &a, const Report &b, int base) {
  int ret = 1;
  bool c[MAX_LAYER_SIZE];
  for(int i = 0; i < base-1; i++) {
    bool isColor1 = a.colors[i] == 0 || b.colors[i] == 0;
    c[i] = isColor1;
    if(isColor1)
      ret++;
  }
  bool improved = true;
  while(improved) {
    improved = false;
    for(int i = 0; i < base-1; i++) {
      if(c[i])
	continue;
      // Attempt to color i:
      for(int j = 0; j < base-1; j++) {
	if(c[j] && (a.colors[i] == a.colors[j] || b.colors[i] == b.colors[j])) {
	  c[i] = true;
	  ret++;
	  improved = true;
	  break;
	}
      }
    }
  }
  return ret == base;
}

Counts countUp(const Report &a, const Report &b, int base) {
  if(!connected(a, b, base))
    return Counts();
  return Counts(a.counts.all * b.counts.all,
		a.counts.symmetric180 * b.counts.symmetric180,
		a.counts.symmetric90 * b.counts.symmetric90);
}

int runSumPrecomputations(int argc, char** argv) {
  if(argc != 5) {
    std::cerr << "This is for handling the precomputations. Usage: LEFT BASE RIGHT MAX_DIST" << std::endl;
    return 3;
  }
  int leftToken = getInt(argv[1]);
  int base = getInt(argv[2]);
  int rightToken = getInt(argv[3]);
  int maxDist = getInt(argv[4]);
  
  int leftSize = Combination::sizeOfToken(leftToken);
  int rightSize = Combination::sizeOfToken(rightToken);
  leftToken = leftToken * 10 + base;
  int token = leftToken;
  {
    int tk = rightToken;
    while(tk > 0) {
      token = 10 * token + tk % 10;
      tk /= 10;
    }
  }
  
  rightToken = Combination::reverseToken(rightToken) * 10 + base;
  
  Counts counts;
  for(int D = 2; D <= maxDist; D++) {
    // Read files and handle batches one by one:
    BitReader reader1(base, leftSize + base, leftToken, D);
    BitReader reader2(base, rightSize + base, rightToken, D);

    std::vector<Report> l, r;

    while(reader1.next(l) && reader2.next(r)) {
      bool bs180, bs90;

      Counts c;
      // Match all connectivities:
      for(std::vector<Report>::const_iterator it1 = l.begin(); it1 != l.end(); it1++) {
	const Report &report1 = *it1;
	bs180 = report1.baseSymmetric180;
	bs90 = report1.baseSymmetric90;
	for(std::vector<Report>::const_iterator it2 = r.begin(); it2 != r.end(); it2++) {
	  const Report &report2 = *it2;
	  assert(bs180 == report2.baseSymmetric180);
	  assert(bs90 == report2.baseSymmetric90);
	  c += countUp(report1, report2, base);
	}
      }
      if(bs90) {
	c.all -= c.symmetric180 + c.symmetric90;
	assert(c.symmetric90 % 4 == 0);
	c.symmetric90 /= 4;
	assert(c.symmetric180 % 2 == 0);
	c.symmetric180 /= 2;
	assert(c.all % 4 == 0);
	c.all = c.all/4 + c.symmetric180 + c.symmetric90;
      }
      else if(bs180) {	    
	c.all -= c.symmetric180;
	assert(c.all % 2 == 0);
	c.all /= 2;
	c.all += c.symmetric180;
      }
      counts += c;
      l.clear();
      r.clear();
    } // while(reader1.next(rm) && reader2.next(rm))
  } // for d = 2 .. maxDist

  std::cout << "<" << token << "> " << counts << std::endl;
  
  return 0;
}

int main(int argc, char** argv) {
#ifdef LEMMAS
  return runLemma2or3(argc, argv);
#else
#ifdef SUMS
  return runSumPrecomputations(argc, argv);
#else
  return runStandardConstruction(argc, argv);
#endif
#endif
}
