#include <sys/types.h>
#include <sys/time.h>
#include <spawn.h>
#include <signal.h>
#include <unistd.h>
#include <string>
#include <fstream>
#include <iostream>

#include "support/sage_interface.h"
#include "config-host.h"
#include "expr/context.h"
#include "support/globals_cpp.h"
#include "support/mybitset.h"
#include "support/debug.h"
#include "support/bv_const.h"

extern char **environ;

namespace eqspace
{

sage_interface::sage_interface(context *ctx) : m_context(ctx), m_helper_pid(-1)
{ }

sage_interface::~sage_interface()
{
  if (m_helper_pid != -1) {
    // kill helper process
    int rc = kill(m_helper_pid, SIGKILL);
    ASSERT(rc == 0);
    m_helper_pid = -1;
  }
}

/* TODO add as static members in sage_interface? */

int
sage_interface::sage_helper_init(void)
{
  int rc, helper_pid;
  posix_spawn_file_actions_t child_actions;

  /* create send and recv pipes */
  rc = pipe(send_channel);
  ASSERT(rc == 0);

  rc = pipe(recv_channel);
  ASSERT(rc == 0);

  posix_spawn_file_actions_init(&child_actions);
  posix_spawn_file_actions_addclose(&child_actions, recv_channel[0]); /* close pipe read end */
  posix_spawn_file_actions_addclose(&child_actions, send_channel[1]); /* close pipe write end */

  char *const process_name = (char *)(BUILD_DIR "/" SAGE_DRIVER);
  char in_fd_buf[16], out_fd_buf[16];
  snprintf(out_fd_buf, sizeof out_fd_buf, "%d", send_channel[0]);
  snprintf(in_fd_buf, sizeof in_fd_buf, "%d", recv_channel[1]);
  char * const argv[] = { process_name, out_fd_buf, in_fd_buf, NULL };

  int status = posix_spawn(&helper_pid, process_name, &child_actions, NULL, argv, ::environ);
  if (status != 0) {
    cout << "posix_spawn failed! errno=" << strerror(errno) << endl;
  }
  ASSERT(status == 0);
  ASSERT(helper_pid != -1);
  close(recv_channel[1]); /* close pipe write end */
  close(send_channel[0]); /* close pipe read end */
  return helper_pid;
}


static void
read_into_string(int fd, std::string& out)
{
  /* get the # of bytes to read */
  size_t bytes_to_read;
  int nread = read(fd, &bytes_to_read, sizeof bytes_to_read);
  ASSERT(nread == sizeof bytes_to_read);

  std::vector<char> buf(bytes_to_read, 0);  /* initialize to size `bytes_to_read` */
  char *ptr = &buf[0];
  char *end = &buf[0] + bytes_to_read;
  do {
    nread = read(fd, ptr, end-ptr);
    if (   (nread < 0 && errno != EINTR)   /* retry on EINTR -- exit for others */
        || (nread == 0)) {                 /* exit on EOF */
      break;
    }
    else if (nread > 0) ptr += nread;
  } while (ptr != end);
  out.insert(out.end(), buf.begin(), buf.end()); /* flush contents of buffer into out and return */
}

#define SEND_SIZE SMT_HELPER_PROCESS_MSG_SIZE

sage_interface::sage_query_res
sage_interface::spawn_sage_query(string filename, unsigned timeout_secs, std::vector<vector<bv_const>>& result)
{
  int rc;
  char *ptr, *end;
  char out_buffer[SEND_SIZE];

  if (m_helper_pid == -1) {
    m_helper_pid = sage_helper_init();
  }
  ASSERT(m_helper_pid != -1);

  /* prepare the buffer to be sent */

  ASSERT(filename.size() <= SEND_SIZE - 5);        /* 1 null byte + 4 timeout bytes */
  ptr = out_buffer;
  end = ptr + sizeof out_buffer;
  strncpy(ptr, filename.c_str(), end-ptr);         /* write the filename */
  ptr += strlen(ptr) + 1;
  while (ptr < end - 4) {                          /* pad with 0-bytes */
    *ptr = '\0';
    ptr++;
  }
  *(uint32_t *)ptr = timeout_secs;                 /* 4 bytes of timeout at the end */
  ptr += 4; /* ptr == end */

  /* send the buffer */

  int oret = write(send_channel[1], out_buffer, ptr - out_buffer);
  ASSERT(oret == ptr - out_buffer);

  /* read the result */

  char in_header[8];
  ptr = in_header;
  end = ptr + sizeof in_header;
  do {
    rc = read(recv_channel[0], ptr, end-ptr);
    if (rc < 0) {
      if (errno != EINTR) {
        cout << "errno = " << errno << " (" << strerror(errno) << "),  ptr - in_header = " << (ptr - in_header) << endl;
        ASSERT(0);
      }
    }
    else if (rc > 0) ptr += rc;
    else /* rc == 0 => EOF */ break;
  } while (ptr != end);

  if (!memcmp(in_header, "result", sizeof "result" - 1)) {
    string sres;
    read_into_string(recv_channel[0], sres);
    parse_bv_const_list_list(sres, result);
    return sage_interface::OK;
  }
  else if (!memcmp(in_header, "none", sizeof "none" -1)) {
    return sage_interface::OK; /* no solution is also OK :) */
  } else if (!memcmp(in_header, "timeout", sizeof "timeout" - 1)) {
    return sage_interface::TIMEOUT;
  } else if (!memcmp(in_header, "error", sizeof "error" - 1)) {
    return sage_interface::ERROR;
  } else NOT_REACHED();
}


static string sage_query_res_to_string(sage_interface::sage_query_res r)
{
  switch(r) {
    case sage_interface::TIMEOUT:
      return "TIMEOUT";
    case sage_interface::OK:
      return "OK";
    case sage_interface::ERROR:
      return "ERROR";
    default:
      return "UNKNOWN";
  }
}

bool
sage_interface::get_nullspace_basis(const std::vector<std::vector<bv_const>>& points, const bv_const& mod, query_comment const& qc, vector<vector<bv_const>>& soln)
{
  long long elapsed = stats::get().get_timer("query:sage")->get_elapsed();
  stats::get().inc_counter("sage-queries");

  /* `points` vector to sage (python) list of list */
  ostringstream pss;
  pss << "[";
  for (size_t i = 0; i < points.size(); ++i) {
    string tmp = bv_const_vector2str(points[i], ",");
    pss << " [ " << tmp << "],";
  }
  pss << " ]\n" << mod << '\n';
  /* CPP_DBG_EXEC2(SAGE_QUERY, std::cout << __func__ << " " << __LINE__ << ": file header = " << pss.str() << endl); */

  /* write to file */
  string filename = g_query_dir + "/" + qc.to_string() + ".sage";
  CPP_DBG_EXEC(SAGE_QUERY, std::cout << __func__ << " " << __LINE__ << ": filename = " << filename << endl);
  auto fout = ofstream(filename);
  fout << pss.str();
  fout.close();

   /* spawn the solver */
  ASSERT(m_context->get_config().sage_timeout_secs != 0);
  /* CPP_DBG_EXEC2(SAGE_QUERY, std::cout << __func__ << " " << __LINE__ << ": calling spawn_sage_query" << endl); */
  sage_interface::sage_query_res ret;
  {
    autostop_timer query_sage_timer("query:sage");
    ret = spawn_sage_query(filename, m_context->get_config().sage_timeout_secs, soln);
  }
  CPP_DBG_EXEC(SAGE_QUERY, std::cout << __func__ << " " << __LINE__ << ": spawn_sage_query returned " << sage_query_res_to_string(ret) << endl);

  elapsed = stats::get().get_timer("query:sage")->get_elapsed() - elapsed;
  CPP_DBG_EXEC(SAGE_QUERY, cout << __func__ << ':' << __LINE__ << " elapsed = " << elapsed << " (" << elapsed/1.0e6 << " s)\n");
  stats::get().add_counter(string("sage-queries-time-result-") + sage_query_res_to_string(ret), elapsed);

  ASSERT(ret != sage_interface::ERROR);
  return ret == sage_interface::OK;
}

}
