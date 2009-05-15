#ifndef _CONN_H_
#define _CONN_H_

#define CONN_TYPE_UNKNOWN 1
#define CONN_TYPE_LISTEN 2
#define CONN_TYPE_HTTP 3
#define CONN_TYPE_SSH 4
#define CONN_TYPE_TELNET 5
#define CONN_TYPE_MANAGE 6

#define CONN_FLAGS_NONBLOCKING 1
#define CONN_FLAGS_ACCEPTING 2
#define CONN_FLAGS_CONNECTING 4
#define CONN_FLAGS_INUSE 8
#define CONN_FLAGS_CLOSEIN 16
#define CONN_FLAGS_CLOSEOUT 32
#define CONN_FLAGS_GONE 64

struct conn_t {
  struct connlist_t *connlist;
  int fd[2];
  int type;
  unsigned long flags;
  unsigned char *buffer[2];
  int bufsize[2];
  int curpos[2];
  struct conn_t *next;
  long time;
  int (*do_send)(struct conn_t *, int);
  int (*do_recv)(struct conn_t *, int);
  int (*rx_handler)(struct pm_session_t *, struct conn_t *, int);
};


struct connlist_t {
  struct conn_t *root;
  struct conn_t **rootp;
  int flags;
  void *priv;
};


struct conn_t *conn_add(struct connlist_t *, int, int, unsigned long);
struct conn_t *conn_find_byfd(struct connlist_t *, int);
int conn_free(struct connlist_t *, struct conn_t *) ;
int do_prune(struct conn_t *);
int do_accept(struct conn_t *, int);
int do_connect(struct conn_t *, int);
int do_recv(struct conn_t *, int);
int _do_send(struct conn_t *, int);
struct connlist_t *connlist_init(struct connlist_t *);

#endif /* def _CONN_H_ */
