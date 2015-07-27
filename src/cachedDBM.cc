/*
  @copyright Russell Standish 2000-2013
  @author Russell Standish
  This file is part of EcoLab

  Open source licensed under the MIT license. See LICENSE for details.
*/

#include "cachedDBM.h"
#include "ecolab_epilogue.h"
using namespace ecolab;

#if BDB
/*
  Unfortunately Berkley DB does not install the C++ API by default, 
  so it is better for EcoLab users if this file uses the C API 
  rather than the C++ API
*/

#include <db.h>
#if !defined(DB_VERSION_MAJOR) || DB_VERSION_MAJOR < 4
#error "Old version of Berkeley DB installed. Please upgrade it, or disable it with BDB= on the make command line"
#endif

inline DB* Db_(void *x) {return static_cast<DB*>(x);}
inline DBC* Dbc_(void *x) {return static_cast<DBC*>(x);}

class Dbt: public DBT
{
public:
  Dbt() {memset(this,0,sizeof(DBT));}
  Dbt(void *d, int s) {memset(this,0,sizeof(DBT)); data=d; size=s;}
  void *get_data() {return data;}
  int get_size() {return size;}
};

void Db::open(const char* filename,Db::rw openmode)
{
  /* we alloocate DB on heap, and use opaque pointers to avoid 
     polluting global namespace */
  int err;
  if ((err=db_create((DB**)&db,0,0))) 
    throw error("DB create failed: %s because %s",filename,db_strerror(err));
  if ((err=Db_(db)->open(Db_(db),NULL,filename,NULL,DB_HASH,
                         (openmode==read)? DB_RDONLY: DB_CREATE,0644)))
    throw error("DB open failed: %s because %s",filename,db_strerror(err));
  if ((err=Db_(db)->cursor(Db_(db),NULL,(DBC**)&dbc,0)))
    throw error("DB cursor create failed: %s because %s",filename,db_strerror(err));
}


void Db::close()
{
  if (db) 
    {
      int err;
      if ((err=Dbc_(dbc)->c_close(Dbc_(dbc))) ||
	  (err=Db_(db)->close(Db_(db),0)))
	throw error("DB close error: %s",db_strerror(err));
      dbc=db=NULL;
    }
}

void Db::flush()
{
  if (db) Db_(db)->sync(Db_(db),0);
}

bool Db::fetch(const Datum& key, Datum& val) const
{
  Dbt k(const_cast<char*>(key.data()),key.size()), v;
  bool last = Db_(db)->get(Db_(db),NULL,&k,&v,0);
  if (!last) 
    val.copy_ptr(v.get_data(),v.get_size());
  return last;
}
  
void Db::store(const Datum& key, const Datum& val)
{
  Dbt k(const_cast<char*>(key.data()),key.size()), 
    v(const_cast<char*>(val.data()),val.size());
  int err;
  if ((err=Db_(db)->put(Db_(db),NULL,&k,&v,0)))
    throw error("DB store failed %s",db_strerror(err));
}

void Db::del(const Datum& key)
{
  Dbt k(const_cast<char*>(key.data()),key.size());
  int err;
  if ((err=Db_(db)->del(Db_(db),NULL,&k,0)))
    throw error("DB delete error %s",db_strerror(err));
}

bool Db::firstkey(Datum& key, Datum& val) const
{
  Dbt k, v;
  bool last = Dbc_(dbc)->c_get(Dbc_(dbc),&k,&v,DB_FIRST);
  if (!last)
    {
      key.copy_ptr(k.data,k.size);
      val.copy_ptr(v.data,v.size);
    }
  return last;
}

bool Db::nextkey(Datum& key, Datum& val) const
{
  Dbt k, v;
  bool last = Dbc_(dbc)->c_get(Dbc_(dbc),&k,&v,DB_NEXT);
  if (!last)
    {
      key.copy_ptr(k.get_data(),k.get_size());
      val.copy_ptr(v.get_data(),v.get_size());
    }
  return last;
}

 
#elif defined(USE_DBM)  /* use ndbm */

/* the buggers have not used prototypes in ndbm.h -- grrr! */
typedef void DBM;
struct datum {void *dptr; int dsize;};

/* I hope these are right! - appears to be true on Irix, Linux, Tru64 and AIX */
#define DBM_INSERT	0
#define DBM_REPLACE	1

extern "C" {
DBM* dbm_open(const char *,int,mode_t);
void dbm_close(DBM*);
datum dbm_fetch(DBM*, datum);
int dbm_store(DBM *, datum, datum, int);
int dbm_delete(DBM *, datum);
datum dbm_firstkey(DBM *);
datum dbm_nextkey(DBM *);
}

/* end of include ndbm.h */
void Db::open(const char* filename,Db::rw openmode)
{
  db=dbm_open(filename,(openmode==read)?O_RDONLY:(O_RDWR | O_CREAT),0644);
}

void Db::close()
{
  if (db) dbm_close(db);
  db=NULL;
}

void Db::flush()
{ // nop
}

bool Db::fetch(const Datum& key, Datum& val) const
{
  datum k, v;
  k.dptr=const_cast<char*>(key.data()); k.dsize=key.size();
  v=dbm_fetch(db,k);
  bool last = v.dptr==NULL;
  if (!last) val.copy_ptr(v.dptr,v.dsize);
  return last;
}
  
void Db::store(const Datum& key, const Datum& val)
{
  datum k, v;
  k.dptr=const_cast<char*>(key.data()); k.dsize=key.size();
  v.dptr=const_cast<char*>(val.data()); v.dsize=val.size();
  if (dbm_store(db,k,v,DBM_REPLACE))
    throw error("DBM store failed");
}

void Db::del(const Datum& key)
{
  datum k;
  k.dptr=const_cast<char*>(key.data()); k.dsize=key.size();
  if (dbm_delete(db,k))
    throw error("delete failed");
}

bool Db::firstkey(Datum& key, Datum& val) const
{
  datum k, v;
  k=dbm_firstkey(db);
  bool last = k.dptr == NULL;
  if (!last)
    {
      v=dbm_fetch(db,k);
      key.copy_ptr(k.dptr,k.dsize);
      val.copy_ptr(v.dptr,v.dsize);
    }
  return last;
}

bool Db::nextkey(Datum& key, Datum& val) const
{
  datum k, v;
  k=dbm_nextkey(db);
  bool last = k.dptr == NULL;
  if (!last)
    {
      v=dbm_fetch(db,k);
      key.copy_ptr(k.dptr,k.dsize);
      val.copy_ptr(v.dptr,v.dsize);
    }
  return last;
}

#endif
