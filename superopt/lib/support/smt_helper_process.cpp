#include "config-host.h"
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

#include "expr/expr.h"
#include "support/consts.h"
#include "support/debug.h"
#include "support/timers.h"

using namespace std;

//globals
string record_filename;
string replay_filename;
string stats_filename;

#define ASSERT assert
#define BUFFER_SIZE 1024

#define PIPE_READ 0
#define PIPE_WRITE 1

struct solver_attrs {
  char const * path;
  char const * name;
  int solver_id;
  char ** args;
  int pipefd[2];
  pid_t pid;
};

class solver_result_t {
private:
  size_t m_idx;
  int m_id;
  vector<char> m_data;

public:
  solver_result_t(size_t idx, int solver_id, vector<char> const& data)
    : m_idx(idx),
      m_id(solver_id),
      m_data(data)
  {
    ASSERT(idx >= 0);
  }

  size_t get_index() const { return m_idx; }
  int get_solver_id() const { return m_id; }
  vector<char> const& get_data() const { return m_data; }
};

class solver_stats_t {
public:
  enum solver_result { sat, unsat };

  solver_stats_t(int solver_id, chrono::duration<double> const& solver_time, solver_result res) : m_solver_id(solver_id), m_solver_time(solver_time), m_solver_result(res)
{ }

  int get_solver_id() const { return m_solver_id; }
  chrono::duration<double> const& get_solver_time() const { return m_solver_time; }
  solver_result get_solver_result() const { return m_solver_result; }

  string get_solver_result_string() const { return m_solver_result == sat ? "SAT" : "UNSAT"; }
  string get_solver_id_string() const
  {
    switch (m_solver_id) {
      case SOLVER_ID_Z3:
        return "z3";
      case SOLVER_ID_YICES:
        return "yices";
      case SOLVER_ID_CVC4:
        return "cvc4";
      case SOLVER_ID_BOOLECTOR:
        return "boolector";
      default:
        return "unknown";
    }
  }

  string to_string() const
  {
    stringstream ss;
    ss << "( " << get_solver_id_string() << ", " << chrono_duration_to_string(m_solver_time, 4) << ", " << get_solver_result_string() << " )";
    return ss.str();
  }

private:
  int m_solver_id;
  chrono::duration<double> m_solver_time;
  solver_result m_solver_result;
};

class query_stats_t {
public:
  query_stats_t()
  {}

  void add_solver_stats(solver_stats_t const& solver_stats)
  {
    m_solver_stats.push_back(solver_stats);
  }

  vector<solver_stats_t> const& get_solver_stats() const { return m_solver_stats; }

  string to_string() const
  {
    stringstream ss;
    for (size_t i = 0; i < m_solver_stats.size(); ++i) {
      ss << m_solver_stats.at(i).to_string();
      if (i < m_solver_stats.size()-1) {
        ss << " ; ";
      }
    }
    return ss.str();
  }

private:
  vector<solver_stats_t> m_solver_stats;
};

class mystats_t {
public:
  ~mystats_t() { }
  mystats_t(mystats_t const&) = delete;
  mystats_t& operator=(mystats_t const&) = delete;

  static mystats_t& get() {
    static mystats_t instance;
    return instance;
  }

  void clear() { m_query_stats.clear(); }
  void add_query_stats(query_stats_t const& qs) { m_query_stats.push_back(qs); }

  string to_string() const
  {
    stringstream ss;
    for (auto const& qs : m_query_stats) {
      ss << qs.to_string() << '\n';
    }
    return ss.str();
  }

private:
  vector<query_stats_t> m_query_stats;

  mystats_t() {}
};

static void
client_log(char const *fieldname, int val)
{
  if (record_filename == "") {
    return;
  }
  ofstream ofs;
  ofs.open(record_filename, ofstream::out | ofstream::app);
  ofs << "==" << fieldname << ": " << val << endl;
  ofs.close();
}

static void
client_log(char const *fieldname, vector<char> const &val)
{
  if (record_filename == "") {
    return;
  }
  ofstream ofs;
  ofs.open(record_filename, ofstream::out | ofstream::app);
  ofs << "==" << fieldname << "\n";
  ofs << "==DATA_START==\n";
  for (auto v : val) {
    ofs << v;
  }
  ofs << "==DATA_END==\n";
  ofs.close();
}

static void
client_log(char const *fieldname, string const &val)
{
  if (record_filename == "") {
    return;
  }
  ofstream ofs;
  ofs.open(record_filename, ofstream::out | ofstream::app);
  ofs << "==" << fieldname << ": " << val << endl;
  ofs.close();
}

static void
client_log_file(string const &filename)
{
  if (record_filename == "") {
    return;
  }
  ifstream t(filename);
  string str((std::istreambuf_iterator<char>(t)),
              std::istreambuf_iterator<char>());
  ofstream ofs;
  ofs.open(record_filename, ofstream::out | ofstream::app);
  ofs << "==FILE_START==\n" << str << "\n==FILE_END==\n";
  ofs.close();
}

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

#define FOUND_SAT 3
#define FOUND_UNSAT 5
#define FOUND_ERROR 6       // solver returned "error"
#define FOUND_UNKNOWN 7     // solver returned unexpected output

char const* FOUND_TO_STR(int i)
{
  switch (i) {
    case FOUND_SAT:
      return "FOUND_SAT";
    case FOUND_UNSAT:
      return "FOUND_UNSAT";
    case FOUND_ERROR:
      return "FOUND_ERROR";
    case FOUND_UNKNOWN:
      return "FOUND_UNKNOWN";
    default:
      NOT_REACHED();
  }
}

static int
found_result(char const *buf, size_t size)
{
  if (   size >= 3
      && (buf[0] == 's')
      && (buf[1] == 'a')
      && (buf[2] == 't')) {
    return FOUND_SAT;
  } else if (   size >= 5
             && (buf[0] == 'u')
             && (buf[1] == 'n')
             && (buf[2] == 's')
             && (buf[3] == 'a')
             && (buf[4] == 't')) {
    return FOUND_UNSAT;
  } else {
    if (   size >= 6
        && (   strncmp(buf, "(error", sizeof "(error" - 1) == 0 // yices
            || strncmp(buf, "ERROR:", sizeof "ERROR:" - 1) == 0)) { // z3
      return FOUND_ERROR;
    } else {
      printf("%s %s() %d: unexpected output from solver. buf=\n%s\n", __FILE__, __func__, __LINE__, buf);
      fflush(stdout);
      return FOUND_UNKNOWN;
    }
  }
}

#define RET_SAT 0
#define RET_UNSAT 1
#define RET_TIMEOUT 2
#define RET_ERROR 3
#define RET_UNDEF 999

static int
ret_from_found(int found)
{
  if (found == FOUND_SAT) {
    return RET_SAT;
  } else if (found == FOUND_UNSAT) {
    return RET_UNSAT;
  } else NOT_REACHED();
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

#define MASKED(mask, i) (mask & (1 << i))
#define FULL_MASK(n) ((1 << n)-1)

char* read_input_data(int input_fd)
{
  static char message[SMT_HELPER_PROCESS_MSG_SIZE];
  char *ptr, *end;
  ptr = message;
  end = ptr + sizeof message;
  /* read SMT_HELPER_PROCESS_MSG_SIZE bytes from input_fd
   * We do not expect more than above # of bytes from input_fd
   */
  do {
    ssize_t iret = client_read(input_fd, ptr, end-ptr);
    if (iret < 0) {
      if (errno != EINTR) {    /* fail if any error other than EINTR occurred */
        cout << __func__ << " " << __LINE__ << ": iret = " << iret << " errno = " << errno << endl;
        cout.flush();
        ASSERT(0);
      }
    }
    else if (iret > 0) ptr += iret;
    else /* iret == 0 => EOF reached */ {
      return nullptr;
    }
  } while (ptr != end);
  ptr = message;
  return ptr;
}

void spawn_solvers_with_argfile(struct solver_attrs *solvers, int num_solvers, char* smt2_file)
{
  for (int i = 0; i < num_solvers; i++) {
    posix_spawn_file_actions_t child_actions;
    int status;
    CPP_DBG_EXEC(SMT_HELPER_PROCESS, cout << "forking solver " << i << ": " << solvers[i].name << ": " << solvers[i].path << '\n');
    int rc = pipe(solvers[i].pipefd);
    ASSERT(rc == 0);

    posix_spawn_file_actions_init(&child_actions);
    posix_spawn_file_actions_addclose(&child_actions, solvers[i].pipefd[PIPE_READ]);
    posix_spawn_file_actions_adddup2(&child_actions, solvers[i].pipefd[PIPE_WRITE], 1); // connect write end of pipe to stdout
    posix_spawn_file_actions_adddup2(&child_actions, solvers[i].pipefd[PIPE_WRITE], 2); // likewise for stderr
    posix_spawn_file_actions_addclose(&child_actions, solvers[i].pipefd[PIPE_WRITE]);

    // set filename as argument
    char** arg = solvers[i].args;
    while (*arg) ++arg;
    *arg = smt2_file;

    extern char **environ;
    status = posix_spawn(&solvers[i].pid, solvers[i].path, &child_actions, NULL, (char * const *)solvers[i].args, environ);
    if (status != 0) {
      cout << "\n\n" << __FILE__ << " " << __func__ << " " << __LINE__ << ": spawn failed for " << solvers[i].path << '\n';
      cout << "args: ";
      for (char** arg = solvers[i].args; (*arg); ++arg) {
        cout << *arg << ' ';
      }
      cout << '\n';
      cout.flush();
    }
    ASSERT(status == 0);

    // reset the argument
    *arg = NULL;

    //fprintf(fp, "solvers[%d].pid = %d\n", i, solvers[i].pid);
    ASSERT(solvers[i].pid > 0);
    rc = close(solvers[i].pipefd[PIPE_WRITE]);
    ASSERT(rc >= 0);
    /*int opt = 1;
      ioctl(solvers[i].pipefd[0], FIONBIO, &opt);*/
  }
}

/* return value
 * -1   : error
 *  0   : timeout
 *  > 0 : output available for at least one fd
 */
static int
wait_for_solvers_output(struct solver_attrs* solvers, int num_solvers, struct timeval& timeout_val, vector<bool> const& done, fd_set& readset)
{
  int result;
  do {
    result = -1;
    int maxfd = -1;
    FD_ZERO(&readset);
    for (int i = num_solvers - 1; i >= 0; i--) {
      if (done[i]) continue;
      FD_SET(solvers[i].pipefd[PIPE_READ], &readset);
      CPP_DBG_EXEC2(SMT_HELPER_PROCESS, cout << __func__ << " " << __LINE__ << ": maxfd = " << maxfd << " solvers[" << i << "].pipefd[PIPE_READ] = " << solvers[i].pipefd[PIPE_READ] << endl);
      maxfd = (maxfd < solvers[i].pipefd[PIPE_READ]) ? solvers[i].pipefd[PIPE_READ] : maxfd;
    }
    ASSERT(maxfd != -1);
    result = select(maxfd + 1, &readset, NULL, NULL, &timeout_val);
    CPP_DBG_EXEC2(SMT_HELPER_PROCESS, cout << __func__ << " " << __LINE__ << ": maxfd = " << maxfd << " select result = " << result << endl);
  } while (result == -1 && errno == EINTR); // select got interrupted -- try again
  return result;
}

/* returns found status which can be:
 * FOUND_SAT: solver returned sat
 * FOUND_UNSAT: solver returned unsat
 * FOUND_ERROR: solver returned error
 * FOUND_UNKNOWN: solver returned unknown output
 *
 * recvd_data is populated with output from solver fd
 */
static int
read_solver_output(struct solver_attrs const& solver, vector<char>& recvd_data)
{
  static char buffer[BUFFER_SIZE];
  char *ptr = buffer;
  char *end = ptr + sizeof buffer;
  int found = FOUND_UNKNOWN;
  bool read_header = false;

  recvd_data.clear();

  CPP_DBG_EXEC(SMT_HELPER_PROCESS,
      stopwatch_tell(&smt_helper_process_solve_timer);
      double secs = (double)smt_helper_process_solve_timer.wallClock_elapsed/1e6;
      printf("%s finished at  %d:%d:%d [%f]\n", solver.name, (int)(secs/3.6e3), ((int)((secs/60))) % 60, ((int)secs)%60, secs)
      );
  size_t num_errors = 0;
  do {
    CPP_DBG_EXEC2(SMT_HELPER_PROCESS, printf("reading pipefd[PIPE_READ] for %s\n", solver.name));
    const size_t iret = read(solver.pipefd[PIPE_READ], ptr, end - ptr);
    if (iret > 0) {
      ptr += iret;
      if (!read_header) {
        found = found_result(buffer, ptr - buffer);
        read_header = (found == FOUND_SAT || found == FOUND_UNSAT || found == FOUND_ERROR);
        CPP_DBG_EXEC2(SMT_HELPER_PROCESS, printf("found = %s, read_header = %d\n", FOUND_TO_STR(found), read_header));
      }
      if(end == ptr) {  //buffer is full
        recvd_data.insert(recvd_data.end(), buffer, ptr);
        ptr = buffer;
      }
    } else if (iret < 0) {
      num_errors++;
      if (num_errors > 1024) {
        // too many read errors; just return with whatever data we have
        break;
      }
    } else /* iret == 0 */ {
      // EOF reached
      break;
    }
  } while (1);

  // transfer the remaining data
  recvd_data.insert(recvd_data.end(), buffer, ptr);

  return found;
}

static void
kill_solver(struct solver_attrs const& solver)
{
  ASSERT (close(solver.pipefd[PIPE_READ]) == 0); // close our end of pipe
  ASSERT (solver.pid > 0);
  kill(solver.pid, SIGKILL);
  waitpid(solver.pid, NULL, 0);
}

static void
killall_solvers(struct solver_attrs* solvers, int num_solvers)
{
  for(int i = 0; i < num_solvers; ++i) {
    kill_solver(solvers[i]);
  }
}

static int
handle_input_request(char const *ptr, struct solver_attrs const *solvers, int num_solvers, vector<solver_result_t>& solver_result, string &smt2_file)
{
  // decode the package -- first 4 bytes for timeout
  unsigned timeout_secs = *(uint32_t *)(ptr);

  int ret = RET_UNDEF;
  // next byte for mask -- masked solver's model is not used (UNSATs are fine)
  ptr += 4;
  uint8_t mask = *(uint8_t*)(ptr);
  if (mask == FULL_MASK(num_solvers)) {
    cout << "no solver selected! exiting!";
  }
  ASSERT(mask != FULL_MASK(num_solvers));

  // and finally the filename
  ptr += 1;
  smt2_file = ptr;

  CPP_DBG_EXEC(SMT_HELPER_PROCESS, stopwatch_reset(&smt_helper_process_solve_timer));
  CPP_DBG_EXEC(SMT_HELPER_PROCESS, stopwatch_run(&smt_helper_process_solve_timer));

  spawn_solvers_with_argfile((struct solver_attrs *)solvers, num_solvers, (char *)smt2_file.c_str());

  struct timeval timeout_val = { timeout_secs, 0 };
  vector<bool> done(num_solvers, false);

  struct time start_timer;
  stopwatch_reset(&start_timer);
  stopwatch_run(&start_timer);
  bool finished = false;

  auto chrono_start = chrono::system_clock::now();
  query_stats_t this_query_stats;

  do {
    fd_set readset; // fds ready to be read from
    int result = wait_for_solvers_output((struct solver_attrs *)solvers, num_solvers, timeout_val, done, readset);
    if (result == -1) {
      if (!solver_result.empty()) {
        // ignore error if we already have something to return
        break;
      }
      CPP_DBG_EXEC(SMT_HELPER_PROCESS, cout << __func__ << " " << __LINE__ << ": Error\n");
      ret = RET_ERROR;
      break;
    }
    else if (result == 0) {
      if (!solver_result.empty()) {
        // additional timer expired; just return with what we have
        break;
      }
      ASSERT (timeout_val.tv_sec == 0 && timeout_val.tv_usec == 0);
      ret = RET_TIMEOUT;
      CPP_DBG_EXEC(SMT_HELPER_PROCESS,
          double secs;
          stopwatch_tell(&smt_helper_process_solve_timer);
          secs = (double)smt_helper_process_solve_timer.wallClock_elapsed/1e6;
          printf("timeout at  %d:%d:%d [%f]\n", (int)(secs/3.6e3), ((int)((secs/60))) % 60, ((int)secs)%60, secs);
      );
      break;
    } else {
      // got result from one of the solver
      ASSERT(result > 0);
      for (int i = num_solvers-1; i >= 0; i--) {
        if (done[i]) continue;
        CPP_DBG_EXEC2(SMT_HELPER_PROCESS, printf("checking solver %d/%d : %s\n", (int)i, (int)num_solvers, solvers[i].name));
        if (FD_ISSET(solvers[i].pipefd[PIPE_READ], &readset)) {
          vector<char> recvd_data;
          int found = read_solver_output(solvers[i], recvd_data);
          done[i] = true; // mark solver as finished

          if (   found == FOUND_ERROR
              || found == FOUND_UNKNOWN) {
            printf("solver %s found = %s\n", solvers[i].name, FOUND_TO_STR(found));
            printf("args: ");
            for (char** p = solvers[i].args; *p != NULL; ++p) {
              printf("%s ", *p);
            }
            printf("%s\n", smt2_file.c_str());
            if (found == FOUND_ERROR) {
              cout << "Error:\n";
              for (auto ch : recvd_data) cout << ch;
              cout << '\n';
            }
            continue;
          }
          ASSERT(found == FOUND_SAT || found == FOUND_UNSAT);

          auto chrono_now = chrono::system_clock::now();
          this_query_stats.add_solver_stats(solver_stats_t(solvers[i].solver_id, chrono_now-chrono_start, found == FOUND_SAT ? solver_stats_t::sat : solver_stats_t::unsat));

          solver_result.emplace_back(solver_result_t(i, solvers[i].solver_id, recvd_data));
          ret = ret_from_found(found);

          if (   MASKED(mask, solvers[i].solver_id)
              && found == FOUND_SAT) {// NOTE comment this to skip masked solver even for UNSAT result */
            //CPP_DBG_EXEC(SMT_HELPER_PROCESS, cout << "skipping " << solvers[i].name << " result as it is masked\n");
            CPP_DBG_EXEC(SMT_HELPER_PROCESS, cout << __func__ << ": waiting for others as " << solvers[i].name << " is masked\n");
            continue;
          }

          if (found == FOUND_SAT && start_timer.running && !all_of(done.begin(), done.end(), [](bool b) { return b; })) {
            // wait for additional time to get results from other solvers as well
            stopwatch_stop(&start_timer);
            ASSERT(stopwatch_tell(&start_timer) >= 0);
            long first_solver_time_secs  = start_timer.wallClock_elapsed/(1000000l);
            long first_solver_time_usecs = start_timer.wallClock_elapsed%(1000000l);

            long min_waiting_time_usecs = 100000l;
            //long min_waiting_time_secs = (MASKED(mask, solvers[i].solver_id)) ? 1l : 0; // min wait of 1.1 sec for masked solver
            long min_waiting_time_secs = 0;

            long additional_wait_secs = min(first_solver_time_secs + min_waiting_time_secs, timeout_val.tv_sec);
            long additional_wait_usecs = min(first_solver_time_usecs + min_waiting_time_usecs , timeout_val.tv_usec); // wait for at least 100ms
            timeout_val.tv_sec = additional_wait_secs;
            timeout_val.tv_usec = additional_wait_usecs;
            //CPP_DBG_EXEC(SMT_HELPER_PROCESS, cout << solvers[i].name << " finished at: " << first_solver_time_secs+first_solver_time_usecs/1e6 << endl);
            CPP_DBG_EXEC(SMT_HELPER_PROCESS, cout << __func__ << " " << __LINE__ << ": waiting for additional " << additional_wait_secs+additional_wait_usecs/1e6 << endl);
            continue;
          }
          finished = true;
          break;
        }
      }
      if (finished) {
        // valid finish
        break;
      }
      if (all_of(done.begin(), done.end(), [](bool b) { return b; })) {
        CPP_DBG_EXEC(SMT_HELPER_PROCESS, cout << __func__ << " " << __LINE__ << ": no fd left to be read. Returning\n");
        ret = RET_ERROR; // all finished without valid output
        break;
      }
    }
  } while (1);

  killall_solvers((struct solver_attrs *)solvers, num_solvers);
  ASSERT(ret != RET_UNDEF);

  mystats_t::get().add_query_stats(this_query_stats);

  return ret;
}

static int
replay_input_request(ifstream &in, char const *ptr, struct solver_attrs const *solvers, int num_solvers, size_t &query_number, vector<solver_result_t>& solver_result, string &smt2_file)
{
  string line;
  int ret, result_count;
  bool end;

  unsigned timeout_secs_UNUSED = *(uint32_t *)(ptr);
  ptr += 4;
  uint8_t mask_UNUSED = *(uint8_t*)(ptr);
  ptr += 1;
  char const *input_smt2_file = ptr;
  string input_query_str = file_to_string(input_smt2_file);
  input_query_str += "\n";

  end = !getline(in, line);
  if (end) {
    cout << __func__ << " " << __LINE__ << ": reached premature end of replay file\n";
    NOT_REACHED();
  }
  ASSERT(line.substr(0, 16) == "==query_number: ");
  query_number = string_to_int(line.substr(16));
  cout << "Replaying query number " << query_number << endl;

  end = !getline(in, line);
  ASSERT(!end);
  ASSERT(line == "==FILE_START==");
  string query_str;
  while (!(end = !getline(in, line)) && line != "==FILE_END==") {
    query_str += line;
    query_str += "\n";
  }
  ASSERT(!end);
  if (query_str != input_query_str) {
    cout << "=query_str_begin\n";
    cout << query_str;
    cout << "\n=query_str_end\n";
    cout << "=input_query_str_begin\n";
    cout << input_query_str;
    cout << "\n=input_query_str_end\n";
    fflush(stdout);
  }
  ASSERT(query_str == input_query_str);

  end = !getline(in, line);
  ASSERT(!end);
  ASSERT(line.substr(0, 7) == "==ret: ");
  ret = string_to_int(line.substr(7));

  end = !getline(in, line);
  ASSERT(!end);
  ASSERT(line.substr(0, 16) == "==result_count: ");
  result_count = string_to_int(line.substr(16));

  for (int i = 0; i < result_count; ++i) {
    end = !getline(in, line);
    ASSERT(!end);
    ASSERT(line.substr(0, 9) == "==index: ");
    size_t idx = string_to_int(line.substr(9));

    end = !getline(in, line);
    ASSERT(!end);
    ASSERT(line.substr(0, 13) == "==solver_id: ");
    int solver_id = string_to_int(line.substr(13));

    end = !getline(in, line);
    ASSERT(!end);
    ASSERT(line == "==recvd_data");
    end = !getline(in, line);
    ASSERT(!end);
    ASSERT(line == "==DATA_START==");
    vector<char> recvd_data;
    while (!(end = !getline(in, line)) && line != "==DATA_END==") {
      for (auto ch : line) {
        recvd_data.push_back(ch);
      }
      recvd_data.push_back('\n');
    }

    solver_result.emplace_back(solver_result_t(idx, solver_id, recvd_data));
  }
  ASSERT(!end);
  end = !getline(in, line);
  ASSERT(line.substr(0, 13) == "==smt2_file: ");
  //smt2_file = line.substr(13);
  smt2_file = input_smt2_file;
  return ret;
}

void save_stats_to_file()
{
  if (stats_filename.length()) {
    CPP_DBG_EXEC(SMT_HELPER_PROCESS, cout << __func__ << ':' << __LINE__ << ": Saving stats to file '" << stats_filename << "'...\n");
    ofstream ofs(stats_filename, ofstream::out|ofstream::trunc);
    ASSERT(ofs.good());
    ofs << mystats_t::get().to_string();
    ofs.close();
    CPP_DBG_EXEC(SMT_HELPER_PROCESS, cout << __func__ << ':' << __LINE__ << ": Saved stats to file '" << stats_filename << "'...\n");
  }
}

int
main(int argc, char **argv)
{
  int i, ret, num_solvers;

  ASSERT(argc == 6);
  int input_fd = atoi(argv[1]);
  int output_fd = atoi(argv[2]);
  record_filename = argv[3];
  replay_filename = argv[4];
  stats_filename = argv[5];
  //printf("input_fd = %d, output_fd = %d\n", input_fd, output_fd);
  if (record_filename != "") {
    file_truncate(record_filename);
  }

  struct solver_attrs solvers[4];
  i = 0;
  char yices_name[] = YICES_EXE;
  solvers[i].path = yices_name;
  solvers[i].name = "yices_smt2";
  solvers[i].solver_id = SOLVER_ID_YICES;
  char *tmp[] = { yices_name, 0/*smt2_file*/, 0};
  solvers[i].args = tmp;
  ++i;
  char z3_name[] = Z3_EXE;
  solvers[i].path = z3_name;
  solvers[i].name = "z3";
  solvers[i].solver_id = SOLVER_ID_Z3;
  char tmp_a[] = "-smt2";
  char tmp_b[] = "model.compact=false"; // for new z3
  char tmp_z[] = "memory_high_watermark=10240"; // 10 GiB limit on memory
  char *tmp2[] = { z3_name, tmp_a, tmp_b, 0/*smt2_file*/, 0 };
  solvers[i].args = tmp2;
  ++i;
  char cvc4_name[] = CVC4_EXE;
  solvers[i].path = cvc4_name;
  solvers[i].name = "cvc4";
  solvers[i].solver_id = SOLVER_ID_CVC4;
  char tmp_c[] = "--produce-models"; // required for producing models
  char *tmp3[] = { cvc4_name, tmp_c, 0/*smt2_file*/, 0 };
  solvers[i].args = tmp3;
  ++i;
  //char boolector_name[] = BOOLECTOR_EXE;
  //solvers[i].path = boolector_name;
  //solvers[i].name = "boolector";
  //solvers[i].solver_id = SOLVER_ID_BOOLECTOR;
  //char tmp_d[] = "--smt2";       // smt2 as input
  //char tmp_e[] = "--smt2-model"; // required for producing smt2 models (compatible with z3)
  //char *tmp4[] = { boolector_name, tmp_d, tmp_e, 0/*smt2_file*/, 0 };
  //solvers[i].args = tmp4;
  //++i;

  num_solvers = i;
  ASSERT(num_solvers <= 8);

  cout << argv[0] << ": " << num_solvers << " solvers initialized: ";
  for (i = 0; i < num_solvers; ++i) cout << solvers[i].name << ' ';
  cout << endl;

  ifstream replay_ifs;
  if (replay_filename != "") {
    replay_ifs.open(replay_filename);
  }
  if (stats_filename.length()) {
    CPP_DBG_EXEC(SMT_HELPER_PROCESS, cout << __func__ << ':' << __LINE__ << ": stats_filename = " << stats_filename << endl);
    mystats_t::get().clear();
    //atexit(save_stats_to_file);
  }

  size_t query_number = 0;
  while (1) {
    char* ptr = read_input_data(input_fd);
    if (ptr == nullptr) {
      break;
    }

    vector<solver_result_t> solver_result;
    string smt2_file;
    if (replay_filename == "") {
      ret = handle_input_request(ptr, solvers, num_solvers, solver_result, smt2_file);
    } else {
      size_t replayed_query_number;
      ret = replay_input_request(replay_ifs, ptr, solvers, num_solvers, replayed_query_number, solver_result, smt2_file);
      ASSERT(replayed_query_number == query_number);
    }
    client_log("query_number", query_number++);
    client_log_file(smt2_file);
    client_log("ret", ret);
    client_log("result_count", solver_result.size());
    for (auto const& sr : solver_result) {
      client_log("index", sr.get_index());
      client_log("solver_id", sr.get_solver_id());
      client_log("recvd_data", sr.get_data());
    }
    client_log("smt2_file", smt2_file);

    if (ret == RET_SAT) {
      // first 8 bytes for result
      int rc = client_write(output_fd, "sat\0\0\0\0\0", 8);
      ASSERT(rc == 8);
      CPP_DBG_EXEC2(SMT_HELPER_PROCESS, cout << "smt_helper_process: written \"sat\" to output_fd\n");

      // next byte for # of counter examples
      uint8_t n_ce = solver_result.size();
      rc = client_write(output_fd, &n_ce, sizeof(n_ce));
      ASSERT(rc == sizeof(n_ce));
      for (auto itr = solver_result.crbegin(); itr != solver_result.crend(); ++itr) {
        auto const& sr = *itr;
        stringstream ss;
        ss << smt2_file << "." << solvers[sr.get_index()].name << ".counter_example";

        string fname = ss.str();
        write_counter_example_to_file(fname, sr.get_data());
        CPP_DBG_EXEC2(SMT_HELPER_PROCESS, cout << "smt_helper_process: written counter_example to " << fname << endl);

        // write solver_id
        int solver_id = sr.get_solver_id();
        rc = client_write(output_fd, &solver_id, sizeof(solver_id));
        ASSERT(rc == sizeof(solver_id));

        // and filename size
        int size_send = fname.size();
        CPP_DBG_EXEC2(SMT_HELPER_PROCESS, cout << "smt_helper_process: written size_send to output_fd. rc = " << rc << ". size_send = " << size_send << "\n");
        rc = client_write(output_fd, &size_send, sizeof(size_send));
        ASSERT(rc == sizeof(size_send));

        // and finally the filename itself
        rc = client_write(output_fd,fname.c_str(), size_send);
        ASSERT(rc == size_send);

        CPP_DBG_EXEC2(SMT_HELPER_PROCESS, cout << "smt_helper_process: written " << fname << " to output_fd. rc = " << rc << ". fname = " << fname << "\n");
      }
    }
    else if (ret == RET_UNSAT) {
      int rc = client_write(output_fd, "unsat\0\0\0", 8);
      ASSERT(rc == 8);
    }
    else if (ret == RET_TIMEOUT) {
      int rc = client_write(output_fd, "timeout\0", 8);
      ASSERT(rc == 8);
    }
    else {
      client_write(output_fd, "ERROR\0\0\0", 8);
      NOT_REACHED();
    }
    flush(cout);
  }

  save_stats_to_file();
  return 0;
}
