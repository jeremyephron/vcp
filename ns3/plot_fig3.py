import glob
from os import listdir
from os.path import isfile, join

util_trace_files = glob.glob(f'outputs/fig3-*-1/bytesSent.tr')
q_trace_files = glob.glob(f'outputs/fig3-*-1/q.tr')
drop_trace_files = glob.glob(f'outputs/fig3-*-1/drop.tr')

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

    utils.append((float(bwBottleneck) / 1000, util))

for filename in q_trace_files:
    bwBottleneck = filename.split("-")[1]
    qTrFile = open(filename)
    lines = qTrFile.readlines()
    avgs_sum = 0.0
    for line in lines:
        avgs_sum += float(line.split()[1])
    q_avg = avgs_sum / len(lines)

    q_avgs.append((float(bwBottleneck) / 1000, q_avg))

for filename in drop_trace_files:
    bwBottleneck = filename.split("-")[1]
    dropTrFile = open(filename)
    lines = dropTrFile.readlines()
    packet_drops = int(lines[0].split()[1])
    total_sent= int(lines[1].split()[1])
    drop_pct = float(packet_drops) / float(total_sent)

    drops.append((float(bwBottleneck) / 1000, drop_pct))

utils.sort()
q_avgs.sort()
drops.sort()
print(utils)
print(drops)
print(q_avgs)

'''
qFileName = "outputs/fig3/util.png"
utilBWs = [bw for (bw, util) in utils]
utils = [util for (bw, util) in utils]

plt.figure()
plt.plot(utilBWs, utils, c="C2")
plt.ylabel('Bottleneck Utilization')
plt.xlabel('Bottleneck Capacity (Mbps)')
plt.set_xscale('log')
plt.grid()
plt.savefig(qFileName)
print('Saving ' + qFileName)
'''




    
