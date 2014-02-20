/*
  Copyright 2013 ICT  

  Developer : Jiyuan Lin (linjiyuan90@gmail.com)
  Date : 05/13/2013

  file : drawline_ds.cpp
*/

#include "cctype"
#include "cstdio"
#include "cstring"
#include "cassert"
#include "cmath"

#include "sstream"
#include "iostream"
#include "algorithm"
#include "vector"
#include "map"
#include "set"
#include "queue"
#include "deque"
#include "list"
#include "string"

#include "./drawline_ds.h"
#include "./utility.h"

/* Implement of SymbolTable */
int SymbolTable::operator[] (const std::wstring &key) {
  MSI::iterator it = table.find(key);
  return it == table.end() ? table[key] = table.size() - 1 : it->second;
}

int SymbolTable::count(const std::wstring &key) {
  return table.count(key);
}

/* Implement of Trie */
Trie::Trie() {
  fail = NULL;
  child_vt.reserve(50);
  tags = 0;
}

Trie::~Trie() {
  for (VT::iterator it = child_vt.begin(); it != child_vt.end(); it++) {
    delete *it;
  }
}

/* Find child[k], if it doesn't exist, insert it. */
Trie * Trie::operator[](int k) {
  MIT::iterator it = child.find(k);
  if (it != child.end()) {
    return it->second;
  }
  child_vt.push_back(child[k] = new Trie());
  return child_vt.back();
}

/*
  Implement of Aho-Corasick automaton
  Reference:
    http://blog.watashi.ws/2102/icpc-2011-dalian-dlut-cont/ZOJ3545/
 */
AhoCorasick::AhoCorasick() {
  root = new Trie();
}

AhoCorasick::~AhoCorasick() {
  delete root;
}

void AhoCorasick::insert(int tag, const VI &word) {
  if (word.empty()) {
    return;
  }
  Trie* cur_node = root;
  for (int i = 0; i < word.size(); i++) {
    cur_node = (*cur_node)[word[i]];
  }
  cur_node->tag.insert(tag);
  cur_node->tags = 1;
}

void AhoCorasick::insert(int tag, SymbolTable &table,
                         const std::wstring &word) {
  if (word.empty()) {
    return;
  }
  Trie* cur_node = root;
  for (int i = 0; i < word.size(); i++) {
    cur_node = (*cur_node)[table[std::wstring(1, word[i])]];
  }
  cur_node->tag.insert(tag);
  cur_node->tags = 1;
}

/*
  Modify chaining nodes.
  This find operation is similar to the one in disjoint-set.
*/
Trie *AhoCorasick::find(Trie *r, int ch) {
  MIT::iterator it = r->child.find(ch);
  if (it != r->child.end()) {
    return it->second;
  }
  if (r == root) {
    return root;
  }
  return r->child[ch] = find(r->fail, ch);
}

/*
  Generally, buidling AC automaton need to set:
    r->child[i]->fail = r->fail->child[i],  r has child i
    r->child[i] = r->fail->child[i],    r doesn't have child i  
  But this require full storage of the children, for example, 
    Trie * child[CHARSET];
  Since CHARSET may be very large in real text, this implement only stores its 
  necessary children, that's
    map<int, Trie*> children
  But this lead to a problem when building automaton or matching when a node
  does not have required children.
  Therefore, this implement genereates the required children when building and 
  matching, and set this new node's fail pointer to suitable node. Of course,
  this will lead to chaining modification. I adapt the 'find' operation of
  disjoint-set to tackle it.
*/ 
void AhoCorasick::build() {
  std::queue<Trie *> bfs;
  root->fail = root;
  for (MIT::iterator it = root->child.begin(); it != root->child.end(); it++) {
    it->second->fail = root;
    bfs.push(it->second);
  }
  while (!bfs.empty()) {
    Trie *r = bfs.front();
    bfs.pop();
    SI &tag = r->fail->tag;
    r->tags |= r->fail->tags;  // needed!
    // Don't do the following insert, it's slow! Do it in the match stage.
    // r->tag.insert(tag.begin(), tag.end());
    for (MIT::iterator it = r->child.begin(); it != r->child.end(); it++) {
      it->second->fail = find(r->fail, it->first);
      bfs.push(it->second);
    }
  }
}

VM AhoCorasick::match(SymbolTable &table, const std::wstring &text,
                      VI &sent_pos, VI &word_pos) {
  int ch, last_ch = 0;
  for (int i = 0; i < text.size(); i++) {
    ch = text[i];
    // Guessed from MTIE's manual, a word is:
    //    Single Chinese character (including puncations),
    //    English word,
    //    Number,
    //    sentence delimiter.
    //    consecutive punctuations/spaces
    // Ignore spaces, other punctuations
    if (ch > 127) {
      word_pos.push_back(i);
    } else if (SENTENCE_DELIMITER.find(text[i]) != std::wstring::npos) {
      // sentence delimiter
      word_pos.push_back(i);
    } else if (isalpha(ch) && (last_ch > 127 || !isalpha(last_ch))) {
      // start letter of a word
      word_pos.push_back(i);
    } else if (isdigit(ch) && (last_ch > 127 || !isdigit(last_ch))) {
      // start digit of a number
      word_pos.push_back(i);
    } else if ((ispunct(ch) || isspace(ch)) &&
             (last_ch > 127 || !ispunct(last_ch) && !isspace(last_ch))) {
      word_pos.push_back(i);
    }
    last_ch = ch;
  }

  VM match_nodes;
  int sent_id = 0;
  Trie *cur_node = root;
  for (int i = 0; i < text.size(); i++) {
    // encounter a sentence delimiter
    if (SENTENCE_DELIMITER.find(text[i]) != std::wstring::npos) {
      sent_pos.push_back(i);
      sent_id++;
    }
    std::wstring str(1, text[i]);
    if (!table.count(str)) {  // skip unrelated chars
      cur_node = root;
      continue;
    }
    ch = table[str];
    MIT::iterator ct = cur_node->child.find(ch);
    if (ct == cur_node->child.end()) {
      /*
        The following assigment is danger, since it'll insert ch automatically
        before find.
        Use insert.
      */
      // cur_node->child[ch] = find(cur_node->fail, ch);
      ct = cur_node->child.insert(
             std::make_pair(ch, find(cur_node->fail, ch))).first;
    }
    cur_node = ct->second;
    // if (!cur_node->tag.empty()) {
    // this may lead to some miss
    // for example:
    // abcdefg to match abcd, bc, only abcd can be matched
    if (cur_node->tags) {
      for (Trie *jump_node = cur_node; jump_node->tags;
          jump_node = jump_node->fail) {
        for (SI::iterator jt = jump_node->tag.begin();
             jt != jump_node->tag.end(); jt++) {
          // st need to determine later
          match_nodes.push_back(new MatchNode(*jt, sent_id, i, i));
        }
      }
    }
  }

  return match_nodes;
}


