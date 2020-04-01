#include <stdlib.h>
#include <string.h>
#include <scheme.h>
#include <uv.h>

#define LOG_ERR(MSG) \
  fprintf(stderr, "ERROR %s %s:%d\n", MSG,  __FILE__, __LINE__)
#define LOG_UVERR(MSG, ERRNO)	      \
  fprintf(stderr, "ERROR %s - %s %s:%d\n", \
	  MSG, uv_strerror(ERRNO), __FILE__, __LINE__)

// 1. unexported definitions start with '_'
// 2. functions that call into Scheme and variables whose memory belongs to
// Scheme start with 'S'
// 3. How to deal with naming callbacks? We have function pointer (ie callback) types
// that call into Scheme, local callbacks called from libuv, and variable names of
// Scheme callbacks ... Perhaps function pointers should be suv_*_cb, local callbacks
// _*_cb, and variable names into scheme S*_cb
//

typedef void (*_Sread_cb)(const char* request);
typedef void (*_Swrite_cb)(int status);
typedef void (*_Sconn_cb)(uv_stream_t *client);

typedef struct {
  uv_buf_t *buf;
  _Swrite_cb Swrite_cb;
} _write_data_t;

#define WRITE_DATA(w) ((_write_data_t *)w->data)

typedef struct {
  uv_stream_t *server;
  _Sread_cb Sread_cb;
} _client_data_t;

#define CLIENT_DATA(c) ((_client_data_t *) c->data)


typedef struct {
  _Sconn_cb Sconn_cb;
} _server_data_t;

#define SERVER_DATA(s) ((_server_data_t *) s->data)

static void _uv_alloc_cb(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf) {
  buf->base = calloc(1, suggested_size);
  buf->len = suggested_size;
}

// Note: we claim the memory in buf
static char *_uv_buf_to_string(const uv_buf_t *buf) {
  char *str;
  str = realloc(buf->base, buf->len + 1);
  str[buf->len] = '\0';
  return str;
}

// Note: unlike to_string we leave str alone (as we assume it is Chez's)
static uv_buf_t *_string_to_uv_buf(const char *str) {
  uv_buf_t *buf = calloc(1, sizeof(*buf));
  buf->len = strlen(str);
  buf->base = malloc(buf->len);
  memcpy(buf->base, str, buf->len);
  return buf;
}

static void _uv_close_cb(uv_handle_t *client) {
  // TODO: should probably check if Sread_cb is set
  Sunlock_object(CLIENT_DATA(client)->Sread_cb);
  free(client->data);
  free(client);
}

void suv_close(uv_stream_t *client) {
  uv_close(client, _uv_close_cb);
}

int suv_accept(uv_stream_t *client) {
  int err = uv_accept(CLIENT_DATA(client)->server, client);
  if (err) {
    LOG_UVERR("accepting client", err);
    uv_close(client, _uv_close_cb);
  }
  return err;
}

void _uv_write_cb(uv_write_t *write, int status) {
  // TODO: check if Swrite_cb is null
  _write_data_t *data= WRITE_DATA(write);
  data->Swrite_cb(status);
  Sunlock_object(data->Swrite_cb);
  free(data->buf->base);
  free(data->buf);
  free(data);
  free(write);
}

int suv_write(uv_stream_t *client, const char* Sdata, _Swrite_cb Scb) {
  uv_write_t *write = calloc(1, sizeof(*write));
  write->data = calloc(1, sizeof(_write_data_t));
  // we stash to the callback so we can unlock it when it isn't needed
  WRITE_DATA(write)->Swrite_cb = Scb;
  // similarly need to copy Sdata and stash so we can free on cb
  WRITE_DATA(write)->buf = _string_to_uv_buf(Sdata);

  int err = uv_write(write, client, WRITE_DATA(write)->buf, 1, _uv_write_cb);
  if (err) {
    LOG_UVERR("writing", err);
    return err;
  }

  return err;
}

void _uv_read_cb(uv_stream_t *client, ssize_t nread, const uv_buf_t *buf) {
  if (nread < 0) {
    if (nread != UV_EOF) {
      LOG_ERR("read callback");  
    }
    uv_close(client, _uv_close_cb);
    return;
  }

  char *str = _uv_buf_to_string(buf);
  CLIENT_DATA(client)->Sread_cb(str);
  free(str);
}

int suv_read_start(uv_stream_t *client, _Sread_cb Scb) {
  CLIENT_DATA(client)->Sread_cb = Scb;
  
  int err = uv_read_start(client, _uv_alloc_cb, _uv_read_cb);
  if (err) {
    LOG_UVERR("read start", err);
    uv_close(client, _uv_close_cb);
    return err;
  }

  return err;
}

void _uv_conn_cb(uv_stream_t *server, int status) {
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
  
  client->data = calloc(1, sizeof(_client_data_t));
  CLIENT_DATA(client)->server = server;

  SERVER_DATA(server)->Sconn_cb(client);
}

#define DEFAULT_BACKLOG 128
int suv_listen(const char* ip, int port, _Sconn_cb Scb) { 
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
  server->data = calloc(1, sizeof(_server_data_t));
  // stash callback into scheme on connection
  SERVER_DATA(server)->Sconn_cb = Scb; 

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

  err = uv_listen(server, DEFAULT_BACKLOG, _uv_conn_cb);
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
    
