#import ecolab
#print(ecolab.device())

from starComplexity import starC
from datetime import datetime
nodes=7
# computed from max_{l\in[0,L]}min(n+L-l,2l) where L=n(n-1)/2
maxStars=0
L=int(0.5*nodes*(nodes-1))
for l in range(L):
    v=min(nodes+L-l, 2*l)
    maxStars=max(maxStars,v)

print('maxStars=',maxStars)
maxStars=6

#starC.blockSize(256)
starC.blockSize(4096)
#starC.blockSize(8192)
#starC.blockSize(17920)
#starC.blockSize(40320)

starC.generateElementaryStars(nodes)
for numStars in range(1,maxStars+1):
    starC.fillStarMap(numStars)
    print('completed',numStars,datetime.now())
    starC.canonicaliseStarMap()
    with open(f'{nodes}-{maxStars}.csv','w') as out:
        print('id,links','*','C','C*','omega(g)','omega(g\')',sep=',',file=out)
        id=0
        for i in starC.starMap.keys():
            c=starC.complexity(i)
            links=0
            for j in i:
                links+=bin(j).count('1')

            #print(id,links,starC.symmStar(i)-1,c.complexity(),c.starComplexity(),starC.counts[i],sep=',',file=out)
            print(id,bin(i[0]),starC.starMap[i],starC.starUpperBound(i));
            id+=1
