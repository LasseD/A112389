# Wave Approach to Compute A112389

This is a code base for computing the numbers of OEIS A112389 using our "wave" approach

## How to run

Compile the code:

```
g++ -std=c++11 -O3 -DNDEBUG *.cpp -o run.o
```

### Count for a specific refinement <R> using T threads

```
./run.o R T
```

Example:
a(8,3,4,2,2) with the shorthand <422>. Here a refinement is the subset of models with a specific number of bricks in the layers: <422> indicates models with 8 bricks of height 3 (3 layers) with 4 bricks in the lower-most layer and 2 bricks in each of the other two layers.

```
./run.o 422 9
```

### Construct precomputation files up to maximal distance D for base B, refinement <R> using T threads

```
./run.o R D T
```

Example:
For the base 2 the models of refinement <21> will be placed in the directory /base_2_size_3_refinement_21/
To compute the files up to maximal distance 8 between the bricks in the base and using 9 threads, run:


```
mkdir base_2_size_3_refinement_21

./run.o P 21 8 9
```

### Sum precomputation files up to maximal distance D for base B, for refinement <ABC>

```
./run.o S A B C D
```

Example:
After computing the base 2 precomputations of refinement <21> up to distance 8, the refinement <121> can be computed by running:


```
./run.o S 1 2 1 8
```

The code is in public domain, and you may copy and add to it as you see fit.


## Code overview

The code base is structured in 3 files:

- main.cpp which performs command line parameter validation and executes the requested function

- rectilinear.* files which have the actual implementation for computing refinements and precomputations

See the comments in top of the structs and classes of rectilinear.h for brief overviews.
