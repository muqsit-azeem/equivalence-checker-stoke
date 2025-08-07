
#ifndef STOKE_SRC_VALIDATOR_NODATA_H
#define STOKE_SRC_VALIDATOR_NODATA_H

#include "operation.h"
#include "smt_alignment_checker.h"
#include "cvc4/expr/datatype.h"
#include "src/solver/cvc4solver.h"
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
  //void NoDataValidator::add_edge_to_paa(ProgramAlignmentAutomata::Edge& edge, ProgramAlignmentAutomata& paa);

  SmtAlignmentChecker* alignment_checker_;
  SMTSolver* solver_;
};

} // namespace stoke

#endif