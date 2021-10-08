#!/usr/bin/env bash
sudo pktgen -l 0,1,2,3 -n 4 --proc-type auto --socket-mem 1024 --file-prefix ccc -a 0000:03:00.0 -- -P -m 3.0 -j
