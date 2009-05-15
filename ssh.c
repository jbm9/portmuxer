#include "pm.h"

int do_ssh(struct pm_session_t *sess, struct conn_t *conn, int direction)
{
  return conn->do_send(conn, direction);
}


int make_ssh(struct pm_session_t *sess, struct conn_t *cur)
{
  
  int fd;

  struct sockaddr_in sa;
  struct hostent *hp;

  if(!sess || !sess->connlist)
    return -1;

  if(!(hp = gethostbyname(SSH_HOST))) {
    printf("connect: unable to resolve %s... name\n", SSH_HOST);
    return -1;
  }

  memset(&sa, 0, sizeof(sa));
  sa.sin_port = htons(SSH_PORT);
  memcpy(&sa.sin_addr, hp->h_addr, hp->h_length);
  sa.sin_family = hp->h_addrtype;

  fd = socket(hp->h_addrtype, SOCK_STREAM, 0);
  if(connect(fd, (struct sockaddr *)&sa, sizeof(struct sockaddr_in)) < 0) {

    printf("connect: unable to connect to host\n");
    close(fd);
    return -1;
  }

  cur->type = CONN_TYPE_SSH;
  
  cur->fd[OUT] = fd;

  cur->do_send = _do_send;
  cur->rx_handler = do_ssh;
 
  return cur->do_send(cur, IN);
}

