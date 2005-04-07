#!/usr/bin/perl -w
# ===================================================================
# @(#)cbgp-validation.pl
#
# This script is used to validate C-BGP. It performs various tests in
# order to detect erroneous behaviour.
#
# @author Bruno Quoitin (bqu@info.ucl.ac.be)
# @lastdate 07/04/2005
# ===================================================================

use strict;

use Getopt::Long;
use CBGP;

use constant VERSION => '1.0';
use constant CACHE_FILE => ".cbgp-validation.cache";

use constant TEST_FAILURE => 0;
use constant TEST_SUCCESS => 1;
use constant TEST_SKIPPED => 2;

use constant CBGP_RT_NEXTHOP => 0;
use constant CBGP_RT_METRIC => 1;
use constant CBGP_RT_PROTO => 2;

use constant CBGP_TR_SRC => 0;
use constant CBGP_TR_DST => 1;
use constant CBGP_TR_STATUS => 2;
use constant CBGP_TR_PATH => 3;

use constant CBGP_RIB_STATUS => 0;
use constant CBGP_RIB_PREFIX => 1;
use constant CBGP_RIB_NEXTHOP => 2;
use constant CBGP_RIB_PREF => 3;
use constant CBGP_RIB_MED => 4;
use constant CBGP_RIB_PATH => 5;
use constant CBGP_RIB_ORIGIN => 6;
use constant CBGP_RIB_COMMUNITY => 7;
#use constant CBGP_RIB_ => 8;

use constant MRT_PREFIX => 5;
use constant MRT_NEXTHOP => 8;

my $num_failures= 0;
my $num_warnings= 0;

my $max_failures= 1;

my $cbgp_path= "../src/cbgp";
my $cbgp_version;

my %cache;
my $use_cache= 1;

# -----[ show_error ]------------------------------------------------
sub show_error($)
{
    my ($msg)= @_;

    print STDERR "Error: \033[1;31m$msg\033[0m\n";
}

# -----[ show_warning ]----------------------------------------------
sub show_warning($)
{
    my ($msg)= @_;

    print STDERR "Warning: \033[1;33m$msg\033[0m\n";
    $num_warnings++;
}

# -----[ show_info ]-------------------------------------------------
sub show_info($)
{
    my ($msg)= @_;

    print STDERR "Info: \033[37;1m$msg\033[0m\n";
}

# -----[ show_testing ]----------------------------------------------
sub show_testing($)
{
    my ($msg)= @_;

    print STDERR "Testing: \033[37;1m$msg\033[0m";
}

# -----[ show_success ]----------------------------------------------
sub show_success()
{
    print STDERR "\033[70G\033[32;1mSUCCESS\033[0m\n";
}

# -----[ show_cache ]------------------------------------------------
sub show_cache()
{
    print STDERR "\033[70G\033[32;1m(CACHE)\033[0m\n";
}

# -----[ show_failure ]----------------------------------------------
sub show_failure()
{
    print STDERR "\033[70G\033[31;1mFAILURE\033[0m\n";
    $num_failures++;
}

# -----[ show_skipped ]----------------------------------------------
sub show_skipped()
{
    print STDERR "\033[70G\033[33;1mSKIPPED\033[0m\n";
}

# -----[ test_cache_read ]-------------------------------------------
sub test_cache_read()
{
    if (open(CACHE, "<".CACHE_FILE)) {
	while (<CACHE>) {
	    chomp;
	    my ($test_result, $test_name)= split /\t/;
	    (exists($cache{$test_name})) and
		die "duplicate test in cache file";
	    $cache{$test_name}= $test_result;
	}
	close(CACHE);
    }
}

# -----[ test_cache_write ]------------------------------------------
sub test_cache_write()
{
    if (open(CACHE, ">".CACHE_FILE)) {
	foreach my $test (keys %cache)
	{
	    print CACHE "$cache{$test}\t$test\n";
	}
	close(CACHE);
    }
}

# -----[ topo_from_ntf ]---------------------------------------------
sub topo_from_ntf($)
{
    my ($ntf_file)= @_;
    my %topo;
    
    if (!open(NTF, "<$ntf_file")) {
	show_error("unable to open \"$ntf_file\": $!");
	exit(-1);
    }
    while (<NTF>) {
	chomp;
	(m/^\#/) and next;
	my @fields= split /\s+/;
	if (@fields < 3) {
	    show_error("invalid NTF record");
	    exit(-1);
	}
	if (@fields == 3) {
	    $topo{$fields[0]}{$fields[1]}= [$fields[2], $fields[2]];
	} else {
	    $topo{$fields[0]}{$fields[1]}= [$fields[2], $fields[3]];
	}
    }
    close(NTF);

    return \%topo;
}

# -----[ topo_2nodes ]-----------------------------------------------
sub topo_2nodes()
{
    my %topo;
    $topo{'1.0.0.1'}{'1.0.0.2'}= [10, 10];
    return \%topo;
}

# -----[ topo_3nodes_triangle ]--------------------------------------
sub topo_3nodes_triangle()
{
    my %topo;
    $topo{'1.0.0.1'}{'1.0.0.2'}= [10, 10];
    $topo{'1.0.0.1'}{'1.0.0.3'}= [10, 5];
    $topo{'1.0.0.2'}{'1.0.0.3'}= [10, 4];
    return \%topo;
}

# -----[ topo_3nodes_line ]------------------------------------------
sub topo_3nodes_line()
{
    my %topo;
    $topo{'1.0.0.1'}{'1.0.0.2'}= [10, 10];
    $topo{'1.0.0.2'}{'1.0.0.3'}= [10, 10];
    return \%topo;
}

# -----[ topo_get_nodes ]--------------------------------------------
sub topo_get_nodes($)
{
    my ($topo)= @_;
    my %nodes;

    foreach my $node1 (keys %$topo) {
	foreach my $node2 (keys %{$topo->{$node1}}) {
	    (!exists($nodes{$node1})) and $nodes{$node1}= 1;
	    (!exists($nodes{$node2})) and $nodes{$node2}= 1;
	}
    }
    return \%nodes;
}

# -----[ topo_included ]---------------------------------------------
sub topo_included($$)
{
    my ($topo1, $topo2)= @_;

    foreach my $node1 (keys %$topo1) {
	foreach my $node2 (keys %{$topo1->{$node1}}) {
	    my $link1= $topo1->{$node1}{$node2};
	    my $link2;
	    if (exists($topo2->{$node1}{$node2})) {
		$link2= $topo2->{$node1}{$node2};
	    } elsif (exists($topo2->{$node2}{$node1})) {
		$link2= $topo2->{$node2}{$node1};
	    }
	    if (!defined($link2)) {
		print "missing $node1 $node2\n";
		return 0;
	    }
	    if (@$link1 != @$link2) {
		print "incoherent $node1 $node2\n";
		return 0;
	    }
	    for (my $index= 0; $index < @$link1; $index++) {
		if ($link1->[$index] != $link2->[$index]) {
		    print "incoherent [$index] $node1 $node2\n";
		    return 0;
		}
	    }
	}
    }
    return 1;
}

# -----[ cbgp_topo ]-------------------------------------------------
sub cbgp_topo($$)
{
    my ($cbgp, $topo)= @_;
    my %nodes;

    foreach my $node1 (keys %$topo) {
	foreach my $node2 (keys %{$topo->{$node1}}) {
	    my ($delay, $weight)= @{$topo->{$node1}{$node2}};
	    if (!exists($nodes{$node1})) {
		die if $cbgp->send("net add node $node1\n");
		$nodes{$node1}= 1;
	    }
	    if (!exists($nodes{$node2})) {
		die if $cbgp->send("net add node $node2\n");
		$nodes{$node2}= 1;
	    }
	    die if $cbgp->send("net add link $node1 $node2 $delay\n");
	    die if $cbgp->send("net link $node1 $node2 igp-weight $weight\n");
	    die if $cbgp->send("net link $node2 $node1 igp-weight $weight\n");
	}
    }
}

# -----[ cbgp_topo_filter ]------------------------------------------
sub cbgp_topo_filter($)
{
    my ($cbgp)= @_;

    die if $cbgp->send("net add node 1.0.0.1\n");
    die if $cbgp->send("net add node 2.0.0.1\n");
    die if $cbgp->send("net add link 1.0.0.1 2.0.0.1 0\n");
    die if $cbgp->send("net node 1.0.0.1 route add 2.0.0.1/32 2.0.0.1 0\n");
    die if $cbgp->send("bgp add router 1 1.0.0.1\n");
    cbgp_peering($cbgp, "1.0.0.1", "2.0.0.1", 2, "virtual");
}

# -----[ cbgp_topo_dp ]----------------------------------------------
sub cbgp_topo_dp($$)
{
    my ($cbgp, $num_peers)= @_;

    die if $cbgp->send("net add node 1.0.0.1\n");
    die if $cbgp->send("bgp add router 1 1.0.0.1\n");
    for (my $index= 0; $index < $num_peers; $index++) {
	my $asn= $index+2;
	my $peer= "$asn.0.0.1";
	die if $cbgp->send("net add node $peer\n");
	die if $cbgp->send("net add link 1.0.0.1 $peer 0\n");
	die if $cbgp->send("net node 1.0.0.1 route add $peer/32 $peer 0\n");
	cbgp_peering($cbgp, "1.0.0.1", $peer, $asn, "virtual");
    }
}

# -----[ cbgp_topo_dp2 ]---------------------------------------------
sub cbgp_topo_dp2($$)
{
    my ($cbgp, $num_peers)= @_;

    die if $cbgp->send("net add node 1.0.0.1\n");
    die if $cbgp->send("bgp add router 1 1.0.0.1\n");
    for (my $index= 0; $index < $num_peers; $index++) {
	my $peer= "1.0.0.".($index+2);
	die if $cbgp->send("net add node $peer\n");
	die if $cbgp->send("net add link 1.0.0.1 $peer ".($index+1)."\n");
	die if $cbgp->send("net node 1.0.0.1 route add $peer/32 $peer 0\n");
	cbgp_peering($cbgp, "1.0.0.1", $peer, "1", "virtual");
    }
}

# -----[ cbgp_topo_dp3 ]---------------------------------------------
sub cbgp_topo_dp3($;@)
{
    my ($cbgp, @peers)= @_;

    die if $cbgp->send("net add node 1.0.0.1\n");
    die if $cbgp->send("bgp add router 1 1.0.0.1\n");
    for (my $index= 0; $index < @peers; $index++) {
	my ($peer, $asn, $weight)= @{$peers[$index]};
	die if $cbgp->send("net add node $peer\n");
	die if $cbgp->send("net add link 1.0.0.1 $peer $weight\n");
	die if $cbgp->send("net node 1.0.0.1 route add $peer/32 $peer $weight\n");
	cbgp_peering($cbgp, "1.0.0.1", $peer, $asn, "virtual");
    }
}

# -----[ cbgp_topo_igp_compute ]-------------------------------------
sub cbgp_topo_igp_compute($$$)
{
    my ($cbgp, $topo, $prefix)= @_;
    my $nodes= topo_get_nodes($topo);

    foreach my $node (keys %$nodes) {
	die if $cbgp->send("net node $node spf-prefix $prefix\n");
    }
}

# -----[ cbgp_topo_bgp_routers ]-------------------------------------
sub cbgp_topo_bgp_routers($$$) {
    my ($cbgp, $topo, $asn)= @_;
    my $nodes= topo_get_nodes($topo);

    foreach my $node (keys %$nodes) {
	die if $cbgp->send("bgp add router $asn $node\n");
    }
}

# -----[ cbgp_topo_ibgp_sessions ]-----------------------------------
sub cbgp_topo_ibgp_sessions($$$)
{
    my ($cbgp, $topo, $asn)= @_;
    my $nodes= topo_get_nodes($topo);

    foreach my $node1 (keys %$nodes) {
	foreach my $node2 (keys %$nodes) {
	    ($node1 eq $node2) and next;
	    die if $cbgp->send("bgp router $node1 add peer $asn $node2\n");
	    die if $cbgp->send("bgp router $node1 peer $node2 up\n");
	}
    }
}

# -----[ cbgp_topo_check_bgp_networks ]------------------------------
sub cbgp_topo_check_bgp_networks($$;$;$)
{
    my ($cbgp, $topo, $prefixes, $x)= @_;
    my $nodes= topo_get_nodes($topo);

    if (!defined($prefixes)) {
	push @$prefixes, ("255.255.0.0/16");
    }

    foreach my $node1 (keys %$nodes) {

	# Add networks
	foreach my $prefix (@$prefixes) {
	    die if $cbgp->send("bgp router $node1 add network $prefix\n");
	    die if $cbgp->send("sim run\n");
	}
	
	# Check that the routes are present in each router's RIB
	foreach my $node2 (keys %$nodes) {
	    my $rib= cbgp_show_rib($cbgp, $node2);
	    if (scalar(keys %$rib) != @$prefixes) {
		print "invalid number of routes in RIB (".
		    scalar(keys %$rib)."), should be ".@$prefixes."\n";
		return 0;
	    }
	    foreach my $prefix (@$prefixes) {
		if (!exists($rib->{$prefix})) {
		    print "missing prefix $prefix :-(\n";
		    return 0;
		}
		# Nexthop should be the origin router
		if ($rib->{$prefix}->[CBGP_RIB_NEXTHOP] ne $node1) {
		    print "incorrect next-hop :-(\n";
		    return 0;
		}

		# Route should be marked as 'best' in all routers
		if (!($rib->{$prefix}->[CBGP_RIB_STATUS] =~ m/>/)) {
		    print "not best :-(\n";
		    return 0;
		}

		if ($node2 eq $node1) {
		    # Route should be marked as 'internal' in the origin router
		    if (!($rib->{$prefix}->[CBGP_RIB_STATUS] =~ m/i/)) {
			print "not internal :-(\n";
			return 0;
		    }
		} else {
		    # Route should be marked as 'reachable' in other routers
		    if (!($rib->{$prefix}->[CBGP_RIB_STATUS] =~ m/\*/)) {
			print "not reachable :-(\n";
			return 0;
		    }
		}
		# Check that the route is installed into the IP
		# routing table
		if ($node1 ne $node2) {
		    my $rt= cbgp_show_rt($cbgp, $node2);
		    if (!exists($rt->{$prefix}) ||
			($rt->{$prefix}->[CBGP_RT_PROTO] ne "BGP")) {
			print "route not installed in IP routing table\n";
			return 0;
		    }
		}
	    }
	}	

	# Remove networks
	foreach my $prefix (@$prefixes) {	
	    die if $cbgp->send("bgp router $node1 del network $prefix\n");
	    die if $cbgp->send("sim run\n");

	    # Check that the routes have been withdrawn
	    foreach my $node2 (keys %$nodes) {
		my $rib;
		# Check that the route is not installed into the Loc-RIB
		# (best)
		$rib= cbgp_show_rib($cbgp, $node2);
		if (scalar(keys %$rib) != 0) {
		    print "invalid number of routes in RIB (".
			scalar(keys %$rib)."), should be 0\n";
		    return 0;
		}
		# Check that the route is not present in the Adj-RIB-in
		$rib= cbgp_show_rib_in($cbgp, $node2);
		if (scalar(keys %$rib) != 0) {
		    print "invalid number of routes in Adj-RIB-in (".
			scalar(keys %$rib)."), should be 0\n";
		}
		# Check that the route has been uninstalled from the IP
		# routing table
		if ($node1 ne $node2) {
		    my $rt= cbgp_show_rt($cbgp, $node2);
		    if (exists($rt->{$prefix}) &&
			($rt->{$prefix}->[CBGP_RT_PROTO] eq "BGP")) {
			print "route still installed in IP routing table\n";
			return 0;
		    }
		}
	    }
	}

    }

    return 1;
}

# -----[ cbgp_topo_check_igp_reachability ]--------------------------
sub cbgp_topo_check_igp_reachability($$)
{
    my ($cbgp, $topo)= @_;
    my $nodes= topo_get_nodes($topo);

    foreach my $node1 (keys %$nodes) {
	my $routes= cbgp_show_rt($cbgp, $node1);
	foreach my $node2 (keys %$nodes) {
	    ($node1 eq $node2) and next;
	    if (!exists($routes->{"$node2/32"})) {
		return 0;
	    }
	    if ($routes->{"$node2/32"}->[CBGP_RT_PROTO] eq "IGP") {
		return 0;
	    }
	}
    }
    return 1;
}

# -----[ cbgp_topo_check_sessions ]----------------------------------
sub cbgp_topo_check_sessions($$;$)
{
    my ($cbgp, $topo, $state)= @_;
    my $nodes= topo_get_nodes($topo);

    foreach my $node1 (keys %$nodes) {
	my $peers= cbgp_show_peers($cbgp, $node1);
	foreach my $node2 (keys %$nodes) {
	    ($node1 eq $node2) and next;
	    if (!exists($peers->{$node2})) {
		return 0;
	    }
	    if (defined($state) &&
		($peers->{$node2}->[1] ne $state)) {
		print "$state\t$peers->{$node2}->[1]\n";
		return 0;
	    }
	}
    }
    return 1;
}

# -----[ cbgp_topo_check_record_route ]------------------------------
sub cbgp_topo_check_record_route($$$)
{
    my ($cbgp, $topo, $status)= @_;
    my $nodes= topo_get_nodes($topo);

    foreach my $node1 (keys %$nodes) {
	foreach my $node2 (keys %$nodes) {
	    ($node1 eq $node2) and next;
	    my $trace= cbgp_record_route($cbgp, $node1, $node2);
	    # Result should be equal to the given status
	    ($trace->[CBGP_TR_STATUS] ne $status) and return 0;
	    if ($trace->[CBGP_TR_STATUS] eq "SUCCESS") {
		# Last hop should be destination
		($#{$trace->[CBGP_TR_PATH]} eq $node2) and return 0;
	    }
	}
    }

    return 1;
}

# -----[ cbgp_topo_check_static_routes ]-----------------------------
sub cbgp_topo_check_static_routes($$)
{
    my ($cbgp, $topo)= @_;
    my $nodes= topo_get_nodes($topo);

    foreach my $node1 (keys %$nodes) {
	my $links= cbgp_show_links($cbgp, $node1);

	# Add a static route over each link
	foreach my $link (@$links) {
	    die if $cbgp->send("net node $node1 route add $link->[0]/32 $link->[0] $link->[1]\n");
	}

	# Test static routes (presence)
	my $rt= cbgp_show_rt($cbgp, $node1);
	(scalar(keys %$rt) != @$links) and return 0;
	foreach my $link (@$links) {
	    my $prefix= $link->[0]."/32";
	    # static route exists
	    (!exists($rt->{$prefix})) and return 0;
	    # next-hop is link tail-end
	    ($rt->{$prefix}->[0] ne $link->[0]) and return 0;
	}
	
	# Test static routes (forward)
	foreach my $link (@$links) {
	    my $trace= cbgp_record_route($cbgp, $node1, $link->[0]);
	    # Destination was reached
	    ($trace->[CBGP_TR_STATUS] ne "SUCCESS") and return 0;
	    # Path is 2 hops long
	    (@{$trace->[CBGP_TR_PATH]} != 2) and return 0;
	}
	
    }

    return 1;
}

# -----[ cbgp_peering ]----------------------------------------------
sub cbgp_peering($$$$;@)
{
    my ($cbgp, $router, $peer, $asn, @options)= @_;

    die if $cbgp->send("bgp router $router\n");
    die if $cbgp->send("\tadd peer $asn $peer\n");
    foreach my $option (@options) {
	die if $cbgp->send("\tpeer $peer $option\n");
    }
    die if $cbgp->send("\tpeer $peer up\n");
    die if $cbgp->send("\texit\n");
}

# -----[ cbgp_filter ]-----------------------------------------------
sub cbgp_filter($$$$$$)
{
    my ($cbgp, $router, $peer, $direction, $predicate, $actions)= @_;

    die if $cbgp->send("bgp router $router\n");
    die if $cbgp->send("\tpeer $peer\n");
    die if $cbgp->send("\t\tfilter $direction\n");
    die if $cbgp->send("\t\t\tadd-rule\n");
    die if $cbgp->send("\t\t\t\tmatch \"$predicate\"\n");
    die if $cbgp->send("\t\t\t\taction \"$actions\"\n");
    die if $cbgp->send("\t\t\t\texit\n");
    die if $cbgp->send("\t\t\texit\n");
    die if $cbgp->send("\t\texit\n");
    die if $cbgp->send("\texit\n");
}

# -----[ cbgp_recv_update ]------------------------------------------
sub cbgp_recv_update($$$$$)
{
    my ($cbgp, $router, $asn, $peer, $msg)= @_;

    die if $cbgp->send("bgp router $router peer $peer recv ".
		       "\"BGP4|0|A|$router|$asn|$msg\"\n");
}

# -----[ cbgp_recv_withdraw ]----------------------------------------
sub cbgp_recv_withdraw($$$$$)
{
    my ($cbgp, $router, $asn, $peer, $prefix)= @_;

    die if $cbgp->send("bgp router $router peer $peer recv ".
		       "\"BGP4|0|W|$router|$asn|$prefix\"\n");
}

# -----[ cbgp_show_links ]-------------------------------------------
sub cbgp_show_links($$)
{
    my ($cbgp, $node)= @_;
    my @links;

    die if $cbgp->send("net node $node show links\n");
    die if $cbgp->send("print \"done\\n\"\n");
    while ((my $result= $cbgp->expect(1)) ne "done") {
	if ($result =~
    m/^([0-9.]+)\s+([0-9]+)\s+([0-9]+)\s+(UP|DOWN)\s+(DIRECT|TUNNEL)\s+IGP_ADV$/)
    {
	push @links, ([$1, $2, $3, $4, $5]);
	} else {
	    show_error("incorrect format (show links): \"$result\"");
	    exit(-1);
	}
    }
    return\@links;
}

# -----[ cbgp_show_rt ]----------------------------------------------
sub cbgp_show_rt($$;$)
{
    my ($cbgp, $node, $destination)= @_;
    my %routes;

    if (!defined($destination)) {
	$destination= "*";
    }
    
    die if $cbgp->send("net node $node show rt $destination\n");
    die if $cbgp->send("print \"done\\n\"\n");
    while ((my $result= $cbgp->expect(1)) ne "done") {
	if ($result =~ m/^([0-9.\/]+)\s+([0-9.]+)\s+([0-9]+)\s+(STATIC|IGP|BGP)$/) {
	    $routes{$1}= [$2, $3, $4];
	} else {
	    show_error("incorrect format (show rt): \"$result\"");
	    exit(-1);
	}
    }
    return \%routes;
}

# -----[ cbgp_show_peers ]-------------------------------------------
sub cbgp_show_peers($$)
{
    my ($cbgp, $node)= @_;
    my %peers;
    
    die if $cbgp->send("bgp router $node show peers\n");
    die if $cbgp->send("print \"done\\n\"\n");
    while ((my $result= $cbgp->expect(1)) ne "done") {
	if ($result =~ m/^([0-9.]+)\s+AS([0-9]+)\s+([A-Z]+)(\s+.+)?$/) {
	    $peers{$1}= [$2, $3, $4];
	} else {
	    show_error("incorrect format (show peers): \"$result\"");
	    exit(-1);
	}
    }
    return \%peers;
}

# -----[ cbgp_show_rib ]---------------------------------------------
sub cbgp_show_rib($$;$)
{
    my ($cbgp, $node, $destination)= @_;
    my %rib;

    if (!defined($destination)) {
	$destination= "*";
    }

    die if $cbgp->send("bgp router $node show rib $destination\n");
    die if $cbgp->send("print \"done\\n\"\n");
    while ((my $result= $cbgp->expect(1)) ne "done") {
	if ($result =~ m/^([i*>]+)\s+([0-9.\/]+)\s+([0-9.]+)\s+([0-9]+)\s+([0-9]+)\s+([^\t]+)\s+([ie\?])$/) {
	    my @path= split /\s+/, $6;
	    $rib{$2}= [$1, $2, $3, $4, $5, \@path, $7];
	} else {
	    show_error("incorrect format (show rib): \"$result\"");
	    exit(-1);
	}
    }
    return \%rib;
}

# -----[ cbgp_show_rib_in ]------------------------------------------
sub cbgp_show_rib_in($$;$)
{
    my ($cbgp, $node, $destination)= @_;
    my %rib;

    if (!defined($destination)) {
	$destination= "*";
    }

    die if $cbgp->send("bgp router $node show rib-in * $destination\n");
    die if $cbgp->send("print \"done\\n\"\n");
    while ((my $result= $cbgp->expect(1)) ne "done") {
	if ($result =~ m/^([i*>]+)\s+([0-9.\/]+)\s+([0-9.]+)\s+([0-9]+)\s+([0-9]+)\s+([^\t]+)\s+([ie\?])$/) {
	    my @path= split /\s+/, $6;
	    $rib{$2}= [$1, $2, $3, $4, $5, \@path, $7];
	} else {
	    show_error("incorrect format (show rib-in): \"$result\"");
	    exit(-1);
	}
    }
    return \%rib;
}

# -----[ cbgp_show_rib_mrt ]-----------------------------------------
sub cbgp_show_rib_mrt($$;$)
{
    my ($cbgp, $node, $destination)= @_;
    my %rib;

    if (!defined($destination)) {
	$destination= "*";
    }

    die if $cbgp->send("bgp options show-mode mrt\n");
    die if $cbgp->send("bgp router $node show rib $destination\n");
    die if $cbgp->send("print \"done\\n\"\n");
    die if $cbgp->send("bgp options show-mode cisco\n");
    while ((my $result= $cbgp->expect(1)) ne "done") {
	if ($result =~ m/^(.?)\|([0-9.]+)\|([0-9]+)\|([0-9.\/]+)\|([0-9 ]+)\|(IGP|EGP|INCOMPLETE)\|([0-9.]+)\|([0-9]+)\|.+$/) {
	    my @fields= split /\|/, $result;
	    my @path= split /\s/, $fields[4];
	    my @communities= split /\s/, $fields[9];
	    my @route;
	    $route[CBGP_RIB_STATUS]= $fields[0];
	    $route[CBGP_RIB_PREFIX]= $fields[3];
	    $route[CBGP_RIB_NEXTHOP]= $fields[6];
	    $route[CBGP_RIB_PATH]= \@path;
	    $route[CBGP_RIB_PREF]= $fields[7];
	    $route[CBGP_RIB_MED]= $fields[8];
	    if (@fields > 9) {
		$route[CBGP_RIB_COMMUNITY]= \@communities;
	    }
	    $rib{$4}= \@route;
	} else {
	    show_error("incorrect format (show rib): \"$result\"");
	    exit(-1);
	}
    }
    return \%rib;
}

# -----[ cbgp_record_route ]-----------------------------------------
sub cbgp_record_route($$$)
{
    my ($cbgp, $src, $dst)= @_;
    my $trace;

    die if $cbgp->send("net node $src record-route $dst\n");
    my $result= $cbgp->expect(1);
    if ($result =~ m/^([0-9.]+)\s+([0-9.\/]+)\s+([A-Z_]+)\s+([0-9.\s]+)$/) {
	my @fields= split /\s+/, $result;
	$trace= [shift @fields, shift @fields, shift @fields, \@fields];
    } else {
	show_error("incorrect format (record-route): \"$result\"");
	exit(-1);
    }
    return $trace;
}

# -----[ cbgp_topo_check_links ]-------------------------------------
sub cbgp_topo_check_links($$)
{
    my ($cbgp, $topo)= @_;
    my %cbgp_topo;
    my %nodes;

    # Retrieve all the links reported by C-BGP through "show links"
    foreach my $node1 (keys %$topo) {
	foreach my $node2 (keys %{$topo->{$node1}}) {
	    if (!exists($nodes{$node1})) {
		$nodes{$node1}= 1;
		my $links= cbgp_show_links($cbgp, $node1);
		foreach my $link (@$links) {
		    $cbgp_topo{$node1}{$link->[0]}= [$link->[1], $link->[2]];
		}
	    }
	    if (!exists($nodes{$node2})) {
		$nodes{$node2}= 1;
		my $links= cbgp_show_links($cbgp, $node2);
		foreach my $link (@$links) {
		    $cbgp_topo{$node2}{$link->[0]}= [$link->[1], $link->[2]];
		}
	    }
	}
    }

    # Compare these links to the initial topology
    return (topo_included($topo, \%cbgp_topo) &&
	    topo_included(\%cbgp_topo, $topo));
}

# -----[ cbgp_check_peering ]----------------------------------------
sub cbgp_check_peering($$$;$)
{
    my ($cbgp, $router, $peer, $state)= @_;

    my $peers= cbgp_show_peers($cbgp, $router);
    if (!exists($peers->{$peer})) {
	return 0;
    }
    if (defined($state) && ($peers->{$peer}->[1] ne $state)) {
	return 0;
    }
    return 1;
}

# -----[ cbgp_check_bgp_route ]--------------------------------------
sub cbgp_check_bgp_route($$$)
{
    my ($cbgp, $node, $prefix)= @_;

    my $rib= cbgp_show_rib($cbgp, $node);
    if (!exists($rib->{$prefix})) {
	return 0;
    } else {
	return 1;
    }
}

# -----[ aspath_equals ]---------------------------------------------
sub aspath_equals($$)
{
    my ($path1, $path2)= @_;

    (@$path1 != @$path2) and return 0;
    for (my $index= 0; $index < @$path1; $index++) {
	($path1->[$index] != $path2->[$index]) and return 0;
    }
    return 1;
}

# -----[ community_equals ]------------------------------------------
sub community_equals($$)
{
    my ($comm1, $comm2)= @_;

    if (@$comm1 != @$comm2) {
	print "community length mismatch\n";
	return 0;
    }
    my %comm1;
    my %comm2;
    foreach my $comm (@$comm1) { $comm1{$comm}= 1; };
    foreach my $comm (@$comm2) { $comm2{$comm}= 1; };
    foreach my $comm (keys %comm1) {
	if (!exists($comm2{$comm})) {
	    print "$comm not in comm2\n";
	    return 0;
	}
    }
    foreach my $comm (keys %comm2) {
	if (!exists($comm1{$comm})) {
	    print "$comm not in comm1\n";
	    return 0;
	}
    }
    return 1;
}

# -----[ cbgp_valid_version ]----------------------------------------
sub cbgp_valid_version($)
{
    my ($cbgp)= @_;
    die if $cbgp->send("show version\n");
    my $result= $cbgp->expect(1);
    if ($result =~ m/^version:\s+cbgp\s+([0-9.]+)(\s+.+)$/) {
	$cbgp_version= $1;
	return TEST_SUCCESS;
    } else {
	return TEST_FAILURE;
    }
}

# -----[ cbgp_valid_net_create ]-------------------------------------
sub cbgp_valid_net_create($$)
{
    my ($cbgp, $topo)= @_;
    cbgp_topo($cbgp, $topo);
    return cbgp_topo_check_links($cbgp, $topo);
}

# -----[ cbgp_valid_net_igp ]----------------------------------------
sub cbgp_valid_net_igp($$)
{
    my ($cbgp, $topo)= @_;
    cbgp_topo($cbgp, $topo);
    cbgp_topo_igp_compute($cbgp, $topo, "1.0.0.0/30");
    return cbgp_topo_check_igp_reachability($cbgp, $topo);
}

# -----[ cbgp_valid_net_ntf_load ]-----------------------------------
sub cbgp_valid_net_ntf_load($$)
{
    my ($cbgp, $ntf_file)= @_;
    my $topo= topo_from_ntf($ntf_file);
    die if $cbgp->send("net ntf load \"$ntf_file\"\n");
    return cbgp_topo_check_links($cbgp, $topo);
}

# -----[ cbgp_valid_net_record_route ]-------------------------------
sub cbgp_valid_net_record_route($$)
{
    my ($cbgp, $topo)= @_;
    cbgp_topo($cbgp, $topo);
    if (!cbgp_topo_check_record_route($cbgp, $topo, "UNREACH")) {
	return TEST_FAILURE;
    }
    cbgp_topo_igp_compute($cbgp, $topo, "1.0.0.0/30");
    if (!cbgp_topo_check_record_route($cbgp, $topo, "SUCCESS")) {
	return TEST_FAILURE;
    }
    return TEST_SUCCESS;
}

# -----[ cbgp_valid_net_static_routes ]------------------------------
sub cbgp_valid_net_static_routes($$)
{
    my ($cbgp, $topo)= @_;
    cbgp_topo($cbgp, $topo);
    my $nodes= topo_get_nodes($topo);
    if (!cbgp_topo_check_static_routes($cbgp, $topo)) {
	return TEST_FAILURE;
    }
    return TEST_SUCCESS;
}

# -----[ cbgp_valid_net_longest_matching ]---------------------------
sub cbgp_valid_net_longest_matching($)
{
    my ($cbgp)= @_;
    my $topo= topo_3nodes_line();
    cbgp_topo($cbgp, $topo);
    die if $cbgp->send("net node 1.0.0.2 route add 1/8 1.0.0.1 10\n");
    die if $cbgp->send("net node 1.0.0.2 route add 1.1/16 1.0.0.3 10\n");
    # Test longest-matching in show-rt
    my $rt;
    $rt= cbgp_show_rt($cbgp, "1.0.0.2", "1.0.0.0");
    if (!exists($rt->{"1.0.0.0/8"}) ||
	($rt->{"1.0.0.0/8"}->[CBGP_RT_NEXTHOP] ne "1.0.0.1")) {
	return TEST_FAILURE;
    }
    $rt= cbgp_show_rt($cbgp, "1.0.0.2", "1.1.0.0");
    if (!exists($rt->{"1.1.0.0/16"}) ||
	($rt->{"1.1.0.0/16"}->[CBGP_RT_NEXTHOP] ne "1.0.0.3")) {
	return TEST_FAILURE;
    }
    # Test longest-matching in record-route
    my $trace;
    $trace= cbgp_record_route($cbgp, "1.0.0.2", "1.0.0.0");
    if ($trace->[CBGP_TR_PATH]->[1] ne "1.0.0.1") {
	return TEST_FAILURE;
    }
    $trace= cbgp_record_route($cbgp, "1.0.0.2", "1.1.0.0");
    if ($trace->[CBGP_TR_PATH]->[1] ne "1.0.0.3") {
	return TEST_FAILURE;
    }
    return TEST_SUCCESS;
}

# -----[ cbgp_valid_net_protocol_priority ]--------------------------
sub cbgp_valid_net_protocol_priority($)
{
    my ($cbgp)= @_;
    my $topo= topo_3nodes_line();
    cbgp_topo($cbgp, $topo);
    cbgp_topo_igp_compute($cbgp, $topo, "1.0.0.0/30");
    die if $cbgp->send("net node 1.0.0.2 route add 1.0.0.1/32 1.0.0.3 10\n");
    # Test protocol-priority in show-rt
    my $rt;
    $rt= cbgp_show_rt($cbgp, "1.0.0.2", "1.0.0.1");
    if (!exists($rt->{"1.0.0.1/32"}) ||
	($rt->{"1.0.0.1/32"}->[CBGP_RT_PROTO] ne "STATIC") ||
	($rt->{"1.0.0.1/32"}->[CBGP_RT_NEXTHOP] ne "1.0.0.3")) {
	print "1.0.0.2 --> 1.0.0.1\n";
	return TEST_FAILURE;
    }
    $rt= cbgp_show_rt($cbgp, "1.0.0.2", "1.0.0.3");
    if (!exists($rt->{"1.0.0.3/32"}) ||
	($rt->{"1.0.0.3/32"}->[CBGP_RT_PROTO] ne "IGP") ||
	($rt->{"1.0.0.3/32"}->[CBGP_RT_NEXTHOP] ne "1.0.0.3")) {
	print "1.0.0.2 --> 1.0.0.3\n";
	return TEST_FAILURE;
    }
    # Test protocol-priority in record-route
    my $trace;
    $trace= cbgp_record_route($cbgp, "1.0.0.2", "1.0.0.1");
    if ($trace->[CBGP_TR_PATH]->[1] ne "1.0.0.3") {
	return TEST_FAILURE;
    }
    $trace= cbgp_record_route($cbgp, "1.0.0.2", "1.0.0.3");
    if ($trace->[CBGP_TR_PATH]->[1] ne "1.0.0.3") {
	return TEST_FAILURE;
    }    
    return TEST_SUCCESS;
}

# -----[ cbgp_valid_bgp_session_ibgp ]-------------------------------
# - establishment
# - propagation
# - no iBGP reflection
# - no AS-path update
# - no next-hop update
# -------------------------------------------------------------------
sub cbgp_valid_bgp_session_ibgp($)
{
    my ($cbgp)= @_;
    my $topo= topo_3nodes_line();
    cbgp_topo($cbgp, $topo);
    cbgp_topo_igp_compute($cbgp, $topo, "1.0.0.0/30");
    cbgp_topo_bgp_routers($cbgp, $topo, 1);
    cbgp_peering($cbgp, "1.0.0.1", "1.0.0.2", 1);
    cbgp_peering($cbgp, "1.0.0.2", "1.0.0.1", 1);
    cbgp_peering($cbgp, "1.0.0.2", "1.0.0.3", 1);
    cbgp_peering($cbgp, "1.0.0.3", "1.0.0.2", 1);
    if (!cbgp_check_peering($cbgp, "1.0.0.1", "1.0.0.2", "OPENWAIT") ||
	!cbgp_check_peering($cbgp, "1.0.0.2", "1.0.0.1", "OPENWAIT") ||
	!cbgp_check_peering($cbgp, "1.0.0.2", "1.0.0.3", "OPENWAIT") ||
	!cbgp_check_peering($cbgp, "1.0.0.3", "1.0.0.2", "OPENWAIT")) {
	return TEST_FAILURE;
    }
    die if $cbgp->send("sim run\n");
    if (!cbgp_check_peering($cbgp, "1.0.0.1", "1.0.0.2", "ESTABLISHED") ||
	!cbgp_check_peering($cbgp, "1.0.0.2", "1.0.0.1", "ESTABLISHED") ||
	!cbgp_check_peering($cbgp, "1.0.0.2", "1.0.0.3", "ESTABLISHED") ||
	!cbgp_check_peering($cbgp, "1.0.0.3", "1.0.0.2", "ESTABLISHED")) {
	return TEST_FAILURE;
    }
    die if $cbgp->send("bgp router 1.0.0.1 add network 255.255.0.0/16\n");
    die if $cbgp->send("sim run\n");
    my $rib;
    # Check that 1.0.0.2 has received the route, that the next-hop is
    # 1.0.0.1 and the AS-path is empty. 
    $rib= cbgp_show_rib($cbgp, "1.0.0.2");
    if (!exists($rib->{"255.255.0.0/16"}) ||
	($rib->{"255.255.0.0/16"}->[CBGP_RIB_NEXTHOP] ne "1.0.0.1") ||
	($rib->{"255.255.0.0/16"}->[CBGP_RIB_PATH]->[0] ne "null")) {
	return TEST_FAILURE;
    }
    # Check that 1.0.0.3 has not received the route (1.0.0.2 did not
    # send it since it was learned through an iBGP session).
    $rib= cbgp_show_rib($cbgp, "1.0.0.3");
    if (exists($rib->{"255.255.0.0/16"})) {
	return TEST_FAILURE;
    }
    return TEST_SUCCESS;
}

# -----[ cbgp_valid_bgp_session_ebgp ]-------------------------------
# - establishment
# - propagation
# - update AS-path on eBGP session
# - update next-hop on eBGP session
# - [TODO] local-pref is reset in routes received through eBGP
# -------------------------------------------------------------------
sub cbgp_valid_bgp_session_ebgp($)
{
    my ($cbgp)= @_;
    my $topo= topo_3nodes_line();
    cbgp_topo($cbgp, $topo);
    cbgp_topo_igp_compute($cbgp, $topo, "1.0.0.0/30");
    die if $cbgp->send("bgp add router 1 1.0.0.1\n");
    die if $cbgp->send("bgp add router 2 1.0.0.2\n");
    die if $cbgp->send("bgp add router 2 1.0.0.3\n");
    cbgp_peering($cbgp, "1.0.0.1", "1.0.0.2", 2);
    cbgp_peering($cbgp, "1.0.0.2", "1.0.0.1", 1);
    cbgp_peering($cbgp, "1.0.0.2", "1.0.0.3", 2);
    cbgp_peering($cbgp, "1.0.0.3", "1.0.0.2", 2);
    if (!cbgp_check_peering($cbgp, "1.0.0.1", "1.0.0.2", "OPENWAIT") ||
	!cbgp_check_peering($cbgp, "1.0.0.2", "1.0.0.1", "OPENWAIT")) {
	return TEST_FAILURE;
    }
    die if $cbgp->send("sim run\n");
    if (!cbgp_check_peering($cbgp, "1.0.0.1", "1.0.0.2", "ESTABLISHED") ||
	!cbgp_check_peering($cbgp, "1.0.0.2", "1.0.0.1", "ESTABLISHED")) {
	return TEST_FAILURE;
    }
    die if $cbgp->send("bgp router 1.0.0.1 add network 255.255.0.0/16\n");
    die if $cbgp->send("bgp router 1.0.0.3 add network 255.254.0.0/16\n");
    die if $cbgp->send("sim run\n");
    my $rib;
    # Check that AS-path contains remote ASN and that the next-hop is
    # the last router in remote AS
    $rib= cbgp_show_rib($cbgp, "1.0.0.1");
    if (!exists($rib->{"255.254.0.0/16"}) ||
	!aspath_equals($rib->{"255.254.0.0/16"}->[CBGP_RIB_PATH],[2]) ||
	($rib->{"255.254.0.0/16"}->[CBGP_RIB_NEXTHOP] ne "1.0.0.2")) {
	return TEST_FAILURE;
    }
    # Check that AS-path contains remote ASN
    $rib= cbgp_show_rib($cbgp, "1.0.0.2");
    if (!exists($rib->{"255.255.0.0/16"}) ||
	!aspath_equals($rib->{"255.255.0.0/16"}->[CBGP_RIB_PATH], [1]) ||
	($rib->{"255.255.0.0/16"}->[CBGP_RIB_NEXTHOP] ne "1.0.0.1")) {
	return TEST_FAILURE;
    }
    # Check that AS-path contains remote ASN
    $rib= cbgp_show_rib($cbgp, "1.0.0.3");
    if (!exists($rib->{"255.255.0.0/16"}) ||
	!aspath_equals($rib->{"255.255.0.0/16"}->[CBGP_RIB_PATH], [1]) ||
	($rib->{"255.255.0.0/16"}->[CBGP_RIB_NEXTHOP] ne "1.0.0.1")) {
	return TEST_FAILURE;
    }
    return TEST_SUCCESS;
}

# -----[ cbgp_valid_bgp_session_sm ]---------------------------------
#sub cbgp_valid_bgp_session_sm()
#{
#    return TEST_SKIPPED;
#}

# -----[ cbgp_valid_bgp_domain_fullmesh ]----------------------------
sub cbgp_valid_bgp_domain_fullmesh($$)
{
    my ($cbgp, $topo)= @_;
    cbgp_topo($cbgp, $topo);
    cbgp_topo_igp_compute($cbgp, $topo, "1.0.0.0/30");
    cbgp_topo_bgp_routers($cbgp, $topo, 1);
    die if $cbgp->send("bgp domain 1 full-mesh\n");
    if (!cbgp_topo_check_sessions($cbgp, $topo, "OPENWAIT")) {
	return TEST_FAILURE;
    }
    die if $cbgp->send("sim run\n");
    if (!cbgp_topo_check_sessions($cbgp, $topo, "ESTABLISHED")) {
	return TEST_FAILURE;
    }
    if (!cbgp_topo_check_bgp_networks($cbgp, $topo)) {
	return TEST_FAILURE;
    }
    return TEST_SUCCESS;
}

# -----[ cbgp_valid_bgp_peering_up_down ]----------------------------
sub cbgp_valid_bgp_peering_up_down($)
{
    my ($cbgp)= @_;
    my $topo= topo_2nodes();
    cbgp_topo($cbgp, $topo);
    cbgp_topo_igp_compute($cbgp, $topo, "1.0.0.0/30");
    cbgp_topo_bgp_routers($cbgp, $topo, 1);
    die if $cbgp->send("bgp domain 1 full-mesh\n");
    die if $cbgp->send("bgp router 1.0.0.1 add network 255.255.0.0/16\n");
    die if $cbgp->send("bgp router 1.0.0.2 add network 255.254.0.0/16\n");
    die if $cbgp->send("sim run\n");
    if (!cbgp_check_bgp_route($cbgp, "1.0.0.1", "255.254.0.0/16") ||
	!cbgp_check_bgp_route($cbgp, "1.0.0.2", "255.255.0.0/16")) {
	return TEST_FAILURE;
    }
    die if $cbgp->send("bgp router 1.0.0.1 peer 1.0.0.2 down\n");
    die if $cbgp->send("sim run\n");
    if (cbgp_check_bgp_route($cbgp, "1.0.0.1", "255.254.0.0/16") ||
	cbgp_check_bgp_route($cbgp, "1.0.0.2", "255.255.0.0/16")) {
	return TEST_FAILURE;
    }
    die if $cbgp->send("bgp router 1.0.0.1 peer 1.0.0.2 up\n");
    die if $cbgp->send("sim run\n");
    if (!cbgp_check_bgp_route($cbgp, "1.0.0.1", "255.254.0.0/16") ||
	!cbgp_check_bgp_route($cbgp, "1.0.0.2", "255.255.0.0/16")) {
	return TEST_FAILURE;
    }
    return TEST_SUCCESS;
}

# -----[ cbgp_valid_bgp_peerings_nexthopself ]-----------------------
sub cbgp_valid_bgp_peerings_nexthopself($)
{
    my ($cbgp)= @_;
    my $topo= topo_2nodes();
    cbgp_topo($cbgp, $topo);
    cbgp_topo_igp_compute($cbgp, $topo, "1.0.0.0/30");
    cbgp_topo_bgp_routers($cbgp, $topo, 1);
    die if $cbgp->send("bgp domain 1 full-mesh\n");
    die if $cbgp->send("net add node 2.0.0.1\n");
    die if $cbgp->send("net add link 1.0.0.1 2.0.0.1 0\n");
    die if $cbgp->send("net node 1.0.0.1 route add 2.0.0.1/32 2.0.0.1 0\n");
    die if $cbgp->send("net node 2.0.0.1 route add 1.0.0.1/32 1.0.0.1 0\n");
    die if $cbgp->send("bgp add router 2 2.0.0.1\n");
    die if $cbgp->send("bgp router 2.0.0.1 add network 2.0.0.0/8\n");
    cbgp_peering($cbgp, "2.0.0.1", "1.0.0.1", 1, "next-hop-self");
    cbgp_peering($cbgp, "1.0.0.1", "2.0.0.1", 2, "next-hop-self");
    die if $cbgp->send("sim run\n");
    # Next-hop must be unchanged in 1.0.0.1
    my $rib= cbgp_show_rib($cbgp, "1.0.0.1");
    if ((!exists($rib->{"2.0.0.0/8"})) ||
	($rib->{"2.0.0.0/8"}->[CBGP_RIB_NEXTHOP] ne "2.0.0.1")) {
	return TEST_FAILURE;
    }
    # Next-hop must be updated with address of 1.0.0.1 in 1.0.0.2
    $rib= cbgp_show_rib($cbgp, "1.0.0.2");
    if (!exists($rib->{"2.0.0.0/8"}) ||
	($rib->{"2.0.0.0/8"}->[CBGP_RIB_NEXTHOP] ne "1.0.0.1")) {
	return TEST_FAILURE;
    }
    return TEST_SUCCESS;
}

# -----[ cbgp_valid_bgp_virtual_peer ]-------------------------------
sub cbgp_valid_bgp_virtual_peer($)
{
    my ($cbgp)= @_;
    my $topo= topo_2nodes();
    cbgp_topo($cbgp, $topo);
    cbgp_topo_igp_compute($cbgp, $topo, "1.0.0.0/30");
    cbgp_topo_bgp_routers($cbgp, $topo, 1);
    die if $cbgp->send("bgp domain 1 full-mesh\n");
    die if $cbgp->send("net add node 2.0.0.1\n");
    die if $cbgp->send("net add link 1.0.0.1 2.0.0.1 0\n");
    die if $cbgp->send("net node 1.0.0.1 route add 2.0.0.1/32 2.0.0.1 0\n");
    cbgp_peering($cbgp, "1.0.0.1", "2.0.0.1", 2, "next-hop-self", "virtual");
    die if $cbgp->send("sim run\n");    
    if (!cbgp_check_peering($cbgp, "1.0.0.1", "2.0.0.1", "ESTABLISHED")) {
	return TEST_FAILURE;
    }
    return TEST_SUCCESS;
}

# -----[ cbgp_valid_bgp_recv_mrt ]-----------------------------------
sub cbgp_valid_bgp_recv_mrt($)
{
    my ($cbgp)= @_;
    my $topo= topo_2nodes();
    cbgp_topo($cbgp, $topo);
    cbgp_topo_igp_compute($cbgp, $topo, "1.0.0.0/30");
    cbgp_topo_bgp_routers($cbgp, $topo, 1);
    die if $cbgp->send("bgp domain 1 full-mesh\n");
    die if $cbgp->send("net add node 2.0.0.1\n");
    die if $cbgp->send("net add link 1.0.0.1 2.0.0.1 0\n");
    die if $cbgp->send("net node 1.0.0.1 route add 2.0.0.1/32 2.0.0.1 0\n");
    cbgp_peering($cbgp, "1.0.0.1", "2.0.0.1", 2, "next-hop-self", "virtual");
    die if $cbgp->send("sim run\n");
    die if $cbgp->send("bgp router 1.0.0.1 peer 2.0.0.1 recv ".
		       "\"BGP4|0|A|1.0.0.1|1|255.255.0.0/16|2|IGP|2.0.0.1|12|34|\"\n");
    die if $cbgp->send("sim run\n");
    my $rib;
    # Check attributes of received route (note: local-pref has been
    # reset since the route is received over an eBGP session).
    $rib= cbgp_show_rib($cbgp, "1.0.0.1");
    if (!exists($rib->{"255.255.0.0/16"})) {
	return TEST_FAILURE;
    }
    if (($rib->{"255.255.0.0/16"}->[CBGP_RIB_NEXTHOP] ne "2.0.0.1") ||
	($rib->{"255.255.0.0/16"}->[CBGP_RIB_PREF] != 0) ||
	($rib->{"255.255.0.0/16"}->[CBGP_RIB_MED] != 34) ||
	!aspath_equals($rib->{"255.255.0.0/16"}->[CBGP_RIB_PATH], [2])) {
	return TEST_FAILURE;
    }
    die if $cbgp->send("bgp router 1.0.0.1 peer 2.0.0.1 recv ".
		       "\"BGP4|0|W|1.0.0.1|1|255.255.0.0/16\"\n");
    die if $cbgp->send("sim run\n");
    # Check that the route has been withdrawn
    $rib= cbgp_show_rib($cbgp, "1.0.0.1");
    if (exists($rib->{"255.255.0.0/16"})) {
	return TEST_FAILURE;
    }
    return TEST_SUCCESS;
}

# -----[ cbgp_valid_bgp_soft_restart ]-------------------------------
sub cbgp_valid_bgp_soft_restart($)
{
    my ($cbgp)= @_;
    my $topo= topo_2nodes();
    cbgp_topo($cbgp, $topo);
    cbgp_topo_igp_compute($cbgp, $topo, "1.0.0.0/30");
    cbgp_topo_bgp_routers($cbgp, $topo, 1);
    die if $cbgp->send("bgp domain 1 full-mesh\n");
    die if $cbgp->send("net add node 2.0.0.1\n");
    die if $cbgp->send("net add link 1.0.0.1 2.0.0.1 0\n");
    die if $cbgp->send("net node 1.0.0.1 route add 2.0.0.1/32 2.0.0.1 0\n");
    cbgp_peering($cbgp, "1.0.0.1", "2.0.0.1", 2,
		 "next-hop-self", "virtual", "soft-restart");
    die if $cbgp->send("sim run\n");    
    if (!cbgp_check_peering($cbgp, "1.0.0.1", "2.0.0.1", "ESTABLISHED")) {
	return TEST_FAILURE;
    }
    die if $cbgp->send("bgp router 1.0.0.1 peer 2.0.0.1 recv ".
		       "\"BGP4|0|A|1.0.0.1|1|255.255.0.0/16|2|IGP|2.0.0.1|0|0\"\n");
    die if $cbgp->send("sim run\n");
    my $rib;
    # Check that route is available
    $rib= cbgp_show_rib($cbgp, "1.0.0.1");
    if (!exists($rib->{"255.255.0.0/16"})) {
	return TEST_FAILURE;
    }
    die if $cbgp->send("bgp router 1.0.0.1 peer 2.0.0.1 down\n");
    die if $cbgp->send("sim run\n");
    # Check that route has been withdrawn
    $rib= cbgp_show_rib($cbgp, "1.0.0.1");
    if (exists($rib->{"255.255.0.0/16"})) {
	return TEST_FAILURE;
    }
    die if $cbgp->send("bgp router 1.0.0.1 peer 2.0.0.1 up\n");
    die if $cbgp->send("sim run\n");
    # Check that route is still announced after peering up
    $rib= cbgp_show_rib($cbgp, "1.0.0.1");
    if (!exists($rib->{"255.255.0.0/16"})) {
	return TEST_FAILURE;
    }    
    return TEST_SUCCESS;
}

# -----[ cbgp_valid_bgp_aspath_loop ]--------------------------------
# Test that a route with an AS-path that already contains the local AS
# will be filtered on input.
# -------------------------------------------------------------------
sub cbgp_valid_bgp_aspath_loop($)
{
    my ($cbgp)= @_;
    cbgp_topo_dp3($cbgp, ["2.0.0.1", 2, 0]);
    cbgp_recv_update($cbgp, "1.0.0.1", 1, "2.0.0.1",
		     "255/16|2 1|IGP|2.0.0.1|0|0");
    die if $cbgp->send("sim run\n");
    my $rib;
    $rib= cbgp_show_rib($cbgp, "1.0.0.1");
    if (exists($rib->{"255.0.0.0/16"})) {
	return TEST_FAILURE;
    }
    return TEST_SUCCESS;
    die;
}

# -----[ cbgp_valid_bgp_ssld ]---------------------------------------
# Test sender-side loop detection.
# -------------------------------------------------------------------
sub cbgp_valid_bgp_ssld($)
{
    my ($cbgp)= @_;

    return TEST_SKIPPED;
}

# -----[ cbgp_valid_bgp_implicit_withdraw ]--------------------------
sub cbgp_valid_bgp_implicit_withdraw($)
{
    my ($cbgp)= @_;
    cbgp_topo_dp3($cbgp, ["1.0.0.2", 1, 10], ["2.0.0.1", 2, 0],
		  ["2.0.0.2", 2, 0]);
    cbgp_recv_update($cbgp, "1.0.0.1", 1, "2.0.0.1",
		     "255/8|2 3 4|IGP|2.0.0.1|0|0");
    cbgp_recv_update($cbgp, "1.0.0.1", 1, "2.0.0.2",
		     "255/8|2 4|IGP|2.0.0.2|0|0");
    die if $cbgp->send("sim run\n");
    my $rib;
    $rib= cbgp_show_rib($cbgp, "1.0.0.1");
    if (!exists($rib->{"255.0.0.0/8"}) ||
	($rib->{"255.0.0.0/8"}->[CBGP_RIB_NEXTHOP] ne "2.0.0.2")) {
	return TEST_FAILURE;
    }
    cbgp_recv_update($cbgp, "1.0.0.1", 1, "2.0.0.1",
		     "255/8|2 4|IGP|2.0.0.1|0|0");
    die if $cbgp->send("sim run\n");
    $rib= cbgp_show_rib($cbgp, "1.0.0.1");
    if (!exists($rib->{"255.0.0.0/8"}) ||
	($rib->{"255.0.0.0/8"}->[CBGP_RIB_NEXTHOP] ne "2.0.0.1")) {
	return TEST_FAILURE;
    }
    return TEST_SUCCESS;
}

# -----[ cbgp_valid_bgp_dp_localpref ]-------------------------------
sub cbgp_valid_bgp_dp_localpref($)
{
    my ($cbgp)= @_;
    cbgp_topo_dp($cbgp, 2);
    cbgp_filter($cbgp, "1.0.0.1", "2.0.0.1", "in", "any", "local-pref 100");
    cbgp_filter($cbgp, "1.0.0.1", "3.0.0.1", "in", "any", "local-pref 80");
    cbgp_recv_update($cbgp, "1.0.0.1", 1, "2.0.0.1",
		     "255/16|2 4|IGP|2.0.0.1|0|0");
    cbgp_recv_update($cbgp, "1.0.0.1", 1, "3.0.0.1",
		     "255/16|3 4|IGP|3.0.0.1|0|0");
    die if $cbgp->send("sim run\n");
    my $rib;
    my @routes;
    $rib= cbgp_show_rib($cbgp, "1.0.0.1", "255/16");
    @routes= values %$rib; 
    if (scalar(@routes) < 1) {
	return TEST_FAILURE;
    }
    if ($routes[0]->[CBGP_RIB_PREF] != 100) {
	return TEST_FAILURE;
    }
    die if $cbgp->send("bgp router 1.0.0.1 peer 2.0.0.1 down\n");
    die if $cbgp->send("bgp router 1.0.0.1 peer 3.0.0.1 down\n");
    die if $cbgp->send("sim run\n");
    die if $cbgp->send("bgp router 1.0.0.1 peer 2.0.0.1 filter in remove-rule 0\n\n");
    die if $cbgp->send("bgp router 1.0.0.1 peer 3.0.0.1 filter in remove-rule 0\n\n");
    die if $cbgp->send("bgp router 1.0.0.1 peer 2.0.0.1 up\n");
    die if $cbgp->send("bgp router 1.0.0.1 peer 3.0.0.1 up\n");
    die if $cbgp->send("sim run\n");
    cbgp_filter($cbgp, "1.0.0.1", "2.0.0.1", "in", "any", "local-pref 80");
    cbgp_filter($cbgp, "1.0.0.1", "3.0.0.1", "in", "any", "local-pref 100");
    cbgp_recv_update($cbgp, "1.0.0.1", 1, "2.0.0.1",
		     "255/16|2|IGP|2.0.0.1|0|0");
    cbgp_recv_update($cbgp, "1.0.0.1", 1, "3.0.0.1",
		     "255/16|2|IGP|3.0.0.1|0|0");
    die if $cbgp->send("sim run\n");
    $rib= cbgp_show_rib($cbgp, "1.0.0.1", "255/16");
    @routes= values %$rib; 
    if (scalar(@routes) < 1) {
	return TEST_FAILURE;
    }
    if ($routes[0]->[CBGP_RIB_PREF] != 100) {
	return TEST_FAILURE;
    }
    return TEST_SUCCESS;
}

# -----[ cbgp_valid_bgp_dp_aspath ]----------------------------------
sub cbgp_valid_bgp_dp_aspath($)
{
    my ($cbgp)= @_;
    cbgp_topo_dp($cbgp, 2);
    cbgp_recv_update($cbgp, "1.0.0.1", 1, "2.0.0.1",
		     "255/16|2 4 5|IGP|2.0.0.1|0|0");
    cbgp_recv_update($cbgp, "1.0.0.1", 1, "3.0.0.1",
		     "255/16|3 5|IGP|3.0.0.1|0|0");
    die if $cbgp->send("sim run\n");
    my $rib;
    my @routes;
    $rib= cbgp_show_rib($cbgp, "1.0.0.1", "255/16");
    @routes= values %$rib; 
    if (scalar(@routes) < 1) {
	return TEST_FAILURE;
    }
    if (!aspath_equals($routes[0]->[CBGP_RIB_PATH], [3, 5])) {
	return TEST_FAILURE;
    }
    return TEST_SUCCESS;
}

# -----[ cbgp_valid_bgp_dp_ebgp_ibgp ]-------------------------------
sub cbgp_valid_bgp_dp_ebgp_ibgp($)
{
    my ($cbgp)= @_;
    cbgp_topo_dp3($cbgp, ["1.0.0.2", 1, 0], ["2.0.0.1", 2, 0]);
    cbgp_recv_update($cbgp, "1.0.0.1", 1, "1.0.0.2",
		     "255/16|2 5|IGP|1.0.0.2|0|0");
    cbgp_recv_update($cbgp, "1.0.0.1", 1, "2.0.0.1",
		     "255/16|2 5|IGP|2.0.0.1|0|0");
    die if $cbgp->send("sim run\n");    
    my $rib;
    my @routes;
    $rib= cbgp_show_rib($cbgp, "1.0.0.1", "255/16");
    @routes= values %$rib;
    if (scalar(@routes) < 1) {
	return TEST_FAILURE;
    }
    if ($routes[0]->[CBGP_RIB_NEXTHOP] ne "2.0.0.1") {
	return TEST_FAILURE;
    }
    return TEST_SUCCESS;
}

# -----[ cbgp_valid_bgp_dp_med ]-------------------------------------
sub cbgp_valid_bgp_dp_med($)
{
    my ($cbgp)= @_;
    cbgp_topo_dp3($cbgp, ["2.0.0.1", 2, 0], ["2.0.0.2", 2, 0],
		  ["3.0.0.1", 3, 0], ["3.0.0.2", 3, 0]);
    die if $cbgp->send("bgp options med deterministic\n");
    cbgp_recv_update($cbgp, "1.0.0.1", 1, "2.0.0.1",
		     "255/16|2 4|IGP|2.0.0.1|0|80");
    cbgp_recv_update($cbgp, "1.0.0.1", 1, "2.0.0.2",
		     "255/16|2 4|IGP|2.0.0.2|0|60");
    cbgp_recv_update($cbgp, "1.0.0.1", 1, "3.0.0.1",
		     "255/16|3 4|IGP|3.0.0.1|0|40");
    cbgp_recv_update($cbgp, "1.0.0.1", 1, "3.0.0.2",
		     "255/16|3 4|IGP|3.0.0.2|0|20");
    die if $cbgp->send("sim run\n");
    my $rib;
    my @routes;
    $rib= cbgp_show_rib($cbgp, "1.0.0.1", "255/16");
    @routes= values %$rib; 
    if (scalar(@routes) < 1) {
	return TEST_FAILURE;
    }
    if ($routes[0]->[CBGP_RIB_NEXTHOP] ne "2.0.0.2") {
	return TEST_FAILURE;
    }
    die if $cbgp->send("bgp options med always-compare\n");
    cbgp_recv_withdraw($cbgp, "1.0.0.1", 1, "2.0.0.1", "255/16");
    cbgp_recv_withdraw($cbgp, "1.0.0.1", 1, "2.0.0.2", "255/16");
    cbgp_recv_withdraw($cbgp, "1.0.0.1", 1, "3.0.0.1", "255/16");
    cbgp_recv_withdraw($cbgp, "1.0.0.1", 1, "3.0.0.2", "255/16");
    die if $cbgp->send("sim run\n");
    cbgp_recv_update($cbgp, "1.0.0.1", 1, "2.0.0.1",
		     "255/16|2 4|IGP|2.0.0.1|0|80");
    cbgp_recv_update($cbgp, "1.0.0.1", 1, "2.0.0.2",
		     "255/16|2 4|IGP|2.0.0.2|0|60");
    cbgp_recv_update($cbgp, "1.0.0.1", 1, "3.0.0.1",
		     "255/16|3 4|IGP|3.0.0.1|0|40");
    cbgp_recv_update($cbgp, "1.0.0.1", 1, "3.0.0.2",
		     "255/16|3 4|IGP|3.0.0.2|0|20");
    die if $cbgp->send("sim run\n");
    $rib= cbgp_show_rib($cbgp, "1.0.0.1", "255/16");
    @routes= values %$rib; 
    if (scalar(@routes) < 1) {
	return TEST_FAILURE;
    }
    if ($routes[0]->[CBGP_RIB_NEXTHOP] ne "3.0.0.2") {
	return TEST_FAILURE;
    }
    return TEST_SUCCESS;
}

# -----[ cbgp_valid_bgp_dp_igp ]-------------------------------------
sub cbgp_valid_bgp_dp_igp($)
{
    my ($cbgp)= @_;
    cbgp_topo_dp3($cbgp, ["1.0.0.2", 1, 30], ["1.0.0.3", 1, 20],
		  ["1.0.0.4", 1, 10]);
    cbgp_recv_update($cbgp, "1.0.0.1", 1, "1.0.0.2",
		     "255/16|2|IGP|1.0.0.2|0|0");
    cbgp_recv_update($cbgp, "1.0.0.1", 1, "1.0.0.3",
		     "255/16|2|IGP|1.0.0.3|0|0");
    cbgp_recv_update($cbgp, "1.0.0.1", 1, "1.0.0.4",
		     "255/16|2|IGP|1.0.0.4|0|0");
    die if $cbgp->send("sim run\n");
    my $rib;
    my @routes;
    $rib= cbgp_show_rib($cbgp, "1.0.0.1", "255/16");
    @routes= values %$rib; 
    if (scalar(@routes) < 1) {
	return TEST_FAILURE;
    }
    if ($routes[0]->[CBGP_RIB_NEXTHOP] ne "1.0.0.4") {
	return TEST_FAILURE;
    }
    cbgp_recv_withdraw($cbgp, "1.0.0.1", 1, "1.0.0.4", "255/16");
    die if $cbgp->send("sim run\n");
    $rib= cbgp_show_rib($cbgp, "1.0.0.1", "255/16");
    @routes= values %$rib; 
    if (scalar(@routes) < 1) {
	return TEST_FAILURE;
    }
    if ($routes[0]->[CBGP_RIB_NEXTHOP] ne "1.0.0.3") {
	return TEST_FAILURE;
    }
    return TEST_SUCCESS;
}

# -----[ cbgp_valid_bgp_dp_cluster_id_list ]-------------------------
sub cbgp_valid_bgp_dp_cluster_id_list($)
{
    my ($cbgp)= @_;

    return TEST_SKIPPED;
}

# -----[ cbgp_valid_bgp_dp_router_id ]-------------------------------
sub cbgp_valid_bgp_dp_router_id($)
{
    my ($cbgp)= @_;
    cbgp_topo_dp($cbgp, 2);
    cbgp_recv_update($cbgp, "1.0.0.1", 1, "2.0.0.1",
		     "255/16|2 4|IGP|2.0.0.1|0|0");
    cbgp_recv_update($cbgp, "1.0.0.1", 1, "3.0.0.1",
		     "255/16|3 4|IGP|3.0.0.1|0|0");
    die if $cbgp->send("sim run\n");
    my $rib;
    my @routes;
    $rib= cbgp_show_rib($cbgp, "1.0.0.1", "255/16");
    @routes= values %$rib; 
    if (scalar(@routes) < 1) {
	return TEST_FAILURE;
    }
    if ($routes[0]->[CBGP_RIB_NEXTHOP] ne "2.0.0.1") {
	return TEST_FAILURE;
    }
    return TEST_SUCCESS;
}

# -----[ cbgp_valid_bgp_filter ]-------------------------------------
sub cbgp_valid_bgp_filter($)
{
    my ($cbgp)= @_;

    return TEST_SKIPPED;
}

# -----[ cbgp_valid_bgp_filter_action_community ]--------------------
sub cbgp_valid_bgp_filter_action_community()
{
    my ($cbgp)= @_;
    cbgp_topo_filter($cbgp);
    cbgp_filter($cbgp, "1.0.0.1", "2.0.0.1", "in", "any", "community add 1:2");
    cbgp_recv_update($cbgp, "1.0.0.1", 1, "2.0.0.1",
		     "255/16|2|IGP|2.0.0.1|0|0");
    die if $cbgp->send("sim run\n");
    my $rib;
    $rib= cbgp_show_rib_mrt($cbgp, "1.0.0.1");
    if (!exists($rib->{"255.0.0.0/16"})) {
	return TEST_FAILURE;
    }
    if (!community_equals($rib->{"255.0.0.0/16"}->[CBGP_RIB_COMMUNITY],
			 ["1:2"])) {
	return TEST_FAILURE;
    }
    return TEST_SUCCESS;
}

# -----[ cbgp_valid_bgp_filter_action_deny ]-------------------------
sub cbgp_valid_bgp_filter_action_deny($)
{
    my ($cbgp)= @_;
    cbgp_topo_filter($cbgp);
    cbgp_filter($cbgp, "1.0.0.1", "2.0.0.1", "in", "any", "deny");
    cbgp_recv_update($cbgp, "1.0.0.1", 1, "2.0.0.1",
		     "255/16|2|IGP|2.0.0.1|0|0");
    die if $cbgp->send("sim run\n");
    my $rib;
    $rib= cbgp_show_rib($cbgp, "1.0.0.1");
    if (exists($rib->{"255.0.0.0/16"})) {
	return TEST_FAILURE;
    }
    return TEST_SUCCESS;
}

# -----[ cbgp_valid_bgp_filter_action_localpref ]--------------------
sub cbgp_valid_bgp_filter_action_localpref($)
{
    my ($cbgp)= @_;
    cbgp_topo_filter($cbgp);
    cbgp_filter($cbgp, "1.0.0.1", "2.0.0.1", "in", "any", "local-pref 57");
    die if $cbgp->send("bgp router 1.0.0.1 peer 2.0.0.1 recv ".
		       "\"BGP4|0|A|1.0.0.1|1|255/16|2|IGP|2.0.0.1|0|0\"\n");
    die if $cbgp->send("sim run\n");
    my $rib;
    $rib= cbgp_show_rib($cbgp, "1.0.0.1");
    if (!exists($rib->{"255.0.0.0/16"})) {
	return TEST_FAILURE;
    }
    if ($rib->{"255.0.0.0/16"}->[CBGP_RIB_PREF] != 57) {
	return TEST_FAILURE;
    }
    return TEST_SUCCESS;
}

# -----[ cbgp_valid_bgp_filter_action_med ]--------------------------
sub cbgp_valid_bgp_filter_action_med()
{
    my ($cbgp)= @_;
    cbgp_topo_filter($cbgp);
    cbgp_filter($cbgp, "1.0.0.1", "2.0.0.1", "in", "any", "metric 57");
    die if $cbgp->send("bgp router 1.0.0.1 peer 2.0.0.1 recv ".
		       "\"BGP4|0|A|1.0.0.1|1|255/16|2|IGP|2.0.0.1|0|0\"\n");
    die if $cbgp->send("sim run\n");
    my $rib;
    $rib= cbgp_show_rib($cbgp, "1.0.0.1");
    if (!exists($rib->{"255.0.0.0/16"})) {
	return TEST_FAILURE;
    }
    if ($rib->{"255.0.0.0/16"}->[CBGP_RIB_MED] != 57) {
	return TEST_FAILURE;
    }
    # TODO: test IGP-based med values
    return TEST_SUCCESS;
}

# -----[ cbgp_valid_bgp_filter_action_aspath ]-----------------------
sub cbgp_valid_bgp_filter_action_aspath()
{
    my ($cbgp)= @_;
    cbgp_topo_filter($cbgp);
    cbgp_filter($cbgp, "1.0.0.1", "2.0.0.1", "in", "any", "as-path prepend 2");
    die if $cbgp->send("bgp router 1.0.0.1 peer 2.0.0.1 recv ".
		       "\"BGP4|0|A|1.0.0.1|1|255/16|2|IGP|2.0.0.1|0|0\"\n");
    die if $cbgp->send("sim run\n");
    my $rib;
    $rib= cbgp_show_rib($cbgp, "1.0.0.1");
    if (!exists($rib->{"255.0.0.0/16"})) {
	return TEST_FAILURE;
    }
    if (!aspath_equals($rib->{"255.0.0.0/16"}->[CBGP_RIB_PATH], [1, 1, 2])) {
	return TEST_FAILURE;
    }
    return TEST_SUCCESS;
}

# -----[ cbgp_valid_bgp_filter_match_nexthop ]-----------------------
sub cbgp_valid_bgp_filter_match_nexthop($)
{
    my ($cbgp)= @_;
    cbgp_topo_dp3($cbgp, ["2.0.0.1", 2, 0], ["2.0.0.2", 2, 0],
		  ["2.0.0.3", 2, 0], ["2.0.1.4", 2, 0]);
    cbgp_filter($cbgp, "1.0.0.1", "2.0.0.1", "in", "(next-hop is 2.0.0.2)", "community add 1:2");
    cbgp_filter($cbgp, "1.0.0.1", "2.0.0.1", "in", "(next-hop is 2.0.0.3)", "community add 1:3");
    cbgp_filter($cbgp, "1.0.0.1", "2.0.0.1", "in", "(next-hop in 2.0.0/24)", "community add 1:2000");
    cbgp_filter($cbgp, "1.0.0.1", "2.0.0.1", "in", "(next-hop in 2.0.1/24)", "community add 1:2001");
    cbgp_recv_update($cbgp, "1.0.0.1", 1, "2.0.0.1",
		     "255/8|2|IGP|2.0.0.2|0|0");
    cbgp_recv_update($cbgp, "1.0.0.1", 1, "2.0.0.1",
		     "254/8|2|IGP|2.0.0.3|0|0");
    cbgp_recv_update($cbgp, "1.0.0.1", 1, "2.0.0.1",
		     "253/8|2|IGP|2.0.1.4|0|0");
    my $rib;
    $rib= cbgp_show_rib_mrt($cbgp, "1.0.0.1");
    (exists($rib->{"255.0.0.0/8"})) or return TEST_FAILURE;
    (community_equals($rib->{"255.0.0.0/8"}->[CBGP_RIB_COMMUNITY],
		      ["1:2", "1:2000"])) or
			  return TEST_FAILURE;
    (exists($rib->{"254.0.0.0/8"})) or return TEST_FAILURE;
    (community_equals($rib->{"254.0.0.0/8"}->[CBGP_RIB_COMMUNITY],
		      ["1:3", "1:2000"])) or
	return TEST_FAILURE;
    (exists($rib->{"253.0.0.0/8"})) or return TEST_FAILURE;
    (community_equals($rib->{"253.0.0.0/8"}->[CBGP_RIB_COMMUNITY],
		      ["1:2001"])) or
	return TEST_FAILURE;
    return TEST_SUCCESS;
}

# -----[ cbgp_valid_bgp_filter_match_prefix ]------------------------
sub cbgp_valid_bgp_filter_match_prefix()
{
    return TEST_SKIPPED;
}

# -----[ cbgp_valid_bgp_filter_match_aspath ]------------------------
sub cbgp_valid_bgp_filter_match_aspath()
{
    return TEST_SKIPPED;
}

# -----[ cbgp_valid_bgp_filter_match_community ]---------------------
sub cbgp_valid_bgp_filter_match_community()
{
    my ($cbgp)= @_;
    cbgp_topo_dp3($cbgp, ["2.0.0.1", 2, 0]);
    cbgp_filter($cbgp, "1.0.0.1", "2.0.0.1", "in",
		"community is 1:100",
		"local-pref 100");
    cbgp_filter($cbgp, "1.0.0.1", "2.0.0.1", "in",
		"community is 1:80",
		"local-pref 80");
    cbgp_filter($cbgp, "1.0.0.1", "2.0.0.1", "in",
		"community is 1:60",
		"local-pref 60");
    cbgp_recv_update($cbgp, "1.0.0.1", 1, "2.0.0.1",
		     "255/8|2|IGP|2.0.0.1|0|0|1:100");
    cbgp_recv_update($cbgp, "1.0.0.1", 1, "2.0.0.1",
		     "254/8|2|IGP|2.0.0.1|0|0|1:80");
    cbgp_recv_update($cbgp, "1.0.0.1", 1, "2.0.0.1",
		     "253/8|2|IGP|2.0.0.1|0|0|1:60");
    my $rib;
    $rib= cbgp_show_rib_mrt($cbgp, "1.0.0.1");
    (exists($rib->{"255.0.0.0/8"})) or return TEST_FAILURE;
    ($rib->{"255.0.0.0/8"}->[CBGP_RIB_PREF] == 100) or
	return TEST_FAILURE;
    (exists($rib->{"254.0.0.0/8"})) or return TEST_FAILURE;
    ($rib->{"254.0.0.0/8"}->[CBGP_RIB_PREF] == 80) or
	return TEST_FAILURE;
    (exists($rib->{"253.0.0.0/8"})) or return TEST_FAILURE;
    ($rib->{"253.0.0.0/8"}->[CBGP_RIB_PREF] == 60) or
	return TEST_FAILURE;
    return TEST_SUCCESS;
}

# -----[ cbgp_valid_igp_bgp ]----------------------------------------
sub cbgp_valid_igp_bgp()
{
    my ($cbgp)= @_;
    my $topo= topo_from_ntf("valid-igp-bgp.ntf");
    cbgp_topo($cbgp, $topo);
    cbgp_topo_igp_compute($cbgp, $topo, "1.0.0/24");
    die if $cbgp->send("bgp add router 1 1.0.0.1\n");
    die if $cbgp->send("bgp add router 1 1.0.0.2\n");
    die if $cbgp->send("bgp add router 1 1.0.0.3\n");
    die if $cbgp->send("bgp add router 1 1.0.0.4\n");
    die if $cbgp->send("bgp add router 1 1.0.0.5\n");
    die if $cbgp->send("bgp domain 1 full-mesh\n");
    die if $cbgp->send("net node 1.0.0.1 route add 2.0.0.1/32 2.0.0.1 0\n");
    die if $cbgp->send("net node 1.0.0.2 route add 2.0.0.2/32 2.0.0.2 0\n");
    die if $cbgp->send("net node 1.0.0.3 route add 2.0.0.3/32 2.0.0.3 0\n");
    cbgp_peering($cbgp, "1.0.0.1", "2.0.0.1", 2,
		 "virtual", "next-hop-self", "soft-restart");
    cbgp_peering($cbgp, "1.0.0.2", "2.0.0.2", 2,
		 "virtual", "next-hop-self", "soft-restart");
    cbgp_peering($cbgp, "1.0.0.3", "2.0.0.3", 2,
		 "virtual", "next-hop-self", "soft-restart");
    cbgp_recv_update($cbgp, "1.0.0.1", 1, "2.0.0.1",
		     "255/8|2|IGP|2.0.0.1|0|0");
    cbgp_recv_update($cbgp, "1.0.0.2", 1, "2.0.0.2",
		     "255/8|2|IGP|2.0.0.2|0|0");
    cbgp_recv_update($cbgp, "1.0.0.3", 1, "2.0.0.3",
		     "255/8|2|IGP|2.0.0.3|0|0");
    die if $cbgp->send("sim run\n");
    my $rib;
    $rib= cbgp_show_rib($cbgp, "1.0.0.4");
    (exists($rib->{"255.0.0.0/8"})) or return TEST_FAILURE;
    ($rib->{"255.0.0.0/8"}->[CBGP_RIB_NEXTHOP] eq "1.0.0.1") or
	return TEST_FAILURE;
    $rib= cbgp_show_rib($cbgp, "1.0.0.5");
    (exists($rib->{"255.0.0.0/8"})) or return TEST_FAILURE;
    ($rib->{"255.0.0.0/8"}->[CBGP_RIB_NEXTHOP] eq "1.0.0.3") or
	return TEST_FAILURE;
    # Peering failure 1.0.0.1 2.0.0.1
    die if $cbgp->send("net link 1.0.0.1 2.0.0.1 down\n");
    cbgp_topo_igp_compute($cbgp, $topo, "1.0.0/24");
    die if $cbgp->send("bgp domain 1 rescan\n");
    die if $cbgp->send("sim run\n");
    $rib= cbgp_show_rib($cbgp, "1.0.0.4");
    (exists($rib->{"255.0.0.0/8"})) or return TEST_FAILURE;
    ($rib->{"255.0.0.0/8"}->[CBGP_RIB_NEXTHOP] eq "1.0.0.2") or
	return TEST_FAILURE;
    # Link failure 1.0.0.1 1.0.0.2
    die if $cbgp->send("net link 1.0.0.1 1.0.0.2 down\n");
    cbgp_topo_igp_compute($cbgp, $topo, "1.0.0/24");
    die if $cbgp->send("bgp domain 1 rescan\n");
    die if $cbgp->send("sim run\n");
    $rib= cbgp_show_rib($cbgp, "1.0.0.4");
    (exists($rib->{"255.0.0.0/8"})) or return TEST_FAILURE;
    ($rib->{"255.0.0.0/8"}->[CBGP_RIB_NEXTHOP] eq "1.0.0.3") or
	return TEST_FAILURE;
    # Peering failure 1.0.0.3 2.0.0.3
    die if $cbgp->send("net link 1.0.0.3 2.0.0.3 down\n");
    cbgp_topo_igp_compute($cbgp, $topo, "1.0.0/24");
    die if $cbgp->send("bgp domain 1 rescan\n");
    die if $cbgp->send("sim run\n");
    $rib= cbgp_show_rib($cbgp, "1.0.0.4");
    (exists($rib->{"255.0.0.0/8"})) or return TEST_FAILURE;
    ($rib->{"255.0.0.0/8"}->[CBGP_RIB_NEXTHOP] eq "1.0.0.2") or
	return TEST_FAILURE;
    $rib= cbgp_show_rib($cbgp, "1.0.0.5");
    (exists($rib->{"255.0.0.0/8"})) or return TEST_FAILURE;
    ($rib->{"255.0.0.0/8"}->[CBGP_RIB_NEXTHOP] eq "1.0.0.2") or
	return TEST_FAILURE;
    print STDERR " OK :-)";
    # TODO: test link up
    # TODO: check presence of routes in Adj-RIB-ins, etc.
    # TODO: check presence of routes in IP table...
    return TEST_SUCCESS;
}

# -----[ cbgp_valid_bgp_load_rib ]-----------------------------------
sub cbgp_valid_bgp_load_rib($)
{
    my ($cbgp)= @_;
    die if $cbgp->send("net add node 213.200.87.254\n");
    die if $cbgp->send("bgp add router 3257 213.200.87.254\n");
    die if $cbgp->send("bgp router 213.200.87.254 load rib rib.20050401.0116.3257\n");
    my $rib;
    $rib= cbgp_show_rib($cbgp, "213.200.87.254");
    if (scalar(keys %$rib) != `cat rib.20050401.0116.3257 | wc -l`) {
	return TEST_FAILURE;
    }
    open(RIB, "<rib.20050401.0116.3257") or die;
    while (<RIB>) {
	chomp;
	my @fields= split /\|/;
	$rib= cbgp_show_rib($cbgp, "213.200.87.254", $fields[MRT_PREFIX]);
	if (scalar(keys %$rib) != 1) {
	    print "no route\n";
	    return TEST_FAILURE;
	}
	if (!exists($rib->{$fields[MRT_PREFIX]})) {
	    print "could not find route towards prefix $fields[MRT_PREFIX]\n";
	    return TEST_FAILURE;
	}
	if ($fields[MRT_NEXTHOP] ne
	    $rib->{$fields[MRT_PREFIX]}->[CBGP_RIB_NEXTHOP]) {
	    print "incorrect next-hop for $fields[MRT_PREFIX]\n";
	    return TEST_FAILURE;
	}
	#print STDERR "\rcheck prefix $fields[MRT_PREFIX] ".
	#    "$rib->{$fields[MRT_PREFIX]}->[CBGP_RIB_PREFIX]       ";
    }
    close(RIB);
    return TEST_SUCCESS;
}

# -----[ cbgp_valid_bgp_deflection ]---------------------------------
sub cbgp_valid_bgp_deflection()
{
    return TEST_SKIPPED;
}

# -----[ cbgp_valid_bgp_rr ]-----------------------------------------
sub cbgp_valid_bgp_rr()
{

    return TEST_SKIPPED;
}

# -----[ main ]------------------------------------------------------
show_info("c-bgp validation ".VERSION);
show_info("(c) 2005, Bruno Quoitin");

my %opts;
if (!GetOptions(\%opts, "cbgp-path:s",
		"cache!")) {
    show_error("Invalid command-line options");
    exit(-1);
}
if (exists($opts{"cbgp-path"})) {
    $cbgp_path= $opts{"cbgp-path"};
}
if (exists($opts{"cache"})) {
    $use_cache= $opts{"cache"};
}

my $topo= topo_3nodes_triangle();

my @tests;
push @tests, (["show version", \&cbgp_valid_version]);
push @tests, (["net create", \&cbgp_valid_net_create, $topo]);
push @tests, (["net igp", \&cbgp_valid_net_igp, $topo]);
push @tests, (["net ntf load", \&cbgp_valid_net_ntf_load,
	       "valid-record-route.ntf"]);
push @tests, (["net record-route", \&cbgp_valid_net_record_route, $topo]);
push @tests, (["net static routes", \&cbgp_valid_net_static_routes, $topo]);
push @tests, (["net longest-matching", \&cbgp_valid_net_longest_matching]);
push @tests, (["net protocol priority", \&cbgp_valid_net_protocol_priority]);
push @tests, (["bgp session ibgp", \&cbgp_valid_bgp_session_ibgp]);
push @tests, (["bgp session ebgp", \&cbgp_valid_bgp_session_ebgp]);
push @tests, (["bgp as-path loop", \&cbgp_valid_bgp_aspath_loop]);
push @tests, (["bgp ssld", \&cbgp_valid_bgp_ssld]);
#push @tests, (["bgp session state-machine", \&cbgp_valid_bgp_session_sm]);
push @tests, (["bgp domain full-mesh",
	       \&cbgp_valid_bgp_domain_fullmesh, $topo]);
push @tests, (["bgp peering up/down", \&cbgp_valid_bgp_peering_up_down]);
push @tests, (["bgp peering next-hop-self",
	       \&cbgp_valid_bgp_peerings_nexthopself]);
push @tests, (["bgp virtual peer", \&cbgp_valid_bgp_virtual_peer]);
push @tests, (["bgp recv mrt", \&cbgp_valid_bgp_recv_mrt]);
push @tests, (["bgp soft-restart", \&cbgp_valid_bgp_soft_restart]);
push @tests, (["bgp decision-process local-pref",
	       \&cbgp_valid_bgp_dp_localpref]);
push @tests, (["bgp decision process as-path", \&cbgp_valid_bgp_dp_aspath]);
push @tests, (["bgp decision process ebgp vs ibgp",
	       \&cbgp_valid_bgp_dp_ebgp_ibgp]);
push @tests, (["bgp decision process med", \&cbgp_valid_bgp_dp_med]);
push @tests, (["bgp decision process igp", \&cbgp_valid_bgp_dp_igp]);
push @tests, (["bgp decision process cluster-id-list",
	       \&cbgp_valid_bgp_dp_cluster_id_list]);
push @tests, (["bgp decision process router-id",
	       \&cbgp_valid_bgp_dp_router_id]);
push @tests, (["bgp filter", \&cbgp_valid_bgp_filter]);
push @tests, (["bgp filter action deny",
	       \&cbgp_valid_bgp_filter_action_deny]);
push @tests, (["bgp filter action community",
	       \&cbgp_valid_bgp_filter_action_community]);
push @tests, (["bgp filter action local-pref",
	       \&cbgp_valid_bgp_filter_action_localpref]);
push @tests, (["bgp filter action med", \&cbgp_valid_bgp_filter_action_med]);
push @tests, (["bgp filter action as-path",
	       \&cbgp_valid_bgp_filter_action_aspath]);
push @tests, (["bgp filter match community",
	       \&cbgp_valid_bgp_filter_match_community]);
push @tests, (["bgp filter match as-path",
	       \&cbgp_valid_bgp_filter_match_aspath]);
push @tests, (["bgp filter match next-hop",
	       \&cbgp_valid_bgp_filter_match_nexthop]);
push @tests, (["bgp filter match prefix",
	       \&cbgp_valid_bgp_filter_match_prefix]);
push @tests, (["igp/bgp", \&cbgp_valid_igp_bgp]);
push @tests, (["bgp load rib", \&cbgp_valid_bgp_load_rib]);
push @tests, (["bgp deflection", \&cbgp_valid_bgp_deflection]);
push @tests, (["bgp route-reflection", \&cbgp_valid_bgp_rr]);
#push @tests, (["bgp route-reflection (originator-id)",
#\&cbgp_valid_bgp_rr]);
#push @tests, (["bgp route-reflection (cluster-id-list)",
#\&cbgp_valid_bgp_rr]);
#push @tests, (["bgp route-reflection (cluster-id-list)",
#\&cbgp_valid_bgp_rr]);
push @tests, (["bgp implicit-withdraw",
	       \&cbgp_valid_bgp_implicit_withdraw]);
#push @tests, (["", \&]);

($use_cache) and test_cache_read();

foreach my $test (@tests) {
    my $test_name= shift @$test;
    show_testing($test_name);
    my $result;
    if (!exists($cache{$test_name}) ||
	($cache{$test_name} != TEST_SUCCESS)) {
	my $cbgp= CBGP->new($cbgp_path);
	$cbgp->{log}= 1;
	$cbgp->spawn();
	die if $cbgp->send("set autoflush on\n");
	my $func= shift @$test;
	$result= &$func($cbgp, @$test);
	$cbgp->finalize();
	if ($result == TEST_SUCCESS) {
	    show_success();
	} elsif ($result == TEST_SKIPPED) {
	    show_skipped();
	} else {
	    show_failure();
	}
	$cache{$test_name}= $result;
    } else {
	show_cache();
    }
    if ($num_failures >= $max_failures) {
	show_error("Error: too many failures.");
	last;
    }
}

if ($num_failures > 0) {
    show_error("$num_failures test(s) failed.");
} else {
    show_info("all tests passed.");
}

test_cache_write();

show_info("done.");
if ($num_failures > 1) {
    exit(-1);
} else {
    exit(0);
}
