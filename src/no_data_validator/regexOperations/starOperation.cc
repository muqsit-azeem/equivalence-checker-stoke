#include "empty.h"
#include "plusOperation.h"
#include "concatenationOperation.h"
#include "starOperation.h"

using namespace std;
using namespace stoke;

std::shared_ptr<Operation> StarOperation::connect_regex(std::shared_ptr<Operation> operation) {
  if (operation->getType() == OperationEnum::EMPTY) {
    return std::make_shared<StarOperation>(subexpressions_);
  }
  if (operation->getType() == OperationEnum::SYMBOL) {
    std::vector<std::shared_ptr<Operation>>  concat_vector;
    concat_vector.push_back(std::make_shared<StarOperation>(subexpressions_));
    concat_vector.push_back(operation);
    auto ret_op = std::make_shared<ConcatenationOperation>(concat_vector);
    return ret_op;
  }
  if (operation->getType() == OperationEnum::CONCATENATION) {
    std::vector<std::shared_ptr<Operation>>  concat_vector;
    concat_vector.push_back(std::make_shared<StarOperation>(subexpressions_));
    for (auto op: operation->getSubexpressions()) {concat_vector.push_back(op);}
    auto ret_op = std::make_shared<ConcatenationOperation>(concat_vector);
    return ret_op;
  }
  if (operation->getType() == OperationEnum::PLUS) {
    std::vector<std::shared_ptr<Operation>>  plus_vector;
    for (auto op : operation->getSubexpressions()) {
      auto connected = this->connect_regex(op);
      plus_vector.push_back(connected);
    }
    if (operation->getType() == OperationEnum::STAR) {
      std::vector<std::shared_ptr<Operation>>  concat_vector;
      concat_vector.push_back(std::make_shared<StarOperation>(subexpressions_));
      concat_vector.push_back(operation);
      auto ret_op = std::make_shared<ConcatenationOperation>(concat_vector);
      return ret_op;
    }

    auto ret_op = std::make_shared<PlusOperation>(plus_vector);
    return ret_op;
  }
  return operation;
}