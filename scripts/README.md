# Documentation for Scripts

To run the programs and the microsoft sample using the script, you need to create a key and a certificate for the QUIC server.

For TCP only to build the project.

## MEASURE CPU QUIC 

Example of Quic w/ Stream server:

```
./quic_measure_cpu.sh server_name off
```

- arg 1: Name for the folder and some files
<br />
- arg 2: On/Off if you want to capture ethtool
<br />
- [arg 3: Ethtool parameter for which interface name to look] (only when ethtool on)
<br />

Example of Quic w/ Stream client(s):

```
./run_clients.sh Quic 2 127.0.0.1
```

- arg 1: Number of clients to run
<br />
- arg 2: Name for the folder and some files
<br />
- arg 3: IP address to target 
<br />


## MEASURE CPU TCP

Example of Quic w/ Stream server:

```
./tcp_measure_cpu_servers.sh 2 server_name off
```

- arg 1: Number of servers to run
<br />
- arg 2: Name for the folder and some files
<br />
- arg 3: On/Off if you want to capture ethtool
<br />
- [arg 4: Ethtool parameter for which interface name to look] (only when ethtool on) 
<br />

Example of Quic w/ Stream client(s):

```
./run_clients.sh TCP 2 127.0.0.1
```

- arg 1: Number of clients to run
<br />
- arg 2: Name for the folder and some files
<br />
- arg 3: IP address to target 
<br />
