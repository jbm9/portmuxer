#include "pm.h"

struct conn_t *conn_add(struct connlist_t *connlist, int fd, int type, unsigned long flags)
{
  struct conn_t *cur;
  long args;
  
  if(!connlist)
    return NULL;

  if(!(cur = malloc(sizeof(struct conn_t)))) {
    perror("malloc");
    return NULL;
  }

  cur->fd[IN] = fd;
  cur->fd[OUT] = -1;
  cur->type = type;
  cur->flags = flags;
  cur->bufsize[IN] = BUFSIZE_DEFAULT;
  cur->bufsize[OUT] = BUFSIZE_DEFAULT;
  cur->curpos[IN] = 0;
  cur->curpos[OUT] = 0;
  cur->time = time(NULL);
  cur->do_send = _do_send;
  cur->connlist = connlist;
  cur->do_recv = do_recv;

  if(connlist->flags & CONN_FLAGS_NONBLOCKING) {
    args = fcntl(fd, F_GETFL);
    fcntl(fd, F_SETFL, (args | O_NONBLOCK));
    cur->flags |= CONN_FLAGS_NONBLOCKING;
  }

  //  printf("** add_conn: %p: %d %d/%d @ %ld\n", cur, cur->fd[IN], cur->type, cur->flags, cur->time);
  //  printf("add_conn: %ld\n", cur->time);

  if(!(cur->buffer[IN] = malloc(cur->bufsize[IN]))) {
    perror("malloc");
    free(cur);
    return NULL;
  }
  if(!(cur->buffer[OUT] = malloc(cur->bufsize[OUT]))) {
    perror("malloc");
    free(cur->buffer[IN]);
    free(cur);
    return NULL;
  }

  cur->next = *(connlist->rootp);
  *(connlist->rootp) = cur;    
  connlist->root = *(connlist->rootp);
  
  return *(connlist->rootp);
}


struct conn_t *conn_find_byfd(struct connlist_t *connlist, int fd)
{
  struct conn_t *cl_root;
  struct conn_t *cur;
  
  if(!connlist)
    return NULL;

  cl_root = connlist->root;

  //  printf("find by fd: %d\n", fd);

  for(cur = cl_root; cur; cur = cur->next)
    if(cur->fd[IN] == fd || cur->fd[OUT] == fd)
      return cur;

  return NULL;
}

int conn_free(struct connlist_t *connlist, struct conn_t *gone) 
{
  struct conn_t *cur, *cl_root;

  //  printf("free conn: %p\n", gone);

  if(!connlist)
    return -1;

  cl_root = connlist->root;

  if(cl_root == NULL)
    return 0;

  if (gone == cl_root)
    cl_root = cl_root->next;
  else 
    for(cur = cl_root; cur; cur = cur->next)
      if(cur->next == gone)
	cur->next = cur->next->next;

  free(gone->buffer[IN]);
  free(gone->buffer[OUT]);

  if(gone->fd[IN] != -1)
    close(gone->fd[IN]);
  if(gone->fd[OUT] != -1)
    close(gone->fd[OUT]);

  free(gone);
  *(connlist->rootp) = cl_root;
  connlist->root = cl_root;
  return 0;
}


int do_prune(struct conn_t *conn)
{
  //  printf("XXX Pruning conn %d/%d\n", conn->fd[IN], conn->fd[OUT]);
  return conn_free(conn->connlist, conn);
}


int do_accept(struct conn_t *conn, int i)
{
  struct sockaddr sa;
  socklen_t addrlen = sizeof(sa);
  int acceptfd;

  struct hostent;
  struct conn_t *newconn;
  struct connlist_t *connlist;
  unsigned long flags;
  
  //  printf("do_accept: %d (0x%p)/", conn->fd[IN], conn);

  if(!conn)
    return -1;

  connlist = conn->connlist;

  if((acceptfd = accept(conn->fd[IN], &sa, &addrlen)) == -1) {
    if(errno != EAGAIN && errno != EINTR) {
      perror("accept");
      return -1;
    } else {
      return 0;
    }
  }

  //  printf("%d\n", acceptfd);
  printf("Connection from %s (%d)\n", inet_ntoa(((struct sockaddr_in *)&sa)->sin_addr), acceptfd );

  flags = CONN_FLAGS_CONNECTING;
  if(connlist->flags & CONN_FLAGS_NONBLOCKING)
    flags |= CONN_FLAGS_NONBLOCKING;

  if((newconn = conn_add(connlist, acceptfd, CONN_TYPE_UNKNOWN, flags)) == NULL) {
    return -1;
  }

  newconn->do_recv = do_connect;

  return 0;
}


int do_connect(struct conn_t *conn, int i)
{
  if(!(conn))
    return -1;

  //    printf("do_connect: %d/%d\n", conn->fd[IN], conn->fd[OUT]);
  conn->flags |= (CONN_FLAGS_INUSE);
  conn->flags &= (~CONN_FLAGS_CONNECTING);
  conn->do_recv = do_recv;

  return 0;
}

int do_recv(struct conn_t *curconn, int direction)
{
  int fd;
  int i, j;
  unsigned char *bufpos;

  struct pm_session_t *sess;
  struct connlist_t *connlist;

  if(!curconn) {
    printf("no current conn\n");
    return -1;
  }

  if(!(connlist = curconn->connlist)) {
    printf("no current connlist\n");
    return -1;
  }

  if(!(sess = (struct pm_session_t *)connlist->priv)) {
    printf("no current session\n");
    return -1;
  }

  fd = curconn->fd[direction];

  j = curconn->curpos[direction];
  bufpos = curconn->buffer[direction]+j;

  if(j == curconn->bufsize[direction]) {
    //    printf("overflow averted on %d->%d! %d %d\n", curconn->fd[direction], curconn->fd[swap_direction(direction)], j, curconn->bufsize[direction]);
    return curconn->do_send(curconn, direction);
}
  
  i = recv(fd, bufpos, (curconn->bufsize[direction] - j), MSG_DONTWAIT);

  //  printf("i: %d\n", i);

  if (i < 0) {
    if(errno == EAGAIN) {
      //printf("recv nonblocking read waited\n");
    } else {
      printf("recv error... %d\n", errno);
      perror("recv");
      return -1;
    }
  } else if(i == 0) {
    //    printf("EOF on %d %d\n", fd, direction);
    close(fd);
    curconn->fd[direction] = -1;

    if(direction == IN) {
      curconn->flags |= CONN_FLAGS_CLOSEIN;
    } else {
      curconn->flags |= CONN_FLAGS_CLOSEOUT;
    }
    if(curconn->fd[swap_direction(direction)] == -1) {
      return 0;
    }
    
    return curconn->do_send(curconn, direction);  
  } else {
    curconn->curpos[direction] += i;
    if(curconn->type == CONN_TYPE_UNKNOWN) {
      char *c = curconn->buffer[direction];     
      if(j +i > 3) {
	if(strncmp(c, "POS", 3) == 0 ||
	   strncmp(c, "GET", 3) == 0) {
	  make_http(sess, curconn);
	} else if(strncmp(c, "hi", 2) == 0) {
	  make_manage(sess, curconn);
	} else {
	  printf("unknown magic %x%x%x on fd %d\n", c[0], c[1], c[2], curconn->fd[direction]);
	}
      }    
    } else {
      return curconn->rx_handler(sess, curconn, direction);
    }
  }
  return i;
}


int _do_send(struct conn_t *conn, int direction)
{
  int i;

  //  printf("do_send: %p %d", conn, direction);

  if(!conn)
    return -1;
  
  //  printf(" %d/%d (bytes: %d/%d)\n", conn->fd[IN], conn->fd[OUT], conn->curpos[IN], conn->curpos[OUT]);

  if( (direction == IN && conn->flags & CONN_FLAGS_CLOSEOUT) || 
      (direction == OUT && conn->flags & CONN_FLAGS_CLOSEIN)) {
    printf("um. printing to a closed connection. this is probably a BUG\n");
    conn->curpos[direction] = 0;
    return 0;
  }


  if(conn->curpos[direction] < 0) {
    printf("BUG!: somehow or another, curpos for this conn got below 0. this is a BUG.\n");    
    return -1;
  } else if (conn->curpos[direction] == 0) {
    //    printf("nothing to send\n");
    return 0;
  } else {
    i = send(conn->fd[swap_direction(direction)], conn->buffer[direction], conn->curpos[direction], MSG_DONTWAIT);

    if(i < 0) {
      if(conn->flags & CONN_FLAGS_NONBLOCKING)
	if(errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)
	  return 0;    
      printf("send error: %d\n", errno);
      perror("send");
      return -1;
    } else if(i > 0) {	
      conn->curpos[direction] -= i;
      if(!(conn->buffer[direction] = 
	   memmove(conn->buffer[direction], (conn->buffer[direction] + i), conn->curpos[direction]))) {	
	perror("memmove");
	return -1;
      }
      memset(conn->buffer[direction]+conn->curpos[direction], 0, conn->bufsize[direction]-conn->curpos[direction]);
    }
  }
  	//printf("i: %d\n", i);
  return i;
}


struct connlist_t *connlist_init(struct connlist_t *connlist)
{
  if(!connlist)
    if(!(connlist = malloc(sizeof(struct connlist_t))))
      return NULL;

  memset(connlist, 0, sizeof(struct connlist_t));
  connlist->flags = CONN_FLAGS_NONBLOCKING;
  connlist->root = NULL;
  connlist->rootp = &connlist->root;
  return connlist;
}
