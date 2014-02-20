/*
  Copyright 2013 ICT

  Developer : Jiyuan Lin (linjiyuan90@gmail.com)
  Date : 05/13/2013

  file : drawline.cpp
*/

/*
  push
  transform_rules
  build
    AC
  match
    lowlevel
      AC
      Regex
    highlevel


  ix: index of a rule in rules
  pix: father's ix
  id: rule id, one id may have several ix
  nodes: MatchNode * type
  level

  rules form a tree
  matchnodes form a tree as well
*/

#include "ctime"
#include "cassert"

#include "algorithm"
#include "iostream"
#include "iterator"
#include "map"
#include "string"
#include "stack"
#include "vector"

#include "boost/xpressive/xpressive.hpp"
#include "boost/thread/thread.hpp"

#include "./utility.h"
#include "./drawline.h"

namespace boost_xp = boost::xpressive;

namespace drawline {

MyLog mylog;  // share log for Rule, Drawline

/*
  Implement of Rule
*/
Rule::Rule(const std::wstring &_name, const std::wstring &_rule,
           RULE_TYPE _rule_type, int _ix, int _id, bool _is_input) {
  name = _name;
  rule = _rule;
  rule_type = _rule_type;
  ix = _ix;
  id = _id;
  is_input = _is_input;

  is_transformed = false;
  level = 0;
  dist_k = MAX_DIST;
  constraint_type = NONE;
  constraints = 0;
}

void Rule::split_rule(SymbolTable &rule_table) {
  words.clear();
  child.clear();

  if (rule_type == CONCEPT) {
    words.push_back(rule);
  }

  if (rule_type == REGEX) {
    words.push_back(rule);
  }

  if (rule_type == MCONCEPT) {
    /*
      MCONCEPT:name:MNAMExDIGITyTIMEz

      dfs to split the rule, each time find a longest substr of rule that is a
      existing rule name

      For example:
      rule: xMNAMEyDIGITzTIME
      other rule name: MN, NAME, MNAME, DIGIT, TIME
      Note MN, NAME, MNAME are substr of the rule, but I will choose the
	  longest one, MNAME

      After split:
        words:  [MNAME, x, DIGIT, y, TIME, z]
        child:  [(0, -1), (2, -1), (4, -1), (5, -1)]
        Since z is the last word, I'll create a CONCEPT for it for convinence
         -1 will be change later in transform
    */
    std::stack<PII> S;
    S.push(std::make_pair(0, rule.length() - 1));
    while (!S.empty()) {
      int st = S.top().first, ed = S.top().second;
      S.pop();
      std::wstring w = rule.substr(st, ed - st + 1);
      if (rule_table.count(w)) {
        words.push_back(w);
        child.push_back(std::make_pair(words.size() - 1, -1));
        continue;
      }
      bool split = false;
      for (int l = ed - st; !split && l > 0; l--) {
        for (int _st = st; _st + l - 1 <= ed; _st ++) {
          std::wstring w = rule.substr(_st, l);
          if (rule_table.count(w)) {
            if (_st + l <= ed) {
              S.push(std::make_pair(_st + l, ed));
            }
            S.push(std::make_pair(_st, _st + l - 1));
            if (_st - 1 >= st) {
              S.push(std::make_pair(st, _st - 1));
            }
            split = true;
            break;
          }
        }
      }
      if (!split) {
        words.push_back(w);
      }
    }
    if (child.empty() || child.back().first != words.size() - 1) {
      child.push_back(std::make_pair(words.size() - 1, -1));
    }
  }

  if (rule_type == MCONCEPT_RULE) {
    if (name != rule) {
      std::string srule = wstr2str(rule);
      while (!srule.empty() && isspace(srule[0])) {  // strim the rule
        srule = srule.substr(1);
        rule = rule.substr(1);
      }
      while (!srule.empty() && isspace(srule[srule.length() - 1])) {
        srule.resize(srule.length() - 1);
        rule.resize(rule.length() - 1);
      }
    }
    /*
      when print the result, remember to print the arguments

      NAME_COLLEGE(person, college):(SENT, "_person{NAME}",
	    "_college{ORGANIZATION}", (ORD, "DEGREE_GET_OR", "DEGREE"))

      After split:
        words:  [NAME, ORGANIZATION, (ORD, "DEGREE_GET_OR", "DEGREE")]
        child:  [(0, -1), (1, -1), (2, -1)]
            -1 will be change later in transform function
    */
    int num_pair = 0;
    std::wstring::size_type st = rule.find(L"("), ed, _ed;
    // assert(st != std::wstring::npos);
    if (st == std::wstring::npos) {
#ifdef VERBOSE
      std::cerr << "Can't find MCONCEPT_RULE " << wstr2str(name)
		<< ". Set it as CONCEPT." << std::endl;
#endif
      mylog.add("Can't find MCONCEPT_RULE", wstr2str(name));
      rule_type = CONCEPT;
      return;
    }
    // assert(rule[rule.length() - 1] == L')');
    if (rule[rule.length() - 1] != L')') {
#ifdef VERBOSE
      std::cerr << "Not a MCONCEPT_RULE " << wstr2str(rule) << std::endl;
#endif      
      mylog.add("Not a MCONCEPT_RULE", wstr2str(rule));
      rule_type = CONCEPT;
      return;
    }
    rule[rule.length() - 1] = L',';  // for convinence
    for (ed = ++st; ed < rule.length(); ed ++) {
      if (rule[ed] == L'(') {
        num_pair++;
      }
      if (rule[ed] == L')') {
        num_pair--;
      }
      if (num_pair == 0 && rule[ed] == L',') {
        std::wstring word;
        st = rule.find_first_not_of(L" \"", st);
        if (st > 0 && rule[st-1] == L'\"') {  // lowlevel_node
          /*
            "aa_person{NAME}bb_loc{LOCATION}cc" ==> aa_person{NAME}bb_loc{LOCATION}cc
          */
          _ed = ed;
          while (_ed > st && (rule[_ed-1] == L' ' || rule[_ed-1] == L'"')) {
            _ed--;
          }
          word = rule.substr(st, _ed - st);
          // this is a simple case, it has no arguments
          // such as: "BIRTH_OR"
          // The following code assume no such situation: "BIRTH_ORxxx"
          if (word.find(L"{") == std::wstring::npos) {
            words.push_back(word);
            child.push_back(std::make_pair(child.size(), -1)); 
          } else {
          /*
            aa_person{NAME}bb_loc{LOCATION}cc =>
            aa, NAME, bb, LOCATION, cc
          */
          while (!word.empty()) {
            if (word[0] == L'_') {
              std::wstring::size_type a = word.find(L"{");
              std::wstring::size_type b = word.find(L"}", a);
              // what if the name don't have the argument?
              if (a != std::wstring::npos &&
                  name.find(word.substr(1, a - 1)) != std::wstring::npos) {
                std::wstring arg = word.substr(1, a - 1);
                // record which child has this arg
                arg_child[arg].push_back(child.size());
                a++;
                words.push_back(word.substr(a, b - a));
                child.push_back(std::make_pair(child.size(), -1));
                if (b + 1 >= word.length()) {
                  break;
                }
                word = word.substr(b+1);
              } else if (a != std::wstring::npos) {
                a++;
                words.push_back(word.substr(a, b-a));
                child.push_back(std::make_pair(child.size(), -1));
                if (b < word.length()) {
                  word = word.substr(b+1);
                } else {
                  break;
                }
              } else {
                words.push_back(word);
                child.push_back(std::make_pair(child.size(), -1));
                break;
              }
            } else {
              std::wstring::size_type a = word.find(L"_");
              words.push_back(word.substr(0, a));
              child.push_back(std::make_pair(child.size(), -1));
              if (a == std::wstring::npos) {
                break;
              }
              word = word.substr(a);
            }
          }
          }
        } else {
          // SENT, (ORD, ...), ...
          word = rule.substr(st, ed - st);
          words.push_back(word);
          if (words.size() == 1) {  // operator name
            if (swscanf(word.c_str(), L"DIST_%d", &dist_k) != 1) {
              dist_k = MAX_DIST;
            }
          } else {
            child.push_back(std::make_pair(child.size(), -1));
          }
        }
        st = ed + 1;
      }
    }
    rule[rule.length() - 1] = L')';  // roll back
    if (words[0] == L"OR") {
      // this one don't have to push down, so no need to record it
      constraint_type = OR;
    }
    if (words[0] == L"AND") {
      constraints |= (constraint_type = AND);
    }
    if (words[0] == L"ORD") {
      constraints |= (constraint_type = ORD);
    }
    if (words[0] == L"SENT") {
      constraints |= (constraint_type = SENT);
    }
    if (words[0].find(L"DIST_") == 0) {
      constraints |= (constraint_type = DIST_K);
    }
    words.erase(words.begin());  // don't need it any more
  }
}

////////////////////////////////////////////////////////////////////////////////
/*
  Implement of Drawline
*/
Drawline::Drawline() {
  is_built = false;
  is_transformed = false;
  // explicitly add newline into the word table
  word_table[L"\n"];
  word_table[L"\r"];
}

/*
  Push a rule to the rules collections after parsing the rule.
*/
void Drawline::push(const std::string &srule) {
  // Assume rule has been trimed!
  std::wstring wrule;
  decode_utf8(srule, wrule);
  if (wrule.empty()) {
    return;
  }

  RULE_TYPE rule_type;

  if (wrule.find(WS_MCONCEPT_RULE) == 0) {
    rule_type = MCONCEPT_RULE;
  } else if (wrule.find(WS_MCONCEPT) == 0) {
    rule_type = MCONCEPT;
  } else if (wrule.find(WS_REGEX) == 0) {
    rule_type = REGEX;
  } else if (wrule.find(WS_CONCEPT) == 0) {
    rule_type = CONCEPT;
  } else {
#ifdef VERBOSE
    std::cerr << "Drawline::push Can't recognize " << srule << " Skip it."
              << std::endl;
#endif
    mylog.add("Drawline::push Can't recognize ", srule);
    return;
  }

  std::wstring::size_type st, ed;
  st = wrule.find(L":");
  ed = wrule.find(L":", st + 1);
  std::wstring name = wrule.substr(st + 1, ed - st - 1);
  std::wstring rule = wrule.substr(ed + 1);
  rules.push_back(Rule(name, rule, rule_type, rules.size(),
                                         // in case of MCONCEPT_RULE
                  rule_table[name.substr(0, name.find(L"("))], true));
  resize_push(id_ix, rule_table[name.substr(0, name.find(L"("))],
              rules.size() - 1);

  is_transformed = false;
}

/*
  Sort the rules according to their level, create temporal rules as needed,
  and link them to build a layerd graph.

  Able to transform newly added rules.
*/
void Drawline::transform_rules() {
  for (int ix = 0; ix < rules.size(); ix++) {
    if (!rules[ix].is_transformed) {
      transform(ix);
      if (rules[ix].rule_type == CONCEPT || rules[ix].rule_type == REGEX) {
        is_built = false;
      }
    }
  }

  // from top to buttom
  // push constraints to children
  // only if the child have only one parent
  for (int l = level_rules.size() - 2; l > 0; l --) {
    for (int i = 0; i < level_rules[l].size(); i++) {
      int ix = level_rules[l][i];
      int id = rules[ix].id;
      if (rules[ix].rule_type != MCONCEPT_RULE ||
        id >= id_pix.size() ||
        id_pix[id].size() != 1) {
        continue;
      }
      int pix = id_pix[id][0];
      rules[ix].constraints |= rules[pix].constraints;
      rules[ix].dist_k = std::min(rules[ix].dist_k, rules[pix].dist_k);
    }
  }

  is_transformed = true;
}

/*
  A temporal rule's name is the same as its content
*/
void Drawline::transform(int ix) {
  /*
    Note!!!
    Recording pointer/iterator/reference is danger, since they may become
    invalid if the container changes its size.
  */

  rules[ix].is_transformed = true;  // don't use Rule &rule = rules[ix];
  int max_level = 0;
  rules[ix].split_rule(rule_table);
  for (int i = 0; i < rules[ix].child.size(); i++) {
    std::wstring word = rules[ix].words[rules[ix].child[i].first];
    /*
      Create temporal rules.
      And this new rule can only be MCONCEPT_RULE, right?
    */
    if (!rule_table.count(word)) {
      // what about other rule_type?
      // no this situation
      rules.push_back(Rule(word, word, MCONCEPT_RULE, rules.size(),
                           rule_table[word], false));
      resize_push(id_ix, rule_table[word], rules.size() - 1);
    }
    /*
      child id, not child ixes!
       there maybe several rules that have this ch_id
     */
    int ch_id = (rules[ix].child[i].second = rule_table[word]);
    for (int j = 0, ch_ix; j < id_ix[ch_id].size(); j++) {
      ch_ix = id_ix[ch_id][j];
      if (!rules[ch_ix].is_transformed) {
        transform(ch_ix);
      }
      max_level = std::max(max_level, rules[ch_ix].level + 1);
    }

    /*
      ch_id -- parent_ix
      for each child, record its parents
    */
    // if ORD/MCONCEPT, only record the last child/word
    if (rules[ix].rule_type == MCONCEPT || rules[ix].constraint_type == ORD) {
      if (i == rules[ix].child.size() - 1) {
        resize_push(id_pix, ch_id, ix);
      }
    } else {
      resize_push(id_pix, ch_id, ix);
    }
  }

  rules[ix].level = max_level;
  resize_push(level_rules, rules[ix].level, ix);
}

/*
  Build AC automaton for low level rules (user inputed or temporally generated)
*/
void Drawline::build() {
#ifdef DRAWLINE_DEBUG
  std::cerr << "Drawline: building ... " << std::endl;
#endif

  /*
    level 0
    CONCEPT/REGEX
  */
  if (!level_rules.empty()) {
    for (VI::iterator it = level_rules[0].begin(); it != level_rules[0].end();
         it ++) {
      Rule &rule = rules[*it];
      if (rule.rule_type == CONCEPT) {
        ac.insert(rule.ix, word_table, rule.rule);  // ix, not id
      }
      if (rule.rule_type == REGEX) {
        regex_rules.push_back(*it);
      }
    }

    ac.build();
  }

  is_built =  true;
#ifdef DRAWLINE_DEBUG
  std::cerr << "Drawline: build finished " << std::endl;
#endif
}

void Drawline::lowlevel_match(const std::wstring &text) {
  long long start = time(NULL);
  // AC automaton match
  long long _start = time(NULL);
  VM match_nodes = ac.match(word_table, text, sent_pos, word_pos);
  for (int i = 0; i < match_nodes.size(); i++) {
    MatchNode* pt = match_nodes[i];
    pt->rule_id = rules[pt->rule_ix].id;
    pt->st = pt->ed - rules[pt->rule_ix].rule.length() + 1;
    if (is_in_pos(pt->st, word_pos) && is_in_pos(pt->ed + 1, word_pos)) {
      lowlevel_nodes.push_back(pt);
    } else {
      delete pt;
    }
  }
  // std::cerr << "AC match cost: " << (time(NULL)-_start) << "s" << std::endl;

  // REGEX macth
  /*
    The following method need to scan text several times.
    It can be improved by combining all the regex, for example:
    (?P<1>...)|(?P<2>...)|...
    But this method may loss some strings if they are overlapped
    with the already extracted strings.
    So I still use the naive approach.
  */

  _start = time(NULL);
  for (VI::iterator it = regex_rules.begin(); it != regex_rules.end(); it++) {
    Rule &rule = rules[*it];
    boost_xp::wsregex re = boost_xp::wsregex::compile(rule.rule);
    boost_xp::wsregex_iterator cur(text.begin(), text.end(), re);
    boost_xp::wsregex_iterator end;
    for (; cur != end ; cur++) {
      const boost::xpressive::wsmatch &what = *cur;
      if (what[0].length() == 0) {
        continue;
      }
      int st = what.position(), ed = st + what[0].length() - 1;
      if (is_in_pos(st, word_pos) && is_in_pos(ed+1, word_pos)) {
        MatchNode *pt = new MatchNode(*it, rule.id, get_sent_id(ed), st, ed);
        lowlevel_nodes.push_back(pt);
      }
    }
  }
  // std::cerr << "Regex match cost: " << (time(NULL) - _start)
  //	    << "s" << std::endl;

  sort(lowlevel_nodes.begin(), lowlevel_nodes.end(), MATCHNODE_ED_CMP);

  // std::cerr << "Drawline: lowlevel_match finished. Time eclapsed: "
  //     << (time(NULL) - start) << "s" << std::endl;
}

void Drawline::highlevel_match(const std::wstring &text) {
  /*
    Scan from left to right of the level_0 nodes.
    Try to build new node that has currently scanned node, and pass the new node
    to higher level and go on building new nodes.

    For example,
    rule is:
      (SENT, ORD(A, B), DIST_K(C, D), E, F)
    the sequence is:
      A, B, F, C, D, E
    now E is scanned, retrieve ORD(A, B), DIST_K(C, D), F, E to build (SENT, ... )
    Note, ORD(A, B), DIST_K(C, D), F have been constructed.

    The NOT logic will be implemented as a post processing.
  */
  long long start = time(NULL);
  // lowlevel_nodes may have duplicate nodes
  // for (int i = 0; i < lowlevel_nodes.size() && i < 500; i++) {
  for (int i = 0; i < lowlevel_nodes.size(); i++) {
    /*
    // for those who want to invoke pthread_cancel
    // if drawline taking too long
    pthread_testcancel();
    */
    try {
      boost::this_thread::interruption_point();
    } catch(boost::thread_interrupted) {
      // need to remove remaining lowlevel_nodes
      // since it can not be sensed from the destructor
      for (; i < lowlevel_nodes.size(); i++) {
	delete lowlevel_nodes[i];
      }
      throw boost::thread_interrupted();
    }
    MatchNode *pt = lowlevel_nodes[i];
    if (!check_duplicate(pt->rule_id, pt->st, pt->ed)) {
      resize_push(id_nodes, pt->rule_id, pt, false);
      up_construct(text, pt);
    } else {
      delete lowlevel_nodes[i];  // noted!!!!
    }
  }

  // std::cerr << "Drawline: highlevel_match finished. Time eclapsed: "
  //          << (time(NULL) - start) << "s" << std::endl;
}

/*
  It can construct several nodes!
  Not only one.
*/
VM Drawline::left_construct(const Rule &rule, int i, int j, int st, int ed,
                            const std::wstring &text) {
#ifdef DRAWLINE_DEBUG
  std::cerr << "construct: " << rule.ix << " ";
  std::cerr << st << " " << ed << " i:" << i << std::endl;
#endif

  VM nodes;
  if (i == -1) {  // ok, new node
    if (!check_duplicate(rule.id, st, ed)) {
      int a = get_sent_id(st);
      int b = get_sent_id(ed);
      nodes.push_back(new MatchNode(rule.ix, rule.id, a == b ? a : -1, st, ed));
      resize_push(id_nodes, rule.id, nodes.back(), true);
    }
    return nodes;
  }
  if (rule.child[j].first == i) {
    int ch_id = rule.child[j].second;
    if (ch_id < id_nodes.size() && !id_nodes[ch_id].empty()) {
      for (int k = id_nodes[ch_id].size() - 1; k >= 0; k--) {
        MatchNode *pt = id_nodes[ch_id][k];
        if (pt->ed == st - 1) {
          VM _nodes = left_construct(rule, i-1, j-1, pt->st, ed, text);
          nodes.insert(nodes.end(), _nodes.begin(), _nodes.end());
          // copy(_nodes.begin(), _nodes.end(), back_inserter(nodes));
        }
        if (pt->ed < st - 1) {
          break;
        }
      }
    }
  } else if (st >= rule.words[i].length() &&
      text.substr(st - rule.words[i].length(), rule.words[i].length()) ==
      rule.words[i]) {  // word matched
    VM _nodes = left_construct(rule, i-1, j, st - rule.words[i].length(),
                               ed, text);
    nodes.insert(nodes.end(), _nodes.begin(), _nodes.end());
  }
  return nodes;
}

// SENT has been implied from all's combination approximatly!
// should consider DIST_K
VM Drawline::left_construct(const Rule &rule, int st, int ed, const VM &all,
                            int now, VII child_ids, VM child_nodes) {
#ifdef DRAWLINE_DEBUG
  std::cerr << "rule construct: " << wstr2str(rule.name) << " " << st << " "
            << ed << " " << all.size() << " " << now << " " << child_ids.size()
            << " " << rule.constraints << " " << rule.constraint_type
            << std::endl;
#endif

  assert(st <= ed);
  if ((rule.constraints & SENT) &&
       st <= ed && get_sent_id(st) != get_sent_id(ed)) {
    return VM();
  }

  // check order
  if (rule.constraints & ORD) {
    // need to check whether the child_nodes satisfy ORD
    // for example,
    // (ORD, OR(A, B, C, D))
    // now is constructing OR(A, B, C, D)
    // and we have construct (C, B, D), this is illegal
    MatchNode *last = NULL;
    for (int i = 0; i < child_nodes.size(); i++) {
      MatchNode *now = child_nodes[i];
      if (last != NULL && now != NULL && last->ed > now->ed) {
        return VM();
      }
      if (now != NULL) {
        last = now;
      }
    }
  }

  SI have;
  if (!child_ids.empty()) {
    // order
    if (rule.constraints & ORD) {
      have.insert(child_ids.back().second);
    } else {
      for (int i = 0; i < child_ids.size(); i++) {
        have.insert(child_ids[i].second);
      }
    }
  }

  while (now >= 0 && all[now]->st >= st) {
    now--;
  }

  VM nodes;
  if (child_ids.empty() || rule.constraint_type == OR) {  // ok, new nodes
    if (!check_duplicate(rule.id, st, ed)) {
#ifdef DRAWLINE_DEBUG
      std::cerr << "new node st:" << st << " ed:" << ed << " "
                << rule.level << std::endl;
#endif
      int a = get_sent_id(st);
      int b = get_sent_id(ed);
      nodes.push_back(new MatchNode(rule.ix, rule.id, a == b ? a : -1,
                                    st, ed, child_nodes));
      resize_push(id_nodes, rule.id, nodes.back(), true);
    }
  }
  if (now == -1 || child_ids.empty()) {  // can't go deeper
    return nodes;
  }

  // damn OR
  if (rule.constraint_type == OR) {
    if (get_word_id(st) - get_word_id(all[now]->ed) - 1 <= rule.dist_k) {
      if (have.count(all[now]->rule_id)) {
        int x = erase_last(child_ids, all[now]->rule_id);
        if (x >= 0 && x < child_nodes.size()) {
          child_nodes[x] = all[now];
        }
        // child_nodes[erase_last(child_ids, all[now]->rule_id)] = all[now];
      }
      // assert(all[now]->st < st);
      VM _nodes = left_construct(rule, std::min(all[now]->st, st), ed,
                                 all, now-1, child_ids, child_nodes);
      nodes.insert(nodes.end(), _nodes.begin(), _nodes.end());
    }
    return nodes;
  }

  // implicit AND
  while (now >= 0 && have.count(all[now]->rule_id) == 0) {
    // std::cerr << all[now]->rule_id << " ";
    now--;
  }
  // std::cerr << " now" << now << std::endl;
  if (now >= 0 &&
      get_word_id(st) - get_word_id(all[now]->ed) - 1 <= rule.dist_k) {
    // skip all[now];
    VM _nodes = left_construct(rule, std::min(all[now]->st, st), ed, all, now-1,
                               child_ids, child_nodes);
    nodes.insert(nodes.end(), _nodes.begin(), _nodes.end());

    // choose all[now]
    int x = erase_last(child_ids, all[now]->rule_id);
    if (x >= 0 && x < child_nodes.size()) {
      child_nodes[x] = all[now];
    }
    // child_nodes[erase_last(child_ids, all[now]->rule_id)] = all[now];
    // assert(all[now]->st < st);
    _nodes = left_construct(rule, std::min(all[now]->st, st), ed, all, now-1,
                            child_ids, child_nodes);
    nodes.insert(nodes.end(), _nodes.begin(), _nodes.end());
  }
  return nodes;
}


/*
  what if the nodes are too much, and stack overflow?
  I assume no such situation.
*/
void Drawline::up_construct(const std::wstring &text, MatchNode *pt) {
  int ch_id = pt->rule_id;
  int ch_ix = pt->rule_ix;
#ifdef DRAWLINE_DEBUG
  std::cerr << "now " << ch_ix << " " << ch_id << std::endl;
  std::cerr << "now " << wstr2str(rules[ch_ix].name) << " "
            << wstr2str(rules[ch_ix].rule) << " " << pt->st << " " << pt->ed
            << " sent_id" << get_sent_id(pt->st) << " " << pt->sent_id
            << std::endl;
#endif

  if (ch_id >= id_pix.size()) {
    return;
  }
  for (VI::iterator it = id_pix[ch_id].begin(); it != id_pix[ch_id].end();
       it++) {
    int pix = *it;
    Rule &rule = rules[pix];

    // make sure pix really has this child
    int is_my_child = 0;
    for (int i = 0; i < rule.child.size(); i++) {
      if (rule.child[i].second == ch_id) {
        is_my_child = 1;
      }
    }
    if (!is_my_child) {
      continue;
    }

    VM nodes;
#ifdef DRAWLINE_DEBUG
    std::cerr << pix << "--->" << rule.rule_type << std::endl;
#endif
    if (rule.rule_type == MCONCEPT) {  // continous
      // aAbBC
      nodes = left_construct(rule, rule.words.size() - 2, rule.child.size() - 2,
			     pt->st, pt->ed, text);
    }

    if (rule.rule_type == MCONCEPT_RULE) {
      /*
        Note this
          (ORD, (OR, A, B, C)) where only A appears

        ORD will be check in left_construct
       */
      if ((rule.constraint_type & ORD) &&
        rule.child.back().second != pt->rule_id ||
        (rule.constraints & SENT) &&
        pt->sent_id == -1) {
        continue;
      }

      // combine the needed nodes
      int st = pt->st, ed = pt->ed;
      // this is approximatly, need to check again in left_construct
      int sent_id = (rule.constraints & SENT) ? pt->sent_id: -1;
      int dist_k = rule.dist_k;
      VII child_ids = rule.child;
      VM child_nodes(child_ids.size(), NULL);  // record children match nodes
      VM all;
      SI iset;
      for (VII::iterator ct = rule.child.begin(); ct != rule.child.end();
           ct++) {
        int cid = ct->second;
        if (cid >= id_nodes.size() || id_nodes[cid].empty() ||
          !iset.insert(cid).second) {
          continue;
        }
#ifdef DRAWLINE_DEBUG
        std::cerr << "id" << cid << " child size " << id_nodes[cid].size()
                  << std::endl;
#endif
        for (int k = id_nodes[cid].size() - 1; k >= 0; k--) {
          MatchNode *_pt = id_nodes[cid][k];
#ifdef DRAWLINE_DEBUG
          std::cerr << "pt ix" << _pt->rule_ix << " sentid" << _pt->sent_id
		    << " st" << _pt->st << " ed" << _pt->ed << " "
		    << get_word_id(_pt->st) << " " << get_word_id(_pt->ed)
		    << " " << get_word_id(st) << " " << dist_k 
		    << std::endl;
#endif
          if (_pt == pt) {
            continue;
          }
          if (sent_id != -1 && sent_id != _pt->sent_id) {
            break;
          }
          // if (get_word_id(min_st) - get_word_id(_pt->ed) - 1 > dist_k) {
          if (get_word_id(st) - get_word_id(_pt->ed) - 1 > dist_k) {
            // break;
          }
          all.push_back(_pt);
        }
      }
      sort(all.begin(), all.end(), MATCHNODE_ED_CMP);
#ifdef DRAWLINE_DEBUG
      std::cerr << "all size:" << all.size() << std::endl;
      for (VM::iterator jt = all.begin(); jt != all.end(); jt++) {
        std::cerr << wstr2str(rules[(*jt)->rule_ix].name) << " st" << (*jt)->st
                  << " ed" << (*jt)->ed << " id " << (*jt)->rule_id << " sentid"
                  << (*jt)->sent_id << std::endl;
      }
#endif

      child_nodes[erase_last(child_ids, pt->rule_id)] = pt;
      nodes = left_construct(rule, st, ed, all, all.size() - 1, child_ids,
                             child_nodes);
    }

    for (VM::iterator ct = nodes.begin(); ct != nodes.end(); ct++) {
      up_construct(text, *ct);
    }
  }
}

VS Drawline::match(const std::string &stext) {
  mylog.clear();
  if (!is_transformed) {
    transform_rules();
    is_transformed = true;
  }

  if (!is_built) {
    build();
    is_built =  true;
  }

  std::wstring text;
  decode_utf8(stext, text);
  if (text.empty()) {
    return VS();
  }

  long long start = time(NULL);
  lowlevel_match(text);

  highlevel_match(text);
  // std::cerr << "Match time: " << (time(NULL) - start) << "s" << std::endl;
  // mylog.print(std::cerr);
  return get_result(text);
}


void Drawline::print(const std::wstring &text) {
  VI mark(id_nodes.size());
  for (int l = 0; l < level_rules.size(); l++) {
    for (int i = 0; i < level_rules[l].size(); i++) {
      int id = rules[level_rules[l][i]].id;
      Rule &rule = rules[level_rules[l][i]];
      if (id >= id_nodes.size() ||
          mark[id] ||
          !rule.is_input) {
        continue;
      }
      mark[id] = 1;
      for (VM::iterator it = id_nodes[id].begin(); it != id_nodes[id].end();
           it ++) {
        MatchNode *pt = *it;
        std::cerr << "name:" << wstr2str(rules[pt->rule_ix].name) << " rule:"
             << wstr2str(rules[pt->rule_ix].rule) << " st:" << pt->st << " ed:"
             << pt->ed << " rule_id:" << pt->rule_id << " rule_ix:"
             << pt->rule_ix << " sent_id:" << pt->sent_id << " \""
             << wstr2str(text.substr(pt->st, pt->ed - pt->st + 1)) << "\""
             << std::endl;
      }
    }
  }
}


/*

  34440 21
  action:建立
  person:薛建军
  -----
  *****
  NAME:吕尧臣
  NAME:徐汉棠
*/
#ifndef DRAWLINE_BEAUTY_OUTPUT
VS Drawline::get_result(const std::wstring &text) {
  std::string stext = wstr2str(text);

  VM facts;
  VM concepts;

  VI mark(id_nodes.size());
  for (int l = 0; l < level_rules.size(); l++) {
    for (int i = 0; i < level_rules[l].size(); i++) {
      int id = rules[level_rules[l][i]].id;
      Rule &rule = rules[level_rules[l][i]];
      if (id >= id_nodes.size() ||
        mark[id] ||
        !rule.is_input) {
        continue;
      }
      mark[id] = 1;
      for (VM::iterator it = id_nodes[id].begin(); it != id_nodes[id].end();
           it ++) {
        MatchNode *pt = *it;
        if (rules[pt->rule_ix].rule_type == MCONCEPT_RULE) {
          facts.push_back(pt);
        } else {
          concepts.push_back(pt);
        }
      }
    }
  }

  sort(facts.begin(), facts.end(), MATCHNODE_ED_CMP);
  sort(concepts.begin(), concepts.end(), MATCHNODE_ED_CMP);

  VS vs;
  std::map<std::string, PII> mp;  // to delete duplicated output
  int tot = 0;

  // find from end since facts are sorted by ed
  // note rfind only search characters before pos (excluding pos)
  for (int i = facts.size() - 1, cur_ix = stext.length(); i >= 0; i--) {
    MatchNode *pt = facts[i];
    std::string match_sent = wstr2str(text.substr(pt->st, pt->ed - pt->st + 1));
    if (match_sent == "") {
      continue;
    }
    // std::cerr << cur_ix << std::endl;
    int _cur_ix = stext.rfind(match_sent, cur_ix);
    // I don't know why it will be -1
    if (_cur_ix < 0) {
      continue;
    }
    cur_ix = _cur_ix;
    assert(cur_ix >= 0);
    std::string offset = int2str(cur_ix);
    std::string len = int2str(match_sent.length());
    std::string name = wstr2str(rules[pt->rule_ix].name);
    name = name.substr(0, name.find("("));
    std::string res = retrieve_args(pt, text);
    deal_special_case(name, res);
    if (res.empty() || mp.count(name+res) && mp[name+res].first == pt->st &&
        mp[name+res].second + 5 > pt->ed) {
      // I treat these two as same
      // [st, ed]
      // [st, ed, ... ]
      //        |<-k->|
      // k is a small dist, for example 4
      continue;
    }
    mp[name+res] = std::make_pair(pt->st, pt->ed);
    vs.push_back(
          offset + " " + len + "\n" +
          res +
          std::string("-----\n"));
    cur_ix += match_sent.length();  // rewind back for next rfind
  }
  // since I use rfind (for convenience), here need to reverse back
  // however, it seems that the required result is descending, so no need
  // to reverse
  // reverse(vs.begin(), vs.end());

  vs.push_back(std::string("*****\n"));
  for (int i = 0; i < concepts.size(); i++) {
    MatchNode *pt = concepts[i];
    std::string word_st = int2str(get_word_id(pt->st));
    std::string word_ed = int2str(get_word_id(pt->ed) + 1);
    std::string st = int2str(pt->st);
    std::string ed = int2str(pt->ed);
    std::string name = wstr2str(rules[pt->rule_ix].name);
    if (!OUTPUT_CONCEPT.count(name)) {
      continue;
    }
    vs.push_back(name + ":" +
                 wstr2str(text.substr(pt->st, pt->ed - pt->st + 1)) + "\n");
  }
  return vs;
}
#endif

/*
  1 facts:

  -----------------
  FACT 0: [1580(1606)-1593(1618)] /NAME_NAME_COEXIST/: 周志良】【周志和】【周国芳
  3 ARGS:
  ARG 0 [person] : 周志良
  ARG 1 [person] : 周国芳
  ARG 2 [coexist] : 和

  -----------------

  1214 concepts:
  CONCEPT 0: [1(1)-3(2)] /LOCATION/: 宜兴
  CONCEPT 1: [21(22)-23(23)] /ACTION/: 欢迎

  TODO:
  Change the output from word_id to byte-offset, byte-len
*/
#ifdef DRAWLINE_BEAUTY_OUTPUT
VS Drawline::get_result(const std::wstring &text) {

  VM facts;
  VM concepts;

  VI mark(id_nodes.size());
  for (int l = 0; l < level_rules.size(); l++) {
    for (int i = 0; i < level_rules[l].size(); i++) {
      int id = rules[level_rules[l][i]].id;
      Rule &rule = rules[level_rules[l][i]];
      if (id >= id_nodes.size() ||
        mark[id] ||
        !rule.is_input) {
        continue;
      }
      mark[id] = 1;
      for (VM::iterator it = id_nodes[id].begin(); it != id_nodes[id].end();
           it ++) {
        MatchNode *pt = *it;
        if (rules[pt->rule_ix].rule_type == MCONCEPT_RULE) {
          facts.push_back(pt);
        } else {
          concepts.push_back(pt);
        }
      }
    }
  }

  sort(facts.begin(), facts.end(), MATCHNODE_ED_CMP);
  sort(concepts.begin(), concepts.end(), MATCHNODE_ED_CMP);

  VS vs;
  std::map<std::string, PII> mp;  // to delete duplicated output
  int tot = 0;

  for (int i = 0; i < facts.size(); i++) {
    MatchNode *pt = facts[i];
    std::string word_st = int2str(get_word_id(pt->st));
    std::string word_ed = int2str(get_word_id(pt->ed) + 1);
    std::string st = int2str(pt->st);
    std::string ed = int2str(pt->ed);
    std::string name = wstr2str(rules[pt->rule_ix].name);
    name = name.substr(0, name.find("("));
    std::string res = retrieve_args(pt, text);
    if (res.empty() || mp.count(name+res) &&
        mp[name+res].first == pt->st && mp[name+res].second + 5 > pt->ed) {
      // I treat these two as same
      // [st, ed]
      // [st, ed, ... ]
      //        |<-k->|
      // k is a small dist, for example 4
      continue;
    }
    mp[name+res] = std::make_pair(pt->st, pt->ed);
    vs.push_back(std::string("\n-----------------\n") +
          "FACT " + int2str(tot++) + ": [" +
          word_st + "(" + st + ")-" +
          word_ed + "(" + ed +")] /" +
          name + "/: " +
          wstr2str(text.substr(pt->st, pt->ed - pt->st + 1)) + "\n" +
          res +
          "\n-----------------\n");
  }
  vs.insert(vs.begin(), int2str(tot) + " facts:\n");

  vs.push_back(int2str(concepts.size()) + " concepts:\n");
  for (int i = 0; i < concepts.size(); i++) {
    MatchNode *pt = concepts[i];
    std::string word_st = int2str(get_word_id(pt->st));
    std::string word_ed = int2str(get_word_id(pt->ed) + 1);
    std::string st = int2str(pt->st);
    std::string ed = int2str(pt->ed);
    std::string name = wstr2str(rules[pt->rule_ix].name);
    vs.push_back(std::string("CONCEPT ") + int2str(i) + ": [" +
          word_st + "(" + st + ")-" +
          word_ed + "(" + ed + ")] /" + name + "/: " +
          wstr2str(text.substr(pt->st, pt->ed - pt->st + 1)) + "\n");
  }
  return vs;
}
#endif

/*
  Print the transformed rules to check whether it's ok or not
*/
void Drawline::print_rules() {
  if (!is_transformed) {
    transform_rules();
  }

  for (int ix = 0; ix < rules.size(); ix++) {
    std::cerr << rules[ix].id << " " << rules[ix].rule_type << " "
              << wstr2str(rules[ix].name) <<" " << wstr2str(rules[ix].rule)
              << " " << rules[ix].level << " " << rules[ix].dist_k << std::endl;
    std::cerr << "words: ";
    for (VW::iterator jt = rules[ix].words.begin(); jt != rules[ix].words.end();
         jt++) {
      std::cerr << wstr2str(*jt) << " ";
    }
    std::cerr << "\nchild: ";
    for (VII::iterator jt = rules[ix].child.begin();
         jt != rules[ix].child.end(); jt++) {
      std::cerr << jt->first << "," << jt->second << " ";
    }
    std::cerr << std::endl << std::endl;
  }
}

Drawline::~Drawline() {
  for (int id = 0; id < id_nodes.size(); id++) {
    std::vector<MatchNode *> &vm = id_nodes[id];
    for (int i = 0; i < vm.size(); i++) {
      delete vm[i];
    }
  }
  // std::cout << "~Drawline() " << std::endl;
}

int Drawline::dist_match_node(MatchNode *a, MatchNode *b) {
  if (*b < *a) {
    std::swap(a, b);
  }
  return std::max(b->st - a->ed - 1, 0);
}

int Drawline::get_sent_id(int x) {
  return lower_bound(sent_pos.begin(), sent_pos.end(), x) - sent_pos.begin();
}

/*
  Check whether those have same id with ix have (st, ed)
*/
bool Drawline::check_duplicate(int id, int st, int ed) {
  if (id >= id_nodes.size() || id_nodes[id].empty()) {
    return false;
  }
  for (VM::iterator it = id_nodes[id].end() - 1; it >= id_nodes[id].begin();
       it--) {
    if ((*it)->ed == ed && (*it)->st == st) {
      return true;
    }
    if ((*it)->ed < ed || (*it)->st < st) {
      break;
    }
  }
  return false;
}

int Drawline::erase_last(VII &vt, int x) {
  for (int i = vt.size() - 1; i >= 0; i--) {
    if (vt[i].second == x) {
      x = vt[i].first;
      vt.erase(vt.begin() + i);
      return x;
    }
  }
  return -1;
}

void Drawline::resize_push(std::vector<VM> &vt, int x, MatchNode *y, bool order) {
  if (x + 1 > vt.size()) {
    vt.resize(x + 1);
  }
  if (!order) {
    vt[x].push_back(y);
  } else {
    // duplication should had been checked before calling this function
    VM::iterator it = vt[x].end();
    while (it != vt[x].begin()) {
      it--;
      if ((*it)->ed < y->ed || (*it)->st <= y->st) {
        it++;
        break;
      }
    }
    vt[x].insert(it, y);
  }
}

void Drawline::resize_push(std::vector<VI> &vt, int x, int y) {
  if (x + 1 > vt.size()) {
    vt.resize(x + 1);
  }
  vt[x].push_back(y);
}

int Drawline::get_word_id(int x) {
  return upper_bound(word_pos.begin(), word_pos.end(), x) -
         word_pos.begin() - 1;
}

void Drawline::retrieve_args_nodes(MatchNode *pt,
                                   std::map<std::wstring, VM> &args_nodes) {
  Rule &rule = rules[pt->rule_ix];
  std::map<std::wstring, VI> &arg_child = rule.arg_child;
  for (std::map<std::wstring, VI>::iterator it = arg_child.begin();
       it != arg_child.end(); it++) {
    VI &vt = it->second;
    for (int i = 0; i < vt.size(); i++) {
      if (pt->child_nodes[vt[i]] != NULL) {
        MatchNode *cpt = pt->child_nodes[vt[i]];
        if (rules[cpt->rule_ix].rule_type != MCONCEPT_RULE) {
          args_nodes[it->first].push_back(cpt);
        }
      }
    }
  }

  for (VM::iterator it = pt->child_nodes.begin(); it != pt->child_nodes.end();
       it++) {
    if (*it != NULL) {
      retrieve_args_nodes(*it, args_nodes);
    }
  }
}

// I assume every MCONCEPT_RULE has its args
std::string Drawline::retrieve_args(MatchNode *pt, const std::wstring &text) {
  // retrieve all arguments' nodes
  std::map<std::wstring, VM> args_nodes;
  retrieve_args_nodes(pt, args_nodes);
  for (std::map<std::wstring, VM>::iterator it = args_nodes.begin();
       it != args_nodes.end(); it++) {
    sort(it->second.begin(), it->second.end(), MATCHNODE_ED_CMP);
  }

  Rule &rule = rules[pt->rule_ix];
  std::wstring args_str = rule.name;
  std::wstring::size_type a = args_str.find(L"("), b = args_str.find(L")");
  args_str = args_str.substr(a+1, b - a - 1);
  VW args = split(args_str, L", ");
  std::string ans;
  int tot = 0, entity_len = 0;
  for (int i = 0; i < args.size(); i++) {
    std::string res;
    MatchNode *qt;
    if (!args_nodes[args[i]].empty()) {
      VM::iterator it = args_nodes[args[i]].begin();
      qt = *it;
      res = wstr2str(text.substr(qt->st, qt->ed - qt->st + 1));
      args_nodes[args[i]].erase(it);
    }
    if (!res.empty()) {
#ifdef DRAWLINE_BEAUTY_OUTPUT
      ans += "ARG " + int2str(tot++) + " [" + wstr2str(args[i]) + "] : ";
#else
      ans += wstr2str(args[i]) + ":";
      tot++;
#endif
      ans += res + "\n";
      entity_len += qt->ed - qt->st + 1;
    }
  }
  // remove situation when the entities overlap
  if (pt->ed - pt->st + 1 < entity_len) {
    return "";
  }
#ifdef DRAWLINE_BEAUTY_OUTPUT
  return tot < args.size() ? "" : int2str(tot) + " ARGS:\n" + ans;
#else
  return tot < args.size() ? "" : ans;
#endif
}

void Drawline::deal_special_case(const std::string &name, std::string &res) {
  // NAME_NAME_COEXIS(person,person)
  // output:
  // person:
  // coexist: <--add this
  // persion:
  if (name == "NAME_NAME_COEXIST" &&
      res.find("coexist:") == std::string::npos) {
    std::string::size_type pos = res.find("\n");
    // if name is english, use 'and'
    // otherwise, use '同'
    // this is a simple rule of checking whether a name is english
    if (isalpha(res[pos-1])) {
        res = res.substr(0, pos+1) + wstr2str(L"coexist:and\n") + res.substr(pos+1);
    } else {
        res = res.substr(0, pos+1) + wstr2str(L"coexist:同\n") + res.substr(pos+1);
    }
  }
}


bool Drawline::is_in_pos(int x, const VI &pos) {
  VI::const_iterator it = lower_bound(pos.begin(), pos.end(), x);
  return it == pos.end() || *it == x;
}
}
