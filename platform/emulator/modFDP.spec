%builtins_all =
(

 #
 # Distribution stuff
 #
 'selVarNaive'  => { in     => ['+tuple'],
                             out    => ['*int'],
                             bi     => BIfdd_selVarNaive},

 'selVarSize'   => { in     => ['+tuple'],
                             out    => ['*int'],
                             bi     => BIfdd_selVarSize},

 'selVarMin'    => { in     => ['+tuple'],
                             out    => ['*int'],
                             bi     => BIfdd_selVarMin},

 'selVarMax'    => { in     => ['+tuple'],
                             out    => ['*int'],
                             bi     => BIfdd_selVarMax},

 'selVarNbSusps' => { in     => ['+tuple'],
                          out    => ['*int'],
                          bi     => BIfdd_selVarNbSusps},

 #
 # Propagators
 #

 'sum'          => { in  => ['+value','+atom','int'],
                             out => [],
                             bi  => fdp_sum},

 'sumC'         => { in  => ['+value','+value','+atom','int'],
                             out => [],
                             bi  => fdp_sumC},

 'sumCN'                => { in  => ['+value','+value','+atom','int'],
                             out => [],
                             bi  => fdp_sumCN},

 'sumR'         => { in  => ['+value','+atom','*int','*int'],
                             out => [],
                             bi  => fdp_sumR},

 'sumCR'                => { in  => ['+value','+value','+atom','*int','*int'],
                             out => [],
                             bi  => fdp_sumCR},

 'sumCNR'               => { in  => ['+value','+value','+atom','*int','*int'],
                             out => [],
                             bi  => fdp_sumCNR},

 'sumCD'                => { in  => ['+value','+atom','*int','*int'],
                             out => [],
                             bi  => fdp_sumCD},

 'sumCCD'       => { in  => ['+value','+value','+atom','*int','*int'],
                     out => [],
                     bi  => fdp_sumCCD},

 'sumCNCD'      => { in  => ['+value','+value','+atom','*int','*int'],
                     out => [],
                     bi  => fdp_sumCNCD},

 'plus'         => { in  => ['int','int','int'],
                             out => [],
                             bi  => fdp_plus},

 'minus'                => { in  => ['int','int','int'],
                             out => [],
                             bi  => fdp_minus},

 'times'                => { in  => ['int','int','int'],
                             out => [],
                             bi  => fdp_times},

 'power'                => { in  => ['int','+int','int'],
                             out => [],
                             bi  => fdp_power},

 'divD'         => { in  => ['int','+int','int'],
                             out => [],
                             bi  => fdp_divD},

 'divI'         => { in  => ['int','+int','int'],
                             out => [],
                             bi  => fdp_divI},

 'modD'         => { in  => ['int','+int','int'],
                             out => [],
                             bi  => fdp_modD},

 'modI'         => { in  => ['int','+int','int'],
                             out => [],
                             bi  => fdp_modI},

 'conj'         => { in  => ['int','int','int'],
                             out => [],
                             bi  => fdp_conj},

 'disj'         => { in  => ['int','int','int'],
                             out => [],
                             bi  => fdp_disj},

 'exor'         => { in  => ['int','int','int'],
                             out => [],
                             bi  => fdp_exor},

 'impl'         => { in  => ['int','int','int'],
                             out => [],
                             bi  => fdp_impl},

 'equi'         => { in  => ['int','int','int'],
                             out => [],
                             bi  => fdp_equi},

 'nega'         => { in  => ['int','int'],
                             out => [],
                             bi  => fdp_nega},

 'intR'         => { in  => ['int','+value','int'],
                             out => [],
                             bi  => fdp_intR},

 'card'         => { in  => ['+value','int','int','int'],
                             out => [],
                             bi  => fdp_card},

 'exactly'      => { in  => ['int','+value','+int'],
                     out => [],
                     bi  => fdp_exactly},

 'atLeast'      => { in  => ['int','+value','+int'],
                     out => [],
                     bi  => fdp_atLeast},

 'atMost'       => { in  => ['int','+value','+int'],
                     out => [],
                     bi  => fdp_atMost},

 'element'      => { in  => ['int','+value','int'],
                     out => [],
                     bi  => fdp_element},

 'lessEqOff'    => { in  => ['int','int','+int'],
                             out => [],
                             bi  => fdp_lessEqOff},

 'minimum'      => { in  => ['int','int','int'],
                     out => [],
                     bi  => fdp_minimum},

 'maximum'      => { in  => ['int','int','int'],
                     out => [],
                     bi  => fdp_maximum},

 'inter'        => { in  => ['int','int','int'],
                     out => [],
                     bi  => fdp_inter},

 'union'        => { in  => ['int','int','int'],
                     out => [],
                     bi  => fdp_union},

 'distinct'     => { in  => ['+value'],
                     out => [],
                     bi  => fdp_distinct},

 'distinctD'    => { in  => ['+value'],
                             out => [],
                             bi  => fdp_distinctD},

 'distinctStream'=> { in  => ['+value','value'],
                          out => [],
                          bi  => fdp_distinctStream},

 'distinctOffset'=> { in  => ['+value','+value'],
                          out => [],
                          bi  => fdp_distinctOffset},

 'disjoint'=> { in  => ['int','+int','int','+int'],
                    out => [],
                    bi  => fdp_disjoint},

 'disjointC'=> { in  => ['int','+int','int','+int','int'],
                     out => [],
                     bi  => fdp_disjointC},

 'distance'     => { in  => ['int','int','+atom','int'],
                     out => [],
                     bi  => fdp_distance},

 'distinct2'    => { in  => ['+value','+value','+value','+value'],
                             out => [],
                             bi  => fdp_distinct2},

 'subset'       => { in  => ['int','int'],
                     out => [],
                     bi  => fdp_subset},

 'dsum'         => { in  => ['+value','+atom','int'],
                             out => [],
                             bi  => fdp_dsum},

 'dsumC'                => { in  => ['+value','+value','+atom','int'],
                             out => [],
                             bi  => fdp_dsumC},

 'sumAC'                => { in  => ['+value','+value','+atom','int'],
                             out => [],
                             bi  => fdp_sumAC},

 );
