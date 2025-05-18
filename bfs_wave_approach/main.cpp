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
*/
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

#ifdef DEBUG
  // Cross check against counted:
  Combination c, maxC;
  maxC.size = Combination::sizeOfToken(token);
  maxC.height = Combination::heightOfToken(token);
  Combination::getLayerSizesFromToken(token, maxC.layerSizes);
  BrickPlane neighbours[MAX_BRICKS];
  for(int i = 0; i < MAX_BRICKS; i++)
    neighbours[i].unsetAll();
  CombinationBuilder builder(c, 0, 1, neighbours, maxC, true, false, true);
  builder.build();
  builder.report();
  std::cout << "Combinations built for " << maxC << std::endl;
#endif

  Counts counts, countsLeft, countsRight;
  for(int D = 2; D <= maxDist; D++) {
    // Read files and handle batches one by one:
    BitReader reader1(base, leftSize + base, leftToken, D);
    BitReader reader2(base, rightSize + base, rightToken, D);

    std::vector<Report> l, r;
    while(reader1.next(l)) {
      bool ok = reader2.next(r); assert(ok);
      bool bs180, bs90, first = true;

      Base baseCombination;
      Counts c, cl, cr;
      // Match all connectivities:
      for(std::vector<Report>::const_iterator it1 = l.begin(); it1 != l.end(); it1++) {
	const Report &report1 = *it1;
	if(first) {
	  bs180 = report1.baseSymmetric180;
	  bs90 = report1.baseSymmetric90;
	  baseCombination = report1.c;
	  first = false;
	}
	else {
	  assert(bs180 == report1.baseSymmetric180);
	  assert(bs90 == report1.baseSymmetric90);
	  assert(baseCombination == report1.c);
	}
	for(std::vector<Report>::const_iterator it2 = r.begin(); it2 != r.end(); it2++) {
	  Counts fromUp = Report::countUp(report1, *it2);
	  c += fromUp;
	  //std::cout << " " << report1 << " <> " << *it2 << " -> " << fromUp << std::endl;
	}
	if(Report::connected(report1, report1))
	  cl += report1.counts;
      }
      for(std::vector<Report>::const_iterator it2 = r.begin(); it2 != r.end(); it2++) {
	const Report &report2 = *it2;
	assert(bs180 == report2.baseSymmetric180);
	assert(bs90 == report2.baseSymmetric90);
	assert(baseCombination == report2.c);
	if(Report::connected(report2, report2))
	  cr += report2.counts;
      }
      if(bs90) {
	assert(c.symmetric90 % 4 == 0);
	c.symmetric90 /= 4;
	assert(c.symmetric180 % 2 == 0);
	c.symmetric180 /= 2;
	assert(c.all % 4 == 0);
	c.all = c.all/4 + c.symmetric180 + c.symmetric90;
      }
      else if(bs180) {
	assert(c.symmetric90 == 0);
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

	cr.all -= cr.symmetric180 + cr.symmetric90;
	assert(cr.symmetric90 % 4 == 0);
	cr.symmetric90 /= 4;
	assert(cr.symmetric180 % 2 == 0);
	cr.symmetric180 /= 2;
	assert(cr.all % 4 == 0);
	cr.all = cr.all/4 + cr.symmetric180 + cr.symmetric90;
      }
      else if(bs180) {
	// The non-symmetric are counted twice:
	cl.all -= cl.symmetric180;
	assert(cl.all % 2 == 0);
	cl.all = cl.all/2 + cl.symmetric180;

	cr.all -= cr.symmetric180;
	assert(cr.all % 2 == 0);
	cr.all = cr.all/2 + cr.symmetric180;
      }

#ifdef DEBUG
      CombinationCountsMap::iterator bit = builder.baseCounts.find(baseCombination);
      if(bit == builder.baseCounts.end()) {
	if(c.all != 0) {
	  std::cerr << "   Unknown base! " << baseCombination << std::endl;
	  std::cerr << "File:    " << c << std::endl;
	  assert(false);
	}
      }
      else {
	Counts fromBuilding = bit->second;
	fromBuilding.all += fromBuilding.symmetric180;
	fromBuilding.all /= 2 * maxC.layerSizes[0];
	fromBuilding.symmetric180 /= maxC.layerSizes[0];

	if(c != fromBuilding) {
	  std::cerr << "Base " << baseCombination << std::endl;
	  std::cerr << "File:    " << c << std::endl;
	  std::cerr << "Counted: " << fromBuilding << std::endl;
	  assert(false);
	}
	//std::cout << "   Handled base " << baseCombination << std::endl;
	builder.baseCounts.erase(bit); // Ensure not counted again
      }
#endif

      countsLeft += cl;
      countsRight += cr;

      l.clear();
      r.clear();
    } // while(reader1.next(rm) && reader2.next(rm))
  } // for d = 2 .. maxDist

  Combination::checkCounts(token, counts);
  Combination::checkCounts(leftToken, countsLeft);
  Combination::checkCounts(rightToken, countsRight);

#ifdef DEBUG
  for(CombinationCountsMap::iterator it = builder.baseCounts.begin(); it != builder.baseCounts.end(); it++) {
    std::cerr << "Unmatched base " << it->first << ": " << it->second << std::endl;
  }
#endif

  return 0;
}

int main(int argc, char** argv) {
  if(argc == 3 || argc == 4) {
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

    if(argc == 4) { // Lemma 3:
      if(base < 2) {
	std::cerr << "Unsupported base of refinement: " << (int)base << std::endl;
	return 3;
      }
      int maxDist = get(argv[2]);
      int threads = get(argv[3]);
      std::cout << "Precomputing refinement " << token << " up to distance of " << maxDist << std::endl;
      Lemma3 lemma3(base, threads, maxCombination);
      lemma3.precompute(maxDist);
    }
    else { // Normal run for a refinement:
      int threads = get(argv[2]);

      BrickPlane neighbours[MAX_BRICKS];
      for(uint8_t i = 0; i < MAX_BRICKS; i++)
	neighbours[i].unsetAll();

      Combination combination;
      CombinationBuilder b(combination, 0, 1, neighbours, maxCombination, true, false, true);
      if(threads > 2) {
	std::cout << "Running with " << threads << " threads for <" << token << "> of size " << (int)maxCombination.size << std::endl;
	b.buildSplit(threads);
      }
      else {
	std::cout << "Running single threaded for <" << token << "> of size " << (int)maxCombination.size << std::endl;
	b.build();
      }
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
  else if(argc == 6) {
    int base = get(argv[1]);
    int size = get(argv[2]);
    int token = Combination::reverseToken(get(argv[3]));
    int D = get(argv[4]);
    BitReader reader(base, size, token, D);

    std::vector<Report> reports;
    while(reader.next(reports)) {
      bool first = true;
      CountsMap m;
      for(std::vector<Report>::const_iterator it = reports.begin(); it != reports.end(); it++) {
	const Report &report = *it;
	if(first) {
	  std::cout << report.c << " s180: " << report.baseSymmetric180 << ", s90: " << report.baseSymmetric90 << std::endl;
	  first = false;
	}
	int t = 1;
	for(int i = 0; i < base-1; i++) {
	  t = 10 * t + (report.colors[i]+1);
	}
	m[t] = report.counts;
      }
      for(CountsMap::const_iterator it = m.begin(); it != m.end(); it++)
	std::cout << it->first << ": " << it->second << std::endl;
      reports.clear();
    }
  }
  else {
    std::cerr << "Usage 1: REFINEMENT [MAX_DIST] THREADS. Include MAX_DIST to compute precomputations. Results of precomputations are saved to files in folder /base_<BASE>_size_<SIZE_TOTAL>[_refinement_REFINEMENT]. THREADS-1 worker threads will be spawned" << std::endl;
    std::cerr << "Usage 2: LEFT BASE RIGHT MAX_DIST. This is for handling the precomputations" << std::endl;
    return 1;
  }
}
