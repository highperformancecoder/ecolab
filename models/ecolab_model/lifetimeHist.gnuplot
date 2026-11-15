set logscale x
set logscale y
alpha=1.25
C=80000
binwidth=1000
bin(val)=ceil(val/binwidth)
plot 'lifetimes' using (bin($1)) smooth frequency with line, C*(1000/x)**alpha
#pause -1
