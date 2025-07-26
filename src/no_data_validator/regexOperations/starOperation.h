//
// Created by andrea on 3/26/25.
//

#ifndef STAROPERATION_H
#define STAROPERATION_H

#include <iostream>
#include <memory>
#include <vector>

#include "src/no_data_validator/operation.h"

namespace stoke {
class StarOperation : public Operation {
  public:
    StarOperation(std::vector<std::shared_ptr<Operation>> subexpressions){
      subexpressions_ = subexpressions;
      symbol_ = '*';
    }

    bool equals(const Operation& other) const {
      if (!dynamic_cast<const StarOperation*>(&other)) { return false; }

      auto range = subexpressions_.size();
      auto other_subexpressions = other.getSubexpressions();

      if (range != other_subexpressions.size()) { return false; }

      for (size_t i = 0; i < range; i++) {
        if (!subexpressions_[i]->equals(*other_subexpressions[i])) { return false;}
      }
      return true;
    }

  };
}

#endif //STAROPERATION_H
