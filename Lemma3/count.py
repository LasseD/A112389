import sys
import re
from pathlib import Path

class Report:
    def __init__(self, connectivity, baseSymmetric, total, symmetric):
        assert(type(connectivity) is str)
        assert(type(baseSymmetric) is bool)
        assert(type(total) is int)
        assert(type(symmetric) is int)
        self.connectivity = connectivity
        self.baseSymmetric = baseSymmetric
        self.total = total
        self.symmetric = symmetric
    def __repr__(self):
        if self.symmetric > 0:
            return f"{self.connectivity} base symmetric: {self.baseSymmetric}. {self.total} ({self.symmetric})"
        else:
            return f"{self.connectivity} base symmetric: {self.baseSymmetric}. {self.total}"

class BitReader:
    def __init__(self, stream, base):
        self.stream = stream
        self.bits = 0
        self.bitIdx = 8
        self.base = base
    def readBit(self):
        if self.bitIdx == 8:
            self.bits = self.stream.read(1)[0]
            self.bitIdx = 0
        bit = (self.bits >> (7-self.bitIdx)) & 1
        self.bitIdx = self.bitIdx + 1
        return bit
    def readUInt(self, size):
        ret = 0
        for i in range(size):
            ret = ret | (self.readBit() << i)
        return ret
    def readColor(self):
        ret = 0
        for i in range(3):
            ret = ret | (self.readBit() << i)
        return ret
    def nextBatch(self, CACHE):
        if not self.stream:
            return True
        bs = bool(self.readBit())
        first = True
        while True:
            if not first:
                indicator = self.readBit()
                if(indicator == 1):
                    return True # Next batch
            first = False
            colors = [0]
            for i in range(self.base-1):
                colors.append(self.readColor())
            connectivity = '-'.join([str(x) for x in colors])
            token = str(self.readUInt(8))
            total = self.readUInt(32)
            symmetric180 = self.readUInt(16)
            if base == 4:
                symmetric90 = self.readUInt(8)
            report = Report(connectivity, bs, total, symmetric180)
            if token == "0":
                # TODO: Read final counts!
                return False # Done
            if not token in CACHE:
                CACHE[token] = []
            CACHE[token].append(report)

left = int(sys.argv[1])
base = int(sys.argv[2])
right = int(sys.argv[3])
size = left + base + right
maxDist = int(sys.argv[4])
print()
print(' Tokens of size', size, 'left-base-right:', left, base, right, 'for distance 2 to', maxDist)

def openStream(n, size):
    print('  Fetching data for base', base, 'size', n, 'distance', size)
    name = '../bfs_wave_approach/base_' + str(base) + '_size_' + str(n) + '/d' + str(size) + '.'
    binFile = Path(name + 'bin')
    if binFile.is_file():
        stream = open(binFile, 'rb')
        reader = BitReader(stream, base)
        assert(1 == reader.readBit())
        return reader

def connected(s1, s2):
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

def countUp(r1, r2):
    if not connected(r1.connectivity, r2.connectivity):
        return (0, 0)
    a1 = r1.total
    s1 = r1.symmetric
    a2 = r2.total
    s2 = r2.symmetric
    all = a1 * a2
    symmetric = s1 * s2
    return (all, symmetric)

countsAll = {}
countsSymmetric = {}

for D in range(2, maxDist+1):
    # Read files and handle batches one by one:
    reader1 = openStream(left+base, D)
    reader2 = openStream(right+base, D) if left != right else False
    CACHE = {} # token -> [Report...]

    while reader1.nextBatch(CACHE) and ((not reader2) or reader2.nextBatch(CACHE)):
        bs = False
        for token1 in CACHE:
            for token2 in CACHE:
                A = 0
                S = 0
                for report1 in CACHE[token1]:
                    bs = report1.baseSymmetric
                    for report2 in CACHE[token2]:
                        (a, s) = countUp(report1, report2)
                        A = A + a
                        S = S + s
                if bs:
                    assert((A - S)%2 == 0)
                    A = int((A - S)/2) + S
                token = token1[::-1] + str(base) + token2
                if not token in countsAll:
                    countsAll[token] = 0
                    countsSymmetric[token] = 0
                countsAll[token] = countsAll[token] + A
                countsSymmetric[token] = countsSymmetric[token] + S
            # Single sided counts for cross checking:
            A = 0
            S = 0
            for report in CACHE[token1]:
                if connected(report.connectivity, report.connectivity):
                    A = A + report.total
                    S = S + report.symmetric
            if bs:
                A = int((A - S)/2) + S
            token = str(base) + token1
            if not token in countsAll:
                countsAll[token] = 0
                countsSymmetric[token] = 0
            countsAll[token] = countsAll[token] + A
            countsSymmetric[token] = countsSymmetric[token] + S
        CACHE = {} # Reset cache

print()
for token in countsAll:
    a = countsAll[token]
    s = countsSymmetric[token]
    print(' <'+token+'>', a, '(' + str(s) + ')')
