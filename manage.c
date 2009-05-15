
#include "pm.h"

static const char *conntypes[] = { "", "Unknown", "Listener", "HTTP", "SSH", "Telnet", "Management" };
//static const char *connflags[] = { "", "Accepting", "Connecting", "In Use", "Closed Input", "Closed Output", "Dead" };

int do_manage(struct pm_session_t *sess, struct conn_t *conn, int direction)
{
  char *reply;
  char *cmd;

  int j, bufsize;
  struct conn_t *cl_root;
  struct conn_t *cur;

  if(!sess || !sess->connlist)
    return -1;

  /* we use this in the management below */
  cl_root = sess->connlist->root;

  j = conn->curpos[swap_direction(direction)];
  bufsize = conn->bufsize[swap_direction(direction)];

  if(!(conn))
    return -1;

  /* XXX Command Queueing... or add a lock here */

  cmd = conn->buffer[direction];
  reply = conn->buffer[swap_direction(direction)];

  memset(reply, 0, conn->bufsize[swap_direction(direction)]);
  //  printf("do_manage: %p (%d/%d) \"%s\" \"%s\"\n", conn, conn->curpos[direction], conn->curpos[swap_direction(direction)], cmd, reply);

  
  if(strncasecmp(cmd, "goodday", 7) == 0) {
    char *c = "Good day to you, too.\n";
    memcpy(reply, c, strlen(c));
  } else if(strncasecmp(cmd, "echo", 4) == 0) {
    memcpy(reply, cmd+4, strlen(cmd)-4);    
  } else if(strncasecmp(cmd, "connlist", 8) == 0) {
    int pos = 0, clen = 0;
    char c[128];

   
    strncpy(c, "Connection List Follows:\n", sizeof(c));

    reply = strncat(reply, c, bufsize-pos);
    pos = strlen(reply);
    for(cur = cl_root; cur; cur = cur->next) {
      memset(c, 0, sizeof(c));
      clen = snprintf(c, 127, "%d/%d: Type: %s(%x)   Flags: %lx\n\tbuf: I %d/%d O %d/%d\n\tCreated: %ld\n\n", 
		      cur->fd[IN], cur->fd[OUT], conntypes[cur->type], cur->type, cur->flags,
		      cur->curpos[IN], cur->bufsize[IN], cur->curpos[OUT], cur->bufsize[OUT], cur->time);
      strncat(reply, c, bufsize-pos);
      pos += clen;

    }      
  } else {
    char c[256];

    snprintf(c, 255, "unknown command: \"%s\"\n", cmd);
    
    memcpy(reply, c, strlen(c));
  }
  
  conn->curpos[direction] = 0;
  conn->curpos[swap_direction(direction)] = strlen(reply);

  memset(cmd, 0, strlen(cmd)+1);
  
  return conn->do_send(conn, swap_direction(direction));
}

int make_manage(struct pm_session_t *sess, struct conn_t *cur)
{  
  //  printf("make_manage: %p\n", cur);

  cur->type = CONN_TYPE_MANAGE;
  
  cur->fd[OUT] = -1;

  memset(cur->buffer[IN], 0, cur->bufsize[IN]);
  memset(cur->buffer[OUT], 0, cur->bufsize[OUT]);

  cur->rx_handler = do_manage;

  return 0;
}



