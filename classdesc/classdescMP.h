/*
  @copyright Russell Standish 2000-2013
  @author Russell Standish
  This file is part of Classdesc

  Open source licensed under the MIT license. See LICENSE for details.
*/

/**\file
\brief MPI parallel processing library
*/
#ifndef CLASSDESCMP_H
#define CLASSDESCMP_H

#include "pack_base.h"

#undef HAVE_MPI_CPP
//#undef SEEK_SET
//#undef SEEK_CUR
//#undef SEEK_END
#include <mpi.h>
#include <stdexcept>
#include <stdio.h>
#include <vector>

namespace classdesc
{

#ifdef HETERO
 /* Use XDR machine independent packing is cluster is heterogeneous*/
  typedef xdr_pack MPIbuf_base;
#else
  typedef pack_t MPIbuf_base;
#endif

/// MPIbuf manipulator to send the MPIbuf's contents to a remote process 
  class send
  {
    send();
  public:
    int proc, tag;
    send(int proc, int tag=0): proc(proc), tag(tag) {}
  };


/// MPIbuf manipulator to asyncronously send the MPIbuf's contents to a remote process 
  class isend
  {
    isend();
  public:
    int proc, tag;
    /// \a proc MPI taskID to send the message to
    isend(int proc, int tag=0): proc(proc), tag(tag) {}
  };


/// MPIbuf manipulator to broadcast the MPIbuf's contents to all processes
  class bcast
  {
    bcast();
  public:
    int root;
    /// \a root is the taskID of the source data
    bcast(int root): root(root) {}
  };

  /// A manipulator to mark a processor boundary for scatterv
  class mark {};
  
  class MPIbuf_array;

  /// buffer object providing MPI functionality
  /** This class inherits from \c pack_t, so full classdesc
      serialisation is available */
  class MPIbuf: public MPIbuf_base
  {
    int *offsets;
    unsigned offsctr;
    /* MPI_Finalized only available in MPI-2 standard */
    bool MPI_running()
    {
      int fi, ff=0; MPI_Initialized(&fi); 
#if (defined(MPI_VERSION) && MPI_VERSION>1 || defined(MPICH_NAME))
      MPI_Finalized(&ff); 
#endif
      return fi&&!ff;
    }
    MPI_Request request;
    friend class MPIbuf_array;
  public:
    /// The MPI communicator to be used for subsequent communications
    MPI_Comm Communicator;

    unsigned myid(); /// current processes taskID
    unsigned nprocs(); /// number of processes
    /// buffer size is same on all processes in a collective communication
    /** set this to true for improved collective communication. Safe
        to leave as false */
    bool const_buffer; 
    int proc, tag; /* store status of receives */

    MPIbuf()
    {
      request=MPI_REQUEST_NULL;
      Communicator=MPI_COMM_WORLD; const_buffer=0;
      offsets=new int[nprocs()+1]; offsctr=1; offsets[0]=0;
    }
    MPIbuf(const MPIbuf& x): offsets(NULL) {*this=x;}
    ~MPIbuf() 
    {
      if (request!=MPI_REQUEST_NULL) wait(); //MPI_Request_free(&request);
      delete [] offsets;
    }
    const MPIbuf& operator=(const MPIbuf& x)
    { 
      Communicator=x.Communicator;
      delete [] offsets;
      offsets=new int[nprocs()+1];
      offsctr=x.offsctr;
      for (unsigned i=0; i<offsctr; i++) offsets[i]=x.offsets[i];
      request=MPI_REQUEST_NULL;
      packraw(x.data(),x.size());
      return *this;
    }
      

    /// returns true if previous asyncronous call has been set
    bool sent() {int is_sent; MPI_Status s; MPI_Test(&request,&is_sent,&s); return is_sent;}
    /// wait for previous asyncronous call to complete
    void wait() {MPI_Status s; MPI_Wait(&request,&s);}

    /// send data to \a dest with tag \a tag
    void send(unsigned dest, int tag);
    /// asyncronously send data to \a dest with tag \a tag
    void isend(unsigned dest, int tag);

    ///receive a message from \a p (MPI_ANY_SOURCE) with tag \a t (MPI_ANY_TAG)
    MPIbuf& get(int p=MPI_ANY_SOURCE, int t=MPI_ANY_TAG);

    /// perform a simultaneous send and receive between a pair of processes
    void send_recv(unsigned dest, int sendtag, int source, int recvtag);
    
    /// broadcast data from \a root
    MPIbuf& bcast(unsigned root);
    
    /// gather data (concatenated) into \a root's buffer
    MPIbuf& gather(unsigned root);
    /// scatter \a root's data (that has been marked with \c mark)
    MPIbuf& scatter(unsigned root); 
    /// reset the buffer to send or receive a new message
    MPIbuf& reset() {reseti(); reseto(); tag=1; return *this;}
    /// is there a message waiting to be received into the buffe
    bool msg_waiting(int source=MPI_ANY_SOURCE, int tag=MPI_ANY_TAG);

    template <class T> MPIbuf& operator<<(const T& x);
  
    /* Manipulators */
    MPIbuf& operator<<(classdesc::send s)
    {send(s.proc,s.tag); return *this;}
    MPIbuf& operator<<(classdesc::isend s)
    {isend(s.proc,s.tag); return *this;}
    MPIbuf& operator<<(classdesc::bcast s)
    {bcast(s.root); return *this;}
    /* Mark a processor boundary for scatterv */
    MPIbuf& operator<<(mark s)
    {offsets[offsctr++]=size(); return *this;}

    //    template <class T> inline MPIbuf& operator<<(const T& x);
    //    {pack(this,string(),x); return *this;}

  };

  /// used for managing groups of messages
  class MPIbuf_array
  {
    std::vector<MPIbuf> bufs;
    std::vector<MPI_Request> requests;
  public:
    
    MPIbuf_array(unsigned n): bufs(n), requests(n) {}

    MPIbuf& operator[](unsigned i) {return bufs[i];}

    /// return true if all messages have been completed
    bool testall()
    {
      int flag;
      for (size_t i=0; i<bufs.size(); i++) requests[i]=bufs[i].request;
      MPI_Testall(bufs.size(),&requests[0],&flag,MPI_STATUSES_IGNORE);
      return flag;
    }
    /// return the index of any request that has completed, or MPI_UNDEFINED if none  
    int testany()
    {
      int flag,index;
      for (size_t i=0; i<bufs.size(); i++) requests[i]=bufs[i].request;
      MPI_Testany(bufs.size(),&requests[0],&index,&flag,MPI_STATUS_IGNORE);
      return index;
    }
    /// return the index of the requests that have completed
    std::vector<int> testsome()
    {
      int count;
      std::vector<int> index(bufs.size());
      for (size_t i=0; i<bufs.size(); i++) requests[i]=bufs[i].request;
      MPI_Testsome(bufs.size(),&requests[0],&count,&index[0],MPI_STATUSES_IGNORE);
      return std::vector<int>(index.begin(),index.begin()+count);
    }
    /// wait for all outstanding requests to complete
    void waitall() 
    {
      for (size_t i=0; i<bufs.size(); i++) requests[i]=bufs[i].request;
      MPI_Waitall(bufs.size(),&requests[0],MPI_STATUSES_IGNORE);
    }
    /// wait for any outstanding request to complete, returning index of completed request
    int waitany()
    {
      int index;
      for (size_t i=0; i<bufs.size(); i++) requests[i]=bufs[i].request;
      MPI_Waitany(bufs.size(),&requests[0],&index,MPI_STATUSES_IGNORE);
      return index;
    }
    /// wait for some outstanding requests to complete, returning an array of request indices
    std::vector<int> waitsome()
    {
      int count;
      std::vector<int> index(bufs.size());
      for (size_t i=0; i<bufs.size(); i++) requests[i]=bufs[i].request;
      MPI_Waitsome(bufs.size(),&requests[0],&count,&index[0],MPI_STATUS_IGNORE);
      return std::vector<int>(index.begin(),index.begin()+count);
    }
  };
    

  inline unsigned MPIbuf::myid()
  {
    int m;
    if (MPI_running())    MPI_Comm_rank(Communicator,&m);
    else m=0;
    return m;
  }
  
  inline unsigned MPIbuf::nprocs()
  {
    int m;
    if (MPI_running()) MPI_Comm_size(Communicator,&m);
    else m=1;
    return m;
  }

  inline void MPIbuf::send(unsigned dest, int tag=0)
  {
    if (dest==myid()) return; /* nothing to be done */
    MPI_Send(data(),size(),MPI_CHAR,dest,tag,Communicator); reseti();
  }

  inline void MPIbuf::isend(unsigned dest, int tag=0)
  {
    if (dest==myid()) return; /* nothing to be done */
    MPI_Isend(data(),size(),MPI_CHAR,dest,tag,Communicator,&request); reseti();
  }

  inline MPIbuf& MPIbuf::get(int p, int t)
  {
    MPI_Status status;
    MPI_Probe(p,t,Communicator,&status);
    int sz;
    MPI_Get_count(&status,MPI_CHAR,&sz); //this is berserk, but MPI must have ints!
    m_size=sz;
    realloc(m_size);
    proc=status.MPI_SOURCE;
    tag=status.MPI_TAG;
    MPI_Recv(data(),size(),MPI_CHAR,proc,tag,Communicator,&status);
    reseto();
    return *this;
  }

  // 2002-05-16 - asynchorous send and receive modification.
  // 2002-05-22 - use MPI_Isend, MPI_Proobe and MPI_Recv
  //
  inline void MPIbuf::send_recv(unsigned dest, int sendtag, 
                                int source=MPI_ANY_SOURCE, 
                                int recvtag=MPI_ANY_TAG)
  {
    if (dest==myid()) return; /* nothing to be done */
    /* send sizes first */
    int tempsize;
    MPI_Status  status;
    MPI_Request r1;

    MPI_Isend(data(), size(), MPI_CHAR, dest, sendtag, Communicator, &r1);
    MPI_Probe(source, sendtag, Communicator, &status);
    MPI_Get_count(&status, MPI_CHAR, &tempsize);
    
    char *tempdata=realloc(NULL, tempsize);
    MPI_Recv(tempdata,tempsize, MPI_CHAR,source,recvtag,Communicator,&status);
    
    MPI_Wait(&r1,&status); // ensure data is actually sent before deleting storage
    realloc(0); m_data=tempdata; m_size=tempsize;
    proc=status.MPI_SOURCE;
    tag=status.MPI_TAG;
    reseto();
  }

  inline MPIbuf& MPIbuf::bcast(unsigned root)
  {
    int myid;
    if (!const_buffer)
      {
        int sz=size();
        MPI_Bcast(&sz,1,MPI_INT,root,Communicator);
        m_size=sz;
      }
    MPI_Comm_rank(Communicator,&myid);
    if (myid!=int(root)) realloc(m_size);
    MPI_Bcast(data(),size(),MPI_CHAR,root,Communicator);
    reseto();
    return *this;
  }
  
  inline MPIbuf& MPIbuf::gather(unsigned root)
  {
    int rootsz=0;
    unsigned i;
    char *rootdata=NULL;
    int *sizes=NULL, *offsets=NULL;
    if (!const_buffer)
      {
        if (myid()==root)
          {
            sizes=new int[nprocs()];
            offsets=new int[nprocs()+1];
          }
        int sz=m_size;
        MPI_Gather(&sz,1,MPI_INT,sizes,1,MPI_INT,root,Communicator);
        if (myid()==root)
          {
            for (offsets[0]=0, i=0; i<nprocs(); i++)
              offsets[i+1]=offsets[i]+sizes[i];
            rootsz=offsets[nprocs()];
            rootdata=realloc(NULL,rootsz);
        }
        MPI_Gatherv(data(),size(),MPI_CHAR,rootdata,sizes,offsets,
                    MPI_CHAR,root,Communicator);
        if (myid()==root)
          {delete [] sizes; delete [] offsets;}
    }
    else
      {
        rootsz=size();
        if (myid()==root) rootdata=realloc(NULL,size()*nprocs());
        MPI_Gather(data(),size(),MPI_CHAR,rootdata,size(),
                   MPI_CHAR,root,Communicator);
      }
    if (myid()==root) {free(m_data); m_data=rootdata; reseto(); m_size=rootsz;}
    else reseti();
    return *this;
  }

  inline MPIbuf& MPIbuf::scatter(unsigned root)
  {
    int *sizes=NULL, np=nprocs(), amroot=myid()==root;
    char *rootdata=NULL;
    if (amroot)
    {
      rootdata=m_data;
      sizes=new int[np];
      for (int i=0; i<np; i++) sizes[i]=offsets[i+1]-offsets[i];
      m_data=realloc(NULL,sizes[root]);
    }
    /* broadcast sizes array to slaves */
    int sz;
    MPI_Scatter(sizes,1,MPI_INT,&sz,1,MPI_INT,root,Communicator);
    if (sz>=0)
      {
        m_size=sz;
        realloc(m_size);
        MPI_Scatterv(rootdata,sizes,offsets,MPI_CHAR,data(),size(),MPI_CHAR,
                     root,Communicator);
      }
    if (amroot)
      {
        free(rootdata); delete [] sizes;
        for (int i=0; i<np; offsets[i++]=0);
      }
    return *this;
  }

  inline bool MPIbuf::msg_waiting(int source,int tag)
  {
    MPI_Status status;
    int waiting;
    MPI_Iprobe(source,tag,Communicator,&waiting,&status);
    return waiting;
  }

  /// Master slave manager
  template<class S>
  class MPIslave: public MPIbuf
  {
    S slave;
    void method(MPIbuf& buffer);
  public:
    std::vector<int> idle; ///< list of waiting slaves, valid on master 
    MPIslave() {init();}
    ~MPIslave() {finalize();}
    void init();
    void finalize();
    MPIbuf& operator<<(void (S::*m)(MPIbuf&))
    {
      reseti();
      pack(*this,string(),m);
      //      ::pack(cmd,string(),is_array(),*(char*)&m,1,sizeof(m));
      return *this;
    }
    template <class T> MPIbuf& operator<<(const T& x)
    {reseti(); return (*this) << x;}
    /// send a request to the next available slave
    void exec(MPIbuf& x) {x.send(idle.back(),0); idle.pop_back();}
    /// process a return value
    MPIbuf& get_returnv(){get(); idle.push_back(proc); return *this;}
    /// true if all slaves are idle
    bool all_idle() {return idle.size()==nprocs()-1;}
    /// wait until all slaves are idle
    void wait_all_idle() {while (!all_idle()) get_returnv();}
    /// broadcast a request to all slaves.
    void bcast(MPIbuf& c);
  };

  template <class S>
  inline void MPIslave<S>::init()
  {
#if MPI_DEBUG
    /* enable this piece of code for debugging under gdb */
    char buf[1];
    if (myid==0) gets(buf);
    MPI_Barrier(MPI_COMM_WORLD);
#endif
    if (myid()>0)
      {
        /* slave loop */
        MPIbuf buffer;
        for (buffer.get(); buffer.tag==0; buffer.get())
          method(buffer);
        MPI_Finalize();
        exit(0);
      }
    else
      for (unsigned i=1; i<nprocs(); i++) idle.push_back(i);
  }

  template <class S>
  void MPIslave<S>::finalize()
  {
    if (myid()==0)
      for (unsigned i=1; i<nprocs(); i++) send(i,1);
  }

  template <class S>
  inline void MPIslave<S>::method(MPIbuf& buffer)
  {
    void (S::*m)(MPIbuf&);
    //    buffer.unpackraw((char*)&m,sizeof(m));
    unpack(buffer,string(),m);
    buffer.tag=0;
    (slave.*m)(buffer);
    if (buffer.tag) buffer.send(buffer.proc,0);
  }

  template <class S>
  inline void MPIslave<S>::bcast(MPIbuf& c)
  {
    for (unsigned i=1; i<nprocs(); i++) 
    {MPI_Send(data(),size(),MPI_CHAR,i,0,c.Communicator);}
    c.reseti();
  }

  /// RAII class to setup and tear down MPI classes. Must be instantiated in main
  class MPISPMD
  {
  public:
    int nprocs, myid;
    MPISPMD() {nprocs=1, myid=0;}
    MPISPMD(int& argc, char**& argv) {init(argc,argv);};
    ~MPISPMD() {finalize();}
    void init(int& argc, char**& argv);
    void finalize() {MPI_Finalize();}
  };

  inline void MPISPMD::init(int& argc, char**& argv)
  {
    MPI_Init(&argc,&argv);
    MPI_Comm_rank(MPI_COMM_WORLD,&myid);
    MPI_Comm_size(MPI_COMM_WORLD,&nprocs);
#if MPI_DEBUG
    /* enable this piece of code for debugging under gdb */
    char buf[1];
    if (myid==0) gets(buf);
    MPI_Barrier(MPI_COMM_WORLD);
#endif
  }

}

#endif /* CLASSDESCMP_H */



