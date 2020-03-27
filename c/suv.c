#include <uv.h>

typedef char* (*listen_cb)(const char* request); 

// TODO mem slab
void _alloc_cb(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf) {
  buf->base = malloc(suggested_size);
  buf->len = suggested_size;
}

// TODO log status
void _write_cb(uv_write_t *w, int status) {
  uv_close(w->handle, free);
  // XXX Ehhhh
  free(((uv_buf_t)w->data)->base);
  free(w->data);
  free(w);
}

void _read_cb(uv_stream_t *client, ssize_t nread, const uv_buf_t *buf) {
  if (nread < 0) {
    // TODO log error
    return;
  }

  // XXX tbd who is responsible for this memory ..
  // assumes we are so I free it
  char *data = ((listen_cb) client->data)(bus->base);

  uv_write_t *resp = calloc(1, sizeof(*resp));
  uv_buf_t *resp_buf = calloc(1, sizeof(*resp_buf));
  *resp_buf = uv_buf_init(data, len(data));
  resp->data = resp_buf;
  uv_write(resp, client, resp_buf, 1, _write_cb);
  
  free(buf->base);
}

void _conn_cb(uv_stream_t *server, int status) {
  if (status < 0) {
    // TODO log error
    return;
  }

  uv_tcp_t *client = malloc(1, sizeof(*client));
  uv_tcp_init(uv_default_loop(), client);
  if (uv_accept(server, client) == 0) {
    client->data = server->data; // pass along our read callback
    uv_read_start(client, _alloc_cb, _read_cb);
  } else {
    uv_close(client, NULL);
  }
}

int suv_listen(const char* ip, int port, listen_cb cb) { 
  struct sockaddr_in addr;
  uv_ip4_addr(ip, port, &addr);

  // XXX leaks obvi
  uv_tcp_t *server = calloc(1, sizeof(*server));
  uv_tcp_init(uv_default_loop());
  server->data = cb; // stash callback for reads

  return uv_tcp_bind(server, DEFAULT_BACKLOG, _conn_cb);
}

int suv_run() {
  return uv_run(uv_default_loop(), UV_RUN_DEFAULT);
}
    
