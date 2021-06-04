import glob
import os
from os import listdir
from os.path import isfile, join
import re

from matplotlib import pyplot as plt
import numpy as np


import sys

assert len(sys.argv) == 2, sys.argv

input_dir = sys.argv[1]

img_prefix = input_dir.split('/')[-1].replace('-', '_')

util_trace_files = glob.glob(os.path.join(input_dir + '*', 'bytesSent.tr'))
q_trace_files = glob.glob(os.path.join(input_dir + '*', 'q.tr'))
drop_trace_files = glob.glob(os.path.join(input_dir + '*', 'drops.tr'))

bottleneck_capacity_util = []
bottleneck_capacity_q = []
bottleneck_capacity_drop = []

utils = []
q_avgs = []
drops = []
for filename in util_trace_files:
    try:
        m = re.search(r'bw(\d+)', filename)
        bwBottleneck = float(m.group(1))
        bytesSentFile = open(filename)
        lines = bytesSentFile.readlines()
        early_bytes_sent = int(lines[0].split()[1])
        final_bytes_sent = int(lines[1].split()[1])
        late_bytes_sent = final_bytes_sent - early_bytes_sent;
        bits_per_sec = (late_bytes_sent * 8) / (0.8 * 120)
        util = float(bits_per_sec) / (bwBottleneck * 1000)

        utils.append(util)
        bottleneck_capacity_util.append(bwBottleneck)
    except:
        pass

for filename in q_trace_files:
    try:
        m = re.search(r'bw(\d+)', filename)
        bwBottleneck = float(m.group(1))
        if bwBottleneck not in bottleneck_capacity_util:
            continue

        qTrFile = open(filename)
        lines = qTrFile.readlines()
        avgs_sum = 0.0
        for line in lines:
            avgs_sum += float(line.split()[1])
        q_avg = avgs_sum / len(lines)

        q_avgs.append(q_avg * 100)
        bottleneck_capacity_q.append(float(bwBottleneck))
    except:
        pass

for filename in drop_trace_files:
    try:
        m = re.search(r'bw(\d+)', filename)
        bwBottleneck = float(m.group(1))
        if bwBottleneck not in bottleneck_capacity_util:
            continue

        dropTrFile = open(filename)
        lines = dropTrFile.readlines()
        packet_drops = int(lines[0].split()[1])
        total_sent= int(lines[1].split()[1])
        drop_pct = float(packet_drops) / float(total_sent)

        drops.append(drop_pct * 100)
        bottleneck_capacity_drop.append(float(bwBottleneck))
    except:
        pass

print(len(utils), len(q_avgs), len(drops))
bottleneck_capacity_util = np.array(bottleneck_capacity_util) / 1000
sorted_inds = np.argsort(bottleneck_capacity_util)
bottleneck_capacity_util = bottleneck_capacity_util[sorted_inds]
utils = np.array(utils[:len(bottleneck_capacity_util)])[sorted_inds]

bottleneck_capacity_q = np.array(bottleneck_capacity_q) / 1000
sorted_inds = np.argsort(bottleneck_capacity_q)
bottleneck_capacity_q = bottleneck_capacity_q[sorted_inds]
q_avgs = np.array(q_avgs[:len(bottleneck_capacity_q)])[sorted_inds]

bottleneck_capacity_drop= np.array(bottleneck_capacity_drop) / 1000
sorted_inds = np.argsort(bottleneck_capacity_drop)
bottleneck_capacity_drop = bottleneck_capacity_drop[sorted_inds]
drops = np.array(drops[:len(bottleneck_capacity_drop)])[sorted_inds]

plt.figure(figsize=(10, 4))
plt.semilogx(bottleneck_capacity_util, utils, 'k-D', label='VCP Utilization')
plt.ylabel('Bottleneck Utilization')
plt.xlabel('Bottleneck Capacity (Mbps)')
plt.ylim(0.6, 1.1)
plt.legend()
plt.savefig(f'{img_prefix}_util_reproduction.png', bbox_inches='tight')
plt.close()

plt.figure(figsize=(10, 4))
plt.semilogx(bottleneck_capacity_q, q_avgs, 'k-D', label='VCP Avg Queue')
plt.ylabel('Bottleneck Queue (% Buf)')
plt.xlabel('Bottleneck Capacity (Mbps)')
plt.legend()
plt.savefig(f'{img_prefix}_q_avg_reproduction.png', bbox_inches='tight')
plt.close()

plt.figure(figsize=(10, 4))
plt.semilogx(bottleneck_capacity_drop, drops, 'k-D', label='VCP Drop Rate')
plt.ylabel('Bottleneck Drops (% Pkt Sent)')
plt.xlabel('Bottleneck Capacity (Mbps)')
plt.legend()
plt.savefig(f'{img_prefix}_drop_reproduction.png', bbox_inches='tight')
plt.close()


print(f'{img_prefix}_util_reproduction.png')
print(f'{img_prefix}_q_avg_reproduction.png')
print(f'{img_prefix}_drop_reproduction.png')
