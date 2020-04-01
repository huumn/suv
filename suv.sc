(library (suv suv)
  (export suv-listen
	  suv-run
	  suv-read-start
	  suv-accept
	  suv-write)
  (import (chezscheme))

  (define lib (load-shared-object "./lib/suv/libsuv.so"))

  ;; TODO: might want to return handle to server for shutdown, etc
  (define suv_listen
    (foreign-procedure "suv_listen"
		       (string int uptr)
		       int))
  
  (define suv_write
    (foreign-procedure "suv_write"
		       (uptr u8* uptr)
		       int))

  (define suv_read_start
    (foreign-procedure "suv_read_start"
		       (uptr uptr)
		       int))

  ;; TODO: an oppertunity for macros (locked-code-pointer cb (param-type ...) ret-type)
  (define (read-start-cb cb)
    (let ([code (foreign-callable cb (u8*) void)])
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

  (define (suv-write client data cb)
    (suv_write client
	       data
	       (write-cb cb)))

  ;; immediately welcome client, then echo whatever they say and close the connection
  ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
  ;; (suv-listen ip						    ;;
  ;; 	      port						    ;;
  ;; 	      (lambda (client)					    ;;
  ;; 		(suv-accept client)				    ;;
  ;; 		(suv-read-start client				    ;;
  ;; 				(lambda (req)			    ;;
  ;; 				  (suv-write client		    ;;
  ;; 					     req		    ;;
  ;; 					     suv-close)))	    ;;
  ;; 		(suv-write client				    ;;
  ;; 			   (string->utf8 "hey")			    ;;
  ;; 			   nil)))				    ;;
  ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	       
)






	
