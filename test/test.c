#include "../c/suv.c"

char *echo_cb(const char* req) {
  // we need to simulate GC, so we just leak
  char *resp = malloc(strlen(req) + 1);
  strcpy(resp, req);
  return resp;
}

int main(void) {
  suv_listen("127.0.0.1", 8080, echo_cb);
  suv_run();
}
