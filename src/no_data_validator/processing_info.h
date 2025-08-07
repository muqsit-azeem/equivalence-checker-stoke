//
// Created by andrea on 8/2/25.
//

#ifndef PROCESSINGINFO_H
#define PROCESSINGINFO_H
#include <gmpxx.h>
#include "../symstate/bitvector.h"

namespace stoke {

  class ProcessingInfo {

  public:

    ProcessingInfo() {
      previous_state = std::numeric_limits<size_t>::max();

      prev_state_ends_with_jump = false;
    }

    size_t previous_state;

    bool prev_state_ends_with_jump;
    std::vector<SymBool> star_constraints;
    std::vector<SymBool> memory_axioms;
	SymBitVector number_of_bits_changed = SymBitVector::constant(64, 0);

  };

};

#endif //PROCESSINGINFO_H
