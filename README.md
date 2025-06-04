# Algorithms for counting models of 2 x 4 LEGO bricks

## Results

### OEIS A112389

[OEIS A112389](https://oeis.org/A112389) is the number of ways, counted up to symmetry, to build a contiguous model with n LEGO bricks of size 2 X 4. The following table contains the known values of this sequence, which we denote a(N).

| N  | a(N)              | Author(s)     |
|:--:|------------------:|:-----------------------|
|  1 |                 1 |                        |
|  2 |                24 | Kristiansen 1974       |
|  3 |              1560 | Anonymous 2002         |
|  4 |            119580 | Eilers 2004            |
|  5 |          10166403 | Eilers 2004            |
|  6 |         915103765 | Eilers 2004            |
|  7 |       85747377755 | Abrahamsen-Eilers 2006 |
|  8 |     8274075616387 | Abrahamsen-Eilers 2006 |
|  9 |   816630819554486 | Nilsson 2014           |
| 10 | 82052796578652749 | Simon 2018             |


### OEIS A123829

[OEIS A123829](https://oeis.org/A123829) is the number of ways, counted up to symmetry, to build a contiguous model with n LEGO bricks of size 2 X 4 which is **symmetric** after a rotation by 180 degrees. The following table contains the known values of this sequence, which we denote a<sup>180</sup>(N).

| N  | a<sup>180</sup>(N)| Author(s)     |
|:--:|---------:|:-----------------------|
|  1 |        1 |                        |
|  2 |        2 | Kristiansen 1974       |
|  3 |       44 | Anonymous 2002         |
|  4 |      185 | Eilers 2004            |
|  5 |     3276 | Eilers 2004            |
|  6 |    15682 | Eilers 2004            |
|  7 |   282377 | Abrahamsen-Eilers 2006 |
|  8 |  1480410 | Abrahamsen-Eilers 2006 |
|  9 | 26264942 | Nilsson 2014           |


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


## Wave Approach

Consider the following by Abrahamsen and Eilers:
Consider the first brick at the first layer of any model as being fixed.
Construct all models by adding bricks to this first brick.
Divide the final counts as outlined in 'Eilers 2016' to address symmetries.

Our "wave" algorithm constructs models iteratively by following this approach.

Consider a model M (initially the single brick on the first layer), as well as a "wave" being the set initially consisting of the single brick.

Let S be the set of bricks that can be connected to the bricks of the wave while not connecting to any previous bricks of M.

New models are constructed by picking a subset T from S, placing the bricks of T on M and applying the algorithm iteratively with the "wave" being T.

This is repeated for all possible subsets T while ensuring that the bricks of T do not overlap with themselves.

The folder [/wave_approach](wave_approach/) contains the source code.


## Lemma 1 “Divide and Conquer” (L1)

We call a layer of size 1 a “bottleneck”.

If a refinement has a bottleneck, then we can easily compute its size from the size of smaller refinements:

Let B = A(N,M,Z1,...,Zn,...ZM), Zn=1, 1<n<M be a refinement with the bottleneck Zn.

Consider the refinements obtained by dividing B at the bottleneck:

C = A(N_C,n,Z1,...,Zn) and D = A(N_D,m-n+1,Zn,...,ZM)

All models in B can be constructed by combining all models from C with those from D.
We have to pay attention for symmetries.
Consider a non-symmetric model c from C. The models in B that can be constructed from c is 2 |D| minus the symmetric models in D, as they would otherwise be double-counted. The multiplier "2" comes from the fact that all non-symmetric models can be rotated 180 degrees at the bottleneck.

If c is symmetric, then all models of D can be used once to construct models in B.
By using the 180-superfix to denote rotationally symmetric models and "-" to denote models that are not, this can be written as following:


|B<sup>-</sup>| = 2 |C<sup>-</sup>| |D<sup>-</sup>| + |C<sup>-</sup>| |D<sup>180</sup>| + |D<sup>-</sup>| |C<sup>180</sup>|


|B<sup>180</sup>| = |C<sup>180</sup>| |D<sup>180</sup>|


|B| = |B<sup>-</sup>| + |B<sup>180</sup>|


L1 thus provides us with an efficient method of computing the size of all refinements that have at least one bottleneck, should the size of smaller refinements be at hand.


### Applying L1 to compute A112389

The method from L1 is used in the python script "sum-for-size.py" where results for refinements without bottlenecks are being typed in, thus allowing you to compute the numbers a(N) and a<sup>180</sup>(N). As an example, compute the numbers for six bricks by typing:

```
python sum-for-size.py 6
```

The results will be printed on a line with "TOTAL" followed by a(N), followed by a<sup>180</sup>(N) in parenthesis.


## Lemma 2 "Two Brick Base" L2

TODO


## Lemma 3 "Generalization to all Bases" L3

### Running Time

Consider the precomputations for base B up to maximal distance D where models of size n are built for each base.

(8D)^B  *  B * O(A(n)) = (8D)^B  *  B * 100^n