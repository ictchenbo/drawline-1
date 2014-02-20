#include "cstdlib"
#include "cstring"

#include "set"
#include "list"
#include "iostream"
#include "string"
#include "fstream"

#include "ne.h"
#include "NameFilter.h"

using namespace ICTLAP::CORE;

const char *USAGE = "./ne_driver article >out";

bool is_blank(const std::string &str);

void strip(std::string &str,
           const std::string delimiters);

int main(int argc, char **argv) {
  if (argc != 2) {
    std::cerr << USAGE << std::endl;
    exit(-1);
  }

  // read article
  const char *article = argv[1];
  std::string text;
  std::ifstream fin(article);
  for (std::string line; std::getline(fin, line); ) {
    text += line + "\n";
  }

  //
  CNameFilter nf;
  std::set<std::string> ne_set;
  NE::NEapi h = NE::Init("ictlap/ictlap.conf");
  if (h == NULL) {
    std::cerr << "NE initialization failed!" << std::endl;
    exit(-1);
  }
  NE::NEres r = NE::Parse(h, text.c_str(), strlen(text.c_str()));
  if (r != NULL) {
    for (std::list<NE::annoteNode>::iterator it = r->labels.begin();
	 it != r->labels.end(); it++) {
      std::string type_name;
      if (it->tp == NE::peo) {
	type_name = "NAME";  // not PERSON
      } else if (it->tp == NE::org) {
          type_name = "ORGANIZATION";        
      } else if (it->tp == NE::loc) {
          type_name = "LOCATION";        
      }
      // remove some long CONCEPT string
      // they're mainly noise
      if (it->len > 100) {
	continue;
      }
      std::string concept = text.substr(it->offset, it->len);
      if (is_blank(concept) ||
	  type_name == "NAME" &&
	  !(concept.size() > 2 && nf.IsValidName(concept))) {
	continue;
      }
      // note the 2byte blank, it's not processed here
      strip(concept, "\r\n ");
      if (ne_set.count(type_name + ":" + concept)) {
	continue;
      }
      ne_set.insert(type_name + ":" + concept);
      std:: cout << "CONCEPT:" << type_name << ":" << concept << std::endl;
    }
  }
  NE::ReleaseRes(h, r);
  return 0;
}

bool is_blank(const std::string &str) {
  if (str.empty()) {
    return true;
  }
  if (str.find(" ") == 0 && is_blank(str.substr(1))) {
    return true;
  }
  if (str.find("　") == 0 &&
      is_blank(str.substr(std::string("　").length()))) {
    return true;
  }
  return false; 
}

void strip(std::string &str,
           const std::string delimiters) {
  size_t beg = str.find_first_not_of(delimiters);
  size_t end = str.find_last_not_of(delimiters);
  if (beg == std::string::npos ||
      end == std::string::npos ||
      beg > end) {
    str = "";
    return;
  }
  str = str.substr(beg, end - beg + 1);
}
