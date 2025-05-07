XrdHttp
-------

HTTP(S) using the XRootD thread-pool and XrdHttp can be enabled in ```/etc/xrd.cf.fst``` like:


```
if exec xrootd
   xrd.protocol XrdHttp:9000 /usr/lib64/libXrdHttp-4.so
   http.exthandler EosFstHttp /usr/lib64/libEosFstHttp.so none
   http.cert /etc/grid-security/daemon/host.cert
   http.key /etc/grid-security/daemon/privkey.pem
   http.cafile /etc/grid-security/daemon/ca.cert
fi
```


