#include "starOperation.h"
#include "empty.h"
#include "plusOperation.h"
#include "concatenationOperation.h"

using namespace std;
using namespace stoke;

std::shared_ptr<Operation> ConcatenationOperation::connect_regex(std::shared_ptr<Operation> operation) {
  if (operation->getType() == OperationEnum::EMPTY) {
    return std::make_shared<ConcatenationOperation>(subexpressions_);
  }
  if (operation->getType() == OperationEnum::SYMBOL || operation->getType() == OperationEnum::CONCATENATION || operation->getType() == OperationEnum::STAR) {
    auto ret_op = std::make_shared<ConcatenationOperation>(subexpressions_);
    ret_op->add_subexpression(operation);
    return ret_op;
  }
  if (operation->getType() == OperationEnum::PLUS) {
    std::vector<std::shared_ptr<Operation>> plus_vector;
    for (auto op : operation->getSubexpressions()) {
      auto connected = std::make_shared<ConcatenationOperation>(subexpressions_);
      connected->add_subexpression(op);
      plus_vector.push_back(connected);
    }

    auto ret_op = std::make_shared<PlusOperation>(plus_vector);
    return ret_op;
  }
  return operation;
}