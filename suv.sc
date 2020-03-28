(library (uv uv)
  (export uv-listen
	  uv-run)
  (import (chezscheme))

  (define suv_listen
    (foreign-procedure "suv_listen"
		       (string int uptr)
		       int))
  (define suv_run
    (foreign-procedure "suv_run"
		       void
		       void))
  
  (define (listen-cb cb)
    (let ([code (foreign-callable __collect_safe cb (string) string)])
      (lock-object code)
      (foreign-callable-entry-point code))))

  ; TODO: should take alist of ip,port,protocol,etc and cb
  (define (suv-listen ip port cb)
    (suv_listen ip
		 port
		 (listen-cb cb)))

  (define (suv-run)
    (suv_run))
)






	
