(library (uv uv)
  (export uv-listen
	  uv-run)
  (import (chezscheme))

  (define uvsc_listen
    (foreign-procedure "uvsc_listen"
		       (string int uptr)
		       int))
  (define uvsc_run
    (foreign-procedure "uvsc_run"
		       void
		       void))
  
  (define (listen-cb cb)
    (let ([code (foreign-callable __collect_safe cb (string) string)])
      (lock-object code)
      (foreign-callable-entry-point code))))

  ; TODO: should take alist of ip,port,protocol,etc and cb
  (define (uv-listen ip port cb)
    (uvsc_listen ip
		 port
		 (listen-cb cb)))

  (define (uv-run)
    (uvsc_run))
)






	
