#!/usr/bin/env python3

import itertools
from mytcp import congestion_control_algorithm

#All parameteric combinations
ki = [1, 4]
km = [1, 1.5]
kn = [0.5, 1]
kf = [0.1, 0.3]
ps = [0.99, 0.9999]

#All combinations of parameters
arg_combinations = list(itertools.product(ki,km,kn,kf,ps))
for comb in arg_combinations:
    congestion_control_algorithm(comb[0],comb[1],comb[2],comb[3],comb[4],T=1000,flag=1)