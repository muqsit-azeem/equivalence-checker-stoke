//Copyright 2025 Andrea Zimovčáková
//
//Licensed under the Apache License, Version 2.0 (the "License");
//you may not use this file except in compliance with the License.
//You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
//Unless required by applicable law or agreed to in writing, software
//distributed under the License is distributed on an "AS IS" BASIS,
//WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//See the License for the specific language governing permissions and
//limitations under the License.


#ifndef STOKE_SRC_VALIDATOR_NODATA_H
#define STOKE_SRC_VALIDATOR_NODATA_H

#include "smt_alignment_checker.h"
#include "src/validator/paa.h"
#include "src/validator/data_collector.h"
#include "src/validator/invariant.h"
#include "src/validator/learner.h"
#include "src/validator/obligation_checker.h"
#include "src/validator/validator.h"
#include "src/validator/ddec.h"

namespace stoke {

class NoDataValidator : public DdecValidator {

public:
  NoDataValidator(ObligationChecker& checker, Sandbox& sandbox, InvariantLearner& inv)
    : DdecValidator(checker, sandbox, inv) {
    solver_ = new Z3Solver();
    alignment_checker_ = new SmtAlignmentChecker(solver_);
  }

  NoDataValidator(const NoDataValidator& rhs) : DdecValidator(rhs) {}

  void printing_cfg();
  bool build_paa_for_alignment_predicate(std::shared_ptr<Invariant> inv, ProgramAlignmentAutomata& paa) override;

private:
  SmtAlignmentChecker* alignment_checker_;
  SMTSolver* solver_;
};

} // namespace stoke

#endif