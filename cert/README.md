# Documentation for Certificate

To run the microsoft sample you need to create a key and a certificate for TLS.

## To create 

Run:

```
openssl req -nodes -new -x509 -keyout server.key -out server.cert
```

And answer few of those question or "." for blank answer.
