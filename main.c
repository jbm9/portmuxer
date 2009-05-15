#include "pm.h"

int do_timer(struct pm_session_t *sess, time_t curtime)
{
  struct conn_t *curconn, *cl_root;
  struct connlist_t *connlist;


  if(!sess || !sess->connlist)
    return -1;
  
  //printf("do_timer: %ld %p\n", curtime, sess);

  connlist = sess->connlist;

  cl_root = connlist->root;

  for(curconn = cl_root; curconn; curconn = curconn->next) {
    if(curconn->flags & CONN_FLAGS_CONNECTING && curconn->type == CONN_TYPE_UNKNOWN) {
      if((curtime - curconn->time) > TIME_OUT) {
	return make_ssh(sess, curconn);
      }
    }
  }
  return 0;
}

int main(int argc, char *argv[]) {
  int fd;
  int stayconn = 1;
  static time_t lasttime = 0;
  time_t curtime;

  struct pm_session_t sess;
  struct conn_t *listener;

  if(!(session_init(&sess)))
    return -1;

  close(0); /* XXX */

  printf("portmuxer version 0.1\n");
  printf("Copyright (C) 2002 Josh Myer <josh@joshisanerd.com>\n");
  printf("Distributed under the terms of the GPL, see COPYING for details\n");
  printf("\n");


  printf("Establishing Listener (%d)...", PORT_IN);
  if((fd = listenestablish(PORT_IN)) < 0) {
    perror("listenestablish: something");
    return -1;
  }
  printf(" done (%d)\n", fd);

  if(!(listener = conn_add(sess.connlist, fd, CONN_TYPE_LISTEN, CONN_FLAGS_ACCEPTING))) {
    perror("conn_add");
    return -1;
  }

  listener->do_recv = do_accept;

  while(stayconn) {
    curtime = time(NULL);
    if((curtime - lasttime) > TIME_INT) {
      lasttime = curtime;
      do_timer(&sess, curtime);
    }
    if(list_poll(&sess) == -1)
      return -1;
    fflush(stdin);
    fflush(stdout);
    fflush(stderr);
  }

  return 0;
}
