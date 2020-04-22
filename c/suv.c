#include <stdlib.h>
#include <string.h>
#include <uv.h>

#define LOG_ERR(MSG) \
  fprintf(stderr, "ERROR %s %s:%d\n", MSG,  __FILE__, __LINE__)
#define LOG_UVERR(MSG, ERRNO)	      \
  fprintf(stderr, "ERROR %s - %s %s:%d\n", \
	  MSG, uv_strerror(ERRNO), __FILE__, __LINE__)

// naming conventions ... cause there's a lot going on
// 1. unexported symbols should start with '_'
// 2. functions that call into Scheme and variables whose memory belongs to
// Scheme start with 'S'
// 3. callback types and instances of callback types should end with _cb
// 4. callback definitionts for libuv should start with _uv

// we do this to avoid linking with Chez kernel ... it allows our code
// to unlock callbacks ... the other ways to do this are more work
// set_Sunlock_code_pointer is expected to be called during initialization of
// the suv library
typedef void (*_Sunlock_code_pointer_f)(void *);
static _Sunlock_code_pointer_f Sunlock_code_pointer = NULL;
void set_Sunlock_code_pointer(_Sunlock_code_pointer_f Scb) {
  Sunlock_code_pointer = Scb;
}

typedef void (*_Sread_cb)(const char* request);
typedef void (*_Sread_err_cb)(int status);
typedef void (*_Swrite_cb)(int status);
typedef void (*_Sconnected_cb)(uv_stream_t *client);
typedef void (*_Sconnect_cb)(uv_stream_t *client);

typedef struct {
  uv_buf_t *buf;
  _Swrite_cb Swrite_cb;
} _write_data_t;

#define WRITE_DATA(w) ((_write_data_t *)w->data)

typedef struct {
  uv_stream_t *server;
  _Sread_cb Sread_cb;
  _Sread_err_cb Sread_err_cb;
} _client_data_t;

#define CLIENT_DATA(c) ((_client_data_t *) c->data)


typedef struct {
  _Sconnected_cb Sconnected_cb;
} _server_data_t;

#define SERVER_DATA(s) ((_server_data_t *) s->data)

typedef struct {
  uv_tcp_t *client;
  _Sconnect_cb Sconnect_cb;
} _connect_data_t;

#define CONNECT_DATA(s) ((_connect_data_t *) s->data)

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
  Sunlock_code_pointer(CLIENT_DATA(client)->Sread_cb);
  Sunlock_code_pointer(CLIENT_DATA(client)->Sread_err_cb);
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

// Note: this returns a pointer to static memory
// Why this is okay: we are single threaded and
// Scheme afaik makes a copy of the return value
const char *suv_getpeername(uv_stream_t *client) {
  struct sockaddr_in name;
  int len = sizeof(name);
  if (uv_tcp_getpeername(client, &name, &len)) {
    LOG_ERR("getting peername");
    return "";
  }

  char addr[16];
  static char buf[32];
  uv_inet_ntop(AF_INET, &name.sin_addr, addr, sizeof(addr));
  snprintf(buf, sizeof(buf), "%s:%d", addr, ntohs(name.sin_port));

  return buf;
}

static void _uv_write_cb(uv_write_t *write, int status) {
  _write_data_t *data= WRITE_DATA(write);
  if (data->Swrite_cb) {
    data->Swrite_cb(status);
    Sunlock_code_pointer(data->Swrite_cb);
  }
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

static void _uv_read_cb(uv_stream_t *client, ssize_t nread, const uv_buf_t *buf) {
  if (nread < 0) {
    CLIENT_DATA(client)->Sread_err_cb(nread);
    return;
  }

  char *str = _uv_buf_to_string(buf);
  CLIENT_DATA(client)->Sread_cb(str);
  free(str);
}

int suv_read_start(uv_stream_t *client, _Sread_cb Scb, _Sread_err_cb Serrcb) {
  CLIENT_DATA(client)->Sread_cb = Scb;
  CLIENT_DATA(client)->Sread_err_cb = Serrcb;
  
  int err = uv_read_start(client, _uv_alloc_cb, _uv_read_cb);
  if (err) {
    LOG_UVERR("read start", err);
    uv_close(client, _uv_close_cb);
    return err;
  }

  return err;
}

static void _uv_connected_cb(uv_stream_t *server, int status) {
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
    free(client);
    return;
  }
  
  client->data = calloc(1, sizeof(_client_data_t));
  CLIENT_DATA(client)->server = server;

  SERVER_DATA(server)->Sconnected_cb(client);
}

static int _addr(const char *ip, int port, struct sockaddr_storage *addr) {
  int err;

  err = uv_ip4_addr(ip, port, addr);
  if (err) {
    err = uv_ip6_addr(ip, port, addr);
  }

  return err;
}
  

#define DEFAULT_BACKLOG 128
int suv_listen(const char* ip, int port, _Sconnected_cb Scb) { 
  struct sockaddr_storage addr;
  int err;

  err = _addr(ip, port, &addr);
  if (err) {
    LOG_UVERR("addr", err);
    return err;
  }

  // XXX leaks obvi
  uv_tcp_t *server = calloc(1, sizeof(*server));
  server->data = calloc(1, sizeof(_server_data_t));
  // stash callback into scheme on connection
  SERVER_DATA(server)->Sconnected_cb = Scb; 

  err = uv_tcp_init(uv_default_loop(), server);
  if (err) {
    LOG_UVERR("init listen", err);
    return err;
  }
  
  err = uv_tcp_bind(server, &addr, 0);
  if (err) {
    LOG_UVERR("binding addr", err);
    return err;
  }

  err = uv_listen(server, DEFAULT_BACKLOG, _uv_connected_cb);
  if (err) {
    LOG_UVERR("listening", err);
    return err;
  }

  return err;
}

void _uv_connect_cb(uv_connect_t *connect, int status) { 
  if (status < 0) {
    LOG_ERR("connect");
    return;
  }

  uv_tcp_t *client = CONNECT_DATA(connect)->client;
  client->data = calloc(1, sizeof(_client_data_t));

  CONNECT_DATA(connect)->Sconnect_cb(client);

  // connect is no longer needed
  Sunlock_code_pointer(CONNECT_DATA(connect)->Sconnect_cb);
  free(connect->data);
  free(connect);
}

int suv_connect(const char* ip, int port, _Sconnect_cb Scb) { 
  struct sockaddr_storage addr;
  int err;

  err = _addr(ip, port, &addr);
  if (err) {
    LOG_UVERR("addr", err);
    return err;
  }

  uv_connect_t *connect = calloc(1, sizeof(*connect));
  connect->data = calloc(1, sizeof(_connect_data_t));
  CONNECT_DATA(connect)->Sconnect_cb = Scb; 

  uv_tcp_t *client = calloc(1, sizeof(*client));
  err = uv_tcp_init(uv_default_loop(), client);
  if (err) {
    LOG_UVERR("init connect", err);
    return err;
  }

  CONNECT_DATA(connect)->client = client;
  err = uv_tcp_connect(connect, client, &addr, _uv_connect_cb);
  if (err) {
    LOG_UVERR("connect", err);
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
    
