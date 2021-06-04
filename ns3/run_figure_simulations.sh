#!/usr/bin/env bash

# Whether to run the simulations sequentially or launch all in background
sequential=0

# Bandwidth values up to 10Mpbs
datapoints=(100 200 500 600 800 1000 1200 1500 3000 5000 8000 10000)

# full set of datapoint values up to 5Gbps (takes many, many hours!)
# datapoints=(100    200    500     600     800   
#             1000   1200   1500    3000    5000   
#             8000   10000  16000   25000   32000 
#             35000  40000  50000   75000   100000 
#             125000 150000 200000  250000  350000 
#             500000 750000 1000000 2000000 5000000)

./waf build

for bw in ${datapoints[@]}; do
    dir="outputs/figure3-default-bw${bw}/"
    mkdir -p ${dir}
    if [ ${sequential} -eq 0 ]; then
        ./waf --run "simulations/single-bottleneck-datapoint \
                     --bwBottleneck=${bw} \
                     --dir=${dir}" &
    else
        ./waf --run "simulations/single-bottleneck-datapoint \
                     --bwBottleneck=${bw} \
                     --dir=${dir}"
    fi
done

dir="outputs/figure1-default/"
mkdir -p ${dir}
if [ ${sequential} -eq 0]; then
    ./waf --run "simulations/figure-1-1 \
                 --bwNet=10 \
                 --estInterval=200 \
                 --delay=20 \
                 --maxQ=200 \
                 --time=260 \
                 --dir=${dir}" &
else
    ./waf --run "simulations/figure-1-1 \
                 --bwNet=10 \
                 --estInterval=200 \
                 --delay=20 \
                 --maxQ=200 \
                 --time=260 \
                 --dir=${dir}"
fi

# Wait for all to finish
wait

# Plot results
dir="outputs/figure3-default"
python3 plotting/plot_fig3.py "${dir}"

dir="outputs/figure1-default"
python3 plotting/plot_fig1.py "${dir}/throughput.tr" "${dir}/throughput2.tr" "figure1_cwndscaling.png"
