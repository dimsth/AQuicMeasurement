# Documantation for Microsoft Sample

Microsoft has build a simple client/server sample to run IETF Quic.

## Run the sample

-To run the sample you first need to made a certification for the server. <br />
(Look at cert folder: ```cd ../cert/```)

-Build the project.
<br /><br /><br />
Time to run the ms_sample.
<br />
Run the server first:<br />
```./ms_sample -server -cert_file:../../cert/server.cert -key_file:../../cert/server.key```
<br /><br />
Then the client:<br />
```./ms_sample -client -unsecure -target:"IPAddress" ``` <br />
--IPAddress = the ip of the server (for example: 196.1.1.1)
<br />
(Note: you can find the ip with ```ip address show```)
