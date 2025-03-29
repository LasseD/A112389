#ifndef BFS_H
#define BFS_H

// DIFFLT = Difference less than is used to check for brick intersection
#define DIFFLT(a,b,c) ((a) < (b) ? ((b)-(a)<(c)) : ((a)-(b)<(c)))

// Goal of 2025 is to construct models with at most 11 bricks
#define MAX_BRICKS 11
// At most 8 bricks, since 2 are used for the 2-brick layer, and 1 is on one side.
#define MAX_LAYER_SIZE 8
// Max height used to restrict constructions, not to reduce object size:

#define PLANE_MID 32
#define PLANE_WIDTH 64
#define BRICK first
#define LAYER second

#include "stdint.h"
#include <stdarg.h>
#include <iostream>
#include <fstream>
#include <map>
#include <mutex>
#include <thread>

namespace rectilinear {

  /**
   * Struct used for totalling the number of models.
   * all includes the models counted for symmetric180 (180 degree symmetries).
   * 90 degree symmetries are not possible due to the layer with 2 bricks.
   */
  struct Counts {
    uint64_t all, symmetric180;

    Counts();
    Counts(uint64_t all, uint64_t symmetric180);
    Counts(const Counts& c);
    Counts& operator +=(const Counts &c);
    Counts operator -(const Counts &c) const;
    Counts operator /(const int &v) const;
    bool operator !=(const Counts &c) const;
    bool operator ==(const Counts &c) const;
    bool empty() const;
    friend std::ostream& operator <<(std::ostream &os, const Counts &c);
    void reset(); // Sets counts to 0.
  };

  /**
   * A Brick represents a 2x4 LEGO brick with an orientation and x,y-position.
   * x,y is the position of the middle of the brick (like in LDRAW).
   */
  struct Brick {
    bool isVertical:1; // :1 gives the compiler the hint that only 1 bit is needed.
    // Could use :6 when 6 bricks is maximum size, but it would not provide practical benefits,
    // as 7+7+1 still packs in 2 bytes:
    int8_t x:7, y:7;

    Brick();
    Brick(bool iv, int8_t x, int8_t y);
    Brick(const Brick &b);

    bool operator <(const Brick& b) const;
    bool operator ==(const Brick& b) const;
    bool operator !=(const Brick& b) const;
    friend std::ostream& operator <<(std::ostream &os, const Brick &b);
    int cmp(const Brick& b) const;
    bool intersects(const Brick &b) const;
    void mirror(Brick &b, const int8_t &cx, const int8_t &cy) const;
    bool mirrorEq(const Brick &b, const int8_t &cx, const int8_t &cy) const;
  };

  typedef std::pair<Brick,uint8_t> LayerBrick;
  
  const Brick FirstBrick = Brick(); // At 0,0, horizontal

  typedef std::pair<uint8_t,uint8_t> BrickIdentifier; // layer, idx
  typedef std::map<int,Counts> CountsMap;

  class BrickPicker {
    const std::vector<LayerBrick> &v;
    int vIdx;
    const int numberOfBricksToPick, bricksIdx;
    LayerBrick *bricks;
    BrickPicker *inner;

    bool checkVIdx() const;
    void nextVIdx();
    
  public:
    BrickPicker(const std::vector<LayerBrick> &v, int vIdx, const int numberOfBricksToPick, LayerBrick *bricks, const int bricksIdx);
     ~BrickPicker();

    bool next();
  };

  struct BrickPlane {
    bool bricks[2][PLANE_WIDTH][PLANE_WIDTH];
    void unsetAll();
    void set(const Brick &b);
    void unset(const Brick &b);
    bool contains(const Brick &b);
  };
  
  class Combination {
  private:
    // State to check connectivity:
    bool connected[MAX_BRICKS][MAX_LAYER_SIZE];
    int countConnected(int layer, int idx);
  public:
    uint8_t layerSizes[MAX_BRICKS], height, size;
    Brick bricks[MAX_BRICKS][MAX_LAYER_SIZE];
    BrickIdentifier history[MAX_BRICKS];

    /*
      Rectilinear models with restrictions on representation:
      - first brick must be vertical at first layer and placed at 0,0
      - model is minimal of all rotations (vs rotated 90, 180, 270 degrees)
      - bricks are lexicographically sorted (orientation,x,y) on each layer
      Init with one brick on layer 0 at 0,0.
    */
    Combination();
    Combination(const Combination &b);

    bool operator <(const Combination& b) const;
    bool operator ==(const Combination& b) const;
    friend std::ostream& operator << (std::ostream &os, const Combination &b);

    void copy(const Combination &b);
    void rotate180();

    void getLayerCenter(const uint8_t layer, int16_t &cx, int16_t &cy) const;
    bool isLayerSymmetric(const uint8_t layer, const int16_t &cx, const int16_t &cy) const;
    bool is180Symmetric() const;
    void sortBricks();
    void translateMinToOrigo();
    void addBrick(const Brick &b, const uint8_t layer);
    bool isConnected();
    void removeLastBrick();
    int getTokenFromLayerSizes() const;
    static int reverseToken(int token);
    static uint8_t heightOfToken(int token);
    static uint8_t sizeOfToken(int token);
    static void getLayerSizesFromToken(int token, uint8_t *layerSizes);
  };

  class CombinationBuilder {
    Combination baseCombination;
    uint8_t waveStart, waveSize, maxSize, indent;
  public:
    CountsMap counts; // token -> counts, negative token for disconnected counts
  private:
    BrickPlane *neighbours;
    bool done;
  public:
    CombinationBuilder(Combination &c, const uint8_t waveStart, const uint8_t waveSize, const uint8_t maxSize);

    CombinationBuilder(Combination &c, const uint8_t waveStart, const uint8_t waveSize, const uint8_t maxSize, const uint8_t indent, BrickPlane *neighbours);

    CombinationBuilder(const CombinationBuilder& b);

    CombinationBuilder();

    void build(bool hasSplitIntoThreads);
    void report();
  private:
    void findPotentialBricksForNextWave(std::vector<LayerBrick> &v);
    bool nextCombinationCanBeSymmetric180();
    void placeAllLeftToPlace(const uint8_t &leftToPlace, const bool &canBeSymmetric180, const std::vector<LayerBrick> &v);
    void joinOne(CombinationBuilder *builders, std::thread **threads, int n);
    void addCountsFrom(const CombinationBuilder &b);
  };

  class Lemma2 {
    int left, right, n;
    CountsMap counts;
  public:
    Lemma2(int n);
    Lemma2(int left, int right, int n);
    void computeAllLeftAndRight();
    void computeOnBase2(bool vertical, int8_t dx, int8_t dy, Counts &c, Counts &d);
    void computeOnBase2();
    void report() const;
  };
}
#endif
