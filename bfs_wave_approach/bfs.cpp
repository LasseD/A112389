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
  Counts& Counts::operator -=(const Counts& c) {
#ifdef PROFILING
    Profiler::countInvocation("Counts::operator -=");
#endif
    all -= c.all;
    symmetric180 -= c.symmetric180;
    symmetric90 -= c.symmetric90;
    return *this;
  }
  Counts Counts::operator -(const Counts& c) const {
#ifdef PROFILING
    Profiler::countInvocation("Counts::operator -");
#endif
    return Counts(all-c.all, symmetric180-c.symmetric180, symmetric90-c.symmetric90);
  }
  Counts Counts::operator /(const int& v) const {
#ifdef PROFILING
    Profiler::countInvocation("Counts::operator /");
#endif
    assert(all % v == 0);
    assert(symmetric180 % v == 0);
    assert(symmetric90 % v == 0);
    return Counts(all/v, symmetric180/v, symmetric90/v);
  }
  bool Counts::operator ==(const Counts& c) const {
#ifdef PROFILING
    Profiler::countInvocation("Counts::operator ==");
#endif
    return all == c.all && symmetric180 == c.symmetric180 && symmetric90 == c.symmetric90;
  }
  bool Counts::operator !=(const Counts& c) const {
#ifdef PROFILING
    Profiler::countInvocation("Counts::operator !=");
#endif
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
      return DIFFLT(b.y, y, 2) && DIFFLT(b.x, x, 4);
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
#ifdef PROFILING
    Profiler::countInvocation("Brick::operator /");
#endif
    return ABS(x-b.x) + ABS(y-b.y);
  }
  bool Brick::canReach(const Brick &a, const Brick &b, uint8_t toAdd) {
#ifdef PROFILING
    Profiler::countInvocation("Brick::canReach()");
#endif
    if(toAdd == 0)
      return false;
    if(a.intersects(b))
      return true;

    // Ensure a.isVertical:
    if(!a.isVertical) {
      if(!b.isVertical)
	return canReach(Brick(true, a.y, a.x), Brick(true, b.y, b.x), toAdd);
      return canReach(b, a, toAdd);
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
    return canReach(Brick(true, a.x+MIN(1, dx)*signX, a.y+MIN(3,dy)*signY), b, toAdd-1) ||
      canReach(Brick(false, a.x+MIN(2,dx)*signX, a.y+MIN(2,dy)*signY), b, toAdd-1);
  }

  BrickPicker::BrickPicker(const std::vector<LayerBrick> &v,
			   const int vIdx,
			   const int numberOfBricksToPick): v(v), vIdx(vIdx-1), numberOfBricksToPick(numberOfBricksToPick), inner(NULL) {
#ifdef PROFILING
    Profiler::countInvocation("BrickPicker::BrickPicker()");
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

  bool BrickPicker::checkVIdx(const Combination &c, const Combination &maxCombination) const {
#ifdef PROFILING
    Profiler::countInvocation("BrickPicker::checkVIdx()");
#endif
    // Check for colissions against placed bricks:
    uint8_t layer = v[vIdx].LAYER;
    assert(layer <= c.height);
    if(c.height == layer)
      return true; // Placed on top!
    if(c.layerSizes[layer] == maxCombination.layerSizes[layer])
      return false;
    for(uint8_t i = 0; i < c.layerSizes[layer]; i++) {
      if(c.bricks[layer][i].intersects(v[vIdx].BRICK))
	return false;
    }
    return true;
  }

  void BrickPicker::nextVIdx(const Combination &c, const Combination &maxCombination) {
#ifdef PROFILING
    Profiler::countInvocation("BrickPicker::nextVIdx()");
#endif
    do {
      vIdx++;
      if(vIdx >= (int)v.size())
	return; // Out of bounds!
    }
    while(!checkVIdx(c, maxCombination));
  }

  bool BrickPicker::next(Combination &c, const Combination &maxCombination) {
#ifdef PROFILING
    Profiler::countInvocation("BrickPicker::next()");
#endif
    if(numberOfBricksToPick == 1) {
      nextVIdx(c, maxCombination);
      bool ok = vIdx < (int)v.size();
      if(ok)
	c.addBrick(v[vIdx].BRICK, v[vIdx].LAYER);
      return ok;
    }

    // More than one brick to pick:
    if(inner != NULL) {
      c.addBrick(v[vIdx].BRICK, v[vIdx].LAYER);
      if(inner->next(c, maxCombination))
	return true; // inner placed remaining bricks
      c.removeLastBrick(); // Could not complete
      delete inner;
      inner = NULL;
    }

    while(true) {
      nextVIdx(c, maxCombination);
      if(vIdx + numberOfBricksToPick - 1 >= (int)v.size())
	return false;
      c.addBrick(v[vIdx].BRICK, v[vIdx].LAYER);
      inner = new BrickPicker(v, vIdx+1, numberOfBricksToPick-1);
      if(inner->next(c, maxCombination))
	return true;
      c.removeLastBrick(); // Could not complete
      delete inner;
      inner = NULL;
    }
  }

  MultiBatchSizeBrickPicker::MultiBatchSizeBrickPicker(const std::vector<LayerBrick> &v, const int maxPick) : v(v), maxPick(maxPick), toPick(1) {
#ifdef PROFILING
    Profiler::countInvocation("MultiBatchSizeBrickPicker::MultiBatchSizeBrickPicker()");
#endif
    assert(maxPick >= 1);
    inner = new BrickPicker(v, 0, 1);
  }

  bool MultiBatchSizeBrickPicker::next(Combination &c, int &picked, const Combination &maxCombination) {
    std::lock_guard<std::mutex> guard(nextMutex);
#ifdef PROFILING
    Profiler::countInvocation("MultiBatchSizeBrickPicker::next()");
#endif
    if(inner == NULL)
      return false;

    bool ret;
    while(!(ret = inner->next(c, maxCombination))) {
      delete inner;
      if(toPick == maxPick) { // Done!
	inner = NULL;
	return false;
      }
      inner = new BrickPicker(v, 0, ++toPick);
    }

    picked = toPick;
    return true;
  }

  void BrickPlane::unsetAll() {
#ifdef PROFILING
    Profiler::countInvocation("BrickPlane::unsetAll()");
#endif
    for(int i = 0; i < 2; i++) {
      for(int j = 0; j < PLANE_WIDTH; j++) {
	for(int k = 0; k < PLANE_WIDTH; k++) {
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
  }
  Combination::Combination(const Combination &b) {
#ifdef PROFILING
    Profiler::countInvocation("Combination::Combination(Combination &)");
#endif
    copy(b);
  }
  Combination::Combination(const Base &b) : height(1), size(b.layerSize) {
#ifdef PROFILING
    Profiler::countInvocation("Combination::Combination(Base &)");
#endif
    layerSizes[0] = size;
    for(uint8_t i = 0; i < size; i++) {
      bricks[0][i] = b.bricks[i];
      history[i] = BrickIdentifier(0, i);
    }
  }

  Base::Base() : layerSize(1) {
#ifdef PROFILING
    Profiler::countInvocation("Base::Base()");
#endif
    bricks[0] = FirstBrick;
  }
  Base::Base(const Base &b) : layerSize(b.layerSize) {
#ifdef PROFILING
    Profiler::countInvocation("Base::Base(Base &)");
#endif
    for(uint8_t i = 0; i < layerSize; i++)
      bricks[i] = b.bricks[i];
  }

   bool Combination::operator <(const Combination& b) const {
#ifdef PROFILING
    Profiler::countInvocation("Combination::operator <");
#endif
    if(height != b.height)
      return height < b.height;

    for(uint8_t i = 0; i < height; i++) {
      uint8_t s = layerSizes[i];

      if(s != b.layerSizes[i])
	return s < b.layerSizes[i];

      for(uint8_t j = 0; j < s; j++) {
	int res = bricks[i][j].cmp(b.bricks[i][j]);
	if(res != 0)
	  return res < 0;
      }
    }
    return false;
  }

   bool Base::operator <(const Base& b) const {
#ifdef PROFILING
    Profiler::countInvocation("Base::operator <");
#endif
    if(layerSize != b.layerSize)
      return layerSize < b.layerSize;

    for(uint8_t j = 0; j < layerSize; j++) {
      int res = bricks[j].cmp(b.bricks[j]);
      if(res != 0)
	return res < 0;
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
	if(bricks[i][j] != b.bricks[i][j])
	  return false;
      }
    }
    return true;
  }

  bool Base::operator ==(const Base& b) const {
#ifdef PROFILING
    Profiler::countInvocation("Base::operator ==");
#endif
    if(layerSize != b.layerSize)
      return false;
    for(uint8_t j = 0; j < layerSize; j++) {
      if(bricks[j] != b.bricks[j])
	return false;
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
	os << " " << b.bricks[i][j];
      }
    }
    return os;
  }

  std::ostream& operator << (std::ostream &os, const Base &b) {
    os << "BASE";
    for(uint8_t j = 0; j < b.layerSize; j++)
      os << " " << b.bricks[j];
    return os;
  }

  void Combination::copy(const Combination &b) {
#ifdef PROFILING
    Profiler::countInvocation("Combination::copy()");
#endif
    height = b.height;
    size = b.size;
    for(uint8_t i = 0; i < b.height; i++)
      layerSizes[i] = b.layerSizes[i];
    for(uint8_t i = 0; i < height; i++) {
      for(uint8_t j = 0; j < layerSizes[i]; j++)
	bricks[i][j] = b.bricks[i][j];
    }
    for(uint8_t i = 0; i < size; i++)
      history[i] = b.history[i];
  }

  void Base::copy(const Base &b) {
#ifdef PROFILING
    Profiler::countInvocation("Base::copy()");
#endif
    layerSize = b.layerSize;
    for(uint8_t j = 0; j < layerSize; j++)
      bricks[j] = b.bricks[j];
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

  void Base::sortBricks() {
#ifdef PROFILING
    Profiler::countInvocation("Base::sortBricks()");
#endif
    std::sort(bricks, &bricks[layerSize]);
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

  void Base::translateMinToOrigo() {
#ifdef PROFILING
    Profiler::countInvocation("Base::translateMinToOrigo()");
#endif
    int16_t minx = 10000, miny = 10000;

    for(uint8_t j = 0; j < layerSize; j++) {
      Brick &b = bricks[j];
      if(b.isVertical) {
	// Vertical bricks in layer 0 can be 'min':
	if(b.x < minx || (b.x == minx && b.y < miny)) {
	  minx = b.x;
	  miny = b.y;
	}
      }
    }
    // Move all in relation to smallest min:
    for(uint8_t j = 0; j < layerSize; j++) {
      bricks[j].x += PLANE_MID - minx;
      bricks[j].y += PLANE_MID - miny;
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

  void Base::rotate90() {
#ifdef PROFILING
    Profiler::countInvocation("Base::rotate90()");
#endif
    for(uint8_t j = 0; j < layerSize; j++) {
      const Brick &b = bricks[j];
      bricks[j] = Brick(!b.isVertical, b.y, PLANE_MID - (b.x - PLANE_MID));
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
    sortBricks(); // TODO: Is std::sort fast enough?
  }

  void Base::rotate180() {
#ifdef PROFILING
    Profiler::countInvocation("Base::rotate180()");
#endif
    // Perform rotation:
    for(uint8_t j = 0; j < layerSize; j++) {
      Brick &b = bricks[j];
      b.x = PLANE_MID - (b.x - PLANE_MID);
      b.y = PLANE_MID - (b.y - PLANE_MID);
    }
    translateMinToOrigo();
    sortBricks(); // TODO: Is std::sort fast enough?
  }

  void Base::mirrorX() {
#ifdef PROFILING
    Profiler::countInvocation("Base::mirrorX()");
#endif
    for(uint8_t j = 0; j < layerSize; j++) {
      Brick &b = bricks[j];
      b.x = PLANE_MID - (b.x - PLANE_MID);
    }
    translateMinToOrigo();
    sortBricks(); // TODO: Is std::sort fast enough?
  }

  void Base::mirrorY() {
#ifdef PROFILING
    Profiler::countInvocation("Base::mirrorY()");
#endif
    for(uint8_t j = 0; j < layerSize; j++) {
      Brick &b = bricks[j];
      b.y = PLANE_MID - (b.y - PLANE_MID);
    }
    translateMinToOrigo();
    sortBricks(); // TODO: Is std::sort fast enough?
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

  void Base::getLayerCenter(int16_t &cx, int16_t &cy) const {
#ifdef PROFILING
    Profiler::countInvocation("Base::getLayerCenter()");
#endif
    cx = 0;
    cy = 0;
    for(uint8_t i = 0; i < layerSize; i++) {
      cx += bricks[i].x;
      cy += bricks[i].y;
    }
    cx *= 2;
    cy *= 2;
    cx /= layerSize;
    cy /= layerSize;
  }

  bool Combination::isLayerSymmetric(const uint8_t layer, const int16_t &cx, const int16_t &cy) const {
#ifdef PROFILING
    Profiler::countInvocation("Combination::isLayerSymmetric()");
#endif
    assert(layer < height);
    const uint8_t layerSize = layerSizes[layer];
    if(layerSize == 2) {
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
      Brick seen[MAX_LAYER_SIZE];
      int seenSize = 0;
      for(uint8_t i = 0; i < layerSize; i++) {
	const Brick &b = bricks[layer][i];
	if(b.x*2 == cx && b.y*2 == cy)
	  continue; // Skip brick in center
	bool found = false;
	for(int i = 0; i < seenSize; i++) {
	  if(b.mirrorEq(seen[i], cx, cy)) {
	    found = true;
	    // RM:
	    seenSize--;
	    for(int j = i; j < seenSize; j++)
	      seen[j] = seen[j+1];
	    break;
	  }
	}
	if(!found)
	  seen[seenSize++] = b;
      }
      return seenSize == 0;
    }
  }

  bool Base::isLayerSymmetric(const int16_t &cx, const int16_t &cy) const {
#ifdef PROFILING
    Profiler::countInvocation("Base::isLayerSymmetric()");
#endif
    Brick seen[MAX_LAYER_SIZE];
    int seenSize = 0;
    for(uint8_t i = 0; i < layerSize; i++) {
      const Brick &b = bricks[i];
      if(b.x*2 == cx && b.y*2 == cy)
	continue; // Skip brick in center
      bool found = false;
      for(int i = 0; i < seenSize; i++) {
	if(b.mirrorEq(seen[i], cx, cy)) {
	  found = true;
	  // RM:
	  seenSize--;
	  for(int j = i; j < seenSize; j++)
	    seen[j] = seen[j+1];
	  break;
	}
      }
      if(!found)
	seen[seenSize++] = b;
    }
    return seenSize == 0;
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

  bool Base::is180Symmetric() const {
#ifdef PROFILING
    Profiler::countInvocation("Base::is180Symmetric()");
#endif
    int16_t cx0, cy0;
    getLayerCenter(cx0, cy0);
    return isLayerSymmetric(cx0, cy0);
  }

  bool Combination::is90Symmetric() const {
#ifdef PROFILING
    Profiler::countInvocation("Combination::is90Symmetric()");
#endif
    if((size & 3) != 0)
      return false;
    if(!canRotate90())
      return false;
    for(uint8_t i = 0; i < height; i++) {
      if((layerSizes[i] & 3) != 0)
	return false;
    }
    Combination rotated(*this);
    rotated.rotate90();
    Combination dis(*this);
    dis.translateMinToOrigo();
    dis.sortBricks();      
    return rotated == dis;
  }

  bool Base::is90Symmetric() const {
#ifdef PROFILING
    Profiler::countInvocation("Base::is90Symmetric()");
#endif
    if((layerSize & 3) != 0)
      return false;
    if(!canRotate90())
      return false;
    Base rotated(*this);
    rotated.rotate90();
    Base dis(*this);
    dis.translateMinToOrigo();
    dis.sortBricks();      
    return rotated == dis;
  }

  bool Combination::canRotate90() const {
#ifdef PROFILING
    Profiler::countInvocation("Combination::canRotate90()");
#endif
    for(uint8_t i = 0; i < layerSizes[0]; i++) {
      if(!bricks[0][i].isVertical)
	return true;
    }
    return false;
  }

  bool Base::canRotate90() const {
#ifdef PROFILING
    Profiler::countInvocation("Base::canRotate90()");
#endif
    for(uint8_t i = 0; i < layerSize; i++) {
      if(!bricks[i].isVertical)
	return true;
    }
    return false;
  }

  void Combination::addBrick(const Brick &b, const uint8_t layer) {
#ifdef PROFILING
    Profiler::countInvocation("Combination::addBrick()");
#endif
    if(layer == height) {
      height++;
      layerSizes[layer] = 0;
    }
    const int8_t layerSize = layerSizes[layer];
    history[size] = BrickIdentifier(layer, layerSize);
    bricks[layer][layerSize] = b;
    size++;
    layerSizes[layer]++;
  }

  void Combination::removeLastBrick() {
#ifdef PROFILING
    Profiler::countInvocation("Combination::removeLastBrick()");
#endif
    size--;
    const uint8_t layer = history[size].first;

    layerSizes[layer]--;
    if(layerSizes[layer] == 0)
      height--;
  }

  void Combination::colorConnected(uint8_t layer, uint8_t idx, uint8_t color) {
#ifdef PROFILING
    Profiler::countInvocation("Combination::colorConnected()");
#endif
    assert(layer < height);
    assert(idx < layerSizes[layer]);
    assert(color > 0);
    if(colors[layer][idx] != 0)
      return; // Already colored
    colors[layer][idx] = color;
    const Brick &b = bricks[layer][idx];
    // Color for layers below and above:
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

  uint8_t Combination::countConnected(uint8_t layer, uint8_t idx) {
    assert(layer < height);
    assert(idx < layerSizes[layer]);
    if(colors[layer][idx] != 0)
      return 0; // Already counted
    colors[layer][idx] = 1;
    uint8_t ret = 1;
    const Brick &b = bricks[layer][idx];
    // Add for layers below and above:
    for(int8_t layer2 = -1+(int8_t)layer; layer2 <= layer+1; layer2 += 2) {
      if(layer2 < 0 || layer2 >= height)
	continue;

      uint8_t s = layerSizes[layer2];
      for(uint8_t i = 0; i < s; i++) {
	if(colors[layer2][i] != 0)
	  continue; // Already counted
	const Brick &b2 = bricks[layer2][i];
	if(b.intersects(b2))
	  ret += countConnected(layer2, i);
      }
    }
    return ret;
  }

  bool Combination::isConnected() {
    // Reset state:
    for(uint8_t i = 0; i < height; i++) {
      uint8_t s = layerSizes[i];
      for(uint8_t j = 0; j < s; j++)
	colors[i][j] = 0;
    }
    // Count:
    return size == countConnected(0, 0);
  }

  int64_t Combination::encodeConnectivity(int64_t token) {
#ifdef PROFILING
    Profiler::countInvocation("Combination::encodeConnectivity()");
#endif
    assert(height > 1);
    assert(layerSizes[0] >= 2);

    // Reset colors (state):
    for(uint8_t i = 0; i < height; i++) {
      uint8_t s = layerSizes[i];
      for(uint8_t j = 0; j < s; j++)
	colors[i][j] = 0;
    }

    // Run DFS:
    uint8_t s0 = layerSizes[0];
    for(uint8_t i = 0; i < s0-1; i++)
      colorConnected(0, i, i+1);
    if(colors[0][s0-1] == 0)
      colors[0][s0-1] = s0; // No need to run DFS on last

    // Encode:
    for(uint8_t i = 0; i < layerSizes[0]; i++)
      token = 10 * token + colors[0][i];
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

  void Base::normalize() {
#ifdef PROFILING
    Profiler::countInvocation("Base::normalize()");
#endif
    // Ensure FirstBrick is first and all is sorted:
    bool hasVerticalLayer0Brick = false;
    for(uint8_t i = 0; i < layerSize; i++) {
      Brick &b = bricks[i];
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
    else
      rotate90();

    Base c(*this);
    if(canRotate90()) {
      for(int i = 0; i < 3; i++) {
	c.rotate90();
	if(c < *this)
	  copy(c);
      }
    }
    else {
      c.rotate180();
      if(c < *this)
	copy(c);
    }
  }

  int64_t Combination::reverseToken(int64_t token) {
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
      uint8_t size_add = token % 10;
      layerSizes[layers++] = size_add;
      token /= 10;
    }
    // Flip the layer sizes:
    for(uint8_t i = 0; i < layers/2; i++) {
      std::swap(layerSizes[i], layerSizes[layers-i-1]);
    }
  }

  int Combination::countBricksToBridge(const Combination &maxCombination) {
    if(maxCombination.height == 2) {
      return MIN(2, maxCombination.layerSizes[1]); // Only two bricks from second layer can be used.
    }
    else if(maxCombination.height == 3) {
      uint8_t L2 = maxCombination.layerSizes[1];
      uint8_t L3 = maxCombination.layerSizes[2];
      assert(L2 >= 2);
      assert(L3 > 0);

      // Consider <242>: The two L3 bricks can bridge all from L3:
      uint8_t usefulL2 = MIN(L2, L3+2);
      // Each L2 bricks can each support an L3 brick:
      uint8_t usefulL3 = MIN(L2, L3);

      return usefulL2 + usefulL3;
    }
    else {
      // TODO: Improved analysis for other heights
      // For now we assume all bricks above base layer can be used:
      return maxCombination.size - maxCombination.layerSizes[0];
    }
  }

  bool Combination::checkCounts(uint64_t token, const Counts &c) {
    CountsMap m;
    m[11] = Counts(24, 2, 0);
    m[121] = Counts(37081, (32), 0);
    m[21] = Counts(250, (20), 0);
    m[22] = Counts(10411, (49), 0);
    m[221] = Counts(1297413, (787), 0);
    m[222] = Counts(43183164, (3305), 0);
    m[321] = Counts(17111962, (671), 0);
    m[322] = Counts(561114147, (17838), 0);
    m[323] = Counts(7320657167, (14953), 0);
    m[1221] = Counts(157116243, (663), 0);
    m[12221] = Counts(625676928843, (19191), 0);
    m[3221] = Counts(68698089712, (14219), 0);
    m[4221] = Counts(392742794892, (301318), 0);

    m[31] = Counts(648, (8), 0);
    m[131] = Counts(433685, (24), 0);
    m[32] = Counts(148794, (443), 0);
    m[231] = Counts(41019966, (1179), 0);
    m[232] = Counts(3021093957, (46219), 0);
    m[33] = Counts(6246077, (432), 0);
    m[331] = Counts(1358812234, (1104), 0);
    m[332] = Counts(90630537410, (52944), 0);
    m[333] = Counts(2609661915535, (52782), 0);
    m[1321] = Counts(4581373745, (1471), 0);
    m[2321] = Counts(334184934526, (47632), 0);
    m[3321] = Counts(10036269263050, (59722), 0);
    m[12321] = Counts(36790675675026, (39137), 0);

    m[41] = Counts(550, (28), 0);
    m[141] = Counts(2101339, (72), 0);
    m[42] = Counts(849937, (473), 0);
    m[242] = Counts(84806603578, (143406), 0);
    m[241] = Counts(561350899, (15089), 0);

    uint64_t reversed = Combination::reverseToken(token);
    if(m.find(reversed) != m.end())
      token = reversed;
    else if(m.find(token) == m.end()) {
      std::cout << "NEW <" << token << "> " << c << std::endl;
      return true; // No cross check!
    }
    Counts c2 = m[token];
    if(c == c2) {
      std::cout << "OK <" << token << "> " << c << std::endl;
      return true; // All OK
    }
    std::cerr << "CROSS CHECK ERROR!" << std::endl << "EXPECTED" << std::endl;
    std::cerr << " <" << token << "> " << c2 << std::endl;
    std::cerr << "RECEIVED" << std::endl;
    std::cerr << " <" << token << "> " << c << std::endl;
    return false;
  }

  int Base::countUnreachable(const Combination &maxCombination) const {
#ifdef PROFILING
    Profiler::countInvocation("Base::mapUnreachable()");
#endif
    int countUnreachable = 0;

    int bricksBetween = Combination::countBricksToBridge(maxCombination);

    // Notice: Starting at the right side to avoid changing colors
    for(uint8_t i = layerSize-1; i > 0; i--) {
      bool isReachable = false;
      for(uint8_t j = 0; j < i; j++) {
	if(Brick::canReach(bricks[i], bricks[j], bricksBetween)) {
	  isReachable = true;
	  break;
	}
      }
      if(isReachable)
	break;
      countUnreachable++;
    }

    return countUnreachable;
  }

  CombinationBuilder::CombinationBuilder(const Combination &c,
					 const uint8_t waveStart,
					 const uint8_t waveSize,
					 BrickPlane *neighbours,
					 Combination &maxCombination,
					 bool isFirstBuilder,
					 bool encodeConnectivity,
					 bool encodingLocked) : baseCombination(c),
								waveStart(waveStart),
								waveSize(waveSize),
								neighbours(neighbours),
								maxCombination(maxCombination),
								isFirstBuilder(isFirstBuilder),
								encodeConnectivity(encodeConnectivity),
								encodingLocked(encodingLocked) {
#ifdef PROFILING
    Profiler::countInvocation("CombinationBuilder::CombinationBuilder(...)");
#endif
  }

  CombinationBuilder::CombinationBuilder(const Base &c,
					 BrickPlane *neighbours,
					 Combination &maxCombination) : baseCombination(c),
									waveStart(0),
									waveSize(c.layerSize),
									neighbours(neighbours),
									maxCombination(maxCombination),
									isFirstBuilder(false),
									encodeConnectivity(true),
									encodingLocked(false) {
#ifdef PROFILING
    Profiler::countInvocation("CombinationBuilder::CombinationBuilder(Base...)");
#endif
  }

  CombinationBuilder::CombinationBuilder(const CombinationBuilder& b) : baseCombination(b.baseCombination),
									waveStart(b.waveStart),
									waveSize(b.waveSize),
									neighbours(b.neighbours),
									maxCombination(b.maxCombination),
									isFirstBuilder(b.isFirstBuilder),
									encodeConnectivity(b.encodeConnectivity),
									encodingLocked(b.encodingLocked) {
#ifdef PROFILING
    Profiler::countInvocation("CombinationBuilder::CombinationBuilder(CombinationBuilder&)");
#endif
  }

  CombinationBuilder::CombinationBuilder() : waveStart(0),
					     waveSize(0),
					     neighbours(NULL),
					     isFirstBuilder(false),
					     encodeConnectivity(false),
					     encodingLocked(false) {
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
	if(layer2 >= maxCombination.height)
	  continue; // Out of range
	if(layer2 < baseCombination.height && baseCombination.layerSizes[layer2] == maxCombination.layerSizes[layer2])
	  continue; // Already at maximum allowed for layer

	// Add crossing bricks (one vertical, one horizontal):
	for(int16_t x = -2; x < 3; x++) {
	  for(int16_t y = -2; y < 3; y++) {
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
	int16_t w = 4, h = 2;
	if(brick.isVertical) {
	  w = 2;
	  h = 4;
	}
	for(int16_t y = -h+1; y < h; y++) {
	  for(int16_t x = -w+1; x < w; x++) {
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
	  } // for x
	} // for y
      } // for layer2
    }

    // Cleanup, so that neighbours can be shared by all:
    for(std::vector<LayerBrick>::const_iterator it = v.begin(); it != v.end(); it++)
      neighbours[it->LAYER].unset(it->BRICK);
  }

  /*
    Check if model cannot be made symmetric when placing remaining bricks.
  */
  bool CombinationBuilder::nextCombinationCanBeSymmetric180() {
#ifdef PROFILING
    Profiler::countInvocation("CombinationBuilder::nextCombinationCanBeSymmetric180()");
#endif
    bool canBeSymmetric180 = true;

    bool hasFullLayers = false; // Layers where all bricks are already placed: If they are non-symmetric or have misalignment of centers, then the resulting models cannot be symmetric
    int16_t cx0, cy0, cx1, cy1;

    for(uint8_t i = 0; i < baseCombination.height; i++) {
      int8_t diff = maxCombination.layerSizes[i] - (int8_t)baseCombination.layerSizes[i];
      if(diff == 0) {
	// Full layer: Check if can be symmetric:
	if(!hasFullLayers) {
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
	hasFullLayers = true; // Because diff is 0
      }
    }
    return canBeSymmetric180;
  }

  void CombinationBuilder::placeAllLeftToPlace(const uint8_t &leftToPlace, const bool &canBeSymmetric180, const std::vector<LayerBrick> &v) {
#ifdef PROFILING
    Profiler::countInvocation("CombinationBuilder::placeAllLeftToPlace()");
#endif
#ifdef TRACE
    std::cout << "   Placing " << (int)leftToPlace << " bricks onto " << baseCombination << std::endl;
#endif
    // Optimization: Check if all layers can be filled:
    // "non-full" layers: Layers that are not filled by v:
    int cntNonFullLayers = 0;
    for(uint8_t i = 0; i < maxCombination.height; i++) {
      if(i >= baseCombination.height || maxCombination.layerSizes[i] > baseCombination.layerSizes[i])
	cntNonFullLayers++;
    }
    bool checkedLayers[MAX_HEIGHT];
    for(uint8_t i = 0; i < maxCombination.height; i++)
      checkedLayers[i] = false;
    for(std::vector<LayerBrick>::const_iterator it = v.begin(); it != v.end(); it++) {
      const LayerBrick &lb = *it;
      uint8_t layer = lb.LAYER;
      if(checkedLayers[layer])
	continue;
      if(layer >= baseCombination.height || maxCombination.layerSizes[layer] > baseCombination.layerSizes[layer]) {
	checkedLayers[layer] = true;
	cntNonFullLayers--;
      }
    }
    if(cntNonFullLayers > 0)
      return; // Can't possibly fill!
    // End of optimization

    BrickPicker picker(v, 0, leftToPlace);
    int64_t token = -1, prevToken = -1;
    CountsMap::iterator it;
    while(picker.next(baseCombination, maxCombination)) {
      if(!encodingLocked || token == -1) {
	token = baseCombination.getTokenFromLayerSizes();
	if(encodeConnectivity)
	  token = baseCombination.encodeConnectivity(token);

	if(token != prevToken) {
	  it = counts.find(token);
	  if(it == counts.end()) {
	    std::pair<CountsMap::iterator,bool> pp = counts.insert(std::pair<int64_t,Counts>(token, Counts()));
	    assert(pp.second);
	    it = pp.first;
	  }
	  prevToken = token;
	}
      }

      it->second.all++;
      if(canBeSymmetric180 && baseCombination.is180Symmetric()) {
	it->second.symmetric180++;
	if(baseCombination.is90Symmetric())
	  it->second.symmetric90++;
      }

#ifdef DEBUG
      if(maxCombination.height == 3) {
	Counts cx(1,0,0);
	if(canBeSymmetric180 && baseCombination.is180Symmetric()) {
	  cx.symmetric180++;
	  if(baseCombination.is90Symmetric())
	    cx.symmetric90++;
	}
	// Report on bases from middle layer:
	Base base; base.layerSize = baseCombination.layerSizes[1];
	for(uint8_t i = 0; i < base.layerSize; i++)
	  base.bricks[i] = baseCombination.bricks[1][i];
	base.normalize();
	CombinationCountsMap::iterator bit = baseCounts.find(base);
	if(bit != baseCounts.end())
	  bit->second += cx;
	else
	  baseCounts[base] = cx;
      }
#endif

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

      if(doubleCount)
	toAdd += toAdd; // Since we skipped "the other time" (optimization 1)

      CountsMap::iterator it2;
      if((it2 = counts.find(token)) == counts.end())
	counts[token] = toAdd;
      else
	it2->second += toAdd;
    }

#ifdef DEBUG
    for(CombinationCountsMap::const_iterator bit = b.baseCounts.begin(); bit != b.baseCounts.end(); bit++) {
      CombinationCountsMap::iterator bit2 = baseCounts.find(bit->first);
      Counts toAdd = bit->second;

      if(doubleCount)
	toAdd += toAdd; // Since we skipped "the other time" (optimization 1)

      if(bit2 != baseCounts.end())
	bit2->second += toAdd;
      else
	baseCounts[bit->first] = toAdd;
    }
#endif
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
    std::vector<LayerBrick> v;
    findPotentialBricksForNextWave(v);

    const uint8_t leftToPlace = maxCombination.size - baseCombination.size;
    const bool canBeSymmetric180 = nextCombinationCanBeSymmetric180();
    placeAllLeftToPlace(leftToPlace, canBeSymmetric180, v);

    int layersTodo = 0;
    for(uint8_t i = 0; i < maxCombination.height; i++) {
      if(i >= baseCombination.height || maxCombination.layerSizes[i] > baseCombination.layerSizes[i]) {
	layersTodo++;
	if(layersTodo > 1)
	  break;
      }
    }
    if(layersTodo <= 1) {
#ifdef TRACE
      std::cout << "   Early exit of build due to only " << layersTodo << " layers not done" << std::endl;
#endif
      return; // All done in placeAllLeftToPlace()
    }

    for(uint8_t toPick = 1; toPick < leftToPlace; toPick++) {
      // Pick toPick from neighbours:
      BrickPicker picker(v, 0, toPick);

      while(picker.next(baseCombination, maxCombination)) {
	// Optimization: Skip half of constructions in first builder (unless symmetric)
	// Is not enabled for precomputations, as base order changes on symmetries.
	bool doubleCount = isFirstBuilder && !baseCombination.is180Symmetric(); // TODO RM false!
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

	// Encoding can only take on a single value if bricks being picked belong to same base bricks.
	// TODO: Check encoding
	bool nextEncodingLocked = encodingLocked || toPick == 1;
	CombinationBuilder builder(baseCombination, waveStart+waveSize, toPick, neighbours, maxCombination, false, encodeConnectivity, nextEncodingLocked);
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
#ifdef PROFILING
    Profiler::countInvocation("ThreadEnablingBuilder::ThreadEnablingBuilder(copy)");
#endif
  }

  ThreadEnablingBuilder::ThreadEnablingBuilder(Combination &c,
 					       const uint8_t waveStart,
					       BrickPlane *neighbours,
					       Combination &maxCombination,
 					       MultiBatchSizeBrickPicker *picker,
					       int threadIndex,
					       bool encodeConnectivity) : picker(picker) {
#ifdef PROFILING
    Profiler::countInvocation("ThreadEnablingBuilder::ThreadEnablingBuilder(...)");
#endif

    b = CombinationBuilder(c, waveStart, 0, neighbours, maxCombination, true, encodeConnectivity, false);
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

  bool CombinationBuilder::addFromPicker(MultiBatchSizeBrickPicker *p, int &picked, const std::string &threadName) {
#ifdef PROFILING
    Profiler::countInvocation("ThreadEnablingBuilder::addFromPicker()");
#endif
    bool ret = p->next(baseCombination, picked, maxCombination);
    if(ret) {
      if(maxCombination.size - baseCombination.size > 3)
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
    for(int i = 0; i < toRemove; i++)
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

  void CombinationBuilder::buildSplit(int threadCount) {
#ifdef PROFILING
    Profiler::countInvocation("ThreadEnablingBuilder::buildSplit()");
#endif
    std::vector<LayerBrick> v;
    findPotentialBricksForNextWave(v);

    const uint16_t leftToPlace = maxCombination.size - baseCombination.size;
    const bool canBeSymmetric180 = nextCombinationCanBeSymmetric180();
    placeAllLeftToPlace(leftToPlace, canBeSymmetric180, v);
    if(leftToPlace <= 1)
      return;

    LayerBrick bricks[MAX_BRICKS];

    int processorCount = MAX(2, threadCount-1);//std::thread::hardware_concurrency();
    std::cout << "Using " << processorCount << " builder threads" << std::endl;

    MultiBatchSizeBrickPicker picker(v, leftToPlace-1); // Shared picker
    BrickPlane *neighbourCache = new BrickPlane[processorCount * MAX_HEIGHT];
    for(int i = 0; i < processorCount * MAX_HEIGHT; i++)
      neighbourCache[i].unsetAll();
    ThreadEnablingBuilder *threadBuilders = new ThreadEnablingBuilder[processorCount];
    std::thread **threads = new std::thread*[processorCount];

    for(int i = 0; i < processorCount; i++) {
      threadBuilders[i] = ThreadEnablingBuilder(baseCombination, waveStart+waveSize, &neighbourCache[i*MAX_HEIGHT], maxCombination, &picker, i, encodeConnectivity);
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
    uint8_t layerSizes[MAX_HEIGHT];
    for(CountsMap::const_iterator it = counts.begin(); it != counts.end(); it++) {
      int64_t token = it->first;
      Combination::getLayerSizesFromToken(token, layerSizes);
      Counts countsForToken(it->second);

      countsForToken.symmetric180 += countsForToken.symmetric90;
      countsForToken.all += countsForToken.symmetric90;
      countsForToken.all += countsForToken.symmetric180;

      countsForToken.all /= 2 * layerSizes[0]; // Because each model is built toward two directions
      countsForToken.symmetric180 /= layerSizes[0];
      countsForToken.symmetric90 /= layerSizes[0] / 2;
      //std::cout << " <" << token << "> " << countsForToken << std::endl;
      Combination::checkCounts(token, countsForToken);
    }
  }

  BitWriter::BitWriter() : ostream(NULL), base(0), bits(0), cntBits(0), sumTotal(0), sumSymmetric180(0), sumSymmetric90(0), lines(0) {
#ifdef PROFILING
    Profiler::countInvocation("BitWriter::BitWriter()");
#endif
  }
  BitWriter::BitWriter(const BitWriter &w) : ostream(w.ostream), base(w.base), bits(w.bits), cntBits(w.cntBits), sumTotal(w.sumTotal), sumSymmetric180(w.sumSymmetric180), sumSymmetric90(w.sumSymmetric90), lines(w.lines) {
#ifdef PROFILING
    Profiler::countInvocation("BitWriter::BitWriter(copy)");
#endif
  }
  BitWriter::BitWriter(const std::string &fileName, uint8_t base) : base(base), bits(0), cntBits(0), sumTotal(0), sumSymmetric180(0), sumSymmetric90(0), lines(0)  {
#ifdef PROFILING
    Profiler::countInvocation("BitWriter::BitWriter(...)");
#endif
    ostream = new std::ofstream(fileName.c_str(), std::ios::binary);
  }
  BitWriter::~BitWriter() {
#ifdef PROFILING
    Profiler::countInvocation("BitWriter::~BitWriter()");
#endif
    // End indicator:
    writeBit(1);
    writeBit(0); // baseSymmetric180
    if((base & 3) == 0)
      writeBit(0); // baseSymmetric90
    for(int i = 1; i < base; i++)
      writeBrick(FirstBrick);
    for(uint8_t i = 0; i < base-1; i++)
      writeColor(0);
    writeUInt32(0); // total
    writeUInt16(0); // symmetric180
    if((base & 3) == 0)
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
#ifdef PROFILING
    Profiler::countInvocation("BitWriter::writeColor()");
#endif
    assert(toWrite < 8);
    for(int j = 0; j < 3; j++) {
      writeBit(toWrite & 1);
      toWrite >>= 1;
    }
    assert(toWrite == 0);
  }
  void BitWriter::writeBrick(const Brick &b) {
#ifdef PROFILING
    Profiler::countInvocation("BitWriter::writeBrick()");
#endif
    writeBit(b.isVertical);
    writeUInt16(b.x);
    writeUInt16(b.y);
  }
  void BitWriter::writeBit(bool bit) {
#ifdef PROFILING
    Profiler::countInvocation("BitWriter::writeBit()");
#endif
    bits = (bits << 1) + (bit ? 1 : 0);
    cntBits++;
    if(cntBits == 8) {
      ostream->write((char*)&bits, 1);
      cntBits = 0;
    }
  }
  void BitWriter::writeCounts(const Counts &c) {
#ifdef PROFILING
    Profiler::countInvocation("BitWriter::writeCounts()");
#endif
    if(c.all >= 4294967295 || c.symmetric180 >= 65535 || c.symmetric90 >= 255) {
      std::cerr << "Counts too large! " << c << std::endl;
      assert(false); // Graceful
      int *a = NULL; a[10]=10; // Not so graceful
    }

    writeUInt32(c.all);
    sumTotal += c.all;

    writeUInt16(c.symmetric180);
    sumSymmetric180 += c.symmetric180;

    if((base & 3) == 0) {
      writeUInt8(c.symmetric90);
      sumSymmetric90 += c.symmetric90;
    }

    lines++;
  }
  void BitWriter::flushBits() {
#ifdef PROFILING
    Profiler::countInvocation("BitWriter::flushBits()");
#endif
    while(cntBits > 0) {
      writeBit(0);
    }
  }
  void BitWriter::writeUInt8(uint8_t toWrite) {
#ifdef PROFILING
    Profiler::countInvocation("BitWriter::writeUInt8()");
#endif
    for(int j = 0; j < 8; j++) {
      writeBit(toWrite & 1);
      toWrite >>= 1;
    }
  }
  void BitWriter::writeUInt16(uint16_t toWrite) {
#ifdef PROFILING
    Profiler::countInvocation("BitWriter::writeUInt16()");
#endif
    for(int j = 0; j < 16; j++) {
      writeBit(toWrite & 1);
      toWrite >>= 1;
    }
  }
  void BitWriter::writeUInt32(uint32_t toWrite) {
#ifdef PROFILING
    Profiler::countInvocation("BitWriter::writeUInt32()");
#endif
    for(int j = 0; j < 32; j++) {
      writeBit(toWrite & 1);
      toWrite >>= 1;
    }
  }
  void BitWriter::writeUInt64(uint64_t toWrite) {
#ifdef PROFILING
    Profiler::countInvocation("BitWriter::writeUInt64()");
#endif
    for(int j = 0; j < 64; j++) {
      writeBit(toWrite & 1);
      toWrite >>= 1;
    }
  }

  std::ostream& operator << (std::ostream &os,const Report &r) {
    os << "Base ";
#ifdef DEBUG
    os << r.c << " ";
#else
    os << (int)r.base;
#endif
    os << " colors 1";
    for(uint8_t i = 0; i < r.base-1; i++)
      os << " " << (int)(1+r.colors[i]);
    if(r.baseSymmetric90)
      os << " [90]";
    else if(r.baseSymmetric180)
      os << " [180]";
    os << ": " << r.counts;
    return os;
  }

  bool Report::connected(const Report &a, const Report &b) {
    int ret = 1;
    bool c[MAX_LAYER_SIZE];
    for(uint8_t i = 0; i < a.base-1; i++) {
      bool isColor1 = a.colors[i] == 0 || b.colors[i] == 0;
      c[i] = isColor1;
      if(isColor1)
	ret++;
    }
    bool improved = true;
    while(improved) {
      improved = false;
      for(uint8_t i = 0; i < a.base-1; i++) {
	if(c[i])
	  continue;
	// Attempt to color i:
	for(uint8_t j = 0; j < a.base-1; j++) {
	  if(c[j] && (a.colors[i] == a.colors[j] || b.colors[i] == b.colors[j])) {
	    c[i] = true;
	    ret++;
	    improved = true;
	    break;
	  }
	}
      }
    }
    assert(ret <= a.base);
    return ret == a.base;
  }

  Counts Report::countUp(const Report &reportA, const Report &reportB) {
    if(!Report::connected(reportA, reportB))
      return Counts();
    Counts ret;
    // a and b are raw counts from building on base, so non-symmetric are double-counted!
    const Counts &a = reportA.counts;
    const Counts &b = reportB.counts;

    if(reportA.baseSymmetric180) {
      Counts A = a; A.all-=A.symmetric180; A.symmetric180-=A.symmetric90;
      Counts B = b; B.all-=B.symmetric180; B.symmetric180-=B.symmetric90;
      if(reportA.baseSymmetric90) {
	assert(reportB.baseSymmetric90);
	return Counts(a.all * B.all +          // All a x Non-symmetric B
		      A.all * B.symmetric180 + // Non-symmetric a x symmetric180 B
		      A.all * B.symmetric90,   // Non-symmetric a x symmetric90 B
		      a.symmetric180 * b.symmetric180 -
		      A.symmetric90 * B.symmetric90,
		      A.symmetric90 * B.symmetric90);
      }
      else {
	assert(reportB.baseSymmetric180);
	assert(a.symmetric90 == 0);
	assert(b.symmetric90 == 0);
	return Counts(a.all * B.all +         // All a x Non-symmetric B
		      A.all * B.symmetric180, // Non-symmetric a x symmetric B
		      A.symmetric180 * B.symmetric180,
		      0);
      }
    }
    else {
      assert(a.symmetric180 == 0);
      assert(b.symmetric180 == 0);
      assert(a.symmetric90 == 0);
      assert(b.symmetric90 == 0);
      return Counts(a.all * b.all, 0, 0);
    }
  }

  void Report::getReports(const CountsMap &cm, std::vector<Report> &reports, uint8_t base, bool b180, bool b90) {
    for(CountsMap::const_iterator it = cm.begin(); it != cm.end(); it++) {
      Report r;
      r.base = base;
      r.baseSymmetric180 = b180;
      r.baseSymmetric90 = b90;
      r.counts = it->second;
      uint64_t token = it->first;
      for(uint8_t i = 0; i < base-1; i++) {
	r.colors[base-2-i] = token % 10 - 1;
	token/=10;
      }
      reports.push_back(r);
    }
  }
  
  bool BitReader::readBit() {
#ifdef PROFILING
    Profiler::countInvocation("BitReader::readBit()");
#endif
    if(bitIdx == 8) {
      istream->read((char*)&bits, 1);
      bitIdx = 0;
    }
    bool bit = (bits >> (7-bitIdx)) & 1;
    bitIdx++;
    return bit;
  }
  uint8_t BitReader::readColor() {
#ifdef PROFILING
    Profiler::countInvocation("BitReader::readColor()");
#endif
    uint8_t ret = 0;
    for(int i = 0; i < 3; i++) {
      ret = ret | (uint8_t)(readBit() << i);
    }
    return ret;
  }
  void BitReader::readBrick(Brick &b) {
    b.isVertical = readBit();
    b.x = readUInt16();
    b.y = readUInt16();
  }
  uint8_t BitReader::readUInt8() {
#ifdef PROFILING
    Profiler::countInvocation("BitReader::readUint8()");
#endif
    uint8_t ret = 0;
    for(int i = 0; i < 8; i++) {
      ret = ret | (uint8_t)(readBit() << i);
    }
    return ret;
  }
  uint16_t BitReader::readUInt16() {
#ifdef PROFILING
    Profiler::countInvocation("BitReader::readUint16()");
#endif
    uint16_t ret = 0;
    for(int i = 0; i < 16; i++) {
      ret = ret | (uint16_t)(readBit() << i);
    }
    return ret;
  }
  uint32_t BitReader::readUInt32() {
#ifdef PROFILING
    Profiler::countInvocation("BitReader::readUint32()");
#endif
    uint32_t ret = 0;
    for(int i = 0; i < 32; i++) {
      ret = ret | ((uint32_t)readBit() << i);
    }
    return ret;
  }
  uint64_t BitReader::readUInt64() {
#ifdef PROFILING
    Profiler::countInvocation("BitReader::readUint64()");
#endif
    uint64_t ret = 0;
    for(int i = 0; i < 64; i++) {
      ret = ret | ((uint64_t)readBit() << i);
    }
    return ret;
  }
  void BitReader::readCounts(Counts &c) {
#ifdef PROFILING
    Profiler::countInvocation("BitReader::readCounts()");
#endif
    c.all = readUInt32();
    c.symmetric180 = readUInt16();
    c.symmetric90 = ((base & 3) == 0) ? readUInt8() : 0;
  }
  BitReader::BitReader(uint8_t base, int n, int token, int D) : bits(0), bitIdx(8), base(base), sumTotal(0), sumSymmetric180(0), sumSymmetric90(0), lines(0) {
#ifdef PROFILING
    Profiler::countInvocation("BitReader::BitReader()");
#endif
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
#ifdef PROFILING
    Profiler::countInvocation("BitReader::~BitReader()");
#endif
    if(istream != NULL) {
      istream->close();
      delete istream;
    }
  }
  bool BitReader::next(std::vector<Report> &v) {
#ifdef PROFILING
    Profiler::countInvocation("BitReader::next()");
#endif
    Report r;
    r.base = base;
    r.baseSymmetric180 = readBit();
    r.baseSymmetric90 = ((base & 3) == 0) && readBit();
    r.c.bricks[0] = FirstBrick;
    for(uint8_t i = 1; i < base; i++)
      readBrick(r.c.bricks[i]);
    r.c.layerSize = base;
    bool first = true;
    while(true) {
      if(!first) {
	bool indicator = readBit();
	if(indicator)
	  return true; // Now at next batch
      }
      first = false;
      for(uint8_t i = 0; i < base-1; i++)
	r.colors[i] = readColor();
      r.counts.all = readUInt32();
      sumTotal += r.counts.all;
      r.counts.symmetric180 = readUInt16();
      sumSymmetric180 += r.counts.symmetric180;
      r.counts.symmetric90 = 0;
      if((base & 3) == 0)
	r.counts.symmetric90 = readUInt8();
      sumSymmetric90 += r.counts.symmetric90;
      if(r.counts.all == 0) {
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
      //std::cout << "Read report " << r << std::endl;
      v.push_back(r);
    }
  }

  BaseBuilder::BaseBuilder(const std::vector<int> distances, BitWriter &writer) : distances(distances), writer(writer), reachSkips(0), mirrorSkips(0), noSkips(0) {
#ifdef PROFILING
    Profiler::countInvocation("BaseBuilder::BaseBuilder()");
#endif
    int size = (int)distances.size();
    if(size == 1)
      innerBuilder = new Size1InnerBaseBuilder(distances[0]);
    else
      innerBuilder = new InnerBaseBuilder(size-1, distances); // size - 1 to indicate last idx
  }

  BaseBuilder::~BaseBuilder() {
#ifdef PROFILING
    Profiler::countInvocation("BaseBuilder::~BaseBuilder()");
#endif
    delete innerBuilder;
  }

  bool BaseBuilder::checkMirrorSymmetries(const Base &c) {
#ifdef PROFILING
    Profiler::countInvocation("BaseBuilder::checkMirrorSymmetries()");
#endif
    Base mx(c);
    mx.mirrorX();
    if(resultsMap.find(mx) != resultsMap.end()) {
      duplicates[c] = mx;
      return true;
    }
    CombinationMap::iterator it = duplicates.find(mx);
    if(it != duplicates.end()) {
      duplicates[c] = it->second;
      return true;
    }

    Base my(c);
    my.mirrorY();
    if(resultsMap.find(my) != resultsMap.end()) {
      duplicates[c] = my;
      return true;
    }
    it = duplicates.find(my);
    if(it != duplicates.end()) {
      duplicates[c] = it->second;
      return true;
    }

    // We do not mirror in both X and Y, as that is equal to 180 degree rotation.
    return false;
  }

  Size1InnerBaseBuilder::Size1InnerBaseBuilder(int16_t D) : encoded(0), d(0), D(D) {
#ifdef PROFILING
    Profiler::countInvocation("Size1InnerBaseBuilder::Size1InnerBaseBuilder()");
#endif
    assert(D > 0);
  }

  bool Size1InnerBaseBuilder::nextBase(Base &c) {
#ifdef PROFILING
    Profiler::countInvocation("Size1InnerBaseBuilder::nextBase()");
#endif
    assert(c.bricks[0] == FirstBrick);
    while(true) {
      if(encoded == 8 && d == D)
	return false;
      if(encoded == 8) {
	d++;
	encoded = 0;
      }
      bool isVertical = (encoded & 1) == 1;
      int16_t multX = (encoded & 2) == 2 ? -1 : 1;
      int16_t multY = (encoded & 4) == 4 ? -1 : 1;
      b = Brick(isVertical, PLANE_MID + multX * (D-d), PLANE_MID + multY * d);
      encoded++;
      if(!FirstBrick.intersects(b)) {
	resetCombination(c);
	return true;
      }
    }
  }

  void Size1InnerBaseBuilder::resetCombination(Base &c) {
    c.bricks[1] = b;
  }

  InnerBaseBuilder::InnerBaseBuilder(int16_t idx, const std::vector<int> &distances) :
    idx(idx), encoded(8), d(distances[idx]), D(distances[idx]) {
#ifdef PROFILING
    Profiler::countInvocation("InnerBaseBuilder::InnerBaseBuilder()");
#endif
    assert(idx >= 1);
    if(idx == 1)
      inner = new Size1InnerBaseBuilder(distances[0]);
    else
      inner = new InnerBaseBuilder(idx-1, distances);
  }

  InnerBaseBuilder::~InnerBaseBuilder() {
#ifdef PROFILING
    Profiler::countInvocation("InnerBaseBuilder::~InnerBaseBuilder()");
#endif
    delete inner;
  }

  bool InnerBaseBuilder::nextBase(Base &c) {
#ifdef PROFILING
    Profiler::countInvocation("InnerBaseBuilder::nextBase()");
#endif
    while(true) {
      if(encoded == 8 && d == D) {
	bool ret = inner->nextBase(c);
	if(!ret)
	  return false;
	encoded = 0;
	d = 0;
      }
      else if(encoded == 8) {
	d++;
	encoded = 0;
      }
      bool isVertical = (encoded & 1) == 1;
      int16_t multX = (encoded & 2) == 2 ? -1 : 1;
      int16_t multY = (encoded & 4) == 4 ? -1 : 1;
      b = Brick(isVertical, PLANE_MID + multX * (D-d), PLANE_MID + multY * d);
      inner->resetCombination(c); // Ensure bricks to compare to
      encoded++;
      bool ok = true;
      for(int16_t i = 0; i <= idx; i++) {
	if(c.bricks[i].intersects(b)) {
	  ok = false;
	  break;
	}
      }
      if(ok) {
	resetCombination(c);
	return true;
      }
    }
  }

  void InnerBaseBuilder::resetCombination(Base &c) {
    inner->resetCombination(c);
    c.bricks[idx+1] = b;
  }

  bool BaseBuilder::nextBaseToBuildOn(Base &outBase, const Combination &maxCombination) {
    std::lock_guard<std::mutex> guard(mutex);
#ifdef PROFILING
    Profiler::countInvocation("BaseBuilder::nextBaseToBuildOn()");
#endif
    uint8_t base = (uint8_t)distances.size() + 1;
    Base c; c.layerSize = base;
    while(true) {
      if(!innerBuilder->nextBase(c))
	return false;
      c.normalize();

      // Check if already seen:
      if(resultsMap.find(c) != resultsMap.end())
	continue; // Already seen!
      if(duplicates.find(c) != duplicates.end())
	continue; // Already seen as duplicate!

      // Check that baseCombination does not belong to another time:
      bool belongsToAnotherTime = false;
      std::vector<int> baseCombinationDistances;
      for(uint8_t i = 1; i < base; i++) {
	int dist = c.bricks[i].dist(FirstBrick);
	baseCombinationDistances.push_back(dist);
      }
      std::sort(baseCombinationDistances.begin(), baseCombinationDistances.end());
      for(uint8_t i = 0; i < base-1; i++) {
	if(baseCombinationDistances[i] != distances[i]) {
	  belongsToAnotherTime = true;
	  break;
	}
      }
      if(belongsToAnotherTime)
	continue; // Not an actual skip

      // Check for mirror symemtries:
      if(checkMirrorSymmetries(c)) {
	//std::cout << "  Mirror symmetric base " << c << std::endl;
	bases.push_back(c);
	if(++mirrorSkips % 10000 == 0)
	  std::cout << "Skips: reach " << (reachSkips/1000) << " k, MIRROR " << (mirrorSkips/1000) << " k, none " << (noSkips/1000) << " k" << std::endl;
	continue; // duplicates handled in checkMirrorSymmetries
      }

      // Check for smaller bases:
      if(!c.is180Symmetric()) { // Do not do this for symmetric bases, as counts rely on symmetry
	int unreachableInBaseCount = c.countUnreachable(maxCombination);
	if(unreachableInBaseCount > 0) {
	  Base smallerBase(c);
	  smallerBase.layerSize = base - unreachableInBaseCount;

	  CombinationMap::iterator it = duplicates.find(smallerBase);
	  if(it != duplicates.end()) { // Known smaller base: Point to same original:
	    duplicates[c] = it->second;
	    //std::cout << "  Smaller base skip " << c << std::endl;
	    bases.push_back(c);
	    if(++reachSkips % 50000 == 0)
	      std::cout << "Skips: REACH " << (reachSkips/1000) << " k, mirror " << (mirrorSkips/1000) << " k, none " << (noSkips/1000) << " k" << std::endl;
	    continue;
	  }
	  else // First time the smaller base is encountered: Mark it
	    duplicates[smallerBase] = c;
	}
      }

      resultsMap[c] = CountsMap(); // Reserve the entry so that check for "seen" above works.
      //std::cout << "  Normal base " << c << std::endl;
      bases.push_back(c);
      if(++noSkips % 10000 == 0)
	std::cout << "Skips: reach " << (reachSkips/1000) << " k, mirror " << (mirrorSkips/1000) << " k, NONE " << (noSkips/1000) << " k" << std::endl;
      outBase = c;
      return true;
    }
  }

  void BaseBuilder::registerCounts(Base &base, CountsMap counts) {
    std::lock_guard<std::mutex> guard(mutex);
#ifdef PROFILING
    Profiler::countInvocation("BaseBuilder::registerCounts()");
#endif
    resultsMap[base] = counts;
  }

  void BaseBuilder::report() {
#ifdef PROFILING
    Profiler::countInvocation("BaseBuilder::report()");
#endif
    int base = 1 + (int)distances.size();
    int colors[MAX_LAYER_SIZE];
    for(std::vector<Base>::const_iterator it = bases.begin(); it != bases.end(); it++) {
      Base c = *it;
      CombinationMap::const_iterator it2 = duplicates.find(c);
      if(it2 != duplicates.end())
	c = it2->second; // Use original instead
      CountsMap cm = resultsMap[c];

      // Write results:
      bool baseSymmetric180 = c.is180Symmetric();
      bool baseSymmetric90 = baseSymmetric180 && c.is90Symmetric();
      writer.writeBit(1); // New batch
      writer.writeBit(baseSymmetric180);
      if((base & 3) == 0)
	writer.writeBit(baseSymmetric90);
      for(int i = 1; i < base; i++)
	writer.writeBrick(it->bricks[i]);

      bool any = false;
      for(CountsMap::const_iterator it3 = cm.begin(); it3 != cm.end(); it3++) {
	if(any)
	  writer.writeBit(0); // Indicate we are still in same batch
	any = true;
	int64_t token = it3->first;
	for(int i = 0; i < base; i++) {
	  colors[base-1-i] = token % 10;
	  token /= 10;
	}
	for(int i = 1; i < base; i++)
	  writer.writeColor(colors[i] - 1);
	writer.writeCounts(it3->second);
      }
    }
  }

  Lemma3Runner::Lemma3Runner() : baseBuilder(NULL),
				 maxCombination(NULL),
				 neighbours(NULL),
				 threadName("") {
#ifdef PROFILING
    Profiler::countInvocation("Lemma3Runner::Lemma3Runner()");
#endif
  }
  Lemma3Runner::Lemma3Runner(const Lemma3Runner &b) : baseBuilder(b.baseBuilder),
						      maxCombination(b.maxCombination),
						      neighbours(b.neighbours),
						      threadName(b.threadName) {
#ifdef PROFILING
    Profiler::countInvocation("Lemma3Runner::Lemma3Runner(copy)");
#endif
  }
  Lemma3Runner::Lemma3Runner(BaseBuilder *b,
			     Combination *maxCombination,
			     int threadIndex,
			     BrickPlane *neighbours) : baseBuilder(b),
						       maxCombination(maxCombination),
						       neighbours(neighbours) {
#ifdef PROFILING
    Profiler::countInvocation("Lemma3Runner::Lemma3Runner(...)");
#endif
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

#ifdef DEBUG
  void Lemma3Runner::run(CombinationCountsMap &computed1XY) {
#else
  void Lemma3Runner::run() {
#endif
#ifdef PROFILING
    Profiler::countInvocation("Lemma3Runner::run()");
#endif
    Base c;
    while(baseBuilder->nextBaseToBuildOn(c, *maxCombination)) {
      if(maxCombination->size - c.layerSize > 3)
	std::cout << threadName << " builds on " << c << std::endl;
      CombinationBuilder builder(c, neighbours, *maxCombination);
      builder.build();
      baseBuilder->registerCounts(c, builder.counts);
    } // while baseBuilder->nextBaseToBuildOn

#ifdef DEBUG
    for(std::vector<Base>::const_iterator it = baseBuilder->bases.begin(); it != baseBuilder->bases.end(); it++) {
      c = *it;
      Base original(c);
      CombinationBuilder builder(c, neighbours, *maxCombination);
      CombinationMap::const_iterator countsIt = baseBuilder->duplicates.find(c);
      if(countsIt != baseBuilder->duplicates.end()) {
	original = countsIt->second;
      }
      builder.counts = baseBuilder->resultsMap[original];

      uint8_t base = c.layerSize;
      Combination maxX1; maxX1.size = base+1; maxX1.height = 2; maxX1.layerSizes[0] = base; maxX1.layerSizes[1] = 1;
      CombinationBuilder builderX1(original, neighbours, maxX1);

      // Build all <1X2> combinations with this base:
      std::vector<LayerBrick> potentialBricksX1, potentialBricksX2;
      builder.findPotentialBricksForNextWave(potentialBricksX2);
      builderX1.findPotentialBricksForNextWave(potentialBricksX1);

      // Compare counts:
      builderX1.build();
      std::vector<Report> rX2, rX1;
      Counts countsMult;
      bool b180 = c.is180Symmetric();
      bool b90 = b180 && c.is90Symmetric();
      Report::getReports(builder.counts, rX2, base, b180, b90);
      Report::getReports(builderX1.counts, rX1, base, b180, b90);
      for(std::vector<Report>::const_iterator it1 = rX2.begin(); it1 != rX2.end(); it1++) {
	for(std::vector<Report>::const_iterator it2 = rX1.begin(); it2 != rX1.end(); it2++) {
	  Counts c2 = Report::countUp(*it1, *it2);
	  countsMult += c2;
	}
      }
      if(b90) {
	assert(countsMult.symmetric90 % 4 == 0);
	countsMult.symmetric90 /= 4;
	assert(countsMult.symmetric180 % 2 == 0);
	countsMult.symmetric180 /= 2;
	assert(countsMult.all % 4 == 0);
	countsMult.all = countsMult.all/4 + countsMult.symmetric180 + countsMult.symmetric90;
      }
      else if(b180) {
	assert(countsMult.symmetric90 == 0);
	assert(countsMult.all % 2 == 0);
	countsMult.all = countsMult.all/2 + countsMult.symmetric180;
      }

      CombinationCountsMap::iterator bit = computed1XY.find(c);
      if(bit == computed1XY.end()) {
	if(countsMult.all == 0)
	  continue; // All OK
	std::cerr << "Unknown base! " << c << std::endl;
	std::cerr << "Mults:   " << countsMult << std::endl;
	std::cerr << "CountsMap <" << (int)base << "Y>: " << builder.counts.size() << std::endl;
	for(CountsMap::const_iterator it = builder.counts.begin(); it != builder.counts.end(); it++)
	  std::cerr << " " << it->first << ": " << it->second << std::endl;
	std::cerr << "CountsMap <" << (int)base << "1>: " << builderX1.counts.size() << std::endl;
	for(CountsMap::const_iterator it = builderX1.counts.begin(); it != builderX1.counts.end(); it++)
	  std::cerr << " " << it->first << ": " << it->second << std::endl;
	assert(false);
      }
      Counts fromBuilding = bit->second;
      fromBuilding.all += fromBuilding.symmetric180;
      fromBuilding.all /= 2;

      if(countsMult != fromBuilding) {
	std::cerr << "Base " << c << std::endl;
	std::cerr << "Mults:   " << countsMult << std::endl;
	std::cerr << "Counted: " << fromBuilding << std::endl;
	std::cerr << "CountsMap <" << (int)base << "Y>: " << builder.counts.size() << std::endl;
	for(CountsMap::const_iterator it = builder.counts.begin(); it != builder.counts.end(); it++)
	  std::cerr << " " << it->first << ": " << it->second << std::endl;
	std::cerr << "CountsMap <" << (int)base << "1>: " << builderX1.counts.size() << std::endl;
	for(CountsMap::const_iterator it = builderX1.counts.begin(); it != builderX1.counts.end(); it++)
	  std::cerr << " " << it->first << ": " << it->second << std::endl;
	assert(false);
      }
      computed1XY.erase(bit);
    }
#endif
  }

  Lemma3::Lemma3(int base, int threadCount, Combination &maxCombination): base(base), threadCount(threadCount), maxCombination(maxCombination) {
#ifdef PROFILING
    Profiler::countInvocation("Lemma3::Lemma3()");
#endif
    assert(base >= 2);
    assert(base < maxCombination.size);
    assert(maxCombination.size <= MAX_BRICKS);
  }

  void Lemma3::precompute(int maxDist) {
#ifdef PROFILING
    Profiler::countInvocation("Lemma3::precompute(int)");
#endif

#ifdef DEBUG
    assert(maxCombination.height == 2);
    std::cout << "Building all <1XY> combinations for comparison" << std::endl;
    // Debugging <1XY>:
    // Compare with normal construction for <1XY>:
    Combination base1XY, max1XY; max1XY.size = 1+maxCombination.size; max1XY.height = 3;
    max1XY.layerSizes[1] = base; max1XY.layerSizes[2] = maxCombination.layerSizes[1];
    BrickPlane neighbours[3];
    for(int i = 0; i < 3; i++)
      neighbours[i].unsetAll();
    CombinationBuilder builder1XY(base1XY, 0, 1, neighbours, max1XY, true, false, true);
    builder1XY.buildSplit(threadCount);
    counts1XY = builder1XY.baseCounts;
    builder1XY.report();
    std::cout << "Combinations built!" << std::endl;
    // Cross check counting for bases:
    Counts xCheck;
    for(CombinationCountsMap::const_iterator it = counts1XY.begin(); it != counts1XY.end(); it++)
      xCheck += it->second;
    xCheck.all += xCheck.symmetric180;
    xCheck.all /= 2;
    uint64_t token1XY = maxCombination.getTokenFromLayerSizes();
    token1XY = Combination::reverseToken(token1XY) * 10 + 1;
    if(!Combination::checkCounts(token1XY, xCheck))
      return;
#endif

    for(int d = 2; d <= maxDist; d++) {
      std::chrono::time_point<std::chrono::steady_clock> timeStart { std::chrono::steady_clock::now() };

      std::stringstream ss; ss << "base_" << base << "_size_" << (int)maxCombination.size << "_refinement_";
      for(uint8_t i = 0; i < maxCombination.height; i++)
	ss << (int)maxCombination.layerSizes[i];
      ss << "/d" << d << ".bin";
      std::string fileName = ss.str();

      std::ifstream istream(fileName.c_str());
      if(istream.good()) {
	std::cout << "Precomputation for d=" << d << " already exists. Skipping!" << std::endl;
	continue;
      }

      BitWriter writer(fileName, base);
      std::vector<int> distances;

      precompute(distances, writer, d);

      std::chrono::duration<double, std::ratio<1> > duration = std::chrono::duration_cast<std::chrono::duration<double, std::ratio<1> > >(std::chrono::steady_clock::now() - timeStart);
      std::cout << "Precomputation done for max distance " << d << " in " << duration.count() << " seconds" << std::endl;
    }

#ifdef DEBUG
    if(!counts1XY.empty()) {
      std::cerr << "Bases not covered:" << std::endl;
      for(CombinationCountsMap::const_iterator it = counts1XY.begin(); it != counts1XY.end(); it++)
	std::cout << " " << it->first << ": " << it->second << std::endl;
    }
#endif
  }

  void Lemma3::precompute(std::vector<int> &distances, BitWriter &writer) {
#ifdef PROFILING
    Profiler::countInvocation("Lemma3::precompute(vector, BitWriter)");
#endif
    BaseBuilder baseBuilder(distances, writer);

#ifdef DEBUG
    BrickPlane *neighbourCache = new BrickPlane[MAX_HEIGHT];
    for(int i = 0; i < MAX_HEIGHT; i++)
      neighbourCache[i].unsetAll();
    Lemma3Runner runner(&baseBuilder, &maxCombination, 0, neighbourCache);
    runner.run(counts1XY);
#else
    int processorCount = threadCount-1;//std::thread::hardware_concurrency() - 1;

    BrickPlane *neighbourCache = new BrickPlane[processorCount * MAX_HEIGHT];
    for(int i = 0; i < processorCount * MAX_HEIGHT; i++)
      neighbourCache[i].unsetAll();

    Lemma3Runner *builders = new Lemma3Runner[processorCount];
    std::thread **threads = new std::thread*[processorCount];

    for(int i = 0; i < processorCount; i++) {
      builders[i] = Lemma3Runner(&baseBuilder, &maxCombination, i, &neighbourCache[i*MAX_HEIGHT]);
      threads[i] = new std::thread(&Lemma3Runner::run, std::ref(builders[i]));
    }

    for(int i = 0; i < processorCount; i++) {
      threads[i]->join();
      delete threads[i];
    }
    delete[] threads;
    delete[] builders;
    delete[] neighbourCache;
#endif
    baseBuilder.report();
  }

  void Lemma3::precompute(std::vector<int> &distances, BitWriter &writer, int maxDist) {
#ifdef PROFILING
    Profiler::countInvocation("Lemma3::precompute(vector, BitWriter, int)");
#endif
    int S = (int)distances.size();

    if(S == base-2) {
      std::cout << " Precomputing for distances";
      for(int i = 0; i < S; i++)
	std::cout << " " << distances[i];
      std::cout << " " << maxDist << std::endl;

      std::chrono::time_point<std::chrono::steady_clock> timeStart { std::chrono::steady_clock::now() };

      distances.push_back(maxDist); // Last dist is max dist
      precompute(distances, writer);
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
