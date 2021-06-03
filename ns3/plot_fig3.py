import glob
from os import listdir
from os.path import isfile, join

from matplotlib import pyplot as plt


util_trace_files = glob.glob(f'outputs/fig3-*-1/bytesSent.tr')
q_trace_files = glob.glob(f'outputs/fig3-*-1/q.tr')
drop_trace_files = glob.glob(f'outputs/fig3-*-1/drop.tr')

bottleneck_capacity = []

utils = []
q_avgs = []
drops = []
for filename in util_trace_files:
    bwBottleneck = filename.split("-")[1]
    bytesSentFile = open(filename)
    lines = bytesSentFile.readlines()
    early_bytes_sent = int(lines[0].split()[1])
    final_bytes_sent = int(lines[1].split()[1])
    late_bytes_sent = final_bytes_sent - early_bytes_sent;
    bits_per_sec = (late_bytes_sent * 8) / (0.8 * 120)
    util = float(bits_per_sec) / (float(bwBottleneck) * 1000)

    utils.append(util)
    bottleneck_capacity.append(bwBottleneck)

for filename in q_trace_files:
    bwBottleneck = filename.split("-")[1]
    qTrFile = open(filename)
    lines = qTrFile.readlines()
    avgs_sum = 0.0
    for line in lines:
        avgs_sum += float(line.split()[1])
    q_avg = avgs_sum / len(lines)

    q_avgs.append(q_avg)

for filename in drop_trace_files:
    bwBottleneck = filename.split("-")[1]
    dropTrFile = open(filename)
    lines = dropTrFile.readlines()
    packet_drops = int(lines[0].split()[1])
    total_sent= int(lines[1].split()[1])
    drop_pct = float(packet_drops) / float(total_sent)

    drops.append(drop_pct)

plt.figure(figsize=(10, 4))
plt.semilogx(bottleneck_capacity, utils, 'kD', label='VCP Utilization')
plt.ylabel('Bottleneck Utilization')
plt.xlabel('Bottleneck Capacity (Mbps)')
plt.legend()
plt.savefig('figure3_util_reproduction.png')
plt.close()

plt.figure(figsize=(10, 4))
plt.semilogx(bottleneck_capacity, q_avgs, 'kD', label='VCP Avg Queue')
plt.ylabel('Bottleneck Queue (% Buf)')
plt.xlabel('Bottleneck Capacity (Mbps)')
plt.legend()
plt.savefig('figure3_q_avg_reproduction.png')
plt.close()

plt.figure(figsize=(10, 4))
plt.semilogx(bottleneck_capacity, drops, 'kD', label='VCP Drop Rate')
plt.ylabel('Bottleneck Drops (% Pkt Sent)')
plt.xlabel('Bottleneck Capacity (Mbps)')
plt.legend()
plt.savefig('figure3_drop_reproduction.png')
plt.close()
