//Copyright 2025 Andrea Zimovčáková
//
//Licensed under the Apache License, Version 2.0 (the "License");
//you may not use this file except in compliance with the License.
//You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
//Unless required by applicable law or agreed to in writing, software
//distributed under the License is distributed on an "AS IS" BASIS,
//WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//See the License for the specific language governing permissions and
//limitations under the License.

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