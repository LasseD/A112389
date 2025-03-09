#include <set>
#include <vector>
#include <algorithm>
#include <assert.h>
#include <chrono>
#include <thread>
#include <sstream>

#include "bfs.h"

#ifdef PROFILING
void Profiler::countInvocation(const std::string &s) {
  if(invocationCounts == NULL)
    invocationCounts = new InvocationMap();
  if(invocationCounts->find(s) == invocationCounts->end()) {
    invocationCounts->insert(std::pair<std::string,uint64_t>(s, 1));
  }
  else {
    (*invocationCounts)[s]++;
  }
}
void Profiler::reportInvocations() {
  std::vector<InvocationPair> v;
  for(InvocationMap::const_iterator it = invocationCounts->begin(); it != invocationCounts->end(); it++) {
    v.push_back(InvocationPair(it->second, it->first));
  }
  std::sort(v.rbegin(), v.rend());
  std::cout << "Invocations:" << std::endl;
  for(std::vector<InvocationPair>::const_iterator it = v.begin(); it != v.end(); it++) {
    std::cout << " " << it->first << "\t" << it->second << std::endl;
  }
}
#endif

namespace rectilinear {

  Counts::Counts() : all(0), symmetric180(0), symmetric90(0) {
#ifdef PROFILING
    Profiler::countInvocation("Counts::Counts()");
#endif
  }
  Counts::Counts(uint64_t all, uint64_t symmetric180, uint64_t symmetric90) : all(all), symmetric180(symmetric180), symmetric90(symmetric90) {
#ifdef PROFILING
    Profiler::countInvocation("Counts::Counts(uint64_t, uint64_t, uint64_t)");
#endif
  }
  Counts::Counts(const Counts& c) : all(c.all), symmetric180(c.symmetric180), symmetric90(c.symmetric90) {
#ifdef PROFILING
    Profiler::countInvocation("Counts::Counts(Counts&)");
#endif
  }
  Counts& Counts::operator +=(const Counts& c) {
#ifdef PROFILING
    Profiler::countInvocation("Counts::operator +=");
#endif
    all += c.all;
    symmetric180 += c.symmetric180;
    symmetric90 += c.symmetric90;
    return *this;
  }
  Counts Counts::operator -(const Counts& c) {
#ifdef PROFILING
    Profiler::countInvocation("Counts::operator -");
#endif
    return Counts(all-c.all, symmetric180-c.symmetric180, symmetric90-c.symmetric90);
  }
  std::ostream& operator << (std::ostream &os,const Counts &c) {
    os << c.all;
    if(c.symmetric180 > 0)
      os << " (" << c.symmetric180 << ")";
    if(c.symmetric90 > 0)
      os << " {" << c.symmetric90 << "}";
    return os;
  }
  void Counts::reset() {
#ifdef PROFILING
    Profiler::countInvocation("Counts::reset()");
#endif
    all = 0;
    symmetric180 = 0;
    symmetric90 = 0;
  }

  Brick::Brick() : isVertical(true), x(PLANE_MID), y(PLANE_MID) {
#ifdef PROFILING
    Profiler::countInvocation("Brick::Brick()");
#endif
  }
  Brick::Brick(bool iv, int8_t x, int8_t y) : isVertical(iv), x(x), y(y) {	
#ifdef PROFILING
    Profiler::countInvocation("Brick::Brick(bool, int8_t, int8_t)");
#endif
  }
  Brick::Brick(const Brick &b) : isVertical(b.isVertical), x(b.x), y(b.y) {
#ifdef PROFILING
    Profiler::countInvocation("Brick::Brick(Brick&)");
#endif
  }

  bool Brick::operator <(const Brick& b) const {
#ifdef PROFILING
    Profiler::countInvocation("Brick::operator <");
#endif
    if(isVertical != b.isVertical)
      return isVertical < b.isVertical;
    if(x != b.x)
      return x < b.x;
    return y < b.y;
  }
  bool Brick::operator ==(const Brick& b) const {
#ifdef PROFILING
    Profiler::countInvocation("Brick::operator ==");
#endif
    return x == b.x && y == b.y && isVertical == b.isVertical;
  }
  bool Brick::operator !=(const Brick& b) const {
#ifdef PROFILING
    Profiler::countInvocation("Brick::operator !=");
#endif
    return !(*this == b);
  }
  std::ostream& operator << (std::ostream &os,const Brick &b) {
    os << (b.isVertical?"|":"=") << (int)b.x << "," << (int)b.y << (b.isVertical?"|":"=");
    return os;
  }
  bool Brick::intersects(const Brick &b) const {
#ifdef PROFILING
    Profiler::countInvocation("Brick::intersects()");
#endif
    if(isVertical != b.isVertical)
      return DIFFLT(b.x, x, 3) && DIFFLT(b.y, y, 3);
    if(isVertical)
      return DIFFLT(b.x, x, 2) && DIFFLT(b.y, y, 4);
    else
      return DIFFLT(b.x, x, 4) && DIFFLT(b.y, y, 2);
  }
  void Brick::mirror(Brick &b, const int8_t &cx, const int8_t &cy) const {
#ifdef PROFILING
    Profiler::countInvocation("Brick::mirror()");
#endif
    b.isVertical = isVertical;
    b.x = cx - x; // cx/2 + (cx/2 - x) = cx - x
    b.y = cy - y;
  }
  bool Brick::mirrorEq(const Brick &b, const int8_t &cx, const int8_t &cy) const {
#ifdef PROFILING
    Profiler::countInvocation("Brick::mirrorEq()");
#endif
    if(b.isVertical != isVertical)
      return false;
    if(b.x != cx - x)
      return false;
    return b.y == cy - y;
  }

  BrickPicker::BrickPicker(const std::vector<LayerBrick> &v, int vIdx, const int numberOfBricksToPick, LayerBrick *bricks, const int bricksIdx) : v(v), vIdx(vIdx-1), numberOfBricksToPick(numberOfBricksToPick), bricksIdx(bricksIdx), bricks(bricks), inner(NULL) {
#ifdef PROFILING
    Profiler::countInvocation("BrickPicker::BrickPicker()");
#endif
#ifdef TRACE
    std::cout << "   BrickPicker for " << numberOfBricksToPick << " bricks, starting at " << v[vIdx].BRICK << " at layer " << (int)v[vIdx].LAYER << " created with " << bricksIdx << " previous bricks added " <<std::endl;
#endif
  }

  BrickPicker::~BrickPicker() {
#ifdef PROFILING
    Profiler::countInvocation("BrickPicker::~BrickPicker()");
#endif
    if(inner != NULL) {
      delete inner;
    }
  }

  bool BrickPicker::checkVIdx() const {
#ifdef PROFILING
    Profiler::countInvocation("BrickPicker::checkVIdx()");
#endif
    // Check for colissions against placed bricks:
    for(int i = 0; i < bricksIdx; i++) {
      if(bricks[i].LAYER == v[vIdx].LAYER &&
	 bricks[i].BRICK.intersects(v[vIdx].BRICK)) {
	return false;
      }
    }
    return true;
  }

  void BrickPicker::nextVIdx() {
#ifdef PROFILING
    Profiler::countInvocation("BrickPicker::nextVIdx()");
#endif
    do {
      vIdx++;
      if(vIdx >= (int)v.size())
	return; // Out of bounds!
      bricks[bricksIdx] = v[vIdx];
    }
    while(!checkVIdx());
  }

  bool BrickPicker::next() {
#ifdef PROFILING
    Profiler::countInvocation("BrickPicker::next()");
#endif
    if(numberOfBricksToPick == 1) {
      nextVIdx();
      return vIdx < (int)v.size();
    }

    if(inner != NULL) {
      if(inner->next())
	return true;
      else {
	delete inner;
	inner = NULL;
      }
    }

    while(true) {
      nextVIdx();
      if(vIdx + numberOfBricksToPick - 1 >= (int)v.size()) {
	return false;
      }
      inner = new BrickPicker(v, vIdx+1, numberOfBricksToPick-1, bricks, bricksIdx+1);
      if(inner->next()) {
	return true;
      }
      delete inner;
      inner = NULL;
    }
  }

  void BrickPlane::unsetAll() {
#ifdef PROFILING
    Profiler::countInvocation("BrickPlane::unsetAll()");
#endif
    for(uint8_t i = 0; i < 2; i++) {
      for(uint8_t j = 0; j < PLANE_WIDTH; j++) {
	for(uint8_t k = 0; k < PLANE_WIDTH; k++) {
	  bricks[i][j][k] = false;
	}
      }
    }
  }

  void BrickPlane::set(const Brick &b) {
#ifdef PROFILING
    Profiler::countInvocation("BrickPlane::set()");
#endif
    bricks[b.isVertical][b.x][b.y] = true;
  }

  void BrickPlane::unset(const Brick &b) {
#ifdef PROFILING
    Profiler::countInvocation("BrickPlane::unset()");
#endif
    bricks[b.isVertical][b.x][b.y] = false;
  }

  bool BrickPlane::contains(const Brick &b) {
#ifdef PROFILING
    Profiler::countInvocation("BrickPlane::contains()");
#endif
    return bricks[b.isVertical][b.x][b.y];
  }
  
  Combination::Combination() : height(1), size(1) {
#ifdef PROFILING
    Profiler::countInvocation("Combination::Combination()");
#endif
    bricks[0][0] = FirstBrick;
    layerSizes[0] = 1;
    history[0] = BrickIdentifier(0,0);
    for(uint8_t i = 1; i < MAX_BRICKS; i++)
      layerSizes[i] = 0;
  }
  Combination::Combination(const Combination &b) {
#ifdef PROFILING
    Profiler::countInvocation("Combination::Combination(Combination &)");
#endif
    copy(b);
  }

  bool Combination::operator ==(const Combination& b) const {
#ifdef PROFILING
    Profiler::countInvocation("Combination::operator ==");
#endif
    assert(height == b.height);
    for(uint8_t i = 0; i < height; i++) {
      assert(layerSizes[i] == b.layerSizes[i]);
      uint8_t s = layerSizes[i];
      for(uint8_t j = 0; j < s; j++) {
	if(bricks[i][j] != b.bricks[i][j]) {
	  return false;
	}
      }
    }
    return true;
  }

  std::ostream& operator << (std::ostream &os, const Combination &b) {
    os << "<";
    for(uint8_t i = 0; i < b.height; i++) {
      os << (int)b.layerSizes[i];
    }
    os << "> combination:";
    for(uint8_t i = 0; i < b.height; i++) {
      for(uint8_t j = 0; j < b.layerSizes[i]; j++) {
	os << " " << b.bricks[i][j] << " ";
      }
    }
    return os;
  }

  void Combination::copy(const Combination &b) {
#ifdef PROFILING
    Profiler::countInvocation("Combination::copy()");
#endif
    height = b.height;
    size = b.size;
    for(uint8_t i = 0; i < MAX_BRICKS; i++)
      layerSizes[i] = b.layerSizes[i];
    for(uint8_t i = 0; i < height; i++) {
      for(uint8_t j = 0; j < layerSizes[i]; j++) {
	bricks[i][j] = b.bricks[i][j];
      }
    }
    for(uint8_t i = 0; i < size; i++) {
      history[i] = b.history[i];
    }
  }

  void Combination::sortBricks() {
#ifdef PROFILING
    Profiler::countInvocation("Combination::sortBricks()");
#endif
    for(uint8_t layer = 0; layer < height; layer++) {
      uint8_t layerSize = layerSizes[layer];
      if(layerSize > 1) {
	std::sort(bricks[layer], &bricks[layer][layerSize]);
      }
    }
  }

  void Combination::translateMinToOrigo() {
#ifdef PROFILING
    Profiler::countInvocation("Combination::translateMinToOrigo()");
#endif
    int8_t minx = 127, miny = 127;

    for(uint8_t i = 0; i < layerSizes[0]; i++) {
      Brick &b = bricks[0][i];
      if(b.isVertical) {
	// Vertical bricks in layer 0 can be 'min':
	if(b.x < minx || (b.x == minx && b.y < miny)) {
	  minx = b.x;
	  miny = b.y;
	}
      }
    }
    // Move all in relation to smallest min:
    for(uint8_t i = 0; i < height; i++) {
      for(uint8_t j = 0; j < layerSizes[i]; j++) {
	bricks[i][j].x -= minx;
	bricks[i][j].y -= miny;
      }
    }
  }

  void Combination::rotate90() {
#ifdef PROFILING
    Profiler::countInvocation("Combination::rotate90()");
#endif
    for(uint8_t i = 0; i < height; i++) {
      for(uint8_t j = 0; j < layerSizes[i]; j++) {
	const Brick &b = bricks[i][j];
	bricks[i][j] = Brick(!b.isVertical, b.y, -b.x);
      }
    }
    translateMinToOrigo();
    sortBricks();
  }

  void Combination::getLayerCenter(const uint8_t layer, int16_t &cx, int16_t &cy) const {
#ifdef PROFILING
    Profiler::countInvocation("Combination::getLayerCenter()");
#endif
    cx = 0;
    cy = 0;
    for(uint8_t i = 0; i < layerSizes[layer]; i++) {
      cx += bricks[layer][i].x;
      cy += bricks[layer][i].y;
    }
    cx *= 2;
    cy *= 2;
    cx /= layerSizes[layer];
    cy /= layerSizes[layer];
  }

  bool Combination::isLayerSymmetric(const uint8_t layer, const int16_t &cx, const int16_t &cy) const {
#ifdef PROFILING
    Profiler::countInvocation("Combination::isLayerSymmetric()");
#endif
    const uint8_t layerSize = layerSizes[layer];
    if(layerSize == 1) {
      const Brick &b = bricks[layer][0];
      return b.x*2 == cx && b.y*2 == cy;
    }
    else if(layerSize == 2) {
      return bricks[layer][0].mirrorEq(bricks[layer][1], cx, cy);
    }
    else if(layerSize == 3) {
      const Brick &b0 = bricks[layer][0];
      const Brick &b1 = bricks[layer][1];
      const Brick &b2 = bricks[layer][2];
      if(b0.x*2 == cx && b0.y*2 == cy) {
	return b1.mirrorEq(b2, cx, cy);
      }
      if(b1.x*2 == cx && b1.y*2 == cy) {
	return b0.mirrorEq(b2, cx, cy);
      }
      if(b2.x*2 == cx && b2.y*2 == cy) {
	return b0.mirrorEq(b1, cx, cy);
      }
      return false; // None in origo
    }
    /*else if(layerSize == 4) {
      const Brick &b0 = bricks[layer][0];
      const Brick &b1 = bricks[layer][1];
      const Brick &b2 = bricks[layer][2];
      const Brick &b3 = bricks[layer][3];
      if(b0.mirrorEq(b1, cx, cy) && b2.mirrorEq(b3, cx, cy))
	return true;
      if(b0.mirrorEq(b2, cx, cy) && b1.mirrorEq(b3, cx, cy))
	return true;
      if(b0.mirrorEq(b3, cx, cy) && b1.mirrorEq(b2, cx, cy))
	return true;
      return false;
    }*/
    else {
#ifdef PROFILING
    Profiler::countInvocation("Combination::isLayerSymmetric::FALLBACK");
#endif
      std::set<Brick> seen;
      Brick mirror;
      for(uint8_t i = 0; i < layerSize; i++) {
	const Brick &b = bricks[layer][i];
	if(b.x*2 == cx && b.y*2 == cy)
	  continue; // Skip brick in center
	b.mirror(mirror, cx, cy);
	if(seen.find(mirror) == seen.end()) {
	  seen.insert(b);
	}
	else {
	  seen.erase(mirror);
	}
      }
      return seen.empty();
    }
  }

  /* 
  g++ -std=c++11 -O3 *.cpp -DNDEBUG -o run.o && time ./run.o 6
     Total for size 6: 915103765 (15682)
  ./run.o 6  6370.01s user 0.23s system 67% cpu 9:06.73 total
  ./run.o 6  248.01s user 0.19s system 98% cpu 4:11.30 total // custom is180Symmetric()
  ./run.o 6  214.07s user 0.12s system 99% cpu 3:36.03 total // fast size 1 and 2 check
  ./run.o 6  200.56s user 0.11s system 99% cpu 3:21.93 total // fast size 3 check // Best!
  ./run.o 6  200.62s user 0.13s system 99% cpu 3:22.39 total // fast size 4 check!
  ./run.o 6  205.36s user 0.16s system 98% cpu 3:27.81 total // removing pre-check for layer center
  ./run.o 6  185.23s user 0.11s system 99% cpu 3:06.46 total // using removeLastBrick in CombinationBuilder
  ./run.o 6  101.39s user 0.04s system 99% cpu 1:42.08 total // Sorting vectors instead of using map<LayerBrick>
  ./run.o 6  100.08s user 0.06s system 99% cpu 1:40.58 total // Initialize all layerSize values to 0
  ./run.o 6  96.16s user 0.11s system 98% cpu 1:37.26 total // Using BrickPlane
  ./run.o 6  75.16s user 0.03s system 99% cpu 1:15.84 total // Shared BrickPlane
  ./run.o 6  71.80s user 0.03s system 96% cpu 1:14.63 total // Use canBeSymmetric180

  Performance of code by Eilers: 2.33 seconds for size 6. This goal should be reached.

  vs old rectilinear algorithm (no countX2):
  g++ -std=c++11 -O3 *.cpp -DNDEBUG -o runCompare.o && time ./runCompare.o ->
             211.85s user 0.18s system 98% cpu 3:34.61 total
  */
  bool Combination::is180Symmetric() const {
#ifdef PROFILING
    Profiler::countInvocation("Combination::is180Symmetric()");
#endif
    int16_t cx0, cy0;
    getLayerCenter(0, cx0, cy0);

    if(!isLayerSymmetric(0, cx0, cy0)) {
      return false;
    }

    for(uint8_t i = 1; i < height; i++) {
      int16_t cx1, cy1;
      getLayerCenter(i, cx1, cy1);

      if(cx0 != cx1 || cy0 != cy1) {
	return false;
      }
      if(!isLayerSymmetric(i, cx0, cy0)) {
	return false;
      }
    }
    return true;
  }

  bool Combination::is90Symmetric() const {
    for(uint8_t i = 0; i < height; i++) {
      if(layerSizes[i] % 4 != 0)
	return false;
    }
#ifdef PROFILING
    Profiler::countInvocation("Combination::is90Symmetric()");
#endif
    Combination rotated(*this);
    rotated.rotate90();
    Combination dis(*this);
    dis.translateMinToOrigo();
    dis.sortBricks();      
    return rotated == dis;
  }

  void Combination::addBrick(const Brick &b, const uint8_t layer) {
#ifdef PROFILING
    Profiler::countInvocation("Combination::addBrick()");
#endif
    const int8_t &layerSize = layerSizes[layer];
    history[size] = BrickIdentifier(layer, layerSize);
    bricks[layer][layerSize] = b;

    size++;
    if(layer == height)
      height++;
    layerSizes[layer]++;
  }

  void Combination::removeLastBrick() {
#ifdef PROFILING
    Profiler::countInvocation("Combination::removeLastBrick()");
#endif
    size--;
    const uint8_t &layer = history[size].first;

    layerSizes[layer]--;
    if(layerSizes[layer] == 0)
      height--;
  }

  int Combination::getTokenFromLayerSizes() const {
#ifdef PROFILING
    Profiler::countInvocation("Combination::getTokenFromLayerSizes()");
#endif
    int ret = 0;
    for(uint8_t i = 0; i < height; i++) {
      ret = (ret * 10) + layerSizes[i];
    }
    return ret;
  }

  int Combination::reverseToken(int token) {
#ifdef PROFILING
    Profiler::countInvocation("Combination::reverseToken()");
#endif
    int ret = 0;
    while(token > 0) {
      ret = (ret * 10) + (token % 10);
      token /= 10;
    }
    return ret;
  }

  uint8_t Combination::heightOfToken(int token) {
#ifdef PROFILING
    Profiler::countInvocation("Combination::heightOfToken()");
#endif
    uint8_t ret = 0;
    while(token > 0) {
      ret++;
      token = token/10;
    }
    return ret;
  }

  uint8_t Combination::sizeOfToken(int token) {
#ifdef PROFILING
    Profiler::countInvocation("Combination::sizeOfToken()");
#endif
    uint8_t ret = 0;
    while(token > 0) {
      ret += token % 10;
      token = token/10;
    }
    return ret;
  }

  void Combination::getLayerSizesFromToken(int token, uint8_t *layerSizes) {
#ifdef PROFILING
    Profiler::countInvocation("Combination::getLayerSizesFromToken()");
#endif
    uint8_t layers = 0;
    while(token > 0) {
      int size_add = token % 10;
      layerSizes[layers++] = size_add;
      token /= 10;
    }
    // Flip the layer sizes:
    for(uint8_t i = 0; i < layers/2; i++) {
      std::swap(layerSizes[i], layerSizes[layers-i-1]);
    }
  }  

  CombinationBuilder::CombinationBuilder(Combination &c,
					 const uint8_t waveStart,
					 const uint8_t waveSize,
					 const uint8_t maxSize,
					 uint8_t *maxLayerSizes) :
    baseCombination(c), waveStart(waveStart), waveSize(waveSize), maxSize(maxSize), indent(0), maxLayerSizes(maxLayerSizes) {
    neighbours = new BrickPlane[MAX_BRICKS];
    for(uint8_t i = 0; i < MAX_BRICKS; i++) {
      neighbours[i].unsetAll();
    }
#ifdef PROFILING
    Profiler::countInvocation("CombinationBuilder::CombinationBuilder()::SLOW");
#endif
  }

  CombinationBuilder::CombinationBuilder(Combination &c,
					 const uint8_t waveStart,
					 const uint8_t waveSize,
					 const uint8_t maxSize,
					 const uint8_t indent,
					 BrickPlane *neighbours,
					 uint8_t *maxLayerSizes) :
    baseCombination(c), waveStart(waveStart), waveSize(waveSize), maxSize(maxSize), indent(indent), neighbours(neighbours), maxLayerSizes(maxLayerSizes) {
#ifdef PROFILING
    Profiler::countInvocation("CombinationBuilder::CombinationBuilder()::FAST");
#endif
  }

  void CombinationBuilder::findPotentialBricksForNextWave(std::vector<LayerBrick> &v) {
#ifdef PROFILING
    Profiler::countInvocation("CombinationBuilder::findPotentialBricksForNextWave()");
#endif
    // Find all potential neighbours above and below all in wave:
    for(uint8_t i = 0; i < waveSize; i++) {
      const BrickIdentifier &bi = baseCombination.history[waveStart+i];
      const uint8_t &layer = bi.first;
      const Brick &brick = baseCombination.bricks[layer][bi.second];

      for(int8_t layer2 = layer-1; layer2 <= layer+1; layer2+=2) {
	if(layer2 < 0) {
	  continue; // Do not allow building below base layer
	}
#ifdef REFINEMENT
	if(baseCombination.layerSizes[layer2] == maxLayerSizes[layer2]) {
	  continue; // Already at maximum allowed for layer!
	}
#endif

	// Add crossing bricks (one vertical, one horizontal):
	for(int x = -2; x < 3; x++) {
	  for(int y = -2; y < 3; y++) {
	    const Brick b(!brick.isVertical, brick.x+x, brick.y+y);
	    bool ok = true;
	    // If b connects to or overlaps a brick before the wave, then disregard:
	    for(uint8_t j = 0; j < waveStart; j++) {
	      const BrickIdentifier &bi3 = baseCombination.history[j];
	      const uint8_t layer3 = bi3.first;
	      if(layer3 == layer2 || layer3 == layer2+1 || layer3+1 == layer2) {
		const Brick &b2 = baseCombination.bricks[layer3][bi3.second];
		if(b.intersects(b2)) {
		  ok = false;
		  break;
		}
	      }
	    }
	    if(ok && !neighbours[layer2].contains(b)) {
	      neighbours[layer2].set(b);
	      v.push_back(LayerBrick(b, layer2));
	    }
	  }
	}

	// Add parallel bricks:
	int w = 4, h = 2;
	if(brick.isVertical) {
	  w = 2;
	  h = 4;
	}
	for(int y = -h+1; y < h; y++) {
	  for(int x = -w+1; x < w; x++) {
	    const Brick b(brick.isVertical, brick.x+x, brick.y+y);
	    bool ok = true;
	    // If b connects to or overlaps a brick before the wave, then disregard:
	    for(uint8_t j = 0; j < waveStart; j++) {
	      const BrickIdentifier &bi3 = baseCombination.history[j];
	      const uint8_t layer3 = bi3.first;
	      if(layer3 == layer2 || layer3 == layer2+1 || layer3+1 == layer2) {
		const Brick &b2 = baseCombination.bricks[layer3][bi3.second];
		if(b.intersects(b2)) {
		  ok = false;
		  break;
		}
	      }
	    }
	    if(ok && !neighbours[layer2].contains(b)) {
	      neighbours[layer2].set(b);
	      v.push_back(LayerBrick(b, layer2));
	    }
	  }
	}
      }
    }

    // Cleanup, so that neighbours can be shared by all:
    for(std::vector<LayerBrick>::const_iterator it = v.begin(); it != v.end(); it++) {
      neighbours[it->LAYER].unset(it->BRICK);
    }
  }

  bool CombinationBuilder::nextCombinationCanBeSymmetric180() {
#ifdef PROFILING
    Profiler::countInvocation("CombinationBuilder::nextCombinationCanBeSymmetric180()");
#endif
    bool canBeSymmetric180 = true;

    int fullLayers = 0; // Layers where all bricks are already placed: If they are non-symmetric or have misalignment of centers, then the resulting models cannot be symmetric
    int16_t cx0, cy0, cx1, cy1;
#ifdef REFINEMENT
    /*
      Speed up if model cannot be made symmetric
      when placing all leftToPlace bricks.
    */
    if(maxLayerSizes != NULL) {
      for(uint8_t i = 0; i < MAX_BRICKS; i++) {
	uint8_t diff = maxLayerSizes[i] - baseCombination.layerSizes[i];
	if(diff == 0) {
	  // Full layer: Check if can be symmetric:
	  if(fullLayers == 0) {
	    baseCombination.getLayerCenter(i, cx0, cy0);
	    if(!baseCombination.isLayerSymmetric(i, cx0, cy0)) {
	      canBeSymmetric180 = false;
	      break;
	    }
	  }
	  else {
	    baseCombination.getLayerCenter(i, cx1, cy1);
	    if(cx0 != cx1 || cy0 != cy1) {
	      canBeSymmetric180 = false;
	      break;
	    }
	    if(!baseCombination.isLayerSymmetric(i, cx0, cy0)) {
	      canBeSymmetric180 = false;
	      break;
	    }
	  }
	}
	else if(diff == 1 && baseCombination.layerSizes[i] % 2 == 1) {
	  // There is an odd number of bricks on the layer and 1 is added: If already symmetric, then the result cannot be symmtric (if even, a brick placed in center would uphold symmetry)
	  baseCombination.getLayerCenter(i, cx1, cy1);
	  if(baseCombination.isLayerSymmetric(i, cx1, cy1)) {
	    canBeSymmetric180 = false;
	    break;
	  }
	}
	fullLayers++;
      }
    }
#else
    /*
      Early check for symmetry:
     */
    bool solidLayers[MAX_BRICKS];
    for(uint8_t i = 0; i < baseCombination.height; i++) {
      solidLayers[i] = true;
    }
    for(uint8_t i = 0; i < waveSize; i++) {
      const BrickIdentifier &bi = baseCombination.history[waveStart+i];
      const uint8_t &layer = bi.first;
      if(layer > 0)
	solidLayers[i-1] = false;
      solidLayers[i] = false;
      solidLayers[i+1] = false;
    }
    for(uint8_t i = 0; i < baseCombination.height; i++) {
      if(!solidLayers[i])
	continue;
      if(fullLayers == 0) {
	baseCombination.getLayerCenter(i, cx0, cy0);
	if(!baseCombination.isLayerSymmetric(i, cx0, cy0)) {
	  canBeSymmetric180 = false;
	  break;
	}
      }
      else {
	baseCombination.getLayerCenter(i, cx1, cy1);
	if(cx0 != cx1 || cy0 != cy1) {
	  canBeSymmetric180 = false;
	  break;
	}
	if(!baseCombination.isLayerSymmetric(i, cx0, cy0)) {
	  canBeSymmetric180 = false;
	  break;
	}
      }
      fullLayers++;
    }
#endif

    return canBeSymmetric180;
  }

  /*
    BFS construction of models:
    Assume a non-empty wave:
     Pick 1..|wave| bricks from wave:
      Find next wave and recurse until model contains n bricks.
   */
  void CombinationBuilder::build() {
#ifdef PROFILING
    Profiler::countInvocation("CombinationBuilder::build()");
#endif
    std::vector<LayerBrick> v;
    findPotentialBricksForNextWave(v);

    uint8_t leftToPlace = maxSize - baseCombination.size;
    bool canBeSymmetric180 = nextCombinationCanBeSymmetric180();

    LayerBrick bricks[MAX_BRICKS];
    const bool spam = leftToPlace > 5;

    for(uint8_t toPick = 1; toPick <= leftToPlace; toPick++) {
      // Pick toPick from neighbours:
      BrickPicker picker(v, 0, toPick, bricks, 0);

      while(picker.next()) {
	if(spam) {
	  for(int chars = 0; chars < indent; chars++)
	    std::cout << " ";
	  std::cout << " Building " << (int)toPick << " on " << baseCombination << ": ";
	  for(uint8_t i = 0; i < toPick; i++) {
	    std::cout << "[" << bricks[i].BRICK << "@" << (int)bricks[i].LAYER << "]";
	  }
	  std::cout << std::endl;
	}
	
	// toPick bricks ready in bricks: Use as next wave!
	for(uint8_t i = 0; i < toPick; i++) {
	  baseCombination.addBrick(bricks[i].BRICK, bricks[i].LAYER);
	}

#ifdef REFINEMENT
	// Check if layer sizes are restricted:
	if(maxLayerSizes != NULL) {
	  bool ok = true;
	  for(uint8_t i = 0; i < baseCombination.height; i++) {
	    if(baseCombination.layerSizes[i] > maxLayerSizes[i]) {
	      ok = false; break;
	    }
	  }
	  if(!ok) {
	    for(uint8_t i = 0; i < toPick; i++) {
	      baseCombination.removeLastBrick();
	    }
	    continue; // Skip invalid combination
	  }
	}
#endif

	if(toPick == leftToPlace) {
	  int token = baseCombination.getTokenFromLayerSizes();
	  Counts cx;
	  cx.all++;
	  if(canBeSymmetric180 && baseCombination.is180Symmetric()) {
	    cx.symmetric180++;
	    if(baseCombination.is90Symmetric())
	      cx.symmetric90++;
	  }

	  if(counts.find(token) == counts.end())
	    counts[token] = cx;
	  else
	    counts[token] += cx;
	}
	else { // toPick < leftToPlace)
	  // Recurse:
	  CombinationBuilder builder(baseCombination, waveStart+waveSize, toPick, maxSize, indent+1, neighbours, maxLayerSizes);
	  builder.build();

	  for(CountsMap::iterator it = builder.counts.begin(); it != builder.counts.end(); it++) {
	    int token = it->first;
	    if(counts.find(token) == counts.end())
	      counts[token] = it->second;
	    else
	      counts[token] += it->second;
	  }
	}

	for(uint8_t i = 0; i < toPick; i++) {
	  baseCombination.removeLastBrick();
	}
	
      }
    }
  }

  void CombinationBuilder::report() {
#ifdef PROFILING
    Profiler::countInvocation("CombinationBuilder::report()");
#endif
    // Setup for reporting for Figure 7 in Eilers (2016):
    uint8_t layerSizes[MAX_BRICKS];
    Counts f, C[MAX_LAYER_SIZE], total;
    for(uint8_t i = 0; i < MAX_LAYER_SIZE; i++) {
      C[i].reset();
    }

    for(CountsMap::const_iterator it = counts.begin(); it != counts.end(); it++) {
      int token = it->first;
      //int reverseToken = Combination::reverseToken(token);
      uint8_t height = Combination::heightOfToken(token);
      Combination::getLayerSizesFromToken(token, layerSizes);
      Counts countsForToken(it->second);
      bool fat = true;
      for(uint8_t i = 1; i < height-1; i++) {
	if(layerSizes[i] < 2) {
	  fat = false;
	  break;
	}
      }

      if(layerSizes[0] == 1 && layerSizes[height-1] == 1 && fat) {
	f += countsForToken;
      }
      else if(layerSizes[height-1] > 1 && fat) {
	C[layerSizes[0]] += countsForToken;		       
      }
      countsForToken.all += countsForToken.symmetric180;
      countsForToken.all /= 2 * layerSizes[0];
      countsForToken.symmetric180 /= layerSizes[0];
      countsForToken.symmetric90 /= layerSizes[0] / 2;
      std::cout << " <" << token << "> " << countsForToken << std::endl;
      total += countsForToken;
    }
#ifndef REFINEMENT
    std::cout << "Total for size " << (int)maxSize << ": " << total << std::endl;

    // Reporting for Figure 7 in Eilers (2016):
    std::cout << std::endl << "Figure 7 numbers for Eilers (2016)" << std::endl;
    std::cout << "n\tf(n)\tf180(n)";
    for(uint8_t i = 1; i < maxSize; i++)
      std::cout << "\tC(n," << (int)i << ")\tC180(n," << (int)i << ")";
    std::cout << std::endl;
    std::cout << maxSize-1 << "\t" << f.all << "\t" << f.symmetric180;
    for(uint8_t i = 1; i < maxSize; i++)
      std::cout << "\t-\t-";
    std::cout << std::endl;
    std::cout << (int)maxSize << "\t-\t-";
    for(uint8_t i = 1; i < maxSize; i++)
      std::cout << "\t" << C[i].all << "\t" << C[i].symmetric180;
    std::cout << std::endl;
#endif
  }

  void CombinationBuilder::fast() {
#ifdef PROFILING
    Profiler::countInvocation("CombinationBuilder::fast()");
#endif

    /*
    unsigned int processor_count = std::thread::hardware_concurrency() - 2; // allow 2 processors for OS and other...

    std::cout << "   Splitting computation into " << processor_count << " threads" << std::endl;
    // Fill from readers in threads:
    std::vector<std::thread*> threads;
    std::vector<CombinationBuilder*> builders;

    for(unsigned int j = 0; j < processor_count; j++) {
      builders.push_back(new CombinationBuilder(*this));
      std::thread *t = new std::thread(&CombinationBuilder::build);
      threads.push_back(t);
    }
    for(unsigned int j = 0; j < processor_count; j++) {
      (*threads[j]).join();
      // TODO counts += builders[j]->counts;
      // TODO: Delete builder
      }*/
  }

} // namespace rectilinear
