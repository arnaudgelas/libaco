# Introduction #

libaco is a library which can be used for solving combinatorial optimization problems using the Ant Colony Optimization (ACO) meta-heuristic. The library implements the following variants of ACO algorithms:

  * Simple Ant System
  * Elitist Ant System
  * Rank-Based Ant System
  * Max-Min Ant System
  * Ant Colony System

For detailed descriptions of these algorithms take a look at [Ant Colony Optimization](http://books.google.at/books?id=_aefcpY8GiEC&hl=en) by Marco Dorigo and Thomas Stuetzle.

The project repository contains the following:

  * libaco, the library itself.
  * acotsp, example code applying libaco to the Travelling Salesman Problem.
  * acotreewidth, example code applying libaco to the problem of finding [Tree Decompositions](http://en.wikipedia.org/wiki/Tree_Decomposition) of small width.
  * acotemplate, a template project including a command line client.
  * liblocalsearch. a library implementing some basic local search algorithms and neighbourhoods (Hill Climbing, Iterative Local Search, 2-opt Neighbourhood)

All this was implemented in the course of my [master's thesis](http://libaco.googlecode.com/files/thesis.pdf) entitled "Ant Colony Optimization for Tree and Hypertree Decompositions" at the Vienna University of Technology.

# Documentation #

  * Section 6.2 of my [master's thesis](http://libaco.googlecode.com/files/thesis.pdf) gives an introduction to the libaco library and the acotsp application.

# External Links #

  * [Ant Colony Optimization](http://books.google.at/books?id=_aefcpY8GiEC&dq=Ant+Colony+Optimization), Marco Dorigo and Thomas Stuetzle, 2004
  * [The Ant System - Optimization by a colony of cooperating agents](http://citeseer.ist.psu.edu/dorigo96ant.html), Marco Dorigo and Thomas Stuetzle, 1996