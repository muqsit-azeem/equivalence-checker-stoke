//
// Created by andrea on 10/15/25.
//

#ifndef EQUIVALENCE_CHECKER_STOKE_OPERATION_ENUM_H
#define EQUIVALENCE_CHECKER_STOKE_OPERATION_ENUM_H

namespace stoke {

  enum class OperationEnum {
    EMPTY = 0,
    SYMBOL = 1,
    CONCATENATION = 2,
    STAR = 3,
    PLUS = 4
  };

} // namespace stoke

#endif //EQUIVALENCE_CHECKER_STOKE_OPERATION_ENUM_H