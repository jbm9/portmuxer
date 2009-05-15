#ifndef _PM_H_
#define _PM_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#include <sys/poll.h>

#include <errno.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT_IN 80
#define SSH_HOST "localhost"
#define SSH_PORT 22
#define HTTP_HOST "localhost"
#define HTTP_PORT 81

#define BUFSIZE_DEFAULT 4096
#define IN 0
#define OUT 1
#define CHUNK_SIZE 3

#define TIME_INT 15
#define TIME_OUT 30

#define swap_direction(x) ( (x) == IN ? OUT : IN )

#include "session.h"
#include "conn.h"
#include "http.h"
#include "ssh.h"
#include "manage.h"
#include "misc.h"
#endif /* ndef _PM_H_ */
