(library (suv suv)
  (export suv-listen
	  suv-connect
	  suv-run
	  suv-read-start
	  suv-accept
	  suv-getpeername
	  suv-write
	  suv-close)
  (import (chezscheme))

  (define lib (load-shared-object "./lib/suv/libsuv.so"))

  (define suv_connect
    (foreign-procedure "suv_connect"
		       (string int uptr)
		       int))

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

  ;; This prevents the "code" (ie exp and env) from being garbage
  ;; collected/relocated while it is in C-land. When this locked
  ;; code is no longer in use, unlock-code-pointer (below) should be
  ;; called on it ... for now, this is handled in C-land
  (define-syntax locked-code-pointer
    (syntax-rules ()
      [(_ cb param-types ret-type)
       (let ([code (foreign-callable cb param-types ret-type)])
	 (lock-object code)
	 (foreign-callable-entry-point code))]))

  (define (suv-connect ip port cb)
    (suv_connect ip
		 port
		 (locked-code-pointer cb
				      (uptr)
				      void)))

  ;; TODO: should take alist of ip, port, protocol, backlog etc
  (define (suv-listen ip port cb)
    (suv_listen ip
		port
		(locked-code-pointer cb
				     (uptr)
				     void)))

  (define suv-accept
    (foreign-procedure "suv_accept"
		       (uptr)
		       int))

  (define suv-getpeername
    (foreign-procedure "suv_getpeername"
		       (uptr)
		       string))
  
  (define (suv-read-start client cb)
    (suv_read_start client
		    (locked-code-pointer cb
					 (string)
					 void)))

  (define suv-write
    (case-lambda
      [(client data)
       (suv_write client data 0)]
      [(client data cb)
       (suv_write client
		  data
		  (locked-code-pointer cb
				       (int)
				       void))]))
  
  (define suv-close
    (foreign-procedure "suv_close"
		       (uptr)
		       void))

  ;; TODO: might want to expose run args
  (define suv-run
    (foreign-procedure "suv_run"
		       ()
		       void))

  
  (define (unlock-code-pointer code-ptr)
    (unlock-object (foreign-callable-code-object code-ptr)))
  
  ((foreign-procedure "set_Sunlock_code_pointer"
		      (uptr)
		      void)
   (locked-code-pointer unlock-code-pointer
			(uptr)
			void))

)






	
