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
