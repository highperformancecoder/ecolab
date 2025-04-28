#import ecolab
#print(ecolab.device())

from starComplexity import starC
#nodes=7
nodes=4
# computed from max_{l\in[0,L]}min(n+L-l,2l) where L=n(n-1)/2
maxStars=0
L=int(0.5*nodes*(nodes-1))
for l in range(L):
    v=min(nodes+L-l, 2*l)
    maxStars=max(maxStars,v)

print('maxStars=',maxStars)
#maxStars=10

starC.blockSize(1)
#starC.blockSize(256)
#starC.blockSize(4096)
#starC.blockSize(2048)
#starC.blockSize(40320)

starC.generateElementaryStars(nodes)
starC.fillStarMap(maxStars)
print(len(starC.starMap))
starC.canonicaliseStarMap()
print(starC.starMap())
