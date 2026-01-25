/*
  @copyright Russell Standish 2000-2013
  @author Russell Standish
  This file is part of EcoLab

  Open source licensed under the MIT license. See LICENSE for details.
*/

/**\file
\brief persistent hashmap
*/
#ifndef CACHEDBM_H
#define  CACHEDBM_H

#include "pack_base.h"
#include "classdesc_access.h"
#include "error.h"
#include "omp_rw_lock.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <limits>
#include <memory>
#include <string>
#include <utility>

namespace ecolab
{
  class Datum: public classdesc::xdr_pack
  {
  public:
    /** used for loading data into Datum. assign_ptr assumes pointer has been malloc'd */
    //  void assign_ptr(void *d, size_t sz) {Realloc(data,0); data=(char*)d; size=sz;}
    void copy_ptr(void *d, size_t sz) {packraw((char*)d,sz);}
    Datum(): classdesc::xdr_pack() {}
    Datum(const Datum& x) {packraw(x.data(),x.size());}
    Datum& operator<<(const Datum& x) {packraw(x.data(),x.size()); return *this;}
    const Datum& operator>>(Datum& x) {x.packraw(data(),size()); return *this;}
    template <class T> 
    Datum& operator<<(const T& x) {pack(*this,"",const_cast<T&>(x)); return *this;}
    template <class T> 
    Datum& operator>>(T& x) {unpack(*this,"",x); return *this;}
    Datum& operator=(const Datum& x) {reseti(); packraw(x.data(),x.size()); return *this;}
    template <class T> Datum& operator=(const T& x){reseti(); pack(*this,"",const_cast<T&>(x)); return *this;}
    template <class T> operator T() {T x;  reseto(); unpack(*this,"",x); return x;}
  };

}

// must be in global namespace due to a classdesc bug
class Db
{
  void *db;   // opaque pointer to database object (be it ndbm or Berkley)
  void *dbc;  // opaque pointer to database cursor (BDB)
  Db(const Db& x);
  void operator=(const Db& x);
public:
  enum rw {read,write}; ///< open database readonly, or writable
  
  void open(const char* filename,rw readwrite);
  void close();
  void flush(); ///< flush to disk
  bool fetch(const ecolab::Datum& key, ecolab::Datum& val) const;
  void store(const ecolab::Datum& key, const ecolab::Datum& val);
  void del(const ecolab::Datum& key);
  /** for iterating over database */
  bool firstkey(ecolab::Datum& key, ecolab::Datum& val) const;
  bool nextkey(ecolab::Datum& key, ecolab::Datum& val) const;
  bool opened() const {return db!=NULL;}
  Db(const char* f,rw rdwr): db(NULL), dbc(NULL) {open(f,rdwr);}
  ~Db() {close();}
};

namespace ecolab
{
  template <class key, class val> struct base_map: public std::map<key,val>
  {
    void set(const key& k, Datum& v) 
    {v>>std::map<key,val>::operator[](k);}
    val& get(const key& k) {return std::map<key,val>::operator[](k);}
    //virtual val& operator[] (const key& k)=0; //used to access cachedDBM::operator[] in TCL_obj()
    virtual ~base_map() {}
  };

  template <class U, class V>
  struct sortPair
  {
    bool operator()(const std::pair<U,V>& x, const std::pair<U,V>& y) {return x.second<y.second;}
  };

  class limited_set: public std::set<int>
  {
    size_t limit;
    CLASSDESC_ACCESS(limited_set);
  public:
    limited_set(size_t limit): limit(limit) {}
    void insert(int x) {
      if (size() < limit || x>*begin()) std::set<int>::insert(x);
      if (size() >= limit) erase(begin());
    }
  };

  /// iterator type for iterating over keys
  template <class key, class val>
  class KeyValueIterator
  {
    std::shared_ptr<Db> db;
    std::pair<key,val> keyValue; 
    void getKV(bool (Db::*op)(Datum&, Datum&) const)
    {
      Datum k,v;
      if ((db.get()->*op)(k,v))
        db.reset();
      else
        {
          k>>keyValue.first;
          v>>keyValue.second;
        }
    }
  public:
    /// initialises to an end() iterator
    KeyValueIterator() {}
    /// initialises to a begin() iterator of database \a fname
    KeyValueIterator(const std::string& fname): db(new Db(fname.c_str(), Db::read)) 
    {getKV(&Db::firstkey);}
    KeyValueIterator& operator++() {getKV(&Db::nextkey); return *this;}
    // iterator comparison is undefined when referring to different
    // databases, and keys are unique within a given database, so we
    // can use comparisons of keys
    bool operator==(const KeyValueIterator& x) const
    // TODO check whether operator< should be used here???
    {return (!db && !x.db) || (db && x.db && keyValue.first==x.keyValue.first);}
    bool operator!=(const KeyValueIterator& x) const
    {return !operator==(x);}
    const std::pair<key, val> operator*() const {return keyValue;}
    const std::pair<key, val>* operator->() const {return &keyValue;}
  };

  template <class key, class val>
  class KeyIterator: public KeyValueIterator<key,val>
  {
  public:
    KeyIterator() {}
    KeyIterator(const std::string& fname): KeyValueIterator<key,val>(fname) {}
    const key operator*() const {return KeyValueIterator<key,val>::operator*().first;}
    const key* operator->() const 
    {return &KeyValueIterator<key,val>::operator*().first;}
  };

  template <class C>
  struct Keys
  {
    C& _this;
    Keys(C& _this): _this(_this) {}
    typename C::KeyIterator begin()  
    {_this.commit(); return typename C::KeyIterator(_this.filename());}
    typename C::KeyIterator end() const {return typename C::KeyIterator();}
  };
  template <class C>
  struct ConstKeys
  {
    const C& _this;
    ConstKeys(const C& _this): _this(_this) {}
    typename C::KeyIterator begin() const {return typename C::KeyIterator(_this.filename());}
    typename C::KeyIterator end() const {return typename C::KeyIterator();}
  };
  
  /* make the main class a base class in order to derive a special case for 
     strings */
  /// implementation of cacheDBM common to all specialisations
  template<class key, class val>
  class cachedDBM_base : protected base_map<key,val>
  {
    typedef base_map<key,val> Base;
    std::shared_ptr<Db> db;
    bool readonly;
    classdesc::string m_filename;
    typedef std::map<key,size_t> TSMap;
    TSMap timestamp;
    size_t ts;
    RWlock rwl;
    mutable bool last;
  public:
    size_t max_elem;   /* limit number of elements to this value */
    cachedDBM_base(): ts(1), max_elem(std::numeric_limits<int>::max()) {}
    void init(const char *fname, char mode='w')
    {
      db.reset(new Db((char*)fname, (mode=='w')? Db::write: Db::read)); 
      readonly=mode=='r';
      if (!db->opened()) throw error("DBM file %s open failed",fname);
      m_filename=fname;
    }
    //  void init(const char *fname, char *mode="w") {init(fname,mode[0]);}
    void close() {if (db) {commit(); db->close(); db.reset();} clear();}
    const std::string& filename() const {return m_filename;}
    ~cachedDBM_base() {close();}
    bool opened() const {return bool(db);}
    bool load(const key& k)
    {
      Datum dk, vv; 
      dk<<k;
      write_lock w(rwl);
      if (db && !db->fetch(dk,vv)) 
        {
          this->set(k,vv);
          timestamp[k] = ts++;
          return true;
        }
      return false;
    }

    /// returns true if key is in data base or added with [] operator
    bool key_exists(const key& k) const
    {
      if (this->count(k)) return true;
      if (db) 
        return const_cast<cachedDBM_base*>(this)->load(k);
      return false;
    }
    val& operator[] (const key& k) 
    {
      read_lock r(rwl);
      if (Base::size()>=max_elem) {commit();} /* do a simple purge of database */
      if (!key_exists(k)) //read data into memory if it exists
        {
          write_lock w(rwl);
          Base::insert(std::make_pair(k,val()));
          timestamp[k]=ts++;
        }
#ifdef _OPENMP
#pragma omp atomic
#endif
      ts++;
      timestamp[k]=ts; //not so important if we pick up a simultaneous timestamp
      return Base::get(k);
    }

    // accessor routines, until we can make it easy to add operator[] to the python object
    const val& elem(const key& k) {return operator[](k);}
    const val& elem(const key& k, const val& v) {return operator[](k)=v;}
    
    /// number of elements in cache
    size_t cacheSize() const {return Base::size();}

    /// clear the cache
    void clear() {Base::clear();}

    /// write any changes out to the file, and clear some of the cache 
    void commit()
    {
      write_lock w(rwl);
      typename Base::iterator i, j;
      Datum k, v;
      size_t sum_ts=0;
      for (typename TSMap::const_iterator t=timestamp.begin(); t!=timestamp.end(); ++t)
        sum_ts+=t->second;
      size_t cut_time=sum_ts/2;
      limited_set ls(cut_time);
      for (i=Base::begin(); i!=Base::end(); ) 
        {
          k=i->first; v=i->second;
          if (db)
            {
              if (readonly && db->fetch(k,v))
                {
                  ++i; 
                  continue; // do not erase element not in database
                }
              else if (!readonly  && v.data()) 
                db->store(k,v);
            }
          
          typename TSMap::iterator ts=timestamp.find(i->first);
          j=i; i++; //save current iter for later erase
          // erase map element if timestamp earlier than cut_time.
          if (ts->second<cut_time) 
            {
              Base::erase(j);
              timestamp.erase(ts);
            }
        }
      if (db) db->flush();
      // rebase timestamps to cut_time
      ts-=cut_time;
      for (typename TSMap::iterator t=timestamp.begin(); t!=timestamp.end(); ++t)
        t->second-=cut_time;

    }

    /// delete entry associated with key \a k
    void del(key k)   
    {
      if (db && db->opened())
        {
          write_lock w(rwl);
          Datum dk; 
          dk=k; 
          db->del(dk);
        }
      Base::erase(k); 
    }
    /**
       \brief obtain first key for iteration through database.

       iterators are inherently not thread safe
    */
    key firstkey() const
    {
      key k; 
      if (db)
        {
          Datum kk,vv;
          if (!(last=db->firstkey(kk,vv))) 
            kk>>k;
        }
      return k;
    }
    /// advance to next key in database
    key nextkey() const
    {
      key k; 
      if (db)
        {
          Datum kk,vv;
          if (!(last=db->nextkey(kk,vv))) 
            kk>>k; 
        }
      return k;
    }
    /// true if no further keys remain when iterating
    bool eof() const {return last || !db;}

    void packConst(classdesc::pack_t& b) const {
      if (opened()) {
        ::pack(b,"",m_filename);
        ::pack(b,"",readonly);
        ::pack(b,"",static_cast<const Base&>(*this));
      } else {
        classdesc::string nullstring;
        ::pack(b,"",nullstring);
        ::pack(b,"",static_cast<const Base&>(*this));
      }
    }

    void pack(classdesc::pack_t& b)  {
      if (opened()) commit();
      packConst(b);
    }
    void pack(classdesc::pack_t& b) const {packConst(b);}
    
    void unpack(classdesc::pack_t& b) {
      close();
      classdesc::string fname;
      bool readonly;
      ::unpack(b,"",m_filename);
      if (m_filename!="") {
        ::unpack(b,"",readonly);
        init(m_filename.c_str(),readonly? 'r': 'w');
      } 
      ::unpack(b,"",static_cast<Base&>(*this));
    }

    using KeyValueIterator=ecolab::KeyValueIterator<key,val>;
    using KeyIterator=ecolab::KeyIterator<key,val>;

    ecolab::KeyValueIterator<key,val> begin() const {return KeyValueIterator(filename());}
    ecolab::KeyValueIterator<key,val> begin() 
    {commit(); return KeyValueIterator(filename());}
    ecolab::KeyValueIterator<key,val> end() const {return KeyValueIterator();}

    /// access an iterator range of keys [keys.begin()...keys.end())
    ecolab::Keys<cachedDBM_base> keys() {return Keys(*this);}
    ecolab::ConstKeys<cachedDBM_base> keys() const {return ConstKeys(*this);}

  };

  /// persistent map
  template<class key, class val>
  class cachedDBM: public  cachedDBM_base<key,val>
  {
  public:
    cachedDBM(){}
    cachedDBM(const char* f, char mode='w'){this->init(f,mode);}
  };

  /*
    specialisations to handle char * cases
  */

  struct cachedDBM_string: public std::string
  {
    operator const char*() const {return c_str();}
    cachedDBM_string& operator=(const char* s) 
    {std::string::operator=(s); return *this;}
    cachedDBM_string() {}
    cachedDBM_string(const char*x): std::string(x) {}
  };

  template<class val>
  class cachedDBM<char *,val>: public cachedDBM_base<std::string,val>
  {
  public: 
    cachedDBM(){}
    cachedDBM(const char* f, char mode='w'){this->init(f,mode);}
  };

  template<class key>
  class cachedDBM<key,char *>: public cachedDBM_base<key,cachedDBM_string>
  {
  public: 
    cachedDBM(){}
    cachedDBM(const char* f, char mode='w'){this->init(f,mode);}
  };

  template <>
  class cachedDBM<char *,char *>: public cachedDBM_base<std::string,cachedDBM_string>
  {
  public: 
    cachedDBM(){}
    cachedDBM(const char* f, char mode='w'){init(f,mode);}
  };

#ifdef _CLASSDESC
#pragma omit pack ecolab::Datum
#pragma omit unpack ecolab::Datum
#pragma omit pack ecolab::cachedDBM
#pragma omit unpack ecolab::cachedDBM
#pragma omit pack ecolab::cachedDBM_base
#pragma omit unpack ecolab::cachedDBM_base

#pragma omit pack ecolab::Db
#pragma omit unpack ecolab::Db
#pragma omit pack Db
#pragma omit unpack Db

#endif
}

namespace classdesc_access
{
  namespace cd=classdesc;

  template <>
  struct access_pack<ecolab::Datum>
  {
    void operator()(cd::pack_t& t, const cd::string& d, const ecolab::Datum& x)
    {pack(t,"",x.size()); t.packraw(x.data(),x.size());}
  };

  template <>
  struct access_unpack<ecolab::Datum>
  {
    void operator()(cd::pack_t& t, const cd::string& d, ecolab::Datum& x)
    {
      size_t size; unpack(t,"",size);
      x.packraw(t.data()+t.pos(), size);
      t.seeko(size);
    }
    void operator()(classdesc::pack_t& t, const classdesc::string& d, const ecolab::Datum& x1)
    {
      ecolab::Datum x(x1);
      size_t size; unpack(t,"",size);
      x.packraw(t.data()+t.pos(), size);
      t.seeko(size);
    }
  };

  template <class K, class V>
  struct access_pack<ecolab::cachedDBM<K,V> >
  {
    template <class U>
    void operator()(classdesc::pack_t& b,const classdesc::string& d, U& a) 
    {a.pack(b);}
  };

  template <class K, class V>
  struct access_unpack<ecolab::cachedDBM<K,V> >
  {
    template <class U>
    void operator()(classdesc::unpack_t& b,const classdesc::string& d,U& a) 
    {a.unpack(b);}
  };
}

#include "cachedDBM.cd"
#endif


