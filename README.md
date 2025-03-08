# Algorithms for OEIS A112389


OEIS A112389 is "Number of ways, counted up to symmetry, to build a contiguous building with n LEGO blocks of size 2 X 4."

https://oeis.org/A112389

Known values of the sequence a(N). a~sym~(N) are those of a(N) that are symmetric at 180 degrees of rotation.

| n  | a(N)              | a~sym~(N)| Author(s)              | 
|:--:|------------------:|--------:|:-----------------------|
|  1 |                 1 |       1 |                        |
|  2 |                24 |       2 | Kristiansen 1974       |
|  3 |              1560 |      44 | Anonymous 2002         |
|  4 |            119580 |     185 | Eilers 2004            |
|  5 |          10166403 |    3276 | Eilers 2004            |
|  6 |         915103765 |   15682 | Eilers 2004            |
|  7 |       85747377755 |  282377 | Abrahamsen-Eilers 2006 |
|  8 |     8274075616387 | 1480410 | Abrahamsen-Eilers 2006 |
|  9 |   816630819554486 |         | Nilsson 2014           |
| 10 | 82052796578652749 |         | Simon 2018             |
| 11 |                   |         |                        |



# Overview

This repository is divided into different approaches to computing OEIS A112389. Each approach is contained in a sub-folder.


## Brute-Force Approach

The folder /brute_force_approach contains the source code for the brute force algorithm.

The "brute force" algorithm constructs a(N) from a(N-1) by iterating over all models of a(N-1) and adding 1 brick at every location.

Each constructed model is checked for being the first of its kind being computed. Only the first models of their kind are counted, and optionally written to disk.

This is a very simplistic and inefficient approach that has the advantage of being able to write all models to disk.


## "BFS Wave" Approach

This approach uses the framework of computation presented by Abrahamsen and Eilers:
Consider the first brick at the first layer as being fixed and construct all models onto this first brick.
Divide the final counts as outlined in Eilers 2016.

The approach constructs models iteratively.

Consider a model M (initially the single brick on the first layer), as well as a "wave" initially consisting of the single brick as well.

A new model is constructed by picking the next wave from the set of bricks that connect to the existing wave, while not connecting to any other bricks of the model.

Waves are iteratively constructed until all models have been exhautively counted.

This approach has the waved behaving like the classic BFS approach of graph traversal, hence the name "BFS Wave".

The folder /bfs_wave_approach contains the source code for this approach.