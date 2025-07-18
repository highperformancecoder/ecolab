#include "starComplexity.h"
#ifdef SYCL_LANGUAGE_VERSION
#include "vecBitSet.cd"
#endif
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
  if (nodes>maxNodes)
    throw runtime_error("nodes requested exceeds maximum configured: "+to_string(maxNodes));
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

  // Simple tests of validity. Don't end on a push, numOps<2*-1 preceding etc.
  bool valid() const {
    for (size_t p=2; p<size(); ++p)
      if (operator[](p)>int(2*p-1))
        return false;
    return true;
  }
  
  bool next() {
    bool r=false;
    do
      {
        r=next(size()-1);
      } while (r && !valid());
    if (r) copy();
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

    assert(nodes<=maxNodes);
    auto elemStars=&data.elemStars[0];
    auto pos=&data.pos[0];
    linkRep stack[maxStars];
    for (unsigned p=0, opIdx=0, starIdx=2, range=3; p<recipeSize; ++p)
      if (p<2)
        stack[stackTop++]=elemStars[p];
      else if (starIdx<numStars && pos[starIdx]==int(p)) // push a star, according to idx
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
const linkRep noGraph=~linkRep(0);

class OutputBuffer
{
public:
  static constexpr size_t maxQ=20000;
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
        unsigned numOps=1<<(pos.size()-1);
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
    syclQ().copy(backedResult.begin(),r.data(),r.size()).wait();
#else
    r.swap(result);
#endif
    return r;
  }
  size_t size() const {return block.size();}
};

#ifdef SYCL_LANGUAGE_VERSION
sycl::info::event_command_status eventStatus(Event ev)
{
  return ev.get_info<sycl::info::event::command_execution_status>();
}
#endif

void StarComplexityGen::fillStarMap(unsigned numStars)
{
  if (elemStars.empty()) return;
  if (numStars==1)
    {
      // insert the single star graph
      starMap.emplace(elemStars[0],1);
      counts.emplace(elemStars[0],1);
      return;
    }

  ecolab::DeviceType<BlockEvaluator> block(blockSize, numStars,elemStars);
  Event resultSwapped, alreadySeenSwapped, starMapPopulated,backedResultsReset;
  atomic<unsigned> resultBufferConsumed=0;
  const unsigned numOps=1<<(numStars-1);

  bool blown=false;
  auto populateStarMap=[&]() {
    for (auto& j: block->getResults())
      {
        if (j.blown())
          {
            blown=true;
            return;
          }
        for (auto k: j)
          {
            auto res=starMap.emplace(k, numStars);
            if (res.first->second==numStars) // only count least star operations
              counts[k]++;
          }
      }
  };

  auto checkBlown=[&]() {
    if (blown)
      {
        cout<<"buffer blown, throwing"<<endl;
#ifdef SYCL_LANGUAGE_VERSION
        syclQ().wait(); // flush queue before destructors called
#endif
        throw runtime_error("Output buffer blown");
      }
  };
  
  //int numLoops=3000;
  do
    {
      auto start=time(nullptr);
      
#ifdef SYCL_LANGUAGE_VERSION
      /* Using SYCL dependency graph structure, to run the compute
         side simultaneously with accumulating results in the
         starMap on the host. sycl::event is used to coordinate host
         and device threads. */
      vector<Event> compute;

      auto consumeResults=[&](bool lastLoop) {
        // retrieve results to host by swapping with backing buffers
        resultSwapped=syclQ().single_task
          (compute,[block=&*block](){
            block->result.swap(block->backedResult);
          });
        
        // accumulate starMap results on separate host thread
        starMapPopulated=syclQ().submit([&](auto& handler) {
          handler.depends_on(resultSwapped);
          handler.host_task([&]() {
            populateStarMap();
          });
        });

        // reset resultBufferConsumed to number of compute threads outstanding
        resultBufferConsumed=0;
        for (auto& i: compute)
          if (eventStatus(i)!=sycl::info::event_command_status::complete)
            resultBufferConsumed+=numOps;

        backedResultsReset=syclQ().parallel_for
           (blockSize,starMapPopulated,[block=&*block](auto i){block->backedResult[i].reset();});
      };
        
      for (unsigned i=0; i<block->numGraphs; i+=blockSize)
        {
          // if we're in danger of blowing the buffer, stop submitting compute tasks
          if (resultBufferConsumed+numOps>=OutputBuffer::maxQ)
            {
              if (eventStatus(backedResultsReset)==sycl::info::event_command_status::complete)
                consumeResults(i+blockSize>=block->numGraphs);
              cout<<"."<<flush;
              backedResultsReset.wait();
            }
          
          // compute a block of graphs
          compute.push_back
            (syclQ().parallel_for(blockSize, {resultSwapped, alreadySeenSwapped},
                                  [=,block=&*block](auto j){block->eval(i,j);}));
          resultBufferConsumed+=numOps;
          
          if (eventStatus(backedResultsReset)==sycl::info::event_command_status::complete)
            // only queue consumeResult if previous one finished
            consumeResults(i+blockSize>=block->numGraphs);
        }
      // wait until all compute threads finish before bumping pos
      Event::wait(compute);
#else
      for (unsigned i=0; i<block->numGraphs; i+=blockSize)
#pragma omp parallel for
        for (unsigned j=0; j<blockSize; ++j)
          block->eval(i,j);
      populateStarMap();
#pragma omp parallel for
      for (unsigned j=0; j<blockSize; ++j)
        block->result[j].reset();
#endif

      checkBlown();

     //cout<<"loops rem="<<numLoops<<" resultBufferConsumed:"<<unsigned(resultBufferConsumed)<<" "<<(time(nullptr)-start)<<"secs\n";
    } while (block->pos.next() /*&& --numLoops>0*/);
#ifdef SYCL_LANGUAGE_VERSION
  // final mop up
  syclQ().single_task
    (backedResultsReset,[block=&*block](){
      block->result.swap(block->backedResult);
    }).wait();
  populateStarMap();
  checkBlown();
  syclQ().wait_and_throw(); // flush queue before destructors called
#endif

}

NautyRep toNautyRep(linkRep g, unsigned nodes)
{
  NautyRep n(nodes);
  for (unsigned i=0; i<nodes; ++i)
    for (unsigned j=0; j<i; ++j)
      if (!(g&(linkRep(1)<<(i*(i-1)/2+j))).empty())
        //if (g(i,j))
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
            l|=linkRep(1)<<(i*(i-1)/2+j);
          else if (i<j)
            l|=linkRep(1)<<(j*(j-1)/2+i);
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
          if (starC==0 || i->second<starC)
            {
              starC=i->second;
              counts[c]=counts[i->first];
            }
          else if (i->second==starC)
            counts[c]+=counts[i->first];
          assert(counts[c]);
          toErase.push_back(i->first);
        }
    }
  for (auto i: toErase)
    {
      starMap.erase(i);
      counts.erase(i);
    }
}

linkRep StarComplexityGen::complement(linkRep x) const
{
  return toLinkRep(toNautyRep(~x, elemStars.size()).canonicalise());
}

unsigned StarComplexityGen::symmStar(linkRep x) const
{
  auto star=starMap.find(x);
  if (star==starMap.end()) return 0;
  auto starComp=starMap.find(complement(x));
  if (starComp==starMap.end()) return star->second;
  return min(star->second, starComp->second);
}

double lnFactorial(unsigned n)
{
  double r=0;
  for (unsigned i=2; i<n; ++i) r+=log(i);
  return r;
}

GraphComplexity StarComplexityGen::complexity(linkRep g) const
{
  const double ilog2=1/log(2.0);
  GraphComplexity r;
  double lnomega;
  NautyRep ng=toNautyRep(g, elemStars.size());
  ecolab_nauty(ng, ng, lnomega, false);
  r.complexity=ecolab::baseComplexity(ng.nodes(), ng.links(), true) - ilog2*lnomega;
  auto star=symmStar(g);
  switch (star)
    {
    case 0:
      r.starComplexity=nan("");
      break;
    case 1:
      r.starComplexity=2*ceil(ilog2*log(ng.nodes()))+1;
      break;
    default:
      {
        auto gIter=starMap.find(g);
        unsigned count=0;
        if (gIter!=starMap.end() && gIter->second==star)
          {
            auto countIter=counts.find(g);
            assert(countIter!=counts.end());
            count=countIter->second;
          }
        else
          {        
            auto countIter=counts.find(complement(g));
            assert(countIter!=counts.end());
            count=countIter->second;
          }
        assert(count>0);
        r.starComplexity=2*ceil(ilog2*log(ng.nodes()))+ceil(ilog2*log(star))+(star-2)-ilog2*log(count);
        if (star<=ng.nodes())
          r.starComplexity+=ilog2*(lnFactorial(star));
        else
          r.starComplexity+=ilog2*(lnFactorial(ng.nodes())+(star-ng.nodes())*log(ng.nodes()));
      }
      break;
    }
  return r;
}


unsigned starUpperBound(linkRep x, unsigned nodes) 
{
  if (x.empty()) return 3; // intersection of 3 stars is empty.
  // Firstly remove those nodes that are full stars
  vector<unsigned> fullStars;
  for (unsigned i=0; i<nodes; ++i)
    {
      bool fullStar=true;
      for (unsigned j=0; fullStar && j<nodes; ++j)
        if (i!=j && !x(i,j))
          fullStar=false;
      if (fullStar) fullStars.push_back(i);
    }

  unsigned stars=fullStars.size();
  for (auto i: fullStars)
    for (unsigned j=0; j<nodes; ++j)
      x&=~edge(i,j); // remove all edges to this node


  // calculate node degree
  map<unsigned,unsigned> nodeDegree;
  for (unsigned i=0; i<nodes; ++i)
    for (unsigned j=0; j<i; ++j)
      if (x(i,j))
        {
          ++nodeDegree[i];
          ++nodeDegree[j];
        }


  struct SecondLess
  {
    bool operator()(const pair<unsigned,unsigned>& x, const pair<unsigned,unsigned>& y)
      const {return x.second < y.second;}
  };
  
  // compute the number of operations xᵢ∪(xₖ∩xₗ…)
  auto maxDegree=max_element(nodeDegree.begin(), nodeDegree.end(), SecondLess());

  vector<unsigned> prevNbrs;
  while (maxDegree->second>0)
    {
      vector<unsigned> neighbours;
      for (unsigned j=0; j<maxNodes; ++j)
        if (x(maxDegree->first, j) && nodeDegree[j])
          {
            --nodeDegree[j];
            neighbours.push_back(j);
          }
      if (neighbours==prevNbrs)
        ++stars;
      else
        stars+=1+maxDegree->second;
      prevNbrs=std::move(neighbours);
      maxDegree->second=0; // accounted for all links to this node
      maxDegree=max_element(nodeDegree.begin(), nodeDegree.end(), SecondLess());
    }

  return stars;
}

unsigned StarComplexityGen::starUpperBound(const linkRep& x) const
{
  unsigned n=elemStars.size();
  unsigned ub=::starUpperBound(x, n);
  unsigned complementUb=::starUpperBound((~x).maskOut(n), n);
  assert(ub>0 && complementUb>0);
  return min(ub, complementUb);
}

GraphComplexity StarComplexityGen::randomERGraph(unsigned nodes, unsigned links)
{
  linkRep g=0;
  if (links>nodes*(nodes-1)/2)
    throw runtime_error("links requested exceeds maximum possible: "+to_string(nodes*(nodes-1)/2));
  for (unsigned l=0; l<links; ++l)
    {
      unsigned node1=nodes*uni.rand(), node2;
      do
        node2=nodes*uni.rand();
      while (node1==node2);
      g|=edge(node1,node2);
    }

  GraphComplexity r;
  r.starComplexity=::starUpperBound(g,nodes);
  NautyRep ng=toNautyRep(g, nodes);
  r.complexity=ecolab::complexity(ng, true);
  return r;
}
