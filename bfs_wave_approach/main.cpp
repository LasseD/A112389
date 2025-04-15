#include <iostream>
#include <stdlib.h>
#include <assert.h>
#include "bfs.h"

using namespace rectilinear;

#ifdef PROFILING
  InvocationMap *invocationCounts;
#endif

uint64_t get(char *argv) {
  char c;
  uint64_t ret = 0;
  for(int i = 0; (c = argv[i]); i++) {
    if(i > 63) {
      std::cerr << "Unable to read parameter!" << std::endl;
      assert(false);
      return 0;
    }
    ret = 10 * ret + (c-'0');
  }
  return ret;
}

/*
  Compile by:

  g++ -std=c++11 -O3 -DNDEBUG *.cpp -o build.o

  Examples:

  Build precomputations for base 2 token <21> up to max dist 16:
  time ./build.o 21 16

  Count all for token <21>
  time ./build.o 21

  Sum up for <121> considering max dist 8:
  ./build.o 1 2 1 8

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

  Cross check results:
  <121> 37081 (32)
  <21> 250 (20)
  <22> 10411 (49)
  <221> 1297413 (787)
  <222> 43183164 (3305)
  <31> 648 (8)
  <131> 433685 (24)
  <32> 148794 (443)
  <231> 41019966 (1179)
  <232> 3021093957 (46219)
  <41> 550 (28)
  <141> 2101339 (72)
*/
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
  assert(ret <= base);
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
  int leftToken = get(argv[1]);
  int base = get(argv[2]);
  int rightToken = get(argv[3]);
  int maxDist = get(argv[4]);
  
  int leftSize = Combination::sizeOfToken(leftToken);
  int rightSize = Combination::sizeOfToken(rightToken);
  leftToken = leftToken * 10 + base;
  rightToken = Combination::reverseToken(rightToken);
  int token = leftToken;
  {
    int tk = rightToken;
    while(tk > 0) {
      token = 10 * token + tk % 10;
      tk /= 10;
    }
  }
  
  rightToken = rightToken * 10 + base;
  
  Counts counts, countsLeft;
  for(int D = 2; D <= maxDist; D++) {
    // Read files and handle batches one by one:
    BitReader reader1(base, leftSize + base, leftToken, D);
    BitReader reader2(base, rightSize + base, rightToken, D);

    std::vector<Report> l, r;

    while(reader1.next(l) && reader2.next(r)) {
      bool bs180, bs90;

      Counts c, cl;
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
	if(connected(report1, report1, base)) {
	  cl += report1.counts;
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
	c.all = c.all/2 + c.symmetric180;
      }
      counts += c;

      // Cross check:
      if(bs90) {
	// The non-rotational symmetric are overcounted:
	cl.all -= cl.symmetric180 + cl.symmetric90;
	assert(cl.symmetric90 % 4 == 0);
	cl.symmetric90 /= 4;
	assert(cl.symmetric180 % 2 == 0);
	cl.symmetric180 /= 2;
	assert(cl.all % 4 == 0);
	cl.all = cl.all/4 + cl.symmetric180 + cl.symmetric90;
      }
      else if(bs180) {
	// The non-symmetric are counted twice:
	cl.all -= cl.symmetric180;
	assert(cl.all % 2 == 0);
	cl.all = cl.all/2 + cl.symmetric180;
      }

      countsLeft += cl;

      l.clear();
      r.clear();
    } // while(reader1.next(rm) && reader2.next(rm))
  } // for d = 2 .. maxDist

  std::cout << std::endl << " <" << token << "> " << counts << std::endl;
  std::cout << " <" << leftToken << "> " << countsLeft << std::endl << std::endl;
  
  return 0;
}

int main(int argc, char** argv) {
  if(argc == 2 || argc == 3) {
    uint64_t token = get(argv[1]);
    Combination maxCombination;
    Combination::getLayerSizesFromToken(token, maxCombination.layerSizes);
    maxCombination.size = Combination::sizeOfToken(token);
    maxCombination.height = Combination::heightOfToken(token);

    uint8_t base = maxCombination.layerSizes[0];
    if(maxCombination.size > MAX_BRICKS || base > MAX_LAYER_SIZE) {
      std::cerr << "Unsupported refinement!" << std::endl;
      return 3;
    }

    if(argc == 3) { // Lemma 3:
      if(base < 2) {
	std::cerr << "Unsupported base of refinement: " << (int)base << std::endl;
	return 3;
      }
      int maxDist = get(argv[2]);
      std::cout << "Precomputing refinement " << token << " up to distance of " << maxDist << std::endl;
      Lemma3 lemma3(maxCombination.size, base, maxCombination);
      lemma3.precompute(maxDist);
    }
    else { // Normal run for a refinement:
      BrickPlane neighbours[MAX_BRICKS];
      for(uint8_t i = 0; i < MAX_BRICKS; i++)
	neighbours[i].unsetAll();

      Combination combination;
      CombinationBuilder b(combination, 0, 1, maxCombination.size, neighbours, maxCombination, true, false);
      b.buildSplit();
      b.report();
    }
#ifdef PROFILING
    Profiler::reportInvocations();
#endif

    return 0;
  }
  else if(argc == 5) {
    return runSumPrecomputations(argc, argv);
  }
  else {
    std::cerr << "Usage 1: REFINEMENT [MAX_DIST]. Include MAX_DIST to compute precomputations. Results of precomputations are saved to files in folder /base_<BASE>_size_<SIZE_TOTAL>[_refinement_REFINEMENT]" << std::endl;
    std::cerr << "Usage 2: LEFT BASE RIGHT MAX_DIST. This is for handling the precomputations" << std::endl;
    return 1;
  }
}
