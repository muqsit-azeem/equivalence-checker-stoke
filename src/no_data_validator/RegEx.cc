//
// Created by andrea on 3/18/25.
//

#include "src/no_data_validator/RegEx.h"

#include "src/no_data_validator/regexOperations/empty.h"
#include "src/no_data_validator/regexOperations/concatenationOperation.h"
#include "src/no_data_validator/regexOperations/starOperation.h"
#include "src/no_data_validator/regexOperations/plusOperation.h"

using namespace stoke;
using namespace std;

#define INPUT_STOP(X) { std::cout << "****************" << X << "****************" << std::endl; std::string input; std::cin >> input; }

void RegEx::dfs(size_t curr, std::set<size_t>& visited, bool find_succ)
{
  if (visited.find(curr) != visited.end()) {
    return;
  }
  visited.insert(curr);

  auto begin_iterator = cfg_.succ_begin(curr);
  auto end_iterator = cfg_.succ_end(curr);
  if (!find_succ) {
    begin_iterator = cfg_.pred_begin(curr);
    end_iterator = cfg_.pred_end(curr);
  }

  while (begin_iterator != end_iterator) {
    dfs(*begin_iterator, visited, find_succ);
    ++begin_iterator;
  }
}

bool RegEx::getPath(size_t start, size_t end, std::map<std::tuple<size_t,size_t>, std::shared_ptr<Operation>>& result,
std::set<size_t>& nodes_between, std::map<size_t, std::set<size_t>>& succs_map, std::map<size_t, std::set<size_t>>& preds_map) {
  cout << "Start " << start << " End " << end << endl;
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
    for (size_t _ = 0; _ < cfg_.succ_size(node); ++_) {
      if (nodes_between.find(*succs) != nodes_between.end()) {
        if (node == cfg_.get_entry()) {
          result.insert(std::make_pair(std::tuple<size_t,size_t>(node,*succs),std::make_shared<Symbol>()));
        } else {
          result.insert(std::make_pair(std::make_tuple(node, *succs), std::make_shared<Symbol>(node)));
        }

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

  return !result.empty();     // if start == end intersection won't be empty but the map is going to be if there is no (start, start) edge
}

void RegEx::joinEdges(size_t pred, size_t succ, size_t node, std::map<std::tuple<size_t,size_t>, std::shared_ptr<Operation>>& regex_map, bool contains_loop) {

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
    std::vector<std::shared_ptr<Operation>> star_vector;
    star_vector.push_back(regex_map[std::make_tuple(node, node)]);
    std::shared_ptr<StarOperation> star = std::make_shared<StarOperation>(star_vector);
    subexpressions.push_back(star);
  }
  if (!succ_empty) {
    subexpressions.push_back(regex_map[node_to_succ]);
  }

  if (subexpressions.empty()) {
    op = std::make_shared<Symbol>();
  } else if (subexpressions.size() == 1) {
    op = subexpressions[0];
  } else {
    std::shared_ptr<ConcatenationOperation> baseConc;

    if (auto conc_op = std::dynamic_pointer_cast<ConcatenationOperation>(subexpressions[0])) {
      baseConc = conc_op;
    } else {
      std::vector<std::shared_ptr<Operation>> subexpressions_conc;
      subexpressions_conc.push_back(subexpressions[0]);
      baseConc = std::make_shared<ConcatenationOperation>(subexpressions_conc);
    }

    if (auto conc_op = std::dynamic_pointer_cast<ConcatenationOperation>(subexpressions[1])) {
      for (auto op_: conc_op->getSubexpressions()) {
        baseConc->add_subexpression(op_);
      }
    } else {
      baseConc->add_subexpression(subexpressions[1]);
    }

    if (subexpressions.size() == 3) {
      if (auto conc_op = std::dynamic_pointer_cast<ConcatenationOperation>(subexpressions[2])) {
        for (auto op_: conc_op->getSubexpressions()) {
          baseConc->add_subexpression(op_);
        }
      } else {
        baseConc->add_subexpression(subexpressions[2]);
      }
    }

    op = baseConc;
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

  regex_map.erase(node_to_succ);
}

/*
 * In regex_map should exist just one instance of edge at time
 */
void RegEx::sympifyRegex(size_t start, size_t end, std::map<std::tuple<size_t,size_t>,std::shared_ptr<Operation>>& regex_map,
std::set<size_t>& nodes_between, std::map<size_t, std::set<size_t>>& succs, std::map<size_t, std::set<size_t>>& preds) {
  //INPUT_STOP("SIMPLIFY REGEX")
  tuple<size_t,size_t> START = make_tuple(std::numeric_limits<size_t>::max(), start);
  tuple<size_t,size_t> END = make_tuple(end, std::numeric_limits<size_t>::max());
  regex_map.insert(std::make_pair(START, std::make_shared<Symbol>()));
  regex_map.insert(std::make_pair(END, std::make_shared<Symbol>()));

  if (preds.find(start) == preds.end()) { preds.insert(std::make_pair(start, std::set<size_t>()));}
  preds[start].insert(std::numeric_limits<size_t>::max());
  if (succs.find(end) == succs.end()) {succs.insert(std::make_pair(end, std::set<size_t>()));}
  succs[end].insert(std::numeric_limits<size_t>::max());

  for (size_t node : nodes_between) {
    bool contains_loop = (regex_map.find(std::make_tuple(node,node)) != regex_map.end());
    for (size_t pred : preds[node]) {
      if (pred == node) {
        continue;
      }
      for (size_t succ : succs[node]) {
        if (succ == node) {
          continue;
        }

        joinEdges(pred, succ, node, regex_map, contains_loop);

        preds[succ].erase(node);
        preds[succ].insert(pred);
        succs[pred].insert(succ);
      }
      succs[pred].erase(node);
      regex_map.erase(std::make_tuple(pred,node));
    }
    if (contains_loop) {regex_map.erase(std::make_tuple(node,node));}
    succs.erase(node);
    preds.erase(node);
  }
}

bool RegEx::getRegex(size_t start, size_t end, std::shared_ptr<Operation>& regex) {
  std::map<std::tuple<size_t,size_t>, std::shared_ptr<Operation>> regex_on_edges;
  std::set<size_t> nodes_between;
  std::map<size_t, std::set<size_t>> succs;
  std::map<size_t, std::set<size_t>> preds;

  if (!getPath(start, end, regex_on_edges, nodes_between, succs, preds)) {  // if rex is empty set
    return false;
  }

  std::cout << "PRINTING THE MAP" << std::endl;
  for (const auto& pair : regex_on_edges) {
    size_t a, b;
    std::tie(a, b) = pair.first; // unpack the tuple
    std::shared_ptr<Operation> op  = pair.second;

    std::cout << "(" << a << ", " << b << ") => ";
    std::cout << *op << std::endl;
  }

  sympifyRegex(start, end, regex_on_edges, nodes_between, succs, preds);

  regex = regex_on_edges[make_tuple(std::numeric_limits<size_t>::max(), std::numeric_limits<size_t>::max())];
  //regex = std::make_shared<Symbol>();

  std::cout << "REGEX: " << *regex << std::endl;
  return true;
}