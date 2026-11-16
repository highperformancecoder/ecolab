set logscale x
set logscale y
set style data line
alpha=0.923
C=1.1e6
plot "lifetimeHist.dat", C*(1000/x)**alpha
