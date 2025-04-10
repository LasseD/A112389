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
  TODO: Attempt to improve Precomputation performance by:
  - Use baseGenerator that flips isVertical on all bricks of base, skip if colission or seen.
   - Use in CombinationBuilder.buildSplit()

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
  if(argc == 3 || argc == 4) {
    int base = getInt(argv[1]);
    int n = getInt(argv[2]);
    if(n <= base || n > MAX_BRICKS || base > MAX_LAYER_SIZE) {
      std::cerr << "Invalid parameter! SIZE_TOTAL=" << n << ", should be between BASE+1 and " << MAX_BRICKS << std::endl;
      return 1;
    }

    if(base == 2) {
      if(argc == 4) {
	std::cerr << "Parameter MAX_DIST should be used for base 3!" << std::endl;
	return 1;
      }
      Lemma2 lemma2(n);
      lemma2.computeOnBase2();
    }
    else {
      if(argc == 3) {
	std::cerr << "Parameter MAX_DIST should be used for base 3!" << std::endl;
	return 1;
      }
      int maxDist = getInt(argv[3]);
      std::cout << "Precomputing for base " << base << " up to size " << n << " and up to distance of " << maxDist << std::endl;
      Lemma3 lemma3(n, base);
      lemma3.precompute(maxDist);
    }
    return 0;
  }
  else {
    std::cerr << "Usage: BASE SIZE_TOTAL [MAX_DIST] to build on BASE bricks. Results are saved to files in folder /base_<BASE>_size_<SIZE_TOTAL>" << std::endl;
    return 1;
  }
}

int runStandardConstruction(int argc, char** argv) {
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

  BrickPlane neighbours[MAX_BRICKS];
  for(uint8_t i = 0; i < MAX_BRICKS; i++)
    neighbours[i].unsetAll();

  std::cout << "Building models for size " << n << std::endl;
  Combination combination;
  CombinationBuilder b(combination, 0, 1, n, neighbours, maxLayerSizes);
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
  int left = getInt(argv[1]);
  int base = getInt(argv[2]);
  int right = getInt(argv[3]);
  int maxDist = getInt(argv[4]);
  int size = left + base + right;
  std::cout << std::endl << " Tokens of size " << size << " left-base-right: " <<  left << " " <<  base << " " << right << " for distance 2 to " << maxDist << std::endl;

  CountsMap m;
  for(int D = 2; D <= maxDist; D++) {
    // Read files and handle batches one by one:
    BitReader reader1(base, left+base, D, false, 0);
    BitReader reader2(base, right+base, D, true, left+base);
    ReportMap rm;

    while(reader1.next(rm) && reader2.next(rm)) {
      bool bs180, bs90;

      // Mix of all tokens:
      for(ReportMap::const_iterator itA = rm.begin(); itA != rm.end(); itA++) {
	const uint64_t token1 = itA->first;
	for(ReportMap::const_iterator itB = rm.begin(); itB != rm.end(); itB++) {
	  const uint64_t token2 = itB->first;

	  Counts c;
	  // Match all connectivities:
	  for(std::vector<Report>::const_iterator it1 = itA->second.begin(); it1 != itA->second.end(); it1++) {
	    const Report &report1 = *it1;
	    bs180 = report1.baseSymmetric180;
	    bs90 = report1.baseSymmetric90;
	    for(std::vector<Report>::const_iterator it2 = itB->second.begin(); it2 != itB->second.end(); it2++) {
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
	  // Merge tokens:
	  uint64_t token = 10*token2 + base;
	  uint64_t stak = token1;
	  while(stak > 0) {
	    token = 10 * token + (stak % 10);
	    stak /= 10;
	  }
	  // Count:
	  if(m.find(token) == m.end())
	    m[token] = c;
	  else
	    m[token] += c;
	} // for rm -> token2

	// Single sided counting for cross checking:
	Counts c;
	for(std::vector<Report>::const_iterator it1 = itA->second.begin(); it1 != itA->second.end(); it1++) {
	  const Report &report = *it1;
	  bool connected = true;
	  for(int i = 0; i < base-1; i++) {
	    if(report.colors[i] != 0) {
	      connected = false;
	      break;
	    }
	  }
	  if(!connected)
	    continue;
	  c += report.counts;
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
	// Count:
	uint64_t token = 10*token1 + base;
	if(m.find(token) == m.end())
	  m[token] = c;
	else
	  m[token] += c;
      } // for rm -> token1
      rm.clear();
    } // while(reader1.next(rm) && reader2.next(rm))
  } // for d = 2 .. maxDist

  std::cout << "Counts:" << std::endl;
  for(CountsMap::const_iterator it = m.begin(); it != m.end(); it++) {
    uint64_t token = it->first;
    Counts c = it->second;
    std::cout << " <" << token << ">: " << c << std::endl;
  }
  
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
