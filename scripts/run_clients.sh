#!/bin/bash

# Check if the number of clients argument is provided
if [ $# -ne 3 ]; then
  echo "Usage: $0 <Quic | TCP> <number_of_clients> <Target IP>"
  exit 1
fi

# Extract the number of clients from the argument
num_clients=$2

# Define the base command without the -port argument
if [ "$1" = "Quic" ]; then
  base_command="./../build/src/q_stream -client -unsecure -num_of_blocks:10 -size_of_blocks:100"
elif [ "$1" = "TCP" ]; then
  base_command="./../build/src/tcp_conn -client -num_of_blocks:10 -size_of_blocks:100"
else
  echo "Invalid transport protocol. Use 'Quic' or 'TCP'."
  exit 1
fi

# Loop to run 'num_clients' clients
for ((i = 0; i < num_clients; i++)); do

  output_file="$output_folder/client_log_$i.txt"
  # Run the client with a unique port number for each client (incrementing by 1)
  port=$((8880 + i))
  echo "Running client $i on port $port"
  if [ "$1" = "Quic" ]; then
    $base_command -target:$3 > "$output_file" &
  elif [ "$1" = "TCP" ]; then
    $base_command -port:$port -target:$3 > "$output_file" &
  else
    echo "Invalid transport protocol. Use 'Quic' or 'TCP'."
    exit 1
  fi
done

# Wait for all background processes to finish
wait

echo "All clients have finished."

