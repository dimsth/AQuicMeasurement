#!/bin/bash

# Check if at least two arguments are provided
if [ $# -lt 3 ]; then
  echo "Usage: $0 <program_command> <client/server> <(ethtool) on/off>"
    exit 1
fi
output_folder="../measurements/meas_$2"
mkdir "$output_folder"

program_command="$1"
file_path_util="$output_folder/cpu_util_log_$2.txt"
e_tool_before="$output_folder/ethtool_before_$2.txt"
e_tool_after="$output_folder/ethtool_after_$2.txt"

if [ "$3" = "on" ] && [ -n "$4" ]; then
  ethtool -S $4 > "$e_tool_before"
else
  echo "Ethtool is set off or not interface was set"
fi

# Run the provided program command in the background
eval "$program_command" &

# Capture the process ID (PID) of the program
pid=$!

sar -u 1 >> "$file_path_util" &
sar_pid=$!

wait "$pid"  

kill "$sar_pid"

if [ "$3" = "on" ]; then
  ethtool -S $4 > "$e_tool_after"
fi

exit 0
