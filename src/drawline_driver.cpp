/*
  Copyright 2013 ICT  

  Developer : Jiyuan Lin (linjiyuan90@gmail.com)
  Date : 05/13/2013

  file : drawline_driver.cpp
  This file is an example about how to use Drawline.
*/

#include "cstdio"
#include "cstring"
#include "cassert"

#include "sstream"
#include "fstream"
#include "iostream"
#include "vector"
#include "string"

#include "./drawline.h"

int main(int argc, char * args[]) {
  if (argc < 3) {
    std::cerr << "Usage: ./drawline [-run] cfg dat >out 2>err"
              << std::endl;
    exit(-1);
  }

  VS args_vs;
  for (int i = 1; i < argc; i++) {
    if (std::string(args[i]) == "-run") {
      continue;
    }
    args_vs.push_back(args[i]);
  }

  if (args_vs.size() != 2) {
    std::cerr << "Usage: ./drawline [-run] cfg dat >out 2>err"
              << std::endl;
    exit(-1);
  }

  const char *cfg_file = args_vs[0].c_str();
  const char *dat_file = args_vs[1].c_str();

  drawline::Drawline dl;

  // read config file which consists of rules
  std::ifstream in;
  in.open(cfg_file, std::ifstream::in);
  for (std::string line; getline(in, line); ) {
    // ignore comments
    if (line[0] == '/' || line[0] == '#' || line.length() < 3) {
      continue;
    }
    dl.push(line);
  }
  in.close();

  // read text file
  in.open(dat_file, std::ifstream::in);
  std::string text;
  for (std::string line; getline(in, line); ) {
    text += line + "\n";
  }
  in.close();

  const VS &vs = dl.match(text);
  for (int i = 0; i < vs.size(); i++) {
    // vs[i] has three types:
    // 1)
    // 34440 21
    // action:建立
    // person:薛建军
    // 2)
    // *****
    // 3)
    // name:薛建军
    printf("%s", vs[i].c_str());
  }

  return 0;
}

