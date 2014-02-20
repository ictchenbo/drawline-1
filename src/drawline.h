/*
  Copyright 2013 ICT  

  Developer : Jiyuan Lin (linjiyuan90@gmail.com)
  Date : 05/13/2013

  file : drawline.h
*/

#ifndef SRC_DRAWLINE_H_
#define SRC_DRAWLINE_H_

/*
Supported Rules:

  CONCEPT:concept_name:sequence_of_words
    Use AC automaton to extract those words  
  REGEX:UPSTR:[A-Z]{2,}
    Directly use regex library to extract  
  MCONCEPT:mconcept_name:aAbB CcDd
    Get the matched nodes from CONCEPT/REGEX, and use them to construct it.
    Recurrsively combines the answsers.
  MCONCEPT_RULE:name:(SENT, AND(A, B), DIST_K(C, D))
    Operations can be AND, OR, ORD, SENT, DIST_k, NO is not supported yet.
*/

#include "vector"
#include "map"
#include "string"
#include "set"
#include "utility"
#include "algorithm"

#include "./drawline_ds.h"
#include "./mylog.h"

const int MAX_DIST = 12306;

const std::wstring WS_CONCEPT = L"CONCEPT";
const std::wstring WS_REGEX = L"REGEX";
const std::wstring WS_MCONCEPT = L"MCONCEPT";
const std::wstring WS_MCONCEPT_RULE = L"MCONCEPT_RULE";

enum RULE_TYPE {CONCEPT, REGEX, MCONCEPT, MCONCEPT_RULE};
enum CONSTRAINT_TYPE {NONE = 0, OR = 1, AND = 2, ORD = 4, SENT = 8,
                      DIST_K = 16};

typedef std::pair<int, int> PII;
typedef std::vector<std::wstring> VW;
typedef std::vector<std::string> VS;
typedef std::vector<PII> VII;
typedef std::vector<CONSTRAINT_TYPE> VC;
typedef std::set<std::string> SS;

namespace drawline {

const std::set<std::string> OUTPUT_CONCEPT = {"NAME", "LOCATION", "ORGANIZATION", "DATE"};
  
extern MyLog mylog;  // share log for Rule, Drawline

/* Rule */
class Rule {
 public:
  RULE_TYPE rule_type;
  std::wstring rule;
  std::wstring name;

  VW words;
  VII child;  // (which_word, rule_id)

  bool is_input;  // is the rule inputed or temporally genereated
  bool is_transformed;

  // name <-> id
  // name map to id, and vice versa
  // But a name/id may have several rules
  int id;
  int ix;  // record its position in drawline rules
  int level;

  // for MCONCEPT_RULE
  int dist_k;
  std::map<std::wstring, VI> arg_child;
  // Not only record its constraint, but also its ancestors'.
  // Of course, only if the ancestors have only one parents.
  // By adding ancestors' constraints, can prune a lot combinations.
  int constraints;
  CONSTRAINT_TYPE constraint_type;

  Rule(const std::wstring &_name, const std::wstring &_rule,
       RULE_TYPE _rule_type, int _ix, int _id, bool _is_input);
  // split rules
  void split_rule(SymbolTable &);
};

/*
Drawline

  Internally, Drawline use std::wstring for convience, though its input and output
  are string.

  I use UTFCPP to convert from utf8 string to std::wstring.
  If you want to wcin or wcout a std::wstring, remember to
    ios_base::sync_with_stdio(false);
    wcin.imbue(locale("zh_CN.UTF-8"));
    wcout.imbue(locale("zh_CN.UTF-8"));
*/
class Drawline {
 public:
  Drawline();
  ~Drawline();

  void push(const std::string &srule);
  void build();
  VS match(const std::string &text);
  void print(const std::string &text);
  void transform_rules();
  void print_rules();

 private:
  typedef std::vector<Rule> VR;

  void transform(int ix);
  void lowlevel_match(const std::wstring &text);
  void highlevel_match(const std::wstring &text);
  void resize_push(std::vector<VI> &, int x, int y);
  void resize_push(std::vector<VM> &, int x, MatchNode *y, bool order);
  void print(const std::wstring &text);
  int dist_match_node(MatchNode *a, MatchNode *b);
  VM left_construct(const Rule &rule, int i, int j, int st, int ed,
                    const std::wstring &text);
  VM left_construct(const Rule &rule, int st, int ed, const VM &all,
                    int now, VII child_ids, VM child_nodes);
  void up_construct(const std::wstring &text, MatchNode *pt);
  int get_sent_id(int x);
  int get_word_id(int x);
  bool check_duplicate(int id, int st, int ed);
  int erase_last(VII &vt, int x);
  VS get_result(const std::wstring &text);
  VS get_result_back(const std::wstring &text);
  std::string retrieve_args(MatchNode *pt, const std::wstring &args_nodees);
  void retrieve_args_nodes(MatchNode *pt,
                           std::map<std::wstring, VM> &args_nodes);
  void deal_special_case(const std::string &name, std::string &res);
  bool is_in_pos(int x, const VI &pos);
  
  AhoCorasick ac;
  SymbolTable word_table;
  SymbolTable rule_table;
  VR rules;
  std::vector<VI> level_rules;
  // id: ix1, ix2, .. for each id, records its rule ix
  std::vector<VI> id_ix;
  // id: pix1, pix2 ... for each id, record its parents ix
  std::vector<VI> id_pix;
  VI regex_rules;
  VM lowlevel_nodes;
  // nodes that have same id, and nodes are sorted by (ed, st)
  std::vector<VM> id_nodes;
  VI sent_pos;  // sentence delimiter positions
  VI word_pos;  // word delimiter positions
  bool is_transformed;
  bool is_built;
};
}

#endif  // SRC_DRAWLINE_H_
