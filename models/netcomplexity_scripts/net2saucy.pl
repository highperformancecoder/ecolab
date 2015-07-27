#!/usr/bin/perl
$reading_arcs=0;
while (<>)
{
    if (/\*Vertices/)
    {
        ($dummy,$nodes)=split;
    }
    if (/\*Arcs/)
    {
        $reading_arcs=1;
    }
    elsif ($reading_arcs)
    {
        push @arcs,$_;
    }
}

print "$nodes ",$#arcs+1," 1\n";
foreach $edge (@arcs) 
{
    ($source,$target)=split / /,$edge;
    print $source-1," ",$target-1,"\n";
}
