#!/bin/bash
[ ! -d outputs ] && mkdir outputs

TIME=120
DELAY=20

for BWBOTTLE in 100 5000000; do
    DIR=outputs/bb-q$QSIZE
    [ ! -d $DIR ] && mkdir $DIR

    # Run the NS-3 Simulation
    ./waf --run "scratch/bufferbloat --bwNet=$BWNET --delay=$DELAY --time=$TIME --maxQ=$QSIZE"
    # Plot the trace figures
    python3 scratch/plot_bufferbloat_figures.py --dir $DIR/
done

echo "Simulations are done! Results can be retrieved via the server"
python3 -m http.server
