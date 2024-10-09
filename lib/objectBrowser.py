import ecolab
from tkinter import Tk,ttk,Frame,Label,Listbox,Text
import json

# executed when mouse clicked
def doMethod(event):
    listbox=event.widget
    selectedItems=listbox.curselection()
    if len(selectedItems)==0: return
    index=selectedItems[0]
    selection=listbox.get(index)
    if selection[-1]=="▶":
        # browse the compound object
        member=getattr(listbox.object,selection[:-1])
        Browser(member,selection[:-1])
    else:
        selection=selection.split('=')[0]
        
        args=json.loads('['+listbox.args.get('1.0','end')+']')
        val=getattr(listbox.object,selection)(*args)
        listbox.delete(index)
        listbox.insert(index,selection+'='+str(val))
        listbox.itemconfigure(index,foreground='blue')

class Browser:
    def __init__(self, object, title=''):
        if str(type(object))!="<class 'CppWrapperType'>":
            raise RuntimeError("Cannot browse non-C++ wrapped objects")

        # separate attributes into members and methods
        members=set()
        methods=set()
        for m in object._list():
            mm=m[1:].split('.')
            if len(mm)==1:
                methods.add(mm[0])
            else:
                members.add(mm[0]+"▶")

        self.browser=Tk()
        self.browser.wm_title(title)
        argFrame=Frame(self.browser)
        argLabel=Label(argFrame,text="Args")
        argLabel.pack(side='left')
        args=Text(argFrame,height=1,width=15)
        args.pack(side='left')
        argFrame.pack()
        listbox=Listbox(self.browser)
        listbox.insert('end',*sorted(members))
        listbox.insert('end',*sorted(methods))
        for i in range(len(members)+len(methods)):
            listbox.itemconfigure(i,foreground='red' if i<len(members) else 'blue')
        listbox.pack(fill='both',expand=True)

        listbox.object=object
        listbox.args=args
        listbox.bind('<Double-Button-1>',doMethod)

    
