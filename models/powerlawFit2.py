from sys import path, argv
path.append('../lib')
from ecolab import HistoStats, Plot
import numpy
import matplotlib.pyplot as plt

hist=HistoStats()
i=0
with open(argv[1],"r") as f:
    for line in f:
        x=float(line.rstrip())
        hist.appendData(float(line.rstrip()))
        i+=1
        if i % 10000 == 0:
            print(i,end='\r')
        
[alpha,xmin]=hist.fitFatPowerLaw(1e6)();
print(alpha,xmin)
[alpha,xmin]=hist.fitPowerLaw(1000)();
print(alpha,xmin)

hist.logbins(True)
hist.nbins(100)
bins=hist.bins()()
histogram=hist.histogram()()
with open("lifetimeHist.dat","w") as o:
    for i in range(len(bins)):
        print(bins[i],histogram[i],file=o)
