#ifndef _MISC_H_
#define _MISC_H_

int listenestablish(unsigned short);
int handle_event(struct pm_session_t *sess, struct pollfd *pfd);
int list_poll(struct pm_session_t *sess);


#endif /* def _MISC_H_ */
