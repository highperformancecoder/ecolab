set logscale x
set logscale y
alpha=0.923
#plot 'lifetimes' using 1:(100) smooth frequency with boxes, 50000*1000**alpha*x**-alpha
plot 'lifetimes' using 1:(100) smooth frequency with line, 50000*1000**alpha*x**-alpha
#pause -1
