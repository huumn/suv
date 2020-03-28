(("name" . "uv")
("version" . "0.1.0")
("description" . "")
("keywords")
("author" 
    ("huumn"))
("private" . #f)
("scripts"
    ("build" . "cd ./lib/uv/c && cc -o3 -fPIC -shared uv.c -luv -o ../uv.so")
    ("repl" . "scheme")
    ("run" . "scheme --script"))
("dependencies")
("devDependencies"))
