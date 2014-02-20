#ifndef _NAME_FILTER_
#define _NAME_FILTER_

//add on 2012-4-11
//filter person name and organization

#include <iconv.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <string>
#include <set>

using namespace std;

class CNameFilter
{
   public:
	  static std::set<string> invalid_names_;

	  //judge if the organization is valid or not
	  bool IsValidOrg(const string &name);

	  //judge if the person name is valid or not
	  bool IsValidName(const string &name);

	  //Load invalid names from file.
	  static std::set<string> LoadInvalidNames(const string &filename);


   private:
	  //Transcoding from utf8 to gbk, if failed return empty string
	  string UTF8ToGBK(char src[]);

	  //valid special charactors, not include Chinese
	  bool IsValidSpecialChar(char ch);
};

#endif
