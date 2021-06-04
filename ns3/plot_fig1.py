#!/usr/bin/env python

import sys 

import numpy as np
from matplotlib import pyplot as plt

assert sys.argv == 4

fpaths = sys.argv[1:3]
output_path = sys.argv[3]

throughputs = []
for fpath in fpaths:
    with open(fpath, 'r') as f:
        rxbytes = f.readlines()
        
    rxbytes = list([int(x.strip()) for x in rxbytes])
    rxbytes = (np.array(rxbytes) * 8) / 1000000
    shifted = np.roll(rxbytes, 1)
    shifted[0] = 0
    throughput = (rxbytes - shifted) / 0.5 # interval per sample in seconds
    throughputs.append(throughput)


def plot_figure_1(throughput1, throughput2):
    fig = plt.figure(figsize=(12.8, 4))
    plt.plot(
        list(range(41)) + list(range(40, 81)) + list(range(80, 181)) + list(range(180, 221)) + list(range(220, 260)),
        ([10] * 41) + ([20] * 41) + ([10] * 101) + ([20] * 41) + ([10] * 40),
        'k-.',
        label='capacity'
    )
    
    plt.plot(
        np.arange(0, len(throughput1) // 2,  0.5),
        throughput1,
        'b-', linewidth=0.9,
        label='1st flow'
    )
    
    plt.plot(
        np.arange(120, 120 + len(throughput2) // 2,  0.5),
        throughput2,
        'r--', linewidth=0.9,
        label='2nd flow'
    )
    
    plt.ylim(0, 21)
    plt.legend()
    plt.ylabel('Flow Throughput (Mbps)')
    plt.xlabel('Time (sec)')
    plt.title('Throughput Dynamics of VCP')


plot_figure_1(throughput1, throughput2)
plt.savefig(output_path)
