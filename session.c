#include "pm.h"

struct pm_session_t *session_init(struct pm_session_t *sess)
{
  if(!sess)
    if(!(sess = malloc(sizeof(struct pm_session_t))))
      return NULL;

  memset(sess, 0, sizeof(struct pm_session_t));
  
  if(!(sess->connlist = connlist_init(sess->connlist))) {
    free(sess);
    return NULL;
  }

  sess->connlist->priv = (void *)sess;

  return sess;
}
     
