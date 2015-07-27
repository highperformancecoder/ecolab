/*
  @copyright Russell Standish 2000-2013
  @author Russell Standish
  This file is part of Classdesc

  Open source licensed under the MIT license. See LICENSE for details.
*/

/* process a header file, inserting the macro
CLASSDESC_ACCESS(classname) into each class or struct encountered
conatining private definitions. CLASSDESC_ACCESS(classname) can then
be defined to be a list of friend statments: eg

#define CLASSDESC_ACCESS(type)\
friend void pack(pack_t *,eco_string,type&);

*/

#include <string>
#include <vector>
#include <stdio.h>
//#include <fstream>
using namespace std;
#include "tokeninput.h"

/* mapping of template arguments as how insert-friend sees them, to
   how they should really be */
hash_map<string,string> targ_map;

/* create list of arguments used in a template */
string extract_targs(string targs)
{
  if (targs.empty()) return targs; /* nothing to do */
  string r="<", tok;
  const char *targsptr=targs.c_str()+1;
  for (; *targsptr; targsptr++)
    {
      if (isalnum(*targsptr)||*targsptr=='_') tok+=*targsptr;
      else  
	{
	  while (isspace(*targsptr)) targsptr++;/* skip any spaces */
	  if (*targsptr=='<')  /* skip over stuff between <..> */
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
	      r+=tok+*targsptr;
	    }
	  else if (strchr(">,",*targsptr))  r+=tok+*targsptr;
	  else if (isalnum(*targsptr)||*targsptr=='_') 
	    targsptr--;  /* rewind ptr so next loop starts token */
	  tok.erase();
	}
    }
  return r;
}

void deal_with_commas(string& classname)
{
  if (classname.find(',')!=classname.npos) 
    {       // must deal with typenames containing ','
      static int cnt=0;
      char tname[20];
      sprintf(tname,"___ecotif%-10d",cnt++);
      printf("typedef %s %s;\n",classname.c_str(),tname);
      classname=tname;
    }
}

void insert_friend(tokeninput& input, string prefix, string targs, 
		   int is_struct)
{
  string classname = prefix + input.token;
  input.nexttok();
  if (input.token=="<") /* template specialisation */
    classname += get_template_args(input);
  if (strchr(":{",input.token[0]))
    {
      for (;input.token!="{"; input.nexttok() );
      /* structs are complicated by the possibility of protected and
         private members - if none, then no friend statement is
         necessary */
      if (is_struct)
	{	
	  int numbraces=0;
	  for (input.nexttok(); 
	       input.token!="}" || numbraces>0; 
	       input.nexttok() )
	    {
	      if (input.token=="{") numbraces++; 
	      if (input.token=="}") numbraces--;
	      if (input.token==":" && 
		  (input.lasttoken=="protected" || input.lasttoken=="private"))
		goto really_insert_friend;
	    }
	  return;   
	  /* struct has no protected or private members, nothing to do */
	}

    really_insert_friend:
      puts("");
      /* check if typename is mapped to something else */
      if (targ_map.count(classname)) classname=targ_map[classname];
      deal_with_commas(classname);
      if (targs=="")
	printf("CLASSDESC_ACCESS(%s);\n",classname.c_str());
      else
	printf("CLASSDESC_ACCESS_TEMPLATE(%s);\n",classname.c_str());
      /* emit a CLASSDESC_FDEF to handle g++'s requirements */
      if (targs.size())
	{
	  /* advance to end of class definition */
	  if (input.token!="}") gobble_delimited(input,"{","}");
	  printf("\n#define CLASSDESC_TDEC template%s\n",targs.c_str());
	  if (classname.find('<')==classname.npos) 
	    classname += extract_targs(targs);
	  deal_with_commas(classname);
	  printf("CLASSDESC_FDEF(%s);\n",classname.c_str());
	}
    }
}


int main(int argc, char* argv[])
{
  
  tokeninput input(stdin,stdout);

  string targs;

  while (argc>1 && argv[1][0]=='-')  /* process options */
    {
//      if (strcmp(argv[1],"-targ_map")==0)  /* read in targ map */
//	{
//	  ifstream targmapfile(argv[2]);
//	  string guessed_type, real_type;
//	  while (targmapfile>>guessed_type>>real_type)
//	    targ_map[guessed_type]=real_type;	
//	  argc-=2; argv+=2;
//	}
    }
  

  try
    {
      for (;;)
	{
	  if (input.token=="template")
	    {
	      input.nexttok();
	      if (input.token!="<") /* g++'s explicit instantiation syntax */
		{ /* we must parse until end of declaration */
		  for (int nbraces=0; nbraces>0 || input.token!=";"; 
		       input.nexttok())
		    {
		      if (input.token=="{") nbraces++;
		      if (input.token=="}") nbraces--;
		    }
		  continue;
		}
	      targs=get_template_args(input);
	      
	      if (input.token!="class" && input.token!="struct") 
		{
		  targs.erase();  /* not a template class */
		  continue;
		}
	    }
 	  if ((input.token=="class" || input.token=="struct") &&
	      input.lasttoken!="friend") 
	    {
	      input.nexttok();
#if (defined(__osf__) && !defined(__GNUC__))
	      /* handle a rogue wave hack for Windoze support - aarrgh! */
	      if (input.token.substr(0,12)=="_RWSTDExport") input.nexttok();
#endif
	      if (isalpha(input.token[0]))  /* named class */
		insert_friend(input,"",targs,input.lasttoken=="struct");
	      targs.erase();
	    }
	  input.nexttok();
	}
    }

  catch(tokeninput::eof) {}

  return 0;
}


