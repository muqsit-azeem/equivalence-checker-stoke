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

#include "src/no_data_validator/RegEx.h"
#include "cvc4/util/bitvector.h"
#include "src/no_data_validator/regexOperations/empty.h"
#include "src/no_data_validator/regexOperations/concatenationOperation.h"
#include "src/no_data_validator/regexOperations/starOperation.h"
#include "src/no_data_validator/regexOperations/plusOperation.h"

using namespace stoke;
using namespace std;

void RegEx::dfs(size_t curr, std::set<size_t>& visited, bool find_succ)
{
  if (visited.find(curr) != visited.end()) {
    return;
  }
  visited.insert(curr);

  auto begin_iterator = find_succ ? cfg_.succ_begin(curr) : cfg_.pred_begin(curr);
  auto end_iterator = find_succ ? cfg_.succ_end(curr) : cfg_.pred_end(curr);

  while (begin_iterator != end_iterator) {
    dfs(*begin_iterator, visited, find_succ);
    ++begin_iterator;
  }
}

bool RegEx::initializeRegEx(size_t start, size_t end, std::map<std::tuple<size_t,size_t>,
    std::shared_ptr<Operation>>& result, std::set<size_t>& nodes_between, std::map<size_t, std::set<size_t>>& succs_map,
    std::map<size_t, std::set<size_t>>& preds_map) {
  std::set<size_t> succ;
  dfs(start, succ, true);

  std::set<size_t> pred;
  dfs(end, pred, false);

  std::set_intersection(
      succ.begin(), succ.end(),
      pred.begin(), pred.end(),
      std::inserter(nodes_between, nodes_between.begin())
  );

  if (nodes_between.empty()) {    //path from start to end doesn't exist
    return false;
  }

  for (size_t node : nodes_between) {
    auto succs = cfg_.succ_begin(node);
    while (succs != cfg_.succ_end(node)) {
      if (nodes_between.find(*succs) != nodes_between.end()) {
        result.insert(std::make_pair(std::make_tuple(node, *succs), std::make_shared<Symbol>(node)));

        if (succs_map.find(node) == succs_map.end()) {
          succs_map.insert(std::make_pair(node, std::set<size_t>()));
        }
        succs_map[node].insert(*succs);

        if (preds_map.find(*succs) == preds_map.end()) {
          preds_map.insert(std::make_pair(*succs, std::set<size_t>()));
        }
        preds_map[*succs].insert(node);
      }
      ++succs;
    }
  }

  if (start == end) {
    auto self_pair = std::tuple<size_t,size_t>(start,end);
    if (result.find(self_pair) == result.end()) {
      result.insert(std::make_pair(self_pair,std::make_shared<Symbol>()));
    }
  }

  return !result.empty();
}


void RegEx::joinEdgesSplit(size_t pred, size_t succ, size_t node, std::map<std::tuple<size_t,size_t>,
    std::shared_ptr<Operation>>& regex_map, bool contains_loop) {
  std::tuple<size_t,size_t> node_to_succ = std::make_tuple(node, succ);
  std::tuple<size_t,size_t> pred_to_node = std::make_tuple(pred, node);
  std::tuple<size_t,size_t> pred_to_succ = std::make_tuple(pred, succ);

  bool pred_empty = regex_map[pred_to_node]->isEmpty();
  bool succ_empty = regex_map[node_to_succ]->isEmpty();

  std::shared_ptr<Operation> op;
  std::vector<std::shared_ptr<Operation>> subexpressions;

  if (!pred_empty) {
    subexpressions.push_back(regex_map[pred_to_node]);
  }
  if (contains_loop) {
    auto existing_operation = regex_map[std::make_tuple(node, node)];

    if (!existing_operation->isEmpty()) {
      std::shared_ptr<StarOperation> star = std::make_shared<StarOperation>(existing_operation);
      subexpressions.push_back(star);
    }
  }
  if (!succ_empty) {
    subexpressions.push_back(regex_map[node_to_succ]);
  }

  if (subexpressions.empty()) {
    op = std::make_shared<Symbol>();
  } else if (subexpressions.size() == 1) {
    op = subexpressions[0];
  } else {
    op = subexpressions[0];
    if (subexpressions.size() == 3) {
      op = op->connect_regex(subexpressions[1]);
    }
    op = op->connect_regex(subexpressions.back());
  }

  if (regex_map.find(pred_to_succ) == regex_map.end()) {
    regex_map[pred_to_succ] = op;
  } else {
    std::shared_ptr<Operation> existing_op = regex_map[pred_to_succ];
    if (auto existing_op_casted = std::dynamic_pointer_cast<PlusOperation>(existing_op)) {
      existing_op_casted->add_subexpression(op);
    } else {
      std::vector<std::shared_ptr<Operation>> plus_vector;
      plus_vector.push_back(op);
      plus_vector.push_back(existing_op);
      std::shared_ptr<PlusOperation> plus_operation = std::make_shared<PlusOperation>(plus_vector);
      regex_map[pred_to_succ] = plus_operation;
    }
  }

}

// this function expects the subexpression to be at least of size 2
void RegEx::connectRegex(shared_ptr<Operation>& op, std::vector<std::shared_ptr<Operation>>& subexpressions) {
  op = subexpressions[0];

  if (subexpressions[1]->getType() == OperationEnum::PLUS) {
    if (op->getType() == OperationEnum::PLUS) {
      std::vector<std::shared_ptr<Operation>>  plus_vector;

      for (auto first_subex: op->getSubexpressions()) { // should not be PlusOperation
        for (auto last_subex : subexpressions[1]->getSubexpressions()) { // should not be PlusOperation
          if (first_subex->isEmpty()) {
            plus_vector.push_back(last_subex);
          } else if (auto first_op = std::dynamic_pointer_cast<ConcatenationOperation>(first_subex)) {
            first_op->add_subexpression(last_subex);
            plus_vector.push_back(first_op);
          } else {
            std::shared_ptr<ConcatenationOperation> new_operation = std::make_shared<ConcatenationOperation>(first_subex);
            new_operation->add_subexpression(last_subex);
            plus_vector.push_back(new_operation);
          }
        }
      }
    }
  }
}

void RegEx::sympifyRegex(size_t start, size_t end, std::map<std::tuple<size_t,size_t>,
    std::shared_ptr<Operation>>& regex_map, std::set<size_t>& nodes_between, std::map<size_t,
    std::set<size_t>>& succs, std::map<size_t, std::set<size_t>>& preds) {
  tuple<size_t,size_t> start_state = make_tuple(std::numeric_limits<size_t>::max(), start);
  tuple<size_t,size_t> end_state = make_tuple(end, std::numeric_limits<size_t>::max());
  regex_map.insert(std::make_pair(start_state, std::make_shared<Symbol>()));
  regex_map.insert(std::make_pair(end_state, std::make_shared<Symbol>()));

  if (preds.find(start) == preds.end()) { preds.insert(std::make_pair(start, std::set<size_t>()));}
  preds[start].insert(std::numeric_limits<size_t>::max());
  if (succs.find(end) == succs.end()) {succs.insert(std::make_pair(end, std::set<size_t>()));}
  succs[end].insert(std::numeric_limits<size_t>::max());

  for (size_t node : nodes_between) {
    bool contains_loop = regex_map.find(std::make_tuple(node,node)) != regex_map.end();
    for (size_t pred : preds[node]) {
      if (pred == node) {
        continue;
      }

      for (size_t succ : succs[node]) {
        if (succ == node) {
          continue;
        }

        joinEdgesSplit(pred, succ, node, regex_map, contains_loop);

      }

      succs[pred].erase(node);
      regex_map.erase(std::make_tuple(pred,node));

      for (size_t succ : succs[node]) {
        preds[succ].insert(pred);
        if (node != succ) {
          succs[pred].insert(succ);
        }
      }
    }

    for (size_t succ : succs[node]) {
      preds[succ].erase(node);
      regex_map.erase(std::make_tuple(node, succ));
    }

    if (contains_loop) { regex_map.erase(std::make_tuple(node,node)); }
    succs.erase(node);
    preds.erase(node);
  }
}

bool RegEx::getRegex(size_t start, size_t end, std::shared_ptr<Operation>& regex) {
  if (regex_in_cfg.find(std::make_tuple(start,end)) != regex_in_cfg.end()) {
    regex = regex_in_cfg[std::make_tuple(start,end)];
    return true;
  }

  std::map<std::tuple<size_t,size_t>, std::shared_ptr<Operation>> regex_on_edges;
  std::set<size_t> nodes_between;
  std::map<size_t, std::set<size_t>> succs;
  std::map<size_t, std::set<size_t>> preds;

  if (!initializeRegEx(start, end, regex_on_edges, nodes_between, succs, preds)) {  // if rex is empty set
    return false;
  }

  sympifyRegex(start, end, regex_on_edges, nodes_between, succs, preds);

  regex = regex_on_edges[make_tuple(std::numeric_limits<size_t>::max(), std::numeric_limits<size_t>::max())];

  regex_in_cfg[make_tuple(start, end)] = regex;
  return true;
}

bool RegEx::get_CfgPath_base(CfgPath& cfg_path, std::shared_ptr<Operation>& regex, std::map<string, uint64_t> star_map) {
  if (regex->isEmpty()) { return false; }
  if (auto symbol = dynamic_pointer_cast<Symbol>(regex)) {
    cfg_path.push_back(symbol->getNumber());
    return true;
  }
  if (auto concat = dynamic_pointer_cast<ConcatenationOperation>(regex)) {
    bool result = true;
    for (auto operation : concat->getSubexpressions()) {
      bool bool_from_operation = get_CfgPath_base(cfg_path, operation, star_map);
      result = bool_from_operation;
    }
    return result;
  }
  if (auto star = dynamic_pointer_cast<StarOperation>(regex)) {
    CfgPath temp;
    for (auto operation : star->getSubexpressions()) {
      if (!get_CfgPath_base(temp, operation, star_map)) { return false; }
    }

    if (star_map.find(star->get_loop_var()) == star_map.end()) {
      std::cout << "SOMETHING WENT WRONG" << std::endl;
      return false;
    }

    for (uint64_t i = 0; i < star_map[star->get_loop_var()]; i++) {
      cfg_path.insert(cfg_path.end(), temp.begin(), temp.end());
    }
    return !star->is_plus_one();
  }
  return false;
}

bool RegEx::get_CfgPath(CfgPath& cfg_path, std::shared_ptr<Operation>& regex, std::map<string, uint64_t> star_map) {
  get_CfgPath_base(cfg_path, regex, star_map);

  return true;
}