#include <stdlib.h>
#include <string.h>
#include <uv.h>

#define LOG_ERR(MSG) \
  fprintf(stderr, "ERROR %s %s:%d\n", MSG,  __FILE__, __LINE__)
#define LOG_UVERR(MSG, ERRNO)	      \
  fprintf(stderr, "ERROR %s - %s %s:%d\n", \
	  MSG, uv_strerror(ERRNO), __FILE__, __LINE__)

typedef char* (*listen_cb)(const char* request); 

// TODO mem slab
void _alloc_cb(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf) {
  buf->base = calloc(1, suggested_size);
  buf->len = suggested_size;
}

void _write_cb(uv_write_t *w, int status) {
  if (status < 0) {
    LOG_ERR("write");
  }

  uv_buf_t *buf = w->data;
  free(buf->base);
  free(buf);
  free(w);
}

// Note: we claim the memory in buf
char *_uv_buf_to_string(const uv_buf_t *buf) {
  char *str;
  str = realloc(buf->base, buf->len + 1);
  str[buf->len] = '\0';
  return str;
}

// Note: unlike to_string we leave str alone (as it belongs to Chez ... maybe)
uv_buf_t *_string_to_uv_buf(char *str) {
  uv_buf_t *buf = calloc(1, sizeof(*buf));
  buf->len = strlen(str);
  buf->base = malloc(buf->len);
  memcpy(buf->base, str, buf->len);
  return buf;
}
  

void _read_cb(uv_stream_t *client, ssize_t nread, const uv_buf_t *buf) {
  if (nread < 0) {
    if (nread != UV_EOF) {
      LOG_ERR("read callback");  
    }
    uv_close(client, NULL);
    return;
  }

  // chez wants a null terminated string
  char *req = _uv_buf_to_string(buf);
  char *resp = ((listen_cb) client->data)(req);
  free(req);

  // libuv wants a uv_buf_t
  uv_write_t *write = calloc(1, sizeof(*write));
  write->data = _string_to_uv_buf(resp);

  int err = uv_write(write, client, write->data, 1, _write_cb);
  if (err) {
    LOG_UVERR("writing", err);
  }
}

void _conn_cb(uv_stream_t *server, int status) {
  int err;
  
  if (status < 0) {
    LOG_ERR("connect");
    return;
  }

  uv_tcp_t *client = calloc(1, sizeof(*client));
  err = uv_tcp_init(uv_default_loop(), client);
  if (err) {
    LOG_UVERR("initing client", err);
    uv_close(client, NULL);
    return;
  }

  err = uv_accept(server, client);
  if (err) {
    LOG_UVERR("accepting client", err);
    uv_close(client, NULL);
    return;
  }

  // pass along our read callback
  client->data = server->data; 
  err = uv_read_start(client, _alloc_cb, _read_cb);
  if (err) {
    LOG_UVERR("read start", err);
    uv_close(client, NULL);
    return;
  }
}

#define DEFAULT_BACKLOG 128
int suv_listen(const char* ip, int port, listen_cb cb) { 
  struct sockaddr_storage addr;
  int err;

  err = uv_ip4_addr(ip, port, &addr);
  if (err) {
    err = uv_ip6_addr(ip, port, &addr);
    if (err) {
      LOG_UVERR("setting addr", err);
      return err;
    }
  }

  // XXX leaks obvi
  uv_tcp_t *server = calloc(1, sizeof(*server));
  // stash callback for reads
  server->data = cb; 

  err = uv_tcp_init(uv_default_loop(), server);
  if (err) {
    LOG_UVERR("initing server", err);
    return err;
  }
  
  err = uv_tcp_bind(server, &addr, 0);
  if (err) {
    LOG_UVERR("binding addr", err);
    return err;
  }

  err = uv_listen(server, DEFAULT_BACKLOG, _conn_cb);
  if (err) {
    LOG_UVERR("listening", err);
    return err;
  }

  return err;
}

int suv_run() {
  int err;

  err = uv_run(uv_default_loop(), UV_RUN_DEFAULT);
  if (err) {
    LOG_UVERR("starting loop", err);
  }

  return err;
}
    
