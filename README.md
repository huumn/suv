# suv
async io (tcp sockets now) for [Chez Scheme](https://scheme.com) via [libuv](https://github.com/libuv/libuv)

## Heads up
The api is *guaranteed* to change

## Getting Started
Install libuv then ...
```bash
git clone git@github.com:huumn/suv.git 
cd suv
cc -o3 -fPIC -shared c/suv.c -luv -o libsuv.so
mkdir lib
ln -s `pwd` lib
scheme --libdirs .. --script examples/echo.sc
```

## Examples
### Echo
```Scheme
(import (suv suv))

(suv-listen "127.0.0.1"
	    8000
	    (lambda (client)
	      (suv-accept client)
	      (suv-read-start client
			      (lambda (req)
				(suv-write client
					   req)))
	      (suv-write client
			 (format "Welcome to Echo server, ~a!\r\n"
				 (suv-getpeername client)))))

(suv-run)
```

### State via closure
```Scheme
(import (suv suv))

(suv-listen "127.0.0.1"
	    8000
	    (lambda (client)
	      (suv-accept client)
	      (suv-read-start client
			      (let ([acc '()])
				(lambda (req)
				  (set! acc (append acc (list req)))
				  (if (> (length acc) 1)
				      (begin
					(suv-write client
						   (apply string-append
							  acc))
					(set! acc '()))))))
	      (suv-write client
			 "Welcome, I echo after 2 requests!\r\n")))

(suv-run)
```

### Client
```Scheme
(import (suv suv))

(suv-connect "127.0.0.1"
	     8000
	     (lambda (server)
	       (suv-read-start server
			       (lambda (req)
				 (display req)
				 (flush-output-port)))
	       (suv-write server
			  "Howdy, echo server!\r\n")))

(suv-run)
```

## Note
* Again, the API *will* change
* The library only supports operations on TCP sockets currently
* Performance is not really considered (and likely won't be for some time). That said, making it higher performing is straightfoward
  - Use a mempool
  - Don't copy memory on the request/response path (e.g. when calling in and out of Chez)
  - Provide an async api for other blocking operations that might be performed
