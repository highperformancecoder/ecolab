import powerlaw, numpy
import matplotlib.pyplot as plt
from sys import argv
extinctions=[]
with open(argv[1],"r") as f:
    for line in f:
        extinctions.append(line)
        #        print(line[0])
#        if line[0]=='#': continue
#        data=line.split()
#        if data[0]=='0': continue
#        extinctions.append(data[0])
#        print(data[0])
        
fit=powerlaw.Fit(numpy.array(extinctions),discrete=True)
print(fit.power_law.alpha,fit.power_law.xmin)
R, p = fit.distribution_compare('power_law', 'lognormal')
print('R, p=',R,p)

fig, ax = plt.subplots()
fit.plot_pdf(ax=ax, label='PDF')
fit.power_law.plot_pdf(ax=ax, label='Power law fit')
plt.legend()
plt.show()
