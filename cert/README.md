# Documentation for Certificate

To run the programs and the microsoft sample you need to create a key and a certificate for the server.

## To create 

Run:

```
openssl req -nodes -new -x509 -keyout server.key -out server.cert
```

And answer the question or answer "." to let them blank.
