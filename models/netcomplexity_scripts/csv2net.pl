$node=0;
$row = 0;
while (<>)
{
    split/,/;
    if ($nodes==0)
    {
        $nodes = $#_+1;
        print "*Vertices $nodes\r\n";
        for ($i=1; $i<=$nodes; $i++)
        {
            print "$i $i\r\n";
        }
        print "*Arcs\r\n";
    }
    $row++;
    for ($i=1; $i<=$nodes; $i++)
    {
        if ($_[$i-1]==1 && $i != $row)
        {
            print "$row $i\r\n";
        }
    }
}
