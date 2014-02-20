/*
  Copyright 2013 ICT  

  Developer : Jiyuan Lin (linjiyuan90@gmail.com)
  Date : 05/13/2013

  file : mylog.h
*/

#ifndef SRC_MYLOG_H_
#define SRC_MYLOG_H_

#include "map"
#include "string"
#include "set"
#include "ostream"

class MyLog {
 public:
  void add(const std::string &key, const std::string &value) {
    mylog[key].insert(value);
  }

  void add(const char *key, const std::string &value) {
    mylog[key].insert(value);
  }
    
  void print(std::ostream &out) {
    for (auto it = mylog.begin(); it != mylog.end(); it++) {
      if (it->second.empty()) {
	continue;
      }
      out << it->first;
      bool is_first = true;
      for (auto value : it->second) {
	if (!value.empty()) {
	  out << (is_first ? ":" : ",");
	}
	out << value;
	is_first = false;
      }
      out << std::endl;
    }
  }

  void merge(const MyLog &that) {
    for (auto it = that.mylog.begin(); it != that.mylog.end(); it++) {
      mylog[it->first].insert(it->second.begin(), it->second.end());
    }
  }

  void clear() {
    mylog.clear();
  }

  std::map<std::string, std::set<std::string>> mylog;
};

#endif  // SRC_MYLOG_H_
