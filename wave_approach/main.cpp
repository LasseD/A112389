#include <iostream>
#include <stdlib.h>
#include <assert.h>
#include <sstream>
#include <fstream>
#include "rectilinear.h"

using namespace rectilinear;

// Parse input parameter as unsigned integer:
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

  g++ -std=c++11 -O3 -DNDEBUG *.cpp -o run.o

  This code base uses an approach inspired by Eilers (2016) to compute all LEGO models of n 2x4 bricks:
  For a given brick in the first (base) layer:
  Let the base brick be the first "wave"
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
void printUsage() {
  std::cout << "Usage: [RPST] [parameters...]" << std::endl;
  std::cout << "R: Compute a single refinement. Parameters: REFINEMENT [THREADS]" << std::endl;
  std::cout << "P: Compute precomputations. Parameters: REFINEMENT MAX_DIST [THREADS]. Results of precomputations are saved to files in folder /base_<BASE>_size_<SIZE_TOTAL>_refinement_REFINEMENT. THREADS-1 worker threads will be spawned" << std::endl;
  std::cout << "S: Sum precomputations for a refinement. Parameters: LEFT BASE RIGHT MAX_DIST" << std::endl;
  std::cout << "T: Test precomputations against previous results. Parameters: BASE REFINEMENT MIN_DIST MAX_DIST FOLDER_SUFFIX" << std::endl;
  std::cout << "X: Run a test suite with regression tests. No parameters needed." << std::endl;
}

int runSumPrecomputations(int leftToken, int base, int rightToken, int maxDist) {
  std::cout << "Summing precomputations, leftToken=" << leftToken << ", base=" << base << ", rightToken=" << rightToken << ", maxDist=" << maxDist << std::endl;
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

  const Combination maxL(Combination::reverseToken(leftToken));
  const Combination maxR(Combination::reverseToken(rightToken));
  Counts counts, countsLeft, countsRight;
  for(int D = 2; D <= maxDist; D++) {
    // Read files and handle batches one by one:
    BitReader reader1(maxL, D, "");
    BitReader reader2(maxR, D, "");

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

      countsLeft += cl;
      countsRight += cr;

      l.clear();
      r.clear();
    } // while(reader1.next(rm) && reader2.next(rm))
  } // for d = 2 .. maxDist

  if(!Combination::checkCounts(token, counts))
    return 1;
  if(!Combination::checkCounts(leftToken, countsLeft))
    return 2;
  if(!Combination::checkCounts(rightToken, countsRight))
    return 3;

  return 0;
}

int runSumPrecomputations(int argc, char** argv) {
  if(argc < 6) {
    printUsage();
    return 1;
  }
  int leftToken = get(argv[2]);
  int base = get(argv[3]);
  int rightToken = get(argv[4]);
  int maxDist = get(argv[5]);
  return runSumPrecomputations(leftToken, base, rightToken, maxDist);
}

int runRefinement(int argc, char** argv) {
  if(argc < 3) {
    printUsage();
    return 1;
  }
  std::chrono::time_point<std::chrono::steady_clock> timeStart { std::chrono::steady_clock::now() };

  uint64_t token = get(argv[2]);
  Combination maxCombination(token);

  int threads = argc > 3 ? get(argv[3]) : std::thread::hardware_concurrency();

  BrickPlane neighbours[MAX_BRICKS];
  for(uint8_t i = 0; i < MAX_BRICKS; i++)
      neighbours[i].reset();

  Counts counts;

  Combination combination;
  NonEncodingCombinationBuilder b(combination, 0, 1, neighbours, maxCombination);
  std::cout << "Counting for <" << token << "> of size " << (int)maxCombination.size << std::endl;
  counts = b.buildSplit(threads);
  Combination::checkCounts(token, counts);

  std::chrono::duration<double, std::ratio<1> > duration = std::chrono::duration_cast<std::chrono::duration<double, std::ratio<1> > >(std::chrono::steady_clock::now() - timeStart);
  std::cout << "Computation time: " << duration.count() << " seconds" << std::endl;

  // Write to file:
  std::stringstream ss; ss << "output_" << token << ".txt";
  std::ofstream fileStream(ss.str().c_str());
  fileStream << "<" << token << "> " << counts << std::endl;
  fileStream << "Computation time: " << duration.count() << " seconds" << std::endl;
  fileStream << std::endl;

  fileStream << "Code line for sums-for-token.py:" << std::endl;
  fileStream << "    '" << token << "', " << counts.all << ", " << counts.symmetric180 << "," << std::endl;
  fileStream << std::endl;

  fileStream << "Code line for Combination::setupKnownCounts():" << std::endl;
  fileStream << "    m[" << token << "] = Counts(" << counts.all << ", " << counts.symmetric180 << ", " << counts.symmetric90 << ");" << std::endl;

  fileStream.flush();
  fileStream.close();
  return 0;
}

int runPrecomputations(int argc, char** argv) {
  if(argc < 4) {
    printUsage();
    return 1;
  }
  std::chrono::time_point<std::chrono::steady_clock> timeStart { std::chrono::steady_clock::now() };

  uint64_t token = get(argv[2]);
  Combination maxCombination(token);

  uint8_t base = maxCombination.layerSizes[0];
  if(base < 2) {
    std::cerr << "Unsupported base of refinement: " << (int)base << std::endl;
    return 2;
  }
  int maxDist = get(argv[3]);
  int threads = argc > 4 ? get(argv[4]) : std::thread::hardware_concurrency();

  std::cout << "Precomputing refinement " << token << " up to distance of " << maxDist << " using " << threads << " threads" << std::endl;
  Lemma3 lemma3(base, threads, maxCombination);
  lemma3.precompute(maxDist);

  std::chrono::duration<double, std::ratio<1> > duration = std::chrono::duration_cast<std::chrono::duration<double, std::ratio<1> > >(std::chrono::steady_clock::now() - timeStart);
  std::cout << "Total precomputation time: " << duration.count() << " seconds" << std::endl;
  return 0;
}

int runPrecomputationComparison(int argc, char** argv) {
  if(argc < 7) {
    printUsage();
    return 1;
  }
  int base = get(argv[2]);
  int token = get(argv[3]);
  int d = get(argv[4]);
  int D = get(argv[5]);
  std::string suffix(argv[6]);

  const Combination maxC(token);
  for(; d <= D; d++) {
    BitReader reader1(maxC, d, suffix);
    BitReader reader2(maxC, d, "");

    std::vector<Report> r1, r2;
    long cnt = 0;
    while(reader1.next(r1)) {
#ifdef DEBUG
      std::cout << " " << suffix << " read, now reading from other" << std::endl;
#endif
      bool ok = reader2.next(r2);
#ifdef DEBUG
      std::cout << " Other read" << std::endl;
#endif
      cnt++;
      if(!ok) {
	std::cerr << "Base pair mismatch for d=" << d << std::endl;
	return 4;
      }
      if(r1.size() != r2.size()) {
	std::cerr << "Report size does not match!" << std::endl;
	std::cerr << "Sizes: " << r1.size() << " / " << r2.size() << std::endl;
	std::cerr << "Bases: " << r1[0].c << " / " << r2[0].c << std::endl;
	return 5;
      }
      bool first = true;
      CountsMap m1, m2;
      Base b1, b2;
      bool s180, s90, t180, t90;
      for(std::vector<Report>::const_iterator it = r1.begin(); it != r1.end(); it++) {
	const Report &report = *it;
	if(first) {
	  b1 = report.c;
#ifdef DEBUG
	  std::cout << "b1 " << b1 << std::endl;
#endif
	  s180 = report.baseSymmetric180;
	  s90 = report.baseSymmetric90;
	  first = false;
	}
	int t = 1;
	for(int i = 0; i < base-1; i++) {
	  t = 10 * t + (report.colors[i]+1);
	}
	m1[t] = report.counts;
      }
      first = true;
      for(std::vector<Report>::const_iterator it = r2.begin(); it != r2.end(); it++) {
	const Report &report = *it;
	if(first) {
	  b2 = report.c;
#ifdef DEBUG
	  std::cout << "b2 " << b2 << std::endl;
#endif
	  t180 = report.baseSymmetric180;
	  t90 = report.baseSymmetric90;
	  first = false;
	}
	int t = 1;
	for(int i = 0; i < base-1; i++) {
	  t = 10 * t + (report.colors[i]+1);
	}
	m2[t] = report.counts;
      }
      if(!(b1 == b2) || s180 != t180 || s90 != t90) {
	std::cerr << "Base mismatch! " << b1 << "(" << suffix << ") != " << b2 << std::endl;
	std::cerr << " 180?: " << s180 << "/" << t180 << " 90?: " << s90 << "/" << t90 << std::endl;
	std::cerr << "Index " << cnt << std::endl;
	return 6;
      }
      for(CountsMap::const_iterator it1 = m1.begin(), it2 = m2.begin(); it1 != m1.end(); it1++, it2++) {
	if(it1->first != it2->first || it1->second != it2->second) {
	  std::cerr << "Counts mismatch!" << std::endl;
	  std::cerr << " Base: " << b1 << std::endl;
	  std::cerr << " tokens: " << it1->first << " / " << it2->first << std::endl;
	  std::cerr << " counts: " << it1->second << " / " << it2->second << std::endl;
	  return 7;
	}
      }
      r1.clear();
      r2.clear();
    }
  }
  return 0;
}

int runRegressionTests() {
#ifndef DEBUG
  std::cerr << "Please compile with -DDEBUG for test suite to test properly!" << std::endl;
  return 1;
#endif
  CountsMap m;
  Combination::setupKnownCounts(m);

  std::chrono::time_point<std::chrono::steady_clock> timeStart { std::chrono::steady_clock::now() };

  // Build refinements:
  uint8_t layerSizes[MAX_HEIGHT];
  BrickPlane neighbours[MAX_BRICKS];
  for(uint8_t i = 0; i < MAX_BRICKS; i++)
    neighbours[i].reset();

  for(CountsMap::const_iterator it = m.begin(); it != m.end(); it++) {
    uint64_t token = Combination::reverseToken(it->first);
    uint8_t size = Combination::sizeOfToken(token);
    if(size >= 9 || it->second.all > 3100000000)
      continue;
    Combination::getLayerSizesFromToken(token, layerSizes);
    uint8_t height = Combination::heightOfToken(token);
    bool ok = true;
    for(int i = 1; i < height-1; i++)
      if(layerSizes[i] == 1) {
	ok = false;
	break;
      }
    if(!ok)
      continue; // Single brick layer in refinement: Not supported.

    std::cout << "Testing computation of refinement " << token << std::endl;
    Combination maxCombination(token);
    Combination combination;
    NonEncodingCombinationBuilder b(combination, 0, 1, neighbours, maxCombination);
    Counts xCheck = b.buildSplit(3); // 2 worker threads
    if(xCheck != it->second) {
      std::cerr << "Exiting due to cross check error: " << xCheck << " != " << it->second << std::endl;
      return 2;
    }
  }
  //return 0;
  // Build precomputations max dist 24 (<41> to 8):
  int tokens[6] = {21, 22, 23, 221, 31, 41};
  for(int i = 0; i < 6; i++) {
    // Run precomputations
    uint64_t token = tokens[i];
    int maxDist = i > 2 ? 16 : 8;
    Combination maxCombination(token);
    uint8_t base = maxCombination.layerSizes[0];
    Lemma3 lemma3(base, 3, maxCombination);
    lemma3.precompute(maxDist, true);

    // Sum together to check results
    token = Combination::reverseToken(token);
    int left = token / 10;
    int right = Combination::reverseToken(left);
    int exitCode = runSumPrecomputations(left, base, right, maxDist);
    if(exitCode != 0) {
      std::cerr << "Error during sums from precomputations" << std::endl;
      return exitCode;
    }
  }

  std::chrono::duration<double, std::ratio<1> > duration = std::chrono::duration_cast<std::chrono::duration<double, std::ratio<1> > >(std::chrono::steady_clock::now() - timeStart);
  std::cout << "Test suite completed in " << duration.count() << " seconds" << std::endl;

  return 0;
}

int main(int argc, char** argv) {
  if(argc < 2) {
    printUsage();
    return 1;
  }
  char function = argv[1][0];

  switch(function) {
  case 'R':
    return runRefinement(argc, argv);
  case 'P':
    return runPrecomputations(argc, argv);
  case 'S':
    return runSumPrecomputations(argc, argv);
  case 'T':
    return runPrecomputationComparison(argc, argv);
  case 'X':
    return runRegressionTests();
  default:
    printUsage();
    return 1;
  }
}
