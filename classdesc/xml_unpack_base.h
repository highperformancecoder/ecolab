/*
  @copyright Russell Standish 2000-2013
  @author Russell Standish
  This file is part of Classdesc

  Open source licensed under the MIT license. See LICENSE for details.
*/

/**\file
\brief XML deserialisation descriptor
*/

#ifndef XML_UNPACK_BASE_H
#define XML_UNPACK_BASE_H
#include <map>
#include <iostream>
#include <sstream>
#include <fstream>
#include <limits>
#include <cstdlib>
#include <cctype>

#include "xml_common.h"
#include "classdesc.h"
// for xml_unpack_t serialisation support
#include "pack_base.h"
#include "pack_stl.h"

namespace classdesc_access
{
  template <class T> struct access_pack;
  template <class T> struct access_unpack;
}

namespace classdesc
{

  namespace 
  {
    /// return true if s is a string of whitespaces
    inline bool isspace(std::string s)
    {
      if (s.empty()) return false;
      for (size_t i=0; i<s.size(); i++)
        if (!std::isspace(s[i]))
          return false;
      return true;
    }
  }

  // for remove() below
  inline bool Isspace(char c) {return std::isspace(c)!=0;}

#ifdef _CLASSDESC
#pragma omit pack classdesc::XMLtoken
#pragma omit pack classdesc::xml_pack_error
#pragma omit unpack classdesc::XMLtoken
#pragma omit unpack classdesc::xml_pack_error
#pragma omit xml_pack classdesc::XMLtoken
#pragma omit xml_pack classdesc::xml_pack_error
#pragma omit xml_unpack classdesc::XMLtoken
#pragma omit xml_unpack classdesc::xml_pack_error
#pragma omit json_pack classdesc::xml_pack_error
#pragma omit json_unpack classdesc::xml_pack_error
#pragma omit dump classdesc::xml_pack_error
#endif

  class xml_pack_error : public exception 
  {
    std::string msg;
  public:
    xml_pack_error(const char *s): msg("xml_pack:") {msg+=s;}
    xml_pack_error(std::string s): msg("xml_pack:") {msg+=s;}
    virtual ~xml_pack_error() throw() {}
    virtual const char* what() const throw() {return msg.c_str();}
  };

  // character accessor functions: istream and FILE* defined here.
  inline bool get(std::istream& i, char& c) {return i.get(c).good();}
  inline bool get(FILE*& i, char& c)  
  {int cc=fgetc(i); c=char(cc); return cc!=EOF;}
  inline void unget(std::istream& i, char c)  {i.putback(c);}
  inline void unget(FILE*& i, char c) {ungetc(c,i);}

  template <class Stream>
  class XMLtoken
  {
    Stream& i;
    char nexttok;

    // basic I/O operations
    bool get(char& c) {return classdesc::get(i,c);}
    void unget(char c) {classdesc::unget(i,c);}
    /// throw error on EOF
    char getNoEOF() {
      char r;
      if (!get(r)) throw xml_pack_error("invalid XML");
      return r;
    }
    
    void gobble_comment();
    void gobble_whitespace() {
      char c;
      bool notEof=get(c); 
      while (notEof && std::isspace(c)) notEof=get(c);
      if (notEof) unget(c);
    }
    char parse_entity();
    std::string retval(char c, const std::string& tok);
  public:
    XMLtoken(Stream& i): i(i), nexttok('\0') {}
    std::string token();
    std::string tokenNoEOF() {
      std::string tok=token();
      if (tok.empty()) throw xml_pack_error("XML token expected");
      else return tok;
    }
  };

  template <class Stream> 
  void XMLtoken<Stream>::gobble_comment()
  {
    int level=1;
    bool inString=false;
    char c;
    while (level)
      {
        c=getNoEOF();
        if (c=='"') inString=!inString;
        if (inString) continue;
        switch(c)
          {
          case '<': level++; break;
          case '>': level--; break;
          }
      }
    gobble_whitespace();
  }
  
  template <class Stream> 
  char XMLtoken<Stream>::parse_entity()
  {
    std::string name;
    char c;
    for (c=getNoEOF(); c!=';'; c=getNoEOF())
      name+=c;
    if (name=="amp") return '&';
    if (name=="lt") return '<';
    if (name=="gt") return '>';
    if (name=="quot") return '"';
    if (name=="apos") return '\'';
    const char* cname=name.c_str();
    if (cname[0]=='#') //character code supplied
      {
        if (cname[1]=='x') //is hex
          {
            //TODO - should we be doing this all in wide chars?
            long r=std::strtol(cname+2,NULL,16);
            if (r>std::numeric_limits<char>::max() || r<std::numeric_limits<char>::min())
              throw xml_pack_error("XML numeric character reference out of range");
            return char(r);
          }
        else
          {
            //TODO - should we be doing this all in wide chars?
            long r=std::strtol(cname+1,NULL,10);
            if (r>std::numeric_limits<char>::max() || r<std::numeric_limits<char>::min())
              throw xml_pack_error("XML numeric character reference out of range");
            return char(r);
          }
      }
    // not sure what to do about user defined entities - throw, or issue a warning
    throw xml_pack_error("Unidentified entity encountered");
  }        

  // This allows a previous token to be return when a single character token in parsed
  template <class Stream>
  std::string XMLtoken<Stream>::retval(char c, const std::string& tok)
  {
    if (tok.empty())
      {
        nexttok='\0';
        switch (c)
          {
          case '/': return "</";
          case '\\': return "/>";
          default: return std::string(1,c);
          }
      }
    else
      {
        nexttok=c;
        return tok;
      }
  }

  template <class Stream> 
  std::string XMLtoken<Stream>::token()
  {
    std::string tok;
    char c;

    // handle any tokens left over from previous parsing
    if (nexttok)
      return retval(nexttok,tok);

    while (get(c))
      {
        // return white space as a separate token
        if (std::isspace(c)) return retval(c,tok);

        switch (c)
          {
          case '&':
            tok+=parse_entity();
            continue;
          case '\'':
          case '"':    //process string literal as single token
            {
              char term=c;
              while ((c=getNoEOF())!=term)
                if (c=='&')
                  tok+=parse_entity();
                else
                  tok+=c;
              return tok;
            }
          case '<':
            c=getNoEOF();
            switch (c)
              {
              case '?':
              case '!': //we have a comment or XML declaration, which we ignore
                gobble_comment(); continue;
              case '/': //we have begin end tag token
                return  retval('/',tok);
              default:
                {
                  unget(c);
                  return retval('<',tok);
                }
              }
          case '/':
            if ((c=getNoEOF())=='>') //we have end empty tag token
              return retval('\\',tok);
            else  //TODO is a / in the middle of a token acceptible XML?
              {
                tok+='/';
                unget(c);
                break;
              }
          case '>': 
          case '=':
            return retval(c,tok);
          default:
            tok+=c;
          }
      }
    if (tok.empty())
      return tok;  //empty token returned on end of file
    else
      throw xml_pack_error("XML file truncated?");
  }

  /**
     XML deserialisation object
  */
  class xml_unpack_t
  {
    std::map<std::string,std::string> contentMap;
    std::map<std::string,unsigned> tokenCount;

    void checkKey(const std::string& key)
    {
      if (missingException && !contentMap.count(key)) 
        throw xml_pack_error(key+" is missing in XML data stream");
    }

    // add "#0" to components if no # label present
    std::string addHashNoughts(const std::string& key)
    {
      std::string r;
      std::string::size_type start=0, end;
      bool hash_read=false;
      for (end=0; end<=key.length(); end++)
        if (key[end]=='#') 
          hash_read=true;
        else if (key[end]=='.')
          {
            if (hash_read)
              hash_read=false;
            else // no hash read, so insert "#0"
              {
                r+=key.substr(start,end-start)+"#0";
                start=end;
              }
          }
      r+=key.substr(start,end-start);
      if (!hash_read)
        r+="#0";
      return r;
    }

    friend class classdesc_access::access_pack<xml_unpack_t>;
    friend class classdesc_access::access_unpack<xml_unpack_t>;
  public:
    /** set this to true if you wish an exception to be thrown if data is missing
        from the XML stream */
    bool missingException;  
    xml_unpack_t(): missingException(false) {}
    xml_unpack_t(const char* fname): missingException(false) {std::ifstream i(fname); parse(i);}
    template <class Stream> xml_unpack_t(Stream& i): missingException(false) {parse(i);}
    template <class Stream> void process_attribute(XMLtoken<Stream>& i, const std::string& scope);
    template <class Stream> void parse(Stream& i);
    template <class Stream> void parse(XMLtoken<Stream>& stream, const std::string& scope);

    ///dump XML contents for debugging 
    void printContentMap() const {
      for (std::map<std::string,std::string>::const_iterator i=contentMap.begin();
           i!=contentMap.end(); i++)
        std::cout << "["<<i->first<<"]="<<i->second<<std::endl;
      std::cout << std::endl;
      for (std::map<std::string,unsigned>::const_iterator i=tokenCount.begin();
           i!=tokenCount.end(); i++)
        std::cout << "Count["<<i->first<<"]="<<i->second<<std::endl;
    }
    ///simple data type deserialisation
    template <class T> void unpack(std::string key, T& var) {
      key=addHashNoughts(key); checkKey(key); 
      std::map<std::string,std::string>::const_iterator it=contentMap.find(key);
      if (it != contentMap.end()) {
          std::istringstream s(it->second);  
          s>>var;
        }
    }
    // specialisation to handle boolean values
    void unpack(std::string key, bool& var) {
      key=addHashNoughts(key); checkKey(key);
      std::map<std::string,std::string>::const_iterator it=contentMap.find(key);
      if (it != contentMap.end())
        {
          std::string val=it->second;
          // strip any white space
          val.erase(remove_if(val.begin(), val.end(), Isspace), val.end());
          for (size_t i=0; i<val.length(); ++i) val[i]=char(tolower(val[i]));
          var = val=="1" || val=="t" || val=="true"|| val=="y"|| val=="yes" || 
            val=="on";
        }
    }
    /// string deserialisation 
    void unpack(std::string key, std::string& var) {
      key=addHashNoughts(key); checkKey(key);  
      std::map<std::string,std::string>::const_iterator it=contentMap.find(key);
      if (it != contentMap.end())
        var=it->second; 
    }
    /// checks for existence of token unpacked from XML stream
    bool exists(const std::string& key) {return count(key)>0;}
    /// returns number of array elements with prefix key
    size_t count(std::string key) {
      key=addHashNoughts(key);
      key=key.substr(0,key.rfind('#')); //strip final # marker
      return tokenCount[key];
    }
    void clear() {contentMap.clear(); tokenCount.clear();}
  };

  /**
     parse XML attribute string from XML stream
  */
  template <class Stream> 
  void xml_unpack_t::process_attribute(XMLtoken<Stream>& stream, const std::string& scope)
  {
    std::string tok; 
    while (isspace(tok=stream.tokenNoEOF()));
    if (tok!="=") throw xml_pack_error("ill-formed attribute");
    while (isspace(tok=stream.tokenNoEOF()));
    contentMap[scope]=tok;
  }

  /**
     parse an input XML file, into the database
     \a Stream is either std::istream or a FILE*
  */
  template <class Stream> 
  void xml_unpack_t::parse(Stream& i) 
  {
    XMLtoken<Stream> stream(i);
    std::string tok;    
    while (isspace(tok=stream.token()));
    if (tok.empty()) return;
    if (tok=="<") 
      parse(stream,stream.tokenNoEOF());
    else
      throw xml_pack_error("no root element found");
  }

  template <class Stream> 
  void xml_unpack_t::parse(XMLtoken<Stream>& stream, const std::string& scope) 
  {
    //count the number of times this token has been read, and append this to database key
    std::string scope_idx=idx(scope,tokenCount[scope]++);

    std::string tok;
    //parse attributes
    for (tok=stream.tokenNoEOF(); tok!=">" && tok!="/>"; tok=stream.tokenNoEOF())
      if (!isspace(tok)) process_attribute(stream, scope_idx+"."+tok);
    
    if (tok=="/>") return;

    //parse content. We assume element is either just content, or just has child elements
    std::string content;
    for (tok=stream.tokenNoEOF(); tok!="</"; tok=stream.tokenNoEOF())
      if (tok=="<")
        parse(stream,scope_idx+"."+stream.tokenNoEOF()); //parse child element
      else 
        content+=tok;

    if (content.size())
      contentMap[scope_idx]=content; //override content (to handle masked private members)

    // finish parsing end tag
    tok=stream.tokenNoEOF();
    if (scope.length()-scope.rfind(tok)!=tok.length()) //tok matches last part of scope
      throw xml_pack_error("unexpected end tag");
    for (; tok!=">"; tok=stream.tokenNoEOF()); //skip rest of end tag
  }


}

namespace classdesc_access
{
  template <class T> struct access_xml_unpack;
}

template <class T> void xml_unpack(classdesc::xml_unpack_t&,const classdesc::string&,T&);

template <class T> classdesc::xml_unpack_t& operator>>(classdesc::xml_unpack_t& t, T& a);

  /*
    base type implementations
  */
namespace classdesc
{
  template <class T>
  void xml_unpack_onbase(xml_unpack_t& x,const string& d,T& a)
  {::xml_unpack(x,d+basename<T>(),a);}

  template <class T>
  typename enable_if<is_fundamental<T>, void>::T
  xml_unpackp(xml_unpack_t& x,const string& d,T& a)
  {x.unpack(d,a);}
}

using classdesc::xml_unpack_onbase;

/* now define the array version  */
#include <stdarg.h>

  template <class T> void xml_unpack(classdesc::xml_unpack_t& x,const classdesc::string& d,classdesc::is_array ia,
                                     T& a, int dims,size_t ncopies,...) 
  {
    va_list ap;
    va_start(ap,ncopies);
    for (int i=1; i<dims; i++) ncopies*=va_arg(ap,int); //assume that 2 and higher D arrays dimensions are int
    va_end(ap);

    classdesc::string eName=classdesc::typeName<T>().c_str();
    // strip leading namespace and qualifiers
    const char *e=eName.c_str()+eName.length();
    while (e!=eName.c_str() && *(e-1)!=' ' && *(e-1)!=':') e--;

    for (size_t i=0; i<ncopies; i++) 
      xml_unpack(x,classdesc::idx(d+"."+e,i),(&a)[i]);
  }

//Enum_handles have reference semantics
template <class T> void xml_unpack(classdesc::xml_unpack_t& x,
                                    const classdesc::string& d,
                                    classdesc::Enum_handle<T> arg)
{
  std::string tmp;
  xml_unpack(x,d,tmp);
  // remove extraneous white space
  int (*isspace)(int)=std::isspace;
  std::string::iterator end=std::remove_if(tmp.begin(),tmp.end(),isspace);
  arg=tmp.substr(0,end-tmp.begin());
}

template <class T1, class T2>
void xml_unpack(classdesc::xml_unpack_t& x, const classdesc::string& d, 
                std::pair<T1,T2>& arg)
{
  xml_unpack(x,d+".first",arg.first);
  xml_unpack(x,d+".second",arg.second);
}

namespace classdesc
{
  template <class T> typename
  enable_if<is_sequence<T>, void>::T
  xml_unpackp(xml_unpack_t& x, const string& d, T& arg, dummy<1> dum=0)
  {
    string eName=typeName<typename T::value_type>().c_str();
    eName=eName.substr(0,eName.find('<')); //trim off any template args
    // strip leading namespace and qualifiers
    const char *e=eName.c_str()+eName.length();
    while (e!=eName.c_str() && *(e-1)!=' ' && *(e-1)!=':') e--;

    arg.clear();
    for (size_t i=0; i<x.count(d+"."+e); ++i) 
      {
        typename T::value_type v;
        ::xml_unpack(x,classdesc::idx(d+"."+e,i),v);
        arg.push_back(v);
      }
  }

  template <class T> typename
  enable_if<is_associative_container<T>, void>::T
  xml_unpackp(xml_unpack_t& x, const string& d, T& arg, dummy<2> dum=0)
  {
    string eName=typeName<typename T::value_type>().c_str();
    eName=eName.substr(0,eName.find('<')); //trim off any template args
    // strip leading namespace and qualifiers
    const char *e=eName.c_str()+eName.length();
    while (e!=eName.c_str() && *(e-1)!=' ' && *(e-1)!=':') e--;

    arg.clear();
    for (size_t i=0; i<x.count(d+"."+e); ++i) 
      {
        typename NonConstKeyValueType<typename T::value_type>::T v;
        ::xml_unpack(x,idx(d+"."+e,i),v);
        arg.insert(v);
      }
  }
}

/* member functions */
template<class C, class T>
void xml_unpack(classdesc::xml_unpack_t& targ, const classdesc::string& desc, C& c, T arg) {} 

template<class T>
void xml_unpack(classdesc::xml_unpack_t& targ, const classdesc::string& desc, 
                classdesc::is_const_static i, T arg) 
{} 

template<class T, class U>
void xml_unpack(classdesc::xml_unpack_t& targ, const classdesc::string& desc, 
                classdesc::is_const_static i, const T&, U) {} 

template<class T>
void xml_unpack(classdesc::xml_unpack_t& targ, const classdesc::string& desc, 
                classdesc::Exclude<T>&) {} 

namespace classdesc
{
  template<class T>
  void xml_unpack(xml_unpack_t& targ, const string& desc, is_graphnode, T&)
  {
    throw exception("xml_unpack of arbitrary graphs not supported");
  } 


}
#endif
