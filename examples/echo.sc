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
