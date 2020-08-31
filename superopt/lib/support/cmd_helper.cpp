#include "support/cmd_helper.h"
#include "config-host.h"
#include "support/debug.h"
#include "unistd.h"
#include <fstream>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <spawn.h>
#include <iostream>
#include <errno.h>
#include <string.h>

using namespace std;

static int g_cmd_helper_pid = -1;
static int send_channel[2], recv_channel[2];

void
cmd_helper_init()
{
  int rc;
  posix_spawn_file_actions_t child_actions;

  rc = pipe(send_channel);
  ASSERT(rc == 0);

  rc = pipe(recv_channel);
  ASSERT(rc == 0);

  posix_spawn_file_actions_init(&child_actions);
  posix_spawn_file_actions_addclose(&child_actions, recv_channel[0]);
  posix_spawn_file_actions_addclose(&child_actions, send_channel[1]);

  char *process_name = (char *)(BUILD_DIR "/cmd_helper_process");
  char in_fd_buf[16], out_fd_buf[16];
  snprintf(out_fd_buf, sizeof out_fd_buf, "%d", send_channel[0]);
  snprintf(in_fd_buf, sizeof in_fd_buf, "%d", recv_channel[1]);
  char * const argv[] = { process_name, out_fd_buf, in_fd_buf, NULL };

  CPP_DBG_EXEC(SMT_HELPER_PROCESS, cout << "calling posix_spawn on " << process_name << endl);
  int status = posix_spawn(&g_cmd_helper_pid, process_name, &child_actions, NULL, argv, ::environ);
  if (status != 0) {
    printf("posix_spawn failed! errno=%s\n", strerror(errno));
  }
  CPP_DBG_EXEC(SMT_HELPER_PROCESS, cout << "posix process " << process_name << " spawned at pid " << g_cmd_helper_pid << endl);
  ASSERT(status == 0);
  ASSERT(g_cmd_helper_pid != -1);
  close(recv_channel[1]);
  close(send_channel[0]);
}

void
cmd_helper_kill()
{
  if (g_cmd_helper_pid != -1) {
    //printf("%s() %d: killing g_helper_pid = %d\n", __func__, __LINE__, g_helper_pid);
    int rc = kill(g_cmd_helper_pid, SIGKILL);
    ASSERT(rc == 0);
    g_cmd_helper_pid = -1;
    //printf("%s() %d: done killing g_helper_pid = %d\n", __func__, __LINE__, g_helper_pid);
  }
}

void
cmd_helper_issue_command(char const *cmd)
{
  ASSERT(g_cmd_helper_pid != -1);
  int oret = write(send_channel[1], cmd, strlen(cmd) + 1);
  ASSERT(oret == strlen(cmd) + 1);
  char buffer[1];
  int num_iter = 0;
  int rc;
  do {
    rc = read(recv_channel[0], buffer, 1);
    ASSERT(rc >= 0);
    if (rc == 0 && errno != EAGAIN) {
      printf("errno = %d %s\n", errno, strerror(errno));
    }
    ASSERT(rc > 0 || errno == EAGAIN || errno == EINTR || errno == EWOULDBLOCK);
    num_iter++;
    ASSERT(num_iter < 1024);
  } while (rc == 0);
  ASSERT(rc == 1);
}
