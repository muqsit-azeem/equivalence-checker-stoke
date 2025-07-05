
#ifndef STOKE_SRC_VALIDATOR_NODATA_H
#define STOKE_SRC_VALIDATOR_NODATA_H

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
      : DdecValidator(checker, sandbox, inv) {}

    NoDataValidator(const NoDataValidator& rhs) : DdecValidator(rhs) {}

    void printing_cfg();
    bool build_paa_for_alignment_predicate(std::shared_ptr<Invariant> inv, ProgramAlignmentAutomata& paa) override;
};

} // namespace stoke

#endif