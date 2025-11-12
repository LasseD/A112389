#ifndef RECTILINEAR_H
#define RECTILINEAR_H

// DIFFLT = "Difference less than" is used to check for brick intersection
#define DIFFLT(a,b,c) ((a) < (b) ? ((b)-(a)<(c)) : ((a)-(b)<(c)))
#define ABS(a) ((a) < 0 ? -(a) : (a))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) < (b) ? (b) : (a))

// Set the MAX_ macros as tightly as possible for your computations.
// Example:
// Lemma 1 can be used to compute all of height 7 and up for 11 bricks,
// so MAX_HEIGHT can be set to 5 for the computations needed here
#ifdef DEBUG
#define MAX_BRICKS 11
#define MAX_HEIGHT 7
#define MAX_LAYER_SIZE 9
#else
#define MAX_BRICKS 11
#define MAX_HEIGHT 5
#define MAX_LAYER_SIZE 6
#endif

// These are used in bitmap lookups for checking positions of bricks:
#define PLANE_MID 100
#define PLANE_WIDTH 200

#define BRICK first
#define LAYER second

#define BINOMIAL_CACHE_SIZE 200

// For reporting on bases:
#define NORMAL 0
#define MIRROR_X 1
#define MIRROR_Y 2
#define SMALLER_BASE 3

#include "stdint.h"
#include <stdarg.h>
#include <iostream>
#include <fstream>
#include <map>
#include <set>
#include <vector>
#include <mutex>
#include <thread>
#include <chrono>

namespace rectilinear {

  class BinomialCoefficient {
    static uint64_t cache[][BINOMIAL_CACHE_SIZE];
    static uint64_t nkSlow(uint64_t n, uint64_t k);
  public:
    static void init();
    static uint64_t nChooseK(uint64_t n, uint64_t k);
  };

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
    Counts& operator -=(const Counts &c);
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

    static bool canReach(const Brick &a, const Brick &b, uint8_t toAdd);
  };

  const Brick FirstBrick = Brick(); // At 0,0, horizontal

  typedef std::pair<Brick,uint8_t> LayerBrick;
  typedef std::pair<uint8_t,uint8_t> BrickIdentifier; // layer, idx
  typedef std::map<int64_t,Counts> CountsMap; // token -> counts

  struct BrickPlane {
    int8_t bricks[2][PLANE_WIDTH][PLANE_WIDTH];
#ifdef DEBUG
    int sum;
#endif
    void reset();
    void set(const bool v, const int16_t x, const int16_t y);
    void set(const bool v, const int16_t x, const int16_t y, const int8_t add);
    void unset(const bool v, const int16_t x, const int16_t y);
    void unset(const Brick &b);
    bool contains(const bool v, const int16_t x, const int16_t y);
  };

  struct Base;

  class Combination {
    // State to check connectivity:
    void colorConnected(uint8_t layer, uint8_t idx, uint8_t color);
    uint8_t countConnected(uint8_t layer, uint8_t idx);
    bool hasVerticalLayer0Brick() const;
  public:
    uint8_t colors[MAX_HEIGHT][MAX_LAYER_SIZE];
    uint8_t layerSizes[MAX_HEIGHT], height, size;
    Brick bricks[MAX_HEIGHT][MAX_LAYER_SIZE];
    BrickIdentifier history[MAX_BRICKS];

    /*
      Rectilinear models with restriction: First brick of first layer must be FirstBrick.
    */
    Combination();
    Combination(int token);
    Combination(const Base &b);
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
    void mirrorX();
    void mirrorY();
    void normalize();
    void addBrick(const Brick &b, const uint8_t layer);
    void addBrick(const LayerBrick &lb);
    int64_t encodeConnectivity(int64_t token);
    void removeLastBrick();
    int64_t getTokenFromLayerSizes() const;
    bool isConnected();
    bool canBecomeSymmetric(const Combination &maxCombination) const;
    void colorFull();
    static int64_t reverseToken(int64_t token);
    static uint8_t heightOfToken(int64_t token);
    static uint8_t sizeOfToken(int64_t token);
    static void getLayerSizesFromToken(int64_t token, uint8_t *layerSizes);
    static int countBricksToBridge(const Combination &maxCombination);
    static void setupKnownCounts(CountsMap &m);
    static bool checkCounts(uint64_t token, const Counts &c);
  };

  struct CBase;
  struct Base {
    uint8_t layerSize;
    Brick bricks[MAX_LAYER_SIZE];

    Base();
    Base(const Base &b);
    Base(const CBase &b);

    bool operator <(const Base& b) const;
    bool operator ==(const Base& b) const;
    friend std::ostream& operator << (std::ostream &os, const Base &b);

    void copy(const Base &b);
    void rotate90();
    void rotate180();

    void getLayerCenter(int16_t &cx, int16_t &cy) const;
    bool isLayerSymmetric(const int16_t &cx, const int16_t &cy) const;
    bool is180Symmetric() const;
    bool is90Symmetric() const;
    void sortBricks();
    void translateMinToOrigo();
    bool canRotate90() const;
    void mirrorX();
    void mirrorY();
    void normalize();
    void reduceFromUnreachable(const Combination &maxCombination, CBase &baseOut) const;
  private:
    bool hasVerticalLayer0Brick() const;
  };

  typedef std::pair<Brick,int> CBrick; // Brick with initial position
  struct CBase {
    uint8_t layerSize;
    CBrick bricks[MAX_LAYER_SIZE];

    CBase();
    CBase(const Base &b);
    CBase(const CBase &b);

    bool operator <(const CBase& b) const;
    friend std::ostream& operator << (std::ostream &os, const CBase &b);

    void copy(const CBase &b);
    void rotate90();
    void rotate180();
    void sortBricks();
    void translateMinToOrigo();
    bool canRotate90() const;
    void mirrorX();
    void mirrorY();
    void normalize();
  private:
    bool hasVerticalLayer0Brick() const;
  };
  
  class BrickPicker {
    const std::vector<LayerBrick> &v; // Available bricks
    int vIdx; // First index of available bricks
    const int numberOfBricksToPick;
    BrickPicker *inner;

    bool checkVIdx(const Combination &c, const Combination &maxCombination) const;
    void nextVIdx(const Combination &c, const Combination &maxCombination);

  public:
    BrickPicker(const std::vector<LayerBrick> &v, const int vIdx, const int numberOfBricksToPick);
    ~BrickPicker();

    bool next(Combination &c, const Combination &maxCombination);
  };

  typedef std::map<Base,CountsMap> BaseResultsMap;
  typedef std::map<Base,Base> CombinationMap;
  typedef std::map<Combination,Counts> CombinationCountsMap;
  typedef std::pair<bool,Combination> CombinationIdentification; // True if self

  class BaseBuildingManager {
    const std::vector<LayerBrick> &v;
    const int maxPick;
    int toPick;
    LayerBrick bricks[MAX_BRICKS];
    BrickPicker *inner;
    CombinationCountsMap countsMap; // Combination -> Counts
    std::vector<Combination> combinations;
    std::mutex mutex;
  public:
    BaseBuildingManager(const std::vector<LayerBrick> &v, const int maxPick);
    uint8_t next(Combination &c, const Combination &maxCombination);
    void add(const Combination &c, const Counts &counts);
    Counts getCounts() const;
  };

  /*
    The "Normal" building manager simply serves combinations and does not
    perform the optimizations of BaseBuildingManager as wave information would
    then have to be used to properly detect duplicates.
   */
  class NormalBuildingManager {
    const std::vector<LayerBrick> &v;
    const int maxPick;
    int toPick;
    LayerBrick bricks[MAX_BRICKS];
    BrickPicker *inner;
    Counts sum;
    std::mutex mutex;
  public:
    NormalBuildingManager(const std::vector<LayerBrick> &v, const int maxPick);
    uint8_t next(Combination &c, const Combination &maxCombination);
    void add(const Counts &counts);
    Counts getCounts() const;
  };

  class CombinationBuilder {
    Combination baseCombination;
    uint8_t waveStart, waveSize;
    BrickPlane *neighbours;
    Combination maxCombination;
    bool isFirstBuilder, encodingLocked;
  public:
    CountsMap counts;

    CombinationBuilder(const Combination &c,
		       const uint8_t waveStart,
		       const uint8_t waveSize,
		       BrickPlane *neighbours,
		       Combination &maxCombination,
		       bool encodingLocked);

    CombinationBuilder(const Base &c,
		       BrickPlane *neighbours,
		       Combination &maxCombination);

    CombinationBuilder(const CombinationBuilder& b);

    CombinationBuilder();

    void build();
    void report();
    Counts report(uint64_t returnToken); // Returns counts for token if present in results
    void addWaveToNeighbours(int8_t add);
  private:
    void findPotentialBricksForNextWave(std::vector<LayerBrick> &v);
    uint64_t simonWithBuckets(std::vector<std::vector<LayerBrick> > &buckets, uint32_t *bucketIndices, uint32_t numBuckets, uint32_t *bucketSizes);
    uint64_t placeAllSizedBuckets(std::vector<std::vector<LayerBrick> > &buckets, uint32_t *bucketIndices, uint32_t numBuckets, uint32_t leftToPlace, uint32_t *bucketSizes, uint32_t bucketSizesI);
    void placeAllInBuckets(std::vector<std::vector<LayerBrick> > &buckets, uint32_t *bucketIndices, uint32_t bucketI, uint32_t bucketIndicesI, uint32_t numBuckets, uint32_t leftToPlace);
    void setUpBucketsForSimon(std::vector<std::vector<LayerBrick> > &buckets, const std::vector<LayerBrick> &v);
    bool placeAllLeftToPlace(const uint8_t &leftToPlace, const std::vector<LayerBrick> &v); // Return true if all done here
    void addCountsFrom(const CountsMap &counts);
    uint64_t countInvalid(std::vector<std::vector<LayerBrick> > &buckets, uint32_t *bucketIndices, uint32_t numBuckets, uint32_t *bucketSizes, uint32_t bucketI, uint32_t bucketII, uint32_t pickedFromCurrentBucket, uint32_t pickedTotal);
  };

  class NonEncodingCombinationBuilder {
    Combination baseCombination;
    uint8_t waveStart, waveSize;
    BrickPlane *neighbours;
    Combination maxCombination;
  public:
    NonEncodingCombinationBuilder(const Combination &c,
				  const uint8_t waveStart,
				  const uint8_t waveSize,
				  BrickPlane *neighbours,
				  Combination &maxCombination);
    NonEncodingCombinationBuilder(const NonEncodingCombinationBuilder& b);
    NonEncodingCombinationBuilder();

    static Counts buildWithPartials(int threadCount, Combination &maxCombination);
    Counts build();
    int addFrom(NormalBuildingManager *manager, const std::string &threadName);
    void countAndRemoveFrom(NormalBuildingManager *manager, const Counts &counts, int toRemove);
    void addWaveToNeighbours(int8_t add);
  private:
    Counts buildSplit(int threadCount);
    void findPotentialBricksForNextWave(std::vector<LayerBrick> &v);
    uint64_t countInvalid(Brick *combination, int combinationSize, int N, Brick const * const v, const int sizeV, const int vIndex) const;
    uint64_t simon(const uint8_t &N, const std::vector<LayerBrick> &v) const;
    Counts placeAllLeftToPlace(const uint8_t &leftToPlace, const std::vector<LayerBrick> &v);
  };

  class SplitBuildingBuilder {
    BrickPlane *neighbours;
    Combination *baseCombination, *maxCombination;
    NormalBuildingManager *manager;
    std::chrono::time_point<std::chrono::steady_clock> timeStart { std::chrono::steady_clock::now() }, timePrev = timeStart;
    std::string threadName;
  public:
    SplitBuildingBuilder();
    SplitBuildingBuilder(const SplitBuildingBuilder &b);
    SplitBuildingBuilder(BrickPlane *neighbours,
			 Combination *baseCombination,
			 Combination *maxCombination,
			 NormalBuildingManager *manager,
			 int threadIndex);
    void build();
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
    bool largeCountsRequired;

  public:
    BitWriter();
    BitWriter(const BitWriter &w);
    BitWriter(const std::string &fileName, const Combination &maxCombination);
    ~BitWriter(); // Write end indicator with symmetric > total
    void writeColor(uint8_t toWrite); // 3 bits, starting from 0
    void writeBrick(const Brick &b); // isVertical, x, y
    void writeBit(bool bit); // Also used for baseSymmetric bit
    void writeUInt16(uint16_t toWrite); // Used for symmetric180 and token
    void writeUInt8(uint8_t toWrite); // Used for symmetric90 - only when base = 4
    void writeCounts(const Counts &c);
    static bool areLargeCountsRequired(const Combination &maxCombination);
  private:
    void flushBits();
    void writeUInt32(uint32_t toWrite); // Used for total
    void writeUInt64(uint64_t toWrite); // Used for totals
  };

  struct Report {
    uint8_t base, colors[6]; // Lemma 3 is only used up to base 7
    bool baseSymmetric180, baseSymmetric90;
    Counts counts;
    Base c;

    Report();
    Report(const Report &r);

    friend std::ostream& operator <<(std::ostream &os, const Report &r);
    static bool connected(const Report &a, const Report &b);
    static Counts countUp(const Report &reportA, const Report &reportB);
    static void getReports(const CountsMap &cm, std::vector<Report> &reports, uint8_t base, bool b180, bool b90);
  };

  class BitReader {
    std::ifstream *istream;
    uint8_t bits, bitIdx, base;
    uint64_t sumTotal, sumSymmetric180, sumSymmetric90, lines;
    bool largeCountsRequired;

    bool readBit();
    uint8_t readColor();
    void readBrick(Brick &b);
    uint8_t readUInt8();
    uint16_t readUInt16();
    uint32_t readUInt32();
    uint64_t readUInt64();
    void readCounts(Counts &c);
  public:
    BitReader(const Combination &maxCombination, int D, std::string directorySuffix);
    ~BitReader();
    bool next(std::vector<Report> &v);
  };

  class IBaseProducer {
  public:
    virtual bool nextBase(Base &c) = 0;
    virtual void resetCombination(Base &c) = 0;
    virtual ~IBaseProducer() = default;
  };

  class Size1InnerBaseBuilder final : public IBaseProducer {
    int16_t encoded, d;
    const int16_t D;
    Brick b;
  public:
    Size1InnerBaseBuilder(int16_t D);
    bool nextBase(Base &c);
    void resetCombination(Base &c);
  };

  class InnerBaseBuilder final : public IBaseProducer {
    const int16_t idx;
    int16_t encoded, d;
    const int16_t D;
    IBaseProducer * inner;
    Brick b;
  public:
    InnerBaseBuilder(int16_t size, const std::vector<int> &distances);
    ~InnerBaseBuilder();
    bool nextBase(Base &c);
    void resetCombination(Base &c);
  };

  typedef std::pair<int,CBase> BaseIdentification;
  typedef std::pair<Base,BaseIdentification> BaseWithID; // Used to identify how base was computed

  class BaseBuilder {
    std::vector<int> distances;
    IBaseProducer *innerBuilder;
    BitWriter *writer;
  public:
    BaseResultsMap resultsMap; // Base -> Result
    std::vector<BaseWithID> bases;
    std::mutex mutex;
    int checkMirrorSymmetries(const Base &c, CBase &mirrrored); // Return true if handled here
    uint64_t reachSkips, mirrorSkips, noSkips;
  public:
    BaseBuilder();
    ~BaseBuilder();
    bool nextBaseToBuildOn(Base &buildBase, Base &registrationBase, const Combination &maxCombination);
    void registerCounts(Base &registrationBase, CountsMap counts);
    void report(const Combination &maxCombination);
    void setWriter(BitWriter *writer);
    void reset(const std::vector<int> &distances);
  };

  class Lemma3Runner {
    BaseBuilder *baseBuilder;
    Combination *maxCombination;
    BrickPlane *neighbours;
    std::string threadName;
  public:
    Lemma3Runner();
    Lemma3Runner(const Lemma3Runner &b);
    Lemma3Runner(BaseBuilder *b,
		 Combination *maxCombination,
		 int threadIndex,
		 BrickPlane *neighbours);
    void run();
  };

  class Lemma3 {
    int base, threadCount, token;
    CountsMap counts;
    Combination maxCombination;
  public:
    Lemma3(int base, int threads, Combination &maxCombination);
    void precompute(int maxDist);
    void precompute(int maxDist, bool overwriteFiles);
  private:
    void precompute(BaseBuilder *baseBuilder, std::vector<int> &distances);
    void precompute(BaseBuilder *baseBuilder, std::vector<int> &distances, int maxDist);
  };
}

#endif
