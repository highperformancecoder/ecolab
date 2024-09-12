from weakref import ref
from tkinter import Tk, ttk, Button
# list of windows to update each simulation step
windows=[]
runner=Tk()
runner.title('EcoLab')
statusBar=ttk.Label(runner,text='Not started')
statusBar.pack()

class Simulator:
    running=False
    def __init__(self,step):
        self.step=step
        
    def __call__(self):
        self.running=True
        while self.running:
            self.step()
            for i in windows:
                win=i()
                if win: win.update()

    def stop(self):
        self.running=False
    def exit(self):
        self.stop()
        exit()

def gui(step):
    simulator=Simulator(step)
    windows.append(ref(runner))
    buttonBar=ttk.Frame(runner)
    buttonBar.pack()
    ttk.Button(buttonBar,text="quit",command=simulator.exit).pack(side='left')
    ttk.Button(buttonBar,text="run",command=simulator).pack(side='left')
    ttk.Button(buttonBar,text="step",command=step).pack(side='left')
    ttk.Button(buttonBar,text="stop",command=simulator.stop).pack(side='left')
    runner.mainloop()
