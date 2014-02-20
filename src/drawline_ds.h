/*
  Copyright 2013 ICT  

  Developer : Jiyuan Lin (linjiyuan90@gmail.com)
  Date : 05/13/2013

  file : drawline_ds.h
*/

#ifndef SRC_DRAWLINE_DS_H_
#define SRC_DRAWLINE_DS_H_

#include "map"
#include "set"
#include "string"
#include "vector"

typedef std::map<std::wstring, int> MSI;
typedef std::map<int, int> MII;
typedef std::vector<int> VI;
typedef std::set<int> SI;

// special case of english sentence delimiter
// 1.23
// ict.ac.cn
// MTIE has a dict
const std::wstring SENTENCE_DELIMITER = L"\n.!?。？！";

/*
    SymbolTable:
        map a string to a unique id (start from 0)
 */
class SymbolTable {
 public:
  int operator[] (const std::wstring &);
  int count(const std::wstring &);

  MSI table;
};

/* MatchNode */
class MatchNode {
 public:
  int st;
  int ed;
  int sent_id;
  int rule_id;
  int rule_ix;
  bool same_sent;
  std::vector<MatchNode *> child_nodes;

  MatchNode(int _rule_ix, int _rule_id, int _sent_id, int _st, int _ed,
            std::vector<MatchNode *> &_child_nodes) {
    rule_ix = _rule_ix;
    rule_id = _rule_id;
    sent_id = _sent_id;
    st = _st;
    ed = _ed;
    child_nodes = _child_nodes;
  }

  MatchNode(int _rule_ix, int _rule_id, int _sent_id, int _st, int _ed) {
    rule_ix = _rule_ix;
    rule_id = _rule_id;
    sent_id = _sent_id;
    st = _st;
    ed = _ed;
  }

  MatchNode(int _rule_ix, int _sent_id, int _st, int _ed) {
    rule_ix = _rule_ix;
    sent_id = _sent_id;
    st = _st;
    ed = _ed;
  }

  // Note first key is ed, not st
  bool operator < (const MatchNode &that) const {
    return ed < that.ed ||
           ed == that.ed && st < that.st;
  }
};

inline bool MATCHNODE_ED_CMP(const MatchNode *a, const MatchNode *b) {
  return *a < *b;
}

typedef std::vector<MatchNode *> VM;

/*
    Trie:
        Trie node, used in AC automaton
 */
class Trie {
 public:
  typedef std::map<int, Trie *> MIT;
  typedef std::vector<Trie *> VT;

  /*
    hash_std::map is slower due to its huge memory consumption. It is more appropriate 
    for many elements. 

    Though map is faster than hash_std::map in this situation, it's still worse
    than static array, namely 
      Trie *child[CHARSET];
    Therefore, if the number of children is small(tens or hundreds), it's strongly
    recommended to use static array! And if you use static array, the code need to
    be modified. Do it yourself.
  */
  MIT child;
  Trie *fail;  // KMP fail pointer, used in building AC automaton
  // Store matched rule_ids
  SI tag;
  int tags;

  Trie();
  ~Trie();
  Trie *operator[] (int k);

 private:
  // Record all the new nodes created in this node. Used for deconstruction.
  VT child_vt;
};

/* Aho-Corasick automaton */
class AhoCorasick {
 public:
  typedef std::map<int, Trie *> MIT;

  AhoCorasick();
  ~AhoCorasick();
  void insert(int, const VI &);
  void insert(int, SymbolTable &, const std::wstring &);
  void build();
  VM match(SymbolTable &, const std::wstring &, VI &, VI &);

 private:
  Trie *root;
  Trie *find(Trie*, int);
};

#endif  // SRC_DRAWLINE_DS_H_

