#pragma once

namespace eqspace {

typedef enum { proof_status_proven, proof_status_disproven, proof_status_timeout } proof_status_t;
enum solver_res
{
  solver_res_true,
  solver_res_false,
  solver_res_timeout,
  solver_res_unknown,
  //solver_res_undef,
};

}
