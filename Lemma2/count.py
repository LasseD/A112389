import sys
import re
from pathlib import Path

CACHE = {} # Symmetric refinements: refinement -> [connected,cross,dx,dy,total,symmetric]
CACHE['2'] = []

def fetch(n):
    if n == 2:
        return # Nothing to fetch
    dir = '../bfs_wave_approach/base_2_size_' + str(n)
    print(' Fetching data for size', n, 'from folder', dir)
    for file in Path(dir).iterdir():
        if not file.is_file():
            continue
        name = re.split(r"[_\.]", file.name)
        #print('file', name)
        assert len(name) == 6
        cross = name[0] == 'crossing'
        assert name[1] == 'dx'
        dx = int(name[2])
        assert name[3] == 'dy'
        dy = int(name[4])
        assert name[5] == 'txt'
        content = open(file)

        while True:
            line = content.readline()
            if not line:
                break
            tokens = line.split()
            assert len(tokens) == 6
            connected = tokens[0] == 'CONNECTED'
            refinement = tokens[1]
            assert tokens[2] == 'TOTAL'
            total = int(tokens[3])
            assert tokens[4] == 'SYMMETRIC'
            symmetric = int(tokens[5])
            #print(cross, dx, dy, ':', tokens)
            if not refinement in CACHE:
                CACHE[refinement] = []
            CACHE[refinement].append((connected,cross,dx,dy,total,symmetric))

left = int(sys.argv[2])
size = int(sys.argv[1])
right = size - left - 2
print('Refinements of size', size, 'with', left, 'on the left and', right, 'on the right')
print()

fetch(left+2)
if left != right:
    fetch(right+2)

countsAll = {}
countsSymmetric = {}

def last(token, cross, dx, dy):
    if token == '2':
        symmetric = 1 if not cross else 0
        return [(True, 0, 0), (False, 1, symmetric)]
    dys = [x[3] for x in CACHE[token] if x[1] == cross and x[2] == 0]
    dy = max(dys)
    return [(x[0],x[4],x[5]) for x in CACHE[token] if x[1] == cross and x[2] == 0 and x[3] == dy]

def countUp(token1, token2, token, cross, dx, dy):
    cache1 = [(x[0],x[4],x[5]) for x in CACHE[token1] if x[1] == cross and x[2] == dx and x[3] == dy]
    cache2 = [(x[0],x[4],x[5]) for x in CACHE[token2] if x[1] == cross and x[2] == dx and x[3] == dy]
    if len(cache1) == 0 and len(cache2) == 0:
        return
    if len(cache1) == 0:
        cache1 = last(token1, cross, dx, dy)
    if len(cache2) == 0:
        cache2 = last(token2, cross, dx, dy)
    #print('token1 =', token1, 'token2 =',token2, '-> token =', token, 'cross =', cross, dx, dy)
    for (connected1,all1,symmetric1) in cache1:
        for (connected2,all2,symmetric2) in cache2:
            if (not connected1) and (not connected2):
                continue # Nothing connects!

            all = all1 * all2
            symmetric = symmetric1 * symmetric2
            all = all + symmetric
            symmetric = 2 * symmetric

            if dx != 0 and dy != 0:
                # Include mirror symmetries:
                all = 2 * all
                symmetric = 2 * symmetric
            
            #print(' Adding', all, symmetric, 'for cross:', cross, 'dx =', dx, 'dy =', dy,'all1 =', all1, 'all2 =', all2, 'symmetric1 =', symmetric1, 'symmetric2 =',symmetric2,'connected1 =',connected1,'connected2 =',connected2)
            countsAll[token] = countsAll[token] + all
            countsSymmetric[token] = countsSymmetric[token] + symmetric

def handle(token1, token2):
    #print('handle', token1, token2)
    token = token1[::-1] + token2[1::]
    if not token in countsAll:
        countsAll[token] = 0
        countsSymmetric[token] = 0
    dxs = set([x[2] for x in CACHE[token1]]) | set([x[2] for x in CACHE[token2]])
    dys = set([x[3] for x in CACHE[token1]]) | set([x[3] for x in CACHE[token2]])
    for cross in [True,False]:
        for dx in dxs:
            for dy in dys:
                countUp(token1, token2, token, cross, dx, dy)

for token1 in CACHE:
    size1 = sum(int(x) for x in token1)
    if size1 != left + 2:
        continue
    
    for token2 in CACHE:
        size2 = sum(int(x) for x in token2)
        if size2 != right + 2:
            continue
        handle(token1, token2)

print()
for token in countsAll:
    s = countsSymmetric[token]
    if s == 0:
        print(' <'+token+'>', countsAll[token])
    else:
        print(' <'+token+'>', int(countsAll[token]/2), '(' + str(int(s/2)) + ')')
print()
