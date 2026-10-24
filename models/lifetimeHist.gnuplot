set logscale x
set logscale y
alpha=1.147
plot 'lifetimes' using 1:(100) smooth frequency with boxes, 50000*1000**alpha*x**-alpha
pause -1
