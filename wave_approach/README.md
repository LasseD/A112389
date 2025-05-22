# "BFS Wave" Approach

This is a code base for computing the numbers of OEIS A112389 using the "BFS Wave" approach

## How to run

Compile the code:

```
g++ -std=c++11 -O3 -DNDEBUG *.cpp -o run.o
```

Count a(N), a<sup>180</sup>(N), and a<sup>90</sup>(N) for N = 2, 3, ...:

```
./run.o N
```

Count for a specific refinement, such as a(8,3,4,2,2) with the shorthand <422>. Here a refinement is the subset of models with a specific number of bricks in the layers: <422> indicates models with 8 bricks of height 3 (3 layers) with 4 bricks in the lower-most layer and 2 bricks in each of the other two layers.

```
g++ -std=c++11 -O3 -DREFINEMENT -DNDEBUG *.cpp -o run.o

./run.o 8 422
```

The code is in public domain, and you may copy and add to it as you see fit.


## Code Overview

The code computes all models a(N) by adding bricks to existing models in "waves". A "wave" of a model is a subset of the bricks of the model.

### Base Case

a(1) consists of the single model consisting of one 2x4 LEGO brick. The "wave" of the model consists of the brick.

This model is symmetric, so a(N) = a<sup>180</sup>(N) = 1.

### Iterative Case

Consider a model M0 and its wave W0.

New models are created by selecting bricks for new waves:

Let Q be the set of possible brick locations where each brick connects to W0, but does not connect to any other brick of M0 and does not overlap with any brick in M0.

New models are created by selecting all non-empty subset of Q as the new waves.

### Counting

TODO

### Proof of correctness

It must be proven that the "BFS Wave" approach correctly computes A112389.

#### Proof that all models of A112389 are constructed and thus counted.

TODO

#### Proof that the computation of a(N) is correct using refinements

TODO

#### Proof that the computation of a(N) is correct using the appraoch by Eilers (2016)

TODO




