#!/bin/bash
# Check if at least two arguments are provided

if [ $# -lt 3 ]; then
  echo "Usage: $0 <num of servers> <server file name> <(ethtool) on/off> [interface of ethtool (on)]"
    exit 1
fi

output_folder="../measurements/meas_$2"
mkdir -p "$output_folder"
file_path_util="$output_folder/cpu_util_log_$2.txt"
e_tool_before="$output_folder/ethtool_before_$2.txt"
e_tool_after="$output_folder/ethtool_after_$2.txt"

# Check if Ethtool is set to 'on'
if [ "$3" = "on" ]; then
  if [ -n "$4" ]; then
    interface_name="$4"
    ethtool -S "$interface_name" > "$e_tool_before"
  else
    echo "Ethtool is set to 'on' but no interface name provided."
    exit 1
  fi
else
  echo "Ethtool is set to 'off' or no interface name provided."
fi

# Define the base command without the -port argument
base_command="./../build/src/tcp_conn -server"

# Number of server instances to run
num_servers=$1  # You can change this to the desired number

# Start CPU monitoring with sar
sar -u 1 >> "$file_path_util" &
sar_pid=$!

# Loop to run 'num_servers' server instances with different ports
for ((i = 0; i < num_servers; i++)); do
  # Run the server with a unique port number for each instance (incrementing by 1)
  port=$((8880 + i))
  output_file="$output_folder/server_log_$i.txt"
  
  echo "Running server $i on port $port"
  $base_command -port:$port > "$output_file" &
done

# Capture the process IDs (PIDs) of all server instances
server_pids=($(jobs -p))

# Wait for all server instances to finish
for pid in "${server_pids[@]}"; do
  wait "$pid"
done

# Stop CPU monitoring
kill $(jobs -p)

kill "$sar_pid"

if [ "$3" = "on" ]; then
  ethtool -S "$interface_name" > "$e_tool_after"
fi

exit 0

