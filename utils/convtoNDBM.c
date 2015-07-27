#include <ndbm.h>
#include <db.h>
#include <fcntl.h>
#include <stdio.h>

main(int argc, char *argv[])
{
  DBM *odb;
  DB *idb;
  int i=0;
  DBT key, val;
  DBC *dbc;
  int eof, err;
  

  if (argc<3) 
    {
      printf("usage: %s bdb-file ndbm-file\n",argv[0]);
      exit(0);
    }

  err=db_create(&idb,0,0);
  if (err) 
    printf("error %d return from db_create\n",err);
  err=idb->open(idb,NULL,argv[1],NULL,DB_HASH,DB_RDONLY,0644);
  if (err) idb->err(idb,err,"open");
  err=idb->cursor(idb,NULL,&dbc,0);
  if (err) idb->err(idb,err,"cursor");

  odb=dbm_open(argv[2],O_RDWR|O_CREAT,0644);
  memset(&key,0,sizeof(DBT));
  memset(&val,0,sizeof(DBT));

  for (eof=dbc->c_get(dbc,&key,&val,DB_FIRST); !eof; 
       eof=dbc->c_get(dbc,&key,&val,DB_NEXT))
    {
      datum k,v;
      k.dptr=key.data;
      k.dsize=key.size;
      v.dptr=val.data;
      v.dsize=val.size;
      dbm_store(odb,k,v,DBM_REPLACE);
      i++;
    }
  if (eof!=DB_NOTFOUND)
    idb->err(idb,eof,"c_get");
  printf("%d records converted\n",i);
  dbc->c_close(dbc);
  dbm_close(odb);
  idb->close(idb,0);
}
