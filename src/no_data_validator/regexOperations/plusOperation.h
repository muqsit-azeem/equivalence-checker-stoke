//
// Created by andrea on 3/26/25.
//

#ifndef PLUSOPERATION_H
#define PLUSOPERATION_H

#include <iostream>
#include <memory>
#include <vector>

#include "src/no_data_validator/operation.h"

namespace stoke {

  class PlusOperation : public Operation {
  public:
    PlusOperation(std::vector<std::shared_ptr<Operation>> subexpressions) {
      subexpressions_ = subexpressions;
      symbol_ = '+';
    }

    void add_subexpression(std::shared_ptr<Operation> subexpression) {
      subexpressions_.push_back(subexpression);
    }
  };
}

#endif //PLUSOPERATION_H
