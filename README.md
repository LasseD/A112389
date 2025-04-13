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
Given that there currently is no known way of computing the sequence without iterating through (most of) the models, the time to compute a(N+1) is roughly 100 times the time to compute a(N).
As we might not be able to reserve computing resources for 150 years, we seek to find novel ways of computing subsets of a(11) more efficiently and reduce the amount of models, that have to be counted individually, to something more reasonable.

## Definitions

TODO


## Brute-Force Approach

The folder [/brute_force_approach](brute_force_approach/) contains the source code for a "brute force" algorithm.

The brute force algorithm constructs all models of a(N) by iterating through all models of a(N-1) and adding a single brick at every possible location.

Each constructed model is checked for being the first of its kind being computed. Only the first models of their kind are counted, and optionally written to disk.

This is a very simplistic and inefficient approach that has the advantage of being able to write all models to disk for visualization and having a baseline of comparison for more efficient algorithms.


## "BFS Wave" Approach

Consider the following by Abrahamsen and Eilers:
Consider the first brick at the first layer of any model as being fixed.
Construct all models by adding bricks to this first brick.
Divide the final counts as outlined in 'Eilers 2016' to address symmetries.

Our "BFS Wave" algorithm constructs models iteratively by following this approach.

Consider a model M (initially the single brick on the first layer), as well as a "wave" initially consisting of the single brick as well.

Let S be the set of bricks that can be connected to the bricks of the last wave while not connecting to any previous bricks of M.

New models are constructed by picking a subset T from S, placing the bricks of T on M and applying the algorithm iteratively with the lastest "wave" consisting of T.

This is repeated for all possible subsets T while ensuring that the bricks of T do not overlap with themselves.

Our implementation had the selection of T behave like how the iteration works in BFS tree traversal. Our latest implementation uses DFS traversal, but we have kept the name "BFS Wave" to refer back to the initial conception.

The folder [/bfs_wave_approach](bfs_wave_approach/) contains the source code.


## Lemma 1

Lemma “Divide and Conquer” (L1)
The use of “bottlenecks” (layers with a single brick) allows us to easily compute the number of models of a refinement given the number of models of smaller bricks and heights (sub-refinements). Let b = a(X,m,Z1,...,Zn,...Zm), Zn=1, n>1, n<m be a refinement where Zn=1 is a bottleneck indicating that there is only a single brick in layer n.
The models of b can be divided at the bottleneck with the models below (and including the bottleneck) being the refinement c = a(Xc,n,Z1,...,Zn), and above (also including the bottleneck) is the refinement d = a(Xd,m-n+1,Zn,...,Zm). We also call c and d the “sub-refinements” of a.
We again use the subfices “s” and “n” to denote subsets with symmetric and non-symmetric models.
All models of b can be constructed by combining models either by
cn and dn to create non-symmetric models: 2 models for each pair.
cs and dn to create non-symmetric models: 1 model for each pair.
cn and ds to create non-symmetric models: 1 model for each pair.
cs and cs to create symmetric models: 1 model for each pair.

A(X,Y,Z1,...,Zn,...Zm)
= 2*|cn|*|dn| + |cs|*|dn| + |cn|*|ds| + |cs|*|ds|
= (|cn|+|c|)*|dn| + |c|*|ds|
= (|dn|+|d|)*|cn| + |d|*|cs|
Of which |cs|*|ds| are symmetric
Corollary C1
Applying L1 recursively reveals that any refinement with at least one bottleneck can be computed from the sub-refinements that appear from splitting at the bottlenecks.
