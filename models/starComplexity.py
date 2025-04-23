import ecolab
print(ecolab.device())

from starComplexity import starC
nodes=7
# computed from max_{l\in[0,L]}min(n+L-l,2l) where L=n(n-1)/2
maxStars=0
L=int(0.5*nodes*(nodes-1))
for l in range(L):
    v=min(nodes+L-l, 2*l)
    maxStars=max(maxStars,v)

print('maxStars=',maxStars)
maxStars=9

starC.blockSize(128)

starC.generateElementaryStars(nodes)
starC.fillStarMap(maxStars)
starC.canonicaliseStarMap();
