#include <unistd.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <errno.h>
#include <spawn.h>
#include <cstdio>
#include <cassert>
#include <fstream>
#include <string>
#include <iostream>
#include <vector>

#include "config-host.h"

#include "support/consts.h"
#include "support/debug.h"
#include "support/dyn_debug.h"
#include "support/timers.h"

#include "expr/expr.h"
#include "expr/context.h"
#include "expr/counter_example.h"

using namespace std;
using namespace eqspace;

static int input_fd = -1;
static int output_fd = -1;
static int input_cancel_fd = -1;
static int output_cancel_fd = -1;
static bool cancel_current_request = false;

#define ASSERT assert
#define BUFFER_SIZE 1024

#define PIPE_READ 0
#define PIPE_WRITE 1

static ssize_t
client_read(int fd, void *buf, size_t count)
{
  return read(fd, buf, count);
}

static ssize_t
client_write(int fd, const void *buf, size_t count)
{
  return write(fd, buf, count);
}

static void
write_counter_example_to_file(string const& filename, vector<char> const& data)
{
  CPP_DBG_EXEC(SMT_HELPER_PROCESS, cout << __func__ << ':' << __LINE__ << ' ' << filename << endl);
  ofstream out(filename.c_str());
  ASSERT(out.good());
  for (auto const& b : data) {
    out << b;
  }
  out.close();
}

static pair<proof_status_t, list<counter_example_t>>
prove_using_query_decomposition(string const& prove_file)
{
  return make_pair(proof_status_timeout, list<counter_example_t>()); //NOT_IMPLEMENTED()
}

static pair<proof_status_t, list<counter_example_t>>
handle_qd_request(char const *ptr, string& prove_file)
{
  // decode the package -- first 4 bytes for timeout
  unsigned timeout_secs = *(uint32_t *)(ptr);
  ptr += 4;

  // and finally the filename
  prove_file = ptr;

  //CPP_DBG_EXEC(SMT_HELPER_PROCESS, stopwatch_reset(&smt_helper_process_solve_timer));
  //CPP_DBG_EXEC(SMT_HELPER_PROCESS, stopwatch_run(&smt_helper_process_solve_timer));

  return prove_using_query_decomposition(prove_file);
}

static void
sigusr1_handler(int signo)
{
  CPP_DBG_EXEC(QD_HELPER_PROCESS, cout << _FNLN_ << ": signal " << signo << " received\n"; fflush(stdout));
  if (signo == SIGUSR1) {
    cancel_current_request = true;
    CPP_DBG_EXEC(QD_HELPER_PROCESS, cout << _FNLN_ << ": set cancel_current_request to true" << endl; fflush(stdout));
  }
}

static void
do_cancel_current_request(void)
{
  static char recv_buffer[strlen(QD_PROCESS_CANCEL_MESSAGE)];
  ASSERT(input_cancel_fd != -1);
  CPP_DBG_EXEC(QD_HELPER_PROCESS, cout << _FNLN_ << ": cancelling current request\n");
  CPP_DBG_EXEC(QD_HELPER_PROCESS, cout << _FNLN_ << ": reading cancel message from fd " << input_cancel_fd << ".\n"; fflush(stdout));
  int oret = read(input_cancel_fd, recv_buffer, sizeof recv_buffer);
  CPP_DBG_EXEC(QD_HELPER_PROCESS, cout << _FNLN_ << ": read cancel request\n"; fflush(stdout));
  ASSERT(oret == sizeof recv_buffer);
  ASSERT(!memcmp(recv_buffer, QD_PROCESS_CANCEL_MESSAGE, strlen(QD_PROCESS_CANCEL_MESSAGE)));

  static char send_buffer[strlen(QD_PROCESS_CANCEL_RECEIPT_MESSAGE)];
  CPP_DBG_EXEC(QD_HELPER_PROCESS, cout << _FNLN_ << ": writing cancel receipt to fd " << output_cancel_fd << "\n"; fflush(stdout));
  strncpy(send_buffer, QD_PROCESS_CANCEL_RECEIPT_MESSAGE, strlen(QD_PROCESS_CANCEL_RECEIPT_MESSAGE));
  oret = write(output_cancel_fd, send_buffer, sizeof send_buffer);
  ASSERT(oret == sizeof send_buffer);
  CPP_DBG_EXEC(QD_HELPER_PROCESS, cout << _FNLN_ << ": written cancel receipt to fd " << output_cancel_fd << "\n"; cout << _FNLN_ << ": cancelled current request\n"; fflush(stdout));

  oret = read(input_cancel_fd, recv_buffer, sizeof recv_buffer);
  CPP_DBG_EXEC(QD_HELPER_PROCESS, cout << _FNLN_ << ": read cancel request again (three-message handshake)\n"; fflush(stdout));
  ASSERT(oret == sizeof recv_buffer);
  ASSERT(!memcmp(recv_buffer, QD_PROCESS_CANCEL_MESSAGE, strlen(QD_PROCESS_CANCEL_MESSAGE)));
}

int
main(int argc, char **argv)
{
  pair<proof_status_t, list<counter_example_t>> ret;

  ASSERT(argc == 5);
  input_fd = atoi(argv[1]);
  output_fd = atoi(argv[2]);
  input_cancel_fd = atoi(argv[3]);
  output_cancel_fd = atoi(argv[4]);
  CPP_DBG_EXEC(QD_HELPER_PROCESS, printf("input_fd = %d, output_fd = %d\n", input_fd, output_fd));
  CPP_DBG_EXEC(QD_HELPER_PROCESS, printf("input_cancel_fd = %d, output_cancel_fd = %d\n", input_cancel_fd, output_cancel_fd));

  struct sigaction sa;
  sa.sa_handler = sigusr1_handler;

  if (sigaction(SIGUSR1, &sa, NULL) < 0) {
    perror("sigaction");
  }

  size_t query_number = 0;
  while (1) {
    cancel_current_request = false;

    static char message[QD_HELPER_PROCESS_SEND_SIZE];
    char *ptr, *end;
    ptr = message;
    end = ptr + sizeof message;
    /* read QD_HELPER_PROCESS_MSG_SIZE bytes from input_fd
     * We do not expect more than above # of bytes from input_fd
     */
    do {
      CPP_DBG_EXEC(QD_HELPER_PROCESS, cout << _FNLN_ << ": calling read() on " << input_fd << ", end-ptr = " << (end-ptr) << "...\n"; fflush(stdout));
      ssize_t iret = client_read(input_fd, ptr, end-ptr);
      if (iret < 0) {
        if (errno != EINTR) {    /* fail if any error other than EINTR occurred */
          cout << __func__ << " " << __LINE__ << ": iret = " << iret << " errno = " << errno << endl;
          cout.flush();
          ASSERT(0);
        }
        CPP_DBG_EXEC(QD_HELPER_PROCESS, cout << _FNLN_ << ": read() on " << input_fd << " interrupted...\n"; fflush(stdout));
      }
      else if (iret > 0) {
        ptr += iret;
        CPP_DBG_EXEC(QD_HELPER_PROCESS, cout << _FNLN_ << ": read() returned " << iret << " bytes read, end - ptr = " << (end - ptr) << "...\n"; fflush(stdout));
      } else /* iret == 0 => EOF reached */ {
        CPP_DBG_EXEC(QD_HELPER_PROCESS, cout << _FNLN_ << ": EOF reached!\n");
        ptr = nullptr;
        break;
      }
      if (cancel_current_request) {
        CPP_DBG_EXEC(QD_HELPER_PROCESS, cout << _FNLN_ << ": cancelling read(). end - ptr =  " << (end - ptr) << "...\n"; fflush(stdout));
        break;
      }
    } while (ptr != end);

    CPP_DBG_EXEC(QD_HELPER_PROCESS, cout << _FNLN_ << ": finished read() loop. end - ptr =  " << (end - ptr) << "...\n"; fflush(stdout));

    if (ptr == nullptr) {
      CPP_DBG_EXEC(QD_HELPER_PROCESS, cout << _FNLN_ << ": read loop returned nullptr, breaking out...\n"; fflush(stdout));
      break;
    }
    ptr = message;
    if (cancel_current_request) {
      CPP_DBG_EXEC(QD_HELPER_PROCESS, cout << _FNLN_ << ": calling do_cancel_current_request...\n"; fflush(stdout));
      do_cancel_current_request();
      continue;
    }

    string prove_file;
    ret = handle_qd_request(ptr, prove_file);
    CPP_DBG_EXEC(QD_HELPER_PROCESS, cout << _FNLN_ << ": returned from handle_qd_request()...\n");
    //client_log("query_number", query_number++);

    if (cancel_current_request) {
      CPP_DBG_EXEC(QD_HELPER_PROCESS, cout << _FNLN_ << ": calling do_cancel_current_request...\n"; fflush(stdout));
      do_cancel_current_request();
      continue;
    }

    if (ret.first == proof_status_disproven) {
      // recv-size bytes for result
      ASSERT(QD_HELPER_PROCESS_RECV_SIZE == 10);
      int rc = client_write(output_fd, "disproven\0", QD_HELPER_PROCESS_RECV_SIZE);
      ASSERT(rc == QD_HELPER_PROCESS_RECV_SIZE);

      if (cancel_current_request) {
        CPP_DBG_EXEC(QD_HELPER_PROCESS, cout << _FNLN_ << ": calling do_cancel_current_request...\n"; fflush(stdout));
        do_cancel_current_request();
        continue;
      }

      stringstream ss;
      ss << prove_file << ".counter_examples";
      string fname = ss.str();
      ofstream ofs(fname);
      ASSERT(ofs.is_open());
      counter_example_t::counter_examples_to_stream(ofs, ret.second);
      ofs.close();

      // next byte for # of counter examples
      //uint8_t n_ce = ret.second.size();
      //rc = client_write(output_fd, &n_ce, sizeof(n_ce));
      //ASSERT(rc == sizeof(n_ce));
    } else if (ret.first == proof_status_proven) {
      ASSERT(QD_HELPER_PROCESS_RECV_SIZE == 10);
      int rc = client_write(output_fd, "proven\0\0\0\0", QD_HELPER_PROCESS_RECV_SIZE);
      ASSERT(rc == QD_HELPER_PROCESS_RECV_SIZE);
    } else if (ret.first == proof_status_timeout) {
      ASSERT(QD_HELPER_PROCESS_RECV_SIZE == 10);
      int rc = client_write(output_fd, "timeout\0\0\0", QD_HELPER_PROCESS_RECV_SIZE);
      ASSERT(rc == QD_HELPER_PROCESS_RECV_SIZE);
    } else {
      ASSERT(QD_HELPER_PROCESS_RECV_SIZE == 10);
      client_write(output_fd, "ERROR\0\0\0\0\0", QD_HELPER_PROCESS_RECV_SIZE);
      NOT_REACHED();
    }
    flush(cout);
  }

  CPP_DBG_EXEC(QD_HELPER_PROCESS, cout << _FNLN_ << ": exiting...\n");
  return 0;
}
