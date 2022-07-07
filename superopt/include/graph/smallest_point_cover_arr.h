#pragma once

#include "support/bv_const.h"
#include "support/bv_const_ref.h"
#include "support/dyn_debug.h"

#include "graph/addr_that_may_differ.h"
#include "graph/smallest_point_cover.h"
#include "graph/expr_group.h"

namespace eqspace {

using namespace std;

//XXX: KNOWN BUG: we do not account for the fact that different addrs may be overlapping non-exactly; for that we would need to do a byte-granular analysis; for now, we make do with an "access-granular" analysis by assuming that two accesses are either identical (perfect overlap) or disjoint.
class addr_partition_t
{
public:
  static int const arr0_index = 0;
  static int const arr1_index = 1;
  static int const addr_indices_start = 2;
private:
  using partition_set_status_t = enum { SRC_DST_AGREE, SRC_DST_DISAGREE };
  //vector<addr_that_may_differ_t> const& m_addrs_that_may_differ;
  long m_id;
  vector<pair<vector<addr_that_may_differ_t>, partition_set_status_t>> m_partition;
  bool m_arrs_are_unequal;
public:
  string partition_to_string()
  {
    stringstream ss;
    int i =1;
    for(auto const &p : m_partition)
    {
      ss << "partition " << i++ << endl;
      ss << "addrs " << endl;
      for(auto const &a : p.first)
      {
        ss << a.to_string();
      }
      if(p.second == 0)
        ss << "partition_set_status : SRC_DST_AGREE " << endl;
      else
        ss << "partition_set_status : SRC_DST_DISAGREE " << endl;
    }
    return ss.str();
  }
  addr_partition_t(long id, vector<addr_that_may_differ_t> const& addrs_that_may_differ) : /*m_addrs_that_may_differ(addrs_that_may_differ), */m_id(id), m_partition({ make_pair(addrs_that_may_differ, SRC_DST_AGREE) }), m_arrs_are_unequal(false)
  { }
  expr_ref get_indicator_variable_for_pset_num(size_t pset_num, sort_ref const& addr_sort, context *ctx) const
  {
    stringstream ss;
    ss << ADDR_DIFF_INDICATOR_VAR_PREFIX << m_id << "." << pset_num;
    return ctx->mk_var(ss.str(), addr_sort);
  }

  bool
  arrays_differ_beyond_addr_vals(context *ctx, array_constant const& arr1, array_constant const& arr2, point_ref const& addr_pt, expr_group_ref const& exprs_to_be_correlated) const
  {
    vector<sort_ref> arr1_dom = arr1.get_domain_sort();
    vector<sort_ref> arr2_dom = arr2.get_domain_sort();
    ASSERT(arr1_dom.size() == 1 && arr1_dom.front()->is_bv_kind());
    ASSERT(arr2_dom.size() == 1 && arr2_dom.front()->is_bv_kind());
    ASSERT(arr1_dom.front()->get_size() == arr2_dom.front()->get_size());
    unsigned bvsize = arr1_dom.front()->get_size();
    ASSERT(arr1.get_result_sort() == arr2.get_result_sort());
    ASSERT(arr1.get_result_sort()->is_bv_kind());
    ASSERT(arr1.get_result_sort()->get_size() == arr2.get_result_sort()->get_size());
    array_constant_ref zero_mem_ac = ctx->mk_array_constant(arr1_dom, ctx->mk_zerobv(arr1.get_result_sort()->get_size()));
    array_constant_ref masked_arr1 = ctx->mk_array_constant(arr1);
    array_constant_ref masked_arr2 = ctx->mk_array_constant(arr2);
    for(auto const& mask_range : this->get_addr_pt_ranges(ctx, addr_pt, bvsize, exprs_to_be_correlated)) {
      masked_arr1 = masked_arr1->apply_mask_and_overlay_array_constant(mask_range, zero_mem_ac);
      masked_arr2 = masked_arr2->apply_mask_and_overlay_array_constant(mask_range, zero_mem_ac);
    }
    if(masked_arr1 != masked_arr2)
      return true;
   
    //ASSERT(m_addrs_that_may_differ.size());
    return false;
  }

  set<pair<expr_ref, expr_ref>> get_addr_pt_ranges(context *ctx, point_ref const& addr_pt, unsigned bvsize, expr_group_ref const& exprs_to_be_correlated) const
  {
    set<pair<expr_ref, expr_ref>> ranges;
    for (auto const& [id, pt_e] : exprs_to_be_correlated->get_expr_vec()) {
      if (pt_e.get_expr()->get_operation_kind() != expr::OP_MEMMASK) {
        auto const& addr_that_may_differ = pt_e.get_addr_that_may_differ();
        size_t nbytes = addr_that_may_differ.get_nbytes();
        bv_const const& addr_const = addr_pt->at(id).get_bv_const();
        expr_ref l = ctx->mk_bv_const(bvsize, mybitset(addr_const, bvsize));
        expr_ref r = ctx->mk_bv_const(bvsize, mybitset(addr_const + nbytes, bvsize));
        ranges.insert(make_pair(l, r));
      }
    }
    return ranges;
  }

  vector<vector<addr_that_may_differ_t>>
  refine_pset_using_addr_pt(vector<addr_that_may_differ_t> const& pset, point_ref const& addr_pt, expr_group_ref const& exprs_to_be_correlated) const
  {
    map<bv_const, vector<addr_that_may_differ_t>> buckets;
    for (auto const& addr : pset) {
      size_t addr_idx = this->get_addr_idx(addr, exprs_to_be_correlated);
      buckets[addr_pt->at(addr_idx).get_bv_const()].push_back(addr);
    }
    vector<vector<addr_that_may_differ_t>> ret;
    for (auto const& b : buckets) {
      ret.push_back(b.second);
    }
    return ret;
  }

  void
  refine_partition_using_addr_pt(point_ref const& addr_pt, expr_group_ref const& exprs_to_be_correlated)
  {
    vector<pair<vector<addr_that_may_differ_t>, partition_set_status_t>> new_partition;
    DYN_DEBUG(eqclass_arr, cout << __func__ << " " << __LINE__ << "  " << this->partition_to_string());
    for (auto const& pelem: m_partition) {
      auto const& pset = pelem.first;
      ASSERT(pset.size());
      vector<vector<addr_that_may_differ_t>> ret = refine_pset_using_addr_pt(pset, addr_pt, exprs_to_be_correlated);
      ASSERT(ret.size() >= 1);
      if (ret.size() > 1) {
        for (auto const& r : ret) {
          new_partition.push_back(make_pair(r, SRC_DST_AGREE/*pelem.second*/)); //optimistically set to agree
        }
      } else {
        new_partition.push_back(pelem);
      }
    }
    m_partition = new_partition;
    DYN_DEBUG(eqclass_arr, cout << __func__ << " " << __LINE__ << "  " << this->partition_to_string());
  }

  size_t get_addr_idx(addr_that_may_differ_t const& addr, expr_group_ref const& exprs_to_be_correlated) const
  {
    for (auto const& [id, pt_e] : exprs_to_be_correlated->get_expr_vec()) {
      if (pt_e.get_expr()->get_operation_kind() != expr::OP_MEMMASK) {
        auto const& addr_that_may_differ = pt_e.get_addr_that_may_differ();
        if (addr_that_may_differ.equals(addr)) {
          return id;
        }
      }
    }
    NOT_REACHED();
  }

  bool arrays_differ_at_addr(array_constant const& arr1, array_constant const& arr2, point_ref const& addr_pt, addr_that_may_differ_t const& addr, sort_ref const& addr_sort, expr_group_ref const& exprs_to_be_correlated) const
  {
    context *ctx = addr_sort->get_context();
    expr_group_t::expr_idx_t addr_idx = this->get_addr_idx(addr, exprs_to_be_correlated);
    point_val_t const& pval = addr_pt->at(addr_idx);
    bv_const const& addr_const = pval.get_bv_const();
    expr_ref addr_const_ex = ctx->mk_bv_const(addr_sort->get_size(), addr_const);
    size_t nbytes = addr.get_nbytes();
    expr_ref data1 = arr1.array_select({ addr_const_ex }, nbytes, false);
    expr_ref data2 = arr2.array_select({ addr_const_ex }, nbytes, false);
    return data1 != data2;
  }

  void label_partition_sets_using_arrays(array_constant const& arr1, array_constant const& arr2, point_ref const& addr_pt, sort_ref const& addr_sort, expr_group_ref const& exprs_to_be_correlated)
  {
    for (auto& pelem : m_partition) {
      if (pelem.second == SRC_DST_DISAGREE) {
        continue;
      }
      auto const& pset = pelem.first;
      ASSERT(pset.size() > 0);
      if (arrays_differ_at_addr(arr1, arr2, addr_pt, pset.at(0), addr_sort, exprs_to_be_correlated)) {
        pelem.second = SRC_DST_DISAGREE;
      }
    }
  }

  void refine_using_point(array_constant const& arr1, array_constant const& arr2, point_ref const& addr_pt, sort_ref const& addr_sort, expr_group_ref const& exprs_to_be_correlated)
  {
    context *ctx = addr_sort->get_context();
    ASSERT(!(arr1 == arr2));
    vector<addr_that_may_differ_t> differing_addrs;
    if (arrays_differ_beyond_addr_vals(ctx, arr1, arr2, addr_pt, exprs_to_be_correlated)) {
      DYN_DEBUG(eqclass_arr, cout << __func__ << ':' << __LINE__ << ": arrays_differ_beyond_addr_vals returned true.\narray1 = " << arr1.to_string() << "\narray2 = " << arr2.to_string() << endl);
      m_arrs_are_unequal = true;
      return;
    }

    this->refine_partition_using_addr_pt(addr_pt, exprs_to_be_correlated);
    this->label_partition_sets_using_arrays(arr1, arr2, addr_pt, addr_sort, exprs_to_be_correlated);
  }

  static indicator_varset_t get_indicator_varset_from_preconds(map<expr_id_t, pair<expr_ref, set<expr_ref>>> const& preconds, context *ctx)
  {
    indicator_varset_t ret;
    for (auto const& precond : preconds) {
      expr_ref const& var = precond.second.first;
      set<expr_ref> const& s = precond.second.second;
      map<expr_id_t, expr_ref> m;
      for (auto const& e : s) {
        m.insert(make_pair(e->get_id(), e));
      }
      ret.push_back(indicator_var_t(var, m));
    }
    return ret;
  }

  predicate_set_t get_preds(expr_ref const& arr1, expr_ref const& arr2) const
  {
    context *ctx = arr1->get_context();
    if (m_arrs_are_unequal) {
      DYN_DEBUG(eqclass_arr, cout << __func__ << ':' << __LINE__ << ": arrays are unequal; no arr predicate generated!\n");
      return predicate_set_t();
    }
    vector<sort_ref> addrs_sort = arr1->get_sort()->get_domain_sort();
    ASSERT(addrs_sort.size() == 1);
    sort_ref addr_sort = addrs_sort.at(0);
    map<expr_id_t, pair<expr_ref, set<expr_ref>>> preconds;
    vector<addr_that_may_differ_t> masks;
    for (size_t i = 0; i < m_partition.size(); i++) {
      auto const& pelem = m_partition.at(i);
      if (pelem.second == SRC_DST_AGREE) {
        continue;
      }
      auto const& pset = pelem.first;
      ASSERT(pset.size() > 0);
      if (pset.size() > 1) {
        memlabel_ref ml = pset_get_memlabel(pset);
        size_t nbytes = pset_get_nbytes(pset);
        expr_ref indicator_var = get_indicator_variable_for_pset_num(i, addr_sort, ctx);
        set<expr_ref> eqaddrs;
        for (auto const& addr : pset) {
          eqaddrs.insert(addr.get_addr());
        }
        preconds[indicator_var->get_id()] = make_pair(indicator_var, eqaddrs);
        masks.push_back(addr_that_may_differ_t(indicator_var, ml, nbytes));
      } else {
        masks.push_back(pset.at(0));
      }
    }

    ASSERT(arr1->get_operation_kind() == expr::OP_MEMMASK);
    ASSERT(arr2->get_operation_kind() == expr::OP_MEMMASK);
    ASSERT(arr1->get_args().at(OP_MEMMASK_ARGNUM_MEMLABEL) == arr2->get_args().at(OP_MEMMASK_ARGNUM_MEMLABEL));
    memlabel_t const& ml = arr1->get_args().at(OP_MEMMASK_ARGNUM_MEMLABEL)->get_memlabel_value();
    expr_ref const& mem_alloc1 = arr1->get_args().at(OP_MEMMASK_ARGNUM_MEM_ALLOC);
    expr_ref const& mem_alloc2 = arr2->get_args().at(OP_MEMMASK_ARGNUM_MEM_ALLOC);

    expr_ref masked_arr1 = arr1->get_args().at(OP_MEMMASK_ARGNUM_MEM);
    expr_ref masked_arr2 = arr2->get_args().at(OP_MEMMASK_ARGNUM_MEM);
    for (auto const& mask : masks) {
      masked_arr1 = apply_mask(masked_arr1, mem_alloc1, mask);
      masked_arr2 = apply_mask(masked_arr2, mem_alloc2, mask);
    }
    expr_ref memeq = ctx->mk_memmasks_are_equal(masked_arr1, mem_alloc1,
                                                masked_arr2, mem_alloc2,
                                                ml);
    memeq = ctx->expr_do_simplify(memeq);
    indicator_varset_t ivset = get_indicator_varset_from_preconds(preconds, ctx);

    stringstream ss;
    if (!ml.memlabel_contains_stack_or_local()) {
      ss << PRED_MEM_PREFIX << PRED_COMMENT_GUESS_MEM_NONSTACK_EQ;
    } else {
      ss << PRED_MEM_PREFIX << PRED_COMMENT_GUESS_MEM_LOCAL_EQ << ml.to_string();
    }
    predicate_ref pred = predicate::mk_predicate_ref(precond_t(ivset, ctx), memeq, expr_true(ctx), ss.str());
    pred = pred->predicate_canonicalized();

    return { pred };
  }

  static expr_ref apply_mask(expr_ref const& arr, expr_ref const& mem_alloc, addr_that_may_differ_t const& mask)
  {
    context *ctx = arr->get_context();
    expr_ref const& addr = mask.get_addr();
    size_t nbytes = mask.get_nbytes();
    memlabel_ref const& ml = mask.get_memlabel();
    expr_ref zerobv = ctx->mk_zerobv(nbytes * BYTE_LEN);
    return ctx->mk_store(arr, mem_alloc, ml->get_ml(), addr, zerobv, nbytes, false);
  }

  static memlabel_ref pset_get_memlabel(vector<addr_that_may_differ_t> const& pset)
  {
    ASSERT(pset.size() > 0);
    return pset.at(0).get_memlabel();
  }
  static size_t pset_get_nbytes(vector<addr_that_may_differ_t> const& pset)
  {
    ASSERT(pset.size() > 0);
    return pset.at(0).get_nbytes();
  }
  static expr_ref indicator_var_precond_get_expr(map<expr_id_t, pair<expr_ref, set<expr_ref>>> const& preconds, context *ctx)
  {
    expr_ref ret = expr_true(ctx);
    for (auto const& precond : preconds) {
      expr_ref indicator_var = precond.second.first;
      expr_ref cur_precond_expr = expr_false(ctx);
      for (const auto& possible_var : precond.second.second) {
        cur_precond_expr = expr_or(cur_precond_expr, expr_eq(indicator_var, possible_var));
      }
      ret = expr_and(ret, cur_precond_expr);
    }
    return ret;
  }
};

template<typename T_PC, typename T_N, typename T_E, typename T_PRED>
class smallest_point_cover_arr_t : public smallest_point_cover_t<T_PC, T_N, T_E, T_PRED>
{
private:
  //vector<addr_that_may_differ_t> m_addrs_that_may_differ;
  //expr_group_ref m_addrs_expr_group;
  //point_set_t m_addr_point_set; //addr points are maintained in the same order as the corresponding array points in m_point_set
  long m_id; //a unique id is assigned to each eqclass_arr_t instance; used for naming quantified variables
  static long m_eqclass_arr_id; //global counter used to assign a unique id to each eqclass instance

public:
  smallest_point_cover_arr_t(point_set_t const& point_set, set<expr_group_t::expr_idx_t> const& out_of_bound_exprs, expr_group_ref const &global_eg_at_p, context *ctx) : smallest_point_cover_t<T_PC, T_N, T_E, T_PRED>(point_set, out_of_bound_exprs, global_eg_at_p, ctx), m_id(m_eqclass_arr_id++)
  {
    //ASSERT(this->m_point_set.size() == m_addr_point_set.size());
  }
  smallest_point_cover_arr_t(smallest_point_cover_arr_t const& other, point_set_t const& point_set, set<expr_group_t::expr_idx_t> const& out_of_bound_exprs) : smallest_point_cover_t<T_PC, T_N, T_E, T_PRED>(other, point_set, out_of_bound_exprs),
                                                                        //m_addrs_that_may_differ(other.m_addrs_that_may_differ),
                                                                        //m_addrs_expr_group(other.m_addrs_expr_group),
                                                                        //m_addr_point_set(other.m_addr_point_set),
                                                                        m_id(other.m_id)
  {
    //ASSERT(this->m_point_set.size() == m_addr_point_set.size());
  }

  smallest_point_cover_arr_t(istream& is, string const& inprefix, context* ctx, point_set_t const& point_set, set<expr_group_t::expr_idx_t> const& out_of_bound_exprs) : smallest_point_cover_t<T_PC, T_N, T_E, T_PRED>(is, inprefix, ctx, point_set, out_of_bound_exprs), m_id(m_eqclass_arr_id++)
  { }


  virtual dshared_ptr<smallest_point_cover_t<T_PC, T_N, T_E, T_PRED>> clone(point_set_t const& point_set, set<expr_group_t::expr_idx_t> const& out_of_bound_exprs) const override
  {
    PRINT_PROGRESS("%s %s() %d:\n", __FILE__, __func__, __LINE__);
    return make_dshared<smallest_point_cover_arr_t<T_PC, T_N, T_E, T_PRED>>(*this, point_set, out_of_bound_exprs);
  }

  virtual void smallest_point_cover_to_stream(ostream& os, string const& inprefix) const override
  {
    string prefix = inprefix + "type arr";
    os << "=" + prefix + "\n";
    this->smallest_point_cover_to_stream_helper(os, prefix + " ");
    //m_addr_point_set.point_set_to_stream(os, prefix + " addr point set ");
  }

  /*void smallest_point_cover_clear_points_and_preds() override
  {
    m_addr_point_set.point_set_clear();
    this->smallest_point_cover_t<T_PC,T_N,T_E,T_PRED>::smallest_point_cover_clear_points_and_preds();
  }*/

private:
  //returns TRUE if we are done
  virtual bool recompute_preds_for_points(bool last_proof_success, unordered_set<shared_ptr<T_PRED const>>& preds) const override
  {
    autostop_timer func_timer(string("smallest_point_cover_arr.") + __func__);
    if (last_proof_success) {
      return true;
    }
    if (this->m_timeout_info.timeout_occurred()) {
      preds = {};
      return true;
    }
    preds = compute_preds_for_arr_points();
    return false;
  }

  static vector<addr_that_may_differ_t>
  get_addrs_that_may_differ(expr_group_ref const& exprs_to_be_correlated)
  {
    vector<addr_that_may_differ_t> ret;
    for (auto const& [id, pt_e] : exprs_to_be_correlated->get_expr_vec()) {
      if (pt_e.get_expr()->get_operation_kind() != expr::OP_MEMMASK) {
        ret.push_back(pt_e.get_addr_that_may_differ());
      }
    }
    return ret;
  }

  predicate_set_t
  compute_preds_for_arr_points() const
  {
    point_set_t const& point_set = this->m_point_set;
    expr_group_ref const& exprs_to_be_correlated = this->get_exprs_to_be_correlated();
    DYN_DEBUG(eqclass_arr, cout << __func__ << " " << __LINE__ << ": entry\n");
    //list<expr_idx_t> const &arr_indices = m_exprs->get_arr_indices();
    ASSERT(exprs_to_be_correlated->expr_group_get_num_exprs() >= 2);
    //if (exprs_to_be_correlated->expr_group_get_num_exprs() == 0) {
    //  return predicate_set_t();
    //}
    //if (exprs_to_be_correlated->expr_group_get_num_exprs() == 1) {
    //  return predicate_set_t(); //this can happen if only esp is present
    //}
    //ASSERT(exprs_to_be_correlated->expr_group_get_num_exprs() == 2);

//    auto begin_idx = expr_group_t::expr_idx_begin();
    expr_ref arr1_expr, arr2_expr;
    expr_group_t::expr_idx_t arr1_idx, arr2_idx;
    for (auto const& [id, pt_e] : exprs_to_be_correlated->get_expr_vec()) {
      if (pt_e.get_expr()->get_operation_kind() == expr::OP_MEMMASK) {
        ASSERT(!arr2_expr);
        if (arr1_expr) {
          arr2_expr = pt_e.get_expr();
          arr2_idx = id;
        } else {
          arr1_expr = pt_e.get_expr();
          arr1_idx = id;
        }
      }
    }
    ASSERT(arr1_expr && arr2_expr);

    ASSERT(arr1_expr->get_sort()->is_array_kind());
    ASSERT(arr1_expr->get_sort() == arr2_expr->get_sort());
    vector<sort_ref> addrs_sort = arr1_expr->get_sort()->get_domain_sort();
    ASSERT(addrs_sort.size() == 1);
    sort_ref addr_sort = addrs_sort.at(0);

    //addr_partition_t addr_partition(this->m_id, m_addrs_that_may_differ);
    auto addrs_that_may_differ = get_addrs_that_may_differ(exprs_to_be_correlated);
    addr_partition_t addr_partition(this->m_id, addrs_that_may_differ);
    map<string_ref, point_ref> const& arr_points_vec = point_set.get_points_vec();
    //vector<point_ref> const& addr_points_vec = m_addr_point_set.get_points_vec();
    //cout << "arr_points_vec.size() = " << arr_points_vec.size() << endl;
    //cout << "addr_points_vec.size() = " << addr_points_vec.size() << endl;
    //ASSERT(this->point_sets_are_consistent());
    //DYN_DEBUG(eqclass_arr,
    //    for(auto const &a : m_addrs_that_may_differ)
    //      cout << __func__ << "  " << __LINE__ << "  " << a.to_string();
    //      cout << __func__ << "  " << __LINE__ << " \n " << point_set.point_set_to_string(" ") << endl;
    //      cout << __func__ << "  " << __LINE__ << " \n " << m_addr_point_set.point_set_to_string(" ") << endl;
    //);

    set<expr_group_t::expr_idx_t> eg_expr_ids = map_get_keys(this->get_exprs_to_be_correlated()->get_expr_vec());
    //set<expr_group_t::expr_idx_t> const& out_of_bound_expr_ids = point_set.get_out_of_bound_exprs();
    set_intersect(eg_expr_ids, this->m_out_of_bound_exprs/*_ids*/);
    ASSERT(!eg_expr_ids.size());

    for (auto const& [ptname, arr_pt] : arr_points_vec) {
      ASSERT(ptname == arr_pt->get_point_name());
      //auto const& arr_pt = arr_points_vec.at(i);
      if (!this->get_visited_ces().count(arr_pt->get_point_name())) 
        continue;
      // All exprs should be defined in a visited point
      ASSERT(this->point_has_mapping_for_all_exprs(arr_pt));

      //auto const& addr_pt = addr_points_vec.at(i);
//      auto begin_idx = expr_group_t::expr_idx_begin();
      point_val_t const& pv1 = arr_pt->at(arr1_idx);
      point_val_t const& pv2 = arr_pt->at(arr2_idx);
      CPP_DBG_EXEC(ASSERTCHECKS,
        if (!((pv1.get_arr_const() == pv2.get_arr_const()) == (*pv1.get_arr_const() == *pv2.get_arr_const()))) {
          cout << "pv1.get_arr_const() = " << pv1.get_arr_const()->to_string() << "\npv2.get_arr_const() = " << pv2.get_arr_const()->to_string() << endl;
          cout << "pv1.get_arr_const() = " << pv1.get_arr_const() << endl;
          cout << "pv2.get_arr_const() = " << pv2.get_arr_const() << endl;
        }
      );
      if (pv1.get_arr_const() != pv2.get_arr_const()) {
        DYN_DEBUG(eqclass_arr, cout << "Arrays mismatch:\npv1.ac = " << pv1.get_arr_const()->to_string() << "\npv2.ac = " << pv2.get_arr_const()->to_string() << endl);
        //addr_partition.refine_using_point(*pv1.get_arr_const(), *pv2.get_arr_const(), addr_pt, addr_sort);
        addr_partition.refine_using_point(*pv1.get_arr_const(), *pv2.get_arr_const(), arr_pt, addr_sort, exprs_to_be_correlated);
      }
    }

    auto ret = addr_partition.get_preds(arr1_expr, arr2_expr);
    DYN_DEBUG(eqclass_arr, cout << __func__ << " " << __LINE__ << ": returning preds\n"; predicate::predicate_set_to_stream(cout, "", ret));
    return ret;
  }

  //static expr_group_ref
  //compute_addrs_expr_group_from_addrs_that_may_differ(vector<addr_that_may_differ_t> const& addrs_that_may_differ)
  //{
  //  map<expr_group_t::expr_idx_t, point_exprs_t> vec;
  //  expr_group_t::expr_idx_t idx = expr_group_t::expr_idx_begin();
  //  for (auto const& a : addrs_that_may_differ) {
  //    vec.insert(make_pair(idx++, point_exprs_t(a.get_addr())));
  //  }
  //  return mk_expr_group(__func__, expr_group_t::EXPR_GROUP_TYPE_MEMADDRS_DIFF, vec);
  //}

  //virtual bool
  //smallest_point_cover_add_point_using_CE(nodece_ref<T_PC, T_N, T_E> const &nce, graph_with_guessing<T_PC, T_N, T_E, T_PRED> const &g) override
  //{
  //  //m_addr_point_set.point_set_add_point_using_CE<T_PC, T_N, T_E, T_PRED>(nce, g);
  //  bool ret = this->smallest_point_cover_t<T_PC, T_N, T_E, T_PRED>::smallest_point_cover_add_point_using_CE(nce, g);
  //  //ASSERT(this->point_sets_are_consistent());
  //  return ret;
  //}

  //virtual void
  //smallest_point_cover_pop_back_last_point() override
  //{
  //  //ASSERT(this->point_sets_are_consistent());
  //  this->smallest_point_cover_t<T_PC, T_N, T_E, T_PRED>::smallest_point_cover_pop_back_last_point();
  //  //m_addr_point_set.pop_back();
  //  //ASSERT(this->point_sets_are_consistent());
  //}

  //bool point_sets_are_consistent() const
  //{
  //  point_set_t const& point_set = this->m_point_set;
  //  vector<point_ref> const& arr_points_vec = point_set.get_points_vec();
  //  vector<point_ref> const& addr_points_vec = m_addr_point_set.get_points_vec();
  //  bool ret =
  //            arr_points_vec.size() == addr_points_vec.size()
  //         || arr_points_vec.size() + 1 == addr_points_vec.size() //this can happen if we are in the middle of smallest_point_cover_add_point_using_CE (recall that our base class's version of this function calls recompute_preds_for_points()
  //  ;
  //  if (!ret) {
  //    cout << _FNLN_ << ": arr points vec size " << arr_points_vec.size() << ", addr points vec size " << addr_points_vec.size() << endl;
  //  }
  //  return ret;
  //}

  virtual failcond_t eqclass_check_stability(/*expr_ref& failcond*/) const override
  {
    if (   this->smallest_point_cover_get_preds().size()
        && !this->m_timeout_info.timeout_occurred()) {
      return failcond_t::failcond_null();
    }
    //ASSERT(compute_preds_for_arr_points().size() == 0); // in case of timeouts we may not get CE which can falsify the preds
    expr_ref memeq = this->get_weakest_possible_arr_eq_pred();
    return failcond_t::failcond_create(memeq, src_dst_cg_path_tuple_t(), this->get_exprs_to_be_correlated()->get_name() + ".stability-failure");
  }

  expr_ref get_weakest_possible_arr_eq_pred() const
  {
    context *ctx = this->m_ctx;
    expr_group_ref const& exprs_to_be_correlated = this->get_exprs_to_be_correlated();
    ASSERT(exprs_to_be_correlated->expr_group_get_num_exprs() >= 2);
    //ASSERT(exprs_to_be_correlated->expr_group_get_num_exprs() <= 2);
    //if (exprs_to_be_correlated->expr_group_get_num_exprs() < 2) {
    //  return expr_true(ctx);
    //}
    //ASSERT(exprs_to_be_correlated->expr_group_get_num_exprs() == 2);

    expr_ref arr1, arr2;
    expr_group_t::expr_idx_t arr1_idx, arr2_idx;
    for (auto const& [id, pt_e] : exprs_to_be_correlated->get_expr_vec()) {
      if (pt_e.get_expr()->get_operation_kind() == expr::OP_MEMMASK) {
        ASSERT(!arr2);
        if (arr1) {
          arr2 = pt_e.get_expr();
          arr2_idx = id;
        } else {
          arr1 = pt_e.get_expr();
          arr1_idx = id;
        }
      }
    }
    ASSERT(arr1 && arr2);

    ASSERT(arr1->get_args().at(OP_MEMMASK_ARGNUM_MEMLABEL) == arr2->get_args().at(OP_MEMMASK_ARGNUM_MEMLABEL));
    memlabel_t const& ml = arr1->get_args().at(OP_MEMMASK_ARGNUM_MEMLABEL)->get_memlabel_value();
    expr_ref const& mem_alloc1 = arr1->get_args().at(OP_MEMMASK_ARGNUM_MEM_ALLOC);
    expr_ref const& mem_alloc2 = arr2->get_args().at(OP_MEMMASK_ARGNUM_MEM_ALLOC);

    expr_ref masked_arr1 = arr1->get_args().at(OP_MEMMASK_ARGNUM_MEM);
    expr_ref masked_arr2 = arr2->get_args().at(OP_MEMMASK_ARGNUM_MEM);

    //for (auto const& mask : m_addrs_that_may_differ) {
    //  masked_arr1 = addr_partition_t::apply_mask(masked_arr1, mem_alloc1, mask);
    //  masked_arr2 = addr_partition_t::apply_mask(masked_arr2, mem_alloc2, mask);
    //}

    for (auto const& [id, pt_e] : exprs_to_be_correlated->get_expr_vec()) {
      if (pt_e.get_expr()->get_operation_kind() != expr::OP_MEMMASK) {
        auto const& mask = pt_e.get_addr_that_may_differ();
        masked_arr1 = addr_partition_t::apply_mask(masked_arr1, mem_alloc1, mask);
        masked_arr2 = addr_partition_t::apply_mask(masked_arr2, mem_alloc2, mask);
      }
    }
    expr_ref memeq = ctx->mk_memmasks_are_equal(masked_arr1, mem_alloc1,
                                                masked_arr2, mem_alloc2,
                                                ml);
    memeq = ctx->expr_do_simplify(memeq);
    return memeq;
  }
};

}
