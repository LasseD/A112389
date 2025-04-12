#include <set>
#include <vector>
#include <algorithm>
#include <assert.h>
#include <chrono>
#include <thread>
#include <sstream>
#include <iostream>

#include "bfs.h"

#ifdef PROFILING
void Profiler::countInvocation(const std::string &s) {
#ifdef TRACE
  std::cout << "Calling " << s << std::endl;
#endif
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
  Counts Counts::operator -(const Counts& c) const {
#ifdef PROFILING
    Profiler::countInvocation("Counts::operator -");
#endif
    return Counts(all-c.all, symmetric180-c.symmetric180, symmetric90-c.symmetric90);
  }
  Counts Counts::operator /(const int& v) const {
    assert(all % v == 0);
    assert(symmetric180 % v == 0);
    assert(symmetric90 % v == 0);
    return Counts(all/v, symmetric180/v, symmetric90/v);
  }
  bool Counts::operator ==(const Counts& c) const {
    return all == c.all && symmetric180 == c.symmetric180 && symmetric90 == c.symmetric90;
  }
  bool Counts::operator !=(const Counts& c) const {
    return all != c.all || symmetric180 != c.symmetric180 || symmetric90 != c.symmetric90;
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
  bool Counts::empty() {
#ifdef PROFILING
    Profiler::countInvocation("Counts::empty()");
#endif
    return all == 0 && symmetric180 == 0 && symmetric90 == 0;
  }

  Brick::Brick() : isVertical(true), x(PLANE_MID), y(PLANE_MID) {
#ifdef PROFILING
    Profiler::countInvocation("Brick::Brick()");
#endif
  }
  Brick::Brick(bool iv, int16_t x, int16_t y) : isVertical(iv), x(x), y(y) {	
#ifdef PROFILING
    Profiler::countInvocation("Brick::Brick(bool,...)");
#endif
    assert(x >= 0);
    assert(y >= 0);
  }
  Brick::Brick(const Brick &b) : isVertical(b.isVertical), x(b.x), y(b.y) {
#ifdef PROFILING
    Profiler::countInvocation("Brick::Brick(Brick&)");
#endif
    assert(x >= 0);
    assert(y >= 0);
  }

  bool Brick::operator <(const Brick& b) const {
#ifdef PROFILING
    Profiler::countInvocation("Brick::operator <");
#endif
    int d1 = FirstBrick.dist(*this);
    int d2 = FirstBrick.dist(b);
    if(d1 != d2)
      return d1 < d2;
    if(isVertical != b.isVertical)
      return isVertical > b.isVertical;
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
    return x != b.x || y != b.y || isVertical != b.isVertical;
  }
  std::ostream& operator << (std::ostream &os,const Brick &b) {
    os << (b.isVertical?"|":"=") << (int)(b.x-PLANE_MID) << "," << (int)(b.y-PLANE_MID) << (b.isVertical?"|":"=");
    return os;
  }
  int Brick::cmp(const Brick& b) const {
#ifdef PROFILING
    Profiler::countInvocation("Brick::cmp()");
#endif
    if(isVertical != b.isVertical)
      return - isVertical + b.isVertical;
    if(x != b.x)
      return x - b.x;
    return y - b.y;
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
  void Brick::mirror(Brick &b, const int16_t &cx, const int16_t &cy) const {
#ifdef PROFILING
    Profiler::countInvocation("Brick::mirror()");
#endif
    b.isVertical = isVertical;
    b.x = cx - x; // cx/2 + (cx/2 - x) = cx - x
    b.y = cy - y;
  }
  bool Brick::mirrorEq(const Brick &b, const int16_t &cx, const int16_t &cy) const {
#ifdef PROFILING
    Profiler::countInvocation("Brick::mirrorEq()");
#endif
    if(b.isVertical != isVertical)
      return false;
    if(b.x != cx - x)
      return false;
    return b.y == cy - y;
  }
  int Brick::dist(const Brick &b) const {
    return ABS(x-b.x) + ABS(y-b.y);
  }
  bool Brick::canIntercept(const Brick &a, const Brick &b, uint8_t toAdd) {
    if(a.intersects(b))
      return true;
    if(toAdd == 0)
      return false;

    // Ensure a.isVertical:
    if(!a.isVertical) {
      if(!b.isVertical)
	return canIntercept(Brick(true, a.y, a.x), Brick(true, b.y, b.x), toAdd);
      return canIntercept(b, a, toAdd);
    }

    int16_t dx = ABS(a.x-b.x);
    int16_t dy = ABS(a.y-b.y);
    if(toAdd == 1) {
      if(!b.isVertical)
	return (dx < 6 && dy < 4) || (dx < 4 && dy < 6);
      // Both vertical:
      return (dx <= 2 && dy <= 6) || (dx <= 4 && dy <= 4);
    }

    // Quick range checks:
    if(dx+dy <= 3*(toAdd+1) - 2)
      return true; // Can always move 3 if both aligned toward each other. "- 2" to emulate alignment
    if(dx+dy > 4*(toAdd+1))
      return false; // Can not move more than 4

    // toAdd >= 2:
    int16_t signX = a.x < b.x ? 1 : -1;
    int16_t signY = a.y < b.y ? 1 : -1;
    return canIntercept(Brick(true, a.x+MIN(1, dx)*signX, a.y+MIN(3,dy)*signY), b, toAdd-1) ||
      canIntercept(Brick(false, a.x+MIN(2,dx)*signX, a.y+MIN(2,dy)*signY), b, toAdd-1);
  }

  BrickPicker::BrickPicker(const std::vector<LayerBrick> &v, int vIdx, const int numberOfBricksToPick, LayerBrick *bricks, const int bricksIdx) : v(v), vIdx(vIdx-1), numberOfBricksToPick(numberOfBricksToPick), bricksIdx(bricksIdx), bricks(bricks), inner(NULL) {
#ifdef PROFILING
    Profiler::countInvocation("BrickPicker::BrickPicker()");
#endif
#ifdef TRACE
    std::cout << "   BrickPicker for " << numberOfBricksToPick << " bricks, starting at " << vIdx << " of " << v.size() << " at layer created with " << bricksIdx << " previous bricks added " <<std::endl;
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

  MultiLayerBrickPicker::MultiLayerBrickPicker(const std::vector<LayerBrick> &v, const int maxPick) : v(v), maxPick(maxPick), toPick(1) {
#ifdef PROFILING
    Profiler::countInvocation("MultiLayerBrickPicker::MultiLayerBrickPicker()");
#endif
    assert(maxPick >= 1);
    inner = new BrickPicker(v, 0, 1, bricks, 0);
  }

  bool MultiLayerBrickPicker::next(Combination &c, int &picked) {
    std::lock_guard<std::mutex> guard(next_mutex);
#ifdef PROFILING
    Profiler::countInvocation("MultiLayerBrickPicker::next()");
#endif
    if(inner == NULL) {
      return false;
    }
    bool ret;
    while(!(ret = inner->next())) {
      delete inner;
      if(toPick == maxPick) {
	inner = NULL;
	return false;
      }
      inner = new BrickPicker(v, 0, ++toPick, bricks, 0);
    }

    for(int i = 0; i < toPick; i++) {
      c.addBrick(bricks[i].BRICK, bricks[i].LAYER);
    }
    picked = toPick;

    return true;
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

  bool Combination::operator <(const Combination& b) const {
#ifdef PROFILING
    Profiler::countInvocation("Combination::operator <");
#endif
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
      for(uint8_t j = 0; j < layerSizes[i]; j++)
	bricks[i][j] = b.bricks[i][j];
    }
    for(uint8_t i = 0; i < size; i++)
      history[i] = b.history[i];
  }

  void Combination::sortBricks() {
#ifdef PROFILING
    Profiler::countInvocation("Combination::sortBricks()");
#endif
    for(uint8_t layer = 0; layer < height; layer++) {
      const uint8_t &layerSize = layerSizes[layer];
      if(layerSize > 1)
	std::sort(bricks[layer], &bricks[layer][layerSize]);
    }
  }

  void Combination::translateMinToOrigo() {
#ifdef PROFILING
    Profiler::countInvocation("Combination::translateMinToOrigo()");
#endif
    int16_t minx = 10000, miny = 10000;

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
	bricks[i][j].x += PLANE_MID - minx;
	bricks[i][j].y += PLANE_MID - miny;
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
	bricks[i][j] = Brick(!b.isVertical, b.y, PLANE_MID - (b.x - PLANE_MID));
      }
    }
    translateMinToOrigo();
    sortBricks();
  }

  void Combination::rotate180() {
#ifdef PROFILING
    Profiler::countInvocation("Combination::rotate180()");
#endif
    // Perform rotation:
    for(uint8_t i = 0; i < height; i++) {
      for(uint8_t j = 0; j < layerSizes[i]; j++) {
	Brick &b = bricks[i][j];
	b.x = PLANE_MID - (b.x - PLANE_MID);
	b.y = PLANE_MID - (b.y - PLANE_MID);
      }
    }
    translateMinToOrigo();
    sortBricks(); // TODO: Is std::reverse fast enough?
  }

  void Combination::mirrorX() {
    for(uint8_t i = 0; i < height; i++) {
      for(uint8_t j = 0; j < layerSizes[i]; j++) {
	Brick &b = bricks[i][j];
	b.x = PLANE_MID - (b.x - PLANE_MID);
      }
    }
    translateMinToOrigo();
    sortBricks(); // TODO: Is std::reverse fast enough?
  }

  void Combination::mirrorY() {
    for(uint8_t i = 0; i < height; i++) {
      for(uint8_t j = 0; j < layerSizes[i]; j++) {
	Brick &b = bricks[i][j];
	b.y = PLANE_MID - (b.y - PLANE_MID);
      }
    }
    translateMinToOrigo();
    sortBricks(); // TODO: Is std::reverse fast enough?
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

  bool Combination::createMaxCombination(int n, char *argv, Combination &maxCombination) {
    char c;
    maxCombination.size = 0;
    maxCombination.height = 0;
    for(int i = 0; (c = argv[i]); i++) {
      maxCombination.layerSizes[i] = c-'0';
      maxCombination.height++;
      maxCombination.size += maxCombination.layerSizes[i];
    }

    return maxCombination.size == n; // Mismatch bewteen n and size!
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
#ifdef PROFILING
    Profiler::countInvocation("Combination::is90Symmetric()");
#endif
    if(!canRotate90())
      return false;
    for(uint8_t i = 0; i < height; i++) {
      if(layerSizes[i] % 4 != 0)
	return false;
    }
    Combination rotated(*this);
    rotated.rotate90();
    Combination dis(*this);
    dis.translateMinToOrigo();
    dis.sortBricks();      
    return rotated == dis;
  }

  bool Combination::canRotate90() const {
#ifdef PROFILING
    Profiler::countInvocation("Combination::canRotate90()");
#endif
    for(int i = 0; i < layerSizes[0]; i++) {
      if(!bricks[0][i].isVertical) {
	return true;
      }
    }
    return false;
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

  void Combination::colorConnected(uint8_t layer, uint8_t idx, uint8_t color) {
#ifdef PROFILING
    Profiler::countInvocation("Combination::colorConnected()");
#endif
    if(colors[layer][idx] != 0)
      return; // Already colored
    colors[layer][idx] = color;
    const Brick &b = bricks[layer][idx];
    // Add for layer below:
    for(int8_t layer2 = -1+(int8_t)layer; layer2 <= layer+1; layer2 += 2) {
      if(layer2 < 0 || layer2 >= height)
	continue;

      uint8_t s = layerSizes[layer2];
      for(uint8_t i = 0; i < s; i++) {
	if(colors[layer2][i] != 0)
	  continue; // Already colored
	const Brick &b2 = bricks[layer2][i];
	if(b.intersects(b2))
	  colorConnected(layer2, i, color);
      }
    }
  }

  int64_t Combination::encodeConnectivity(int64_t token) {
#ifdef PROFILING
    Profiler::countInvocation("Combination::encodeConnectivity()");
#endif
    assert(height > 1);
    assert(layerSizes[0] >= 2);

    // Reset state:
    for(uint8_t i = 0; i < height; i++) {
      uint8_t s = layerSizes[i];
      for(uint8_t j = 0; j < s; j++)
	colors[i][j] = 0;
    }
    // Run DFS:
    for(uint8_t i = 0; i < layerSizes[0]; i++) {
      colorConnected(0, i, i+1);
    }
    // Encode:
    for(uint8_t i = 0; i < layerSizes[0]; i++) {
      token = 10 * token + colors[0][i];
    }
    return token;
  }
  
  int64_t Combination::getTokenFromLayerSizes() const {
#ifdef PROFILING
    Profiler::countInvocation("Combination::getTokenFromLayerSizes()");
#endif
    int64_t ret = 0;
    for(uint8_t i = 0; i < height; i++) {
      ret = (ret * 10) + layerSizes[i];
    }
    return ret;
  }

  void Combination::normalize(int &rotated) {
#ifdef PROFILING
    Profiler::countInvocation("Combination::normalize(...)");
#endif
    // Ensure FirstBrick is first and all is sorted:
    bool hasVerticalLayer0Brick = false;
    for(int i = 0; i < layerSizes[0]; i++) {
      Brick &b = bricks[0][i];
      if(b.isVertical) {
	hasVerticalLayer0Brick = true;
	break;
      }
    }

    // Ensure first brick is horizontal at 0,0:
    if(hasVerticalLayer0Brick) {
      translateMinToOrigo();
      sortBricks();
    }
    else {
      rotate90();
      rotated = 90;
    }

    Combination c(*this);
    if(canRotate90()) {
      for(int i = 0; i < 3; i++) {
	c.rotate90();
	if(c < *this) {
	  copy(c);
	  rotated = 90*(i+1);
	}
      }
    }
    else {
      c.rotate180();
      if(c < *this) {
	copy(c);
	rotated = 180;
      }
    }
  }
  void Combination::normalize() {
#ifdef PROFILING
    Profiler::countInvocation("Combination::normalize()");
#endif
    int ignore;
    normalize(ignore);
  }

  int Combination::reverseToken(int64_t token) {
#ifdef PROFILING
    Profiler::countInvocation("Combination::reverseToken()");
#endif
    int64_t ret = 0;
    while(token > 0) {
      ret = (ret * 10) + (token % 10);
      token /= 10;
    }
    return ret;
  }

  uint8_t Combination::heightOfToken(int64_t token) {
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

  uint8_t Combination::sizeOfToken(int64_t token) {
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

  void Combination::getLayerSizesFromToken(int64_t token, uint8_t *layerSizes) {
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

  bool Combination::anyInterceptions(int toAdd, uint8_t from) const {
    const uint8_t &S = layerSizes[0];
    if(from == S-1)
      return false;
    for(uint8_t i = from+1; i < S; i++) {
      if(Brick::canIntercept(bricks[0][i], bricks[0][from], toAdd))
	return true;
    }
    return anyInterceptions(toAdd, from+1);
  }

  bool Combination::anyInterceptions(int toAdd) const {
    assert(height == 1);
    return anyInterceptions(toAdd, 0);
  }

  CombinationBuilder::CombinationBuilder(const Combination &c,
					 const uint8_t waveStart,
					 const uint8_t waveSize,
					 const uint8_t maxSize,
					 BrickPlane *neighbours,
					 Combination const * maxCombination,
					 bool isFirstBuilder) :
    baseCombination(c), waveStart(waveStart), waveSize(waveSize), maxSize(maxSize), neighbours(neighbours), maxCombination(maxCombination), isFirstBuilder(isFirstBuilder) {
#ifdef PROFILING
    Profiler::countInvocation("CombinationBuilder::CombinationBuilder()::FAST");
#endif
  }

  CombinationBuilder::CombinationBuilder(const CombinationBuilder& b) :
    baseCombination(b.baseCombination), waveStart(b.waveStart), waveSize(b.waveSize), maxSize(b.maxSize), neighbours(b.neighbours), maxCombination(b.maxCombination), isFirstBuilder(b.isFirstBuilder) {
#ifdef PROFILING
    Profiler::countInvocation("CombinationBuilder::CombinationBuilder(CombinationBuilder&)");
#endif
  }

  CombinationBuilder::CombinationBuilder() : waveStart(0), waveSize(0), maxSize(0), neighbours(NULL), maxCombination(NULL), isFirstBuilder(false) {
#ifdef PROFILING
    Profiler::countInvocation("CombinationBuilder::CombinationBuilder()");
#endif
  }

  void CombinationBuilder::findPotentialBricksForNextWave(std::vector<LayerBrick> &v) {
#ifdef PROFILING
    Profiler::countInvocation("CombinationBuilder::findPotentialBricksForNextWave()");
#endif
    // Find all potential neighbours above and below all in wave:
    for(uint8_t i = 0; i < waveSize; i++) {
      const BrickIdentifier &bi = baseCombination.history[waveStart+i];
      const int8_t waveBrickLayer = bi.first; // Convert to signed
      const Brick &brick = baseCombination.bricks[waveBrickLayer][bi.second];

      for(int8_t layer2 = waveBrickLayer-1; layer2 <= waveBrickLayer+1; layer2+=2) {
	if(layer2 < 0)
	  continue; // Do not allow building below base layer
#ifdef REFINEMENT
	if(layer2 >= maxCombination->height || baseCombination.layerSizes[layer2] == maxCombination->layerSizes[layer2])
	  continue; // Already at maximum allowed for layer
#endif
#ifdef LEMMAS
	if(layer2 == 0)
	  continue; // Do not build on base layer for lemmas
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
    for(std::vector<LayerBrick>::const_iterator it = v.begin(); it != v.end(); it++)
      neighbours[it->LAYER].unset(it->BRICK);
  }

  bool CombinationBuilder::nextCombinationCanBeSymmetric180() {
#ifdef PROFILING
    Profiler::countInvocation("CombinationBuilder::nextCombinationCanBeSymmetric180()");
#endif
    bool canBeSymmetric180 = true;

    uint8_t fullLayers = 0; // Layers where all bricks are already placed: If they are non-symmetric or have misalignment of centers, then the resulting models cannot be symmetric
    int16_t cx0, cy0, cx1, cy1;
#ifdef REFINEMENT
    /*
      Speed up if model cannot be made symmetric
      when placing all leftToPlace bricks.
    */
    for(uint8_t i = 0; maxCombination->layerSizes[i] > 0; i++) {
      uint8_t diff = maxCombination->layerSizes[i] - baseCombination.layerSizes[i];
      if(diff == 0) {
	// Full layer: Check if can be symmetric:
	if(fullLayers == 0) {
	  baseCombination.getLayerCenter(i, cx0, cy0);
	  if(!baseCombination.isLayerSymmetric(i, cx0, cy0)) {
	    canBeSymmetric180 = false;
#ifdef TRACE
	    std::cout << "   Base combination " << baseCombination << " can not be symmetric due to non-symmetric full layer " << (int)i << std::endl;
#endif
	    break;
	  }
	}
	else {
	  baseCombination.getLayerCenter(i, cx1, cy1);
	  if(cx0 != cx1 || cy0 != cy1) {
	    canBeSymmetric180 = false;
#ifdef TRACE
	    std::cout << "   Base combination " << baseCombination << " can not be symmetric due offset center in full layer " << (int)i << std::endl;
#endif
	    break;
	  }
	  if(!baseCombination.isLayerSymmetric(i, cx0, cy0)) {
#ifdef TRACE
	    std::cout << "   Base combination " << baseCombination << " can not be symmetric due to non-symmetric full layer " << (int)i << std::endl;
#endif
	    canBeSymmetric180 = false;
	    break;
	  }
	}
	fullLayers++; // Because diff is 0
      }
    }
#else
    bool solidLayers[MAX_BRICKS];
    for(uint8_t i = 0; i < baseCombination.height; i++) {
      solidLayers[i] = true;
    }
    for(uint8_t i = 0; i < waveSize; i++) {
      const BrickIdentifier &bi = baseCombination.history[waveStart+i];
      const uint8_t &layer = bi.first;
      if(layer > 0)
	solidLayers[layer-1] = false;
      solidLayers[layer+1] = false;
    }
    for(uint8_t i = 0; i < baseCombination.height; i++) {
      if(!solidLayers[i])
	continue; // Only check layers that cannot be reached by last wave
      if(fullLayers == 0) {
	baseCombination.getLayerCenter(i, cx0, cy0);
	if(!baseCombination.isLayerSymmetric(i, cx0, cy0)) {
	  canBeSymmetric180 = false;
#ifdef TRACE
	  std::cout << "   Base combination " << baseCombination << " can not be symmetric due to non-symmetric full layer " << (int)i << std::endl;
#endif
	  break;
	}
      }
      else {
	baseCombination.getLayerCenter(i, cx1, cy1);
	if(cx0 != cx1 || cy0 != cy1) {
	  canBeSymmetric180 = false;
#ifdef TRACE
	  std::cout << "   Base combination " << baseCombination << " can not be symmetric due to non-symmetric full layer " << (int)i << std::endl;
#endif
	  break;
	}
	if(!baseCombination.isLayerSymmetric(i, cx0, cy0)) {
	  canBeSymmetric180 = false;
#ifdef TRACE
	  std::cout << "   Base combination " << baseCombination << " can not be symmetric due to full layer " << (int)i << std::endl;
#endif
	  break;
	}
      }
      fullLayers++;
    }
#endif

#ifdef TRACE
    std::cout << "   Base combination " << baseCombination << " can be symmetric?: " << canBeSymmetric180 << std::endl;
#endif
    return canBeSymmetric180;
  }

  void CombinationBuilder::placeAllLeftToPlace(const uint8_t &leftToPlace, const bool &canBeSymmetric180, const std::vector<LayerBrick> &v) {
#ifdef PROFILING
    Profiler::countInvocation("CombinationBuilder::placeAllLeftToPlace()");
#endif
#ifdef TRACE
    std::cout << "    Placing " << (int)leftToPlace << " bricks onto " << baseCombination << std::endl;
#endif
#ifdef REFINEMENT
    int cntNonFullLayers = 0;
    for(int i = 0; i < MAX_BRICKS; i++) {
      if(maxCombination->layerSizes[i] > baseCombination.layerSizes[i])
	cntNonFullLayers++;
    }
    bool checkedLayers[MAX_BRICKS];
    for(int i = 0; i < MAX_BRICKS; i++)
      checkedLayers[i] = false;
    for(std::vector<LayerBrick>::const_iterator it = v.begin(); it != v.end(); it++) {
      const LayerBrick &lb = *it;
      uint8_t layer = lb.LAYER;
      if(checkedLayers[layer])
	continue;
      if(maxCombination->layerSizes[layer] > baseCombination.layerSizes[layer]) {
	checkedLayers[layer] = true;
	cntNonFullLayers--;
      }
    }
    if(cntNonFullLayers > 0) {
#ifdef TRACE
      std::cout << "  Early exit as layers cannot be filled: " << cntNonFullLayers << std::endl;
#endif
      return; // Can't possibly fill!
    }
#endif
    LayerBrick bricks[MAX_BRICKS];
    BrickPicker picker(v, 0, leftToPlace, bricks, 0);
    while(picker.next()) {
#ifdef REFINEMENT
      bool ok = true;
#endif
      for(uint8_t i = 0; i < leftToPlace; i++) {
#ifdef REFINEMENT
	// Check if layer is already full:
	if(baseCombination.layerSizes[bricks[i].LAYER] == maxCombination->layerSizes[bricks[i].LAYER]) {
	  ok = false;
	  for(uint8_t j = 0; j < i; j++)
	    baseCombination.removeLastBrick();
	  break;
	}
#endif
	baseCombination.addBrick(bricks[i].BRICK, bricks[i].LAYER);
      }
#ifdef REFINEMENT
      if(!ok)
	continue;
#endif

#ifdef MAXHEIGHT
      if(baseCombination.height == 2 && baseCombination.layerSizes[0] > baseCombination.size/2) {
	for(uint8_t i = 0; i < leftToPlace; i++)
	  baseCombination.removeLastBrick();
	continue;
      }
#endif

#ifndef REFINEMENT
      // Do not count refinements where one non-end layer is single:
      bool hasLayerWithOneBrick = false;
      for(uint16_t i = 1; i < baseCombination.height-1; i++) {
	if(baseCombination.layerSizes[i] == 1) {
	  hasLayerWithOneBrick = true;
	  break;
	}
      }
      if(hasLayerWithOneBrick) {
	for(uint8_t i = 0; i < leftToPlace; i++)
	  baseCombination.removeLastBrick();
	continue;
      }
#endif

      int64_t token = baseCombination.getTokenFromLayerSizes();
#ifdef LEMMAS
      token = baseCombination.encodeConnectivity(token);
#endif

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

      for(uint8_t i = 0; i < leftToPlace; i++)
	baseCombination.removeLastBrick();
    }
  }

  void CombinationBuilder::addCountsFrom(const CombinationBuilder &b, bool doubleCount) {
#ifdef PROFILING
    Profiler::countInvocation("CombinationBuilder::addCountsFrom()");
#endif
    for(CountsMap::const_iterator it = b.counts.begin(); it != b.counts.end(); it++) {
      int64_t token = it->first;
      Counts toAdd = it->second;

      if(doubleCount) {
	toAdd += toAdd; // Since we skipped "the other time" (optimization 1)
      }

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
  void CombinationBuilder::build() {
#ifdef PROFILING
    Profiler::countInvocation("CombinationBuilder::build()");
#endif
#ifdef TRACE
    std::cout << "  Building on " << baseCombination << std::endl;
#endif
#ifdef REFINEMENT
    for(uint8_t i = 0; i < baseCombination.height; i++) {
      if(maxCombination->layerSizes[i] < baseCombination.layerSizes[i]) {
#ifdef TRACE
	std::cout << "Initial picker has picked too much!: " << (int)maxCombination->layerSizes[i] << " > " << (int)baseCombination.layerSizes[i] << " on layer " << (int)i << std::endl;
#endif
	return; // In case initial picker picks too much
      }
    }
#endif
    std::vector<LayerBrick> v;
    findPotentialBricksForNextWave(v);

    const uint8_t leftToPlace = maxSize - baseCombination.size;
    assert(leftToPlace < MAX_BRICKS);
    const bool canBeSymmetric180 = nextCombinationCanBeSymmetric180();

    placeAllLeftToPlace(leftToPlace, canBeSymmetric180, v);

#ifdef REFINEMENT
    int cntDoneLayers = 0;
    for(uint8_t i = 0; i < MAX_BRICKS; i++) {
      if(maxCombination->layerSizes[i] == baseCombination.layerSizes[i])
	cntDoneLayers++;
    }
    if(cntDoneLayers <= 1)
      return; // All done in placeAllLeftToPlace()
#endif

    LayerBrick bricks[MAX_BRICKS];

    for(uint8_t toPick = 1; toPick < leftToPlace; toPick++) {
      // Pick toPick from neighbours:
      BrickPicker picker(v, 0, toPick, bricks, 0);

      while(picker.next()) {
#ifdef REFINEMENT
	bool ok = true;
#endif
	for(uint8_t i = 0; i < toPick; i++) {
#ifdef REFINEMENT
	  // Check if layer is already full:
	  if(baseCombination.layerSizes[bricks[i].LAYER] == maxCombination->layerSizes[bricks[i].LAYER]) {
	    ok = false;
	    for(uint8_t j = 0; j < i; j++)
	      baseCombination.removeLastBrick();
	    break;
	  }
#endif
	  baseCombination.addBrick(bricks[i].BRICK, bricks[i].LAYER);
	}
#ifdef REFINEMENT
	if(!ok)
	  continue;
#endif

#ifdef LEMMAS
	bool doubleCount = false;
#else
	// Optimization: Skip half of constructions in first builder (unless symmetric):
	bool doubleCount = isFirstBuilder && !baseCombination.is180Symmetric();
	if(doubleCount) {
	  Combination rotated(baseCombination);
	  Combination baseCopy(baseCombination);
	  baseCopy.translateMinToOrigo();
	  baseCopy.sortBricks();
	  rotated.rotate180();
	  if(rotated < baseCopy) {
	    for(uint8_t i = 0; i < toPick; i++)
	      baseCombination.removeLastBrick();
	    continue; // Skip!
	  }
	}
#endif

	CombinationBuilder builder(baseCombination, waveStart+waveSize, toPick, maxSize, neighbours, maxCombination, false);
 	builder.build();
 	addCountsFrom(builder, doubleCount);

	for(uint8_t i = 0; i < toPick; i++)
	  baseCombination.removeLastBrick();
      }
    }
  }

  ThreadEnablingBuilder::ThreadEnablingBuilder() : picker(NULL), threadName("") {
#ifdef PROFILING
    Profiler::countInvocation("ThreadEnablingBuilder::ThreadEnablingBuilder()");
#endif
  }

  ThreadEnablingBuilder::ThreadEnablingBuilder(const ThreadEnablingBuilder &b) : picker(b.picker), threadName(b.threadName), b(b.b) {
  }

  ThreadEnablingBuilder::ThreadEnablingBuilder(Combination &c,
 					       const uint16_t waveStart,
 					       const uint16_t maxSize,
					       BrickPlane *neighbours,
					       Combination const * maxCombination,
 					       MultiLayerBrickPicker *picker,
					       int threadIndex) : picker(picker) {
#ifdef PROFILING
    Profiler::countInvocation("ThreadEnablingBuilder::ThreadEnablingBuilder(...)");
#endif

    b = CombinationBuilder(c, waveStart, 0, maxSize, neighbours, maxCombination, true);
    std::string names[26] = {
      "Alma", "Bent", "Coco", "Dolf", "Edna", "Finn", "Gaya", "Hans", "Inge", "Jens",
      "Kiki", "Liam", "Mona", "Nils", "Olga", "Pino", "Qing", "Rene", "Sara", "Thor",
      "Ulla", "Vlad", "Wini", "Xiao", "Yrsa", "Zorg"};
    threadName = names[threadIndex % 26];
    if(threadIndex >= 26) {
      std::stringstream ss; ss << threadName << (threadIndex/26);
      threadName = ss.str();
    }
  }

  bool CombinationBuilder::addFromPicker(MultiLayerBrickPicker *p, int &picked, const std::string &threadName) {
#ifdef PROFILING
    Profiler::countInvocation("ThreadEnablingBuilder::addFromPicker()");
#endif
    bool ret = p->next(baseCombination, picked);
    if(ret) {
      if(maxSize - baseCombination.size > 3)
	std::cout << " " << threadName << " builds on " << baseCombination << std::endl;
      waveSize = picked; // Update wave size based on pick
    }
#ifdef TRACE
    if(!ret)
      std::cout << "Done picking" << std::endl;
#endif
    return ret;
  }

  void CombinationBuilder::removeFromPicker(int toRemove) {
#ifdef PROFILING
    Profiler::countInvocation("ThreadEnablingBuilder::removeFromPicker()");
#endif
    for(uint16_t i = 0; i < toRemove; i++)
      baseCombination.removeLastBrick();
  }

  void ThreadEnablingBuilder::build() {
#ifdef PROFILING
    Profiler::countInvocation("ThreadEnablingBuilder::build()");
#endif
    int picked;
    while(b.addFromPicker(picker, picked, threadName)) {
      // Behold the beautiful C++ 11 syntax for showing elapsed time...
      std::chrono::duration<double, std::ratio<60> > duration = std::chrono::duration_cast<std::chrono::duration<double, std::ratio<60> > >(std::chrono::steady_clock::now() - timeStart);
      if(duration > std::chrono::duration<double, std::ratio<60> >(2)) // Avoid the initial chaos:
	std::cout << "Time elapsed: " << duration.count() << " minutes" << std::endl;
      b.build();
      b.removeFromPicker(picked);
    }
  }

  void CombinationBuilder::buildSplit() {
#ifdef PROFILING
    Profiler::countInvocation("ThreadEnablingBuilder::buildSplit()");
#endif
    std::vector<LayerBrick> v;
    findPotentialBricksForNextWave(v);

    const uint16_t leftToPlace = maxSize - baseCombination.size;
    const bool canBeSymmetric180 = nextCombinationCanBeSymmetric180();
    placeAllLeftToPlace(leftToPlace, canBeSymmetric180, v);
    if(leftToPlace <= 1)
      return;

    LayerBrick bricks[MAX_BRICKS];

    int processorCount = std::thread::hardware_concurrency();
#ifndef ALLTHREADS
    processorCount -= 2; // Unless ALLTHREADS specified, leave 2 cores for OS to contiue being functional.
#endif
    std::cout << "Using " << processorCount << " hardware threads" << std::endl;

    MultiLayerBrickPicker picker(v, leftToPlace-1); // Shared picker
    BrickPlane *neighbourCache = new BrickPlane[processorCount * MAX_BRICKS];
    for(int i = 0; i < processorCount * MAX_BRICKS; i++)
      neighbourCache[i].unsetAll();
    ThreadEnablingBuilder *threadBuilders = new ThreadEnablingBuilder[processorCount];
    std::thread **threads = new std::thread*[processorCount];

    for(int i = 0; i < processorCount; i++) {
      threadBuilders[i] = ThreadEnablingBuilder(baseCombination, waveStart+waveSize, maxSize, &neighbourCache[i*MAX_BRICKS], maxCombination, &picker, i);
      threads[i] = new std::thread(&ThreadEnablingBuilder::build, std::ref(threadBuilders[i]));
    }

    for(int i = 0; i < processorCount; i++) {
      threads[i]->join();
      addCountsFrom(threadBuilders[i].b, false);
      delete threads[i];
    }
    delete[] threads;
    delete[] threadBuilders;
    delete[] neighbourCache;
  }

  void CombinationBuilder::report() {
#ifdef PROFILING
    Profiler::countInvocation("CombinationBuilder::report()");
#endif
    uint8_t layerSizes[MAX_BRICKS];
    for(CountsMap::const_iterator it = counts.begin(); it != counts.end(); it++) {
      int64_t token = it->first;
      Combination::getLayerSizesFromToken(token, layerSizes);
      Counts countsForToken(it->second);

      countsForToken.symmetric180 += countsForToken.symmetric90;
      countsForToken.all += countsForToken.symmetric90;
      countsForToken.all += countsForToken.symmetric180;

      countsForToken.all /= 2 * layerSizes[0];
      countsForToken.symmetric180 /= layerSizes[0];
      countsForToken.symmetric90 /= layerSizes[0] / 2;
      std::cout << " <" << token << "> " << countsForToken << std::endl;
    }
  }

  Lemma2::Lemma2(int n): n(n) {
#ifdef PROFILING
    Profiler::countInvocation("Lemma2::Lemma2()");
#endif
  }

  bool Lemma2::computeOnBase2(bool vertical, int16_t dx, int16_t dy, Counts &c, Counts &d) {
#ifdef PROFILING
    Profiler::countInvocation("Lemma2::computeOnBase2(...)");
#endif
    Brick b2(vertical, FirstBrick.x + dx, FirstBrick.y + dy);
    if(FirstBrick.intersects(b2))
      return true;

#ifdef TRACE
    std::cout << " Handling " << b2 << std::endl;
#endif
    Combination baseCombination; // Includes first brick
    baseCombination.addBrick(b2, 0);

    std::stringstream ss; ss << "base_2_size_" << n << "/";
    if(b2.isVertical)
      ss << "parallel_dx_";
    else
      ss << "crossing_dx_";
    ss << (int)dx << "_dy_" << (int)dy << ".txt";
    std::string fileName = ss.str();

    // Check if file already exists:
    std::ifstream istream(fileName.c_str());
    if(istream.good()) {
      std::string connectivity, ignore;
      int64_t token;
      uint64_t total, symmetric180;
      while(istream >> connectivity >> token >> ignore >> total >> ignore >> symmetric180) {
	Counts counts(total, symmetric180, 0);
	if(connectivity == "CONNECTED") {
	  c += counts;
	}
	else {
	  d += counts;
	}
      }
      return false;
    }

    BrickPlane neighbours[MAX_BRICKS];
    for(uint8_t i = 0; i < MAX_BRICKS; i++)
      neighbours[i].unsetAll();
    CombinationBuilder builder(baseCombination, 0, 2, n, neighbours, NULL, true);
    builder.buildSplit();

    std::ofstream ostream(fileName.c_str());

    for(CountsMap::const_iterator it = builder.counts.begin(); it != builder.counts.end(); it++) {
      int64_t token = it->first;
#ifdef TRACE
      std::cout << "Handling token " << token << " on " << baseCombination << ": " << it->second << std::endl;
#endif
      int color1 = token % 10;
      token /= 10;
      int color2 = token % 10;
      token /= 10;
      if(color1 != color2) {
	ostream << "DISCONNECTED ";
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
#ifdef PROFILING
    Profiler::countInvocation("Lemma2::computeOnBase2()");
#endif
    for(int rotation = 0; rotation <= 1; rotation++) {
      Counts prevDisconnectedX;
      for(int16_t dx = 0; true; dx++) {
	Counts connectedX, disconnectedX;

	Counts prevDisconnectedY;
	for(int16_t dy = 0; true; dy++) {
	  assert(dy < PLANE_WIDTH);
	  Counts connectedY, disconnectedY;

	  bool skipped = computeOnBase2((bool)rotation, dx, dy, connectedY, disconnectedY);
	  if(skipped) {
	    continue;
	  }
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

  BitWriter::BitWriter() : ostream(NULL), base(0), bits(0), cntBits(0), sumTotal(0), sumSymmetric180(0), sumSymmetric90(0), lines(0) {
  }
  BitWriter::BitWriter(const BitWriter &w) : ostream(w.ostream), base(w.base), bits(w.bits), cntBits(w.cntBits), sumTotal(w.sumTotal), sumSymmetric180(w.sumSymmetric180), sumSymmetric90(w.sumSymmetric90), lines(w.lines) {
  }
  BitWriter::BitWriter(const std::string &fileName, uint8_t base) : base(base), bits(0), cntBits(0), sumTotal(0), sumSymmetric180(0), sumSymmetric90(0), lines(0)  {
    
    ostream = new std::ofstream(fileName.c_str(), std::ios::binary);
  }
  BitWriter::~BitWriter() {
    // End indicator:
    writeBit(1);
    writeBit(0); // baseSymmetric180
    if(base % 4 == 0)
      writeBit(0); // baseSymmetric90
    for(int i = 0; i < base-1; i++)
      writeColor(0);
    writeUInt8(0); // Token
    writeUInt32(0); // total
    writeUInt16(0); // symmetric180
    if(base % 4 == 0)
      writeUInt8(0); // symmetric90
    // Totals:
    writeUInt64(base);
    writeUInt64(sumTotal);
    writeUInt64(sumSymmetric180);
    writeUInt64(sumSymmetric90);
    writeUInt64(lines);
    // Close and delete stream:
    flushBits();
    ostream->flush();
    ostream->close();
    delete ostream;
  }
  void BitWriter::writeColor(uint8_t toWrite) {
    assert(toWrite < 8);
    for(int j = 0; j < 3; j++) {
      writeBit(toWrite & 1);
      toWrite >>= 1;
    }
  }
  void BitWriter::writeBit(bool bit) {
    bits = (bits << 1) + (bit ? 1 : 0);
    cntBits++;
    if(cntBits == 8) {
      ostream->write((char*)&bits, 1);
      cntBits = 0;
    }
  }
  void BitWriter::writeCounts(const Counts &c) {
    assert(c.all < 4294967295);
    writeUInt32(c.all);
    sumTotal += c.all;

    assert(c.symmetric180 < 65535);
    writeUInt16(c.symmetric180);
    sumSymmetric180 += c.symmetric180;

    if(base % 4 == 0) {
      assert(c.symmetric90 < 255);
      writeUInt8(c.symmetric90);
      sumSymmetric90 += c.symmetric90;
    }

    lines++;
  }
  void BitWriter::flushBits() {
    while(cntBits > 0) {
      writeBit(0);
    }
  }
  void BitWriter::writeUInt8(uint8_t toWrite) {
    for(int j = 0; j < 8; j++) {
      writeBit(toWrite & 1);
      toWrite >>= 1;
    }
  }
  void BitWriter::writeUInt16(uint16_t toWrite) {
    for(int j = 0; j < 16; j++) {
      writeBit(toWrite & 1);
      toWrite >>= 1;
    }
  }
  void BitWriter::writeUInt32(uint32_t toWrite) {
    for(int j = 0; j < 32; j++) {
      writeBit(toWrite & 1);
      toWrite >>= 1;
    }
  }
  void BitWriter::writeUInt64(uint64_t toWrite) {
    for(int j = 0; j < 64; j++) {
      writeBit(toWrite & 1);
      toWrite >>= 1;
    }
  }

  bool BitReader::readBit() {
    if(bitIdx == 8) {
      istream->read((char*)&bits, 1);
      bitIdx = 0;
    }
    bool bit = (bits >> (7-bitIdx)) & 1;
    bitIdx++;
    return bit;
  }
  uint8_t BitReader::readColor() {
    uint8_t ret = 0;
    for(int i = 0; i < 3; i++) {
      ret = ret | ((int)readBit() << i);
    }
    return ret;
  }
  uint8_t BitReader::readUInt8() {
    uint8_t ret = 0;
    for(int i = 0; i < 8; i++) {
      ret = ret | ((int)readBit() << i);
    }
    return ret;
  }
  uint16_t BitReader::readUInt16() {
    uint16_t ret = 0;
    for(int i = 0; i < 16; i++) {
      ret = ret | ((int)readBit() << i);
    }
    return ret;
  }
  uint32_t BitReader::readUInt32() {
    uint32_t ret = 0;
    for(int i = 0; i < 32; i++) {
      ret = ret | ((int)readBit() << i);
    }
    return ret;
  }
  uint64_t BitReader::readUInt64() {
    uint64_t ret = 0;
    for(int i = 0; i < 64; i++) {
      ret = ret | ((uint64_t)readBit() << i);
    }
    return ret;
  }
  void BitReader::readCounts(Counts &c) {
    c.all = readUInt32();
    c.symmetric180 = readUInt16();
    if(base % 4 == 0)
      c.symmetric90 = readUInt8();
  }
  BitReader::BitReader(uint8_t base, int n, int token, int D) : bits(0), bitIdx(8), base(base), sumTotal(0), sumSymmetric180(0), sumSymmetric90(0), lines(0) {
    std::stringstream ss;
    ss << "base_" << (int)base << "_size_" << (int)n;
    ss << "_refinement_" << Combination::reverseToken(token);
    ss << "/d" << (int)D << ".bin";
    std::string fileName = ss.str();
    istream = new std::ifstream(fileName.c_str(), std::ios::binary);
    bool firstBit = readBit();
    assert(firstBit);
    std::cout << "  Reader set up for " << fileName << std::endl;
  }
  BitReader::~BitReader() {
    if(istream != NULL) {
      istream->close();
      delete istream;
    }
  }
  bool BitReader::next(std::vector<Report> &v) {
    Report r;
    r.baseSymmetric180 = readBit();
    r.baseSymmetric90 = (base % 4 == 0) && readBit();
    bool first = true;
    while(true) {
      if(!first) {
	bool indicator = readBit();
	if(indicator)
	  return true; // Now at next batch
      }
      first = false;
      for(int i = 0; i < base-1; i++)
	r.colors[i] = readColor();
      uint64_t token = readUInt8();
      r.counts.all = readUInt32();
      sumTotal += r.counts.all;
      r.counts.symmetric180 = readUInt16();
      sumSymmetric180 += r.counts.symmetric180;
      r.counts.symmetric90 = 0;
      if(base % 4 == 0)
	r.counts.symmetric90 = readUInt8();
      sumSymmetric90 += r.counts.symmetric90;
      if(token == 0) {
	// Cross checks:
	uint64_t readBase = readUInt64();
	uint64_t readTotal = readUInt64();
	uint64_t readSumSymmetric180 = readUInt64();
	uint64_t readSumSymmetric90 = readUInt64();
	uint64_t readLines = readUInt64();
	if(readBase != base ||
	   readTotal != sumTotal ||
	   readSumSymmetric180 != sumSymmetric180 ||
	   readSumSymmetric90 != sumSymmetric90 ||
	   readLines != lines) {
	  std::cerr << "Cross check error: Cross check value from file vs counted:" << std::endl;
	  std::cerr << " Base: " << readBase << " vs " << (int)base << std::endl;
	  std::cerr << " Total: " << readTotal << " vs " << sumTotal << std::endl;
	  std::cerr << " Total 180 degree symmetries: " << readSumSymmetric180 << " vs " << sumSymmetric180 << std::endl;
	  std::cerr << " Total 90 degree symmetries: " << readSumSymmetric90 << " vs " << sumSymmetric90 << std::endl;
	  std::cerr << " Lines: " << readLines << " vs " << lines << std::endl;
	  assert(false);
	}
	return false;
      }
      lines++;
      assert(r.counts.all > 0);
      v.push_back(r);
    }
  }

  Lemma3::Lemma3(int n, int base, Combination const * maxCombination): n(n), base(base), interceptionSkips(0), mirrorSkips(0), maxCombination(maxCombination) {
#ifdef PROFILING
    Profiler::countInvocation("Lemma3::Lemma3()");
#endif
    assert(base >= 3);
    assert(base < n);
    assert(n <= MAX_BRICKS);
    for(uint8_t i = 0; i < MAX_BRICKS; i++)
      neighbours[i].unsetAll();
    shouldSplit = n - base > 3;
    if(maxCombination != NULL) {
      if(maxCombination->height == 2)
	shouldSplit = false;
    }
  }

  void Lemma3::precompute(int maxDist) {
#ifdef PROFILING
    Profiler::countInvocation("Lemma3::precompute()");
#endif
    for(int d = 2; d <= maxDist; d++) {
      std::chrono::time_point<std::chrono::steady_clock> timeStart { std::chrono::steady_clock::now() };
      
      std::stringstream ss; ss << "base_" << base << "_size_" << n;
      if(maxCombination != NULL) {
	ss << "_refinement_";
	for(uint8_t i = 0; i < maxCombination->height; i++)
	  ss << (int)maxCombination->layerSizes[i];
      }
      ss << "/d" << d << ".bin";
      std::string fileName = ss.str();
      
      std::ifstream istream(fileName.c_str());
      if(istream.good()) {
	std::cout << "Precomputation already exists. Skipping!" << std::endl;
	continue; // File already exists
      }

      BitWriter writer(fileName, base);
      std::vector<int> distances;

      precompute(distances, writer, d);

      std::chrono::duration<double, std::ratio<1> > duration = std::chrono::duration_cast<std::chrono::duration<double, std::ratio<1> > >(std::chrono::steady_clock::now() - timeStart);
      std::cout << "Precomputation done for max distance " << d << " in " << duration.count() << " seconds" << std::endl;
    }

    std::cout << " Skips due to no possible overlaps: " << interceptionSkips << ", mirror skips: " << mirrorSkips << std::endl;

    // Cross checks:
    std::cout << "Cross checks:" << std::endl;
    for(CountsMap::const_iterator it = crossCheck.begin(); it != crossCheck.end(); it++) {
      std::cout << " " << it->first << ": " << it->second << std::endl;
    }
  }

  void Lemma3::checkMirrorSymmetries(const Combination &baseCombination,
				     const CombinationResultsMap &seen,
				     CountsMap &cm) {
    Combination mx(baseCombination);
    mx.mirrorX();
    CombinationResultsMap::const_iterator it;
    if((it = seen.find(mx)) != seen.end()) {
      cm = it->second;
      mirrorSkips++;
      return;
    }

    Combination my(baseCombination);
    my.mirrorY();
    if((it = seen.find(my)) != seen.end()) {
      cm = it->second;
      mirrorSkips++;
      return;
    }

    // We do not mirror in both X and Y, as that is equal to 180 degree rotation.
  }

  void Lemma3::precomputeOn(const Combination &baseCombination,
			    BitWriter &writer,
			    CombinationResultsMap &seen) {
#ifdef PROFILING
    Profiler::countInvocation("Lemma3::precomputeOn()");
#endif
    CountsMap cm;

    // Optimization: Mirror symmetries:
    checkMirrorSymmetries(baseCombination, seen, cm);
    bool anyMirrorSymmetries = !cm.empty();

    // Optimization: If no interceptions between bricks, then skip!
    bool anyInterceptions = !anyMirrorSymmetries && baseCombination.anyInterceptions(n-base);

    if(anyMirrorSymmetries) {
      // cm already set!
    }
    else if(anyInterceptions || noInterceptionsMap.empty()) {
      if(shouldSplit)
	std::cout << " Precomputing on " << baseCombination << std::endl;
      CombinationBuilder builder(baseCombination, 0, (uint8_t)base, (uint8_t)n, neighbours, maxCombination, true);
      if(shouldSplit)
	builder.buildSplit();
      else
	builder.build();
      cm = builder.counts;
      if(!anyInterceptions) {
	std::cout << "First no-interceptions map set up!" << std::endl;
	noInterceptionsMap = cm;
      }
    }
    else {
#ifdef TRACE
      std::cout << "No interceptions with " << (n-base) << " for " << baseCombination << std::endl;
#endif
      interceptionSkips++;
      if(interceptionSkips % 10000 == 0) {
	std::cout << " Skips due to no possible overlaps: " << interceptionSkips << ", mirror skips: " << mirrorSkips << std::endl;
      }
      cm = noInterceptionsMap;
    }

    // Update cache:
    seen[baseCombination] = cm;

    // Write results:
    bool baseSymmetric180 = baseCombination.is180Symmetric();
    bool baseSymmetric90 = baseSymmetric180 && (base % 4 == 0) && baseCombination.is90Symmetric();
    writer.writeBit(1); // New batch
    writer.writeBit(baseSymmetric180);
    if(base % 4 == 0)
      writer.writeBit(baseSymmetric90);

    bool any = false;
    CountsMap xCheck;
    for(CountsMap::const_iterator it = cm.begin(); it != cm.end(); it++) {
      if(any)
	writer.writeBit(0);
      any = true;
      int64_t token = it->first;
      int colors[MAX_LAYER_SIZE];
      for(int i = 0; i < base; i++) {
	colors[base-1-i] = token % 10;
	token /= 10;
      }

      bool allOnes = true;
      for(int i = 1; i < base; i++) {
	writer.writeColor(colors[i] - 1);
	if(colors[i] != 1)
	  allOnes = false;
      }
      if(allOnes) {
	if(xCheck.find(token) == xCheck.end())
	  xCheck[token] = Counts();
	xCheck[token] += it->second;
      }

      token = Combination::reverseToken(token);
      token /= 10; // Remove base layer

      assert(Combination::sizeOfToken(token) == n - base);

      writer.writeUInt8(token);
      writer.writeCounts(it->second);
    }
    for(CountsMap::const_iterator it = xCheck.begin(); it != xCheck.end(); it++) {
      uint64_t token = it->first;
      Counts c = it->second;
      if(baseSymmetric180) {
	if(baseSymmetric90) {
	  c.all -= c.symmetric180 + c.symmetric90;
	  assert(c.symmetric90 % 4 == 0);
	  c.symmetric90 /= 4;
	  assert(c.symmetric180 % 2 == 0);
	  c.symmetric180 /= 2;
	  assert(c.all % 4 == 0);
	  c.all = c.all/4 + c.symmetric180 + c.symmetric90;
	}
	else
	  c.all = (c.all - c.symmetric180)/2 + c.symmetric180;
      }
      if(crossCheck.find(token) == crossCheck.end())
	crossCheck[token] = c;
      else
	crossCheck[token] += c;
    }
  }

  void Lemma3::precomputeForPlacements(const std::vector<int> &distances,
				       std::vector<Brick> &bricks,
				       BitWriter &writer,
				       CombinationResultsMap &seen) {
#ifdef PROFILING
    Profiler::countInvocation("Lemma3::precomputeForPlacements()");
#endif
    int S = (int)bricks.size();

    if(S == base-1) {
      Combination baseCombination; // Includes first brick
      for(int i = 0; i < S; i++)
	baseCombination.addBrick(bricks[i], 0);
      baseCombination.normalize();
      if(seen.find(baseCombination) != seen.end()) {
#ifdef TRACE
	std::cout << "   Already seen! " << baseCombination << std::endl;
#endif
	return;
      }

      // Check that baseCombination does not belong to another time:
      std::vector<int> baseCombinationDistances;
      for(int i = 1; i < base; i++) {
	int dist = baseCombination.bricks[0][i].dist(FirstBrick);
	baseCombinationDistances.push_back(dist);
      }
      std::sort(baseCombinationDistances.begin(), baseCombinationDistances.end());
      for(int i = 0; i < base-1; i++) {
	if(baseCombinationDistances[i] != distances[i]) {
#ifdef TRACE
	  std::cout << "Combination belongs to another time: " << baseCombination;
	  for(int j = 0; j < base-1; j++)
	    std::cout << " " << baseCombinationDistances[j];
	  std::cout << std::endl;
#endif
	  return;
	}
      }

      // All OK! Precompute:
      precomputeOn(baseCombination, writer, seen);
      return;
    }

    int DS = distances[S];
    for(int16_t dx = 0; dx <= DS; dx++) {
      int16_t dy = DS - dx;

      for(int16_t multX = -1; multX <= 1; multX += 2) {
	for(int16_t multY = -1; multY <= 1; multY += 2) {
	  for(int v = 0; v <= 1; v++) {
	    Brick b((bool)v, FirstBrick.x + multX*dx, FirstBrick.y + multY*dy);
	    if(b.intersects(FirstBrick))
	      continue;
	    // Check for colission:
	    bool ok = true;
	    for(int i = 0; i < S; i++) {
	      if(bricks[i].intersects(b)) {
		ok = false;
		break;
	      }
	    }
	    if(!ok)
	      continue;

	    // Recursion:
	    bricks.push_back(b);
	    precomputeForPlacements(distances, bricks, writer, seen);
	    bricks.pop_back();
	  } // for v
	} // for multY
      } // for multX
    } // for dx
  }

  void Lemma3::precompute(std::vector<int> &distances, BitWriter &writer, int maxDist) {
#ifdef PROFILING
    Profiler::countInvocation("Lemma3::precompute(...)");
#endif
    int S = (int)distances.size();

    if(S == base-2) {
      std::cout << " Precomputing base " << base << " distances";
      for(int i = 0; i < S; i++)
	std::cout << " " << distances[i];
      std::cout << " " << maxDist << std::endl;

      std::chrono::time_point<std::chrono::steady_clock> timeStart { std::chrono::steady_clock::now() };

      std::vector<Brick> bricks;
      CombinationResultsMap seen;
      distances.push_back(maxDist); // Last dist is max dist
      precomputeForPlacements(distances, bricks, writer, seen);
      distances.pop_back();

      std::chrono::duration<double, std::ratio<1> > duration = std::chrono::duration_cast<std::chrono::duration<double, std::ratio<1> > >(std::chrono::steady_clock::now() - timeStart);
      if(duration.count() > 1)
	std::cout << "  Precomputation time: " << duration.count() << " seconds" << std::endl;
      return;
    }

    int prevD = distances.empty() ? 2 : distances[S-1];
    for(int d = prevD; d <= maxDist; d++) {
      distances.push_back(d);
      precompute(distances, writer, maxDist);
      distances.pop_back();
    }
  }

} // namespace rectilinear
