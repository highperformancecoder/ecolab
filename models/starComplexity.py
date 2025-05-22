#import ecolab
#print(ecolab.device())

from starComplexity import starC
nodes=11
# computed from max_{l\in[0,L]}min(n+L-l,2l) where L=n(n-1)/2
maxStars=0
L=int(0.5*nodes*(nodes-1))
for l in range(L):
    v=min(nodes+L-l, 2*l)
    maxStars=max(maxStars,v)

print('maxStars=',maxStars)
maxStars=9

#starC.blockSize(256)
#starC.blockSize(4096)
starC.blockSize(17920)
#starC.blockSize(40320)

starC.generateElementaryStars(nodes)
starC.fillStarMap(maxStars)
starC.canonicaliseStarMap()

print('g','links','*','C','C*','omega(g)','omega(g\')',sep=',')
for i in starC.starMap.keys():
    c=starC.complexity(i)
    print(i,bin(i).count('1'),starC.symmStar(i),c.complexity(),c.starComplexity(),starC.counts[i],starC.counts[starC.complement(i)],sep=',')

