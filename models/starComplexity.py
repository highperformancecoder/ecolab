from starComplexity import starC
nodes=5
# computed from max_{l\in[0,L]}min(n+L-l,2l) where L=n(n-1)/2
maxStars=0
L=int(0.5*nodes*(nodes-1))
for l in range(L):
    v=min(nodes+L-l, 2*l)
    maxStars=max(maxStars,v)

print('maxStars=',maxStars)
maxStars=8

starC.generateElementaryStars(nodes)
starC.fillStarMap(maxStars)
starC.canonicaliseStarMap();
