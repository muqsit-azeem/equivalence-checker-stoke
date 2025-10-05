//
// Created by andrea on 3/18/25.
//

#include "src/no_data_validator/RegEx.h"

#include "cvc4/util/bitvector.h"
#include "src/no_data_validator/regexOperations/empty.h"
#include "src/no_data_validator/regexOperations/concatenationOperation.h"
#include "src/no_data_validator/regexOperations/starOperation.h"
#include "src/no_data_validator/regexOperations/plusOperation.h"

using namespace stoke;
using namespace std;

#define INPUT_STOP(X) { std::cout << "****************" << X << "****************" << std::endl;} // std::string input; std::cin >> input; }

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

  std::cout << "nodes: " << nodes_between.size() << std::endl;
  for (auto nodes : nodes_between)
  {
    std::cout << nodes << " " ;
  }
  std::cout << std::endl;


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
/*
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
    auto existing_operation = regex_map[std::make_tuple(node, node)];
    if (!existing_operation->isEmpty()) {
      std::vector<std::shared_ptr<Operation>> star_vector;
      star_vector.push_back(existing_operation);
      std::shared_ptr<StarOperation> star = std::make_shared<StarOperation>(star_vector);
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
*/

// TODO: test
void RegEx::joinEdgesSplit(size_t pred, size_t succ, size_t node, std::map<std::tuple<size_t,size_t>,
    std::shared_ptr<Operation>>& regex_map, bool contains_loop) {
  std::cout << "pred: " << pred << " node: " << node << " succ: " << succ << " loop:" << contains_loop << std::endl;

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

    std::cout << "existing operation: " << *existing_operation << std::endl;
    if (!existing_operation->isEmpty()) {
      std::shared_ptr<StarOperation> star = std::make_shared<StarOperation>(existing_operation);
      subexpressions.push_back(star);
    }
  }
  if (!succ_empty) {
    subexpressions.push_back(regex_map[node_to_succ]);
  }

  std::cout << "subexpression count: " << subexpressions.size() << std::endl;

  if (subexpressions.empty()) {
    op = std::make_shared<Symbol>();
  } else if (subexpressions.size() == 1) {
    op = subexpressions[0];
  } else {
    if (auto plus_op = std::dynamic_pointer_cast<PlusOperation>(subexpressions[0])) {
      std::vector<std::shared_ptr<Operation>> final_plus_vector = std::vector<std::shared_ptr<Operation>> (plus_op->getSubexpressions());
      std::shared_ptr<PlusOperation> final_plus_op = std::make_shared<PlusOperation>(plus_op->getSubexpressions());

      if (subexpressions.size() == 3) {
        final_plus_op->add_to_every_subexpression(subexpressions[1]); // will be a star
      }

      if (auto last_plus_op = std::dynamic_pointer_cast<PlusOperation>(subexpressions.back())) {
        std::vector<std::shared_ptr<Operation>>  plus_vector;

        for (auto first_subex: final_plus_op->getSubexpressions()) { // should not be PlusOperation
          for (auto last_subex : last_plus_op->getSubexpressions()) { // should not be PlusOperation
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

        std::shared_ptr<PlusOperation> new_plus_operation = std::make_shared<PlusOperation>(plus_vector);
        op = new_plus_operation;
      } else {
        final_plus_op->add_to_every_subexpression(subexpressions.back());
        op = final_plus_op;
      }
    } else {
      std::vector<std::shared_ptr<Operation>>  operation_vector;

      if (auto first_op = std::dynamic_pointer_cast<ConcatenationOperation>(subexpressions[0])) {
        operation_vector = first_op->getSubexpressions();
      } else{
        operation_vector.push_back(subexpressions[0]);
      }

      if (subexpressions.size() == 3) {
        operation_vector.push_back(subexpressions[1]);
      }

      if (auto last_plus_op = std::dynamic_pointer_cast<PlusOperation>(subexpressions.back())) {
        std::vector<std::shared_ptr<Operation>>  plus_vector;
        for (auto in_plus_operation : last_plus_op->getSubexpressions()) {
          std::vector<std::shared_ptr<Operation>>  new_operation_vector;
          new_operation_vector.insert(new_operation_vector.begin(), operation_vector.begin(), operation_vector.end());
          new_operation_vector.push_back(in_plus_operation);
          std::shared_ptr<ConcatenationOperation> new_operation = std::make_shared<ConcatenationOperation>(new_operation_vector);

          plus_vector.push_back(new_operation);
        }
        std::shared_ptr<PlusOperation> plus_operation = std::make_shared<PlusOperation>(plus_vector);
        op = plus_operation;
      } else {
        operation_vector.push_back(subexpressions.back());
        std::shared_ptr<ConcatenationOperation> new_operation = std::make_shared<ConcatenationOperation>(operation_vector);
        op = new_operation;
      }
    }
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

void RegEx::sympifyRegex(size_t start, size_t end, std::map<std::tuple<size_t,size_t>,
    std::shared_ptr<Operation>>& regex_map, std::set<size_t>& nodes_between, std::map<size_t,
    std::set<size_t>>& succs, std::map<size_t, std::set<size_t>>& preds) {
  //INPUT_STOP("SIMPLIFY REGEX")
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

        preds[succ].erase(node);
        preds[succ].insert(pred);
        succs[pred].insert(succ);
      }

      succs[pred].erase(node);
      regex_map.erase(std::make_tuple(pred,node));
    }

    if (contains_loop) { regex_map.erase(std::make_tuple(node,node)); }
    succs.erase(node);
    preds.erase(node);
  }
}

bool RegEx::getRegex(size_t start, size_t end, std::shared_ptr<Operation>& regex) {
  std::cout << "Finding regex from " << start << " to " << end << std::endl;

  if (regex_in_cfg.find(std::make_tuple(start,end)) != regex_in_cfg.end()) {
    regex = regex_in_cfg[std::make_tuple(start,end)];
    std::cout << "REGEX existed: " << *regex << std::endl;
    return true;
  }

  std::map<std::tuple<size_t,size_t>, std::shared_ptr<Operation>> regex_on_edges;
  std::set<size_t> nodes_between;
  std::map<size_t, std::set<size_t>> succs;
  std::map<size_t, std::set<size_t>> preds;

  if (!initializeRegEx(start, end, regex_on_edges, nodes_between, succs, preds)) {  // if rex is empty set
    std::cout << "PATH NOT FOUND" << std::endl;
    return false;
  }

  /*std::cout << "PRINTING THE MAP" << std::endl;
  for (const auto& pair : regex_on_edges) {
    size_t a, b;
    std::tie(a, b) = pair.first; // unpack the tuple
    std::shared_ptr<Operation> op  = pair.second;

    std::cout << "(" << a << ", " << b << ") => ";
    std::cout << *op << std::endl;
  }
  */

  sympifyRegex(start, end, regex_on_edges, nodes_between, succs, preds);

  regex = regex_on_edges[make_tuple(std::numeric_limits<size_t>::max(), std::numeric_limits<size_t>::max())];




  /*//TODO: if end is loop then at the end should be star operation from end and the end
  if (cfg_.has_conditional_target(end)) {
    auto condition = cfg_.conditional_target(end);
    if (condition == end) {
      std::shared_ptr<StarOperation> star = std::make_shared<StarOperation>(std::make_shared<Symbol>(end));

      if (regex->isEmpty()) { //should not happend
        regex = star;
      } else if (auto casted = dynamic_pointer_cast<Symbol>(regex)) {
        auto concat = make_shared<ConcatenationOperation>(casted);
        concat->add_subexpression(star);
        regex = concat;
      } else if (auto casted = dynamic_pointer_cast<PlusOperation>(regex)) {
        casted->add_to_every_subexpression(star);
        regex = casted;
      } else if (auto casted = dynamic_pointer_cast<ConcatenationOperation>(regex)) {
        casted->add_subexpression(star);
        regex = casted;
      } else if (auto casted = dynamic_pointer_cast<StarOperation>(regex)) {
        auto concat = make_shared<ConcatenationOperation>(casted);
        concat->add_subexpression(star);
        regex = concat;
      }
    }
  }*/
  /*if (!regex->isEmpty()) {
    if (auto casted = dynamic_pointer_cast<Symbol>(regex)) {
      auto concat = make_shared<ConcatenationOperation>(casted);
      concat->add_subexpression(make_shared<Symbol>(end));
      regex = concat;
    } else if (auto casted = dynamic_pointer_cast<PlusOperation>(regex)) {
      casted->add_to_every_subexpression(make_shared<Symbol>(end));
      regex = casted;
    } else if (auto casted = dynamic_pointer_cast<ConcatenationOperation>(regex)) {
      casted->add_subexpression(make_shared<Symbol>(end));
      regex = casted;
    } else if (auto casted = dynamic_pointer_cast<StarOperation>(regex)) {
      /*if (start == end) { casted->set_plus_one(true);}
      auto concat = make_shared<ConcatenationOperation>(casted);
      concat->add_subexpression(make_shared<Symbol>(end));
      regex = concat;#1#
    }
  }*/


  regex_in_cfg[make_tuple(start, end)] = regex;

  std::cout << "REGEX: " << *regex << std::endl;
  return true;
}

bool RegEx::get_CfgPath_base(CfgPath& cfg_path, std::shared_ptr<Operation>& regex, std::map<string, uint64_t> star_map)
{
  std::cout << "Finding PATH from " << *regex << std::endl;
  if (regex->isEmpty()) { return false; }
  if (auto symbol = dynamic_pointer_cast<Symbol>(regex)) {
    cfg_path.push_back(symbol->getNumber());
    std::cout << "RETURNED "  << std::endl;
    return true;
  }
  if (auto concat = dynamic_pointer_cast<ConcatenationOperation>(regex)) {
    bool result = true;
    for (auto operation : concat->getSubexpressions()) {
      std::cout << "REGEX in concat: " << *operation << std::endl;
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


    /*
    cpputil::BitVector num = solver.get_model_bv(star->get_loop_var(), 64);
    cpputil::BitVector rdi_r = solver.get_model_bv("%rdi_rewrite", 64);
    cpputil::BitVector rdi_t = solver.get_model_bv("%rdi_target", 64);
    cpputil::BitVector rax_r = solver.get_model_bv("%rax_rewrite", 64);
    cpputil::BitVector rax_t = solver.get_model_bv("%rax_target", 64);*/
    //uint64_t val = num.contents_[0];
    //std::cout << val << std::endl;


    //auto loop_count = star->is_plus_one() ? num.get_fixed_quad(0) + 1 : num.get_fixed_quad(0);
    //std::cout << star->is_plus_one() << std::endl;
    //std::cout << num.get_fixed_quad(0) << std::endl;

    /*
    for (int i = 1; i < 5000; i = i*2) {
      cpputil::BitVector tr = solver.get_model_bv(star->get_loop_var(), i);
      tr.resize_for_bits(64);
      for (int j = i; j > 0; j -= 64) {
        std::cout << "i: " << i << " value: " << tr.get_fixed_quad(j) << std::endl;
      }
    }
    */


    /*std::cout << "has model: " << solver.has_model() << std::endl;
    std::cout << "star variable name: " << star->get_loop_var() << std::endl;
    std::cout << "LOOP COUNT " << hex << num.get_fixed_quad(0) << std::endl;
    std::cout << "RDI R " << hex << rdi_r.get_fixed_quad(0) << std::endl;
    std::cout << "RDI T " << hex  << rdi_t.get_fixed_quad(0) << std::endl;
    std::cout << "RAX R " << hex << rax_r.get_fixed_quad(0) << std::endl;
    std::cout << "RAX T " << hex << rax_t.get_fixed_quad(0) << std::endl;*/
    //std::string input; std::cin >> input;
    if (star_map.find(star->get_loop_var()) == star_map.end()) {
      std::cout << "SOMETHING WENT WRONG" << std::endl;
    }
    std::cout << "Star variable from map: " << star_map[star->get_loop_var()] << std::endl;
    for (uint64_t i = 0; i < star_map[star->get_loop_var()]; i++) {
      cfg_path.insert(cfg_path.end(), temp.begin(), temp.end());
    }
    return !star->is_plus_one();
  }
  return false;
}

bool RegEx::get_CfgPath(CfgPath& cfg_path, std::shared_ptr<Operation>& regex, std::map<string, uint64_t> star_map) {
  std::cout << "REGEX in get_CfgPath: " << *regex << std::endl;

  get_CfgPath_base(cfg_path, regex, star_map);
  /*if (std::dynamic_pointer_cast<ConcatenationOperation>(regex)) {

    cfg_path.pop_back();
  }*/

  std::cout << "CFG PATH:   " << cfg_path << std::endl;

  return true;
}