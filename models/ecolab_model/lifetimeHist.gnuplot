set logscale x
set logscale y
alpha=0.923
C=1.1e6
#plot 'lifetimes' using 1:(100) smooth frequency with boxes,C*(1000/x)**alpha
binwidth=1000
bin(val)=binwidth*ceil(val/binwidth)
plot 'lifetimes' using (bin($1)) smooth frequency with line,C*(1000/x)**alpha
#pause -1
