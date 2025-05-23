# Wave Approach to Compute A112389

This is a code base for computing the numbers of OEIS A112389 using our "wave" approach

## How to run

Compile the code:

```
g++ -std=c++11 -O3 -DNDEBUG *.cpp -o run.o
```

### Count for a specific refinement R using T threads

```
./run.o R T
```

Example:
a(8,3,4,2,2) with the shorthand <422>. Here a refinement is the subset of models with a specific number of bricks in the layers: <422> indicates models with 8 bricks of height 3 (3 layers) with 4 bricks in the lower-most layer and 2 bricks in each of the other two layers.

```
./run.o 422 9
```

### Construct precomputation files up to maximal distance D for base B, refinement R using T threads

```
./run.o R D T
```

Example:
For the base 2 the models of refinement <21> will be placed in the directory /base_2_size_3_refinement_21/
To compute the files up to maximal distance 8 between the bricks in the base and using 9 threads, run:


```
mkdir base_2_size_3_refinement_21

./run.o 21 8 9
```

The code is in public domain, and you may copy and add to it as you see fit.


## Brief code overview

The code computes all models a(N) by adding bricks to existing models in "waves". A "wave" of a model is a subset of the bricks of the model. See the source code for further documentation.

### Base case

a(1) consists of the single model consisting of one 2x4 LEGO brick. The "wave" of the model consists of the brick.

This model is symmetric, so a(N) = a<sup>180</sup>(N) = 1.

### Iterative case

Consider a model M0 and its wave W0.

New models are created by selecting bricks for new waves:

Let Q be the set of possible brick locations where each brick connects to W0, but does not connect to any other brick of M0 and does not overlap with any brick in M0.

New models are created by selecting all non-empty subset of Q as the new waves.



