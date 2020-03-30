# suv
async io for Chez Scheme via [libuv](https://github.com/libuv/libuv)

# This is a Work in Progress
The api is *guaranteed* to change

## Example
This example echos the received request
```Scheme
(import (suv suv))

(suv-listen "127.0.0.1"
	    8080
	    (lambda (req)
	      req))
		  
(suv-run)
```

## async io does not make *your* code run async
1. If your code blocks, the thread your code is operating in is blocked. An example will make this clear.
```Scheme
(suv-listen "127.0.0.1"
	    8080
	    (lambda (req)
	      (sleep (make-time 'time-duration 0 10))))
```
A request sent to 127.0.0.1:8080 will block the thread it is running on for 10 seconds, preventing other requests from being processed. 
2. This library is not threadsafe

## This is a Work in Progress 2
* Again, the API *will* change
* The library only supports reading and writing to TCP sockets currently
* The library does not deal with partial reads currently
* Performance is not really considered (and likely won't be for some time). That said, the improvements required to make it high performing are straightforward
  - Use a mempool
  - Don't copy memory on the request path (e.g. when calling in and out of Chez)
  - Provide an async api for most blocking operations
