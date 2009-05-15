#ifndef _SESSION_H_
#define _SESSION_H_

struct pm_session_t {
  struct connlist_t *connlist;
};

struct pm_session_t *session_init(struct pm_session_t *);

#endif /* def _SESSION_H_ */
