#include <set>
#include <vector>
#include <algorithm>
#include <assert.h>
#include <chrono>
#include <thread>
#include <sstream>
#include <iostream>

#include "bfs.h"

namespace rectilinear {

  Counts::Counts() : all(0), symmetric180(0) {
  }
  Counts::Counts(uint64_t all, uint64_t symmetric180) : all(all), symmetric180(symmetric180) {
  }
  Counts::Counts(const Counts& c) : all(c.all), symmetric180(c.symmetric180) {
  }
  Counts& Counts::operator +=(const Counts& c) {
    all += c.all;
    symmetric180 += c.symmetric180;
    return *this;
  }
  Counts Counts::operator -(const Counts& c) const {
    return Counts(all-c.all, symmetric180-c.symmetric180);
  }
  Counts Counts::operator /(const int& v) const {
    assert(all % v == 0);
    assert(symmetric180 % v == 0);
    return Counts(all/v, symmetric180/v);
  }
  bool Counts::operator !=(const Counts& c) const {
    return all != c.all || symmetric180 != c.symmetric180;
  }
  bool Counts::operator ==(const Counts& c) const {
    return all == c.all && symmetric180 == c.symmetric180;
  }
  bool Counts::empty() const {
    return all == 0 && symmetric180 == 0;
  }
  std::ostream& operator << (std::ostream &os,const Counts &c) {
    os << c.all;
    if(c.symmetric180 > 0)
      os << " (" << c.symmetric180 << ")";
    return os;
  }
  void Counts::reset() {
    all = 0;
    symmetric180 = 0;
  }

  Brick::Brick() : isVertical(true), x(PLANE_MID), y(PLANE_MID) {
  }
  Brick::Brick(bool iv, int16_t x, int16_t y) : isVertical(iv), x(x), y(y) {	
  }
  Brick::Brick(const Brick &b) : isVertical(b.isVertical), x(b.x), y(b.y) {
  }

  bool Brick::operator <(const Brick& b) const {
    if(isVertical != b.isVertical)
      return isVertical < b.isVertical;
    if(x != b.x)
      return x < b.x;
    return y < b.y;
  }
  bool Brick::operator ==(const Brick& b) const {
    return x == b.x && y == b.y && isVertical == b.isVertical;
  }
  bool Brick::operator !=(const Brick& b) const {
    return !(*this == b);
  }
  std::ostream& operator << (std::ostream &os,const Brick &b) {
    os << (b.isVertical?"|":"=") << (int)b.x << "," << (int)b.y << (b.isVertical?"|":"=");
    return os;
  }
  int Brick::cmp(const Brick& b) const {
    if(isVertical != b.isVertical)
      return - isVertical + b.isVertical;
    if(x != b.x)
      return x - b.x;
    return y - b.y;
  }
  bool Brick::intersects(const Brick &b) const {
    if(isVertical != b.isVertical)
      return DIFFLT(b.x, x, 3) && DIFFLT(b.y, y, 3);
    if(isVertical)
      return DIFFLT(b.x, x, 2) && DIFFLT(b.y, y, 4);
    else
      return DIFFLT(b.x, x, 4) && DIFFLT(b.y, y, 2);
  }
  void Brick::mirror(Brick &b, const int16_t &cx, const int16_t &cy) const {
    b.isVertical = isVertical;
    b.x = cx - x; // cx/2 + (cx/2 - x) = cx - x
    b.y = cy - y;
  }
  bool Brick::mirrorEq(const Brick &b, const int16_t &cx, const int16_t &cy) const {
    if(b.isVertical != isVertical)
      return false;
    if(b.x != cx - x)
      return false;
    return b.y == cy - y;
  }

  BrickPicker::BrickPicker(const std::vector<LayerBrick> &v, int vIdx, const int numberOfBricksToPick, LayerBrick *bricks, const int bricksIdx) : v(v), vIdx(vIdx-1), numberOfBricksToPick(numberOfBricksToPick), bricksIdx(bricksIdx), bricks(bricks), inner(NULL) {
  }

  BrickPicker::~BrickPicker() {
    if(inner != NULL) {
      delete inner;
    }
  }

  bool BrickPicker::checkVIdx() const {
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
    do {
      vIdx++;
      if(vIdx >= (int)v.size())
	return; // Out of bounds!
      bricks[bricksIdx] = v[vIdx];
    }
    while(!checkVIdx());
  }

  bool BrickPicker::next() {
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
    for(uint16_t i = 0; i < 2; i++) {
      for(uint16_t j = 0; j < PLANE_WIDTH; j++) {
	for(uint16_t k = 0; k < PLANE_WIDTH; k++) {
	  bricks[i][j][k] = false;
	}
      }
    }
  }

  void BrickPlane::set(const Brick &b) {
    bricks[b.isVertical][b.x][b.y] = true;
  }

  void BrickPlane::unset(const Brick &b) {
    bricks[b.isVertical][b.x][b.y] = false;
  }

  bool BrickPlane::contains(const Brick &b) {
    return bricks[b.isVertical][b.x][b.y];
  }
  
  Combination::Combination() : height(1), size(1) {
    bricks[0][0] = FirstBrick;
    layerSizes[0] = 1;
    history[0] = BrickIdentifier(0,0);
    for(uint16_t i = 1; i < MAX_BRICKS; i++)
      layerSizes[i] = 0;
  }
  Combination::Combination(const Combination &b) {
    copy(b);
  }

  bool Combination::operator <(const Combination& b) const {
    assert(height == b.height);
    for(int i = 0; i < height; i++) {
      assert(layerSizes[i] == b.layerSizes[i]);
      int s = layerSizes[i];
      for(int j = 0; j < s; j++) {
	int res = bricks[i][j].cmp(b.bricks[i][j]);
	if(res != 0) {
	  return res < 0;
	}
      }
    }
    return false;
  }

  bool Combination::operator ==(const Combination& b) const {
    assert(height == b.height);
    for(uint16_t i = 0; i < height; i++) {
      assert(layerSizes[i] == b.layerSizes[i]);
      uint16_t s = layerSizes[i];
      for(uint16_t j = 0; j < s; j++) {
	if(bricks[i][j] != b.bricks[i][j]) {
	  return false;
	}
      }
    }
    return true;
  }

  std::ostream& operator << (std::ostream &os, const Combination &b) {
    os << "<";
    for(uint16_t i = 0; i < b.height; i++) {
      os << (int)b.layerSizes[i];
    }
    os << "> combination:";
    for(uint16_t i = 0; i < b.height; i++) {
      for(uint16_t j = 0; j < b.layerSizes[i]; j++) {
	os << " " << b.bricks[i][j] << " ";
      }
    }
    return os;
  }

  void Combination::copy(const Combination &b) {
    height = b.height;
    size = b.size;
    for(uint16_t i = 0; i < MAX_BRICKS; i++)
      layerSizes[i] = b.layerSizes[i];
    for(uint16_t i = 0; i < height; i++) {
      for(uint16_t j = 0; j < layerSizes[i]; j++) {
	bricks[i][j] = b.bricks[i][j];
      }
    }
    for(uint16_t i = 0; i < size; i++) {
      history[i] = b.history[i];
    }
  }

  void Combination::sortBricks() {
    for(uint16_t layer = 0; layer < height; layer++) {
      uint16_t layerSize = layerSizes[layer];
      if(layerSize > 1) {
	std::sort(bricks[layer], &bricks[layer][layerSize]);
      }
    }
  }

  void Combination::translateMinToOrigo() {
    int16_t minx = 127, miny = 127;

    for(uint16_t i = 0; i < layerSizes[0]; i++) {
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
    for(uint16_t i = 0; i < height; i++) {
      for(uint16_t j = 0; j < layerSizes[i]; j++) {
	bricks[i][j].x -= minx;
	bricks[i][j].y -= miny;
      }
    }
  }

  void Combination::rotate180() {
    // Perform rotation:
    for(int i = 0; i < height; i++) {
      for(int j = 0; j < layerSizes[i]; j++) {
	Brick &b = bricks[i][j];
	b.x = -b.x;
	b.y = -b.y;
      }
    }
    translateMinToOrigo();
    sortBricks(); // TODO: Is std::reverse fast enough?
  }

  void Combination::getLayerCenter(const uint16_t layer, int16_t &cx, int16_t &cy) const {
    cx = 0;
    cy = 0;
    for(uint16_t i = 0; i < layerSizes[layer]; i++) {
      cx += bricks[layer][i].x;
      cy += bricks[layer][i].y;
    }
    cx *= 2;
    cy *= 2;
    cx /= layerSizes[layer];
    cy /= layerSizes[layer];
  }

  bool Combination::isLayerSymmetric(const uint16_t layer, const int16_t &cx, const int16_t &cy) const {
    const uint16_t layerSize = layerSizes[layer];
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
    else {
      std::set<Brick> seen;
      Brick mirror;
      for(uint16_t i = 0; i < layerSize; i++) {
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

  bool Combination::is180Symmetric() const {
    int16_t cx0, cy0;
    getLayerCenter(0, cx0, cy0);

    if(!isLayerSymmetric(0, cx0, cy0)) {
      return false;
    }

    for(uint16_t i = 1; i < height; i++) {
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

  void Combination::addBrick(const Brick &b, const uint16_t layer) {
    const int16_t &layerSize = layerSizes[layer];
    history[size] = BrickIdentifier(layer, layerSize);
    bricks[layer][layerSize] = b;

    size++;
    if(layer == height)
      height++;
    layerSizes[layer]++;
  }

  void Combination::removeLastBrick() {
    size--;
    const uint16_t &layer = history[size].first;

    layerSizes[layer]--;
    if(layerSizes[layer] == 0)
      height--;
  }

  int Combination::countConnected(int layer, int idx) {
    connected[layer][idx] = true;
    const Brick &b = bricks[layer][idx];
    int ret = 1;
    // Add for layer below:
    if(layer > 0) {
      int s = layerSizes[layer-1];
      for(int i = 0; i < s; i++) {
	if(connected[layer-1][i]) {
	  continue;
	}
	const Brick &b2 = bricks[layer-1][i];
	if(b.intersects(b2)) {
	  ret += countConnected(layer-1, i);
	}
      }
    }
    // Add for layer above:
    if(layer < height-1) {
      int s = layerSizes[layer+1];
      for(int i = 0; i < s; i++) {
	if(connected[layer+1][i]) {
	  continue;
	}
	const Brick &b2 = bricks[layer+1][i];
	if(b.intersects(b2)) {
	  ret += countConnected(layer+1, i);
	}
      }
    }
    return ret;
  }

  bool Combination::isConnected() {
    int Z = 0;

    // Reset state:
    for(int i = 0; i < height; i++) {
      Z += layerSizes[i];
      int s = layerSizes[i];
      for(int j = 0; j < s; j++) {
	connected[i][j] = false;
      }
    }
    // Run DFS:
    int cnt = countConnected(0, 0);

    return cnt == Z;
  }

  int Combination::getTokenFromLayerSizes() const {
    int ret = 0;
    for(uint16_t i = 0; i < height; i++) {
      ret = (ret * 10) + layerSizes[i];
    }
    return ret;
  }

  int Combination::reverseToken(int token) {
    int ret = 0;
    while(token > 0) {
      ret = (ret * 10) + (token % 10);
      token /= 10;
    }
    return ret;
  }

  uint16_t Combination::heightOfToken(int token) {
    uint16_t ret = 0;
    while(token > 0) {
      ret++;
      token = token/10;
    }
    return ret;
  }

  uint16_t Combination::sizeOfToken(int token) {
    uint16_t ret = 0;
    while(token > 0) {
      ret += token % 10;
      token = token/10;
    }
    return ret;
  }

  void Combination::getLayerSizesFromToken(int token, uint16_t *layerSizes) {
    uint16_t layers = 0;
    while(token > 0) {
      int size_add = token % 10;
      layerSizes[layers++] = size_add;
      token /= 10;
    }
    // Flip the layer sizes:
    for(uint16_t i = 0; i < layers/2; i++) {
      std::swap(layerSizes[i], layerSizes[layers-i-1]);
    }
  }  

  CombinationBuilder::CombinationBuilder(Combination &c,
					 const uint16_t waveStart,
					 const uint16_t waveSize,
					 const uint16_t maxSize) :
    baseCombination(c), waveStart(waveStart), waveSize(waveSize), maxSize(maxSize), indent(0), done(false) {
    assert(c.size <= maxSize);
    neighbours = new BrickPlane[MAX_BRICKS];
    for(uint16_t i = 0; i < MAX_BRICKS; i++) {
      neighbours[i].unsetAll();
    }
  }

  CombinationBuilder::CombinationBuilder(Combination &c,
					 const uint16_t waveStart,
					 const uint16_t waveSize,
					 const uint16_t maxSize,
					 const uint16_t indent,
					 BrickPlane *neighbours) :
    baseCombination(c), waveStart(waveStart), waveSize(waveSize), maxSize(maxSize), indent(indent), neighbours(neighbours), done(false) {
    assert(c.size <= maxSize);
  }

  CombinationBuilder::CombinationBuilder(const CombinationBuilder& b) :
    baseCombination(b.baseCombination), waveStart(b.waveStart), waveSize(b.waveSize), maxSize(b.maxSize), indent(b.indent), neighbours(b.neighbours), done(b.done) {
  }

  CombinationBuilder::CombinationBuilder() : waveStart(0), waveSize(0), maxSize(0), indent(0), neighbours(NULL), done(false) {
  }

  void CombinationBuilder::findPotentialBricksForNextWave(std::vector<LayerBrick> &v) {
    // Find all potential neighbours above and below all in wave:
    for(uint16_t i = 0; i < waveSize; i++) {
      const BrickIdentifier &bi = baseCombination.history[waveStart+i];
      const int16_t waveBrickLayer = bi.first; // Convert to signed
      const Brick &brick = baseCombination.bricks[waveBrickLayer][bi.second];

      for(int16_t layer2 = waveBrickLayer-1; layer2 <= waveBrickLayer+1; layer2+=2) {
	if(layer2 <= 0)
	  continue; // Do not allow building on or below base layer

	// Add crossing bricks (one vertical, one horizontal):
	for(int x = -2; x < 3; x++) {
	  for(int y = -2; y < 3; y++) {
	    const Brick b(!brick.isVertical, brick.x+x, brick.y+y);
	    bool ok = true;
	    // If b connects to or overlaps a brick before the wave, then disregard:
	    for(uint16_t j = 0; j < waveStart; j++) {
	      const BrickIdentifier &bi3 = baseCombination.history[j];
	      const uint16_t layer3 = bi3.first;
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
	    for(uint16_t j = 0; j < waveStart; j++) {
	      const BrickIdentifier &bi3 = baseCombination.history[j];
	      const uint16_t layer3 = bi3.first;
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
    bool canBeSymmetric180 = true;

    uint16_t fullLayers = 0; // Layers where all bricks are already placed: If they are non-symmetric or have misalignment of centers, then the resulting models cannot be symmetric
    int16_t cx0, cy0, cx1, cy1;
    bool solidLayers[MAX_BRICKS];
    for(uint16_t i = 0; i < baseCombination.height; i++) {
      solidLayers[i] = true;
    }
    for(uint16_t i = 0; i < waveSize; i++) {
      const BrickIdentifier &bi = baseCombination.history[waveStart+i];
      const uint16_t &layer = bi.first;
      if(layer > 0)
	solidLayers[layer-1] = false;
      solidLayers[layer+1] = false;
    }
    for(uint16_t i = 0; i < baseCombination.height; i++) {
      if(!solidLayers[i])
	continue; // Only check layers that cannot be reached by last wave
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

    return canBeSymmetric180;
  }

  void CombinationBuilder::placeAllLeftToPlace(const uint16_t &leftToPlace, const bool &canBeSymmetric180, const std::vector<LayerBrick> &v) {
    LayerBrick bricks[MAX_BRICKS];
    BrickPicker picker(v, 0, leftToPlace, bricks, 0);
    while(picker.next()) {
      // toPick bricks ready in bricks: Use as next wave!
      for(uint16_t i = 0; i < leftToPlace; i++) {
	baseCombination.addBrick(bricks[i].BRICK, bricks[i].LAYER);
      }

      bool hasLayerWithOneBrick = false;
      for(uint16_t i = 1; i < baseCombination.height-1; i++) {
	if(baseCombination.layerSizes[i] == 1) {
	  hasLayerWithOneBrick = true;
	  break;
	}
      }

      if(!hasLayerWithOneBrick) {
	int token = baseCombination.getTokenFromLayerSizes();
	if(!baseCombination.isConnected())
	  token = -token;
	Counts cx;
	cx.all++;
	if(canBeSymmetric180 && baseCombination.is180Symmetric()) {
	  cx.symmetric180++;
	}
	if(counts.find(token) == counts.end())
	  counts[token] = cx;
	else
	  counts[token] += cx;
      }

      for(uint16_t i = 0; i < leftToPlace; i++) {
	baseCombination.removeLastBrick();
      }
    }
  }

  void CombinationBuilder::joinOne(CombinationBuilder *builders, std::thread **threads, int n) {
    long waitedMs = 0;
    int waitedTickMs;
    int waitTimeMs = 50;
    const int WAIT_REPORT_TICKS = 2 * 60 * 1000, MAX_CHECK_WAIT_TIME_MS = 2000;
    while(true) {
      for(int i = 0; i < n; i++) {
	if(threads[i] == NULL)
	  continue;
	if(builders[i].done) {
	  threads[i]->join();
	  addCountsFrom(builders[i]);

	  delete threads[i];
	  threads[i] = NULL;
	  std::cout << "Thread " << i << " done" << std::endl;
	  return;
	}
      }
      if(waitedTickMs > WAIT_REPORT_TICKS) {
	std::cout << "Waited " << (waitedMs/1000) << " seconds" << std::endl;
	waitedTickMs = 0;
      }

      std::this_thread::sleep_for(std::chrono::milliseconds(waitTimeMs));
      waitedMs += waitTimeMs;
      waitedTickMs += waitTimeMs;

      if(waitTimeMs < MAX_CHECK_WAIT_TIME_MS)
	waitTimeMs *= 2;
    }
  }

  void CombinationBuilder::addCountsFrom(const CombinationBuilder &b) {
    for(CountsMap::const_iterator it = b.counts.begin(); it != b.counts.end(); it++) {
      int token = it->first;
      Counts toAdd = it->second;

      if(counts.find(token) == counts.end())
	counts[token] = toAdd;
      else
	counts[token] += toAdd;
    }
  }

  /*
    BFS construction of models:
    Assume a non-empty wave:
     Pick 1..|wave| bricks from wave:
      Find next wave and recurse until model contains n bricks.
   */
  void CombinationBuilder::build(bool hasSplitIntoThreads) {
    std::vector<LayerBrick> v;
    findPotentialBricksForNextWave(v);

    const uint16_t leftToPlace = maxSize - baseCombination.size;
#ifdef DEBUG
    std::cout << "Building " << (int)leftToPlace << " on " << baseCombination << " of size " << (int)baseCombination.size << " up to size " << (int)maxSize << std::endl;
#endif
    assert(leftToPlace < MAX_BRICKS);
    const bool canBeSymmetric180 = nextCombinationCanBeSymmetric180();

    placeAllLeftToPlace(leftToPlace, canBeSymmetric180, v);

    LayerBrick bricks[MAX_BRICKS];

    int processorCount = hasSplitIntoThreads ? -1 : std::thread::hardware_concurrency() - 2; // allow 2 processors for OS and other...

    for(uint16_t toPick = 1; toPick < leftToPlace; toPick++) {
      if(!hasSplitIntoThreads && leftToPlace > 4)
	std::cout << "Picking " << (int)toPick << "/" << (int)leftToPlace << " from " << v.size() << std::endl;
      bool split = false;
      int activeThreads = 0;
      CombinationBuilder *combinationBuilders;
      std::thread **threads;
      if(processorCount > 1 && v.size() > processorCount && leftToPlace-toPick > 3) {
	split = true;
	threads = new std::thread*[processorCount];
	combinationBuilders = new CombinationBuilder[processorCount];
	for(int i = 0; i < processorCount; i++)
	  threads[i] = NULL;
      }

      // Pick toPick from neighbours:
      BrickPicker picker(v, 0, toPick, bricks, 0);
      
      while(picker.next()) {
	for(uint16_t i = 0; i < toPick; i++)
	  baseCombination.addBrick(bricks[i].BRICK, bricks[i].LAYER);

	if(split) {
	  for(int i = 0; i < processorCount; i++) {
	    if(threads[i] != NULL)
	      continue;
	    for(int j = 0; j < indent; j++)
	      std::cout << " ";
	    std::cout << "Starting thread " << i << " for " << baseCombination << " after picking " << (int)toPick << std::endl;
	    combinationBuilders[i] = CombinationBuilder(baseCombination, waveStart+waveSize, toPick, maxSize);
	    threads[i] = new std::thread(&CombinationBuilder::build, std::ref(combinationBuilders[i]), true);
	    activeThreads++;
	    break;
	  }
	  if(activeThreads == processorCount) {
	    joinOne(combinationBuilders, threads, processorCount);
	    activeThreads--;
	  }
	}
	else {
	  CombinationBuilder builder(baseCombination, waveStart+waveSize, toPick, maxSize, indent+1, neighbours);
	  builder.build(hasSplitIntoThreads);
	  addCountsFrom(builder);
	}

	for(uint16_t i = 0; i < toPick; i++)
	  baseCombination.removeLastBrick();
	// Clean up baseCombination	
      } // while(picker.next())

      // Spool down remaining threads:
      if(split) {
	for(int j = 0; j < indent; j++)
	  std::cout << " ";
	std::cout << "Closing all remaining threads for picking " << (int)toPick << std::endl;
	while(activeThreads > 0) {
	  std::cout << activeThreads << " remaining" << std::endl;
	  joinOne(combinationBuilders, threads, processorCount);
	  activeThreads--;
	}
	delete[] combinationBuilders;
	delete[] threads;
      }
    } // for(uint16_t toPick = 1; toPick < leftToPlace; toPick++)

    done = true;
  }

  Lemma2::Lemma2(int n): n(n) {
  }

  bool Lemma2::computeOnBase2(bool vertical, int16_t dx, int16_t dy, Counts &c, Counts &d) {
    uint16_t layerSizes[MAX_BRICKS];

    Brick b2(vertical, FirstBrick.x + dx, FirstBrick.y + dy);
    if(FirstBrick.intersects(b2))
      return true;

    std::cout << " Handling " << b2 << std::endl;
    Combination baseCombination; // Includes first brick
    baseCombination.addBrick(b2, 0);

    CombinationBuilder builder(baseCombination, 0, 2, n);
    builder.build(false);

    std::stringstream ss; ss << "base_2_size_" << n << "/";
    if(b2.isVertical)
      ss << "parallel_dx_";
    else
      ss << "crossing_dx_";
    ss << (int)dx << "_dy_" << (int)dy << ".txt";
    std::ofstream ostream(ss.str().c_str());

    for(CountsMap::const_iterator it = builder.counts.begin(); it != builder.counts.end(); it++) {
      int token = it->first;
#ifdef TRACE
      std::cout << "Handling tokens " << token << " on " << baseCombination << std::endl;
      std::cout << " " << it->second << std::endl;
#endif
      if(token < 0) {
	ostream << "DISCONNECTED ";
	token = -token;
	d += it->second;
      }
      else {
	ostream << "CONNECTED ";
	c += it->second;
      }
      ostream << token << " TOTAL " << it->second.all << " SYMMETRIC " << it->second.symmetric180 << std::endl;
    }

    ostream.flush();
    ostream.close();
    return false;
  }

  void Lemma2::computeOnBase2() {
    for(int rotation = 0; rotation <= 1; rotation++) {
      Counts prevDisconnectedX;
      for(int16_t dx = 0; true; dx++) {
	Counts connectedX, disconnectedX;

	Counts prevDisconnectedY;
	for(int16_t dy = 0; true; dy++) {
	  Counts connectedY, disconnectedY;

	  bool skipped = computeOnBase2((bool)rotation, dx, dy, connectedY, disconnectedY);
	  if(skipped)
	    continue;
	  connectedX += connectedY;
	  disconnectedX += disconnectedY;

	  if(connectedY.empty() && prevDisconnectedY == disconnectedY) {
	    break; // Nothing connects, and disconnects do not change
	  }
	  prevDisconnectedY = disconnectedY;
	} // for dy

	if(connectedX.empty() && prevDisconnectedX == disconnectedX) {
	  break; // Nothing connects, and disconnects do not change
	}
	prevDisconnectedX = disconnectedX;

      } // for dx
    } // for rotation
  }

} // namespace rectilinear
