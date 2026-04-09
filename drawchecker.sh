#!/bin/bash

count=0
while true; do
    ((count++))
    ./moss -R -IND -AUS
    if [[ $? -eq 19 ]]; then
        echo "Draw detected"
        echo "After $count games"
        break
    fi
done