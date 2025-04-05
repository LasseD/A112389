import sys
import re
from pathlib import Path

class Brick:
    def __init__(self, v, x, y):
        assert(not (v and x == 0 and y == 0))
        self.v = v
        self.x = x
        self.y = y
    def __eq__(self, other):
        return self.v == other.v and self.x == other.x and self.y == other.y
    def __hash__(self):
        return (self.v << 16) + (self.x << 8) + self.y
    def __repr__(self):
        if self.v:
            return f"|{self.x},{self.y}|"
        else:
            return f"={self.x},{self.y}="

class BrickPair:
    def __init__(self, b1, b2):
        self.b1 = b1
        self.b2 = b2
    def __eq__(self, other):
        return self.b1 == other.b1 and self.b2 == other.b2
    def __hash__(self):
        return (self.b1.__hash__() << 17) + self.b2.__hash__()
    def __repr__(self):
        return f" {self.b1}  {self.b2} "

class Report:
    def __init__(self, connectivity, bp, baseSymmetric, total, symmetric):
        self.connectivity = connectivity
        self.bp = bp
        self.baseSymmetric = baseSymmetric
        self.total = total
        self.symmetric = symmetric
    def __repr__(self):
        if self.symmetric > 0:
            return f"{self.connectivity} {self.bp} base symmetric: {self.baseSymmetric}: {self.total} ({self.symmetric})"
        else:
            return f"{self.connectivity} {self.bp} base symmetric: {self.baseSymmetric}: {self.total}"
    def __eq__(self, other):
        return self.bp == other.bp and self.connectivity == other.connectivity

CACHE = {} # refinement -> [Report...]

def fetch(n):
    dir = '../bfs_wave_approach/base_3_size_' + str(n)
    print(' Fetching data for size', n, 'from folder', dir)
    for file in Path(dir).iterdir():
        if not file.is_file():
            continue
        name = re.split(r"[_\.]", file.name)
        #print('file', name)
        assert len(name) == 5
        assert name[0] == 'd1'
        d1 = int(name[1])
        assert name[2] == 'd2'
        d2 = int(name[3])

        if name[4] != 'txt':
            continue # Tmp file or similar
        content = open(file)

        while True:
            # B1-B2 31 V1 0 X1 2 Y1 0 V2 1 X2 5 Y2 0 TOTAL 10 SYMMETRIC 0
            line = content.readline()
            if not line:
                break
            (connectivity, refinement, _b1, v1, x1, y1, _b2, v2, x2, y2, _bs, bs, _total, total, _symmetric, symmetric) = line.split()
            assert(_b1 == 'B1')
            b1 = Brick(v1 == '1', int(x1), int(y1))
            assert(_b2 == 'B2')
            b2 = Brick(v2 == '1', int(x2), int(y2))
            bp = BrickPair(b1, b2)
            report = Report(connectivity, bp, bs == '1', int(total), int(symmetric))
            assert(_bs == 'BASE_SYMMETRIC')
            assert(_total == 'TOTAL')
            assert(_symmetric == 'SYMMETRIC')
            if not refinement in CACHE:
                CACHE[refinement] = {}
            if not bp in CACHE[refinement]:
                CACHE[refinement][bp] = []
            #assert not report in CACHE[refinement]
            CACHE[refinement][bp].append(report)

size = int(sys.argv[1])
left = int(sys.argv[2])
right = size - left - 3
print('Refinements of size', size, 'with', left, 'on the left and', right, 'on the right')
print()

fetch(left+3)
if left != right:
    fetch(right+3)

countsAll = {}
countsSymmetric = {}

def connected(c1, c2):
    if c1 == 'B0-B1-B2' or c2 == 'B0-B1-B2':
        return True
    if c1 == '-' or c2 == '-':
        return False # Could only connect if other size fully connects
    return c1 != c2

def countUp(token1, token2, token, brickPair):
    #print(brickPair)
    cache1 = CACHE[token1][brickPair]
    cache2 = CACHE[token2][brickPair]

    retA = 0
    retS = 0
    bs = False
    for r1 in cache1:
        if bs:
            assert r1.baseSymmetric
        if r1.baseSymmetric:
            bs = True
        for r2 in cache2:
            if not connected(r1.connectivity, r2.connectivity):
                continue # Not connected:
            assert(r1.baseSymmetric == r2.baseSymmetric)

            # Computing the products: (Currently overcounting by factor 8)
            # a1 contains models:
            # - 2 x if base rotationally symmetric and not self symmetric
            # - 1 x if not
            # So add up to 2 x a1 and 2 x a2 and divide by 4 at the very end
            #
            a1 = r1.total
            s1 = r1.symmetric
            a2 = r2.total
            s2 = r2.symmetric

            #print('   Adding for', r1.connectivity, r2.connectivity, a1, s1, a2, s2)
            all = a1 * a2
            symmetric = s1 * s2
            retA = retA + all
            retS = retS + symmetric

    #print('  Counted', retA, retS)
    if bs:
        assert((retA - retS)%2 == 0)
        retA = int((retA - retS)/2) + retS
        #print('  Updated to', retA, retS)
    #print(brickPair, retA, retS)
    countsAll[token] = countsAll[token] + retA
    countsSymmetric[token] = countsSymmetric[token] + retS
                
def getBrickPairs(token):
    return set([x for x in CACHE[token]])

def handle(token1, token2):
    token = token1[::-1] + token2[1::]
    if not token in countsAll:
        countsAll[token] = 0
        countsSymmetric[token] = 0
    print('  Get brick pairs for tokens', token1, token2)
    bricks = getBrickPairs(token1) | getBrickPairs(token2)
    print('  Count for pairs of tokens', token1, token2)
    for brickPair in bricks:
        countUp(token1, token2, token, brickPair)

def handleAllTokens():
    for token1 in CACHE:
        size1 = sum(int(x) for x in token1)
        if size1 == left + 3:
            for token2 in CACHE:
                size2 = sum(int(x) for x in token2)
                if size2 == right + 3:
                    handle(token1, token2)
handleAllTokens()

print()
for token in countsAll:
    a = countsAll[token]
    s = countsSymmetric[token]
    print(' <'+token+'>', a, '(' + str(s) + ')')
print()

# Single sided sum for cross check:
for token in CACHE:
    crossCheckA = 0
    crossCheckS = 0
    for bp in CACHE[token]:
        for report in CACHE[token][bp]:
            if report.connectivity == 'B0-B1-B2': # Connected:
                crossCheckA = crossCheckA + (report.total if not report.baseSymmetric else (report.total-report.symmetric)/2+report.symmetric)
                crossCheckS = crossCheckS + report.symmetric
    print('  Cross check', token, ':', int(crossCheckA), crossCheckS)

# Testing
# OK <31> 648 (8)
# OK <131> 433685 (24)
# OK <32> 148794 (443)
# OK <231> 41019966 (1179)
# OK <232> 3021093957 (46219)
# <33> 6246077 (432)
# <331> 1358812234 (1104)
# <1321> 4581373745 (1471)

# <43> 106461697 (10551)
