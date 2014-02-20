/*
  Copyright 2013 ICT

  Developer : Jiyuan Lin (linjiyuan90@gmail.com)
  Date : 05/13/2013

  file : utility.h
*/

#ifndef SRC_UTILITY_H_
#define SRC_UTILITY_H_

#include "string"
#include "sstream"
#include "vector"

#include "./utf8.h"

inline void decode_utf8(const std::string& bytes, std::wstring& wstr) {
  try {
    utf8::utf8to32(bytes.begin(), bytes.end(), std::back_inserter(wstr));
  } catch (std::exception utfcpp_ex) {
    std::cerr << "invalid utf8" << std::endl;
    wstr = L"";
  }
}

inline void encode_utf8(const std::wstring& wstr, std::string& bytes) {
  utf8::utf32to8(wstr.begin(), wstr.end(), std::back_inserter(bytes));
}

inline std::string wstr2str(const std::wstring &wstr) {
  std::string str;
  encode_utf8(wstr, str);
  return str;
}

inline std::string int2str(int x) {
  std::stringstream ss;
  ss << x;
  return ss.str();
}

inline std::vector<std::wstring> split(const std::wstring &str,
                                       const std::wstring &delim) {
  std::vector<std::wstring> vs;
  std::wstring::size_type st = 0, ed = 0;
  do {
    st = str.find_first_not_of(delim, ed);
    ed = str.find_first_of(delim, st);
    if (ed > st) {
      vs.push_back(str.substr(st, ed - st));
    }
  } while (st != std::wstring::npos);
  return vs;
}

#endif  // SRC_UTILITY_H_
