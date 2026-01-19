extern "C"
{
#include "abc/src/base/abc/abc.h"
}

#ifndef ECOLAB_ABC_H
#define ECOLAB_ABC_H

namespace ecolab {

  class AbcNodes {
    abc::Vec_Ptr_t* vNodes;
public:
    // RAII Constructor: calls Abc_NtkDfs
    explicit AbcNodes(abc::Abc_Ntk_t* pNtk, bool fAllNodes = false) 
      : vNodes(abc::Abc_NtkDfs(pNtk, fAllNodes ? 1 : 0)) {}

    // Destructor: ensures memory is freed
    ~AbcNodes() { if (vNodes) abc::Vec_PtrFree(vNodes); }

    // No copying (to avoid double-free)
    AbcNodes(const AbcNodes&) = delete;
    AbcNodes& operator=(const AbcNodes&) = delete;

    // Range-based loop support
    abc::Abc_Obj_t** begin() { return reinterpret_cast<abc::Abc_Obj_t**>(vNodes->pArray); }
    abc::Abc_Obj_t** end()   { return reinterpret_cast<abc::Abc_Obj_t**>(vNodes->pArray) + vNodes->nSize; }
    
    size_t size() const { return static_cast<size_t>(vNodes->nSize); }
  };

  
  class AIG
  {
    abc::Abc_Ntk_t* aig=abc::Abc_NtkAlloc( abc::ABC_NTK_STRASH, abc::ABC_FUNC_AIG, 1 );
    void operator=(const AIG&)=delete;
    AIG(const AIG&)=delete;
    abc::Abc_Aig_t* aigMan() const {return reinterpret_cast<abc::Abc_Aig_t*>(aig->pManFunc);}
  public:
    AIG()=default;
    ~AIG() {abc::Abc_NtkDelete(aig);}
    void setInputs(unsigned numInputs) {
      for (unsigned i=0; i<numInputs; ++i)
        abc::Abc_NtkCreatePi(aig);
    }
    unsigned numInputs() const {return abc::Abc_NtkPiNum(aig);}
    abc::Abc_Obj_t& input(unsigned i) const {
      assert(i<numInputs());
      return *abc::Abc_NtkPi(aig,i);
    }
    abc::Abc_Obj_t& addOutputs(abc::Abc_Obj_t& expr) {
      auto r=abc::Abc_NtkCreatePo(aig);
      abc::Abc_ObjAddFanin(r, &expr);
      return *r;
    }
    unsigned numOutputs() const {return abc::Abc_NtkPoNum(aig);}
    abc::Abc_Obj_t& output(unsigned i) const {
      assert(i<numOutputs());
      return *abc::Abc_NtkPo(aig,i);
    }
    
    /// add an And gate to x & y,returning the resultant output. x& y must be owned by this.
    abc::Abc_Obj_t& addAnd(abc::Abc_Obj_t& x, abc::Abc_Obj_t& y) {
      return *abc::Abc_AigAnd(aigMan(), &x, &y);
    }
    abc::Abc_Obj_t& addOr(abc::Abc_Obj_t& x, abc::Abc_Obj_t& y) {
      return *abc::Abc_AigOr(aigMan(), &x, &y);
    }

    unsigned numGates() const {return abc::Abc_NtkNodeNum(aig);}
//    void execute(const char* cmd) {
//      auto frame=Abc_FrameGetGlobalFrame();
//      Abc_FrameReplaceCurrentNetwork(frame, aig);
//      abc::Cmd_CommandExecute(aig,cmd);
//      
//    }

    void cleanup() {
      abc::Abc_AigCleanup(aigMan());
    }
    void balance() {
      auto opt=abc::Abc_NtkBalance(aig, 0, 0, 0);
      abc::Abc_NtkDelete(aig);
      aig=opt;
    }
    void rewrite(bool useZeroCostRewrites=false) {
      if (!abc::Abc_NtkRewrite(aig, 0, useZeroCostRewrites, 0, 0, 0))
        throw std::runtime_error("Rewrite failed");
    }
    void refactor() {
      if (!abc::Abc_NtkRefactor(aig, 10, 1, 16, 0, 0, 0, 0))
        throw std::runtime_error("Refactor failed");
    }
    template <class I>
    I eval(const std::vector<I>& input) const {
      assert(input.size()==numInputs());
      assert(numOutputs()==1);
      std::vector<I> workspace(Abc_NtkObjNumMax(aig));
      for (unsigned i=0; i<numInputs(); ++i)
        workspace[abc::Abc_ObjId(&input(i))]=input[i];
      AbcNodes orderedNodes(aig);
      for (auto obj: orderedNodes)
        if (obj)
          {          
            auto v0=workspace[abc::Abc_ObjFaninId0(obj)];
            auto v1=workspace[abc::Abc_ObjFaninId1(obj)];
            // Apply inversions if the edges are complemented
            if (abc::Abc_ObjFaninC0(obj)) v0 = ~v0;
            if (abc::Abc_ObjFaninC1(obj)) v1 = ~v1;
            workspace[abc::Abc_ObjId(obj)] = v0 & v1;
          }
      auto r=workspace[abc::Abc_ObjId(&output(0))];
      return abc::Abc_ObjFaninC0(&output(0))? ~r: r;
    }
  };
}

#endif
