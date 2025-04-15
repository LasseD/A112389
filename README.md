# Algorithms for OEIS A112389

[OEIS A112389](https://oeis.org/A112389) is the number of ways, counted up to symmetry, to build a contiguous model with n LEGO bricks of size 2 X 4.

The following table contains the known values of the sequence a(N). a<sup>180</sup>(N) are those of a(N) that are symmetric at 180 degrees of rotation. This sequence is also known as [OEIS A123829](https://oeis.org/A123829).

| n  | a(N)              | a<sup>180</sup>(N)| Author(s)     |
|:--:|------------------:|---------:|:-----------------------|
|  1 |                 1 |        1 |                        |
|  2 |                24 |        2 | Kristiansen 1974       |
|  3 |              1560 |       44 | Anonymous 2002         |
|  4 |            119580 |      185 | Eilers 2004            |
|  5 |          10166403 |     3276 | Eilers 2004            |
|  6 |         915103765 |    15682 | Eilers 2004            |
|  7 |       85747377755 |   282377 | Abrahamsen-Eilers 2006 |
|  8 |     8274075616387 |  1480410 | Abrahamsen-Eilers 2006 |
|  9 |   816630819554486 | 26264942 | Nilsson 2014           |
| 10 | 82052796578652749 |          | Simon 2018             |
| 11 |                   |          |                        |



# Project Overview

It took "a matter of months" to compute a(9) and more than 1.5 years to compute a(10).
Since there currently is no known way of computing the sequence without iterating through (most of) the models, the time to compute a(N+1) is roughly 100 times the time to compute a(N).
As 150 years of computing resources is a high price to pay for a(11), we seek to find novel ways of computing subsets of a(11) more efficiently and reduce the amount of models that have to be counted individually to something more reasonable.

## Definitions


- A "brick" refers to a 2 x 4 LEGO brick with 8 stod on top. A brick has a position, and since we only concerns ourselves with bricks that are connected to each other using the studs, and only at rectillinear ("right") angles, the position of a brick is identified by its coordinate pair (x,y) and its orientation (horizontal or vertical). A brick also has a height. A "base brick" is located at (0,0) at height 0 and a brick connected to another from above has the height 1 larger than the other.


- A "model" consists of one or more bricks connected by the studs. All connections are rectilinear. [Another project](https://github.com/LasseD/BrickCounting) concerns bricks connected at all possible angles. We consider two models to be the same if they are rotationally symmetric. The code base uses "combination" and "model" interchangeably.


- A(N) is the set of all models with exactly N bricks.


- a(N) is the size of A(N).


- A<sup>180</sup>(N) is the subset of A(N) where the models are symmetric after 180 degrees of rotation. a<sup>180</sup>(N) is the size of A<sup>180</sup>(N). Similarly A<sup>90</sup>(N) is the subset of A(N) where the models are symmetric after 90 degrees of rotation, and a<sup>90</sup>(N) is the size of A<sup>90</sup>(N).


- A "layer" of a model is the subset of bricks of the model whose height is the same. The "base layer" or "first layer" refers to the lower-most layer of a model (at height 0).


- The "height" of a model is the number of layers of it.


- A "refinement" A(N,M,Z1,Z2,...,ZM) is the subset of a(N) where the models have height M and the sized of the layers from base layer and up are Z1, Z2, ..., ZM. We use the shorthand <Z1Z2...ZM> to denote the refinement A(N,M,Z1,Z2,...,ZM). a(N,M,Z1,Z2,...,ZM) is the size of refinement A(N,M,Z1,Z2,...,ZM).


## Brute-Force Approach

The folder [/brute_force_approach](brute_force_approach/) contains the source code for a "brute force" algorithm.

The brute force algorithm constructs A(N) by iterating through A(N-1) and adding a single brick at every possible location of every model.

Each constructed model is checked for being the first of its kind being computed. Only the first models of their kind are counted, and optionally written to disk.

This is a very simplistic and inefficient approach that iterates through all models, but has the advantage of being able to write all models to disk for visualization and providing a baseline of comparison for more efficient algorithms.


## "BFS Wave" Approach

Consider the following by Abrahamsen and Eilers:
Consider the first brick at the first layer of any model as being fixed.
Construct all models by adding bricks to this first brick.
Divide the final counts as outlined in 'Eilers 2016' to address symmetries.

Our "BFS Wave" algorithm constructs models iteratively by following this approach.

Consider a model M (initially the single brick on the first layer), as well as a "wave" being the set initially consisting of the single brick.

Let S be the set of bricks that can be connected to the bricks of the wave while not connecting to any previous bricks of M.

New models are constructed by picking a subset T from S, placing the bricks of T on M and applying the algorithm iteratively with the "wave" being T.

This is repeated for all possible subsets T while ensuring that the bricks of T do not overlap with themselves.

Our implementation had the selection of T behave like how the iteration works in BFS tree traversal. Our latest implementation uses DFS traversal, but we have kept the name "BFS Wave" to refer back to the initial conception.

The folder [/bfs_wave_approach](bfs_wave_approach/) contains the source code.


## Lemma 1 “Divide and Conquer” (L1)

We call a layer of size 1 a “bottleneck”.

Bottlenecks allow us to easily compute the number of models of a models from the size of smaller refinements:

Let B = A(N,M,Z1,...,Zn,...ZM), Zn=1, 1<n<M be a refinement with the bottleneck Zn.

Consider the refinements obtained by dividing B at the bottleneck:

C = A(N_C,n,Z1,...,Zn) and D = A(N_D,m-n+1,Zn,...,ZM)

TODO
All models of B can be constructed by combining models either by
cn and dn to create non-symmetric models: 2 models for each pair.
cs and dn to create non-symmetric models: 1 model for each pair.
cn and ds to create non-symmetric models: 1 model for each pair.
cs and cs to create symmetric models: 1 model for each pair.

A(X,Y,Z1,...,Zn,...Zm)
= 2*|cn|*|dn| + |cs|*|dn| + |cn|*|ds| + |cs|*|ds|
= (|cn|+|c|)*|dn| + |c|*|ds|
= (|dn|+|d|)*|cn| + |d|*|cs|
Of which |cs|*|ds| are symmetric

Applying L1 recursively reveals that any refinement with at least one bottleneck can be computed from the sub-refinements that appear from splitting at the bottlenecks.

TODO: Lemma1 code uses non-bottlenecks to compute all