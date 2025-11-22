#include "starOperation.h"
#include "plusOperation.h"
#include "concatenationOperation.h"
#include "empty.h"


using namespace std;
using namespace stoke;

std::shared_ptr<Operation> Symbol::connect_regex(std::shared_ptr<Operation> operation) {
  if (empty) {return  operation;}

  if (operation->getType() == OperationEnum::EMPTY) {
    return std::make_shared<Symbol>(number_);
  }
  if (operation->getType() == OperationEnum::SYMBOL || operation->getType() == OperationEnum::STAR) {
    std::vector<std::shared_ptr<Operation>>  concat_vector;
    concat_vector.push_back(std::make_shared<Symbol>(number_));
    concat_vector.push_back(operation);
    auto ret_op = std::make_shared<ConcatenationOperation>(concat_vector);
    return ret_op;
  }
  if (operation->getType() == OperationEnum::CONCATENATION) {
    std::vector<std::shared_ptr<Operation>>  concat_vector;
    concat_vector.push_back(std::make_shared<Symbol>(number_));
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

    auto ret_op = std::make_shared<PlusOperation>(plus_vector);
    return ret_op;
  }
  return operation;
}