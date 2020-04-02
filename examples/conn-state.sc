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
