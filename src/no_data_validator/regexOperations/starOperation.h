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

  };
}

#endif //STAROPERATION_H
