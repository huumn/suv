(import (suv suv))

(suv-listen "127.0.0.1"
	    8080
	    (let ([acc ""])
	      (lambda (req)
		(begin
		  (set! acc
			(string-append acc
				       req))
		  acc))))

(suv-run)
