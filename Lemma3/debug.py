import sys
import re
from pathlib import Path

class Report:
    def __init__(self, connectivity, baseSymmetric180, baseSymmetric90, total, symmetric180, symmetric90):
        assert(type(connectivity) is str)
        assert(type(baseSymmetric180) is bool)
        assert(type(baseSymmetric90) is bool)
        assert(type(total) is int)
        assert(type(symmetric180) is int)
        assert(type(symmetric90) is int)
        self.connectivity = connectivity
        self.baseSymmetric180 = baseSymmetric180
        self.baseSymmetric90 = baseSymmetric90
        self.total = total
        self.symmetric180 = symmetric180
        self.symmetric90 = symmetric90
    def __repr__(self):
        if self.symmetric90 > 0:
            return f"{self.connectivity} {self.baseSymmetric180}/{self.baseSymmetric90} : {self.total} ({self.symmetric180}) (({self.symmetric90}))"
        elif self.symmetric180 > 0:
            return f"{self.connectivity} {self.baseSymmetric180}/{self.baseSymmetric90} : {self.total} ({self.symmetric180})"
        else:
            return f"{self.connectivity} {self.baseSymmetric180}/{self.baseSymmetric90} : {self.total}"

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
    def nextBatch(self):
        ret = []
        bs180 = bool(self.readBit())
        bs90 = base % 4 == 0 and bool(self.readBit())
        first = True
        while True:
            if not first:
                indicator = self.readBit()
                if(indicator == 1):
                    return ret
            first = False
            colors = [0]
            for i in range(self.base-1):
                colors.append(self.readColor())
            connectivity = '-'.join([str(x) for x in colors])
            #token = str(self.readUInt(8))[::-1]
            total = self.readUInt(32)
            symmetric180 = self.readUInt(16)
            symmetric90 = 0
            if base % 4 == 0:
                symmetric90 = self.readUInt(8)
            report = Report(connectivity, bs180, bs90, total, symmetric180, symmetric90)
            if total == 0:
                # TODO: Read final counts!
                return [] # Done
            ret.append(report)

token = int(sys.argv[1])
D = int(sys.argv[2])
base =  int(sys.argv[1][0])
size = sum([int(x) for x in sys.argv[1]])
print()
print(' Comparing files for base', base, 'size', size, 'refinement:', token, 'for distance', D)

def openStream(old):
    name = '../bfs_wave_approach/base_' + str(base) + '_size_' + str(size) + '_refinement_' + str(token) + '/'
    if old:
        name = name + 'old_'
    name = name + 'd' + str(D) + '.bin'
    print('Reading file', name)
    binFile = Path(name)
    assert(binFile.is_file())
    stream = open(binFile, 'rb')
    reader = BitReader(stream, base)
    assert(1 == reader.readBit())
    return reader

reader1 = openStream(False)
reader2 = openStream(True)

cnt = 0
while True:
    reports1 = reader1.nextBatch()
    reports2 = reader2.nextBatch()    
    
    if len(reports1) != len(reports2):
        print('Report size mismatch!', len(reports1), len(reports2))
        continue

    if len(reports1) == 0:
        break
    
    connectivities1 = set([r.connectivity for r in reports1])
    connectivities2 = set([r.connectivity for r in reports2])
    
    if connectivities1 != connectivities2:
        print('Connectivity set mismatch!', connectivities1, connectivities2)
        continue

    for connectivity in connectivities1:
        r1 = [r for r in reports1 if r.connectivity == connectivity][0]
        r2 = [r for r in reports2 if r.connectivity == connectivity][0]
        ok = True
        if r1.baseSymmetric180 != r2.baseSymmetric180:
            ok = False
        elif r1.baseSymmetric90 != r2.baseSymmetric90:
            ok = False
        elif r1.total != r2.total:
            ok = False
        elif r1.symmetric180 != r2.symmetric180:
            ok = False
        elif r1.symmetric90 != r2.symmetric90:
            ok = False
        if not ok:
            print('Mismatch')
            print(' ', r1)
            print(' ', r2)
    cnt = cnt + 1
print(cnt, 'report batches read')
