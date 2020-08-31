#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <spawn.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <fstream>
#include <string>
#include <sstream>
#include <iostream>
#include "support/debug.h"
#include "support/consts.h"
#include "support/timers.h"
#include "support/c_utils.h"

using namespace std;

int
main(int argc, char **argv)
{
  int i, ret;

  ASSERT(argc == 3);
  int input_fd = atoi(argv[1]);
  int output_fd = atoi(argv[2]);
  int yices_id, z3_id;
  //printf("smt_helper_process: input_fd = %d, output_fd = %d\n", input_fd, output_fd);
  CPP_DBG_EXEC(SMT_HELPER_PROCESS, printf("smt_helper_process: input_fd = %d, output_fd = %d\n", input_fd, output_fd));
  fflush(stdout);

  while (1) {
#define RECV_SIZE SMT_HELPER_PROCESS_MSG_SIZE
    char message[RECV_SIZE];
    char *ptr, *end;
    ssize_t iret;
    int max_iter = 1024, iter;
    ptr = message;
    end = ptr + sizeof message;
    iter = 0;
    do {
      iret = read(input_fd, message, sizeof message);
      ASSERT(iret >= 0);
      if (!(iret > 0 || errno == EAGAIN || errno == EINTR || errno == EWOULDBLOCK)) {
        cout << __func__ << " " << __LINE__ << ": iret = " << iret << ": errno = " << errno << endl;
      }
      ASSERT(iret > 0 || errno == EAGAIN || errno == EINTR || errno == EWOULDBLOCK);
      ptr += iret;
      iter++;
    } while (((ptr == message || *(ptr - 1) != '\0')) && iter<= max_iter);
    ASSERT(iter < max_iter);
    xsystem(message);
    int rc = write(output_fd, "\0", 1);
    ASSERT(rc == 1);
  }
}
