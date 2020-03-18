# CMEProcessor

This is a pair of tools that reconstruct the limit order book from an arbitrary CME MBO FIX message stream and then produce summary statistics
of daily and total trading.

The `CMEProcessor` project is a C++ console application that takes one command line argument, the input file, and produces a set of statistics files,
descriptions of whose formats can be found under `CMEProcessor/Distribution.cpp` and `CMEProcessor/IntelligentDistribution.h`, together with a list in `main.cpp`.

The `ShapeAnalyst` project, although later changed to `GeneralAnalyst`, is a Python script that takes the statistics records produced above day-by-day and produced one comprehensive summary file of the given statistics, looking at cumulatives, means, and standard deviations.

For more information just look into the in-line documentation in the code files, or raise an issue via the GitHub ticketing system.
