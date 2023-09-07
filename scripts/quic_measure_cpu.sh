#!/bin/bash

# Check if at least two arguments are provided
if [ $# -lt 2 ]; then
  echo "Usage: $0 <server> <(ethtool) on/off>"
    exit 1
fi
output_folder="../measurements/meas_$1"
mkdir "$output_folder"

program_command="./../build/src/q_stream -server -cert_file:../cert/server.cert -key_file:../cert/server.key"
file_path_util="$output_folder/cpu_util_log_$1.txt"
e_tool_before="$output_folder/ethtool_before_$1.txt"
e_tool_after="$output_folder/ethtool_after_$1.txt"


if [ "$2" = "on" ]; then
  if [ -n "$3" ]; then
    interface_name="$3"
    ethtool -S "$interface_name" > "$e_tool_before"
  else
    echo "Ethtool is set to 'on' but no interface name provided."
    exit 1
  fi
else
  echo "Ethtool is set to 'off' or no interface name provided."
fi

output_file="$output_folder/server_log.txt"

# Run the provided program command in the background
eval "$program_command" > "$output_file" &

# Capture the process ID (PID) of the program
pid=$!

sar -u 1 >> "$file_path_util" &
sar_pid=$!

wait "$pid"  

kill "$sar_pid"

if [ "$2" = "on" ]; then
  ethtool -S "$3" > "$e_tool_after"
fi

exit 0
