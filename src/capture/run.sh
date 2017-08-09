#!/bin/bash

./capture -d 0 -m 14 -p 3 -v video.raw -l /dev/null -D 1 -b 2000 -f 2 -q 32 -B beforeFile.txt -A afterFile.txt > error.txt
