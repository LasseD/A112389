import sys
import re
from pathlib import Path

class Brick:
    def __init__(self, v, x, y):
        assert(type(v) is bool)
        assert(type(x) is int)
        assert(type(y) is int)
        assert(not (v and x == 0 and y == 0))
        self.v = v
        self.x = x
        self.y = y
    def __eq__(self, other):
        return self.v == other.v and self.x == other.x and self.y == other.y
    def __hash__(self):
        return (self.v << 7) + (self.x << 7) + self.y
    def __repr__(self):
        if self.v:
            return f"|{self.x},{self.y}|"
        else:
            return f"={self.x},{self.y}="
    def __str__(self):
        return self.__repr__()

class BrickList:
    def __init__(self):
        self.v = []
    def __eq__(self, other):
        if(len(self.v) != len(other.v)):
            return False
        for i in range(len(self.v)):
            if self.v[i] != other.v[i]:
                return False
        return True
    def __hash__(self):
        ret = 0
        for i in range(len(self.v)):
            ret = (ret << 16) + self.v[i].__hash__()
        return ret
    def __repr__(self):
        return '  '.join([str(x) for x in self.v])
    def __str__(self):
        return self.__repr__()
    def append(self, b):
        self.v.append(b)

class Report:
    def __init__(self, connectivity, bricks, baseSymmetric, total, symmetric):
        assert(type(connectivity) is str)
        assert(type(bricks) is BrickList)
        assert(type(baseSymmetric) is bool)
        assert(type(total) is int)
        assert(type(symmetric) is int)
        self.connectivity = connectivity
        self.bricks = bricks
        self.baseSymmetric = baseSymmetric
        self.total = total
        self.symmetric = symmetric
        #print(connectivity, bricks, baseSymmetric, total, symmetric)
    def __repr__(self):
        if self.symmetric > 0:
            return f"{self.connectivity} {self.bricks} base symmetric: {self.baseSymmetric}: {self.total} ({self.symmetric})"
        else:
            return f"{self.connectivity} {self.bricks} base symmetric: {self.baseSymmetric}: {self.total}"
    def __eq__(self, other):
        if len(self.bricks.v) != len(other.bricks.v):
            return False
        for i in range(len(self.bricks.v)):
            if self.bricks.v[i] != other.bricks.v[i]:
                return False
        return self.connectivity == other.connectivity

CACHE = {} # refinement -> [Report...]

left = int(sys.argv[1])
base = int(sys.argv[2])
right = int(sys.argv[3])
size = left + base + right
print('Refinements of size', size, 'left-base-right:', left, base, right)
print()

def fetch(n):
    dir = '../bfs_wave_approach/base_' + str(base) + '_size_' + str(n)
    print(' Fetching data for base ', base, 'size', n, 'from folder', dir)
    cnt = 0
    for file in Path(dir).iterdir():
        if not file.is_file():
            continue
        cnt = cnt + 1
        if cnt % 10 == 0:
            print('.', end='', flush=True)
        name = re.split(r"[_\.]", file.name)
        if name[len(name)-1] != 'txt':
            print('Skipping file', file.name)
            continue # Tmp file or similar
        content = open(file)

        while True:
            line = content.readline()
            if not line:
                print('Warning: Missing file end for file', file.name)
                break
            if line[0] == 'Q':
                break
            parts = line.split()
            connectivity = parts[0]
            refinement = parts[1]
            bs = parts[len(parts)-5] == '1'
            total = int(parts[len(parts)-3])
            symmetric = int(parts[len(parts)-1])
            bricks = BrickList()
            for i in range(2, len(parts)-6, 4):
                bricks.append(Brick(parts[i+1] == '1', int(parts[i+2]), int(parts[i+3])))
            report = Report(connectivity, bricks, bs, total, symmetric)
            if not refinement in CACHE:
                CACHE[refinement] = {}
            if not bricks in CACHE[refinement]:
                CACHE[refinement][bricks] = []
            if report in CACHE[refinement][bricks]:
                print(refinement, bricks, 'has already observed:', report, 'in file', file.name)
            else:
                CACHE[refinement][bricks].append(report)
    print()

fetch(left+base)
if left != right:
    fetch(right+base)

countsAll = {}
countsSymmetric = {}

backwardCompatibleConnections = {
    'B0-B1-B2': '1-1-1',
    'B0-B1': '1-1-2',
    'B0-B2': '1-2-1',
    'B1-B2': '1-2-2',
    '-': '1-2-3',
}

def connected(s1, s2):
    if s1[0] == 'B' or s1[0] == '-':
        s1 = backwardCompatibleConnections[s1]
    if s2[0] == 'B' or s2[0] == '-':
        s2 = backwardCompatibleConnections[s2]
    # Check that all colors can be connected:
    c1 = s1.split('-')
    c2 = s2.split('-')
    color1 = set([0])
    for i in range(1, len(c1)):
        if c1[i] == c1[0]:
            color1.add(i)
        elif c2[i] == c2[0]:
            color1.add(i)
    improved = True
    while improved:
        improved = False
        for i in range(1, len(c1)):
            if i in color1:
                continue
            for j in range(0, len(c1)):
                if j in color1 and (c1[j] == c1[i] or c2[j] == c2[i]):
                    color1.add(i)
                    improved = True
                    break
    return len(color1) == len(c1)

def countUp(token1, token2, token, bricks):
    cache1 = CACHE[token1][bricks]
    cache2 = CACHE[token2][bricks]
    #print('Count up', token1, token2, token, bricks, len(cache1), len(cache2))

    retA = 0
    retS = 0
    bs = False
    for r1 in cache1:
        #print('Counting up for', r1)
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

            all = a1 * a2
            symmetric = s1 * s2
            #if symmetric > 0:
            #    print('   Adding symmtries for', r1, r2, all, symmetric)
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
        print(' Handling token', token1)
        size1 = sum(int(x) for x in token1)
        if size1 == left + base:
            for token2 in CACHE:
                size2 = sum(int(x) for x in token2)
                if size2 == right + base:
                    handle(token1, token2)
handleAllTokens()

print()
for token in countsAll:
    a = countsAll[token]
    s = countsSymmetric[token]
    print(' <'+token+'>', a, '(' + str(s) + ')')
print()

def isSelfConnected(connectivity):
    return connected(connectivity, connectivity)

# Single sided sum for cross check:
for token in CACHE:
    crossCheckA = 0
    crossCheckS = 0
    for bricks in CACHE[token]:
        for report in CACHE[token][bricks]:
            if isSelfConnected(report.connectivity):
                crossCheckA = crossCheckA + (report.total if not report.baseSymmetric else (report.total-report.symmetric)/2+report.symmetric)
                crossCheckS = crossCheckS + report.symmetric
                #if report.symmetric > 0:
                #    print('Symmetric and self connected:', report)
    print('  Cross check', token, ':', int(crossCheckA), '(' + str(crossCheckS) + ')')

# Testing
# <31> 648 (8)
# <131> 433685 (24)
# <32> 148794 (443)
# <231> 41019966 (1179)
# <232> 3021093957 (46219)
# <33> 6246077 (432)
# <331> 1358812234 (1104)
# <1321> 4581373745 (1471)

# <43> 106461697 (10551)
