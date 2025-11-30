#!/bin/bash

printf "\033[30m    black"
printf "\033[31m     red"
printf "\033[32m   green"
printf "\033[33m  yellow"
printf "\033[34m    blue"
printf "\033[35m magenta"
printf "\033[36m    cyan"
printf "\033[37m   white"
printf "\n"

printf "\033[90m    black"
printf "\033[91m     red"
printf "\033[92m   green"
printf "\033[93m  yellow"
printf "\033[94m    blue"
printf "\033[95m magenta"
printf "\033[96m    cyan"
printf "\033[97m   white"
printf "\n"

for i in $(seq 16 255); do
    printf "\033[38;5;%dm%03d " $i $i
    if [ $((($i - 16) % 6)) -eq 5 ]; then
        printf "\n"
    fi
done
