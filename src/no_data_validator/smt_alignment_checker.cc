//
// Created by andrea on 6/9/25.
//

#include "smt_alignment_checker.h"
using namespace cpputil;
using namespace std;
using namespace stoke;
using namespace x64asm;
using namespace std::chrono;

bool SmtAlignmentChecker::check(const Cfg& target, const Cfg& rewrite,
            Cfg::id_type target_start, Cfg::id_type rewrite_start,
            const RegEx& p, const RegEx& q,
            std::shared_ptr<Invariant> assume, std::shared_ptr<Invariant> prove,
            bool override_separate_stack,
            void* optional) {
  vector<SymBool> constraints;
  bool separate_stack = separate_stack_ || override_separate_stack;

  state_t.memory = new FlatMemory(separate_stack);
  state_r.memory = new FlatMemory(separate_stack);
  return true;
}
