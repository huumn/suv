(import (suv suv))

(suv-listen "127.0.0.1"
	    8080
	    (lambda (req)
	      req))

(suv-run)
