
#include "pm.h"


int do_http(struct pm_session_t *sess, struct conn_t *conn, int direction)
{
  return conn->do_send(conn, direction);
}


int make_http(struct pm_session_t *sess, struct conn_t *cur)
{
  
  int fd;

  struct sockaddr_in sa;
  struct hostent *hp;

  if(!sess || !sess->connlist)
    return -1;

  //  printf("make_http %p\n", cur);

  if(!(hp = gethostbyname(HTTP_HOST))) {
    printf("connect: unable to resolve %s... name\n", HTTP_HOST);
    return -1;
  }

  memset(&sa, 0, sizeof(sa));
  sa.sin_port = htons(HTTP_PORT);
  memcpy(&sa.sin_addr, hp->h_addr, hp->h_length);
  sa.sin_family = hp->h_addrtype;

  fd = socket(hp->h_addrtype, SOCK_STREAM, 0);
  /* XXX: connect needs to be abstracted out here  */
  if(connect(fd, (struct sockaddr *)&sa, sizeof(struct sockaddr_in)) < 0) {

    printf("connect: unable to connect to host\n");
    close(fd);
    return -1;
  }

  cur->type = CONN_TYPE_HTTP;
  
  cur->fd[OUT] = fd;

  cur->do_send = _do_send;
  cur->rx_handler = do_http;
  
  return cur->do_send(cur, IN);
}
