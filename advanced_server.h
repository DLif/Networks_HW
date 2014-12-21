#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>

#include "buffered_socket.h"
#include "num_protocol_tools.h"
#include "IO_buffer.h"

#define NETWORK_FUNC_FAILURE -1 /* error code */
#define AWATING_CLIENTS_NUM   10 /* connection queue size+ 1 to send "not able to connect"*/
#define MAX_CLIENT_NUM 25


#define __DEBUG__