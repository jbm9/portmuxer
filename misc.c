#include "pm.h"
/**
 * aim_listenestablish - create a listening socket on a port.
 * @portnum: the port number to bind to.  
 *
 * you need to call accept() when it's connected. returns your fd 
 *
 */
int listenestablish(unsigned short portnum)
{
#if defined(__linux__)
	/* XXX what other OS's support getaddrinfo? */
	int listenfd;
	const int on = 1;
	struct addrinfo hints, *res, *ressave;
	char serv[5];

	snprintf(serv, sizeof(serv), "%d", portnum);
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_flags = AI_PASSIVE;
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	if (getaddrinfo(NULL /*any IP*/, serv, &hints, &res) != 0) {
		perror("getaddrinfo");
		return -1;
	} 
	ressave = res;
	do { 
		listenfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);    
		if (listenfd < 0)
			continue;
		setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
		if (bind(listenfd, res->ai_addr, res->ai_addrlen) == 0)
			break;
		/* success */
		close(listenfd);
	} while ( (res = res->ai_next) );

	if (!res)
		return -1;

	if (listen(listenfd, 1024)!=0) { 
		perror("listen");
		return -1;
	} 

	freeaddrinfo(ressave);
	return listenfd;
#else
	int listenfd;
	const int on = 1;
	struct sockaddr_in sockin;

	if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket(listenfd)");
		return -1;
	}

	if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on)) != 0) {
		perror("setsockopt(listenfd)");
		close(listenfd);
		return -1;
	} 

	memset(&sockin, 0, sizeof(struct sockaddr_in));
	sockin.sin_family = AF_INET;
	sockin.sin_port = htons(portnum);

	if (bind(listenfd, (struct sockaddr *)&sockin, sizeof(struct sockaddr_in)) != 0) {
		perror("bind(listenfd)");
		close(listenfd);
		return -1;
	}
	if (listen(listenfd, 4) != 0) {
		perror("listen(listenfd)");
		close(listenfd);
		return -1;
	}
	return listenfd;
#endif
}


int handle_event(struct pm_session_t *sess, struct pollfd *pfd) 
{
  int fd = 0;
  int direction;
  int events;

  struct conn_t *curconn;
  struct connlist_t *connlist;

  if(!sess || !sess->connlist)
    return -1;

  connlist = sess->connlist;

  fd = pfd->fd;
  events = pfd->revents;

  //  printf("Handling events 0x%x on %d!\n", events, fd);

  if(!(curconn = conn_find_byfd(connlist, fd))) {
    printf("invalid fd -- no conn!\n");
    return -1;
  }

  
  if(fd == curconn->fd[IN]) {
    direction = IN;
  } else if(fd == curconn->fd[OUT]) {
    direction = OUT;
  } else {
    printf("BUG umm.. have an invalid conn here at 0x%p(%d/%d), seeking fd %d\n", curconn, curconn->fd[IN], curconn->fd[OUT], fd);
    return -1;
  }

  if(events & POLLIN) {
    return curconn->do_recv(curconn, direction);
  }
  
  return 0;
}

int list_poll(struct pm_session_t *sess)
{
  struct pollfd *curpfd = NULL ;
  int n = 0;
  int i;

  struct connlist_t *connlist;
  struct conn_t *cl_root;
  struct conn_t *curconn;

  if(!sess || !sess->connlist)
    return -1;

  connlist = sess->connlist;
  cl_root = connlist->root;

  for(curconn = cl_root; curconn; curconn = curconn->next) {
    if(!(curpfd = realloc(curpfd, ((n+2) * sizeof(struct pollfd))))) /* parens!! */
      return -1;

    if(curconn->flags & CONN_FLAGS_CLOSEIN || curconn->flags & CONN_FLAGS_CLOSEOUT) {
      if(curconn->flags & CONN_FLAGS_CLOSEIN && curconn->flags & CONN_FLAGS_CLOSEOUT)
	do_prune(curconn);
      
      if(curconn->flags & CONN_FLAGS_CLOSEIN) {
	if(curconn->curpos[IN]) {
	  if(curconn->do_send(curconn, IN) == -1)
	    do_prune(curconn);
	} else
	  do_prune(curconn);
      } else if(curconn->flags & CONN_FLAGS_CLOSEOUT) {
	if(curconn->curpos[OUT]) {
	  if(curconn->do_send(curconn, OUT) == -1)
	    do_prune(curconn);
	} else
	  do_prune(curconn);
      }
      break;
    }

    if(curconn->fd[IN] != -1) {
      curpfd[n].fd = curconn->fd[IN];
      curpfd[n].events = (POLLIN|POLLHUP);
    } else
      n--;

    if(curconn->fd[OUT] != -1) {	
      curpfd[n+1].fd = curconn->fd[OUT];
      curpfd[n+1].events = (POLLIN|POLLHUP);
    } else 
      n--;
    n += 2;
  }

  //  printf("polling %d conns\n", n);
  
  if(poll(curpfd, n, 1000))
    for(i = 0; i < n; i++)
      if(curpfd[i].revents)
	if(handle_event(sess, &curpfd[i]) < 0) {
	  struct conn_t *prune;
	  int fd;
	  
	  fd = curpfd[i].fd;
	  
	  if(!(prune = conn_find_byfd(connlist, fd))) {
	    printf("invalid fd -- no conn!\n");
	    return -1;
	  }
	  
	  if(prune->flags & CONN_FLAGS_CLOSEIN && prune->flags & CONN_FLAGS_CLOSEOUT) {
	    do_prune(prune);
	    printf("XXX: we ought to handle this more gracefully...\n");	     
	  } else {	    
	    int j;
	    if(fd == prune->fd[IN]) {
	      prune->flags |= CONN_FLAGS_CLOSEIN;
	      while((j = curconn->do_send(curconn, IN))) { if (j == -1) {do_prune(curconn); }};
	    } else if(fd == prune->fd[OUT]) {
	      prune->flags |= CONN_FLAGS_CLOSEOUT;
	      while((j = curconn->do_send(curconn, OUT))) { if (j == -1) {do_prune(curconn); }};
	    } else
	      printf("BUG: prune conn doesnt' contain fd to remove!\n");
	   }	    
	}
  
  return 0;
}
