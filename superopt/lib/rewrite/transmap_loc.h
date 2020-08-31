#ifndef TRANSMAP_LOC_H
#define TRANSMAP_LOC_H
#include <memory>
#include <vector>
#include <sstream>
#include "support/types.h"
#include "support/debug.h"
#include "support/c_utils.h"
#include "valtag/reg_identifier.h"
#include "exec/state.h"
#include "rewrite/stack_offset.h"
//#include "tfg/state.h"

typedef enum { TMAP_LOC_MODIFIER_SRC_SIZED,
               TMAP_LOC_MODIFIER_DST_SIZED } transmap_loc_modifier_t;

class transmap_loc_object_t
{
private:
public:
  //transmap_loc_t() = delete;
  virtual uint8_t get_loc() const = 0;
  virtual dst_ulong get_regnum() const = 0;
  virtual reg_identifier_t const &get_reg_id() const = 0;
  virtual void update_regnum(dst_ulong regnum) = 0;
  virtual void update_reg_id(reg_identifier_t const &regnum) = 0;
  bool overlaps(transmap_loc_object_t const &other) const;
  bool equals(transmap_loc_object_t const &other) const;
};

class transmap_loc_symbol_t : public transmap_loc_object_t
{
private:
  //uint8_t tmap_loc;
  dst_ulong m_addr;
public:
  transmap_loc_symbol_t(uint8_t loc, dst_ulong regnum) : m_addr(regnum)
  {
    ASSERT(loc == TMAP_LOC_SYMBOL);
  }

  virtual ~transmap_loc_symbol_t() {}

  std::string transmap_loc_symbol_to_string() const
  {
    std::stringstream ss;
    ss << " " PEEPTAB_MEM_PREFIX "0x" << std::hex << m_addr << std::dec;
    return ss.str();
  }
  virtual uint8_t get_loc() const
  {
    return TMAP_LOC_SYMBOL;
  }
  virtual dst_ulong get_regnum() const
  {
    return m_addr;
  }
  virtual reg_identifier_t const &get_reg_id() const
  {
    NOT_REACHED();
  }
  virtual void update_regnum(dst_ulong regnum)
  {
    m_addr = regnum;
  }
  virtual void update_reg_id(reg_identifier_t const &reg_id)
  {
    NOT_REACHED();
  }
  bool overlaps(transmap_loc_symbol_t const &other) const
  {
    return m_addr == other.m_addr;
  }
  bool equals(transmap_loc_symbol_t const &other) const
  {
    return m_addr == other.m_addr;
  }

};

class transmap_loc_exreg_t : public transmap_loc_object_t
{
private:
  uint8_t m_loc;
  //dst_ulong m_regnum;
  reg_identifier_t m_reg_id;
  transmap_loc_modifier_t m_modifier;
public:
  transmap_loc_exreg_t(uint8_t loc, reg_identifier_t const &reg_id, transmap_loc_modifier_t mod) : m_loc(loc), m_reg_id(reg_id), m_modifier(mod)
  {
    ASSERT(loc >= TMAP_LOC_EXREG(0) && loc < TMAP_LOC_EXREG(DST_NUM_EXREG_GROUPS));
  }

  virtual ~transmap_loc_exreg_t() {}

  std::string transmap_loc_exreg_to_string() const
  {
    std::stringstream ss;
    ss << " exvr" << (m_loc - TMAP_LOC_EXREG(0)) << "." << m_reg_id.reg_identifier_to_string() << (m_modifier == TMAP_LOC_MODIFIER_SRC_SIZED ? 'S' : 'D');
    return ss.str();
  }
  virtual uint8_t get_loc() const
  {
    return m_loc;
  }

  virtual dst_ulong get_regnum() const
  {
    cout << __func__ << " " << __LINE__ << ": reg_id = " << this->get_reg_id().reg_identifier_to_string() << endl;
    NOT_REACHED();
  }
  virtual reg_identifier_t const &get_reg_id() const
  {
    return m_reg_id;
  }
  virtual void update_regnum(dst_ulong regnum)
  {
    NOT_REACHED();
  }
  virtual void update_reg_id(reg_identifier_t const &reg_id)
  {
    m_reg_id = reg_id;
  }

  bool is_src_sized() const
  {
    return m_modifier == TMAP_LOC_MODIFIER_SRC_SIZED;
  }
  bool is_dst_sized() const
  {
    return m_modifier == TMAP_LOC_MODIFIER_DST_SIZED;
  }
  transmap_loc_modifier_t get_modifier() const
  {
    return m_modifier;
  }
  void set_modifier(transmap_loc_modifier_t mod)
  {
    m_modifier = mod;
  }
  bool overlaps(transmap_loc_exreg_t const &other) const
  {
    if (   m_loc == other.m_loc
        && m_reg_id == other.m_reg_id) {
      return true;
    }
#if ARCH_DST == ARCH_I386
    exreg_group_id_t dst_group = m_loc - TMAP_LOC_EXREG(0);
    exreg_group_id_t other_dst_group = other.m_loc - TMAP_LOC_EXREG(0);
    if (   dst_group >= I386_EXREG_GROUP_EFLAGS_OTHER
        && dst_group <= I386_EXREG_GROUP_EFLAGS_SGE
        && other_dst_group >= I386_EXREG_GROUP_EFLAGS_OTHER
        && other_dst_group <= I386_EXREG_GROUP_EFLAGS_SGE) {
      return true;
    }
#endif
    return false;
  }
  bool equals(transmap_loc_exreg_t const &other) const
  {
    return    m_loc == other.m_loc
           && m_reg_id == other.m_reg_id
           && m_modifier == other.m_modifier;
  }
};

class transmap_loc_t
{
private:
  shared_ptr<transmap_loc_object_t> m_ptr;

  static shared_ptr<transmap_loc_object_t> get_new_ptr_from_obj(transmap_loc_object_t const &obj)
  {
    transmap_loc_symbol_t const *sym = dynamic_cast<transmap_loc_symbol_t const *>(&obj);
    if (sym) {
      return make_shared<transmap_loc_symbol_t>(*sym);
    }
    transmap_loc_exreg_t const *exreg = dynamic_cast<transmap_loc_exreg_t const *>(&obj);
    if (exreg) {
      return make_shared<transmap_loc_exreg_t>(*exreg);
    }
    NOT_REACHED();
  }
public:
  transmap_loc_t() = delete;
  transmap_loc_t transmap_loc__make_exreg_locs_src_sized() const;
  transmap_loc_t(transmap_loc_t const &other) : m_ptr(other.m_ptr)
  {
  }
  transmap_loc_t(transmap_loc_object_t const &obj)
  {
    m_ptr = get_new_ptr_from_obj(obj);
  }
  transmap_loc_id_t get_loc() const
  {
    return m_ptr->get_loc();
  }

  dst_ulong get_regnum() const
  {
    return m_ptr->get_regnum();
  }
  reg_identifier_t const &get_reg_id() const
  {
    return m_ptr->get_reg_id();
  }

  void update_regnum(dst_ulong regnum)
  {
    m_ptr = get_new_ptr_from_obj(*m_ptr); //make a copy
    m_ptr->update_regnum(regnum); //and update the copy; necessary so others holding a reference to orig ptr do not get disturbed
  }

  void scale_and_add_src_env_addr_to_memlocs()
  {
    if (get_loc() == TMAP_LOC_SYMBOL) {
      if (!(get_regnum() >= SRC_ENV_ADDR && get_regnum() < SRC_ENV_ADDR + SRC_ENV_SIZE)) {
        //update_regnum((get_regnum()/STACK_ARG_SIZE)*sizeof(state_reg_t) + SRC_ENV_ADDR);
        update_regnum(get_regnum() + SRC_ENV_ADDR);
      }
    }
  }
  bool transmap_loc_maps_to_src_env_esp_reg() const
  {
    if (get_loc() != TMAP_LOC_SYMBOL) {
      return false;
    }
    return get_regnum() == SRC_ENV_ADDR + DST_ENV_EXOFF(DST_EXREG_GROUP_GPRS, DST_STACK_REGNUM);
  }

  void update_reg_id(reg_identifier_t const &regnum)
  {
    m_ptr = get_new_ptr_from_obj(*m_ptr); //make a copy
    m_ptr->update_reg_id(regnum); //and update the copy; necessary so others holding a reference to orig ptr do not get disturbed
  }

  std::string transmap_loc_to_string() const
  {
    transmap_loc_symbol_t const *sym = dynamic_cast<transmap_loc_symbol_t const *>(m_ptr.get());
    if (sym) {
      return sym->transmap_loc_symbol_to_string();
    }
    transmap_loc_exreg_t const *exreg = dynamic_cast<transmap_loc_exreg_t const *>(m_ptr.get());
    if (exreg) {
      return exreg->transmap_loc_exreg_to_string();
    }
    NOT_REACHED();
  }

  bool is_symbol() const
  {
    transmap_loc_symbol_t const *sym = dynamic_cast<transmap_loc_symbol_t const *>(m_ptr.get());
    return sym != NULL;
  }

  bool is_exreg() const
  {
    transmap_loc_exreg_t const *exreg = dynamic_cast<transmap_loc_exreg_t const *>(m_ptr.get());
    return exreg != NULL;
  }

  transmap_loc_modifier_t get_modifier() const
  {
    transmap_loc_exreg_t const *exreg = dynamic_cast<transmap_loc_exreg_t const *>(m_ptr.get());
    if (!exreg) {
      return TMAP_LOC_MODIFIER_DST_SIZED; //use this as default value
    }
    ASSERT(exreg);
    return exreg->get_modifier();
  }

  void update_modifier(transmap_loc_modifier_t mod)
  {
    transmap_loc_exreg_t *exreg = dynamic_cast<transmap_loc_exreg_t *>(m_ptr.get());
    ASSERT(exreg);
    return exreg->set_modifier(mod);
  }

  bool equals(transmap_loc_t const &other) const
  {
    return m_ptr->equals(*other.m_ptr);
  }

  bool overlaps(transmap_loc_t const &other) const
  {
    return m_ptr->overlaps(*other.m_ptr);
  }

  static std::vector<transmap_loc_t> transmap_locs_from_string(char const *buf)
  {
    char const *ptr = buf;
    std::vector<transmap_loc_t> ret;
    while (ptr) {
      //cout << "src_reg_loc_ptr = " << src_reg_loc_ptr << endl;
      char const *remaining;
      if (strstart(ptr, PEEPTAB_MEM_PREFIX, &remaining)) {
        //locs[num_locs] = TMAP_LOC_SYMBOL;
        //ASSERT(src_reg_group >= 0 && src_reg_group < SRC_NUM_EXREG_GROUPS);
        //dst_regnums[num_locs] = stoi(remaining, NULL, 0);
        dst_ulong addr = (dst_ulong)stol(remaining, NULL, 0);
        transmap_loc_symbol_t sym(TMAP_LOC_SYMBOL, addr);
        ret.push_back(sym);
        //ASSERT(src_regnum >= 0 && src_regnum < SRC_NUM_EXREGS(src_reg_group));
        //ASSERT(src_regnum.reg_identifier_is_valid());
        //dst_regnums[num_locs] = SRC_ENV_ADDR + SRC_ENV_EXOFF(src_reg_group, src_regnum.reg_identifier_get_id());
      } else if (strstart(ptr, "exvr", NULL)) {
        dst_ulong dst_regnum;
        int dst_reg_group;
        char ch;
        int num_read = sscanf(ptr, " exvr%d.%d%c", &dst_reg_group, &dst_regnum, &ch);
        //&dst_regnums[num_locs]
        if (num_read != 3) {
          printf("%s() %d: num_read = %d. ptr = \"%s\"\n", __func__, __LINE__, num_read, ptr);
        }
        ASSERT(num_read == 3);
        //locs[num_locs] = TMAP_LOC_EXREG(dst_reg_group);
        transmap_loc_modifier_t mod = (ch == 'S') ? TMAP_LOC_MODIFIER_SRC_SIZED : (ch == 'D') ? TMAP_LOC_MODIFIER_DST_SIZED : ({NOT_REACHED(); TMAP_LOC_MODIFIER_SRC_SIZED;});
        transmap_loc_exreg_t exreg(TMAP_LOC_EXREG(dst_reg_group), dst_regnum, mod);
        ret.push_back(exreg);
      } else NOT_REACHED();
      ptr = strchr(ptr, ' ');
      while (ptr && *ptr == ' ') {
        ptr++;
      }
      //loc_modifiers[num_locs] = TMAP_LOC_MODIFIER_DST_SIZED; //XXX : FIXME
      //num_locs++;
      //ASSERT(num_locs <= TMAP_MAX_NUM_LOCS);
    }
    return ret;
  }
  transmap_loc_t transmap_loc_make_exreg_locs_src_sized() const;
  void transmap_loc_canonicalize_and_output_mappings(regmap_t &regmap, memslot_map_t &memslot_map);
};

#endif
