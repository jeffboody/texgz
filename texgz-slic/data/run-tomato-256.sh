#!/bin/bash

# usage: ./texgz-slic s m sdx n r steps prefix
# s: superpixel size (sxs)
# m: compactness control
# sdx: stddev threshold
# n: gradient neighborhood (nxn)
# r: recenter clusters
# steps: maximum step count

../texgz-slic 8 10.0 0.0 3 1 10 tomato-256
../texgz-slic 8  1.0 2.0 3 0 10 tomato-256
../texgz-slic 8  1.0 1.0 3 0 10 tomato-256
../texgz-slic 8  1.0 0.0 3 0 10 tomato-256
