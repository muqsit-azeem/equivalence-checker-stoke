//
// Created by andrea on 6/9/25.
//

#ifndef NO_DATA_SMT_ALIGNMENT_CHECKER_H
#define NO_DATA_SMT_ALIGNMENT_CHECKER_H

#include "RegEx.h"
#include "../cfg/cfg.h"

#include "../solver/smtsolver.h"
#include "../validator/invariant.h"
#include "../validator/obligation_checker.h"

namespace stoke {
class SmtAlignmentChecker {
public:
  using Result = ObligationChecker::Result;


  SmtAlignmentChecker(SMTSolver& solver) :
  solver_(solver)
  {
    basic_block_ghosts_ = true;
    nacl_ = false;
    fixpoint_up_ = false;
    separate_stack_ = false;
  }

  ~SmtAlignmentChecker() {}


  bool check(const Cfg& target, const Cfg& rewrite,
               Cfg::id_type target_start, Cfg::id_type rewrite_start,
               const RegEx& p, const RegEx& q,
               std::shared_ptr<Invariant> assume, std::shared_ptr<Invariant> prove,
               bool override_separate_stack,
               void* optional);

  /** Set whether we are going to use separate stack. */
  SmtAlignmentChecker& set_separate_stack(bool b) {
    separate_stack_ = b;
    return *this;
  }

  SmtAlignmentChecker& set_fixpoint_up(bool b) {
    fixpoint_up_ = b;
    return *this;
  }


private:
  bool fixpoint_up_;
  bool separate_stack_;

  SMTSolver& solver_;

};

} // stoke

#endif //NO_DATA_SMT_ALIGNMENT_CHECKER_H
