(library (suv suv)
  (export suv-listen
	  suv-run)
  (import (chezscheme))

  (define lib (load-shared-object "./lib/suv/libsuv.so"))

  (define suv_listen
    (foreign-procedure "suv_listen"
		       (string int uptr)
		       int))
  (define suv_run
    (foreign-procedure "suv_run"
		       ()
		       void))
  
  (define (listen-cb cb)
    (let ([code (foreign-callable cb (string) string)])
      (lock-object code)
      (foreign-callable-entry-point code)))

  ; TODO: should take alist of ip, port, protocol, etc
  (define (suv-listen ip port cb)
    (suv_listen ip
		port
		(listen-cb cb)))

  (define (suv-run)
    (suv_run))
)






	
