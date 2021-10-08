#!/usr/bin/env bash

make -C receive
sudo receive/build/receive -l 8-15 -n 4 --proc-type auto --socket-mem 1024 -a 0000:81:00.0 --file-prefix aaa -- aaa
