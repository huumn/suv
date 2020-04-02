(library (suv suv)
  (export suv-listen
	  suv-run
	  suv-read-start
	  suv-accept
	  suv-write
	  suv-close)
  (import (chezscheme))

  (define lib (load-shared-object "./lib/suv/libsuv.so"))

  ;; TODO: might want to return handle to server for shutdown, etc
  (define suv_listen
    (foreign-procedure "suv_listen"
		       (string int uptr)
		       int))
  
  (define suv_write
    (foreign-procedure "suv_write"
		       (uptr string uptr)
		       int))

  (define suv_read_start
    (foreign-procedure "suv_read_start"
		       (uptr uptr)
		       int))

  ;; TODO: an oppertunity for macros (locked-code-pointer cb (p-type ...) r-type)
   (define (unlock-object-cb)
     (let ([code (foreign-callable unlock-object (uptr) void)])
       (lock-object code)
       (foreign-callable-entry-point code)))
   
  (define (read-start-cb cb)
    (let ([code (foreign-callable cb (string) void)])
      (lock-object code)
      (foreign-callable-entry-point code)))

  (define (connected-cb cb)
    (let ([code (foreign-callable cb (uptr) void)])
      (lock-object code)
      (foreign-callable-entry-point code)))

  (define (write-cb cb)
    (let ([code (foreign-callable cb (int) void)])
      (lock-object code)
      (foreign-callable-entry-point code)))

  ;; TODO: might want to expose run args
  (define suv-run
    (foreign-procedure "suv_run"
		       ()
		       void))

  (define suv-accept
    (foreign-procedure "suv_accept"
		       (uptr)
		       int))

  (define suv-close
    (foreign-procedure "suv_close"
		       (uptr)
		       void))

  ;; TODO: should take alist of ip, port, protocol, backlog etc?
  (define (suv-listen ip port cb)
    (suv_listen ip
		port
		(connected-cb cb)))

  (define (suv-read-start client cb)
    (suv_read_start client
		    (read-start-cb cb)))

  (define suv-write
    (case-lambda
      [(client data)
       (suv_write client data 0)]
      [(client data cb)
       (suv_write client
		  data
		  (write-cb cb))]))

  
   ((foreign-procedure "set_Sunlock_object"
		       (uptr)
		       void) (unlock-object-cb))
)






	
