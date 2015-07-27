#include <ndbm.h>
#include <db.h>
#include <fcntl.h>
#include <stdio.h>

main(int argc, char *argv[])
{
  DBM *idb;
  DB *odb;
  int i=0;
  datum key, val;

  if (argc<3) 
    {
      printf("usage: %s dbm-file bdb-file\n",argv[0]);
      exit(0);
    }

  idb=dbm_open(argv[1],O_RDONLY,0);
  db_create(&odb,0,0);
  odb->open(odb,NULL,argv[2],NULL,DB_HASH,DB_CREATE,0644);

  for (key=dbm_firstkey(idb); key.dptr!=NULL; key=dbm_nextkey(idb))
    {
      val=dbm_fetch(idb,key);
      if (val.dptr!=NULL)
	{
	  DBT k,v;
	  memset(&k,0,sizeof(k));
	  memset(&v,0,sizeof(v));
	  k.data=key.dptr; k.size=key.dsize;
	  v.data=val.dptr; v.size=val.dsize;
	  odb->put(odb,NULL,&k,&v,0);
          i++;
	}
    }
  printf("%d records converted\n",i);
  dbm_close(idb);
  odb->close(odb,0);
}
