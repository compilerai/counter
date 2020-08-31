#pragma once

#include <vector>
#include <sstream>

#include "support/bv_const.h"

namespace eqspace {

class query_comment;
class context;

class sage_interface
{
public:
  enum sage_query_res { TIMEOUT = 0, OK = 1, ERROR = 2 };

  int sage_helper_init();

  sage_interface(context *ctx); /* context has configuration settings such as timeout */
  ~sage_interface();

  bool get_nullspace_basis(const std::vector<std::vector<bv_const>>& points, const bv_const& mod, query_comment const& comment, std::vector<std::vector<bv_const>>& soln);

  std::string stats() const
  {
    std::stringstream ss;
    ss << __func__ << " " << __LINE__ << ":";
    return ss.str();
  }

private:
  sage_query_res spawn_sage_query(std::string filename, unsigned timeout_secs, std::vector<std::vector<bv_const>>& result);

  context *m_context;
  int m_helper_pid;
  int send_channel[2], recv_channel[2];
};

}
