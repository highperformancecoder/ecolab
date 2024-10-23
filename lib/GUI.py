from weakref import ref
from tkinter import Tk, ttk, Button
from ecolab import registerParallel
registerParallel()
from ecolab import Parallel, myid
from objectBrowser import Browser
from math import inf
from time import perf_counter

# list of windows to update each simulation step
windows=[]
runner=Tk()
runner.title('EcoLab')
statusFrame=ttk.Frame(runner)
statusFrame.pack()
statusBar=ttk.Label(statusFrame,text='Not started')
statusBar.pack(side='left')
performance=ttk.Label(statusFrame,text='perf: 0')
performance.pack(side='left')

class Simulator:
    running=False
    def __init__(self,step):
        self.step=step
        self.parallel=Parallel(self)
        
    def __call__(self):
        self.running=True
        while self.running:
            start=perf_counter()
            self.doStep()
            timePerStep=perf_counter()-start
            performance.configure(text='perf: %f'%(1.0/timePerStep))
            for i in windows:
                win=i()
                if win: win.update()

    def doStep(self):
        self.parallel("step");
    def stop(self):
        self.running=False
    def exit(self):
        self.stop()
        self.parallel.exit()
        exit()

def gui(step,model=None):
    simulator=Simulator(step)
    if myid()>0: return
    windows.append(ref(runner))
    buttonBar=ttk.Frame(runner)
    buttonBar.pack()
    ttk.Button(buttonBar,text="quit",command=simulator.exit).pack(side='left')
    ttk.Button(buttonBar,text="run",command=simulator).pack(side='left')
    ttk.Button(buttonBar,text="step",command=simulator.doStep).pack(side='left')
    ttk.Button(buttonBar,text="stop",command=simulator.stop).pack(side='left')
    if model!=None:
        ttk.Button(buttonBar,text="browse",command=lambda: Browser(model)).pack(side='left')
    try:
        runner.mainloop()
    except SystemExit:
        pass
