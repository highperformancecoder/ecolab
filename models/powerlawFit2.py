from sys import path, argv
path.append('../lib')
from ecolab import HistoStats, Plot
import numpy
import matplotlib.pyplot as plt

hist=HistoStats()
with open(argv[1],"r") as f:
    for line in f:
        hist.appendData(float(line.rstrip()))

[alpha,xmin]=hist.fitPowerLaw(500)();
print(alpha,xmin)
