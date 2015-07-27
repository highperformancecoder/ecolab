/*
  @copyright Russell Standish 2000-2013
  @author Russell Standish
  This file is part of EcoLab

  Open source licensed under the MIT license. See LICENSE for details.
*/

/**\file
\brief TCL_obj support for Graphcode
*/
#ifndef TCL_OBJ_GRAPHCODE_H
#define TCL_OBJ_GRAPHCODE_H
#include "TCL_obj_stl.h"
#include "graphcode.h"

namespace ecolab
{
  using classdesc::TCL_obj_t;

  inline void keys_of(const GRAPHCODE_NS::omap& o)
  { 
    tclreturn r;
    for (GRAPHCODE_NS::omap::const_iterator i=o.begin(); i!=o.end(); i++)
    r<<i->ID;
  }

  template <>
  class TCL_obj_of<GRAPHCODE_NS::omap,GRAPHCODE_NS::GraphID_t>: TCL_obj_of_base
  {
    GRAPHCODE_NS::omap& obj;
    string desc;
  public:
    TCL_obj_of(GRAPHCODE_NS::omap& o, const string& d): obj(o), desc(d) {}
    string operator()(const char* index) 
    {
      string elname=desc+"("+index+")";
      GRAPHCODE_NS::object *Tobj;
      if ((Tobj=dynamic_cast<GRAPHCODE_NS::object*>
           (&*(obj[idx<GRAPHCODE_NS::GraphID_t>()(index)]))))
        Tobj->TCL_obj(elname);
      return elname;
    }
    void keys_of() {ecolab::keys_of(obj);}
  };

  void TCL_obj(TCL_obj_t& targ, const string& desc, GRAPHCODE_NS::omap& arg)
  {
    TCL_obj(targ,desc+".size",arg,&GRAPHCODE_NS::omap::size);
    Tcl_CreateCommand(interp(),(desc+".@is_map").c_str(),
                      (Tcl_CmdProc*)null_proc,NULL,NULL);
    ClientData c=(ClientData)new TCL_obj_of<GRAPHCODE_NS::omap, GRAPHCODE_NS::GraphID_t>(arg,desc);
    Tcl_CreateCommand(interp(),(desc+".@elem").c_str(),(Tcl_CmdProc*)elem,c,
                      (Tcl_CmdDeleteProc*)del_obj);
    Tcl_CreateCommand(interp(),(desc+".#keys").c_str(),(Tcl_CmdProc*)keys,c,
                      (Tcl_CmdDeleteProc*)del_obj);
    //  c=(ClientData)new TCL_obj_of_count<GRAPHCODE_NS::omap,GRAPHCODE_NS::GraphID_t>(arg,desc);
    //  Tcl_CreateCommand(interp,(desc+".count").c_str(),(Tcl_CmdProc*)elem,c,
    //		    (Tcl_CmdDeleteProc*)del_obj);
  }
}

#endif
