# Documentation for Scripts

To run the programs and the microsoft sample using the script, you need to create a key and a certificate for the QUIC server.

For TCP only to build the project.

## MEASURE CPU 

Example of Quic w/ Stream server:

```
./measure_cpu.sh "./../build/src/q_stream -server -cert_file:../cert/server.cert -key_file:../cert/server.key" server off
```

-arg 1: Executable Program with all the arguments inside ""
-arg 2: Name for the folder and some files
-arg 3: On/Off if you want to capture ethtool
-arg 4: Ethtool parameter for which interface name to look 
