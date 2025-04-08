#ifndef BFS_H
#define BFS_H

// DIFFLT = "Difference less than" is used to check for brick intersection
#define DIFFLT(a,b,c) ((a) < (b) ? ((b)-(a)<(c)) : ((a)-(b)<(c)))
#define ABS(a) ((a) < 0 ? -(a) : (a))

// Goal of this code base is to construct models with up to 11 bricks
#define MAX_BRICKS 11
// At most 9 bricks can then be in a single layer
#define MAX_LAYER_SIZE 9

#ifdef LEMMAS
// Larger PLANE size due to constructing from multiple starting points:
#define PLANE_MID 64
#define PLANE_WIDTH 128
#else
#define PLANE_MID 32
#define PLANE_WIDTH 64
#endif

#define BRICK first
#define LAYER second

#include "stdint.h"
#include <stdarg.h>
#include <iostream>
#include <fstream>
#include <map>
#include <set>
#include <mutex>
#include <thread>
#include <chrono>

#ifdef PROFILING
typedef std::pair<uint64_t,std::string> InvocationPair;
typedef std::map<std::string,uint64_t> InvocationMap;
extern InvocationMap *invocationCounts;
struct Profiler {
  static void countInvocation(const std::string &s);
  static void reportInvocations();
};
#endif
  
namespace rectilinear {

  /**
   * Struct used for totalling the number of models.
   * all includes the models counted for symmetric180 and symmetric90.
   * symmetric90 is 0 unless counting for <44>. In this case symmetric180
   * also includes the models counted for symmetric90.
   */
  struct Counts {
    uint64_t all, symmetric180, symmetric90;

    Counts();
    Counts(uint64_t all, uint64_t symmetric180, uint64_t symmetric90);
    Counts(const Counts& c);
    Counts& operator +=(const Counts &c);
    Counts operator -(const Counts &c) const;
    Counts operator /(const int &v) const;
    bool operator ==(const Counts &c) const;
    bool operator !=(const Counts &c) const;
    friend std::ostream& operator <<(std::ostream &os, const Counts &c);
    void reset(); // Sets counts to 0.
    bool empty();
  };

  /**
   * A Brick represents a 2x4 LEGO brick with an orientation and x,y-position.
   * x,y is the position of the middle of the brick (like in LDRAW).
   */
  struct Brick {
    bool isVertical;
    int16_t x, y;

    Brick();
    Brick(bool iv, int16_t x, int16_t y);
    Brick(const Brick &b);

    bool operator <(const Brick& b) const;
    bool operator ==(const Brick& b) const;
    bool operator !=(const Brick& b) const;
    friend std::ostream& operator <<(std::ostream &os, const Brick &b);
    int cmp(const Brick& b) const;
    bool intersects(const Brick &b) const;
    void mirror(Brick &b, const int16_t &cx, const int16_t &cy) const;
    bool mirrorEq(const Brick &b, const int16_t &cx, const int16_t &cy) const;
    int dist(const Brick &b) const;
  };

  typedef std::pair<Brick,uint8_t> LayerBrick;
  
  const Brick FirstBrick = Brick(); // At 0,0, horizontal

  typedef std::pair<uint8_t,uint8_t> BrickIdentifier; // layer, idx
  typedef std::map<int64_t,Counts> CountsMap; // token -> counts

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
    uint8_t colors[MAX_BRICKS][MAX_LAYER_SIZE];
    void colorConnected(uint8_t layer, uint8_t idx, uint8_t color);
  public:
    uint8_t layerSizes[MAX_BRICKS], height, size;
    Brick bricks[MAX_BRICKS][MAX_LAYER_SIZE];
    BrickIdentifier history[MAX_BRICKS];

    /*
      Rectilinear models with restriction: First brick of first layer must be FirstBrick.
    */
    Combination();
    Combination(const Combination &b);

    bool operator <(const Combination& b) const;
    bool operator ==(const Combination& b) const;
    friend std::ostream& operator << (std::ostream &os, const Combination &b);

    void copy(const Combination &b);
    void rotate90();
    void rotate180();

    void getLayerCenter(const uint8_t layer, int16_t &cx, int16_t &cy) const;
    bool isLayerSymmetric(const uint8_t layer, const int16_t &cx, const int16_t &cy) const;
    bool is180Symmetric() const;
    bool is90Symmetric() const;
    void sortBricks();
    void translateMinToOrigo();
    bool canRotate90() const;
    void addBrick(const Brick &b, const uint8_t layer);
    int64_t encodeConnectivity(int64_t token);
    void removeLastBrick();
    int64_t getTokenFromLayerSizes() const;
    void normalize(int &rotated);
    void normalize();
    static int reverseToken(int64_t token);
    static uint8_t heightOfToken(int64_t token);
    static uint8_t sizeOfToken(int64_t token);
    static void getLayerSizesFromToken(int64_t token, uint8_t *layerSizes);
  };

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

  class MultiLayerBrickPicker {
    const std::vector<LayerBrick> &v;
    const int maxPick;
    int toPick;
    LayerBrick bricks[MAX_BRICKS];
    BrickPicker *inner;
    std::mutex next_mutex;
  public:
    MultiLayerBrickPicker(const std::vector<LayerBrick> &v, const int maxPick);
    bool next(Combination &c, int &picked);
  };
 
  class CombinationBuilder {
    Combination baseCombination;
    uint8_t waveStart, waveSize, maxSize;
    BrickPlane *neighbours;
    uint8_t *maxLayerSizes;
  public:
    CountsMap counts;

    CombinationBuilder(const Combination &c,
		       const uint8_t waveStart,
		       const uint8_t waveSize,
		       const uint8_t maxSize,
		       BrickPlane *neighbours,
		       uint8_t *maxLayerSizes);

    CombinationBuilder(const CombinationBuilder& b);

    CombinationBuilder();

    void build();
    void buildSplit();
    void report();
    bool addFromPicker(MultiLayerBrickPicker *p, int &picked, const std::string &threadName);
    void removeFromPicker(int toRemove);
  private:
    void build(BrickPicker *picker, int toPick);
    void findPotentialBricksForNextWave(std::vector<LayerBrick> &v);
    bool nextCombinationCanBeSymmetric180();
    void placeAllLeftToPlace(const uint8_t &leftToPlace, const bool &canBeSymmetric180, const std::vector<LayerBrick> &v);
    void addCountsFrom(const CombinationBuilder &b, bool doubleCount);
  };

  class ThreadEnablingBuilder {
    MultiLayerBrickPicker *picker;
    std::chrono::time_point<std::chrono::steady_clock> timeStart { std::chrono::steady_clock::now() };
    std::string threadName;
  public:
    CombinationBuilder b;

    ThreadEnablingBuilder();
    ThreadEnablingBuilder(const ThreadEnablingBuilder &b);
    ThreadEnablingBuilder(Combination &c,
 			  const uint16_t waveStart,
 			  const uint16_t maxSize,
			  BrickPlane *neighbours,
			  uint8_t *maxLayerSizes,
 			  MultiLayerBrickPicker *picker, int threadIndex);
    void build();
  };

  class Lemma2 {
    int n;
    CountsMap counts;
  public:
    Lemma2(int n);
    bool computeOnBase2(bool vertical, int16_t dx, int16_t dy, Counts &c, Counts &d);
    void computeOnBase2();
    void report() const;
  };

  /*
    Write precalculations to stream:
    bit=1 to indicate start of a batch of results
    bit to indicate if base is symmetric
    for each result in batch:
     bit=0 to indicate a result
     (base-1) x 3 bits to indicate colors
     8 bits to indicate refinement without base layer
     32 bits for all
     16 bits for symmetric180
     8 bits for symmetric90 if base == 4

     End of stream:
     1 to indicate a batch, then bs=0, indicator=0, colors all 0, 0 for all counts.
     Finally totals in 64 bit integers for cross checking
   */
  class BitWriter {
    std::ofstream *ostream;
    uint8_t base;
    uint8_t bits, cntBits;
    uint64_t sumTotal, sumSymmetric180, sumSymmetric90, lines;

  public:
    BitWriter();
    BitWriter(const BitWriter &w);
    BitWriter(const std::string &fileName, uint8_t base);
    ~BitWriter(); // Write end indicator with symmetric > total
    void writeColor(uint8_t toWrite); // 3 bits, starting from 0
    void writeBit(bool bit); // Also used for baseSymmetric bit
    void writeUInt16(uint16_t toWrite); // Used for symmetric180 and token
    void writeUInt8(uint8_t toWrite); // Used for symmetric90 - only when base = 4
    void writeCounts(const Counts &c);
  private:
    void flushBits();
    void writeUInt32(uint32_t toWrite); // Used for total
    void writeUInt64(uint64_t toWrite); // Used for totals
  };

  class Lemma3 {
    int n, base;
    CountsMap counts;
    BrickPlane neighbours[MAX_BRICKS];
    CountsMap crossCheck;

  public:
    Lemma3(int n, int base);
    void precompute(int maxDist);

  private:
    void precomputeOn(const Combination &baseCombination, BitWriter &writer);
    void precomputeForPlacements(const std::vector<int> &distances,
				 std::vector<Brick> &bricks,
				 BitWriter &writer,
				 std::set<Combination> &seen);
    void precompute(std::vector<int> &distances, BitWriter &writer, int maxDist);
  };
}

#endif
