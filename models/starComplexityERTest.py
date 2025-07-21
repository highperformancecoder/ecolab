from starComplexity import starC

nodes=1000

out=open(f'ER-{nodes}.csv', 'w')
print("id,links,C, bar{*}", file=out)
for i in range(1000):
    links=int(nodes*(nodes-1)//2*starC.uni.rand())
    g=starC.randomERGraph(nodes, links)
    print(links,g.complexity(), g.starComplexity(),)
    print(i,links,g.complexity(), g.starComplexity(),sep=', ',file=out)
