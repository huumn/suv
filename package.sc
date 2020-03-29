(("name" . "suv")
("version" . "0.1.0")
("description" . "libuv for Chez Scheme")
("keywords" ("scheme"
	     "async"
	     "io"))
("author" 
    ("huumn"))
("private" . #f)
("scripts"
    ("build" . "cd ./lib/suv/c && cc -o3 -fPIC -shared suv.c -luv -o ../libsuv.so"))
("dependencies")
("devDependencies"))
