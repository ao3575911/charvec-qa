#!/bin/sh
set -eu

MODEL="build/smoke.cvec"

./build/charvec build -i examples/dataset.txt -o "$MODEL"
./build/charvec inspect -m "$MODEL" | grep "pairs: 32" >/dev/null
./build/charvec ask -m "$MODEL" -q "Who created C?" | grep "Dennis Ritchie" >/dev/null
./build/charvec ask -m "$MODEL" -q "What is one-hot encoding?" | grep "one active position" >/dev/null
./build/charvec ask -m "$MODEL" -q "What does inspect show?" | grep "model metadata" >/dev/null
