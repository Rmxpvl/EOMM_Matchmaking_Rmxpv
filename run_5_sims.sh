#!/bin/bash

echo "Running 5 simulations..."
echo ""

for i in {1..5}; do
    echo "=== Simulation $i ==="
    ./bin/eomm_system | grep -A 10 "EOMM SIMULATION"
    echo ""
    sleep 1
done

