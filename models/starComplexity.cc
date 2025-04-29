#include "starComplexity.h"
#include "starComplexity.cd"
#include "netcomplexity.h"
#include "pythonBuffer.h"
#include "ecolab_epilogue.h"

#include <algorithm>
#include <assert.h>
#include <time.h>

StarComplexityGen starC;
CLASSDESC_ADD_GLOBAL(starC);
CLASSDESC_PYTHON_MODULE(starComplexity);

using namespace std;
using ecolab::NautyRep;
using ecolab::USMAlloc;

#ifdef SYCL_LANGUAGE_VERSION
using ecolab::syclQ;
template <class T, USMAlloc A=USMAlloc::shared>
using Alloc=ecolab::SyclQAllocator<T, A>;
#else
template <class T, USMAlloc A=USMAlloc::shared> using Alloc=std::allocator<T>;
#endif

// generate a an edge between node i and j, non-directional
linkRep edge(unsigned i, unsigned j)
{
  if (i==j) return 0; // no self-edges
  if (i<j) swap(i,j);
  return linkRep(1)<<(i*(i-1)/2+j);
}

void StarComplexityGen::generateElementaryStars(unsigned nodes)
{
  for (unsigned i=0; i<nodes; ++i)
    {
      linkRep star=0;
      for (unsigned j=0; j<nodes; ++j)
        star|=edge(i,j);
      elemStars.push_back(star);
    }
}

constexpr int setUnion=-2, setIntersection=-1;

// creates a uninitialised array on device
#ifdef SYCL_LANGUAGE_VERSION
template <class T>
class  DeviceArray
{
  T* data;
  size_t m_size;
  DeviceArray(const DeviceArray&)=delete;
  void operator=(const DeviceArray&)=delete;
public:
  DeviceArray(size_t size=0): m_size(size) {
    data=sycl::malloc_device<T>(size,syclQ());
    assert(data);
  }
  ~DeviceArray() {dealloc();}
  void dealloc() {
    sycl::free(data,syclQ());
  }
  DeviceArray(DeviceArray&& x): data(x.data), m_size(x.m_size) {x.data=nullptr; x.m_size=0;}
  DeviceArray& operator=(DeviceArray&& x) {dealloc(); data=x.data; m_size=x.m_size; x.data=nullptr; x.m_size=0; return *this;}
  T& operator[](size_t i) {return data[i];}
  const T& operator[](size_t i) const {return data[i];}
  size_t size() const {return m_size;}
  T* begin() {return data;}
  T* end() {return data+m_size;}
  void swap(DeviceArray& x) {std::swap(data,x.data); std::swap(m_size,x.m_size);}
};
#else
template <class T> using DeviceArray=vector<T>;
#endif

// structure holding position vector of stars within a recipe
class Pos: public DeviceArray<int>
{
  vector<int> hostCopy; // shadow data on host
  bool next(int starIdx) {
    if (starIdx==1) return false;
    auto& p=operator[](starIdx);
    auto& p1=operator[](starIdx-1);
    if (++p>2*starIdx-1) // exhausted positions, move next star down
      {
        p=p1+1;
        return next(starIdx-1);
      }
    return true;
  }
  void copy() {
#ifdef SYCL_LANGUAGE_VERSION
    syclQ().copy(hostCopy.data(), begin(), size()).wait();
#endif
  }
public:
  Pos(int numStars): DeviceArray<int>(numStars) {
    assert(numStars>1);
    for (int i=0; i<numStars; ++i) hostCopy.push_back(i);
#ifdef SYCL_LANGUAGE_VERSION
    copy();
#else
    swap(hostCopy);
#endif
  }


#if defined(SYCL_LANGUAGE_VERSION) && !defined(__SYCL_DEVICE_ONLY__)
  int& operator[](size_t i) {return hostCopy[i];}
  const int& operator[](size_t i) const {return hostCopy[i];}
#endif

  bool next() {
    auto r=next(size()-1);
    copy();
    return r;
  }
  void print() const {
    for (size_t i=0; i<size(); ++i)
      cout << operator[](i) << " ";
    cout << endl;
  }
};

struct EvalStackData
{
  DeviceArray<linkRep> elemStars;
  Pos pos;
  unsigned numGraphs=1;

  EvalStackData(const ElemStars&  elemStars, unsigned numStars):
    elemStars(elemStars.size()), pos(numStars)
  {
    for (unsigned i=2; i<numStars; ++i)
      numGraphs*=min(unsigned(elemStars.size()),(i+1));
#ifdef SYCL_LANGUAGE_VERSION
    syclQ().copy(elemStars.data(), this->elemStars.begin(), elemStars.size());
    syclQ().wait();
#else
    this->elemStars=elemStars;
#endif
  }
};

struct EvalStack
{
  const EvalStackData& data;
  EvalStack(const EvalStackData& data): data(data)  { }

  // evaluate recipe encoded by the op bitset and index \a idx within enumeration of numGraphs
  linkRep evalRecipe(unsigned op, unsigned idx)
  {
    unsigned stackTop=0;
    auto nodes=data.elemStars.size();
    auto numStars=data.pos.size();
    auto recipeSize=2*numStars-1;

    // suitable up to 10 node networks
    constexpr unsigned maxNodes=10, maxStars=2*maxNodes-1;
    assert(nodes<=maxNodes);
    auto elemStars=&data.elemStars[0];
    auto pos=&data.pos[0];
    linkRep stack[maxStars];
    for (unsigned p=0, opIdx=0, starIdx=2, range=3; p<recipeSize; ++p)
      if (p<2)
        stack[stackTop++]=elemStars[p];
      else if (starIdx<numStars && pos[starIdx]==p) // push a star, according to idx
        {
          assert(stackTop<numStars);
          auto divResult=div(int(idx), int(range));
          if (stackTop<numStars)
            stack[stackTop++]=elemStars[divResult.rem];
          idx=divResult.quot;
          if (range<nodes) ++range;
          ++starIdx;
        }
      else
        {
          if (stackTop>1 && stackTop<=numStars)
            {
              if (op&(1<<opIdx)) // set intersection
                {
                  auto v=stack[--stackTop];
                  stack[stackTop-1]&=v;
                }
              else                   // set union
                {
                  auto v=stack[--stackTop];
                  stack[stackTop-1]|=v;
                }
            }
          ++opIdx;
        }

    assert(stackTop>=1);
    return stack[0];
  }
};

// constant representing no graph at all
constexpr linkRep noGraph=~linkRep(0);

class OutputBuffer
{
public:
  static constexpr size_t maxQ=8192;
  using size_type=unsigned;
  using iterator=linkRep*;
  void push_back(linkRep x) {
    if (writePtr<maxQ)
      data[writePtr++]=x;
  }
  linkRep operator[](size_t i) const {return data[i];}
  size_type size() const {return writePtr;}
  iterator begin() {return data;}
  iterator end() {return data+size();}
  bool blown() const {return writePtr>=maxQ;}
  void reset() {writePtr=0;}
private:
  size_type writePtr=0;
  linkRep data[maxQ];
};

#ifdef SYCL_LANGUAGE_VERSION
using Event=sycl::event;
#else
struct Event {
  static void wait(const vector<Event>&) {}
};
#endif

struct BlockEvaluator: public EvalStackData
{

  vector<EvalStack,Alloc<EvalStack>> block;
  // an output queue for the results
  DeviceArray<OutputBuffer> result, backedResult;
  vector<linkRep,Alloc<linkRep>> alreadySeen, alreadySeenBacked; // sorted list of graphs already visited
  BlockEvaluator(unsigned blockSize, unsigned numStars, const ElemStars& elemStars):
    EvalStackData(elemStars, numStars), result(blockSize), backedResult(blockSize)
  {
    for (size_t i=0; i<blockSize; ++i) block.emplace_back(*this);
#ifdef SYCL_LANGUAGE_VERSION
    syclQ().parallel_for(blockSize,[=,this](auto i) {result[i].reset(); backedResult[i].reset();}).wait();
#endif

  }

  void eval(unsigned start, unsigned i)
  {
    if (i+start<numGraphs)
      {
        auto numOps=1<<(pos.size()-1);
        for (unsigned op=0; op<numOps; ++op)
          {
            auto r=block[i].evalRecipe(op,i+start);
            if (!binary_search(alreadySeen.begin(),alreadySeen.end(),r))
              result[i].push_back(r);
          }
      }
  }
  
  vector<OutputBuffer> getResults() {
    vector<OutputBuffer> r(block.size());
#ifdef SYCL_LANGUAGE_VERSION
    auto copied=syclQ().copy(backedResult.begin(),r.data(),r.size());
    syclQ().parallel_for(r.size(),copied,[this](auto i){backedResult[i].reset();}).wait();
#else
    r.swap(result);
#endif
    return r;
  }
  size_t size() const {return block.size();}
};

void StarComplexityGen::fillStarMap(unsigned maxStars)
{
  if (elemStars.empty()) return;
  // insert the single star graph
  starMap.emplace(elemStars[0],GraphComplexity{1,0.0});

  for (unsigned numStars=2; numStars<=maxStars; ++numStars)
  //unsigned numStars=maxStars;
  //int numLoops=10;
  {
    ecolab::DeviceType<BlockEvaluator> block(blockSize, numStars,elemStars);
    vector<Event> compute;
    Event resultSwapped, alreadySeenSwapped, starMapPopulated;
    bool populating=false;
    unsigned resultBufferConsumed=0;
    const unsigned numOps=1<<(numStars-1);
    auto populateStarMap=[&]() {
      for (auto& j: block->getResults())
        {
          if (j.blown())
            throw runtime_error("Output buffer blown");
          for (auto k: j)
            starMap.emplace(k, GraphComplexity{numStars,0.0});
        }
    };

    do
      {
        auto start=time(nullptr);

#ifdef SYCL_LANGUAGE_VERSION
        /* Using SYCL dependency graph structure, to run the compute
           side simultaneously with accumulating results in the
           starMap on the host. sycl::event is used to coordinate host
           and device threads. */
        for (unsigned i=0; i<block->numGraphs; i+=block->size())
          {
            while (populating && resultBufferConsumed+numOps>=OutputBuffer::maxQ)
              usleep(1000); // pause until results consumed
            
            // compute a block of graphs
            compute.push_back
              (syclQ().parallel_for(blockSize, {resultSwapped, alreadySeenSwapped},
                                    [=,block=&*block](auto j){block->eval(i,j);}));
            resultBufferConsumed+=numOps;

            if (!populating) // throttle consuming thread
              {
                populating=true;
                // retrieve results to host by swapping with backing buffers
                resultSwapped=syclQ().submit([&](auto& handler) {
                  handler.depends_on(compute);
                  handler.host_task([&](){block->result.swap(block->backedResult);});
                });
                resultBufferConsumed=0;
                
                // accumulate starMap results on separate host thread
                starMapPopulated=syclQ().submit([&](auto& handler) {
                  handler.depends_on(resultSwapped);
                  handler.host_task([&]() {
                    populateStarMap();
                    if (i+block->size()<block->numGraphs)
                      {
                        block->alreadySeenBacked.clear();
                        for (auto& k: starMap) block->alreadySeenBacked.push_back(k.first);
                        // push update already seen vector to device
                        alreadySeenSwapped=syclQ().single_task
                          (compute,
                           [=,block=&*block]{
                             block->alreadySeen.swap(block->alreadySeenBacked);
                           });
                      }
                    populating=false;
                  });
                });
              }
          }
        // wait until all compute threads finish before bumping pos
        Event::wait(compute);
#else
        for (unsigned i=0; i<block->numGraphs; i+=block->size())
          for (unsigned j=0; j<block->size(); ++j)
            block->eval(i,j);
        populateStarMap();
#endif
        //cout<<(time(nullptr)-start)<<"secs\n";
      } while (block->pos.next() /* && --numLoops>0*/);
#ifdef SYCL_LANGUAGE_VERSION
    auto start=time(nullptr);
    syclQ().wait(); // flush queue before destructors called
    //cout<<"Final wait:"<<(time(nullptr)-start)<<"secs\n";
#endif
  }
}

NautyRep toNautyRep(linkRep g, unsigned nodes)
{
  NautyRep n(nodes);
  for (unsigned i=0; i<nodes; ++i)
    for (unsigned j=0; j<i; ++j)
      if (g&(1<<(i*(i-1)/2+j)))
        n(i,j)=n(j,i)=true;
  return n;
}

linkRep toLinkRep(const NautyRep& g)
{
  linkRep l=0;
  // symmetrise as we go
  for (unsigned i=0; i<g.nodes(); ++i)
    for (unsigned j=0; j<g.nodes(); ++j)
      if (g(i,j))
        {
          if (j<i)
            l|=1<<(i*(i-1)/2+j);
          else if (i<j)
            l|=1<<(j*(j-1)/2+i);
        }
        // else ignore self-links
  return l;
}

void StarComplexityGen::canonicaliseStarMap()
{
  vector<linkRep> toErase;
  for (auto i=starMap.begin(); i!=starMap.end(); ++i)
    {
      auto c=toLinkRep(toNautyRep(i->first, elemStars.size()).canonicalise());
      if (c!=i->first)
        {
          auto& starC=starMap[c];
          starC.star=starC.star>0? min(starC.star, i->second.star): i->second.star;
          toErase.push_back(i->first);
        }
    }
  for (auto i: toErase) starMap.erase(i);
  for (auto& i: starMap)
    i.second.complexity=complexity(toNautyRep(i.first, elemStars.size()),true);
}
