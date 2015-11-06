/*
  @copyright Russell Standish 2000-2013
  @author Russell Standish
  This file is part of Classdesc

  Open source licensed under the MIT license. See LICENSE for details.
*/

/* Analyse all class types within a preprocessed file, and output an
  action statement for all parent classes and members */

#include <assert.h>
#include <algorithm>
#include <string>
#include <vector>
#include <stack>
#include <stdio.h>
#include <iostream>
#include <ctype.h>
using namespace std;
#include "tokeninput.h"

// for parsing Objective flag
int objc = 0;
bool respect_private=false; /* default is to call descriptor on private members  */
bool typeName=false; // generate type and enum symbol tables
bool onBase=false;

// name of file being processed
string inputFile="stdin";
int errorcode=0;

// start of a legal C++ identifier
inline bool isIdentifierStart(char x)
{return x=='_'||isalpha(x);}

// valid interior character of legal identifier
inline bool isIdentifierChar(char x)
{return x=='_'||isalnum(x);}

/* store the action strings (arguments passed) for base classes and
   members, for each class parsed */

struct act_pair
{
  string name, action;
  bool base; // is a base (not a member)
  act_pair(string n, string a, bool b=false): 
    name(n), action(a), base(b) {}
};

typedef vector<act_pair> actionlist_t;

struct action_t
{
  string type, templ, namespace_name;
  actionlist_t actionlist;
  action_t(string t, actionlist_t a, string ns, string te="")
  {
    type=t; actionlist=a; templ=te; namespace_name=ns;
    string::size_type p=namespace_name.rfind("::");
    namespace_name.erase( p==string::npos? 0:p, string::npos);  
  }
};

// predicate indicating invalid preprocessor token character
struct NotAlNumUnderScore
{
  bool operator()(char x) const {
    return !isIdentifierChar(x);
  }
};

#if 0    // used for debugging purposes
class actions_t: public vector<action_t>
{
public:
  void push_back(action_t x)
  {
    cerr << "registering:" << x.type << endl;
    vector<action_t>::push_back(x);
  }
} actions;
#else

vector<action_t> actions;

#endif

// store nested types here
map<string,vector<string> > nested;

/* store a list of virtual functions here */
typedef hash_set<string> virt_list_t;
hash_map<string,virt_list_t> virtualsdb;
string namespace_name;

/* type_defined[class]=1 if defined, else 0 if only declared */
struct type_defined_t: hash_map<string,int>
{
  void defined(string name) {(*this)[name]=1; }
  void declared(string name)
  {
    /* strip class qualifiers */
    if (name.rfind("::")!=name.npos)
      name=name.substr(name.rfind("::")+2);
    /* don't register template declarations */
    if (name.find("<")!=name.npos) return;

    if (count(name)==0 || !(*this)[name])
      {	(*this)[name]=0; }
  }
} type_defined;

std::map<std::string, std::vector<std::string> > enum_keys;

// quick and dirty tokenizer
set<string> token_set(const string& s)
{
  set<string> r;
  string::size_type start, end;
  for (start=0; start<s.size(); start=end)
    {
      // advance to next alpha
      for (; start<s.size() && !isIdentifierStart(s[start]); ++start);
      //find end of token
      for (end=start+1; end<s.size() && isIdentifierChar(s[end]); ++end);
      if (start<s.size() && start<end) 
          r.insert(s.substr(start,end-start));
    }
  return r;
}

void assign_enum_action(tokeninput& input, string prefix)
{
  string enumname="enum ";
  if (input.token=="class")
    {
      input.nexttok();
      enumname+="class ";
    }
  enumname+=prefix + input.token;
  input.nexttok();

  if (input.token==":") // type specification supplied, so skip it
    while (!strchr(";{",input.token[0])) input.nexttok();


  if (input.token=="{") /* only assign action if definition, not declaration */
    {
      //      gobble_delimited(input,"{","}");
      // load up the list of enum symbols
      for ( ; input.token != ";"; input.nexttok())
        {
          if (input.token == "=" || input.token == "," || input.token=="}")
            if (input.lasttoken!=",") //not sure if this is correct C++, but
              //compilers accept blank enumerator-definitions
              enum_keys[enumname].push_back(input.lasttoken);
          if (input.token == "=") {input.nexttok(); input.nexttok();} //skip value
        }
      actionlist_t actionlist;
      if (typeName)
        actionlist.push_back(act_pair("","classdesc::enum_handle(arg)"));
      else
        actionlist.push_back(act_pair("","(int&)arg"));
      actions.push_back(action_t(enumname,actionlist,namespace_name));
    }
}
  
void parse_typedef(tokeninput& input, string prefix="");
void assign_class_action(tokeninput& input, string prefix,
			 string template_args, string classargs, 
                         bool is_private);

class register_classname
{
  actionlist_t& actionlist;
public:
  bool is_static;
  bool is_const;
  bool is_private;
  register_classname(actionlist_t& a, bool p): 
    actionlist(a), is_static(false), is_const(false), is_private(p) {}

  void register_class(string name, string action)
  {
    // We need to treat static const members differently, as simple
    // ones do not have addresses
    if (is_static && is_const)
      actionlist.push_back(act_pair("."+name,"classdesc::is_const_static(),"+action));
    else
      if (!is_private) /* don't register private, if privates respected */
      actionlist.push_back(act_pair("."+name,action));
    is_static=0;
  }
};

actionlist_t parse_class(tokeninput& input, bool is_class, string prefix="", string targs="")
{
  actionlist_t         actionlist;
  string               varname="arg", targs1;
  int                  is_virtual=0;
  bool is_template=false, is_private=is_class; // track private, protected decls
  virt_list_t          virt_list;
  register_classname   reg(actionlist, respect_private && is_class);
  hash_map<string,int> num_instances;
  string               baseclass;
  string               argList;
  string               rType;

  /* handle inheritance */
  for (;; input.nexttok() )
    {
      if (input.token=="virtual") continue;
      if (input.token==":" ||
	  input.token=="private" || input.token=="protected" ||
	  input.token=="public") 
        {
          is_private= (is_class && input.token!="public") ||
            (!is_class && (input.token=="private" || input.token=="protected"));
          continue;
        }
      if (input.token=="<")
	{
	  for (int ang_count=0; ang_count || input.lasttoken!=">";
	       input.nexttok())	
	    {
	      baseclass += input.token;
	      if (input.token=="<") ang_count++;
	      if (input.token==">") 
		{
		  ang_count--; 
		  if (ang_count) baseclass+=" ";/* handle nested templates */
		} 
	    }
	}
      if (strchr(",{", input.token[0]) && baseclass.length())
	{
          // work out whether a typename qualifier is needed. Check
          // whether any component of baseclass is contained in the
          // parameter list of the template arguments
          set<string> arg_toks=token_set(targs), 
            base_toks=token_set(baseclass), args_in_base;

          // set intersection
          for (set<string>::iterator i=base_toks.begin(); 
               i!=base_toks.end(); ++i)
            if (arg_toks.count(*i))
              args_in_base.insert(*i);

          string::size_type dbl_colon=baseclass.rfind("::");
          bool typename_needed=false;
          if (dbl_colon!=string::npos)
            for (set<string>::iterator i=args_in_base.begin(); 
                 i!=args_in_base.end(); ++i)
              if (baseclass.find(*i)<dbl_colon)
                {
                  typename_needed=true;
                  break;
                }
                
	  if (!respect_private || !is_private)
            actionlist.push_back(
			       act_pair("", 
                                        "classdesc::base_cast<"+
    // look for member qualification in a template context
           string(typename_needed? "typename ": "")+
                                        baseclass+" >::cast("+varname+")",
                                        onBase
                                        )
                               );
	  virt_list_t t=virtualsdb[baseclass+"::"];
	  virt_list.insert(t.begin(),t.end());
	  baseclass.erase();
          // reset is_private flag
          is_private=is_class;
	}
      else
	baseclass += input.token;
      

      if (input.token=="{") break;
    }

  /* now handle members */
  for  (input.nexttok(); input.token!="}"; input.nexttok() )
    {

      // cout << "Token = [" << input.token << "]\n";
      rType += input.token + " ";

      /* skip public/private etc labels */
      if (input.token=="private" || input.token=="protected" ||
	  input.token=="public")
	{
          is_private = (input.token=="private" || input.token=="protected");
	  reg.is_private = respect_private && is_private;
	  input.nexttok(); 
	  rType.erase(); 
	  continue;  /* skip ':' */
	}

      if (input.token=="typedef")
	{
          /* grab typedef name to handle nested type definitions */
          tokeninput::mark_t mark=input;
          while (input.token!=";") 
            {
              if (input.token=="{") gobble_delimited(input,"{","}");
              input.nexttok();
            }
          if (isIdentifierStart(input.lasttoken[0])) //ignore any strange function type typedefs
            nested[prefix].push_back(input.lasttoken);
          input=mark;
	  parse_typedef(input,prefix);
          while (input.token!=";") input.nexttok();
	  continue;
	}
      if (input.token=="using") //ignore using declarations
        {
          while (input.token!=";") input.nexttok();       
          continue;
        }
      if (input.token=="template")
	{
	  is_template=true;
	  input.nexttok();
	  targs1=get_template_args(input);
	  if ((input.token!="class" && input.token!="struct" )
	      || targs1=="< > ")
	    {
	      targs1.erase();  /* not a template class */
	    }
	}
      if (input.token=="class" || input.token=="struct")
	{
	  /* if boths targs and targs1 are defined, then we must
             merge template arguments */
	  string targs2;
	  if (targs.length() && targs1.length())
	    /* strip angle brackets and insert into targs */
	    targs2=targs.substr(0,targs.find('>')) + "," +
	      targs1.substr(1,targs1.find('>'));
	  else
	    targs2=targs+targs1;
	  input.nexttok();
          nested[prefix].push_back(input.token);
	  /* handle new templated typename rules */
	  if (targs.length()) input.lasttoken="typename";
	  if (isIdentifierStart(input.token[0]))  /* named class */
	    assign_class_action(input,prefix,targs2,targs1,is_private);
	  else
	    {
              cerr <<inputFile<<":"<<input.line()<<"::error";
	      cerr << "anonymous structs not allowed within class defs\n";
              errorcode=1;
#if 0
	      vector<string> sublist=parse_class(input,"#");
	      /* find var name -  checking it is not an array aaargh!*/
	
	      for (vector<string>::iterator i=sublist.begin();
		   i!=sublist.end(); i++)
		i->replace(i->find("#"),1,varname);
#endif
	    }
	  targs1.erase();
	  continue;
	}
      if (input.token=="enum")
	{
	  input.nexttok();
	  if (!is_private && 
              isIdentifierStart(input.token[0]))  /* named enum */
	    assign_enum_action(input,prefix);
	}

      /* handle templated types */
      if (input.token=="<") gobble_delimited(input,"<",">");

      /* handle unnamed unions as a special case */
      if (input.lasttoken=="union" && input.token=="{")
	{
	  gobble_delimited(input,"{","}");
	  while (input.token!=";") input.nexttok();
          if (isIdentifierStart(input.lasttoken[0]))
            reg.register_class
              (input.lasttoken,
               "classdesc::is_array(),(char&)("+varname+"."+input.lasttoken+
               "),1,sizeof("+varname+"."+input.lasttoken+")");
          continue;
	}
	
      /* scalar member type */
      if (strchr("{;,:=",input.token[0]) && input.token!="::")
	{ rType.erase();
	  if (isIdentifierStart(input.lasttoken[0]))
	    reg.register_class(input.lasttoken,
			       varname+"." + input.lasttoken );
          if (input.token[0]=='{')
            gobble_delimited(input,"{","}");
	  /* skip over field specifier or initializer */
	  while (!strchr(";,",input.token[0])) input.nexttok();
	}

      /* operator members - discard these */
      if (input.token=="operator")
	{
	  input.nexttok();
	  if (input.token=="()") input.nexttok(); /* skip () if operator() */
	  /* find and skip arguments */
	  while (input.token[0]!='(') input.nexttok();
	  if (input.token=="()") input.nexttok();
	  else
	    gobble_delimited(input,"(",")");
	  /* skip to end of statement */
	  while (!strchr("{;",input.token[0])) input.nexttok();
	  reg.is_static=false;
	}

      // alignas declarations need to be discarded
      if (input.token=="alignas") 
        {input.nexttok(); gobble_delimited(input,"(",")");}

      if (input.token=="virtual") is_virtual=true;
      if (input.token=="static") reg.is_static=true;
      if (input.token=="const") reg.is_const=true;
      if (input.token=="friend")  /* skip friend statement */
	while (input.token!=";") input.nexttok();

      if (input.token[0]=='(') /* member functions, or function pointers */
	{
	  string memname=input.lasttoken;        // function name

	  /* check ahead for function pointers */
	  if (input.token!="()")
            {
              input.nexttok();
              if (isIdentifierStart(input.token[0]) && /* possible member pointer */
		  (input.nexttok(),input.token=="::")) input.nexttok();
	      if (input.token=="*" &&        /* member/function pointer */
		  (input.lasttoken=="(" || input.lasttoken=="::"))
		{
		  input.nexttok();
		  memname=input.token;
		  gobble_delimited(input,"(",")");
		  while (input.token[0]!='(') input.nexttok();
		  gobble_delimited(input,"(",")");
		}
	      else if (input.token!=")") /* skip fn args */
              {  argList = gobble_delimited(input,"(",")");
              }
	    }
          else argList.erase();
    
          rType = rType.substr(0, rType.find(memname));

	  while (!strchr(";{",input.token[0])) input.nexttok();

	  /* member functions require object passed as well*/
	  if (isIdentifierStart(memname[0]))
	    {

	      if (!num_instances.count("."+memname))
		num_instances["."+memname]=1;
	      else
		num_instances["."+memname]++;


	      if (is_virtual) virt_list.insert(memname);

	      /* strip out qualifier components, and template
                 arguments, for comparison with the member function
                 name */
	      string sprefix=prefix.substr(0,prefix.length()-2);
	      sprefix=sprefix.substr(0,sprefix.find('<'));
              string::size_type start=sprefix.rfind("::");
	      if (start!=string::npos) sprefix=sprefix.substr(start+2);

	      /* do not register constructors/destructors also do not
		  register virtual members or inline members - g++ has
		  trouble taking their addresses */
	      if (memname!=sprefix && 
		  !is_template &&
		  memname!="CLASSDESC_ACCESS" &&
		  memname!="CLASSDESC_ACCESS_TEMPLATE")
              {  string action = 
                  varname+",&"+ namespace_name + prefix + memname;
                 if (objc) { action += ", \"" + rType + "\", " + "\"" + argList + "\""; }
                 rType.erase();
                 reg.register_class(memname, action);
              }
	    }
	  reg.is_static=0;
	  is_virtual=0;
	  is_template=false;
	}


      /* array member type */
      if (input.token=="[")
	{
          int dim = 0;
	  string name=input.lasttoken;
	  string action  = "classdesc::is_array(),"+varname+"." + name;
          string action2 = ",";
	  if (objc) action2+="\"";
	  for (; input.token=="["; input.nexttok())
	    {
	      dim++;
	      input.nexttok();
	      for (;input.token!="]"; input.nexttok())
              {
                 if (!objc)
                    action2 += " " + input.token;
                 else                                 // objc
                 {  
                    action2 += "[" + input.token + "]";
                 }
              }
	      if (!objc) action2 += ",";
	    }
          for (int i=0; i<dim; i++)
          { action += "[0]"; }
	  if (!objc) {char s[10]; sprintf(s,",%d",dim); action+=s;}
          action += action2;
	  if (!objc) action.erase(action.size()-1,1); /* remove trailing "," */
          else  action+="\"";
	  reg.register_class(name,action);
	}

      if (input.token=="{")  /* skip function/enum/whatever definitions */
	    gobble_delimited(input,"{","}");

    }

  /* remove overloaded member functions */
  actionlist_t copy_of_alist;
  string name;
  for (size_t i=0; i<actionlist.size(); i++)
    {
      name=actionlist[i].name;
      if (!num_instances.count(name) || num_instances[name]==1)
	copy_of_alist.push_back(actionlist[i]);
    }

  virtualsdb[prefix]=virt_list;

  return copy_of_alist;
}


/* parse a typedef statement, handling anonymous structs, unions and
   arrays - all other types are ignored */

class is_ptr {};

string get_typedef_name(tokeninput& input)
{
  int ptr;
  for (; input.token!=";"; input.nexttok())
    {
      if (input.token=="{") gobble_delimited(input,"{","}");
      ptr=input.lasttoken=="*";
    }
  if (ptr||input.lasttoken=="]")
    throw is_ptr(); /* the buggers have probably declared a pointer to */
                           /*  an anonymous struct data type */
  return input.lasttoken;
}

void parse_typedef(tokeninput& input, string prefix)
{
  actionlist_t actionlist;
  input.nexttok();
  if (input.token=="struct" || input.token=="class")
    {
      bool is_class=input.token=="class";
      tokeninput::mark_t mark=input;
      string name, structname;
      input.nexttok();
      if (isIdentifierStart(input.token[0])) structname=input.token;
      while (!strchr(";:{",input.token[0])) input.nexttok();
      if (input.token==";")
	{type_defined.declared(structname); return;} /* nothing to be done */
      try {name=prefix+get_typedef_name(input);}
      catch (is_ptr)   /* the buggers have declared a pointer to */
	{              /*  an anonymous struct data type */
	  /* check first to see if name already assigned to struct */
	  input=mark;
	  input.nexttok();
	  if (isIdentifierStart(input.token[0]))
	    name=input.token;
	  else
	    {
	      static int cnt=0;  /* declare a temporary data type */
	      char tname[20]; /* 10^10 temp identifiers should be ample here!*/
	      sprintf(tname,"___ecotmp%-10d",cnt++);
	      name=tname;
	      input=mark;
              cout << "#ifndef CLASSDESC_TYPENAME_"<<name<<endl;
              cout << "#define CLASSDESC_TYPENAME_"<<name<<endl;
	      cout << input.token << " " << name << " ";
	      for (int nlvl=0; input.token!="}" || nlvl>0; )
		{
		  input.nexttok();
		  if (input.token=="{") nlvl++;
		  if (input.token=="}") nlvl--;
		  cout << input.token << " ";
		}
	      cout << ";" << endl;
              cout << "#endif"<<endl;
	    }
	}
      input=mark;
      while (!strchr(";:{",input.token[0])) input.nexttok();
      // if a struct or class name has been provided, use that in
      // preference to the typedef name - see ticket #11
      if (!structname.empty()) name=prefix+structname;
      actionlist=parse_class(input,is_class,name+"::");
      type_defined.defined(structname);
      actions.push_back(action_t(name,actionlist,namespace_name));
    }
  else if (input.token=="union")
    {
      actionlist.push_back(
		     act_pair("","classdesc::is_array(),(char&)arg,1,sizeof(arg)"));
      try {
	actions.push_back(action_t(prefix+get_typedef_name(input),actionlist,namespace_name));
      }
      catch (is_ptr)
	{
	  actionlist_t actionlist;
	  actionlist.push_back(
			       act_pair("","arg"));
	  actions.push_back(action_t(prefix+input.lasttoken,actionlist,namespace_name));
	}
     }
  else
      while (input.token!=";") input.nexttok();
#if 0  /* what was I thinking here !!! */
  else
    {
      /* otherwise search for an array definition */
      while (input.token!="[" && input.token!=";") input.nexttok();
      if (input.token=="[")
	{
	  string size;
	  for (input.nexttok(); input.token!="]"; input.nexttok())
	    size+=input.token;
	  actions.push_back(action_t(prefix+get_typedef_name(input),actionlist,namespace_name));
	}
    }
#endif
}

/* create list of arguments used in a template */
string extract_targs(string targs)
{
  if (targs.empty()) return targs; /* nothing to do */
  string r="<", tok, elipsis;
  const char *targsptr=targs.c_str()+1;
  for (; *targsptr; targsptr++)
    {
      if (isIdentifierChar(*targsptr)) tok+=*targsptr;
      else
	{
	  while (isspace(*targsptr)) targsptr++;/* skip any spaces */
	  if (!*targsptr) break;    /* end of string already */
	  else if (*targsptr=='<')  /* skip over stuff between <..> */
	    {
	      int angle_count=1;
	      while (angle_count)
		{
		  targsptr++;
		  if (*targsptr=='<') angle_count++;
		  else if (*targsptr=='>') angle_count--;
		}
	    }
	  else if (*targsptr=='=')
	    {
	      for (int angle_count=0;
		   !strchr(">,",*(targsptr)) || angle_count; targsptr++)
		if (*targsptr=='<') angle_count++;
		else if (*targsptr=='>') angle_count--;
              // not sure, but inserting elipsis here might be a C++
              // grammatical error
	      r+=tok+elipsis+*targsptr;
              elipsis.clear();
	    }
	  else if (strchr(">,",*targsptr))  
            {
              r+=tok+elipsis+*targsptr;
              elipsis.clear();
            }
	  else if (isIdentifierChar(*targsptr))
	    targsptr--;  /* rewind ptr so next loop starts token */
          else if (*targsptr=='.')
            elipsis+=*targsptr;
	  tok.erase();
	}
    }
  return r;
}

void assign_class_action(tokeninput& input, string prefix, string targs, string classargs, bool is_private)
{
  /* refer to class/struct type with explicit class/struct leadin
     identifier, so as to avoid typename being hidden by
     object/function declarations */
  string classtok=input.lasttoken + " ";
  string classname = prefix + input.token;
  for (input.nexttok(); input.token=="::"; input.nexttok())
    {
      input.nexttok();
      classname+="::"+input.token;
    }
  if (input.token=="<") /* template specialisation */
    classname += get_template_args(input);
  else
    classname += extract_targs(classargs);
  if (strchr(":{",input.token[0]))
    {
      action_t action(classtok+classname,
			parse_class(input,classtok=="class ",classname+"::",targs),
                      namespace_name,targs);
      //only register action if class is public do not register nested
      // classes of templates, as that causes a partial specialisation
      // error
      if (!is_private && prefix.find("<")==string::npos) 
        actions.push_back(action);
      type_defined.defined(classname);
    }
  else  type_defined.declared(classname);
}


inline int leadeq(string x,const char *y)
{return strncmp(x.c_str(),y,strlen(y))==0;}

string rm_space(string x)
{
  x.erase(remove(x.begin(),x.end(),' '),x.end());
  return x;
}

void output_treenode_or_graphnode(const char* action, size_t i, const char* node_type)
{
  // escape to global namespace
  //  printf("}\n");
  /* now output treenode/graphnode definition */
  printf("template %s\n",
	   actions[i].templ.size()? actions[i].templ.c_str(): "<>");
  printf("struct access_%s<%s*>\n{\n", action, actions[i].type.c_str());
  printf("  template<class _CD_ARG_TYPE>\n");
  printf("  void operator()(classdesc::%s_t& targ, "
         "const classdesc::string& desc, _CD_ARG_TYPE& arg)\n", action);	
  printf("{%s(targ,desc,classdesc::%s,arg);}\n};\n\n",action,node_type);
  //  printf("namespace classdesc_access{\n");
}

class PrintNameSpace
{
  bool print;
public:
  PrintNameSpace(string ns): print(!ns.empty()) {if (print) printf("namespace %s {\n",ns.c_str());}
  ~PrintNameSpace() {if (print) printf("}\n");}
};

string type_qualifier(const string& type)
{
  if (leadeq(type,"class")) return "class";
  if (leadeq(type,"struct")) return "struct";
  if (leadeq(type,"enum class")) return "enum class";
  if (leadeq(type,"enum")) return "enum";
  if (leadeq(type,"typename")) return "typename";
  return string();
}

string without_type_qualifier(const string& type, const string& qualifier)
{
  assert(leadeq(type,qualifier.c_str()));
  string::size_type offs=qualifier.length();
  if (type[offs]==' ') offs++;
  return type.substr(offs);
}

string without_type_qualifier(const string& type)
{
  return without_type_qualifier(type,type_qualifier(type));
}

int main(int argc, char* argv[])
{

  int def=1;

  if (argc<2)
    {
      printf("usage: %s [-workdir dir] [-objc] [-include header] [-nodef] [-respect_private] [-I classdesc_includes] [-typeName] {descriptor name} <input >output\n",argv[0]);
      exit(0);
    }

  /* see if a work directory option has been supplied - if so, then
     output individual .cc files with function definitions. Otherwise,
     define as inlined functions to be output on stdout */
  string workdir, stdinclude;
  vector<string> include_path;
  include_path.push_back(".");
  while (argc>1 && argv[1][0]=='-')  /* process options */
    {
      if (strcmp(argv[1],"-workdir")==0)  /* place output files in workdir */
	{
          if (argc<3) {puts("option argument missing"); exit(1);} 
	  workdir=argv[2]; workdir+="/";  /* ensure trailing slash */
	  argc-=2; argv+=2;
	}
      if (argc>1 && strcmp(argv[1],"-objc")==0)     /* Generate Objective-C files */
	{ objc = 1;
	  argc--; argv++;
	}
      if (argc>1 && strcmp(argv[1],"-include")==0) /* standard header for each output */
	{
          if (argc<3) {puts("option argument missing"); exit(1);} 
	  stdinclude=argv[2];
	  argc-=2; argv+=2;
	}
      if (argc>1 && strcmp(argv[1],"-nodef")==0) /* don't provide definitions for */
	{                              /* undefined types */
	  def=0;
	  argc--; argv++;
	}
      if (argc>1 && strcmp(argv[1],"-respect_private")==0) /* don't describe private */
	{                              /* members */
	  respect_private=true;
	  argc--; argv++;
	}
      if (argc>1 && strcmp(argv[1],"-I")==0)  /* use this directory to find */
				       /*   _base.h files */
	{
          if (argc<3) {puts("option argument missing"); exit(1);} 
	  include_path.push_back(argv[2]);
	  argc-=2; argv+=2;
	}
      if (argc>1 && strcmp(argv[1],"-typeName")==0) 
        { // emit typeName database
          typeName=true;
          argc--; argv++;
        }
      if (argc>1 && strcmp(argv[1],"-onbase")==0) 
        { // enable distinction between base classes and members
          onBase=true;
          argc--; argv++;
        }
      if (argc>1 && strcmp(argv[1],"-i")==0) 
        { // input file given as argument
          if (argc<3) {puts("option argument missing"); exit(1);} 
          inputFile=argv[2];
          argc-=2; argv+=2;
        }
    }

  FILE* inputStream=stdin;
  if (inputFile!="stdin") 
    inputStream=fopen(inputFile.c_str(),"rb");
      
  tokeninput input(inputStream);

  char **action=argv+1;
  int nactions=argc-1;
  string basename;
  FILE *basef;
  char tname[1024];
  hash_map<string,int> omit, header_written, treenode, graphnode;
  int nconv;

  string targs;

  stack<tokeninput::mark_t> namespace_end;

  try
    {
      for (;;)
	{
	  input.nexttok();
	  if (input.token.find("#pragma")==0) /* process pragmas */
	    {
	      if (input.token.find("#pragma omit")==0)
		omit[rm_space(input.token.substr(strlen("#pragma omit ")))]=1;
	      if (input.token.find("#pragma treenode")==0)
		treenode[rm_space(input.token.substr(
				  strlen("#pragma treenode ")))]=1;
	      if (input.token.find("#pragma graphnode")==0)
		graphnode[rm_space(input.token.substr(
				  strlen("#pragma graphnode ")))]=1;
	      continue;
	    }

	  /* namespace support */
	  if (input.token=="using")
	    /* skip over any using namespace statements */
	    while (input.token!=";") input.nexttok();
	  if (namespace_end.size() && input>namespace_end.top())
	    /* finished parsing namespace */
	    {
	      /* strip trailing :: */
	      namespace_name.erase(namespace_name.rfind("::"));

	      /* calculate token boundary */
	      string::size_type p;
	      if (
		  (p=namespace_name.rfind("::"))
		   == string::npos
		  )
		p=0;
	      else
		p+=2;
	      namespace_name.erase(p);
	      namespace_end.pop();
	    }
	  if (input.token=="namespace")
	    /* append the namespace name to namespace, and mark the end of
	      the namespace region in the input stream
	      */
	    {
	      input.nexttok();
              if (input.token!="{") // not anonymous
                {
                  string name=input.token;
                  input.nexttok();
                  if (input.token=="=") // namespace alias, skip over
                    while (input.token!=";") 
                      input.nexttok();
                  else if (input.token=="{")
                    {
                      namespace_name+=name+"::";
                      tokeninput::mark_t here(input);
                      gobble_delimited(input,"{","}");
                      namespace_end.push(tokeninput::mark_t(input));
                      input=here;
                    }
                }
            }
	  /* end namespace support */

	  if (input.token=="typedef") parse_typedef(input);
	  if (input.token=="template")
	    {
	      input.nexttok();
              if (input.token!="<") 
                continue; //template instantiation or template member invocation
	      targs=get_template_args(input);
	      if ((input.token!="class" && input.token!="struct" )
		  || targs=="< > ")
		targs.erase();  /* not a template class */
	    }
 	  if (input.token=="class" || input.token=="struct")
	    {
	      input.nexttok();
	      if (isIdentifierStart(input.token[0]))  /* named class */
		assign_class_action(input,string(),targs,targs,false);
	      targs.erase();
	    }
	  if (input.token=="enum")
	    {
	      input.nexttok();
	      if (isIdentifierStart(input.token[0]))  /* named enum */
		assign_enum_action(input,"");
	    }
	  /* inline function definitions - must remove actionlist
             entry if there is one */
	  if (input.token=="(") /* remove function arguments */
	    gobble_delimited(input,"(",")");
	  
	}
    }

  catch(tokeninput::eof) {}

  printf("#include \"classdesc.h\"\n");

  /* make kludge definitions for all declared classes that aren't defined */
  if (def)
    for (type_defined_t::iterator i=type_defined.begin();
	 i!=type_defined.end(); i++)
      if (!i->second)
	printf("class %s {};\n",i->first.c_str());

  for (int k=0; k<nactions; k++)
      printf("#include \"%s_base.h\"\n",action[k]);

  /* output typeName database on standard output if requested. */
  if (typeName)
    {
      for (size_t i=0; i<actions.size(); i++)
        {
          // n is qualified by the namespace (if any).
          string cn=actions[i].type, n;
          
          /* remove leading type qualifer (if any) */
          if (type_qualifier(cn)=="typename") continue;
          cn=without_type_qualifier(cn);

          if (!actions[i].namespace_name.empty()) 
            {
              if (cn.substr(0,actions[i].namespace_name.size())==
                  actions[i].namespace_name) //strip leading ns
                cn=cn.substr(actions[i].namespace_name.size()+2);
              n=actions[i].namespace_name+"::"+cn;
            }
          else
            n=cn;

          // add a guard macro around this code
          string guardMacro="CLASSDESC_TYPENAME_"+n;
          ::replace_if(guardMacro.begin(), guardMacro.end(), 
                       NotAlNumUnderScore(), '_');
          printf("#ifndef %s\n",guardMacro.c_str());
          printf("#define %s\n",guardMacro.c_str());
          {
            PrintNameSpace cd("classdesc");
            if (actions[i].templ.empty())
              printf("template <> inline std::string typeName<%s >()\n  {return \"%s\";}\n",
                   n.c_str(), n.c_str());
            else
              {
                printf("template %s struct tn<%s >\n{\n",actions[i].templ.c_str(), n.c_str());
                printf("static std::string name()\n  {return \"%s\";}\n};\n",n.c_str());
              }
            
            /* For enums, print out a value name table */
            if (leadeq(actions[i].type,"enum")) 
              {
                vector<string>& keys=enum_keys[actions[i].type];
                {
                  PrintNameSpace anon(" ");
                if (keys.size())
                  {
                    printf("template <> EnumKey enum_keysData<%s>::keysData[]=\n {\n",n.c_str());
                    // enum constant qualifier - set blank if enum is global ns
                    string k;
                    if (leadeq(actions[i].type,"enum class"))
                      k=n; // newer enum class constants are in their own namespace
                    else // old enum constants are in the external namespace
                      k=n.find("::")==string::npos? "":
                      n.substr(0,n.rfind("::"));
                    
                    for (size_t i=0; i<keys.size(); i++)
                      {
                        printf("  {\"%s\",int(%s::%s)}",keys[i].c_str(),
                               k.c_str(),keys[i].c_str());
                        if (i<keys.size()-1)
                          printf(",\n");
                      }
                    printf("\n };\n");
                    printf("template <> EnumKeys<%s> enum_keysData<%s>::keys"
                           "(enum_keysData<%s>::keysData,"
                           "sizeof(enum_keysData<%s>::keysData)/"
                           "sizeof(enum_keysData<%s>::keysData[0]));\n",
                       n.c_str(),n.c_str(),n.c_str(),n.c_str(),n.c_str());
                  }
                  printf("template <> int enumKey<%s>(const std::string& x)"
                         "{return int(enum_keysData<%s>::keys(x));}\n",
                         n.c_str(),n.c_str());
                  printf("template <> std::string enumKey<%s>(int x)"
                         "{return enum_keysData<%s>::keys(x);}\n",
                         n.c_str(),n.c_str());
                }
              }
          }
          printf("#endif\n"); // terminate guard macro
        }

    }

  PrintNameSpace ns("classdesc_access");
  for (int k=0; k<nactions; k++)
    {
      /* parse action_base for types to omit */
      //      omit.clear();
		size_t i;
      for (basef=NULL, i=0; basef==NULL && i<include_path.size(); i++)

	{
	  basename=include_path[i]+"/"+action[k]+"_base.h";
	  basef=fopen(basename.c_str(),"r");
	}
      if (basef!=NULL)
	{
	  while ((nconv=fscanf(basef,"#pragma omit %1024[^\n]",tname))!=EOF)
	    {
	      if (nconv==1)
		{omit[rm_space(tname)]=1;}
	      else
		  {
			int c;
			while ((c=fgetc(basef))!='\n' && c!=EOF);
		  }
	    }
	  fclose(basef);
	}

      for (size_t i=0; i<actions.size(); i++)
	{	  /* check if type has a #pragma associated */
	  bool is_treenode, is_graphnode;
	  {
	    string n=without_type_qualifier(actions[i].type);

	    /* strip out template arguments, collapse multiple spaces, 
	       remove trailing space */
	    string n1;
            if (actions[i].namespace_name.size()) 
              n1=actions[i].namespace_name+"::";
	    unsigned nangle=0;
	    bool space=false;
	    for (unsigned j=0; j<n.length(); j++)
	      {
		if (n[j]=='<') nangle++;
		if (nangle==0) 
		  {
		    if (n[j]==' ')
		      space=true;
		    else
		      {
			if (space) n1+=' ';
			space=false;
			n1+=n[j];
		      }
		  }
		if (n[j]=='>') nangle--;
	      }

	    is_treenode=treenode.count(n1)>0;
	    is_graphnode=graphnode.count(n1)>0;
	    /* if both treenode and graphnode specified,
	       this is technically an error */
	    if (is_treenode&&is_graphnode)
              {
                cerr <<inputFile<<":"<<"0::error";
                cerr << "Both treenode and graphnode specified for type " << actions[i].type << endl;
                errorcode=1;
              }
	    if (omit.count(action[k]+n1))
	      continue;
	  }

	  /* scalar forward function */
	  if (workdir.length()==0 || !actions[i].templ.empty())
	    /* templates must be inline */
	    {
	      /* deal with graphnode/treenode possibilities */
	      if (is_treenode)
		output_treenode_or_graphnode(action[k],i,"is_treenode()");
	      if (is_graphnode)
		output_treenode_or_graphnode(action[k],i,"is_graphnode()");

              // insert the namespace qualifier if needed
              string type_arg_name;
              type_arg_name = (type_qualifier(actions[i].type)=="enum class")?
                "enum": type_qualifier(actions[i].type);
              type_arg_name+=" ::"+actions[i].namespace_name+
                (actions[i].namespace_name.size()?"::":"") + 
                without_type_qualifier(actions[i].type);

              printf("template %s struct access_%s<%s > {\n",
                     actions[i].templ.empty()? "<>": actions[i].templ.c_str(),
                     action[k], type_arg_name.c_str()); 
              printf("template <class _CD_ARG_TYPE>\n");
              printf("void operator()(classdesc::%s_t& targ, const classdesc::string& desc,_CD_ARG_TYPE& arg)\n{\n",
		     action[k]);

              if (actions[i].namespace_name.size())
                printf("using namespace %s;\n",actions[i].namespace_name.c_str());
              string::size_type p=actions[i].type.rfind("::");
              if (p!=string::npos)
                {
                  string::size_type s=actions[i].type.rfind(" "); //strip leading words
                  if (s==string::npos) 
                    s=0;
                  else
                    s++;
                  string prefix=actions[i].type.substr(s,p+2-s);
                  for (size_t j=0; j<nested[prefix].size(); j++)
                    printf("typedef %s %s%s %s;\n",
                           actions[i].templ.empty()? "": "typename",
                           prefix.c_str(),
                           nested[prefix][j].c_str(),nested[prefix][j].c_str());
                }
              for (size_t j=0; j<actions[i].actionlist.size(); j++)
                {
                  string a=action[k];
                  if (actions[i].actionlist[j].base)
                    a+="_onbase";
                  printf("::%s(targ,desc+\"%s\",%s);\n",a.c_str(),
                         actions[i].actionlist[j].name.c_str(),
                         actions[i].actionlist[j].action.c_str());
                }
              printf("}\n};\n");
 	    }
	  else
	    {
	      string oname=actions[i].type;
	      /* replace non-alphanumeric characters with underscores */
	      /* Windoze is intolerant of certain characters in filenames */
	      for (size_t l=0; l<oname.length(); l++)
		if (!isalnum(oname[l])) oname[l]='_';
	      oname=workdir+oname+".cc";
	      FILE *o=fopen(oname.c_str(),"a");
	      if (stdinclude.length() && !header_written.count(oname))
		{
		  fprintf(o,"#include \"%s\"\n",stdinclude.c_str());
		  header_written[oname]=1;
		}
	      fprintf(o,"#include \"%s_base.h\"\n",action[k]);
	      /* deal with graphnode/treenode possibilities */
	      if (is_treenode)
		output_treenode_or_graphnode(action[k],i,"is_treenode()");
	      if (is_graphnode)
		output_treenode_or_graphnode(action[k],i,"is_graphnode()");

              // insert the namespace qualifier if needed
              string type_arg_name;
              type_arg_name=type_qualifier(actions[i].type)
                +" ::"+actions[i].namespace_name+
                (actions[i].namespace_name.size()?"::":"") + 
                without_type_qualifier(actions[i].type);
              
              printf("template %s struct access_%s<%s > {\n",
                     actions[i].templ.empty()? "<>": actions[i].templ.c_str(),
                     action[k], type_arg_name.c_str()); 
              printf("template <class _CD_ARG_TYPE>\n");
              printf("void operator()(classdesc::%s_t& targ, const classdesc::string& desc,_CD_ARG_TYPE& arg);\n};",
		     action[k]);

              fprintf(o,"template <class _CD_ARG_TYPE>\n");
              fprintf(o,"void classdesc_access::access_%s<%s >::",
                     action[k], type_arg_name.c_str()); 
              fprintf(o,"operator()(classdesc::%s_t& targ, const classdesc::string& desc,_CD_ARG_TYPE& arg)\n{\n",
		     action[k]);

              if (actions[i].namespace_name.size())
                fprintf(o,"using namespace %s;\n",actions[i].namespace_name.c_str());
              string::size_type p=actions[i].type.rfind("::");
              if (p!=string::npos)
                {
                  string::size_type s=actions[i].type.rfind(" "); //strip leading words
                  if (s==string::npos) 
                    s=0;
                  else
                    s++;
                  string prefix=actions[i].type.substr(s,p+2-s);
                  for (size_t j=0; j<nested[prefix].size(); j++)
                    fprintf(o,"typedef %s %s%s %s;\n",
                           actions[i].templ.empty()? "": "typename",
                           prefix.c_str(),
                           nested[prefix][j].c_str(),nested[prefix][j].c_str());
                }
              for (size_t j=0; j<actions[i].actionlist.size(); j++)
                {
                  string a=action[k];
                  if (actions[i].actionlist[j].base)
                    a+="_onbase";
                  fprintf(o,"::%s(targ,desc+\"%s\",%s);\n",a.c_str(),
                         actions[i].actionlist[j].name.c_str(),
                         actions[i].actionlist[j].action.c_str());
                }
              fprintf(o,"};\n");
              // explicit instantiation
              fprintf(o,"template\n");
              fprintf(o,"void classdesc_access::access_%s<%s >::",
                     action[k], type_arg_name.c_str()); 
              fprintf(o,"operator()(classdesc::%s_t& targ, const classdesc::string& desc,%s& arg);\n",
		     action[k], type_arg_name.c_str());
              fprintf(o,"template\n");
              fprintf(o,"void classdesc_access::access_%s<%s >::",
                     action[k], type_arg_name.c_str()); 
              fprintf(o,"operator()(classdesc::%s_t& targ, const classdesc::string& desc,const %s& arg);\n",
		     action[k], type_arg_name.c_str());
              fprintf(o,"#include <classdesc_epilogue.h>\n");

	      fclose(o);
	    }
	}

    }

  return errorcode;
}
