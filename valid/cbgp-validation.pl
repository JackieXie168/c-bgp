#!/usr/bin/perl -w
# ===================================================================
# @(#)cbgp-validation.pl
#
# This script is used to validate C-BGP. It performs various tests in
# order to detect erroneous behaviour.
#
# @author Bruno Quoitin (bqu@info.ucl.ac.be)
# @lastdate 07/11/2006
# ===================================================================
# Syntax:
#
#   ./cbgp-validation.pl [OPTIONS]
#
# where possible options are
#
#   --cbgp-path=PATH
#                   Specifies the path of the C-BGP binary. The
#                   default path is "../src/valid"
#
#   --no-cache
#                   The default behaviour of the validation script is
#                   to store (cache) the test results. When a test
#                   was succesfull, it is not executed again, unless
#                   the --no-cache argument is passed.
#
#  --include=TEST
#                   Specifies a TEST to include in the validation.
#                   This argument can be mentionned multiple times in
#                   order to include multiple tests. Note that a test
#                   is executed only once, regardless of the number
#                   of times it appears with --include. The default
#                   behaviour is to execute all the tests. If at
#                   least one --include argument is passed, the
#                   script will only execute the explicitly specified
#                   tests.
#
# --report=FORMAT
#                   Generates a report summarizing the tests results.
#                   The script currently only support HTML format, so
#                   use --report=html. The report is composed of two
#                   pages. The first one shows a table with all tests
#                   results. The second one contains the description
#                   of each test. The description is automatically
#                   extracted from the test Perl functions.
#
# --report-prefix=PREFIX
#                   Changes the prefix of the report file names. The
#                   result page will be called "PREFIX.html" while
#                   the documentation page will be called
#                   "PREFIX-doc.html".
#
# --resources-path=PATH
#                   Set the path to the resources needed by the
#                   validation script. The needed resources are the
#                   topology files, the BGP RIB dumps, and so on.
#                   Usually, this path must be set to the installation
#                   directory of the validation script.
#
# ===================================================================

use strict;

use Getopt::Long;
use CBGP;
use CBGPValid::BaseReport;
use CBGPValid::HTMLReport;
use CBGPValid::TestConstants;
use CBGPValid::Tests;
use CBGPValid::UI;
use POSIX;

use constant CBGP_VALIDATION_VERSION => '1.7.0';

# -----[ IP link fields ]-----
use constant CBGP_LINK_TYPE => 0;
use constant CBGP_LINK_DST => 1;
use constant CBGP_LINK_WEIGHT => 2;
use constant CBGP_LINK_DELAY => 3;
use constant CBGP_LINK_STATE => 4;

# -----[ BGP peer fields ]-----
use constant CBGP_PEER_IP => 0;
use constant CBGP_PEER_AS => 1;
use constant CBGP_PEER_STATE => 2;
use constant CBGP_PEER_ROUTERID => 3;
use constant CBGP_PEER_OPTIONS => 4;

# -----[ IP route fields ]-----
use constant CBGP_RT_NEXTHOP => 0;
use constant CBGP_RT_IFACE => 1;
use constant CBGP_RT_METRIC => 2;
use constant CBGP_RT_PROTO => 3;

# -----[ Trace route fields ]-----
use constant CBGP_TR_SRC => 0;
use constant CBGP_TR_DST => 1;
use constant CBGP_TR_STATUS => 2;
use constant CBGP_TR_PATH => 3;
use constant CBGP_TR_WEIGHT => 4;
use constant CBGP_TR_DELAY => 5;
use constant CBGP_TR_BANDWIDTH => 6;

# -----[ BGP route fields ]-----
use constant CBGP_RIB_STATUS => 0;
use constant CBGP_RIB_PEER_IP => 1;
use constant CBGP_RIB_PEER_AS => 2;
use constant CBGP_RIB_PREFIX => 3;
use constant CBGP_RIB_NEXTHOP => 4;
use constant CBGP_RIB_PREF => 5;
use constant CBGP_RIB_MED => 6;
use constant CBGP_RIB_PATH => 7;
use constant CBGP_RIB_ORIGIN => 8;
use constant CBGP_RIB_COMMUNITY => 9;
use constant CBGP_RIB_ECOMMUNITY => 10;
use constant CBGP_RIB_ORIGINATOR => 11;
use constant CBGP_RIB_CLUSTERLIST => 12;

use constant MRT_PREFIX => 5;
use constant MRT_NEXTHOP => 8;

# -----[ Expected C-BGP version ]-----
# These are the expected major/minor versions of the C-BGP instance
# we are going to test.If these versions are lower than reported by
# the C-BGP instance, a warning will be issued.
use constant CBGP_MAJOR_MIN => 1;
use constant CBGP_MINOR_MIN => 2;

# -----[ Command-line options ]-----
my $max_failures= 0;
my $max_warnings= 0;
my $report_prefix= "cbgp-validation";
my $resources_path= "";

my $validation= {
		 'cbgp_version' => undef,
		 'program_args' => (join " ", @ARGV),
		 'program_name' => $0,
		 'program_version' => CBGP_VALIDATION_VERSION,
		};

show_info("c-bgp validation v".$validation->{'program_version'});
show_info("(c) 2006, Bruno Quoitin (bqu\@info.ucl.ac.be)");

my %opts;
if (!GetOptions(\%opts,
		"cbgp-path:s",
		"cache!",
		"debug!",
		"max-failures:i",
		"max-warnings:i",
		"include:s@",
		"report:s",
	        "report-prefix:s",
		"resources-path:s")) {
  show_error("Invalid command-line options");
  exit(-1);
}
if (exists($opts{'cache'}) && !$opts{'cache'}) {
  $opts{'cache'}= undef;
} else {
  $opts{'cache'}= ".$0.cache";
}

(!exists($opts{'cbgp-path'})) and
    $opts{'cbgp-path'}= "../src/cbgp";
(exists($opts{'resources-path'})) and
    $resources_path= $opts{'resources-path'};
($resources_path =~ s/^(.*[^\/])$/$1\//);
#(exists($opts{"max-failures"})) and
#  $max_failures= $opts{"max-failures"};
#(exists($opts{"max-warnings"})) and
#  $max_failures= $opts{"max-warnings"};
if (exists($opts{'include'})) {
  my %include_index= ();
  foreach my $test (@{$opts{'include'}}) {
    $include_index{$test}= 1;
  }
  $opts{'include'}= \%include_index;
} else {
  $opts{'include'}= undef;
}
(exists($opts{"report-prefix"})) and
  $report_prefix= $opts{"report-prefix"};

# -----[ Validation setup parameters ]-----
my $tests= CBGPValid::Tests->new(-debug=>$opts{'debug'},
				 -cache=>$opts{'cache'},
				 -cbgppath=>$opts{'cbgp-path'},
				 -include=>$opts{'include'});

# -----[ canonic_prefix ]--------------------------------------------
#
# -------------------------------------------------------------------
sub canonic_prefix($)
{
    my $prefix= shift;
    my ($network, $mask);

    if ($prefix =~ m/^([0-9.]+)\/([0-9]+)$/) {
	$network= $1;
	$mask= $2;
    } else {
	die "Error: invalid prefix [$prefix]";
    }

    my @fields= split /\./, $network;
    my $network_int= (($fields[0]*256+
		       (defined($fields[1])?$fields[1]:0))*256+
		      (defined($fields[2])?$fields[2]:0))*256+
		      (defined($fields[3])?$fields[3]:0);
    $network_int= $network_int >> (32-$mask);
    $network_int= $network_int << (32-$mask);
    return "".(($network_int >> 24) & 255).".".
	(($network_int >> 16) & 255).".".
	(($network_int >> 8) & 255).".".
	($network_int & 255)."/$mask";
}

# -----[ topo_from_ntf ]---------------------------------------------
# Load a topology from a NTF file.
# -------------------------------------------------------------------
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

# -----[ topo_from_subramanian ]-------------------------------------
# Load a topology from an AS-level topology (in "Subramanian" format).
# The resulting topology contains one router/AS. The IP address of
# each router is ASx.ASy.0.0
# -------------------------------------------------------------------
sub topo_from_subramanian($)
{
    my ($subramanian_file)= @_;
    my %topo;

    if (!open(SUBRAMANIAN, "<$subramanian_file")) {
	show_error("unable to open \"$subramanian_file\": $!");
	exit(-1);
    }
    while (<SUBRAMANIAN>) {
	chomp;
	(m/^\#/) and next;
	my @fields= split /\s+/;
	if (@fields != 3) {
	    show_error("invalid NTF record");
	    exit(-1);
	}
	my $ip0= int($fields[0] / 256).".".($fields[0] % 256).".0.0";
	my $ip1= int($fields[1] / 256).".".($fields[1] % 256).".0.0";
	$topo{$ip0}{$ip1}= [0, 0];
    }
    close(SUBRAMANIAN);

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

# -----[ topo_4nodes_star ]------------------------------------------
sub topo_star($)
{
    my ($num_nodes)= @_;
    my %topo;
    
    for (my $index= 0; $index < $num_nodes-1; $index++) {
	$topo{'1.0.0.1'}{'1.0.0.'.($index+2)}= [10, 10];
    }
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

# -----[ cbgp_send ]-------------------------------------------------
# Send a CLI command to C-BGP
# -------------------------------------------------------------------
sub cbgp_send($$)
{
  my ($cbgp, $cmd)= @_;
  die if $cbgp->send("$cmd\n");
}

# -----[ cbgp_checkpoint ]-------------------------------------------
# Wait until C-BGP has finished previous treatment...
# -------------------------------------------------------------------
sub cbgp_checkpoint($)
{
  my ($cbgp)= @_;
  cbgp_send($cbgp, "print \"CHECKPOINT\\n\"");
  while ((my $result= $cbgp->expect(1)) ne "CHECKPOINT") { sleep(1); }
}

# -----[ cbgp_check_error ]------------------------------------------
# Check if C-BGP has returned an error message.
#
# Return values:
#   undef      -> no error message was issued
#   <message>  -> error message returned by C-BGP
# -------------------------------------------------------------------
sub cbgp_check_error($)
{
  my ($cbgp)= @_;

  cbgp_send($cbgp, "print \"CHECKPOINT\\n\"");
  while ((my $result= $cbgp->expect(1)) ne "CHECKPOINT") {
    if ($result =~ m/^Error\:(.*)$/) {
      return $1;
    }
  }

  return undef;
}

# -----[ cbgp_topo ]-------------------------------------------------
sub cbgp_topo($$;$)
{
    my ($cbgp, $topo, $domain)= @_;
    my %nodes;

    if (defined($domain)) {
	cbgp_send($cbgp, "net add domain $domain igp");
    }

    foreach my $node1 (keys %$topo) {
	foreach my $node2 (keys %{$topo->{$node1}}) {
	    my ($delay, $weight)= @{$topo->{$node1}{$node2}};
	    if (!exists($nodes{$node1})) {
		cbgp_send($cbgp, "net add node $node1");
		if (defined($domain)) {
		    cbgp_send($cbgp, "net node $node1 domain $domain");
		}
		$nodes{$node1}= 1;
	    }
	    if (!exists($nodes{$node2})) {
		cbgp_send($cbgp, "net add node $node2");
		if (defined($domain)) {
		    cbgp_send($cbgp, "net node $node2 domain $domain");
		}
		$nodes{$node2}= 1;
	    }
	    cbgp_send($cbgp, "net add link $node1 $node2 $delay");
	    cbgp_send($cbgp, "net link $node1 $node2 igp-weight $weight");
	    cbgp_send($cbgp, "net link $node2 $node1 igp-weight $weight");
	}
    }
}

# -----[ cbgp_topo_domain ]------------------------------------------
sub cbgp_topo_domain($$$$)
{
    my ($cbgp, $topo, $predicate, $domain)= @_;

    cbgp_send($cbgp, "net add domain $domain igp");

    my $nodes= topo_get_nodes($topo);
    foreach my $node (keys %$nodes) {
	if ($node =~ m/$predicate/) {
	    cbgp_send($cbgp, "net node $node domain $domain");
	}
    }
}

# -----[ cbgp_topo_filter ]------------------------------------------
# Create a simple topology suitable for testing filters.
#
# Setup:
#   - R1 (1.0.0.1) in AS1
#   - R2 (1.0.0.2) in AS2 virtual peer
# -------------------------------------------------------------------
sub cbgp_topo_filter($)
{
    my ($cbgp)= @_;

    cbgp_send($cbgp, "net add node 1.0.0.1");
    cbgp_send($cbgp, "net add node 2.0.0.1");
    cbgp_send($cbgp, "net add link 1.0.0.1 2.0.0.1 0");
    cbgp_send($cbgp, "net node 1.0.0.1 route add 2.0.0.1/32 * 2.0.0.1 0");
    cbgp_send($cbgp, "bgp add router 1 1.0.0.1");
    cbgp_peering($cbgp, "1.0.0.1", "2.0.0.1", 2, "virtual");
}

# -----[ cbgp_topo_dp ]----------------------------------------------
# Create a simple topology suitable for testing the decision
# process. All peers are in different ASs.
#
# Setup:
#   - R1 (1.0.0.1) in AS1
#   - R2 (2.0.0.1) in AS2 virtual peer
#   - ...
#   - R[n+1] ([n+1].0.0.1) in AS[n+1] virtual peer
# -------------------------------------------------------------------
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
	die if $cbgp->send("net node 1.0.0.1 route add $peer/32 * $peer 0\n");
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
	die if $cbgp->send("net node 1.0.0.1 route add $peer/32 * $peer 0\n");
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
	die if $cbgp->send("net node 1.0.0.1 route add $peer/32 * $peer $weight\n");
	cbgp_peering($cbgp, "1.0.0.1", $peer, $asn, "virtual");
    }
}

# -----[ cbgp_topo_dp4 ]---------------------------------------------
# Create a topology composed of a central router R1 (1.0.0.1, AS1)
# and a set of virtual and non-virtual peers.
#
# The peer specification is as follows:
#   <IP address>, <ASN>, <IGP weight>, <virtual?>, <options>
# -------------------------------------------------------------------
sub cbgp_topo_dp4($;@)
{
  my ($cbgp, @peers)= @_;

  die if $cbgp->send("net add node 1.0.0.1\n");
  die if $cbgp->send("bgp add router 1 1.0.0.1\n");
  for (my $index= 0; $index < @peers; $index++) {
    my @peer_record= @{$peers[$index]};
    my $peer= shift @peer_record;
    my $asn= shift @peer_record;
    my $weight= shift @peer_record;
    my $virtual= shift @peer_record;
    die if $cbgp->send("net add node $peer\n");
    die if $cbgp->send("net add link 1.0.0.1 $peer $weight\n");
    die if $cbgp->send("net node 1.0.0.1 route add $peer/32 * $peer $weight\n");
    if ($virtual) {
      cbgp_peering($cbgp, "1.0.0.1", $peer, $asn, "virtual", @peer_record);
    } else {
      die if $cbgp->send("net node $peer route add 1.0.0.1/32 * 1.0.0.1 $weight\n");
      die if $cbgp->send("bgp add router $asn $peer\n");
      cbgp_peering($cbgp, "1.0.0.1", $peer, $asn, @peer_record);
      cbgp_peering($cbgp, $peer, "1.0.0.1", 1);
    }
  }
}

# -----[ cbgp_topo_igp_bgp ]-----------------------------------------
# Create a topology along with BGP configuration suitable for testing
# IGP/BGP interaction scenari.
#
# R1.2 --(W2)--\
#               \
# R1.3 --(W3)----*--- R1.1 --- R2.1
#               /
# R2.n --(Wn)--/
#
# AS1: R1.1 - R1.n
# AS2: R2.1
#
# Wi = weight of link
# -------------------------------------------------------------------
sub cbgp_topo_igp_bgp($;@)
{
  my $cbgp= shift @_;

  die if $cbgp->send("net add domain 1 igp\n");
  die if $cbgp->send("net add node 1.0.0.1\n");
  die if $cbgp->send("net node 1.0.0.1 domain 1\n");

  foreach my $peer_spec (@_) {
    my $peer_ip= $peer_spec->[0];
    my $peer_weight= $peer_spec->[1];
    die if $cbgp->send("net add node $peer_ip\n");
    die if $cbgp->send("net node $peer_ip domain 1\n");
    die if $cbgp->send("net add link 1.0.0.1 $peer_ip $peer_weight\n");
  }
  die if $cbgp->send("net domain 1 compute\n");

  die if $cbgp->send("net add node 2.0.0.1\n");

  die if $cbgp->send("net add link 1.0.0.1 2.0.0.1 0\n");
  die if $cbgp->send("net node 1.0.0.1 route add 2.0.0.1/32 * 2.0.0.1 0\n");
  die if $cbgp->send("net node 2.0.0.1 route add 1.0.0.1/32 * 1.0.0.1 0\n");

  die if $cbgp->send("bgp add router 1 1.0.0.1\n");
  die if $cbgp->send("bgp router 1.0.0.1\n");
  foreach my $peer_spec (@_) {
    my $peer_ip= $peer_spec->[0];
    die if $cbgp->send("add peer 1 $peer_ip\n");
    die if $cbgp->send("peer $peer_ip virtual\n");
    for (my $index= 2; $index < scalar(@$peer_spec); $index++) {
      my $peer_option= $peer_spec->[$index];
      die if $cbgp->send("peer $peer_ip $peer_option\n");
    }
    die if $cbgp->send("peer $peer_ip up\n");
  }
  die if $cbgp->send("add peer 2 2.0.0.1\n");
  die if $cbgp->send("peer 2.0.0.1 up\n");
  die if $cbgp->send("exit\n");

  die if $cbgp->send("bgp add router 2 2.0.0.1\n");
  die if $cbgp->send("bgp router 2.0.0.1\n");
  die if $cbgp->send("add peer 1 1.0.0.1\n");
  die if $cbgp->send("peer 1.0.0.1 up\n");
  die if $cbgp->send("exit\n");
}

# -----[ cbgp_topo_igp_compute ]-------------------------------------
sub cbgp_topo_igp_compute($$$)
{
    my ($cbgp, $topo, $domain)= @_;
    cbgp_send($cbgp, "net domain $domain compute");
}

# -----[ cbgp_topo_bgp_routers ]-------------------------------------
sub cbgp_topo_bgp_routers($$$) {
    my ($cbgp, $topo, $asn)= @_;
    my $nodes= topo_get_nodes($topo);

    foreach my $node (keys %$nodes) {
	cbgp_send($cbgp, "bgp add router $asn $node");
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
	    if ($routes->{"$node2/32"}->[CBGP_RT_PROTO] ne "IGP") {
		return 0;
	    }
	}
    }
    return 1;
}

# -----[ cbgp_check_link ]-------------------------------------------
sub cbgp_check_link($$$%)
{
    my ($cbgp, $src, $dest, %args)= @_;

    my $args_str= '';
    foreach (keys %args) {
	$args_str.= ", $_=".$args{$_};
    }
    $tests->debug("check_link($src, $dest$args_str)");

    my $links;
    $links= cbgp_show_links($cbgp, $src);
    if (keys(%$links) == 0) {
	$tests->debug("ERROR no link from \"$src\"");
	return TEST_FAILURE;
    }

    if (!exists($links->{$dest})) {
	$tests->debug("ERROR no link towards \"$dest\"");
	return 0;
    }
    my $link= $links->{$dest};
    if (exists($args{-type})) {
	if ($link->[CBGP_LINK_TYPE] ne $args{-type}) {
	    $tests->debug("ERROR incorrect type \"".$link->[CBGP_LINK_TYPE].
		       "\" (expected=\"".$args{-type}."\")");
	    return 0;
	}
    }
    if (exists($args{-weight})) {
	if ($link->[CBGP_LINK_WEIGHT] != $args{-weight}) {
	    $tests->debug("ERROR incorrect weight \"".$link->[CBGP_LINK_WEIGHT].
		       "\" (expected=\"".$args{-weight}."\")");
	    return 0;
	}
    }
    if (exists($args{-delay})) {
	if ($link->[CBGP_LINK_DELAY] != $args{-delay}) {
	    $tests->debug("ERROR incorrect delay \"".$link->[CBGP_LINK_DELAY].
		       "\" (expected=\"".$args{-delay}."\")");
	    return 0;
	}
    }
    return 1;
}

# -----[ cbgp_rib_check ]--------------------------------------------
sub cbgp_rib_check($$;@)
{
    my ($rib, $prefix, @constraints)= @_;

    $prefix= canonic_prefix($prefix);

    if (!exists($rib->{$prefix})) {
	return 0;
    }
    my $route= $rib->{$prefix};
    foreach my $constraint (@constraints) {
	my ($field, $value)= @$constraint;
	if ($route->[$field] ne $value) {
	    return 0;
	}
    }
    return 1;
}

# -----[ cbgp_topo_check_ibgp_sessions ]-----------------------------
# Check that there is one iBGP session between each pair of nodes in
# the topology (i.e. a full-mesh of iBGP sessions).
#
# Optionally, check that the sessions' state is equal to the given
# state.
# -------------------------------------------------------------------
sub cbgp_topo_check_ibgp_sessions($$;$)
{
  my ($cbgp, $topo, $state)= @_;
  my $nodes= topo_get_nodes($topo);

  foreach my $node1 (keys %$nodes) {
    my $peers= cbgp_show_peers($cbgp, $node1);
    foreach my $node2 (keys %$nodes) {
      ($node1 eq $node2) and next;
      if (!exists($peers->{$node2})) {
	print "missing session $node1 <-> $node2\n";
	return 0;
      }
      if (defined($state) &&
	  ($peers->{$node2}->[CBGP_PEER_STATE] ne $state)) {
	print "$state\t$peers->{$node2}->[CBGP_PEER_STATE]\n";
	return 0;
      }
    }
  }
    return 1;
}

# -----[ cbgp_topo_check_ebgp_sessions ]-----------------------------
# Check that there is one eBGP session for each link in the topology.
# Check that sessions are defined in both ways.
#
# Optionally, check that the sessions' state is equal to the given
# state.
# -------------------------------------------------------------------
sub cbgp_topo_check_ebgp_sessions($$;$)
{
  my ($cbgp, $topo, $state)= @_;

  foreach my $node1 (keys %$topo) {
    my $peers1= cbgp_show_peers($cbgp, $node1);
    foreach my $node2 (keys %{$topo->{$node1}}) {
      my $peers2= cbgp_show_peers($cbgp, $node2);

      if (!exists($peers1->{$node2})) {
	print "missing session $node1 --> $node2\n";
	return 0;
      }
      if (defined($state) &&
	  ($peers1->{$node2}->[CBGP_PEER_STATE] ne $state)) {
	print "$state\t$peers1->{$node2}->[CBGP_PEER_STATE]\n";
	return 0;
      }

      if (!exists($peers2->{$node1})) {
	print "missing session $node2 --> $node1\n";
	return 0;
      }
      if (defined($state) &&
	  ($peers2->{$node1}->[CBGP_PEER_STATE] ne $state)) {
	print "$state\t$peers2->{$node1}->[CBGP_PEER_STATE]\n";
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

    $tests->debug("topo_check_record_route()");

    foreach my $node1 (keys %$nodes) {
	foreach my $node2 (keys %$nodes) {
	    ($node1 eq $node2) and next;
	    my $trace= cbgp_record_route($cbgp, $node1, $node2);

	    # Result should be equal to the given status
	    if ($trace->[CBGP_TR_STATUS] ne $status) {
		$tests->debug("ERROR incorrect status \"".
			   $trace->[CBGP_TR_STATUS].
			   "\" (expected=\"$status\")");
		return 0;
	    }

	    if ($trace->[CBGP_TR_STATUS] eq "SUCCESS") {
		# Last hop should be destination
		my @path= @{$trace->[CBGP_TR_PATH]};
		$tests->debug(join ", ", @path);
		if ($path[$#path] ne $node2) {
		    $tests->debug("ERROR final node != destination ".
			       "(\"".$path[$#path]."\" != \"$node2\")");
		    return 0;
		}
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
	foreach my $link (values %$links) {
	    die if $cbgp->send("net node $node1 route add ".
			       $link->[CBGP_LINK_DST]."/32 * ".
			       $link->[CBGP_LINK_DST]." ".
			       $link->[CBGP_LINK_WEIGHT]."\n");
	}

	# Test static routes (presence)
	my $rt= cbgp_show_rt($cbgp, $node1);
	(scalar(keys %$rt) != scalar(keys %$links)) and return 0;
	foreach my $link (values %$links) {
	    my $prefix= $link->[CBGP_LINK_DST]."/32";
	    # static route exists
	    (!exists($rt->{$prefix})) and return 0;
	    # next-hop is link's tail-end
	    if ($rt->{$prefix}->[CBGP_RT_IFACE] ne
		$link->[CBGP_LINK_DST]) {
		print "next-hop != link's tail-end\n";
		return 0;
	    }
	}
	
	# Test static routes (forward)
	foreach my $link (values %$links) {
	    my $trace= cbgp_record_route($cbgp, $node1,
					 $link->[CBGP_LINK_DST]);
	    # Destination was reached
	    ($trace->[CBGP_TR_STATUS] ne "SUCCESS") and return 0;
	    # Path is 2 hops long
	    (@{$trace->[CBGP_TR_PATH]} != 2) and return 0;
	}
	
    }

    return 1;
}

# -----[ cbgp_peering ]----------------------------------------------
# Setup a BGP peering between a 'router' and a 'peer' in AS 'asn'.
# Possible options are supported:
#   - virtual
#   - next-hop-self
#   - soft-restart
# -------------------------------------------------------------------
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
# Setup a filter on a BGP peering. The filter specification includes
# a direction (in/out), a predicate and a list of actions.
# -------------------------------------------------------------------
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
    my %links= ();

    die if $cbgp->send("net node $node show links\n");
    die if $cbgp->send("print \"done\\n\"\n");
    while ((my $result= $cbgp->expect(1)) ne "done") {

	my @fields= split /\s+/, $result;

	if (scalar(@fields) < 5) {
	    show_error("incorrect format (show links): \"$result\"");
	    exit(-1);
	}

	if (!($fields[0] =~ m/^ROUTER|TRANSIT|STUB$/)) {
	    show_error("invalid router-type (show links): $fields[0]");
	    exit(-1);
	}
	if (!($fields[1] =~ m/^[0-9.\/]+$/)) {
	    show_error("invalid destination (show links): $fields[1]");
	    exit(-1);
	}
	if (!($fields[2] =~ m/^[0-9]+$/)) {
	    show_error("invalid weight (show links): $fields[2]");
	    exit(-1);
	}
	if (!($fields[3] =~ m/^[0-9]+$/)) {
	    show_error("invalid delay (show links): $fields[3]");
	    exit(-1);
	}
	if (!($fields[4] =~ m/^UP|DOWN$/)) {
	    show_error("invalid state (show links): $fields[4]");
	    exit(-1);
	}
	
	my $link_type= $fields[0];
	my $link_destination= $fields[1];
	my $link_weight= $fields[2];
	my $link_delay= $fields[3];
	my $link_state= $fields[4];

	if (exists($links{$link_destination})) {
	    show_error("duplicate link destination: $link_destination");
	    exit(-1);
	}

	$links{$link_destination}=
	    [$link_type,
	     $link_destination,
	     $link_weight,
	     $link_delay,
	     $link_state,
	     ];
    }
    return \%links;
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
	my @fields= split /\s+/, $result;
	if (scalar(@fields) < 4) {
	    show_error("incorrect format (show rt): \"$result\"");
	    exit(-1);
	}

	if (!($fields[0] =~ m/^[0-9.\/]+$/)) {
	    show_error("invalid destination (show rt): $fields[0]");
	    exit(-1);
	}
	if (!($fields[1] =~ m/^[0-9.]+$/)) {
	    show_error("invalid next-hop (show rt): $fields[1]");
	    exit(-1);
	}
	if (!($fields[2] =~ m/^[0-9.\/]+$/)) {
	    show_error("invalid interface (show rt): $fields[2]");
	    exit(-1);
	}
	if (!($fields[3] =~ m/^[0-9]+$/)) {
	    show_error("invalid metric (show rt): $fields[3]");
	    exit(-1);
	}
	if (!($fields[4] =~ m/^STATIC|IGP|BGP$/)) {
	    show_error("invalid type (show rt): $fields[4]");
	    exit(-1);
	}

	my $rt_destination= $fields[0];
	my $rt_nexthop= $fields[1];
	my $rt_iface= $fields[2];
	my $rt_metric= $fields[3];
	my $rt_type= $fields[4];

	$routes{$rt_destination}=
	    [$rt_nexthop,
	     $rt_iface,
	     $rt_metric,
	     $rt_type
	     ];
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
    if ($result =~ m/^([0-9.]+)\s+AS([0-9]+)\s+([A-Z]+)\s+([0-9.]+)(\s+.+)?$/) {
      my @peer;
      $peer[CBGP_PEER_IP]= $1;
      $peer[CBGP_PEER_AS]= $2;
      $peer[CBGP_PEER_STATE]= $3;
      $peer[CBGP_PEER_ROUTERID]= $4;
      $peer[CBGP_PEER_OPTIONS]= $5;
      $peers{$1}= \@peer;
    } else {
      show_error("incorrect format (show peers): \"$result\"");
      exit(-1);
    }
  }
  return \%peers;
}

# -----[ bgp_route_parse_cisco ]-------------------------------------
# Parse a CISCO "show ip bgp" route.
#
# Return:
#   - in case of success: an anonymous array containing the
#                         route's  attributes.
#   - in case of failure: undef
# -------------------------------------------------------------------
sub bgp_route_parse_cisco($)
{
  my ($string)= @_;

  if ($string =~ m/^([i*>]+)\s+([0-9.\/]+)\s+([0-9.]+)\s+([0-9]+)\s+([0-9]+)\s+([^\t]*)\s+([ie\?])$/) {
    my @route;
    $route[CBGP_RIB_STATUS]= $1;
    $route[CBGP_RIB_PREFIX]= $2;
    $route[CBGP_RIB_NEXTHOP]= $3;
    $route[CBGP_RIB_PREF]= $4;
    $route[CBGP_RIB_MED]= $5;
    my @path= ();
    ($6 ne "null") and @path= split /\s+/, $6;
    $route[CBGP_RIB_PATH]= \@path;
    #$route[CBGP_RIB_]= $7;
    return \@route;
  }

  return undef;
}

# -----[ bgp_route_parse_mrt ]---------------------------------------
# Parse an MRT route.
#
# Return:
#   - in case of success: an anonymous array containing the
#                         route's  attributes.
#   - in case of failure: undef
# -------------------------------------------------------------------
sub bgp_route_parse_mrt($)
{
  my ($string)= @_;

  if ($string =~
      m/^(.?)\|(LOCAL|[0-9.]+)\|(LOCAL|[0-9]+)\|([0-9.\/]+)\|([0-9 null]*)\|(IGP|EGP|INCOMPLETE)\|([0-9.]+)\|([0-9]+)\|([0-9]*)\|.+$/) {
    my @fields= split /\|/, $string;
    my @route;
    $route[CBGP_RIB_STATUS]= $fields[0];
    $route[CBGP_RIB_PEER_IP]= $fields[1];
    $route[CBGP_RIB_PEER_AS]= $fields[2];
    $route[CBGP_RIB_PREFIX]= $fields[3];
    $route[CBGP_RIB_NEXTHOP]= $fields[6];
    my @path= ();
    ($fields[4] ne "null") and @path= split /\s/, $fields[4];
    $route[CBGP_RIB_PATH]= \@path;
    $route[CBGP_RIB_PREF]= $fields[7];
    $route[CBGP_RIB_MED]= $fields[8];
    if (@fields > 9) {
      my @communities= split /\s/, $fields[9];
      $route[CBGP_RIB_COMMUNITY]= \@communities;
    } else {
      $route[CBGP_RIB_COMMUNITY]= undef;
    }
    if (@fields > 10) {
      my @communities= split /\s/, $fields[10];
      $route[CBGP_RIB_ECOMMUNITY]= \@communities;
    } else {
      $route[CBGP_RIB_ECOMMUNITY]= undef;
    }
    if (@fields > 11) {
      $route[CBGP_RIB_ORIGINATOR]= $fields[11];
    } else {
      $route[CBGP_RIB_ORIGINATOR]= undef;
    }
    if (@fields > 12) {
      my @clusterlist= split /\s/, $fields[12];
      $route[CBGP_RIB_CLUSTERLIST]= \@clusterlist;
    } else {
      $route[CBGP_RIB_CLUSTERLIST]= undef;
    }
    return \@route;
  }

  return undef;
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
      my $route= bgp_route_parse_cisco($result);
      if (!defined($route)) {
	show_error("incorrect format (show rib): \"$result\"");
	exit(-1);
      }
      $rib{$route->[CBGP_RIB_PREFIX]}= $route;
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

    my $route= bgp_route_parse_mrt($result);
    if (!defined($route)) {
      show_error("incorrect format (show rib mrt): \"$result\"");
      exit(-1);
    }
    $rib{$route->[CBGP_RIB_PREFIX]}= $route;
  }
  return \%rib;
}

# -----[ cbgp_show_rib_custom ]--------------------------------------
sub cbgp_show_rib_custom($$;$)
{
  my ($cbgp, $node, $destination)= @_;
  my %rib;
  
  if (!defined($destination)) {
    $destination= "*";
  }
  
  die if $cbgp->send("bgp options show-mode custom \"?|%i|%a|%p|%P|%o|%n|%l|%m|%c|%e|%O|%C|\"\n");
  die if $cbgp->send("bgp router $node show rib $destination\n");
  die if $cbgp->send("print \"done\\n\"\n");
  die if $cbgp->send("bgp options show-mode cisco\n");
  while ((my $result= $cbgp->expect(1)) ne "done") {
    if ($result =~
	m/^(.?)\|(LOCAL|[0-9.]+)\|(LOCAL|[0-9]+)\|([0-9.\/]+)\|([0-9 null]+)\|(IGP|EGP|INCOMPLETE)\|([0-9.]+)\|([0-9]+)\|([0-9]*)\|.+$/) {
      my @fields= split /\|/, $result;

      my @route;
      $route[CBGP_RIB_STATUS]= 0; #$fields[0];
      $route[CBGP_RIB_PEER_IP]= $fields[1];
      $route[CBGP_RIB_PEER_AS]= $fields[2];
      $route[CBGP_RIB_PREFIX]= $fields[3];
      $route[CBGP_RIB_ORIGIN]= $fields[5];
      $route[CBGP_RIB_NEXTHOP]= $fields[6];
      my @path= ();
      ($fields[4] ne "null") and @path= split /\s/, $fields[4];
      $route[CBGP_RIB_PATH]= \@path;
      $route[CBGP_RIB_PREF]= $fields[7];
      $route[CBGP_RIB_MED]= $fields[8];
      if (@fields > 9) {
	my @communities= split /\s/, $fields[9];
	$route[CBGP_RIB_COMMUNITY]= \@communities;
      } else {
	$route[CBGP_RIB_COMMUNITY]= undef;
      }
      if (@fields > 10) {
	my @communities= split /\s/, $fields[10];
	$route[CBGP_RIB_ECOMMUNITY]= \@communities;
      } else {
	$route[CBGP_RIB_ECOMMUNITY]= undef;
      }
      if (@fields > 11) {
	$route[CBGP_RIB_ORIGINATOR]= $fields[11];
      } else {
	$route[CBGP_RIB_ORIGINATOR]= undef;
      }
      if (@fields > 12) {
	my @clusterlist= split /\s/, $fields[12];
	$route[CBGP_RIB_CLUSTERLIST]= \@clusterlist;
      } else {
	$route[CBGP_RIB_CLUSTERLIST]= undef;
      }
      $rib{$4}= \@route;
    } else {
      show_error("incorrect format (show rib mrt): \"$result\"");
      exit(-1);
    }
  }
  return \%rib;
}

# -----[ cbgp_record_route ]-----------------------------------------
sub cbgp_record_route($$$)
{
    my ($cbgp, $src, $dst)= @_;

    die if $cbgp->send("net node $src record-route $dst\n");
    my $result= $cbgp->expect(1);
    if ($result =~ m/^([0-9.]+)\s+([0-9.\/]+)\s+([A-Z_]+)\s+([0-9.\s]+)$/) {
	my @fields= split /\s+/, $result;

	my @trace;
	$trace[CBGP_TR_SRC]= shift @fields;
	$trace[CBGP_TR_DST]= shift @fields;
	$trace[CBGP_TR_STATUS]= shift @fields;
	$trace[CBGP_TR_PATH]= \@fields;
	return \@trace;
    } else {
	show_error("incorrect format (record-route): \"$result\"");
	exit(-1);
    }
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
		my $links_hash= cbgp_show_links($cbgp, $node1);
		my @links= values %$links_hash;
		foreach my $link (@links) {
		    $cbgp_topo{$node1}{$link->[CBGP_LINK_DST]}=
			[$link->[CBGP_LINK_WEIGHT],
			 $link->[CBGP_LINK_DELAY],
			 ];
		}
	    }
	    if (!exists($nodes{$node2})) {
		$nodes{$node2}= 1;
		my $links_hash= cbgp_show_links($cbgp, $node2);
		my @links= values %$links_hash;
		foreach my $link (@links) {
		    $cbgp_topo{$node2}{$link->[CBGP_LINK_DST]}=
			[$link->[CBGP_LINK_WEIGHT],
			 $link->[CBGP_LINK_DELAY],
			 ];
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

    $tests->debug("check_peering($router, $peer, $state)");

    my $peers= cbgp_show_peers($cbgp, $router);
    if (!exists($peers->{$peer})) {
	$tests->debug("=> ERROR peer does not exist");
	return 0;
    }
    if (defined($state) && ($peers->{$peer}->[CBGP_PEER_STATE] ne $state)) {
	$tests->debug("=> ERROR incorrect state \"".
		   $peers->{$peer}->[CBGP_PEER_STATE].
		   "\" (expected=\"$state\")");
	return 0;
    }
    $tests->debug("=> OK");
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
# Compare to AS-Paths.
#
# Return 1 if both AS-Paths are equal
#        0 if they differ
#
# Note: the function only supports AS-Paths composed of AS_SEQUENCEs
# -------------------------------------------------------------------
sub aspath_equals($$)
{
    my ($path1, $path2)= @_;

    (!defined($path1)) and $path1= [];
    (!defined($path2)) and $path2= [];

    $tests->debug("aspath_equals([".(join " ", @$path1)."], [".(join " ", @$path2)."])");

    (@$path1 != @$path2) and return 0;
    for (my $index= 0; $index < @$path1; $index++) {
	($path1->[$index] != $path2->[$index]) and return 0;
    }
    return 1;
}

# -----[ community_equals ]------------------------------------------
# Compare two sets of communities.
#
# Return 1 if both sets contain the same communities (ordering does
#          not matter!)
#        0 if sets differ
# -------------------------------------------------------------------
sub community_equals($$)
{
    my ($comm1, $comm2)= @_;

    if (!defined($comm1)) {
      (defined($comm2) && @$comm2) and return 0;
      (!defined($comm2) || !@$comm2) and return 1;
    }
    if (!defined($comm2)) {
      (defined($comm1) && @$comm1) and return 0;
      (!defined($comm1) || !@$comm1) and return 1;
    }


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

# -----[ clusterlist_equals ]----------------------------------------
# Compare two cluster-id-lists.
#
# Return 1 if both lists are equal (ordering matters!)
#        0 if they differ
# -------------------------------------------------------------------
sub clusterlist_equals($$)
{
  my ($cl1, $cl2)= @_;

  if (!defined($cl1)) {
    (defined($cl2) && @$cl2) and return 0;
    (!defined($cl2) || !@$cl2) and return 1;
  }
  if (!defined($cl2)) {
    (defined($cl1) && @$cl1) and return 0;
    (!defined($cl1) || !@$cl1) and return 1;
  }

  if (@$cl1 != @$cl2) {
    print "community length mismatch\n";
    return 0;
  }

  for (my $index= 0; $index < @$cl1; $index++) {
    ($cl1->[$index] ne $cl2->[$index]) and return 0;
  }
  return 1;
}

#####################################################################
#
# VALIDATION TESTS
#
#####################################################################

# -----[ cbgp_valid_version ]----------------------------------------
# Check that the version returned by C-BGP is correctly formated. This
# is the first test since it is also used to check that the C-BGP
# executable can be launched.
# -------------------------------------------------------------------
sub cbgp_valid_version($)
{
  my ($cbgp)= @_;
  cbgp_send($cbgp, "show version");
  my $result= $cbgp->expect(1);
  if ($result =~ m/^version:\s+cbgp\s+([0-9]+)\.([0-9]+)\.([0-9]+)(\-(.+))?(\s+.+)$/) {
    my $version= "$1.$2.$3";
    if (defined($4)) {
      $version.= " ($4)";
    }
    if (($1 < CBGP_MAJOR_MIN()) ||
	(($1 == CBGP_MAJOR_MIN()) && ($2 < CBGP_MINOR_MIN))) {
      show_warning("this validation script was designed for version ".
		   CBGP_MAJOR_MIN().".".CBGP_MINOR_MIN().".x");
    }
    $validation->{'cbgp_version'}= $version;
    return TEST_SUCCESS;
  } else {
    return TEST_FAILURE;
  }
}

# -----[ cbgp_valid_net_node ]---------------------------------------
# Test ability to create new nodes.
#
# Scenario:
#   * Create node 1.0.0.1
#   * Check that no error is issued
#   * Create node 123.45.67.89
#   * Check that no error is issued
#
#   TO-DO: read node info...
# -------------------------------------------------------------------
sub cbgp_valid_net_node($)
{
  my ($cbgp)= @_;

  cbgp_send($cbgp, "net add node 1.0.0.1");
  my $error_msg= cbgp_check_error($cbgp);
  if (defined($error_msg)) {
    return TEST_FAILURE;
  }

  cbgp_send($cbgp, "net add node 123.45.67.89");
  $error_msg= cbgp_check_error($cbgp);
  if (defined($error_msg)) {
    return TEST_FAILURE;
  }

  return TEST_SUCCESS;
}

# -----[ cbgp_valid_net_node_duplicate ]-----------------------------
# Test ability to detect creation of duplicated nodes.
#
# Scenario:
#   * Create a node 1.0.0.1
#   * Try to add node 1.0.0.1
#   * Check that an error is returned (and check error message)
# -------------------------------------------------------------------
sub cbgp_valid_net_node_duplicate($)
{
  my ($cbgp)= @_;
  my $ERROR_MSG= "node already exists";

  cbgp_send($cbgp, "net add node 1.0.0.1");
  cbgp_send($cbgp, "net add node 1.0.0.1");

  my $error_msg= cbgp_check_error($cbgp);
  if (!defined($error_msg) ||
      !($error_msg =~ m/$ERROR_MSG/)) {
    return TEST_FAILURE;
  }
  return TEST_SUCCESS;
}

# -----[ cbgp_valid_net_link ]---------------------------------------
# Create a topology composed of 3 nodes and 2 links. Check that the
# links are correctly created. Check the links attributes.
# -------------------------------------------------------------------
sub cbgp_valid_net_link($$)
{
    my ($cbgp, $topo)= @_;
    die if $cbgp->send("net add node 1.0.0.1\n");
    die if $cbgp->send("net add node 1.0.0.2\n");
    die if $cbgp->send("net add node 1.0.0.3\n");
    die if $cbgp->send("net add link 1.0.0.1 1.0.0.2 123\n");
    die if $cbgp->send("net add link 1.0.0.2 1.0.0.3 321\n");
    cbgp_check_link($cbgp, '1.0.0.1', '1.0.0.2',
		    -type=>'ROUTER',
		    -weight=>123) or return TEST_FAILURE;
    cbgp_check_link($cbgp, '1.0.0.2', '1.0.0.1',
		    -type=>'ROUTER',
		    -weight=>123) or return TEST_FAILURE;
    cbgp_check_link($cbgp, '1.0.0.2', '1.0.0.3',
		    -type=>'ROUTER',
		    -weight=>321) or return TEST_FAILURE;
    cbgp_check_link($cbgp, '1.0.0.3', '1.0.0.2',
		    -type=>'ROUTER',
		    -weight=>321) or return TEST_FAILURE;
    return TEST_SUCCESS;
}

# -----[ cbgp_valid_net_link_loop ]----------------------------------
# Test that C-BGP will return an error message if one tries to create
# a link with equal source and destination.
#
# Scenario:
#   * Create a node 1.0.0.1
#   * Try to create link 1.0.0.1 <--> 1.0.0.1
#   * Check that an error is returned (and check error message)
# -------------------------------------------------------------------
sub cbgp_valid_net_link_loop($)
{
  my ($cbgp)= @_;
  my $ERROR_MSG= "link endpoints are equal";

  cbgp_send($cbgp, "net add node 1.0.0.1");
  cbgp_send($cbgp, "net add link 1.0.0.1 1.0.0.1 10");

  my $error_msg= cbgp_check_error($cbgp);
  if (!defined($error_msg) ||
     !($error_msg =~ m/$ERROR_MSG/)) {
    return TEST_FAILURE;
  }
  return TEST_SUCCESS;
}

# -----[ cbgp_valid_net_link_duplicate ]-----------------------------
# Test that C-BGP will return an error message if one tries to create
# a link with equal source and destination.
#
# Scenario:
#   * Create a node 1.0.0.1
#   * Try to create link 1.0.0.1 <--> 1.0.0.1
#   * Check that an error is returned (and check error message)
# -------------------------------------------------------------------
sub cbgp_valid_net_link_duplicate($)
{
  my ($cbgp)= @_;
  my $ERROR_MSG= "link already exists";

  cbgp_send($cbgp, "net add node 1.0.0.1");
  cbgp_send($cbgp, "net add node 1.0.0.2");
  cbgp_send($cbgp, "net add link 1.0.0.1 1.0.0.2 10");
  cbgp_send($cbgp, "net add link 1.0.0.1 1.0.0.2 10");

  my $error_msg= cbgp_check_error($cbgp);
  if (!defined($error_msg) ||
     !($error_msg =~ m/$ERROR_MSG/)) {
    return TEST_FAILURE;
  }
  return TEST_SUCCESS;
}

# -----[ cbgp_valid_net_subnet ]-------------------------------------
# Create a topology composed of 2 nodes and 2 subnets (1 transit and
# one stub). Check that the links are correcly setup. Check the link
# attributes.
#
# Setup:
#   - R1 (1.0.0.1)
#   - R2 (1.0.0.2)
#
# Topology:
#
#   R1 ----(192.168.0/24)---- R2 ----(192.168.1/24)
#      .1                  .2    .2
#
# Scenario:
#   * Check link attributes
# -------------------------------------------------------------------
sub cbgp_valid_net_subnet($$)
{
    my ($cbgp, $topo)= @_;
    die if $cbgp->send("net add node 1.0.0.1\n");
    die if $cbgp->send("net add node 1.0.0.2\n");
    die if $cbgp->send("net add subnet 192.168.0/24 transit\n");
    die if $cbgp->send("net add subnet 192.168.1/24 stub\n");
    die if $cbgp->send("net add link 1.0.0.1 192.168.0.1/24 123\n");
    die if $cbgp->send("net add link 1.0.0.2 192.168.0.2/24 321\n");
    die if $cbgp->send("net add link 1.0.0.2 192.168.1.2/24 456\n");
    cbgp_check_link($cbgp, '1.0.0.1', '192.168.0.0/24',
		    -type=>'TRANSIT',
		    -weight=>123) or return TEST_FAILURE;
    cbgp_check_link($cbgp, '1.0.0.2', '192.168.0.0/24',
		    -type=>'TRANSIT',
		    -weight=>321) or return TEST_FAILURE;
    cbgp_check_link($cbgp, '1.0.0.2', '192.168.1.0/24',
		    -type=>'STUB',
		    -weight=>456) or return TEST_FAILURE;
    return TEST_SUCCESS;
}

# -----[ cbgp_valid_net_create ]-------------------------------------
# Create the topology given as parameter in C-BGP, then checks that
# the topology is correctly setup.
# -------------------------------------------------------------------
sub cbgp_valid_net_create($$)
{
    my ($cbgp, $topo)= @_;
    cbgp_topo($cbgp, $topo);
    return cbgp_topo_check_links($cbgp, $topo);
}

# -----[ cbgp_valid_net_igp ]----------------------------------------
# Create the topology given as parameter in C-BGP and compute the
# shortest paths. Finally checks that each node can reach the other
# ones.
# -------------------------------------------------------------------
sub cbgp_valid_net_igp($$)
{
    my ($cbgp, $topo)= @_;
    cbgp_topo($cbgp, $topo, 1);
    cbgp_topo_igp_compute($cbgp, $topo, 1);
    return cbgp_topo_check_igp_reachability($cbgp, $topo);
}

# -----[ cbgp_valid_net_igp_unknown_domain ]-------------------------
# Check that adding a node to an unexisting domain fails.
#
# Scenario:
#   * Create a node 0.1.0.0
#   * Try to add the node to a domain 1 (not created)
#   * Check the error message
# -------------------------------------------------------------------
sub cbgp_valid_net_igp_unknown_domain($$)
{
  my ($cbgp, $topo)= @_;
  my $ERROR_MSG= "unknown domain";

  cbgp_send($cbgp, "net add node 0.1.0.0");
  cbgp_send($cbgp, "net node 0.1.0.0 domain 1");

  my $error_msg= cbgp_check_error($cbgp);
  if (!defined($error_msg) ||
     !($error_msg =~ m/$ERROR_MSG/)) {
    return TEST_FAILURE;
  }
    return TEST_SUCCESS;
}

# -----[ cbgp_valid_net_ntf_load ]-----------------------------------
# Load a topology from an NTF file into C-BGP (using C-BGP 's "net
# ntf load" command). Check that the links are correctly setup.
#
# Resources:
#   [valid-record-route.ntf]
# -------------------------------------------------------------------
sub cbgp_valid_net_ntf_load($$)
{
    my ($cbgp, $ntf_file)= @_;
    my $topo= topo_from_ntf($ntf_file);
    die if $cbgp->send("net ntf load \"$ntf_file\"\n");
    return cbgp_topo_check_links($cbgp, $topo);
}

# -----[ cbgp_valid_net_record_route ]-------------------------------
# Create the given topology in C-BGP. Check that record-route
# fails. Then, compute the IGP routes. Check that record-route are
# successfull.
# -------------------------------------------------------------------
sub cbgp_valid_net_record_route($$)
{
    my ($cbgp, $topo)= @_;
    cbgp_topo($cbgp, $topo, 1);
    if (!cbgp_topo_check_record_route($cbgp, $topo, "UNREACH")) {
	return TEST_FAILURE;
    }
    cbgp_topo_igp_compute($cbgp, $topo, 1);
    if (!cbgp_topo_check_record_route($cbgp, $topo, "SUCCESS")) {
	return TEST_FAILURE;
    }
    return TEST_SUCCESS;
}

# -----[ cbgp_valid_net_static_routes ]------------------------------
# Check ability to setup static routes.
#
# Setup:
#
# Scenario:
#
# -------------------------------------------------------------------
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
# Check ability to forward along route with longest-matching prefix.
#
# Setup:
#   - R1 (1.0.0.1)
#   - R2 (1.0.0.2)
#   - R3 (1.0.0.3)
#
# Topology:
#
#   R1 ----- R2 ----- R3
#
# Scenario:
#   * Add route towards 1/8 with next-hop R1 in R2
#     Add route towards 1.1/16 with next-hop R3 in R2
#   * Perform a record-route from R2 to 1.0.0.0,
#     checks that first hop is R1
#   * Perform a record-route from R2 to 1.1.0.0,
#     checks that first hop is R3
# -------------------------------------------------------------------
sub cbgp_valid_net_longest_matching($)
{
    my ($cbgp)= @_;
    my $topo= topo_3nodes_line();
    cbgp_topo($cbgp, $topo);
    die if $cbgp->send("net node 1.0.0.2 route add 1/8 * 1.0.0.1 10\n");
    die if $cbgp->send("net node 1.0.0.2 route add 1.1/16 * 1.0.0.3 10\n");
    # Test longest-matching in show-rt
    my $rt;
    $rt= cbgp_show_rt($cbgp, "1.0.0.2", "1.0.0.0");
    if (!exists($rt->{"1.0.0.0/8"}) ||
	($rt->{"1.0.0.0/8"}->[CBGP_RT_IFACE] ne "1.0.0.1")) {
	return TEST_FAILURE;
    }
    $rt= cbgp_show_rt($cbgp, "1.0.0.2", "1.1.0.0");
    if (!exists($rt->{"1.1.0.0/16"}) ||
	($rt->{"1.1.0.0/16"}->[CBGP_RT_IFACE] ne "1.0.0.3")) {
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
# -------------------------------------------------------------------
sub cbgp_valid_net_protocol_priority($)
{
    my ($cbgp)= @_;
    my $topo= topo_3nodes_line();
    cbgp_topo($cbgp, $topo, 1);
    cbgp_topo_igp_compute($cbgp, $topo, 1);
    die if $cbgp->send("net node 1.0.0.2 route add 1.0.0.1/32 * 1.0.0.3 10\n");
    # Test protocol-priority in show-rt
    my $rt;
    $rt= cbgp_show_rt($cbgp, "1.0.0.2", "1.0.0.1");
    if (!exists($rt->{"1.0.0.1/32"}) ||
	($rt->{"1.0.0.1/32"}->[CBGP_RT_PROTO] ne "STATIC") ||
	($rt->{"1.0.0.1/32"}->[CBGP_RT_IFACE] ne "1.0.0.3")) {
	print "1.0.0.2 --> 1.0.0.1\n";
	return TEST_FAILURE;
    }
    $rt= cbgp_show_rt($cbgp, "1.0.0.2", "1.0.0.3");
    if (!exists($rt->{"1.0.0.3/32"}) ||
	($rt->{"1.0.0.3/32"}->[CBGP_RT_PROTO] ne "IGP") ||
	($rt->{"1.0.0.3/32"}->[CBGP_RT_IFACE] ne "1.0.0.3")) {
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

# -----[ cbgp_valid_bgp_options_showmode_cisco ]---------------------
# Test ability to show routes in CISCO "show ip bgp" format.
#
# Setup:
#   - R1 (123.0.0.234, AS255)
#
# Scenario:
#   * R1 has local network prefix "192.168.0/24"
# -------------------------------------------------------------------
sub cbgp_valid_bgp_options_showmode_cisco($)
{
  my ($cbgp)= @_;

  die if $cbgp->send("net add node 123.0.0.234\n");
  die if $cbgp->send("bgp add router 255 123.0.0.234\n");
  die if $cbgp->send("bgp router 123.0.0.234 add network 192.168.0/24\n");
  die if $cbgp->send("bgp options show-mode cisco\n");
  die if $cbgp->send("bgp router 123.0.0.234 show rib *\n");
  die if $cbgp->send("print \"done\\n\"\n");

  while ((my $result= $cbgp->expect(1)) ne "done") {
    (!bgp_route_parse_cisco($result)) and
      return TEST_FAILURE;
  }

  return TEST_SUCCESS;
}

# -----[ cbgp_valid_bgp_options_showmode_mrt ]-----------------------
# Test ability to show routes in MRT format.
#
# Setup:
#   - R1 (123.0.0.234, AS255)
#
# Scenario:
#   * R1 has local network prefix "192.168.0/24"
# -------------------------------------------------------------------
sub cbgp_valid_bgp_options_showmode_mrt($)
{
  my ($cbgp)= @_;

  die if $cbgp->send("net add node 123.0.0.234\n");
  die if $cbgp->send("bgp add router 255 123.0.0.234\n");
  die if $cbgp->send("bgp router 123.0.0.234 add network 192.168.0/24\n");
  die if $cbgp->send("bgp options show-mode mrt\n");
  die if $cbgp->send("bgp router 123.0.0.234 show rib *\n");
  die if $cbgp->send("print \"done\\n\"\n");

  while ((my $result= $cbgp->expect(1)) ne "done") {
    (!bgp_route_parse_mrt($result)) and
      return TEST_FAILURE;
  }

  return TEST_SUCCESS;
}

# -----[ cbgp_valid_bgp_options_showmode_custom ]--------------------
# Test ability to show routes in custom format.
#
# Setup:
#   - R1 (123.0.0.234, AS255)
#
# Scenario:
#   * R1 has local network prefix "192.168.0/24"
# -------------------------------------------------------------------
sub cbgp_valid_bgp_options_showmode_custom($)
{
  my ($cbgp)= @_;

  die if $cbgp->send("net add node 123.0.0.234\n");
  die if $cbgp->send("bgp add router 255 123.0.0.234\n");
  die if $cbgp->send("bgp router 123.0.0.234 add network 192.168.0/24\n");
  die if $cbgp->send("bgp options show-mode custom \"ROUTE:%i;%a;%p;%P;%C;%O;%n\"\n");
  die if $cbgp->send("bgp router 123.0.0.234 show rib *\n");
  die if $cbgp->send("print \"done\\n\"\n");

  while ((my $result= $cbgp->expect(1)) ne "done") {
    (!($result =~ m/^ROUTE\:([0-9.]+|LOCAL)\;([0-9]+|LOCAL)\;([0-9.]+\/[0-9]+)\;([0-9 ]+|null)\;\;\;([0-9.]+)$/)) and
      return TEST_FAILURE;
    (($1 ne "LOCAL") || ($2 ne "LOCAL") || ($3 ne "192.168.0.0/24") ||
     ($4 ne "null") || ($5 ne "123.0.0.234")) and
       return TEST_FAILURE;
  }
  return TEST_SUCCESS;
}

# -----[ cbgp_valid_bgp_topology_load ]------------------------------
# Test ability to load a file in "Subramanian" format. See paper by
# Subramanian et al (Berkeley) at INFOCOM'02 about "Characterizing the
# Internet hierarchy".
#
# Setup:
#   - AS1 <--> AS2
#   - AS1 ---> AS3
#   - AS1 ---> AS4
#   - AS2 ---> AS5
#   - AS3 ---> AS6
#   - AS4 ---> AS6
#   - AS4 ---> AS7
#   - AS5 ---> AS7
#
# Scenario:
#   * One router per AS must have been created with an IP address
#     that corresponds to "ASx.ASy.0.0".
#   * Links must have been created where a business relationship
#     exists.
#
# Resources:
#   [valid-bgp-topology.subramanian]
# -------------------------------------------------------------------
sub cbgp_valid_bgp_topology_load($)
{
  my ($cbgp)= @_;

  my $topo_file= $resources_path."valid-bgp-topology.subramanian";
  my $topo= topo_from_subramanian($topo_file);

  cbgp_send($cbgp, "bgp topology load \"$topo_file\"\n");
  (!cbgp_topo_check_links($cbgp, $topo)) and
    return TEST_FAILURE;

  return TEST_SUCCESS;
}

# -----[ cbgp_valid_bgp_topology_policies ]--------------------------
# Test ability to load AS-level topology and to define route filters
# based on the given business relationships.
#
# Setup:
#   see 'valid bgp topology load'
#
# Scenario:
#
# Resources:
#   [valid-bgp-topology.subramanian]
# -------------------------------------------------------------------
sub cbgp_valid_bgp_topology_policies($)
{
  my ($cbgp)= @_;

  my $topo_file= $resources_path."valid-bgp-topology.subramanian";
  cbgp_send($cbgp, "bgp topology load \"$topo_file\"\n");
  cbgp_send($cbgp, "bgp topology policies\n");

  return TEST_SUCCESS;
}

# -----[ cbgp_valid_bgp_topology_run ]-------------------------------
# Setup:
#   see 'valid bgp topology load'
#
# Scenario:
#
# Resources:
#   [valid-bgp-topology.subramanian]
# -------------------------------------------------------------------
sub cbgp_valid_bgp_topology_run($)
{
  my ($cbgp)= @_;

  my $topo_file= $resources_path."valid-bgp-topology.subramanian";
  my $topo= topo_from_subramanian($topo_file);

  cbgp_send($cbgp, "bgp topology load \"$topo_file\"\n");
  die if $cbgp->send("bgp topology policies\n");
  die if $cbgp->send("bgp topology run\n");
  if (!cbgp_topo_check_ebgp_sessions($cbgp, $topo, "OPENWAIT")) {
    return TEST_FAILURE;
  }
  die if $cbgp->send("sim run\n");
  if (!cbgp_topo_check_ebgp_sessions($cbgp, $topo, "ESTABLISHED")) {
    return TEST_FAILURE;
  }

  return TEST_SUCCESS;
}

# -----[ cbgp_valid_bgp_session_ibgp ]-------------------------------
# Test iBGP redistribution mechanisms.
# - establishment
# - propagation
# - no iBGP reflection
# - no AS-path update
# - no next-hop update
#
# Setup:
#   - R1 (1.0.0.1, AS1)
#   - R2 (1.0.0.2, AS1)
#   - R3 (1.0.0.3, AS1
#
# Topology:
#   R1 ----- R2 ----- R3
#
# Scenario:
#   * Originate 255.255/16 in R1
#   * Check that R2 receives the route (with next-hop=R1 and empty
#     AS-Path)
#   * Check that R3 does not receive the route (no iBGP reflection)
# -------------------------------------------------------------------
sub cbgp_valid_bgp_session_ibgp($)
{
  my ($cbgp)= @_;
  my $topo= topo_3nodes_line();
  cbgp_topo($cbgp, $topo, 1);
  cbgp_topo_igp_compute($cbgp, $topo, 1);
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
      !aspath_equals($rib->{"255.255.0.0/16"}->[CBGP_RIB_PATH], undef)) {
    $tests->debug("=> ERROR route not propagated or wrong attribute");
    return TEST_FAILURE;
  }
  # Check that 1.0.0.3 has not received the route (1.0.0.2 did not
  # send it since it was learned through an iBGP session).
  $rib= cbgp_show_rib($cbgp, "1.0.0.3");
  if (exists($rib->{"255.255.0.0/16"})) {
    $tests->debug("=> ERROR route reflected over iBGP session");
    return TEST_FAILURE;
  }
  return TEST_SUCCESS;
}

# -----[ cbgp_valid_bgp_session_ebgp ]-------------------------------
# Test eBGP redistribution mechanisms.
# - establishment
# - propagation
# - update AS-path on eBGP session
# - update next-hop on eBGP session
# - [TODO] local-pref is reset in routes received through eBGP
#
# Setup:
#   - R1 (1.0.0.1, AS1)
#   - R2 (1.0.0.2, AS2)
#   - R3 (1.0.0.3, AS3)
#
# Topology:
#   R1 ---<eBGP>--- R2 ---<iBGP>--- R3
#
# Scenario:
#   * Originate 255.255/16 in R1, 255.254/16 in R3
#   * Check that R1 has 255.254/16 with next-hop=R2 and AS-Path="2"
#   * Check that R2 has 255.254/16 with next-hop=R3 and empty AS-Path
#   * Check that R2 has 255.255/16 with next-hop=R1 and AS-Path="1"
#   * Check that R3 has 255.255/16 with next-hop=R1 and AS-Path="1"
# -------------------------------------------------------------------
sub cbgp_valid_bgp_session_ebgp($)
{
  my ($cbgp)= @_;
  
  my $topo= topo_3nodes_line();
  cbgp_topo($cbgp, $topo, 1);
  cbgp_topo_igp_compute($cbgp, $topo, 1);
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

  # Check that AS-path contains remote ASN and that the next-hop is
  # the last router in remote AS
  my $rib= cbgp_show_rib($cbgp, "1.0.0.1");
  if (!exists($rib->{"255.254.0.0/16"}) ||
      !aspath_equals($rib->{"255.254.0.0/16"}->[CBGP_RIB_PATH],[2]) ||
      ($rib->{"255.254.0.0/16"}->[CBGP_RIB_NEXTHOP] ne "1.0.0.2")) {
    $tests->debug("=> ERROR no route or wrong attribute in R1");
    return TEST_FAILURE;
  }

  # Check that AS-path contains remote ASN
  $rib= cbgp_show_rib($cbgp, "1.0.0.2");
  if (!exists($rib->{"255.254.0.0/16"}) ||
      !aspath_equals($rib->{"255.254.0.0/16"}->[CBGP_RIB_PATH], undef) ||
      ($rib->{"255.254.0.0/16"}->[CBGP_RIB_NEXTHOP] ne "1.0.0.3")) {
    $tests->debug("=> ERROR no route for 255.254/16 or wrong attribute in R2");
    $tests->debug("   next-hop: ".$rib->{"255.254.0.0/16"}->[CBGP_RIB_NEXTHOP]);
    $tests->debug("   as-path : ".$rib->{"255.254.0.0/16"}->[CBGP_RIB_PATH]);
    return TEST_FAILURE;
  }
  if (!exists($rib->{"255.255.0.0/16"}) ||
      !aspath_equals($rib->{"255.255.0.0/16"}->[CBGP_RIB_PATH], [1]) ||
      ($rib->{"255.255.0.0/16"}->[CBGP_RIB_NEXTHOP] ne "1.0.0.1")) {
    $tests->debug("=> ERROR no route for 255.255/26 or wrong attribute in R2");
    return TEST_FAILURE;
  }

  # Check that AS-path contains remote ASN
  $rib= cbgp_show_rib($cbgp, "1.0.0.3");
  if (!exists($rib->{"255.255.0.0/16"}) ||
      !aspath_equals($rib->{"255.255.0.0/16"}->[CBGP_RIB_PATH], [1]) ||
      ($rib->{"255.255.0.0/16"}->[CBGP_RIB_NEXTHOP] ne "1.0.0.1")) {
    $tests->debug("=> ERROR no route or wrong attribute in R3");
    return TEST_FAILURE;
  }

  return TEST_SUCCESS;
}

# -----[ cbgp_valid_bgp_session_sm ]---------------------------------
sub cbgp_valid_bgp_session_sm()
{
    return TEST_NOT_TESTED;
}

# -----[ cbgp_valid_bgp_domain_fullmesh ]----------------------------
# Test ability to create a full-mesh of iBGP sessions between all the
# routers of a domain.
#
# Topology:
#
#   R1 ---- R2
#     \    /
#      \  /
#       R3
#
# Scenario:
#   * Create a topology composed of nodes R1, R2 and R3
#   * Create BGP routers with R1, R2 and R3 in domain 1
#   * Run 'bgp domain 1 full-mesh'
#   * Check setup of sessions
#   * Check establishment of sessions
# -------------------------------------------------------------------
sub cbgp_valid_bgp_domain_fullmesh($$)
{
    my ($cbgp, $topo)= @_;
    cbgp_topo($cbgp, $topo, 1);
    cbgp_topo_igp_compute($cbgp, $topo, 1);
    cbgp_topo_bgp_routers($cbgp, $topo, 1);
    die if $cbgp->send("bgp domain 1 full-mesh\n");
    if (!cbgp_topo_check_ibgp_sessions($cbgp, $topo, "OPENWAIT")) {
	return TEST_FAILURE;
    }
    die if $cbgp->send("sim run\n");
    if (!cbgp_topo_check_ibgp_sessions($cbgp, $topo, "ESTABLISHED")) {
	return TEST_FAILURE;
    }
    if (!cbgp_topo_check_bgp_networks($cbgp, $topo)) {
	return TEST_FAILURE;
    }
    return TEST_SUCCESS;
}

# -----[ cbgp_valid_bgp_peering_up_down ]----------------------------
# Test ability to shutdown and re-open BGP sessions from the CLI.
#
# Setup:
#   - R1 (1.0.0.1)
#   - R2 (1.0.0.2)
#
# Topology:
#
#   R1 ----- R2
#
# Scenario:
#   * Originate prefix 255.255/16 in R1
#   * Originate prefix 255.254/16 in R2
#   * Check routes in R1 and R2
#   * Shutdown session between R1 and R2
#   * Check routes in R1 and R2
#   * Re-open session between R1 and R2
#   * Check routes in R1 and R2
# -------------------------------------------------------------------
sub cbgp_valid_bgp_peering_up_down($)
{
    my ($cbgp)= @_;
    my $topo= topo_2nodes();
    cbgp_topo($cbgp, $topo, 1);
    cbgp_topo_igp_compute($cbgp, $topo, 1);
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

# -----[ cbgp_valid_bgp_router_up_down ]-----------------------------
# Test ability to start and stop a router. Starting (stopping) a
# router should be equivalent to establishing (shutting down) all its
# BGP sessions.
#
# Setup:
#
# Scenario:
# 
# -------------------------------------------------------------------
sub cbgp_valid_bgp_router_up_down($)
{
  my ($cbgp)= @_;

  return TEST_NOT_TESTED;
}

# -----[ cbgp_valid_bgp_peering_nexthopself ]-----------------------
# Test ability for a BGP router to set the BGP next-hop of a route to
# its own address before propagating it to a given neighbor. This test
# uses the 'next-hop-self' peer option.
#
# Setup:
#   - R1 (1.0.0.1, AS1, IGP domain 1)
#   - R2 (1.0.0.2, AS1, IGP domain 1)
#   - R3 (2.0.0.1, AS2, IGP domain 2)
#   - static routes between R1 and R2
#
# Topology:
#
#   AS2 \    / AS1
#        |  |
#   R3 --]--[-- R1 ----- R2
#        |  | nhs
#       /    \
#
# Scenario:
#   * R3 originates prefix 2/8
#   * R1 propagates 2/8 to R2
#   * Check that R2 has received the route towards 2/8 with
#     next-hop=R1
# -------------------------------------------------------------------
sub cbgp_valid_bgp_peering_nexthopself($)
{
    my ($cbgp)= @_;
    my $topo= topo_2nodes();
    cbgp_topo($cbgp, $topo, 1);
    cbgp_topo_igp_compute($cbgp, $topo, 1);
    cbgp_topo_bgp_routers($cbgp, $topo, 1);
    die if $cbgp->send("bgp domain 1 full-mesh\n");
    die if $cbgp->send("net add node 2.0.0.1\n");
    die if $cbgp->send("net add link 1.0.0.1 2.0.0.1 0\n");
    die if $cbgp->send("net node 1.0.0.1 route add 2.0.0.1/32 * 2.0.0.1 0\n");
    die if $cbgp->send("net node 2.0.0.1 route add 1.0.0.1/32 * 1.0.0.1 0\n");
    die if $cbgp->send("bgp add router 2 2.0.0.1\n");
    die if $cbgp->send("bgp router 2.0.0.1 add network 2.0.0.0/8\n");
    cbgp_peering($cbgp, "2.0.0.1", "1.0.0.1", 1);
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
# Test ability to define virtual peers, i.e. BGP neighbors that do not
# really exist. These virtual peers are usually defined for the
# purpose of injecting routes into the model or to collect routes
# generated by the model.
#
# Setup:
#   - R1 (1.0.0.1, AS1)
#   - R2 (1.0.0.2, AS1)
#   - R3 (2.0.0.1, AS2), virtual
#
# Topology:
#
#   R2 ----- R1 ----- (R3)
#
# Scenario:
#   * Create a virtual peer R3 in R1
#   * Check that the session can be "established"
# -------------------------------------------------------------------
sub cbgp_valid_bgp_virtual_peer($)
{
    my ($cbgp)= @_;
    my $topo= topo_2nodes();
    cbgp_topo($cbgp, $topo, 1);
    cbgp_topo_igp_compute($cbgp, $topo, 1);
    cbgp_topo_bgp_routers($cbgp, $topo, 1);
    die if $cbgp->send("bgp domain 1 full-mesh\n");
    die if $cbgp->send("net add node 2.0.0.1\n");
    die if $cbgp->send("net add link 1.0.0.1 2.0.0.1 0\n");
    die if $cbgp->send("net node 1.0.0.1 route add 2.0.0.1/32 * 2.0.0.1 0\n");
    cbgp_peering($cbgp, "1.0.0.1", "2.0.0.1", 2, "next-hop-self", "virtual");
    die if $cbgp->send("sim run\n");    
    if (!cbgp_check_peering($cbgp, "1.0.0.1", "2.0.0.1", "ESTABLISHED")) {
	return TEST_FAILURE;
    }
    return TEST_SUCCESS;
}

# -----[ cbgp_valid_bgp_recv_mrt ]-----------------------------------
# Test ability to inject BGP routes in the model through virtual
# peers. BGP routes are specified in the MRT format.
#
# Setup:
#   - R1 (1.0.0.1, AS1)
#   - R2 (1.0.0.2, AS1)
#   - R3 (2.0.0.1, AS2), virtual
#
# Topology:
#
#   R2 ----- R1 ----- (R3)
#
# Scenario:
#   * Inject route towards 255.255/16 from R3 to R1
#   * Check that R1 has received the route with correct attributes
#   * Send a withdraw for the same prefix from R3 to R1
#   * Check that R1 has not the route anymore
# -------------------------------------------------------------------
sub cbgp_valid_bgp_recv_mrt($)
{
    my ($cbgp)= @_;
    my $topo= topo_2nodes();
    cbgp_topo($cbgp, $topo, 1);
    cbgp_topo_igp_compute($cbgp, $topo, 1);
    cbgp_topo_bgp_routers($cbgp, $topo, 1);
    cbgp_send($cbgp, "bgp domain 1 full-mesh");
    cbgp_send($cbgp, "net add node 2.0.0.1");
    cbgp_send($cbgp, "net add link 1.0.0.1 2.0.0.1 0");
    cbgp_send($cbgp, "net node 1.0.0.1 route add 2.0.0.1/32 * 2.0.0.1 0");
    cbgp_peering($cbgp, "1.0.0.1", "2.0.0.1", 2, "next-hop-self", "virtual");
    cbgp_send($cbgp, "sim run");
    cbgp_recv_update($cbgp, "1.0.0.1", 1, "2.0.0.1",
		     "255.255/16|2|IGP|2.0.0.1|12|34");
    cbgp_send($cbgp, "sim run");

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

# -----[ cbgp_valid_bgp_aspath_as_set ]------------------------------
# Send an AS-Path with an AS_SET segment and checks that it is
# correctly handled.
# -------------------------------------------------------------------
sub cbgp_valid_bgp_aspath_as_set($)
{
#    my ($cbgp)= @_;
#    my ($rib, $route);
#    my $topo= topo_2nodes();
#    cbgp_topo($cbgp, $topo, 1);
#    cbgp_topo_igp_compute($cbgp, $topo, 1);
#    cbgp_topo_bgp_routers($cbgp, $topo, 1);
#    die if $cbgp->send("bgp domain 1 full-mesh\n");
#    die if $cbgp->send("net add node 2.0.0.1\n");
#    die if $cbgp->send("net add link 1.0.0.1 2.0.0.1 0\n");
#    die if $cbgp->send("net node 1.0.0.1 route add 2.0.0.1/32 * 2.0.0.1 0\n");
#    cbgp_peering($cbgp, "1.0.0.1", "2.0.0.1", 2, "next-hop-self", "virtual");
#    die if $cbgp->send("sim run\n");
#    die if $cbgp->send("bgp router 1.0.0.1 peer 2.0.0.1 recv ".
#		       "\"BGP4|0|A|1.0.0.1|1|255.255.0.0/16|".
#		       "2 3 [4 5 6]|IGP|2.0.0.1|12|34|\"\n");
#    # Check that route is in the Loc-RIB of 1.0.0.1
#    $rib= cbgp_show_rib($cbgp, "1.0.0.1");
#    (!exists($rib->{"255.255.0.0/16"})) and
#	return TEST_FAILURE;
#    $route= $rib->{"255.255.0.0/16"};
#    print "AS-Path: ".$route->[CBGP_RIB_PATH]."\n";
#    ($route->[CBGP_RIB_PATH] =~ m/^2 3 \[([456 ]+)\]$/) or
#	return TEST_FAILURE;
#    
  return TEST_NOT_TESTED;
}

# -----[ cbgp_valid_bgp_soft_restart ]-------------------------------
# Test ability to maintain the content of the Adj-RIB-in of a virtual
# peer when the session is down (or broken).
# -------------------------------------------------------------------
sub cbgp_valid_bgp_soft_restart($)
{
    my ($cbgp)= @_;
    my $topo= topo_2nodes();
    cbgp_topo($cbgp, $topo, 1);
    cbgp_topo_igp_compute($cbgp, $topo, 1);
    cbgp_topo_bgp_routers($cbgp, $topo, 1);
    die if $cbgp->send("bgp domain 1 full-mesh\n");
    die if $cbgp->send("net add node 2.0.0.1\n");
    die if $cbgp->send("net add link 1.0.0.1 2.0.0.1 0\n");
    die if $cbgp->send("net node 1.0.0.1 route add 2.0.0.1/32 * 2.0.0.1 0\n");
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
#
# Setup:
#   - R1 (1.0.0.1, AS1)
#   - R2 (2.0.0.1, AS2)
#
# Topology:
#
#   R2 ----- R1
#
# Scenario:
#   * R2 advertises a route towards 255/8 to R1, with an AS-Path that
#     already contains AS1
#   * Check that R1 has filtered the route
# -------------------------------------------------------------------
sub cbgp_valid_bgp_aspath_loop($)
{
    my ($cbgp)= @_;
    cbgp_topo_dp3($cbgp, ["2.0.0.1", 2, 0]);
    cbgp_recv_update($cbgp, "1.0.0.1", 1, "2.0.0.1",
		     "255/8|2 1|IGP|2.0.0.1|0|0");
    die if $cbgp->send("sim run\n");
    my $rib;
    $rib= cbgp_show_rib($cbgp, "1.0.0.1");
    if (exists($rib->{"255.0.0.0/8"})) {
	return TEST_FAILURE;
    }
    return TEST_SUCCESS;
    die;
}

# -----[ cbgp_valid_bgp_ssld ]---------------------------------------
# Test sender-side loop detection.
#
# Setup:
#   - R1 (1.0.0.1, AS1)
#   - R2 (2.0.0.1, AS2)
#   - R3 (3.0.0.1, AS3)
#
# Topology:
#
#   R3 ----- R1 ----- R2
#
# Scenario:
#   * R3 announces to R1 a route towards 255/8 with an
#       AS-Path [AS3 AS2 AS1].
#   * Check that R1 do not advertise the route to R2.
# -------------------------------------------------------------------
sub cbgp_valid_bgp_ssld($)
{
    my ($cbgp)= @_;

    die if $cbgp->send("net add domain 1 igp\n");
    die if $cbgp->send("net add node 1.0.0.1\n");
    die if $cbgp->send("net add node 2.0.0.1\n");
    die if $cbgp->send("net add node 3.0.0.1\n");
    die if $cbgp->send("net add link 1.0.0.1 2.0.0.1 10\n");
    die if $cbgp->send("net add link 1.0.0.1 3.0.0.1 10\n");
    die if $cbgp->send("net node 1.0.0.1 domain 1\n");
    die if $cbgp->send("net node 2.0.0.1 domain 1\n");
    die if $cbgp->send("net node 3.0.0.1 domain 1\n");
    die if $cbgp->send("net domain 1 compute\n");
    die if $cbgp->send("bgp add router 1 1.0.0.1\n");
    die if $cbgp->send("bgp router 1.0.0.1\n");
    die if $cbgp->send("\tadd peer 3 3.0.0.1\n");
    die if $cbgp->send("\tpeer 3.0.0.1 virtual\n");
    die if $cbgp->send("\tpeer 3.0.0.1 up\n");
    die if $cbgp->send("\tadd peer 2 2.0.0.1\n");
    die if $cbgp->send("\tpeer 2.0.0.1 up\n");
    die if $cbgp->send("\texit\n");
    die if $cbgp->send("bgp add router 2 2.0.0.1\n");
    die if $cbgp->send("bgp router 2.0.0.1\n");
    die if $cbgp->send("\tadd peer 1 1.0.0.1\n");
    die if $cbgp->send("\tpeer 1.0.0.1 up\n");
    die if $cbgp->send("\texit\n");
    die if $cbgp->send("sim run\n");
    cbgp_recv_update($cbgp, "1.0.0.1", 1, "3.0.0.1",
		    "255/8|3 2 255|IGP|3.0.0.1|0|0");
    cbgp_recv_update($cbgp, "1.0.0.1", 1, "3.0.0.1",
		    "254/8|3 4 255|IGP|3.0.0.1|0|0");
    die if $cbgp->send("sim run\n");
    my $rib;
    $rib= cbgp_show_rib($cbgp, "2.0.0.1");
    if (!exists($rib->{"254.0.0.0/8"})) {
	return TEST_FAILURE;
    }
    if (exists($rib->{"255.0.0.0/8"})) {
	return TEST_FAILURE;
    }
    
    return TEST_SUCCESS;
}

# -----[ cbgp_valid_bgp_implicit_withdraw ]--------------------------
# Test the replacement of a previously advertised route by a new one,
# without explicit withdraw.
#
# Setup:
#   - R1 (1.0.0.1, AS1)
#   - R2 (1.0.0.2, AS1)
#   - R3 (2.0.0.1, AS2)
#   - R4 (2.0.0.2, AS2)
#
# Topology:
#
#   (R3) --*
#           \
#            R1 ----- R2
#           /
#   (R4) --*
#
# Scenario:
#   * R3 announces to R1 a route towards 255/8, with AS-Path=2 3 4
#   * R4 announces to R1 a route towards 255/8, with AS-Path=2 4
#   * Check that R1 has selected R4's route
#   * R3 announces to R1 a route towards 255/8, with AS-Path=2 4
#   * Check that R1 has selected R3's route
# -------------------------------------------------------------------
sub cbgp_valid_bgp_implicit_withdraw($)
{
    my ($cbgp)= @_;
    cbgp_topo_dp3($cbgp,
		  ["1.0.0.2", 1, 10],
		  ["2.0.0.1", 2, 0],
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
# Test ability of decision-process to select route with the highest
# value of the local-pref attribute.
#
# Setup:
#
# Scenario:
#
# -------------------------------------------------------------------
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
# Test ability of decision process to select route with the shortest
# AS-Path.
#
# Setup:
#
# Scenario:
#
# -------------------------------------------------------------------
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

# -----[ cbgp_valid_bgp_dp_origin ]----------------------------------
# Test ability to break ties based on the ORIGIN attribute.
#
# Setup:
#   - R1 (1.0.0.1, AS1)
#   - R2 (1.0.0.2, AS1) virtual peer of R1
#   - R3 (1.0.0.3, AS3) virtual peer of R2
#
# Topology:
#
#   R2 --*
#         \
#          *-- R1
#         /
#   R3 --*
#
# Scenario:
#   * Advertise 255/8 to R1 from R2 with ORIGIN=EGP and R3 with
#     ORIGIN=IGP
#   * Check that R1 has received two routes
#   * Check that best route is through R3 (ORIGIN=IGP)
# -------------------------------------------------------------------
sub cbgp_valid_bgp_dp_origin($)
{
  my ($cbgp)= @_;

  cbgp_topo_dp4($cbgp,
		["1.0.0.2", 1, 1, 1],
		["1.0.0.3", 1, 1, 1]);
  cbgp_recv_update($cbgp, "1.0.0.1", 1, "1.0.0.2",
		   "255/8|2|EGP|1.0.0.2|0|0");
  cbgp_recv_update($cbgp, "1.0.0.1", 1, "1.0.0.3",
		   "255/8|2|IGP|1.0.0.3|0|0");

  die if $cbgp->send("sim run\n");

  my $rib= cbgp_show_rib_mrt($cbgp, "1.0.0.1");
  if (!exists($rib->{"255.0.0.0/8"}) ||
      ($rib->{"255.0.0.0/8"}->[CBGP_RIB_NEXTHOP] ne "1.0.0.3")) {
    return TEST_FAILURE;
  }

  return TEST_SUCCESS;
}

# -----[ cbgp_valid_bgp_dp_ebgp_ibgp ]-------------------------------
# Test ability of the decision process to prefer routes received
# through eBGP sessions over routes received through iBGP sessions.
#
# Setup:
#
# Scenario:
#
# -------------------------------------------------------------------
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
# Test ability of the decision process to prefer routes with the
# highest MED value.
#
# Setup:
#   - R1 (1.0.0.1, AS1)
#   - R2 (2.0.0.1, AS2)
#   - R3 (2.0.0.2, AS2)
#   - R4 (3.0.0.1, AS3)
#   - R5 (3.0.0.2, AS3)
#
# Topology:
#
#   (R2)---*
#           \
#   (R3)---* \
#           \ \
#             R1
#           / /
#   (R4)---* /
#           /
#   (R5)---*
#
# Scenario:
#   * R2 announces 255.0/16 with MED=80
#     R3 announces 255.0/16 with MED=60
#     R4 announces 255.0/16 with MED=40
#     R5 announces 255.0/16 with MED=20
#   * Check that best route selected by R1 is the route announced
#     by R3 (2.0.0.2)
# -------------------------------------------------------------------
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

# -----[ cbgp_valid_bgp_rr_dp_cluster_id_list ]----------------------
# Test ability to break ties based on the cluster-id-list length.
#
# Setup:
#   - R1 (1.0.0.1, AS1), virtual peer of R2 and R3
#   - R2 (1.0.0.2, AS1)
#   - R3 (1.0.0.3, AS1)
#   - R4 (1.0.0.4, AS1) rr-client of R2
#   - R5 (1.0.0.5, AS1) rr-client of R3 and R4
#
# Topology:
#
#      *-- R2 ---- R4 --*
#     /                  \
#   R1                    R5
#     \                  /
#      *-- R3 ----------*
#
# Scenario:
#   * Advertise 255/8 from R1 to R2 with community 255:1
#   * Advertise 255/8 from R1 to R3 with community 255:2
#   * Check that R5 has received two routes towards 255/8
#   * Check that R5's best route has community 255:2 (selected based
#     on the smallest cluster-id-list)
# -------------------------------------------------------------------
sub cbgp_valid_bgp_rr_dp_cluster_id_list($)
{
    my ($cbgp)= @_;

    die if $cbgp->send("net add domain 1 igp\n");
    die if $cbgp->send("net add node 1.0.0.1\n");
    die if $cbgp->send("net node 1.0.0.1 domain 1\n");
    die if $cbgp->send("net add node 1.0.0.2\n");
    die if $cbgp->send("net node 1.0.0.2 domain 1\n");
    die if $cbgp->send("net add node 1.0.0.3\n");
    die if $cbgp->send("net node 1.0.0.3 domain 1\n");
    die if $cbgp->send("net add node 1.0.0.4\n");
    die if $cbgp->send("net node 1.0.0.4 domain 1\n");
    die if $cbgp->send("net add node 1.0.0.5\n");
    die if $cbgp->send("net node 1.0.0.5 domain 1\n");
    die if $cbgp->send("net add link 1.0.0.1 1.0.0.2 1\n");
    die if $cbgp->send("net add link 1.0.0.1 1.0.0.3 1\n");
    die if $cbgp->send("net add link 1.0.0.2 1.0.0.4 1\n");
    die if $cbgp->send("net add link 1.0.0.3 1.0.0.5 1\n");
    die if $cbgp->send("net add link 1.0.0.4 1.0.0.5 1\n");
    die if $cbgp->send("net domain 1 compute\n");

    die if $cbgp->send("bgp add router 1 1.0.0.2\n");
    die if $cbgp->send("bgp router 1.0.0.2\n");
    die if $cbgp->send("\tadd peer 1 1.0.0.1\n");
    die if $cbgp->send("\tpeer 1.0.0.1 virtual\n");
    die if $cbgp->send("\tpeer 1.0.0.1 up\n");
    die if $cbgp->send("\tadd peer 1 1.0.0.4\n");
    die if $cbgp->send("\tpeer 1.0.0.4 rr-client\n");
    die if $cbgp->send("\tpeer 1.0.0.4 up\n");
    die if $cbgp->send("\texit\n");

    die if $cbgp->send("bgp add router 1 1.0.0.3\n");
    die if $cbgp->send("bgp router 1.0.0.3\n");
    die if $cbgp->send("\tadd peer 1 1.0.0.1\n");
    die if $cbgp->send("\tpeer 1.0.0.1 virtual\n");
    die if $cbgp->send("\tpeer 1.0.0.1 up\n");
    die if $cbgp->send("\tadd peer 1 1.0.0.5\n");
    die if $cbgp->send("\tpeer 1.0.0.5 rr-client\n");
    die if $cbgp->send("\tpeer 1.0.0.5 up\n");
    die if $cbgp->send("\texit\n");

    die if $cbgp->send("bgp add router 1 1.0.0.4\n");
    die if $cbgp->send("bgp router 1.0.0.4\n");
    die if $cbgp->send("\tadd peer 1 1.0.0.2\n");
    die if $cbgp->send("\tpeer 1.0.0.2 up\n");
    die if $cbgp->send("\tadd peer 1 1.0.0.5\n");
    die if $cbgp->send("\tpeer 1.0.0.5 rr-client\n");
    die if $cbgp->send("\tpeer 1.0.0.5 up\n");
    die if $cbgp->send("\texit\n");

    die if $cbgp->send("bgp add router 1 1.0.0.5\n");
    die if $cbgp->send("bgp router 1.0.0.5\n");
    die if $cbgp->send("\tadd peer 1 1.0.0.3\n");
    die if $cbgp->send("\tpeer 1.0.0.3 up\n");
    die if $cbgp->send("\tadd peer 1 1.0.0.4\n");
    die if $cbgp->send("\tpeer 1.0.0.4 up\n");
    die if $cbgp->send("\texit\n");

    cbgp_recv_update($cbgp, "1.0.0.2", 1, "1.0.0.1",
		    "255/8|2|IGP|1.0.0.1|0|0|255:1");
    cbgp_recv_update($cbgp, "1.0.0.3", 1, "1.0.0.1",
		    "255/8|2|IGP|1.0.0.1|0|0|255:2");

    die if $cbgp->send("sim run\n");

    my $rib= cbgp_show_rib_custom($cbgp, "1.0.0.5");
    if (!exists($rib->{"255.0.0.0/8"}) ||
       !community_equals($rib->{"255.0.0.0/8"}->[CBGP_RIB_COMMUNITY],
			 ["255:2"])) {
      return TEST_FAILURE;
    }

    return TEST_SUCCESS;
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

# -----[ cbgp_valid_bgp_rr_dp_originator_id ]------------------------
# Check that a BGP router that receives two equal routes will break
# the ties based on their Originator-ID.
#
# Setup:
#   - R1 (1.0.0.1, AS1), virtual
#   - R2 (1.0.0.2, AS1), virtual
#   - R3 (1.0.0.3, AS1, RR)
#   - R4 (1.0.0.4, AS1, RR)
#   - R5 (1.0.0.5, AS1, RR)
#
# Topology:
#
#    R1 ----- R4
#               \
#                 R5
#               /
#    R2 ----- R3
#
# Scenario:
#   * Inject 255/8 to R4, from R1
#   * Inject 255/8 to R3, from R2
#   * Check that XXX
# -------------------------------------------------------------------
sub cbgp_valid_bgp_rr_dp_originator_id($)
{
  my ($cbgp)= @_;
  cbgp_send($cbgp, "net add domain 1 igp");
  cbgp_send($cbgp, "net add node 1.0.0.1");
  cbgp_send($cbgp, "net add node 1.0.0.2");
  cbgp_send($cbgp, "net add node 1.0.0.3");
  cbgp_send($cbgp, "net add node 1.0.0.4");
  cbgp_send($cbgp, "net add node 1.0.0.5");
  cbgp_send($cbgp, "net node 1.0.0.1 domain 1");
  cbgp_send($cbgp, "net node 1.0.0.2 domain 1");
  cbgp_send($cbgp, "net node 1.0.0.3 domain 1");
  cbgp_send($cbgp, "net node 1.0.0.4 domain 1");
  cbgp_send($cbgp, "net node 1.0.0.5 domain 1");
  cbgp_send($cbgp, "net add link 1.0.0.1 1.0.0.4 1");
  cbgp_send($cbgp, "net add link 1.0.0.2 1.0.0.3 1");
  cbgp_send($cbgp, "net add link 1.0.0.3 1.0.0.5 1");
  cbgp_send($cbgp, "net add link 1.0.0.4 1.0.0.5 1");
  cbgp_send($cbgp, "net domain 1 compute");
  cbgp_send($cbgp, "bgp add router 1 1.0.0.3");
  cbgp_send($cbgp, "bgp router 1.0.0.3");
  cbgp_send($cbgp, "\tadd peer 1 1.0.0.2");
  cbgp_send($cbgp, "\tpeer 1.0.0.2 virtual");
  cbgp_send($cbgp, "\tpeer 1.0.0.2 rr-client");
  cbgp_send($cbgp, "\tpeer 1.0.0.2 up");
  cbgp_send($cbgp, "\tadd peer 1 1.0.0.5");
  cbgp_send($cbgp, "\tpeer 1.0.0.5 up");
  cbgp_send($cbgp, "\texit");
  cbgp_send($cbgp, "bgp add router 1 1.0.0.4");
  cbgp_send($cbgp, "bgp router 1.0.0.4");
  cbgp_send($cbgp, "\tadd peer 1 1.0.0.1");
  cbgp_send($cbgp, "\tpeer 1.0.0.1 virtual");
  cbgp_send($cbgp, "\tpeer 1.0.0.1 rr-client");
  cbgp_send($cbgp, "\tpeer 1.0.0.1 up");
  cbgp_send($cbgp, "\tadd peer 1 1.0.0.5");
  cbgp_send($cbgp, "\tpeer 1.0.0.5 up");
  cbgp_send($cbgp, "\texit");
  cbgp_send($cbgp, "bgp add router 1 1.0.0.5");
  cbgp_send($cbgp, "bgp router 1.0.0.5");
  cbgp_send($cbgp, "\tadd peer 1 1.0.0.3");
  cbgp_send($cbgp, "\tpeer 1.0.0.3 up");
  cbgp_send($cbgp, "\tadd peer 1 1.0.0.4");
  cbgp_send($cbgp, "\tpeer 1.0.0.4 up");
  cbgp_send($cbgp, "\texit");

  cbgp_recv_update($cbgp, "1.0.0.4", 1, "1.0.0.1",
		   "255/8||IGP|1.0.0.1|0|0");
  cbgp_recv_update($cbgp, "1.0.0.3", 1, "1.0.0.2",
		   "255/8||IGP|1.0.0.2|0|0");
  cbgp_send($cbgp, "sim run");
  my $rib;
  my @routes;
  $rib= cbgp_show_rib_mrt($cbgp, "1.0.0.5", "255/8");
  if (!exists($rib->{"255.0.0.0/8"}) ||
     ($rib->{"255.0.0.0/8"}->[CBGP_RIB_ORIGINATOR] ne '1.0.0.1')) {
    return TEST_FAILURE;
  }

  return TEST_SUCCESS;
}

# -----[ cbgp_valid_bgp_dp_neighbor_address ]------------------------
# Test ability to break ties based on the neighbor's IP address.
# Note: this can only occur in the case were there are at least
# two BGP sessions between two routers.
#
# Setup:
#   - R1 (1.0.0.1, AS1)
#   - R2 (1.0.0.2, AS2)
#   - R3 (2.0.0.1, AS2) virtual peer of R1
#
# Topology:
#
#               *--(192.168.0.0/31)--*
#              /                      \
#   R3 ----- R1                        R2
#              \                      /
#               *--(192.168.0.2/31)--*
#
# Scenario:
#   * Advertise 255/8 from 2.0.0.1 to 1.0.0.1
#   * Check that 1.0.0.2 has two routes, one with next-hop
#     192.168.0.0 and the other one with next-hop 192.168.0.2
#   * Check that best route is through 192.168.0.0 (lowest neighbor
#     address)
# -------------------------------------------------------------------
sub cbgp_valid_bgp_dp_neighbor_address($)
{
  my ($cbgp)= @_;

  die if $cbgp->send("net add domain 1 igp\n");
  die if $cbgp->send("net add node 1.0.0.1\n");
  die if $cbgp->send("net node 1.0.0.1 domain 1\n");
  die if $cbgp->send("net add node 1.0.0.2\n");
  die if $cbgp->send("net node 1.0.0.2 domain 1\n");
  die if $cbgp->send("net add node 2.0.0.1\n");
  die if $cbgp->send("net node 2.0.0.1 domain 1\n");
  die if $cbgp->send("net add subnet 192.168.0.0/31 transit\n");
  die if $cbgp->send("net add subnet 192.168.0.2/31 transit\n");
  die if $cbgp->send("net add link 1.0.0.1 192.168.0.0/31 1\n");
  die if $cbgp->send("net add link 1.0.0.1 192.168.0.2/31 1\n");
  die if $cbgp->send("net add link 1.0.0.2 192.168.0.1/31 1\n");
  die if $cbgp->send("net add link 1.0.0.2 192.168.0.3/31 1\n");
  die if $cbgp->send("net add link 1.0.0.1 2.0.0.1 1\n");
  die if $cbgp->send("net domain 1 compute\n");

  die if $cbgp->send("bgp add router 1 1.0.0.1\n");
  die if $cbgp->send("bgp router 1.0.0.1\n");
  die if $cbgp->send("\tadd peer 1 192.168.0.1\n");
  die if $cbgp->send("\tpeer 192.168.0.1 next-hop 192.168.0.0\n");
  die if $cbgp->send("\tpeer 192.168.0.1 up\n");
  die if $cbgp->send("\tadd peer 1 192.168.0.3\n");
  die if $cbgp->send("\tpeer 192.168.0.3 next-hop 192.168.0.2\n");
  die if $cbgp->send("\tpeer 192.168.0.3 up\n");
  die if $cbgp->send("\tadd peer 2 2.0.0.1\n");
  die if $cbgp->send("\tpeer 2.0.0.1 virtual\n");
  die if $cbgp->send("\tpeer 2.0.0.1 up\n");
  die if $cbgp->send("\texit\n");

  die if $cbgp->send("bgp add router 1 1.0.0.2\n");
  die if $cbgp->send("bgp router 1.0.0.2\n");
  die if $cbgp->send("\tadd peer 1 192.168.0.0\n");
  die if $cbgp->send("\tpeer 192.168.0.0 up\n");
  die if $cbgp->send("\tadd peer 1 192.168.0.2\n");
  die if $cbgp->send("\tpeer 192.168.0.2 up\n");
  die if $cbgp->send("\texit\n");

  cbgp_recv_update($cbgp, "1.0.0.1", 1, "2.0.0.1",
		   "255/8|2|IGP|2.0.0.1|0|0");

  die if $cbgp->send("sim run\n");

  my $rib= cbgp_show_rib_mrt($cbgp, "1.0.0.2");

  if (!exists($rib->{"255.0.0.0/8"}) ||
      ($rib->{"255.0.0.0/8"}->[CBGP_RIB_NEXTHOP] ne "192.168.0.0")) {
    return TEST_FAILURE;
  }

  return TEST_SUCCESS;
}

# -----[ cbgp_valid_bgp_stateful ]-----------------------------------
# Test that BGP router avoids to propagate updates when not necessary.
#
# Setup:
#   - R1 (1.0.0.1, AS1)
#   - R2 (1.0.0.2, AS1)
#   - R3 (1.0.0.3, AS1)
#   - R4 (2.0.0.1, AS2)
#
# Topology:
#
#  R2
#    \
#     *-- R1 --- R4
#    /
#  R3
#
# Scenario:
#   * R3 advertises route R to R1
#   * R1 propagates to R4
#   * R2 advertises same route R to R1
#   * R1 prefers R2's route (lowest router-ID).
#     However, it does not propagate the route to R4 since the
#     route attributes are equal.
# -------------------------------------------------------------------
sub cbgp_valid_bgp_stateful($)
{
  my ($cbgp)= @_;
  my $mrt_record_file= "/tmp/cbgp-record-2.0.0.1.mrt";

  unlink "$mrt_record_file";

  cbgp_topo_dp4($cbgp,
		["1.0.0.2", 1, 1, 1],
		["1.0.0.3", 1, 1, 1],
		["2.0.0.1", 2, 1, 1, "record $mrt_record_file"],
		["3.0.0.1", 3, 1, 1]);
  cbgp_recv_update($cbgp, "1.0.0.1", 1, "1.0.0.2",
		   "255/16|3|IGP|3.0.0.1|0|0");
  cbgp_recv_update($cbgp, "1.0.0.1", 1, "1.0.0.3",
		   "255/16|3|IGP|3.0.0.1|0|0");
  cbgp_send($cbgp, "sim run");
  cbgp_checkpoint($cbgp);

  open(MRT_RECORD, "<$mrt_record_file") or
    die "unable to open \"$mrt_record_file\"";
  my $line_count= 0;
  while (<MRT_RECORD>) { $line_count++; }
  close(MRT_RECORD);

  unlink "$mrt_record_file";

  if ($line_count != 1) {
    return TEST_FAILURE;
  }
  return TEST_SUCCESS;
}

# -----[ cbgp_valid_bgp_filter_action_community_add ]----------------
# Test ability to attach communities to routes.
#
# Setup:
#
# Scenario:
#
# -------------------------------------------------------------------
sub cbgp_valid_bgp_filter_action_community_add()
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

# -----[ cbgp_valid_bgp_filter_action_community_remove ]-------------
# Test ability of a filter to remove a single community in a route.
#
# Setup:
#   - R1 (1.0.0.1, AS1)
#   - R2 (2.0.0.1, AS2) virtual peer
#   - R3 (3.0.0.1, AS3) virtual peer
#   - R4 (4.0.0.1, AS4) virtual peer
#   - filter [from R2] "any" -> "community remove 2:2"
#   - filter [from R3] "any" -> "community remove 3:1"
#   - filter [from R3] "any" -> "community remove 4:1, community remove 4:2"
#
# Topology:
#
#   (R2) --*
#           \
#            \
#   (R3) ---- R1
#            /
#           /
#   (R4) --*
#
# Scenario:
#   * Advertised prefixes: 253/8 [2:1, 2:2, 2:3] from R2,
#                          254/8 [3:5] from R3,
#                          255/8 [4:0, 4:1, 4:2] from R4
#   * Check that 253/8 has [2:1, 2:3]
#   * Check that 254/8 has [3:5]
#   * Check that 255/8 has [4:0]
# -------------------------------------------------------------------
sub cbgp_valid_bgp_filter_action_community_remove($)
{
    my ($cbgp)= @_;

    cbgp_topo_dp3($cbgp,
		  ["2.0.0.1", 2, 0],
		  ["3.0.0.1", 3, 0],
		  ["4.0.0.1", 4, 0]);
    cbgp_filter($cbgp, "1.0.0.1", "2.0.0.1", "in",
		"any", "community remove 2:2");
    cbgp_filter($cbgp, "1.0.0.1", "3.0.0.1", "in",
		"any", "community remove 3:1");
    cbgp_filter($cbgp, "1.0.0.1", "4.0.0.1", "in",
		"any", "community remove 4:1, community remove 4:2");
    cbgp_recv_update($cbgp, "1.0.0.1", 1, "2.0.0.1",
		     "253/8|2|IGP|2.0.0.1|0|0|2:1 2:2 2:3");
    cbgp_recv_update($cbgp, "1.0.0.1", 1, "3.0.0.1",
		     "254/8|3|IGP|3.0.0.1|0|0|3:5");
    cbgp_recv_update($cbgp, "1.0.0.1", 1, "4.0.0.1",
		     "255/8||IGP|4.0.0.1|0|0|4:0 4:1 4:2");

    my $rib;
    $rib= cbgp_show_rib_mrt($cbgp, "1.0.0.1");
    (!exists($rib->{"253.0.0.0/8"}) ||
     !community_equals($rib->{"253.0.0.0/8"}->[CBGP_RIB_COMMUNITY],
			 ["2:1", "2:3"])) and
	return TEST_FAILURE;
    (!exists($rib->{"254.0.0.0/8"}) ||
     !community_equals($rib->{"254.0.0.0/8"}->[CBGP_RIB_COMMUNITY],
			 ["3:5"])) and
	return TEST_FAILURE;
    (!exists($rib->{"255.0.0.0/8"}) ||
     !community_equals($rib->{"255.0.0.0/8"}->[CBGP_RIB_COMMUNITY],
			 ["4:0"])) and
	return TEST_FAILURE;

  return TEST_SUCCESS;
}

# -----[ cbgp_valid_bgp_filter_action_community_strip ]--------------
# Test ability of a filter to remove all communities from a route.
#
# Setup:
#   - R1 (1.0.0.1, AS1)
#   - R2 (2.0.0.1, AS2) virtual peer
#   - filter [from R2] "any" -> "community strip"
#
# Topology:
#
#   (R2) ----- R1
#
# Scenario:
#   * Advertised prefix 255/8 [2:1 2:255] from R2
#   * Check that 255/8 does not contain communities
# -------------------------------------------------------------------
sub cbgp_valid_bgp_filter_action_community_strip($)
{
  my ($cbgp)= @_;

  cbgp_topo_dp3($cbgp,
		["2.0.0.1", 2, 0]);
  cbgp_filter($cbgp, "1.0.0.1", "2.0.0.1", "in",
	      "any", "community strip");
  cbgp_recv_update($cbgp, "1.0.0.1", 1, "2.0.0.1",
		   "255/8|2|IGP|2.0.0.1|0|0|2:1 2:255");

  my $rib;
  $rib= cbgp_show_rib_mrt($cbgp, "1.0.0.1");
  (!exists($rib->{"255.0.0.0/8"}) ||
   !community_equals($rib->{"255.0.0.0/8"}->[CBGP_RIB_COMMUNITY],
		     [])) and
		       return TEST_FAILURE;

  return TEST_SUCCESS;
}

# -----[ cbgp_valid_bgp_filter_action_deny ]-------------------------
# Test ability to deny a route.
#
# Setup:
#
# Scenario:
#
# -------------------------------------------------------------------
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
# Test ability to set the local-pref value of routes.
#
# Setup:
#
# Scenario:
#
# -------------------------------------------------------------------
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
# Test ability to attach a specific MED value to routes.
#
# Setup:
#   - R1 (1.0.0.1, AS1)
#   - R2 (2.0.0.1, AS2)
#   - filter [from R2] "any" -> "metric 57"
#
# Topology:
#
#   (R2) ----- R1
#
# Scenario:
#   * Advertised route 255/16 [metric:0]
#   * Check that route has MED value set to 57
# -------------------------------------------------------------------
sub cbgp_valid_bgp_filter_action_med()
{
  my ($cbgp)= @_;

  cbgp_topo_filter($cbgp);
  cbgp_filter($cbgp, "1.0.0.1", "2.0.0.1", "in",
	      "any", "metric 57");
  die if $cbgp->send("bgp router 1.0.0.1 peer 2.0.0.1 recv ".
		     "\"BGP4|0|A|1.0.0.1|1|255/16|2|IGP|2.0.0.1|0|0\"\n");
  die if $cbgp->send("sim run\n");

  my $rib;
  $rib= cbgp_show_rib($cbgp, "1.0.0.1");
  (!exists($rib->{"255.0.0.0/16"}) ||
   ($rib->{"255.0.0.0/16"}->[CBGP_RIB_MED] != 57)) and
     return TEST_FAILURE;

  return TEST_SUCCESS;
}

# -----[ cbgp_valid_bgp_filter_action_med_internal ]-----------------
# Test ability to attach a MED value to routes, based on the IGP
# cost.
#
# Setup:
#   - R1 (1.0.0.1, AS1)
#   - R2 (1.0.0.2, AS1) virtual peer of R1 (IGP cost:5)
#   - R3 (1.0.0.3, AS1) virtual peer of R1 (IGP cost:10)
#   - R4 (2.0.0.1, AS2) peer of R1
#   - filter [to R2] "any" -> "metric internal"
#
# Topology:
#
#   (R2) ---*
#            \
#             R1 ---- R4
#            /
#   (R3) ---*
#
# Scenario:
#   * Advertise 254/8 from R2, 255/8 from R3
#   * Check that R4 has 254/8 with MED:5 and 255/8 with MED:10
# -------------------------------------------------------------------
sub cbgp_valid_bgp_filter_action_med_internal()
{
  my ($cbgp)= @_;
  cbgp_topo_dp4($cbgp,
		["1.0.0.2", 1, 5, 1],
		["1.0.0.3", 1, 10, 1],
		["2.0.0.1", 2, 0]);
  cbgp_filter($cbgp, "1.0.0.1", "2.0.0.1", "out",
	      "any", "metric internal");
  cbgp_recv_update($cbgp, "1.0.0.1", 1, "1.0.0.2",
		   "254/8|254|IGP|1.0.0.2|0|0");
  cbgp_recv_update($cbgp, "1.0.0.1", 1, "1.0.0.3",
		   "255/8|255|IGP|1.0.0.3|0|0");
  die if $cbgp->send("sim run\n");

  my $rib;
  $rib= cbgp_show_rib($cbgp, "2.0.0.1");
  (!exists($rib->{"254.0.0.0/8"}) ||
   ($rib->{"254.0.0.0/8"}->[CBGP_RIB_MED] != 5)) and
     return TEST_FAILURE;
  (!exists($rib->{"255.0.0.0/8"}) ||
   ($rib->{"255.0.0.0/8"}->[CBGP_RIB_MED] != 10)) and
     return TEST_FAILURE;

  return TEST_SUCCESS;
}

# -----[ cbgp_valid_bgp_filter_action_aspath ]-----------------------
# -------------------------------------------------------------------
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
# -------------------------------------------------------------------
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

# -----[ cbgp_valid_bgp_filter_match_prefix_is ]---------------------
# Test ability of a filter to match route on a prefix basis.
#
# Setup:
#   - R1 (1.0.0.1, AS1)
#   - R2 (2.0.0.1, AS2) virtual peer
#   - filter "prefix is 123.234/16" -> community 1:1
#   - filter "prefix is 123.235/16" -> community 1:2
#
# Topology:
#
#   R1 ----- R2
#
# Scenario:
#   * Advertise 123.234/16, 123.235/16 and 123.236/16
#   * Check that 123.234/16 has community 1:1
#   * Check that 123.235/16 has community 1:2
#   * Check that 123.236/16 has no community
# -------------------------------------------------------------------
sub cbgp_valid_bgp_filter_match_prefix_is($)
{
  my ($cbgp)= @_;

  cbgp_topo_dp3($cbgp, ["2.0.0.1", 2, 0]);
  cbgp_filter($cbgp, "1.0.0.1", "2.0.0.1", "in",
	      "prefix is 123.234/16",
	      "community add 1:1");
  cbgp_filter($cbgp, "1.0.0.1", "2.0.0.1", "in",
	      "prefix is 123.235/16",
	      "community add 1:2");
  cbgp_recv_update($cbgp, "1.0.0.1", 1, "2.0.0.1",
		   "123.234/16|2|IGP|2.0.0.1|0|0");
  cbgp_recv_update($cbgp, "1.0.0.1", 1, "2.0.0.1",
		   "123.235/16|2|IGP|2.0.0.1|0|0");
  cbgp_recv_update($cbgp, "1.0.0.1", 1, "2.0.0.1",
		   "123.236/16|2|IGP|2.0.0.1|0|0");
  my $rib;
  $rib= cbgp_show_rib_mrt($cbgp, "1.0.0.1");
  (exists($rib->{"123.234.0.0/16"})) or return TEST_FAILURE;
  (community_equals($rib->{"123.234.0.0/16"}->[CBGP_RIB_COMMUNITY],
		    ["1:1"])) or
		      return TEST_FAILURE;
  (exists($rib->{"123.235.0.0/16"})) or return TEST_FAILURE;
  (community_equals($rib->{"123.235.0.0/16"}->[CBGP_RIB_COMMUNITY],
		    ["1:2"])) or
		      return TEST_FAILURE;
  (exists($rib->{"123.236.0.0/16"})) or return TEST_FAILURE;
  (community_equals($rib->{"123.236.0.0/16"}->[CBGP_RIB_COMMUNITY],
		    [])) or
		      return TEST_FAILURE;

  return TEST_SUCCESS;
}

# -----[ cbgp_valid_bgp_filter_match_prefix_in ]---------------------
# Test ability of a filter to match route on a prefix basis.
#
# Setup:
#   - R1 (1.0.0.1, AS1)
#   - R2 (2.0.0.1, AS2) virtual peer
#   - filter "prefix in 123.234/16" -> community 1:1
#   - filter "prefix in 123.235/16" -> community 1:2
#
# Topology:
#
#   R1 ----- R2
#
# Scenario:
#   * Advertise 123.234/16, 123.235/16 and 123.236/16
#   * Check that 123.234.1/24 has community 1:1
#   * Check that 123.235.1/24 has community 1:2
#   * Check that 123.236.1/24 has no community
# -------------------------------------------------------------------
sub cbgp_valid_bgp_filter_match_prefix_in($)
{
  my ($cbgp)= @_;

  cbgp_topo_dp3($cbgp, ["2.0.0.1", 2, 0]);
  cbgp_filter($cbgp, "1.0.0.1", "2.0.0.1", "in",
	      "prefix in 123.234/16",
	      "community add 1:1");
  cbgp_filter($cbgp, "1.0.0.1", "2.0.0.1", "in",
	      "prefix in 123.235/16",
	      "community add 1:2");
  cbgp_recv_update($cbgp, "1.0.0.1", 1, "2.0.0.1",
		   "123.234.1/24|2|IGP|2.0.0.1|0|0");
  cbgp_recv_update($cbgp, "1.0.0.1", 1, "2.0.0.1",
		   "123.235.1/24|2|IGP|2.0.0.1|0|0");
  cbgp_recv_update($cbgp, "1.0.0.1", 1, "2.0.0.1",
		   "123.236.1/24|2|IGP|2.0.0.1|0|0");
  my $rib;
  $rib= cbgp_show_rib_mrt($cbgp, "1.0.0.1");
  (exists($rib->{"123.234.1.0/24"})) or return TEST_FAILURE;
  (community_equals($rib->{"123.234.1.0/24"}->[CBGP_RIB_COMMUNITY],
		    ["1:1"])) or
		      return TEST_FAILURE;
  (exists($rib->{"123.235.1.0/24"})) or return TEST_FAILURE;
  (community_equals($rib->{"123.235.1.0/24"}->[CBGP_RIB_COMMUNITY],
		    ["1:2"])) or
		      return TEST_FAILURE;
  (exists($rib->{"123.236.1.0/24"})) or return TEST_FAILURE;
  (community_equals($rib->{"123.236.1.0/24"}->[CBGP_RIB_COMMUNITY],
		    [])) or
		      return TEST_FAILURE;

  return TEST_SUCCESS;
}

# -----[ cbgp_valid_bgp_filter_match_prefix_in_length ]--------------
sub cbgp_valid_bgp_filter_match_prefix_in_length($)
{
  return TEST_NOT_TESTED;
}

# -----[ cbgp_valid_bgp_filter_match_aspath_regex ]------------------
# Test ability of a filter to match route on a prefix basis.
#
# Setup:
#   - R1 (1.0.0.1, AS1)
#   - R2 (2.0.0.1, AS2) virtual peer
#   - filter "path [^2 .*]" -> community 1:1
#   - filter "path [.* 254$]" -> community 1:2
#   - filter "path [.* 255$]" -> community 1:3
#
# Scenario:
#   * Advertise 253/8 [2 253], 254/8 [2 254] and 255/8 [2 255]
#   * Check that 253/8 has community 1:1
#   * Check that 254/8 has community 1:2
#   * Check that 255/8 has no community
# -------------------------------------------------------------------
sub cbgp_valid_bgp_filter_match_aspath_regex($)
{
  my ($cbgp)= @_;

  cbgp_topo_dp3($cbgp, ["2.0.0.1", 2, 0], ["3.0.0.1", 3, 0]);
  cbgp_filter($cbgp, "1.0.0.1", "2.0.0.1", "in",
	      "path \\\"^2 .*\\\"",
	      "community add 1:1");
  cbgp_filter($cbgp, "1.0.0.1", "2.0.0.1", "in",
	      "path \\\".* 254\$\\\"",
	      "community add 1:2");
  cbgp_filter($cbgp, "1.0.0.1", "2.0.0.1", "in",
	      "path \\\".* 255\$\\\"",
  	      "community add 1:3");
  cbgp_filter($cbgp, "1.0.0.1", "3.0.0.1", "in",
	      "path \\\"^2 .*\\\"",
	      "community add 1:1");
  cbgp_filter($cbgp, "1.0.0.1", "3.0.0.1", "in",
	      "path \\\".* 254\$\\\"",
	      "community add 1:2");
  cbgp_filter($cbgp, "1.0.0.1", "3.0.0.1", "in",
	      "path \\\".* 255\$\\\"",
  	      "community add 1:3");
  cbgp_recv_update($cbgp, "1.0.0.1", 1, "2.0.0.1",
		   "253.2/16|2 253|IGP|2.0.0.1|0|0");
  cbgp_recv_update($cbgp, "1.0.0.1", 1, "2.0.0.1",
		   "254.2/16|2 254|IGP|2.0.0.1|0|0");
  cbgp_recv_update($cbgp, "1.0.0.1", 1, "2.0.0.1",
		   "255.2/16|2 255|IGP|2.0.0.1|0|0");
  cbgp_recv_update($cbgp, "1.0.0.1", 1, "3.0.0.1",
		   "253.3/16|3 253|IGP|3.0.0.1|0|0");
  cbgp_recv_update($cbgp, "1.0.0.1", 1, "3.0.0.1",
		   "254.3/16|3 254|IGP|3.0.0.1|0|0");
  cbgp_recv_update($cbgp, "1.0.0.1", 1, "3.0.0.1",
		   "255.3/16|3 255|IGP|3.0.0.1|0|0");

  my $rib;
  $rib= cbgp_show_rib_mrt($cbgp, "1.0.0.1");
  (exists($rib->{"253.2.0.0/16"})) or return TEST_FAILURE;
  (community_equals($rib->{"253.2.0.0/16"}->[CBGP_RIB_COMMUNITY],
		    ["1:1"])) or
		      return TEST_FAILURE;
  (exists($rib->{"254.2.0.0/16"})) or return TEST_FAILURE;
  (community_equals($rib->{"254.2.0.0/16"}->[CBGP_RIB_COMMUNITY],
		    ["1:1", "1:2"])) or
		      return TEST_FAILURE;
  (exists($rib->{"255.2.0.0/16"})) or return TEST_FAILURE;
  (community_equals($rib->{"255.2.0.0/16"}->[CBGP_RIB_COMMUNITY],
		    ["1:1", "1:3"])) or
		      return TEST_FAILURE;
  (exists($rib->{"253.3.0.0/16"})) or return TEST_FAILURE;
  (community_equals($rib->{"253.3.0.0/16"}->[CBGP_RIB_COMMUNITY],
		    [])) or
		      return TEST_FAILURE;
  (exists($rib->{"254.3.0.0/16"})) or return TEST_FAILURE;
  (community_equals($rib->{"254.3.0.0/16"}->[CBGP_RIB_COMMUNITY],
		    ["1:2"])) or
		      return TEST_FAILURE;
  (exists($rib->{"255.3.0.0/16"})) or return TEST_FAILURE;
  (community_equals($rib->{"255.3.0.0/16"}->[CBGP_RIB_COMMUNITY],
		    ["1:3"])) or
		      return TEST_FAILURE;

  return TEST_SUCCESS;
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
    (cbgp_rib_check($rib, "255/8", [CBGP_RIB_PREF, 100])) or
	return TEST_FAILURE;
    (cbgp_rib_check($rib, "254/8", [CBGP_RIB_PREF, 80])) or
	return TEST_FAILURE;
    (cbgp_rib_check($rib, "253/8", [CBGP_RIB_PREF, 60])) or
	return TEST_FAILURE;
    return TEST_SUCCESS;
}

# -----[ cbgp_valid_bgp_route_map ]----------------------------------
# Test the ability to define route-maps (i.e. filters that can be
# shared by multiple BGP sessions.
#
# Setup:
#   - R1 (1.0.0.1, AS1)
#   - R2 (2.0.0.1, AS2, virtual)
#
# Topology:
#
#   R1 ----- (R2)
#
# Scenario:
#   * Define a route-map named RM_LP_100 that changes the local-pref
#     of routes matching prefix 128/8
#   * Define a route-map named RM_LP_200 that changes the local-pref
#     of routes matching prefix 0/8
#   * Assign both route-maps as input filter of R1 for routes
#     received from R2
#   * Inject a route towards 128/8 in R1 from R2
#   * Inject a route towards 0/8 in R1 from R2
#   * Check that the local-pref is as follows
#       128/8 -> LP=100
#       0/8   -> LP=200
# -------------------------------------------------------------------
sub cbgp_valid_bgp_route_map($)
{
  my ($cbgp)= @_;

  # Define two route-maps
  cbgp_send($cbgp, "bgp route-map RM_LP_100");
  cbgp_send($cbgp, "  add-rule");
  cbgp_send($cbgp, "    match \"prefix in 128/8\"");
  cbgp_send($cbgp, "    action \"local-pref 100\"");
  cbgp_send($cbgp, "    exit");
  cbgp_send($cbgp, "  exit");
  cbgp_send($cbgp, "bgp route-map RM_LP_200");
  cbgp_send($cbgp, "  add-rule");
  cbgp_send($cbgp, "    match \"prefix in 0/8\"");
  cbgp_send($cbgp, "    action \"local-pref 200\"");
  cbgp_send($cbgp, "    exit");
  cbgp_send($cbgp, "  exit");

  # Create one BGP router and injects
  cbgp_topo_dp3($cbgp, ["2.0.0.1", 2, 0]);
  cbgp_filter($cbgp, "1.0.0.1", "2.0.0.1", "in",
		"any",
		"call RM_LP_100");
  cbgp_filter($cbgp, "1.0.0.1", "2.0.0.1", "in",
		"any",
		"call RM_LP_200");
  cbgp_recv_update($cbgp, "1.0.0.1", 1, "2.0.0.1",
		   "128/8|2|IGP|2.0.0.1|0|0|");
  cbgp_recv_update($cbgp, "1.0.0.1", 1, "2.0.0.1",
		   "0/8|2|IGP|2.0.0.1|0|0|");
  my $rib;
  $rib= cbgp_show_rib_mrt($cbgp, "1.0.0.1");
  (cbgp_rib_check($rib, "128/8", [CBGP_RIB_PREF, 100])) or
    return TEST_FAILURE;
  (cbgp_rib_check($rib, "0/8", [CBGP_RIB_PREF, 200])) or
    return TEST_FAILURE;
  return TEST_SUCCESS;
}

# -----[ cbgp_valid_bgp_route_map_duplicate ]------------------------
# Check that an error message is issued if an attempt is made to
# define two route-maps with the same name.
#
# Setup:
#
# -------------------------------------------------------------------
sub cbgp_valid_bgp_route_map_duplicate($)
{
  my ($cbgp)= @_;
  my $ERROR_MSG= "route-map already exists";

  cbgp_send($cbgp, "bgp route-map RM_LP_100");
  cbgp_send($cbgp, "  exit");
  cbgp_send($cbgp, "bgp route-map RM_LP_100");
  my $error_msg= cbgp_check_error($cbgp);
  if (!defined($error_msg) ||
     !($error_msg =~ /$ERROR_MSG/)) {
    return TEST_FAILURE;
  }

  return TEST_SUCCESS;
}

# -----[ cbgp_valid_igp_bgp_metric_change ]--------------------------
# Test the ability to recompute IGP paths and reselect BGP routes
# accordingly.
#
# Setup:
#   - R1 (1.0.0.1, AS1)
#   - R2 (1.0.0.2, AS1) virtual peer of R1 (IGP cost:5)
#   - R3 (1.0.0.3, AS1) virtual peer of R1 (IGP cost:10)
#   - R4 (2.0.0.1, AS2) peer of R1
#
# Topology:
#
#   (R2) --*
#           \
#            R1 ----- R4
#           /
#   (R3) --*
#
# Scenario:
#   * Advertised 255/8 [255:1] from R2, 255/8 [255:2] from R3
#   * Check default route selection: 255/8 [255:1]
#   * Increase IGP cost between R1 and R2, re-compute IGP, rescan.
#   * Check that route is now 255/8 [255:2]
#   * Decrease IGP cost between R1 and R2, re-compute IGP, rescan
#   * Check that default route is selected
# -------------------------------------------------------------------
sub cbgp_valid_igp_bgp_metric_change()
{
    my ($cbgp)= @_;

    cbgp_topo_igp_bgp($cbgp, ["1.0.0.2", 5], ["1.0.0.3", 10]);
    cbgp_recv_update($cbgp, "1.0.0.1", 1, "1.0.0.2",
		     "255/8|255|IGP|1.0.0.2|0|0|255:1");
    cbgp_recv_update($cbgp, "1.0.0.1", 1, "1.0.0.3",
		     "255/8|255|IGP|1.0.0.3|0|0|255:2");
    die if $cbgp->send("sim run\n");

    my $rib;
    $rib= cbgp_show_rib_mrt($cbgp, "2.0.0.1");
    (!exists($rib->{"255.0.0.0/8"}) ||
     !community_equals($rib->{"255.0.0.0/8"}->[CBGP_RIB_COMMUNITY],
		       ["255:1"])) and
			 return TEST_FAILURE;

    # Increase weight of link R1-R2
    die if $cbgp->send("net link 1.0.0.1 1.0.0.2 igp-weight 100\n");
    die if $cbgp->send("net link 1.0.0.2 1.0.0.1 igp-weight 100\n");
    die if $cbgp->send("net domain 1 compute\n");
    die if $cbgp->send("bgp domain 1 rescan\n");
    die if $cbgp->send("sim run\n");

    $rib= cbgp_show_rib_mrt($cbgp, "2.0.0.1");
    (!exists($rib->{"255.0.0.0/8"}) ||
     !community_equals($rib->{"255.0.0.0/8"}->[CBGP_RIB_COMMUNITY],
		       ["255:2"])) and
			 return TEST_FAILURE;

    # Restore weight of link R1-R2
    die if $cbgp->send("net link 1.0.0.1 1.0.0.2 igp-weight 5\n");
    die if $cbgp->send("net link 1.0.0.2 1.0.0.1 igp-weight 5\n");
    die if $cbgp->send("net domain 1 compute\n");
    die if $cbgp->send("bgp domain 1 rescan\n");
    die if $cbgp->send("sim run\n");


    $rib= cbgp_show_rib_mrt($cbgp, "2.0.0.1");
    (!exists($rib->{"255.0.0.0/8"}) ||
     !community_equals($rib->{"255.0.0.0/8"}->[CBGP_RIB_COMMUNITY],
		       ["255:1"])) and
			 return TEST_FAILURE;

    return TEST_SUCCESS;
}

# -----[ cbgp_valid_igp_bgp_state_change ]---------------------------
# Test the ability to recompute IGP paths and reselect BGP routes
# accordingly.
#
# Setup:
#   - R1 (1.0.0.1, AS1)
#   - R2 (1.0.0.2, AS1) virtual peer of R1 (IGP cost:5)
#   - R3 (1.0.0.3, AS1) virtual peer of R1 (IGP cost:10)
#   - R4 (2.0.0.1, AS2) peer of R1
#
# Topology:
#
#   (R2) --*
#           \
#            R1 ----- R4
#           /
#   (R3) --*
#
# Scenario:
#   * Advertised 255/8 [255:1] from R2, 255/8 [255:2] from R3
#   * Check default route selection: 255/8 [255:1]
#   * Shut down link R1-R2, re-compute IGP, rescan
#   * Check that route is now 255/8 [255:2]
#   * Restore link R1-R2, re-compute IGP, rescan
#   * Check that default route is selected
# -------------------------------------------------------------------
sub cbgp_valid_igp_bgp_state_change()
{
    my ($cbgp)= @_;

    cbgp_topo_igp_bgp($cbgp,
		      ["1.0.0.2", 5, "soft-restart"],
		      ["1.0.0.3", 10, "soft-restart"]);
    cbgp_recv_update($cbgp, "1.0.0.1", 1, "1.0.0.2",
		     "255/8|255|IGP|1.0.0.2|0|0|255:1");
    cbgp_recv_update($cbgp, "1.0.0.1", 1, "1.0.0.3",
		     "255/8|255|IGP|1.0.0.3|0|0|255:2");
    die if $cbgp->send("sim run\n");

    my $rib;
    $rib= cbgp_show_rib_mrt($cbgp, "2.0.0.1");
    (!exists($rib->{"255.0.0.0/8"}) ||
     !community_equals($rib->{"255.0.0.0/8"}->[CBGP_RIB_COMMUNITY],
		       ["255:1"])) and
			 return TEST_FAILURE;

    # Increase weight of link R1-R2
    die if $cbgp->send("net link 1.0.0.1 1.0.0.2 down\n");
    die if $cbgp->send("net domain 1 compute\n");
    die if $cbgp->send("bgp domain 1 rescan\n");
    die if $cbgp->send("sim run\n");

    $rib= cbgp_show_rib_mrt($cbgp, "2.0.0.1");
    (!exists($rib->{"255.0.0.0/8"}) ||
     !community_equals($rib->{"255.0.0.0/8"}->[CBGP_RIB_COMMUNITY],
		       ["255:2"])) and
			 return TEST_FAILURE;

    # Restore weight of link R1-R2
    die if $cbgp->send("net link 1.0.0.1 1.0.0.2 up\n");
    die if $cbgp->send("net domain 1 compute\n");
    die if $cbgp->send("bgp domain 1 rescan\n");
    die if $cbgp->send("sim run\n");


    $rib= cbgp_show_rib_mrt($cbgp, "2.0.0.1");
    (!exists($rib->{"255.0.0.0/8"}) ||
     !community_equals($rib->{"255.0.0.0/8"}->[CBGP_RIB_COMMUNITY],
		       ["255:1"])) and
			 return TEST_FAILURE;

    return TEST_SUCCESS;
}

# -----[ cbgp_valid_igp_bgp_reach_link ]-----------------------------
sub cbgp_valid_igp_bgp_reach_link($)
{
    return TEST_NOT_TESTED;
}

# -----[ cbgp_valid_igp_bgp_reach_subnet ]-----------------------------
sub cbgp_valid_igp_bgp_reach_subnet($)
{
    return TEST_NOT_TESTED;
}

# -----[ cbgp_valid_igp_bgp_med ]------------------------------------
# Test the ability to recompute IGP paths, reselect BGP routes and
# update MED accordingly.
#
# Setup:
#   - R1 (1.0.0.1, AS1)
#   - R2 (1.0.0.2, AS1) virtual peer of R1 (IGP cost:5)
#   - R3 (1.0.0.3, AS1) virtual peer of R1 (IGP cost:10)
#   - R4 (2.0.0.1, AS2) peer of R1 with "internal" MED advertisement
#
# Topology:
#
#   (R2) --*
#           \
#            R1 ----- R4
#           /
#   (R3) --*
#
# Scenario:
#   * Advertised 255/8 from R2 and R3
#   * Check default route selection: 255/8
#   * Check that MED value equals 5 (IGP weight)
#   * Increase IGP cost between R1 and R2, re-compute IGP, rescan
#   * Check that MED value equals 10 (IGP weight)
#   * Decrease IGP cost between R1 and R2, re-compute IGP, rescan
#   * Check that MED value equals 5 (IGP weight)
# -------------------------------------------------------------------
sub cbgp_valid_igp_bgp_med($)
{
    my ($cbgp)= @_;

    cbgp_topo_igp_bgp($cbgp, ["1.0.0.2", 5], ["1.0.0.3", 10]);
    cbgp_filter($cbgp, "1.0.0.1", "2.0.0.1", "out",
		"any", "metric internal");
    cbgp_recv_update($cbgp, "1.0.0.1", 1, "1.0.0.2",
		     "255/8|255|IGP|1.0.0.2|0|0|255:1");
    cbgp_recv_update($cbgp, "1.0.0.1", 1, "1.0.0.3",
		     "255/8|255|IGP|1.0.0.3|0|0|255:2");
    die if $cbgp->send("sim run\n");

    my $rib;
    $rib= cbgp_show_rib_mrt($cbgp, "2.0.0.1");
    (!exists($rib->{"255.0.0.0/8"}) ||
     !community_equals($rib->{"255.0.0.0/8"}->[CBGP_RIB_COMMUNITY],
		       ["255:1"]) ||
     ($rib->{"255.0.0.0/8"}->[CBGP_RIB_MED] != 5)) and
			 return TEST_FAILURE;

    # Increase weight of link R1-R2
    die if $cbgp->send("net link 1.0.0.1 1.0.0.2 igp-weight 100\n");
    die if $cbgp->send("net link 1.0.0.2 1.0.0.1 igp-weight 100\n");
    die if $cbgp->send("net domain 1 compute\n");
    die if $cbgp->send("bgp domain 1 rescan\n");
    die if $cbgp->send("sim run\n");

    $rib= cbgp_show_rib_mrt($cbgp, "2.0.0.1");
    (!exists($rib->{"255.0.0.0/8"}) ||
     !community_equals($rib->{"255.0.0.0/8"}->[CBGP_RIB_COMMUNITY],
		       ["255:2"]) ||
     ($rib->{"255.0.0.0/8"}->[CBGP_RIB_MED] != 10)) and
			 return TEST_FAILURE;

    # Restore weight of link R1-R2
    die if $cbgp->send("net link 1.0.0.1 1.0.0.2 igp-weight 5\n");
    die if $cbgp->send("net link 1.0.0.2 1.0.0.1 igp-weight 5\n");
    die if $cbgp->send("net domain 1 compute\n");
    die if $cbgp->send("bgp domain 1 rescan\n");
    die if $cbgp->send("sim run\n");


    $rib= cbgp_show_rib_mrt($cbgp, "2.0.0.1");
    (!exists($rib->{"255.0.0.0/8"}) ||
     !community_equals($rib->{"255.0.0.0/8"}->[CBGP_RIB_COMMUNITY],
		       ["255:1"]) ||
     ($rib->{"255.0.0.0/8"}->[CBGP_RIB_MED] != 5)) and
			 return TEST_FAILURE;

    return TEST_SUCCESS;
}

# -----[ cbgp_valid_bgp_load_rib ]-----------------------------------
# Test ability to load a BGP dump into a router. The content of the
# BGP dump must be specified in MRT format.
#
# Setup:
#   - R1 (198.32.12.9, AS11537)
#
# Scenario:
#   * Load BGP dump collected in Abilene into router
#   * Check that all routes are loaded in the router with the right
#     attributes
#
# Resources:
#   [abilene-rib.ascii]
# -------------------------------------------------------------------
sub cbgp_valid_bgp_load_rib($)
{
    my ($cbgp)= @_;
    my $rib_file= $resources_path."abilene-rib.ascii";
    cbgp_send($cbgp, "bgp options auto-create on");
    cbgp_send($cbgp, "net add node 198.32.12.9");
    cbgp_send($cbgp, "bgp add router 11537 198.32.12.9");
    cbgp_send($cbgp, "bgp router 198.32.12.9 load rib $rib_file");
    my $rib;
    $rib= cbgp_show_rib($cbgp, "198.32.12.9");
    if (scalar(keys %$rib) != `cat $rib_file | wc -l`) {
	$tests->debug("number of prefixes mismatch");
	return TEST_FAILURE;
    }
    open(RIB, "<$rib_file") or die;
    while (<RIB>) {
	chomp;
	my @fields= split /\|/;
	$rib= cbgp_show_rib($cbgp, "198.32.12.9", $fields[MRT_PREFIX]);
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
    }
    close(RIB);
    return TEST_SUCCESS;
}

# -----[ cbgp_valid_bgp_deflection ]---------------------------------
sub cbgp_valid_bgp_deflection()
{
    return TEST_NOT_TESTED;
}

# -----[ cbgp_valid_bgp_rr ]-----------------------------------------
# Test basic route-reflection mechanisms.
#
# Setup:
#   - R1 (1.0.0.1, AS1)
#   - R2 (1.0.0.2, AS1) route-reflector
#   - R3 (1.0.0.3, AS1) client of R3
#   - R4 (1.0.0.4, AS1) peer of R4 (non-client)
#   - R5 (1.0.0.5, AS1) peer of R2 (non-client)
#   - R6 (2.0.0.1, AS2) virtual peer of R1
#   - R7 (3.0.0.1, AS3)
#   - R8 (4.0.0.1, AS4)
#
# Topology:
#
#                       *---- R7
#                      /
#   (R6) ---- R1 ---- R2 ---- R3 ---- R4
#                      \       \
#                       \        *---- R8
#                        \
#                         *---- R5
#
# Scenario:
#   * R6 advertise route towards 255/8
#   * Check that R1, R2 and R3 have the route
#   * Check that R4 ad R5 do not have the route
#   * Check that R2 has set the originator-ID to the router-ID of R1
#   * Check that the route received by R3 has a cluster-ID-list
#     containing the router-ID of R2 (i.e. 1.0.0.2)
#   * Check that R7 and R8 have received the route and no
#     RR-attributes (Originator-ID and Cluster-ID-List) are present
# -------------------------------------------------------------------
sub cbgp_valid_bgp_rr($)
{
  my ($cbgp)= @_;

  die if $cbgp->send("net add domain 1 igp\n");
  die if $cbgp->send("net add node 1.0.0.1\n");
  die if $cbgp->send("net node 1.0.0.1 domain 1\n");
  die if $cbgp->send("net add node 1.0.0.2\n");
  die if $cbgp->send("net node 1.0.0.2 domain 1\n");
  die if $cbgp->send("net add node 1.0.0.3\n");
  die if $cbgp->send("net node 1.0.0.3 domain 1\n");
  die if $cbgp->send("net add node 1.0.0.4\n");
  die if $cbgp->send("net node 1.0.0.4 domain 1\n");
  die if $cbgp->send("net add node 1.0.0.5\n");
  die if $cbgp->send("net node 1.0.0.5 domain 1\n");
  die if $cbgp->send("net add node 2.0.0.1\n");
  die if $cbgp->send("net node 2.0.0.1 domain 1\n");
  die if $cbgp->send("net add node 3.0.0.1\n");
  die if $cbgp->send("net node 3.0.0.1 domain 1\n");
  die if $cbgp->send("net add node 4.0.0.1\n");
  die if $cbgp->send("net node 4.0.0.1 domain 1\n");
  die if $cbgp->send("net add link 1.0.0.1 1.0.0.2 1\n");
  die if $cbgp->send("net add link 1.0.0.1 2.0.0.1 1\n");
  die if $cbgp->send("net add link 1.0.0.2 1.0.0.3 1\n");
  die if $cbgp->send("net add link 1.0.0.2 1.0.0.5 1\n");
  die if $cbgp->send("net add link 1.0.0.2 3.0.0.1 1\n");
  die if $cbgp->send("net add link 1.0.0.3 1.0.0.4 1\n");
  die if $cbgp->send("net add link 1.0.0.3 4.0.0.1 1\n");
  die if $cbgp->send("net domain 1 compute\n");

  die if $cbgp->send("bgp add router 1 1.0.0.1\n");
  die if $cbgp->send("bgp router 1.0.0.1\n");
  die if $cbgp->send("\tadd peer 1 1.0.0.2\n");
  die if $cbgp->send("\tpeer 1.0.0.2 up\n");
  die if $cbgp->send("\tadd peer 2 2.0.0.1\n");
  die if $cbgp->send("\tpeer 2.0.0.1 virtual\n");
  die if $cbgp->send("\tpeer 2.0.0.1 up\n");
  die if $cbgp->send("\texit\n");

  die if $cbgp->send("bgp add router 1 1.0.0.2\n");
  die if $cbgp->send("bgp router 1.0.0.2\n");
  die if $cbgp->send("\tadd peer 1 1.0.0.1\n");
  die if $cbgp->send("\tpeer 1.0.0.1 up\n");
  die if $cbgp->send("\tadd peer 1 1.0.0.3\n");
  die if $cbgp->send("\tpeer 1.0.0.3 rr-client\n");
  die if $cbgp->send("\tpeer 1.0.0.3 up\n");
  die if $cbgp->send("\tadd peer 1 1.0.0.5\n");
  die if $cbgp->send("\tpeer 1.0.0.5 up\n");
  die if $cbgp->send("\tadd peer 3 3.0.0.1\n");
  die if $cbgp->send("\tpeer 3.0.0.1 up\n");
  die if $cbgp->send("\texit\n");

  die if $cbgp->send("bgp add router 1 1.0.0.3\n");
  die if $cbgp->send("bgp router 1.0.0.3\n");
  die if $cbgp->send("\tadd peer 1 1.0.0.2\n");
  die if $cbgp->send("\tpeer 1.0.0.2 up\n");
  die if $cbgp->send("\tadd peer 1 1.0.0.4\n");
  die if $cbgp->send("\tpeer 1.0.0.4 up\n");
  die if $cbgp->send("\tadd peer 4 4.0.0.1\n");
  die if $cbgp->send("\tpeer 4.0.0.1 up\n");
  die if $cbgp->send("\texit\n");

  die if $cbgp->send("bgp add router 1 1.0.0.4\n");
  die if $cbgp->send("bgp router 1.0.0.4\n");
  die if $cbgp->send("\tadd peer 1 1.0.0.3\n");
  die if $cbgp->send("\tpeer 1.0.0.3 up\n");
  die if $cbgp->send("\texit\n");

  die if $cbgp->send("bgp add router 1 1.0.0.5\n");
  die if $cbgp->send("bgp router 1.0.0.5\n");
  die if $cbgp->send("\tadd peer 1 1.0.0.2\n");
  die if $cbgp->send("\tpeer 1.0.0.2 up\n");
  die if $cbgp->send("\texit\n");

  die if $cbgp->send("bgp add router 3 3.0.0.1\n");
  die if $cbgp->send("bgp router 3.0.0.1\n");
  die if $cbgp->send("\tadd peer 1 1.0.0.2\n");
  die if $cbgp->send("\tpeer 1.0.0.2 up\n");
  die if $cbgp->send("\texit\n");

  die if $cbgp->send("bgp add router 4 4.0.0.1\n");
  die if $cbgp->send("bgp router 4.0.0.1\n");
  die if $cbgp->send("\tadd peer 1 1.0.0.3\n");
  die if $cbgp->send("\tpeer 1.0.0.3 up\n");
  die if $cbgp->send("\texit\n");

  cbgp_recv_update($cbgp, "1.0.0.1", 1, "2.0.0.1",
		   "255/8|2|IGP|2.0.0.1|0|0");

  die if $cbgp->send("sim run\n");

  my $rib;
  $rib= cbgp_show_rib_mrt($cbgp, "1.0.0.1");
  (!exists($rib->{"255.0.0.0/8"})) and
    return TEST_FAILURE;
  $rib= cbgp_show_rib_mrt($cbgp, "1.0.0.2");
  (!exists($rib->{"255.0.0.0/8"})) and
    return TEST_FAILURE;
  $rib= cbgp_show_rib_custom($cbgp, "1.0.0.3");
  (!exists($rib->{"255.0.0.0/8"}) ||
   ($rib->{"255.0.0.0/8"}->[CBGP_RIB_ORIGINATOR] ne "1.0.0.1") ||
   !clusterlist_equals($rib->{"255.0.0.0/8"}->[CBGP_RIB_CLUSTERLIST],
		       ["1.0.0.2"])) and
			 return TEST_FAILURE;
  $rib= cbgp_show_rib_mrt($cbgp, "1.0.0.4");
  (exists($rib->{"255.0.0.0/8"})) and
    return TEST_FAILURE;
  $rib= cbgp_show_rib_mrt($cbgp, "1.0.0.5");
  (exists($rib->{"255.0.0.0/8"})) and
    return TEST_FAILURE;
  $rib= cbgp_show_rib_custom($cbgp, "3.0.0.1");
  (!exists($rib->{"255.0.0.0/8"}) ||
   !clusterlist_equals($rib->{"255.0.0.0/8"}->[CBGP_RIB_CLUSTERLIST], undef) ||
   defined($rib->{"255.0.0.0/8"}->[CBGP_RIB_ORIGINATOR])) and
     return TEST_FAILURE;
  $rib= cbgp_show_rib_custom($cbgp, "4.0.0.1");
  (!exists($rib->{"255.0.0.0/8"}) ||
   !clusterlist_equals($rib->{"255.0.0.0/8"}->[CBGP_RIB_CLUSTERLIST], undef) ||
   defined($rib->{"255.0.0.0/8"}->[CBGP_RIB_ORIGINATOR])) and
    return TEST_FAILURE;

  return TEST_SUCCESS;
}

# ----[ cbgp_valid_bgp_rr_set_cluster_id ]---------------------------
# Check that the cluster-ID can be defined correctly (including
# large cluster-ids).
# -------------------------------------------------------------------
sub cbgp_valid_bgp_rr_set_cluster_id($)
{
  my ($cbgp)= @_;
  my $ERROR_MSG= "invalid cluster-id";

  cbgp_send($cbgp, "net add node 0.1.0.0");
  cbgp_send($cbgp, "bgp add router 1 0.1.0.0");
  cbgp_send($cbgp, "bgp router 0.1.0.0 set cluster-id 1");
  my $error_msg= cbgp_check_error($cbgp);
  if (defined($error_msg)) {
    return TEST_FAILURE;
  }

  # Maximum value (2^32)-1
  cbgp_send($cbgp, "bgp router 0.1.0.0 set cluster-id 4294967295");
  my $error_msg= cbgp_check_error($cbgp);
  if (defined($error_msg)) {
    return TEST_FAILURE;
  }

  # Larger than (2^32)-1
  cbgp_send($cbgp, "bgp router 0.1.0.0 set cluster-id 4294967296");
  my $error_msg= cbgp_check_error($cbgp);
  if (!defined($error_msg) ||
     !($error_msg =~ /$ERROR_MSG/)) {
    return TEST_FAILURE;
  }

  return TEST_SUCCESS;
}

# -----[ cbgp_valid_bgp_rr_stateful ]--------------------------------
# Test ability to not propagate a route if its attributes have not
# changed. In the case of route-reflectors, this could occur if two
# routes with the same originator-id and cluster-id-list are received,
# and the second one changes the best route. In a stateful BGP
# implementation, no  update should be sent to neighbors after the
# second route is received since the attributes have not changed.
#
# Setup:
#   - R1 (1.0.0.1, AS1), virtual, rr-client
#   - R2 (1.0.0.2, AS1), RR, cluster-id 1
#   - R3 (1.0.0.3, AS1), RR, cluster-id 1
#   - R4 (1.0.0.4, AS1), RR, cluster 1.0.0.4
#   - R5 (1.0.0.5, AS1), rr-client, virtual
#
# Topology:
#
#      *-- R2 --*
#     /          \
#   (R1)          R4 --O-- (R5)
#     \          /     |
#      *-- R3 --*     tap
#
# Scenario:
#   * R1 sends to R3 a route towards 255/8
#   * R3 propagates the route to R4 which selects this route as best
#   * R1 sends to R2 a route towards 255/8 (with same attributes as
#       in [1])
#   * R4 propagates an update to R5
#   * R2 propagates the route to R4 which selects this route as best
#       since the address of R2 is lower than that of R3 (final tie-break)
#   * Check that R4 does not propagate an update to R5
# -------------------------------------------------------------------
sub cbgp_valid_bgp_rr_stateful($)
{
  my ($cbgp)= @_;
  my $mrt_record_file= "/tmp/cbgp-mrt-record-1.0.0.5";

  unlink $mrt_record_file;

  cbgp_send($cbgp, "net add domain 1 igp");
  cbgp_send($cbgp, "net add node 1.0.0.1");
  cbgp_send($cbgp, "net add node 1.0.0.2");
  cbgp_send($cbgp, "net add node 1.0.0.3");
  cbgp_send($cbgp, "net add node 1.0.0.4");
  cbgp_send($cbgp, "net add node 1.0.0.5");
  cbgp_send($cbgp, "net node 1.0.0.1 domain 1");
  cbgp_send($cbgp, "net node 1.0.0.2 domain 1");
  cbgp_send($cbgp, "net node 1.0.0.3 domain 1");
  cbgp_send($cbgp, "net node 1.0.0.4 domain 1");
  cbgp_send($cbgp, "net node 1.0.0.5 domain 1");
  cbgp_send($cbgp, "net add link 1.0.0.1 1.0.0.2 1");
  cbgp_send($cbgp, "net add link 1.0.0.1 1.0.0.3 1");
  cbgp_send($cbgp, "net add link 1.0.0.2 1.0.0.4 1");
  cbgp_send($cbgp, "net add link 1.0.0.3 1.0.0.4 1");
  cbgp_send($cbgp, "net add link 1.0.0.4 1.0.0.5 1");
  cbgp_send($cbgp, "net domain 1 compute");

  cbgp_send($cbgp, "bgp add router 1 1.0.0.2");
  cbgp_send($cbgp, "bgp router 1.0.0.2");
  cbgp_send($cbgp, "\tset cluster-id 1");
  cbgp_send($cbgp, "\tadd peer 1 1.0.0.1");
  cbgp_send($cbgp, "\tpeer 1.0.0.1 rr-client");
  cbgp_send($cbgp, "\tpeer 1.0.0.1 virtual");
  cbgp_send($cbgp, "\tpeer 1.0.0.1 up");
  cbgp_send($cbgp, "\tadd peer 1 1.0.0.4");
  cbgp_send($cbgp, "\tpeer 1.0.0.4 up");
  cbgp_send($cbgp, "\texit");

  cbgp_send($cbgp, "bgp add router 1 1.0.0.3");
  cbgp_send($cbgp, "bgp router 1.0.0.3");
  cbgp_send($cbgp, "\tset cluster-id 1");
  cbgp_send($cbgp, "\tadd peer 1 1.0.0.1");
  cbgp_send($cbgp, "\tpeer 1.0.0.1 rr-client");
  cbgp_send($cbgp, "\tpeer 1.0.0.1 virtual");
  cbgp_send($cbgp, "\tpeer 1.0.0.1 up");
  cbgp_send($cbgp, "\tadd peer 1 1.0.0.4");
  cbgp_send($cbgp, "\tpeer 1.0.0.4 up");
  cbgp_send($cbgp, "\texit");

  cbgp_send($cbgp, "bgp add router 1 1.0.0.4");
  cbgp_send($cbgp, "bgp router 1.0.0.4");
  cbgp_send($cbgp, "\tadd peer 1 1.0.0.2");
  cbgp_send($cbgp, "\tpeer 1.0.0.2 up");
  cbgp_send($cbgp, "\tadd peer 1 1.0.0.3");
  cbgp_send($cbgp, "\tpeer 1.0.0.3 up");
  cbgp_send($cbgp, "\tadd peer 1 1.0.0.5");
  cbgp_send($cbgp, "\tpeer 1.0.0.5 rr-client");
  cbgp_send($cbgp, "\tpeer 1.0.0.5 virtual");
  cbgp_send($cbgp, "\tpeer 1.0.0.5 record $mrt_record_file");
  cbgp_send($cbgp, "\tpeer 1.0.0.5 up");
  cbgp_send($cbgp, "\texit");

  cbgp_recv_update($cbgp, "1.0.0.3", 1, "1.0.0.1",
		   "255/8||IGP|1.0.0.1|0|0");
  cbgp_send($cbgp, "sim run");
  cbgp_recv_update($cbgp, "1.0.0.2", 1, "1.0.0.1",
		   "255/8||IGP|1.0.0.1|0|0");
  cbgp_send($cbgp, "sim run");
  cbgp_checkpoint($cbgp);

  open(MRT_RECORD, "<$mrt_record_file") or
    die "unable to open \"$mrt_record_file\": $!";
  my $line_count= 0;
  while (<MRT_RECORD>) { $line_count++; }
  close(MRT_RECORD);

#  unlink $mrt_record_file;

  if ($line_count != 1) {
    return TEST_FAILURE;
  }

  return TEST_SUCCESS;
}

# -----[ cbgp_valid_bgp_rr_ossld ]------------------------------------
# Test route-reflector ability to avoid redistribution to originator.
#
# Setup:
#   - R1 (1.0.0.1, AS1), rr-client, virtual
#   - R2 (1.0.0.2, AS1)
#   - R3 (1.0.0.3, AS1)
#
#   Note: R2 and R3 are in different clusters (default)
#
# Topology:
#
#   R2 ------ R3
#     \      /
#      \    /
#       (R1)
#
# Scenario:
#   * R1 announces 255/8 to R2
#   * R2 propagates to R3
#   * Check that R3 has received the route and that it does not
#     it send back to R1
# --------------------------------------------------------------------
sub cbgp_valid_bgp_rr_ossld($)
{
  my ($cbgp)= @_;
  my $mrt_record_file= "/tmp/cbgp-mrt-record-1.0.0.1";

  unlink $mrt_record_file;

  cbgp_send($cbgp, "net add domain 1 igp");
  cbgp_send($cbgp, "net add node 1.0.0.1");
  cbgp_send($cbgp, "net add node 1.0.0.2");
  cbgp_send($cbgp, "net add node 1.0.0.3");
  cbgp_send($cbgp, "net node 1.0.0.1 domain 1");
  cbgp_send($cbgp, "net node 1.0.0.2 domain 1");
  cbgp_send($cbgp, "net node 1.0.0.3 domain 1");
  cbgp_send($cbgp, "net add link 1.0.0.1 1.0.0.2 1");
  cbgp_send($cbgp, "net add link 1.0.0.1 1.0.0.3 1");
  cbgp_send($cbgp, "net add link 1.0.0.2 1.0.0.3 1");
  cbgp_send($cbgp, "net domain 1 compute");

  cbgp_send($cbgp, "bgp add router 1 1.0.0.2");
  cbgp_send($cbgp, "bgp router 1.0.0.2");
  cbgp_send($cbgp, "\tadd peer 1 1.0.0.1");
  cbgp_send($cbgp, "\tpeer 1.0.0.1 rr-client");
  cbgp_send($cbgp, "\tpeer 1.0.0.1 virtual");
  cbgp_send($cbgp, "\tpeer 1.0.0.1 up");
  cbgp_send($cbgp, "\tadd peer 1 1.0.0.3");
  cbgp_send($cbgp, "\tpeer 1.0.0.3 up");
  cbgp_send($cbgp, "\texit");

  cbgp_send($cbgp, "bgp add router 1 1.0.0.3");
  cbgp_send($cbgp, "bgp router 1.0.0.3");
  cbgp_send($cbgp, "\tadd peer 1 1.0.0.1");
  cbgp_send($cbgp, "\tpeer 1.0.0.1 rr-client");
  cbgp_send($cbgp, "\tpeer 1.0.0.1 virtual");
  cbgp_send($cbgp, "\tpeer 1.0.0.1 record $mrt_record_file");
  cbgp_send($cbgp, "\tpeer 1.0.0.1 up");
  cbgp_send($cbgp, "\tadd peer 1 1.0.0.2");
  cbgp_send($cbgp, "\tpeer 1.0.0.2 up");
  cbgp_send($cbgp, "\texit");

  cbgp_recv_update($cbgp, "1.0.0.2", 1, "1.0.0.1",
		  "255/8||IGP|1.0.0.1|0|0");
  cbgp_send($cbgp, "sim run");

  my $rib= cbgp_show_rib_mrt($cbgp, "1.0.0.3");
  if (!exists($rib->{"255.0.0.0/8"})) {
    return TEST_FAILURE;
  }

  open(MRT_RECORD, "<$mrt_record_file") or
    die "unable to open \"$mrt_record_file\"";
  my $line_count= 0;
  while (<MRT_RECORD>) { $line_count++; }
  close(MRT_RECORD);

  unlink $mrt_record_file;

  if ($line_count != 0) {
    return TEST_FAILURE;
  }

  return TEST_SUCCESS;
}

# -----[ cbgp_valid_bgp_rr_clssld ]-----------------------------------
# Test route-reflector ability to avoid cluster-id-list loop creation
# from the sender side. In this case, SSLD means "Sender-Side
# cluster-id-list Loop Detection".
#
# Setup:
#
# Scenario:
#
# -------------------------------------------------------------------
sub cbgp_valid_bgp_rr_clssld($)
{
  my ($cbgp)= @_;

  return TEST_NOT_TESTED;
}


# -----[ cbgp_valid_bgp_rr_example ]---------------------------------
# Check that
# - a route received from an rr-client is reflected to all the peers
# - a route received from a non rr-client is only reflected to the
#   rr-clients
# - a route received over eBGP is propagated to all the peers
sub cbgp_valid_bgp_rr_example($)
{
    my ($cbgp)= @_;

    my $topo= topo_star(6);
    cbgp_topo($cbgp, $topo, 1);
    cbgp_topo_igp_compute($cbgp, $topo, 1);
    die if $cbgp->send("bgp add router 1 1.0.0.1\n");
    die if $cbgp->send("bgp router 1.0.0.1\n");
    die if $cbgp->send("\tadd network 255/8\n");
    die if $cbgp->send("\tadd peer 1 1.0.0.2\n");
    die if $cbgp->send("\tpeer 1.0.0.2 up\n");
    die if $cbgp->send("\texit\n");
    die if $cbgp->send("bgp add router 1 1.0.0.2\n");
    die if $cbgp->send("bgp router 1.0.0.2\n");
    die if $cbgp->send("\tadd peer 1 1.0.0.1\n");
    die if $cbgp->send("\tpeer 1.0.0.1 up\n");
    die if $cbgp->send("\tadd peer 1 1.0.0.3\n");
    die if $cbgp->send("\tpeer 1.0.0.3 rr-client\n");
    die if $cbgp->send("\tpeer 1.0.0.3 up\n");
    die if $cbgp->send("\tadd peer 1 1.0.0.4\n");
    die if $cbgp->send("\tpeer 1.0.0.4 up\n");
    die if $cbgp->send("\tadd peer 2 1.0.0.5\n");
    die if $cbgp->send("\tpeer 1.0.0.5 up\n");
    die if $cbgp->send("\tadd peer 1 1.0.0.6\n");
    die if $cbgp->send("\tpeer 1.0.0.6 rr-client\n");
    die if $cbgp->send("\tpeer 1.0.0.6 up\n");
    die if $cbgp->send("\texit\n");
    die if $cbgp->send("bgp add router 1 1.0.0.3\n");
    die if $cbgp->send("bgp router 1.0.0.3\n");
    die if $cbgp->send("\tadd peer 1 1.0.0.2\n");
    die if $cbgp->send("\tpeer 1.0.0.2 up\n");
    die if $cbgp->send("\texit\n");
    die if $cbgp->send("bgp add router 1 1.0.0.4\n");
    die if $cbgp->send("bgp router 1.0.0.4\n");
    die if $cbgp->send("\tadd peer 1 1.0.0.2\n");
    die if $cbgp->send("\tpeer 1.0.0.2 up\n");
    die if $cbgp->send("\texit\n");
    die if $cbgp->send("bgp add router 2 1.0.0.5\n");
    die if $cbgp->send("bgp router 1.0.0.5\n");
    die if $cbgp->send("\tadd network 254/8\n");
    die if $cbgp->send("\tadd peer 1 1.0.0.2\n");
    die if $cbgp->send("\tpeer 1.0.0.2 up\n");
    die if $cbgp->send("\texit\n");
    die if $cbgp->send("bgp add router 1 1.0.0.6\n");
    die if $cbgp->send("bgp router 1.0.0.6\n");
    die if $cbgp->send("\tadd network 253/8\n");
    die if $cbgp->send("\tadd peer 1 1.0.0.2\n");
    die if $cbgp->send("\tpeer 1.0.0.2 up\n");
    die if $cbgp->send("\texit\n");
    die if $cbgp->send("sim run\n");
    my $rib;
    # Peer 1.0.0.1 must have received all routes
    $rib= cbgp_show_rib_mrt($cbgp, "1.0.0.1");
    (cbgp_rib_check($rib, "255/8") &&
     cbgp_rib_check($rib, "254/8") &&
     cbgp_rib_check($rib, "253/8")) or return TEST_FAILURE;
    # RR must have received all routes
    $rib= cbgp_show_rib_mrt($cbgp, "1.0.0.2");
    (cbgp_rib_check($rib, "255/8") &&
     cbgp_rib_check($rib, "254/8") &&
     cbgp_rib_check($rib, "253/8")) or return TEST_FAILURE;
    # RR-client 1.0.0.3 must have received all routes
    $rib= cbgp_show_rib_mrt($cbgp, "1.0.0.3");
    (cbgp_rib_check($rib, "255/8") &&
     cbgp_rib_check($rib, "254/8") &&
     cbgp_rib_check($rib, "253/8")) or return TEST_FAILURE;
    # Peer 1.0.0.4 must only have received the 254/8 and 253/8 routes
    $rib= cbgp_show_rib_mrt($cbgp, "1.0.0.4");
    (!cbgp_rib_check($rib, "255/8") &&
     cbgp_rib_check($rib, "254/8") &&
     cbgp_rib_check($rib, "253/8")) or return TEST_FAILURE;
    # External peer 1.0.0.5 must have received all routes
    $rib= cbgp_show_rib_mrt($cbgp, "1.0.0.5");
    (cbgp_rib_check($rib, "255/8") &&
     cbgp_rib_check($rib, "254/8") &&
     cbgp_rib_check($rib, "253/8")) or return TEST_FAILURE;
    # RR-client 1.0.0.6 must have received all routes
    $rib= cbgp_show_rib_mrt($cbgp, "1.0.0.6");
    (cbgp_rib_check($rib, "255/8") &&
     cbgp_rib_check($rib, "254/8") &&
     cbgp_rib_check($rib, "253/8")) or return TEST_FAILURE;
    return TEST_SUCCESS;
}

# -----[ test_set_result ]-------------------------------------------
sub test_set_result($$;$)
{
  my ($test_id, $result, $duration)= @_;

  my $test= $validation->{'tests_list'}->[$test_id];
  $test->[TEST_FIELD_RESULT]= $result;
  (defined($duration)) and
    $test->[TEST_FIELD_DURATION]= $duration;
}


#####################################################################
#
# MAIN
#
#####################################################################
my $topo= topo_3nodes_triangle();

# -----[ LIST OF TESTS ]---------------------------------------------
# All the C-BGP tests have to be registered here. The syntax is as
# follows:
#
#   $tests->register(<name>, \&<function> [, <args>]);
#
# -----------------------------------------------------------------
$tests->register("show version", "cbgp_valid_version");
$tests->register("net node", "cbgp_valid_net_node");
$tests->register("net node duplicate", "cbgp_valid_net_node_duplicate");
$tests->register("net link", "cbgp_valid_net_link", $topo);
$tests->register("net link loop", "cbgp_valid_net_link_loop");
$tests->register("net link duplicate", "cbgp_valid_net_link_duplicate");
$tests->register("net subnet", "cbgp_valid_net_subnet", $topo);
$tests->register("net create", "cbgp_valid_net_create", $topo);
$tests->register("net igp", "cbgp_valid_net_igp", $topo);
$tests->register("net igp unknown domain", "cbgp_valid_net_igp_unknown_domain");
$tests->register("net ntf load", "cbgp_valid_net_ntf_load",
		 $resources_path."valid-record-route.ntf");
$tests->register("net record-route", "cbgp_valid_net_record_route", $topo);
$tests->register("net static routes", "cbgp_valid_net_static_routes", $topo);
$tests->register("net longest-matching", "cbgp_valid_net_longest_matching");
$tests->register("net protocol priority", "cbgp_valid_net_protocol_priority");
$tests->register("bgp options show-mode cisco",
		 "cbgp_valid_bgp_options_showmode_cisco");
$tests->register("bgp options show-mode mrt",
		 "cbgp_valid_bgp_options_showmode_mrt");
$tests->register("bgp options show-mode custom",
		 "cbgp_valid_bgp_options_showmode_custom");
$tests->register("bgp topology load", "cbgp_valid_bgp_topology_load");
$tests->register("bgp topology policies", "cbgp_valid_bgp_topology_policies");
$tests->register("bgp topology run", "cbgp_valid_bgp_topology_run");
$tests->register("bgp session ibgp", "cbgp_valid_bgp_session_ibgp");
$tests->register("bgp session ebgp", "cbgp_valid_bgp_session_ebgp");
$tests->register("bgp as-path loop", "cbgp_valid_bgp_aspath_loop");
$tests->register("bgp ssld", "cbgp_valid_bgp_ssld");
$tests->register("bgp session state-machine", "cbgp_valid_bgp_session_sm");
$tests->register("bgp domain full-mesh",
	      "cbgp_valid_bgp_domain_fullmesh", $topo);
$tests->register("bgp peering up/down", "cbgp_valid_bgp_peering_up_down");
$tests->register("bgp router up/down", "cbgp_valid_bgp_router_up_down");
$tests->register("bgp peering next-hop-self",
	      "cbgp_valid_bgp_peering_nexthopself");
$tests->register("bgp virtual peer", "cbgp_valid_bgp_virtual_peer");
$tests->register("bgp recv mrt", "cbgp_valid_bgp_recv_mrt");
$tests->register("bgp as-path as_set", "cbgp_valid_bgp_aspath_as_set");
$tests->register("bgp soft-restart", "cbgp_valid_bgp_soft_restart");
$tests->register("bgp decision-process local-pref",
	      "cbgp_valid_bgp_dp_localpref");
$tests->register("bgp decision process as-path", "cbgp_valid_bgp_dp_aspath");
$tests->register("bgp decision process origin", "cbgp_valid_bgp_dp_origin");
$tests->register("bgp decision process med", "cbgp_valid_bgp_dp_med");
$tests->register("bgp decision process ebgp vs ibgp",
	      "cbgp_valid_bgp_dp_ebgp_ibgp");
$tests->register("bgp decision process igp", "cbgp_valid_bgp_dp_igp");
$tests->register("bgp decision process router-id",
	      "cbgp_valid_bgp_dp_router_id");
$tests->register("bgp decision process neighbor address",
	      "cbgp_valid_bgp_dp_neighbor_address");
$tests->register("bgp stateful", "cbgp_valid_bgp_stateful");
$tests->register("bgp filter action deny", "cbgp_valid_bgp_filter_action_deny");
$tests->register("bgp filter action community add",
	      "cbgp_valid_bgp_filter_action_community_add");
$tests->register("bgp filter action community remove",
	      "cbgp_valid_bgp_filter_action_community_remove");
$tests->register("bgp filter action community strip",
	      "cbgp_valid_bgp_filter_action_community_strip");
$tests->register("bgp filter action local-pref",
	      "cbgp_valid_bgp_filter_action_localpref");
$tests->register("bgp filter action med",
	      "cbgp_valid_bgp_filter_action_med");
$tests->register("bgp filter action med internal",
	      "cbgp_valid_bgp_filter_action_med_internal");
$tests->register("bgp filter action as-path",
	      "cbgp_valid_bgp_filter_action_aspath");
$tests->register("bgp filter match community",
	      "cbgp_valid_bgp_filter_match_community");
$tests->register("bgp filter match as-path regex",
	      "cbgp_valid_bgp_filter_match_aspath_regex");
$tests->register("bgp filter match next-hop",
	      "cbgp_valid_bgp_filter_match_nexthop");
$tests->register("bgp filter match prefix is",
	      "cbgp_valid_bgp_filter_match_prefix_is");
$tests->register("bgp filter match prefix in",
	      "cbgp_valid_bgp_filter_match_prefix_in");
$tests->register("bgp filter match prefix in-length",
	      "cbgp_valid_bgp_filter_match_prefix_in_length");
$tests->register("bgp route-map", "cbgp_valid_bgp_route_map");
$tests->register("bgp route-map duplicate",
		 "cbgp_valid_bgp_route_map_duplicate");
$tests->register("igp-bgp metric change", "cbgp_valid_igp_bgp_metric_change");
$tests->register("igp-bgp state change", "cbgp_valid_igp_bgp_state_change");
#$tests->register("igp-bgp reachability link",
#              "cbgp_valid_igp_bgp_reach_link");
#$tests->register("igp-bgp reachability subnet",
#	       "cbgp_valid_igp_bgp_reach_subnet");
$tests->register("igp-bgp update med", "cbgp_valid_igp_bgp_med");
$tests->register("bgp load rib", "cbgp_valid_bgp_load_rib");
$tests->register("bgp deflection", "cbgp_valid_bgp_deflection");
$tests->register("bgp RR", "cbgp_valid_bgp_rr");
$tests->register("bgp RR set cluster-id",
		 "cbgp_valid_bgp_rr_set_cluster_id");
$tests->register("bgp RR decision process (originator-id)",
	      "cbgp_valid_bgp_rr_dp_originator_id");
$tests->register("bgp RR decision process (cluster-id-list)",
	      "cbgp_valid_bgp_rr_dp_cluster_id_list");
$tests->register("bgp RR stateful",
	      "cbgp_valid_bgp_rr_stateful");
$tests->register("bgp RR originator ssld",
	      "cbgp_valid_bgp_rr_ossld");
$tests->register("bgp RR cluster-id-list ssld",
	      "cbgp_valid_bgp_rr_clssld");
$tests->register("bgp implicit-withdraw",
	      "cbgp_valid_bgp_implicit_withdraw");

my $return_value= 0;
if ($tests->run() > 0) {
  show_error("".$tests->{'num-failures'}." test(s) failed.");
  $return_value= -1;
} else {
  show_info("all tests passed.");
}

if (exists($opts{report})) {
  if ($opts{report} eq "html") {
    CBGPValid::HTMLReport::report_write("$report_prefix",
					$validation,
					$tests);
    CBGPValid::BaseReport::save_resources("$report_prefix.resources");
  }
}

show_info("done.");
exit($return_value);

