#include "NameFilter.h"
#include <iostream>
#include <fstream>

using namespace std;

//Transcoding from utf8 to gbk, if failed return empty string
string CNameFilter::UTF8ToGBK(char src[])
{
   string ans;
   int len = strlen(src)*2+1;
   char *dst = (char *)malloc(len);
   if(dst == NULL)
   {
	  return ans;
   }
   memset(dst, 0, len);
   char *in = src;
   char *out = dst;
   size_t len_in = strlen(src);
   size_t len_out = len;

   iconv_t cd = iconv_open("GBK", "UTF-8");
   if ((iconv_t)-1 == cd)
   {
	  printf("init iconv_t failed\n");
	  free(dst);
	  return ans;
   }
   int n = iconv(cd, &in, &len_in, &out, &len_out);
   if(n<0)
   {
	  //printf("iconv failed\n");
   }
   else
   {
	  ans = dst;
   }
   free(dst);
   iconv_close(cd);
   return ans;
}

//valid special charactors, not include Chinese
bool CNameFilter::IsValidSpecialChar(char ch)
{
   string valid = " .'&()/#-\",";
   for(int i=0; i<valid.size(); i++)
   {
	  if(ch == valid[i])
		 return true;
   }
   return false;
}

//judge if the organization is valid or not
bool CNameFilter::IsValidOrg(const string &name)
{
   char *pname = (char*)malloc(name.size()+1);
   memset(pname, 0, name.size()+1);
   strcpy(pname, name.c_str());
   string str = UTF8ToGBK(pname);
   free(pname);
   pname = NULL;
   if(str == "")
	  return false;
   int len = str.size();
   short high, low;
   unsigned int code;
   string s;
   int bracket = 0;
   int mark = 0;
   for(int i=0; i<len; i++)
   {
	  if(str[i]>=0 || i==len-1)
	  {
		 if(!isupper(str[i]) && !islower(str[i]) && !isdigit(str[i]) && !IsValidSpecialChar(str[i]))
		 {

			return false;
		 }
		 else if(IsValidSpecialChar(str[i]) && (i==0 || (i==len-1 && str[i]!='.')) && str[i]!='(' && str[i]!=')' && str[i]!='#' && str[i]!='\"')
		 {
			return false;
		 }
		 else
		 {
			if(str[i]=='(')
			   bracket++;
			else if(str[i]==')')
			   bracket--;
			if(bracket<0)
			   return false;
		 }
	  }
	  else
	  {
		 high = (short)(str[i] + 256);
		 low = (short)(str[i+1]+256);
		 code = high*256 + low;
		 if(!(code>=0xB0A1 && code<=0xF7FE || code>=0x8140 && code<=0xA0FE || code>=0xAA40 && code<=0xFEA0))
		 {
			if(code==0xA1B6 || code==0xA1B0 || code==0xA3A8)  // 《“（
			   mark++;
			else if(code==0xA1B7 || code==0xA1b1 || code==0xA3A9) // 》”）
			   mark--;
			else if(code!=0xA1B6 && code!=0xA3AC && code!=0xA1A2 && code!=0xA1A4 && code!=0xA1F0 && code!=0xA996 && code!=0xA3BA && code!=0xA3A6)  //，，、· ○ 〇 ；：＆
			   return false;

			if(mark<0)
			   return false;
		 }
		 i++;
	  }
   }

   if(bracket!=0 || mark!=0) //括号或书名号引号不配对
	  return false;
   return true;
}

//judge if the person name is valid or not
bool CNameFilter::IsValidName(const string &name)
{
   if (invalid_names_.find(name) != invalid_names_.end())
	   return false;
   char *pname = (char*)malloc(name.size()+1);
   memset(pname, 0, name.size()+1);
   strcpy(pname, name.c_str());
   string str = UTF8ToGBK(pname);
   free(pname);
   pname = NULL;
   if(str == "")
	  return false;
   int len = str.size();
   short high, low;
   unsigned int code;
   unsigned int last_code = 0;
   string s;
   for(int i=0; i<len; i++)
   {
	  if(str[i]>=0 || i==len-1)
	  {
		 if(!isupper(str[i]) && !islower(str[i]) && str[i]!='.' && str[i]!='-' && str[i]!=' ')
		 {
			return false;
		 }
		 else if((i==0 || i==len-1 || (i>0 && (str[i-1]=='.' || str[i-1]=='-'))) && (str[i]=='.' || str[i]=='-'))
		 {
			return false;  //continuous . or - in not valid, neither does appear in the head or tail.
		 }
		 else
		 {
			last_code = (short)(str[i]+256);
		 }
	  }
	  else
	  {
		 high = (short)(str[i] + 256);
		 low = (short)(str[i+1]+256);
		 code = high*256 + low;

		 if(!(code>=0xB0A1 && code<=0xF7FE || code>=0x8140 && code<=0xA0FE || code>=0xAA40 && code<=0xFEA0))
		 {
			if((i>0 && code!=0xA1A4) || i==0 || i==len-2 || code==last_code) //chinese dot
			{
			   return false;
			}
		 }
		 last_code = code;
		 i++;
	  }
   }
   return true;
}

std::set<string>
CNameFilter::LoadInvalidNames(const string& filename)
{
	string name = "";
	std::set<string> invalid_names;
	ifstream ifs(filename.c_str());
	while(getline(ifs, name)) {
		invalid_names.insert(name);
	}
	return invalid_names;
}

std::set<string> CNameFilter::invalid_names_;// = CNameFilter::LoadInvalidNames("invalid-names.list");
