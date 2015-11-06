/*
  @copyright Russell Standish 2000-2013
  @author Russell Standish
  This file is part of Classdesc

  Open source licensed under the MIT license. See LICENSE for details.
*/

#include <stdio.h>
#include <ctype.h>
#include <stdio.h>
#include <iostream>
#include <string.h>

class mark_t;

#ifdef _MSC_VER
#define USE_FSEEK
#endif


class tokeninput
{
  FILE* inputstream;
  FILE* outputstream;
  int c;
  int lineno;
  char getnextc() 
  {
    c=fgetc(inputstream); 
    switch (c)
      {
      case '\n':
        //printf("%d\n",lineno);  /* for debugging purposes */
        lineno++;
        break;

      case '\\': /* check for and remove any line continuations */
        c=fgetc(inputstream); 
        if (c=='\n') 
          {
            c=fgetc(inputstream); 
            if (outputstream) fprintf(outputstream,"\\\n");
          }
        else {ungetc(c,inputstream); c='\\';}
        break;

      case '[':
        c=fgetc(inputstream); 
        if (c=='[') // attribute leadin - discard attribute
          {
            char lc=c;
            while ((c=fgetc(inputstream)) != ']' && lc!=']') lc=c;
            getnextc();
          }
        else
          {ungetc(c,inputstream); c='[';}
        break;
      case EOF:
        throw eof();
        break;
      }
    if (outputstream &&!feof(inputstream)) 
      fputc(c,outputstream); 
    return c;
  }
public:
  struct eof {};  /* signal end of input */
  string token, lasttoken;
  tokeninput(FILE* in, FILE* out=NULL) 
  {inputstream=in; outputstream=out; getnextc(); lineno=1;}

  int line() const {return lineno;}

  /* support for marking and resetting stream */
  class mark_t;
  void operator=(mark_t& x);
  int operator>(mark_t& x); /* returns whether stream is beyond mark */
  friend class tokeninput::mark_t ;

  void nexttok()
  {

    token.swap(lasttoken); 

  nexttok_again:
    token.erase();
    if (feof(inputstream)) throw eof();

    /* skip white space and # control lines */
    while (isspace(c)) getnextc();
    while (c=='#')
      {
        while (c!='\n' && !feof(inputstream)) 
          {
            if (c!='\r') token+=c; //ignore '\r' (DOS files)
            if (c=='/' && getnextc()=='*') /* strip comments */
              for (char l=getnextc(); l!='*' || c!='/'; l=c, getnextc());
            getnextc();
          }
        if (token.find("#pragma")==0) 
          return; /* parse any pragma omits appearing on stdin */
        else
          token.erase();
        while (isspace(c)) getnextc();
      }
    if (feof(inputstream)) throw eof();

    if (c=='"' || c=='\'')   /* handle strings and chars */
      {
        int escape=0;
        char terminal=c;
        do
          {
            token+=c;
            escape=c=='\\' && !escape;
            getnextc();
          }
        while (c!=terminal || escape);
        token+=c;   /* include terminal " */
        getnextc();
      }
    else if (strchr("#\\!@$~(){}[]:;,.?%*|-+=<>^&/",c)) 
      /* operator tokens - 
         treat # and \ as operators, although they're stripped from 
         preprocessed code */
      {
        char lc;
        token+=lc=c;
        getnextc();
        if (c=='=' && !strchr("#\\!@$~(){}[]:;,.?",lc)) 
          { /* compound assignment operator */ 
            token+=c;
            getnextc();
          }
        else
          switch (lc)
            {
            case '<': case '>': case '|': case '&': 
            case '+': case '-': case ':':
              if (c==lc)    /* double symbol */ 
                {
                  token+=c;
                  getnextc();
                  if ((lc=='<' || lc=='>') && c=='=') /* <<= & >>= */
                    {
                      token+=c;
                      getnextc();
                    }
                }
              else if (lc=='-' && c=='>')   /* -> and ->* operators */
                {
                  token+=c;
                  getnextc();
                  if (c=='*')
                    {
                      token+=c;
                      getnextc();
                    }
                }           
              break;

              /* consider the pair [] and () as single token, 
                 for use with operator */ 
              
            case '[':
              if (c==']')
                {
                  token+=c;
                  getnextc();
                }
             break;

            case '(':
              if (c==')')
                {
                  token+=c;
                  getnextc();
                }
              break;
              
            case '/':  /* check for comments, and remove from input stream */
              if (c=='*') 
                {
                  lc=getnextc();
                  while (lc!='*' || c!='/')
                    {
                      lc=c;
                      getnextc();
                    }
                  getnextc();  /* load up next character from input stream */
                  goto nexttok_again; /* get next token */
                }
              else if (c=='/')
                {
                  while (c!='\n') getnextc();
                  goto nexttok_again; /* get next token */              
                }
              break;
            }
      }
    else if (!isalnum(c) && c!='_') 
      {
        getnextc();
        goto nexttok_again; /* skip non 'C' characters */
      }
    else 
      {
        while (isalnum(c) || c=='_') 
          {
            token+=c;
            getnextc();
          }
      }
  }
};

/* support for marking and resetting stream */
class tokeninput::mark_t 
{
  friend void tokeninput::operator=(mark_t& x);
  friend int tokeninput::operator>(mark_t& x);
  fpos_t fp; 
  long offset;
  tokeninput tokinp;
public:
  mark_t(tokeninput& x): tokinp(x) 
  {
    //    if (fgetpos(x.inputstream,&fp)) throw eof();
    // Some systems (eg CYGWIN) have trouble with fgetpos/fsetpos ?
    fgetpos(x.inputstream,&fp);

    offset=ftell(x.inputstream);
  }
};

void tokeninput::operator=(mark_t& x)
{ 
  *this=x.tokinp; 
#ifdef USE_FSEEK /* some OSes have buggy fsetpos routines */
  if (fseek(inputstream,x.offset,SEEK_SET)) throw eof();
#else
  if (fsetpos(inputstream,&(x.fp))) throw eof();
#endif
}

int tokeninput::operator>(mark_t& x)
{return ftell(inputstream) > x.offset;}

/* redefine isalpha so that '_' is considered an alphabetic char */
#undef isalpha
inline int isalpha(char x)
{return x=='_' || (x>='a' && x<='z') || (x>='A' && x<= 'Z');}

#include <map>
#include <set>
template <class K, class T> class hash_map: public map<K,T> {};
template <class K> class hash_set: public set<K> {};


string gobble_delimited(tokeninput& input, const char *left,
                      const char *right)
{ string argList = input.lasttoken;
  argList += " " + input.token + " ";
  int delim_count=0;
  string tmp=(string)left+right;
  if (input.token==tmp) return argList;  /* catch the () and [] token cases */
  for (input.nexttok(); input.token!=right || delim_count>0; 
       input.nexttok())
    { 
      argList += input.token + " ";
      if (input.token==left)  delim_count++;
      if (input.token==right)  delim_count--;
      if (input.token==">>" && string(left)=="<") 
        {
          delim_count-=2;
          if (delim_count<=0) break;
        }
    }
   return argList;
}

/* grab arguments to template */
string get_template_args(tokeninput& input)
{
  string targs;
  size_t angle_count=0;       
  do
    {
      /* strip out default arguments */
      if (input.token[0]=='=')
        while (!strchr(">,",input.token[0]))
          {
            if (input.token[0]=='<') gobble_delimited(input,"<",">");
            input.nexttok();
          }
      targs += input.token;
      if (input.token!=".") targs+=" ";
      if (input.token[0]=='<') angle_count+=input.token.length();
      else if (input.token[0]=='>') 
        angle_count-=input.token.length();
      input.nexttok();
    }
  while (angle_count>0);
  return targs;
}


