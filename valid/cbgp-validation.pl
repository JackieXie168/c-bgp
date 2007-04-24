#!/usr/bin/perl -w
# ===================================================================
# @(#)cbgp-validation.pl
#
# This script is used to validate C-BGP. It performs various tests in
# order to detect erroneous behaviour.
#
# @author Bruno Quoitin (bqu@info.ucl.ac.be)
# @lastdate 23/04/2007
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
#                   The script currently only support HTML and XML
#                   formats, so use --report=html or --report=xml.
#                   The report is composed of two files/pages. The
#                   first one contains a summary table with all tests
#                   results. The second one contains the description
#                   of each test. The description is automatically
#                   extracted from the test Perl functions.
#
# --report-prefix=PREFIX
#                   Changes the prefix of the report file names. The
#                   result page will be called "PREFIX.html" or
#                   "PREFIX.xml" (depending on the reports format)
#                   while the documentation page will be called
#                   "PREFIX-doc.html" or "PREFIX-doc.xml".
#
# --resources-path=PATH
#                   Set the path to the resources needed by the
#                   validation script. The needed resources are the
#                   topology files, the BGP RIB dumps, and so on.
#                   Usually, this path must be set to the installation
#                   directory of the validation script.
#
# ===================================================================
# Contributions:
#   - Sebastien Tandel: record-route loop (March 2007)
# ===================================================================

use strict;

use Getopt::Long;
use CBGP;
use CBGPValid::BaseReport;
use CBGPValid::HTMLReport;
use CBGPValid::TestConstants;
use CBGPValid::Tests;
use CBGPValid::UI;
use CBGPValid::XMLReport;
use POSIX;

use constant CBGP_VALIDATION_VERSION => '1.9.0';

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
use constant CBGP_TR_NUMHOPS => 3;
use constant CBGP_TR_PATH => 4;
use constant CBGP_TR_WEIGHT => 5;
use constant CBGP_TR_DELAY => 6;
use constant CBGP_TR_CAPACITY => 7;

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

# -----[ Interface fields ]-----
use constant CBGP_IFACE_TYPE => 0;
use constant CBGP_IFACE_LOCAL => 1;
use constant CBGP_IFACE_DEST => 2;

# -----[ Ping fields ]-----
use constant CBGP_PING_ADDR   => 0;
use constant CBGP_PING_STATUS => 1;

# -----[ PING status ]-----
use constant CBGP_PING_STATUS_REPLY     => 'reply';
use constant CBGP_PING_STATUS_NO_REPLY  => 'no reply';
use constant CBGP_PING_STATUS_ICMP_TTL  => 'icmp error (time-exceeded)';
use constant CBGP_PING_STATUS_ICMP_NET  => 'icmp error (network-unreachable)';
use constant CBGP_PING_STATUS_ICMP_DST  => 'icmp error (destination-unreachable)';
use constant CBGP_PING_STATUS_ICMP_PORT => 'icmp error (port-unreachable)';
use constant CBGP_PING_STATUS_NET       => 'network unreachable';
use constant CBGP_PING_STATUS_DST       => 'destination unreachable';

# -----[ Traceroute fields ]-----
use constant CBGP_TRACEROUTE_STATUS => 0;
use constant CBGP_TRACEROUTE_NHOPS  => 1;
use constant CBGP_TRACEROUTE_HOPS   => 2;

# -----[ Traceroute status ]-----
use constant CBGP_TRACEROUTE_STATUS_REPLY    => 'reply';
use constant CBGP_TRACEROUTE_STATUS_NO_REPLY => 'no reply';
use constant CBGP_TRACEROUTE_STATUS_ERROR    => 'error';

# -----[ Error messages ]-----
my $CBGP_ERROR_INVALID_SUBNET   = "invalid subnet";
my $CBGP_ERROR_CLUSTERID_INVALID= "invalid cluster-id";
my $CBGP_ERROR_DOMAIN_UNKNOWN   = "unknown domain";
my $CBGP_ERROR_LINK_EXISTS      = "link already exists";
my $CBGP_ERROR_LINK_ENDPOINTS   = "link endpoints are equal";
my $CBGP_ERROR_NODE_EXISTS      = "node already exists";
my $CBGP_ERROR_ROUTE_BAD_IFACE  = "interface is unknown";
my $CBGP_ERROR_ROUTE_EXISTS     = "route already exists";
my $CBGP_ERROR_ROUTE_NH_UNREACH = "next-hop is unreachable";
my $CBGP_ERROR_ROUTEMAP_EXISTS  = "route-map already exists";
my $CBGP_ERROR_SUBNET_EXISTS    = "already exists";

use constant MAX_IGP_WEIGHT => 4294967295;

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
		 'libgds_version' => undef,
		 'program_args' => (join " ", @ARGV),
		 'program_name' => $0,
		 'program_version' => CBGP_VALIDATION_VERSION,
		};

show_info("c-bgp validation v".$validation->{'program_version'});
show_info("(c) 2006, 2007, Bruno Quoitin (bqu\@info.ucl.ac.be)");

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
    if ($cbgp->send("$cmd\n")) {
      show_error("Error: could not send to CBGP instance");
      die "cbgp_send() failed";
    }
  }

# -----[ cbgp_checkpoint ]-------------------------------------------
# Wait until C-BGP has finished previous treatment...
# -------------------------------------------------------------------
sub cbgp_checkpoint($)
  {
    my ($cbgp)= @_;
    cbgp_send($cbgp, "print \"CHECKPOINT\\n\"");
    while ((my $result= $cbgp->expect(1)) ne "CHECKPOINT") {
      sleep(1);
    }
  }

# -----[ cbgp_check_error ]------------------------------------------
# Check if C-BGP has returned an error message.
#
# Return values:
#   undef      -> no error message was issued
#   <message>  -> error message returned by C-BGP
# -------------------------------------------------------------------
sub cbgp_check_error($$)
  {
    my ($cbgp, $cmd)= @_;
    my $result= "";

    cbgp_send($cbgp, "set exit-on-error off");
    cbgp_send($cbgp, $cmd);
    cbgp_send($cbgp, "print \"CHECKPOINT\\n\"");
    while ((my $line= $cbgp->expect(1)) ne "CHECKPOINT") {
      $result.= $line;
    }
    if ($result =~ m/^Error\:(.*)$/) {
      return $1;
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

    cbgp_send($cbgp, "net add node 1.0.0.1");
    cbgp_send($cbgp, "bgp add router 1 1.0.0.1");
    for (my $index= 0; $index < $num_peers; $index++) {
      my $asn= $index+2;
      my $peer= "$asn.0.0.1";
      cbgp_send($cbgp, "net add node $peer");
      cbgp_send($cbgp, "net add link 1.0.0.1 $peer 0");
      cbgp_send($cbgp, "net node 1.0.0.1 route add $peer/32 * $peer 0");
      cbgp_peering($cbgp, "1.0.0.1", $peer, $asn, "virtual");
    }
  }

# -----[ cbgp_topo_dp2 ]---------------------------------------------
sub cbgp_topo_dp2($$)
  {
    my ($cbgp, $num_peers)= @_;

    cbgp_send($cbgp, "net add node 1.0.0.1");
    cbgp_send($cbgp, "bgp add router 1 1.0.0.1");
    for (my $index= 0; $index < $num_peers; $index++) {
      my $peer= "1.0.0.".($index+2);
      cbgp_send($cbgp, "net add node $peer");
      cbgp_send($cbgp, "net add link 1.0.0.1 $peer ".($index+1)."");
      cbgp_send($cbgp, "net node 1.0.0.1 route add $peer/32 * $peer 0");
      cbgp_peering($cbgp, "1.0.0.1", $peer, "1", "virtual");
    }
  }

# -----[ cbgp_topo_dp3 ]---------------------------------------------
sub cbgp_topo_dp3($;@)
  {
    my ($cbgp, @peers)= @_;

    cbgp_send($cbgp, "net add node 1.0.0.1");
    cbgp_send($cbgp, "bgp add router 1 1.0.0.1");
    for (my $index= 0; $index < @peers; $index++) {
      my ($peer, $asn, $weight)= @{$peers[$index]};
      cbgp_send($cbgp, "net add node $peer\n");
      cbgp_send($cbgp, "net add link 1.0.0.1 $peer $weight\n");
      cbgp_send($cbgp, "net node 1.0.0.1 route add $peer/32 * $peer $weight\n");
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

    cbgp_send($cbgp, "net add node 1.0.0.1");
    cbgp_send($cbgp, "bgp add router 1 1.0.0.1");
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

    cbgp_send($cbgp, "net add domain 1 igp");
    cbgp_send($cbgp, "net add node 1.0.0.1");
    cbgp_send($cbgp, "net node 1.0.0.1 domain 1");

    foreach my $peer_spec (@_) {
      my $peer_ip= $peer_spec->[0];
      my $peer_weight= $peer_spec->[1];
      cbgp_send($cbgp, "net add node $peer_ip");
      cbgp_send($cbgp, "net node $peer_ip domain 1");
      cbgp_send($cbgp, "net add link 1.0.0.1 $peer_ip $peer_weight");
      cbgp_send($cbgp, "net link 1.0.0.1 $peer_ip igp-weight $peer_weight");
      cbgp_send($cbgp, "net link $peer_ip 1.0.0.1 igp-weight $peer_weight");
    }
    cbgp_send($cbgp, "net domain 1 compute");

    cbgp_send($cbgp, "net add node 2.0.0.1");

    cbgp_send($cbgp, "net add link 1.0.0.1 2.0.0.1 0");
    cbgp_send($cbgp, "net node 1.0.0.1 route add 2.0.0.1/32 * 2.0.0.1 0");
    cbgp_send($cbgp, "net node 2.0.0.1 route add 1.0.0.1/32 * 1.0.0.1 0");

    cbgp_send($cbgp, "bgp add router 1 1.0.0.1");
    cbgp_send($cbgp, "bgp router 1.0.0.1");
    foreach my $peer_spec (@_) {
      my $peer_ip= $peer_spec->[0];
      cbgp_send($cbgp, "\tadd peer 1 $peer_ip");
      cbgp_send($cbgp, "\tpeer $peer_ip virtual");
      for (my $index= 2; $index < scalar(@$peer_spec); $index++) {
	my $peer_option= $peer_spec->[$index];
	cbgp_send($cbgp, "\tpeer $peer_ip $peer_option");
      }
      cbgp_send($cbgp, "\tpeer $peer_ip up");
    }
    cbgp_send($cbgp, "\tadd peer 2 2.0.0.1");
    cbgp_send($cbgp, "\tpeer 2.0.0.1 up");
    cbgp_send($cbgp, "\texit");

    cbgp_send($cbgp, "bgp add router 2 2.0.0.1");
    cbgp_send($cbgp, "bgp router 2.0.0.1");
    cbgp_send($cbgp, "\tadd peer 1 1.0.0.1");
    cbgp_send($cbgp, "\tpeer 1.0.0.1 up");
    cbgp_send($cbgp, "\texit");
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
# Check that record-routes performed on the given topology correctly
# record the forwarding path. The function can optionaly check that
# the IGP weight and delay of the path are correctly recorded.
#
# Arguments:
#   $cbgp  : CBGP instance
#   $topo  : topology to test
#   $status: expected result (SUCCESS, UNREACH, ...)
#   $delay : true => check that IGP weight and delay are correcly
#            recorded (optional)
#
# Return value:
#   1 success
#   0 failure
# -------------------------------------------------------------------
sub cbgp_topo_check_record_route($$$;$)
  {
    my ($cbgp, $topo, $status, $delay)= @_;
    my $nodes= topo_get_nodes($topo);

    $tests->debug("topo_check_record_route()");

    foreach my $node1 (keys %$nodes) {
      foreach my $node2 (keys %$nodes) {
	($node1 eq $node2) and next;
	my $trace;
	if ($delay) {
	  $trace= cbgp_net_record_route($cbgp, $node1, $node2,
					-delay=>1, -weight=>1);
	} else {
	  $trace= cbgp_net_record_route($cbgp, $node1, $node2);
	}

	# Result should be equal to the given status
	if ($trace->[CBGP_TR_STATUS] ne $status) {
	  $tests->debug("ERROR incorrect status \"".
			$trace->[CBGP_TR_STATUS].
			"\" (expected=\"$status\")");
	  return 0;
	}

	# Check that destination matches in case of success
	my @path= @{$trace->[CBGP_TR_PATH]};
	if ($trace->[CBGP_TR_STATUS] eq "SUCCESS") {
	  # Last hop should be destination
	  $tests->debug(join ", ", @path);
	  if ($path[$#path] ne $node2) {
	    $tests->debug("ERROR final node != destination ".
			  "(\"".$path[$#path]."\" != \"$node2\")");
	    return 0;
	  }
	}

	# Optionaly record/check IGP weight and delay
	if (defined($delay) && $delay) {
	  my $delay= 0;
	  my $weight= 0;
	  my $prev_hop= undef;
	  foreach my $hop (@path) {
	    if (defined($prev_hop)) {
	      if (exists($topo->{$prev_hop}{$hop})) {
		$delay+= $topo->{$prev_hop}{$hop}->[0];
		$weight+= $topo->{$prev_hop}{$hop}->[1];
	      } elsif (exists($topo->{$hop}{$prev_hop})) {
		$delay+= $topo->{$hop}{$prev_hop}->[0];
		$weight+= $topo->{$hop}{$prev_hop}->[1];
	      } else {
		return 0;
	      }
	    }
	    $prev_hop= $hop;
	  }
	  $tests->debug("($delay, $weight) vs ($trace->[CBGP_TR_DELAY], $trace->[CBGP_TR_WEIGHT])"); 
	  if ($delay != $trace->[CBGP_TR_DELAY]) {
	    $tests->debug("delay mismatch ($delay vs $trace->[CBGP_TR_DELAY])");
	    return 0;
	  }
	  if ($weight != $trace->[CBGP_TR_WEIGHT]) {
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
	my $trace= cbgp_net_record_route($cbgp, $node1,
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

    cbgp_send($cbgp, "bgp router $router");
    cbgp_send($cbgp, "\tpeer $peer");
    cbgp_send($cbgp, "\t\tfilter $direction");
    cbgp_send($cbgp, "\t\t\tadd-rule");
    cbgp_send($cbgp, "\t\t\t\tmatch \"$predicate\"");
    cbgp_send($cbgp, "\t\t\t\taction \"$actions\"");
    cbgp_send($cbgp, "\t\t\t\texit");
    cbgp_send($cbgp, "\t\t\texit");
    cbgp_send($cbgp, "\t\texit");
    cbgp_send($cbgp, "\texit");
  }

# -----[ cbgp_recv_update ]------------------------------------------
sub cbgp_recv_update($$$$$)
  {
    my ($cbgp, $router, $asn, $peer, $msg)= @_;

    cbgp_send($cbgp, "bgp router $router peer $peer recv ".
	      "\"BGP4|0|A|$router|$asn|$msg\"");
  }

# -----[ cbgp_recv_withdraw ]----------------------------------------
sub cbgp_recv_withdraw($$$$$)
  {
    my ($cbgp, $router, $asn, $peer, $prefix)= @_;

    cbgp_send($cbgp, "bgp router $router peer $peer recv ".
	      "\"BGP4|0|W|$router|$asn|$prefix\"");
  }

# -----[ cbgp_show_links ]-------------------------------------------
sub cbgp_show_links($$)
  {
    my ($cbgp, $node)= @_;
    my %links= ();

    cbgp_send($cbgp, "net node $node show links");
    cbgp_send($cbgp, "print \"done\\n\"");
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
	show_error("invalid delay (show links): $fields[2]");
	exit(-1);
      }
      if (!($fields[3] =~ m/^[0-9]+$/)) {
	show_error("invalid weight (show links): $fields[3]");
	exit(-1);
      }
      if (!($fields[4] =~ m/^UP|DOWN$/)) {
	show_error("invalid state (show links): $fields[4]");
	exit(-1);
      }

      my @link= ();
      $link[CBGP_LINK_TYPE]= $fields[0];
      $link[CBGP_LINK_DST]= $fields[1];
      $link[CBGP_LINK_DELAY]= $fields[2];
      $link[CBGP_LINK_WEIGHT]= $fields[3];
      $link[CBGP_LINK_STATE]= $fields[4];

      if (exists($links{$link[CBGP_LINK_DST]})) {
	show_error("duplicate link destination: ".$link[CBGP_LINK_DST]."");
	exit(-1);
      }

      $links{$link[CBGP_LINK_DST]}= \@link;
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

    cbgp_send($cbgp, "net node $node show rt $destination");
    cbgp_send($cbgp, "print \"done\\n\"");
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

    cbgp_send($cbgp, "bgp router $node show peers");
    cbgp_send($cbgp, "print \"done\\n\"");
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

# -----[ cbgp_show_ifaces ]------------------------------------------
sub cbgp_show_ifaces($$)
  {
    my ($cbgp, $node)= @_;
    my %ifaces;

    cbgp_send($cbgp, "net node $node show ifaces");
    cbgp_send($cbgp, "print \"done\\n\"");
    while ((my $result= $cbgp->expect(1)) ne "done") {
      my @fields= split /\s+/, $result;
      my $id;
      if ($fields[0] eq "lo") {
	(scalar(@fields) != 2) and return undef;
	$id= $fields[1];
      } elsif (($fields[0] eq "ptp") || ($fields[0] eq "ptmp")) {
	(scalar(@fields) != 3) and return undef;
	(!($fields[2] =~ m/^([0-9.\/]+)$/)) and return undef;
	$id= $fields[1].'_'.$fields[2];
      } else {
	return undef;
      }
      (!($fields[1] =~ m/^([0-9.]+)$/)) and
	return undef;
      my @iface= ();
      $iface[CBGP_IFACE_TYPE]= $fields[0];
      $iface[CBGP_IFACE_LOCAL]= $fields[1];
      $iface[CBGP_IFACE_DEST]= $fields[2];
      $ifaces{$id}= \@iface;
    }
    return \%ifaces;
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

# -----[ cbgp_net_node_info ]----------------------------------------
sub cbgp_net_node_info($$)
  {
    my ($cbgp, $node)= @_;
    my %info;

    cbgp_send($cbgp, "net node $node show info");
    cbgp_send($cbgp, "print \"done\\n\"");
    while ((my $result= $cbgp->expect(1)) ne "done") {
      if (!($result =~ m/^([a-z]+)\s*\:\s*(.*)$/)) {
	show_error("incorrect format (net node show info): \"$result\"");
	exit(-1);
      }
      $info{$1}= $2;
    }
    return \%info;
  }

# -----[ cbgp_net_link_info ]----------------------------------------
sub cbgp_net_link_info($$$)
  {
    my ($cbgp, $src, $dst)= @_;
    my %info;

    cbgp_send($cbgp, "net link $src $dst show info");
    cbgp_send($cbgp, "print \"done\\n\"");
    while ((my $result= $cbgp->expect(1)) ne "done") {
      if (!($result =~ m/^([a-z]+)\s*\:\s*(.*)$/)) {
	show_error("incorrect format (net link show info): \"$result\"");
	exit(-1);
      }
      $info{$1}= $2;
    }
    return \%info;
  }

# -----[ cbgp_bgp_router_info ]--------------------------------------
sub cbgp_bgp_router_info($$)
  {
    my ($cbgp, $router)= @_;
    my %info;

    cbgp_send($cbgp, "bgp router $router show info");
    cbgp_send($cbgp, "print \"done\\n\"");
    while ((my $result= $cbgp->expect(1)) ne "done") {
      if (!($result =~ m/^([a-z]+)\s*\:\s*(.*)$/)) {
	show_error("incorrect format (bgp router show info): \"$result\"");
	exit(-1);
      }
      $info{$1}= $2;
    }
    return \%info;
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

# -----[ cbgp_net_record_route ]-------------------------------------
# Record the route from a source node to a destination node or
# prefix. The function can optionaly request that the route's IGP
# weight and delay be recorded.
#
# Arguments:
#   $cbgp : CBGP instance
#   $src  : IP address of source node
#   $dst  : IP address or IP prefix
#   %args:
#     -capacity=>1  : record max-capacity
#     -checkloop=>1 : check for forwarding loop
#     -delay=>1     : record delay
#     -load=>LOAD   : load path with LOAD
#     -weight=>1    : record IGP weight
# -------------------------------------------------------------------
sub cbgp_net_record_route($$$;%)
  {
    my ($cbgp, $src, $dst, %args)= @_;
    my $options= "";
    my @trace;

    (exists($args{-capacity})) and $options.= "--capacity ";
    (exists($args{'-checkloop'})) and $options.= "--check-loop ";
    (exists($args{-delay})) and $options.= "--delay ";
    (exists($args{-weight})) and $options.= "--weight ";
    (exists($args{-load})) and $options.= "--load=".$args{-load};

    cbgp_send($cbgp, "net node $src record-route $options $dst");
    my $result= $cbgp->expect(1);
    my @fields= split /\s/, $result;
    if (scalar(@fields) < 4) {
      show_error("not enough fields (record-route): \"$result\"");
      return undef;
    }
    if (!($fields[0] =~ m/^[0-9.]+$/)) {
      show_error("invalid source (record-route): \"$result\"");
      return undef;
    }
    if (!($fields[1] =~ m/^[0-9.\/]+$/)) {
      show_error("invalid destination (record-route): \"$result\"");
      return undef;
    }
    if (!($fields[2] =~ m/^[A-Z_]+$/)) {
      show_error("invalid status (record-route): \"$result\"");
      return undef;
    }
    if (!($fields[3] =~ m/^[0-9]+$/)) {
      show_error("invalid number of hops (record-route): \"$result\"");
      return undef;
    }
    $trace[CBGP_TR_SRC]= $fields[0];
    $trace[CBGP_TR_DST]= $fields[1];
    $trace[CBGP_TR_STATUS]= $fields[2];
    $trace[CBGP_TR_NUMHOPS]= $fields[3];

    # Parse paths
    my $field_index= 4;
    my @path= ();
    $trace[CBGP_TR_PATH]= \@path;
    for (my $index= 0; $index < $trace[CBGP_TR_NUMHOPS]; $index++) {
      if (($field_index >= scalar(@fields)) ||
	  !($fields[$field_index] =~ m/^[0-9.]+$/)) {
	show_error("invalid hop[$index] (record-route): \"$result\"");
	return undef;
      }
      push @path, ($fields[$field_index++]);
    }

    # Parse options
    my %options;
    while ($field_index < scalar(@fields)) {
      if (!($fields[$field_index] =~ m/^([a-z]+)\:(.+)$/)) {
	show_error("invalid option (record-route): \"$result\"");
	return undef;
      }
      $options{$1}= $2;
      $field_index++;
    }

    if (exists($args{-capacity})) {
      if (!exists($options{capacity})) {
	show_error("missing option \"capacity\" (record-route): \"$result\"");
	return undef;
      }
      $trace[CBGP_TR_CAPACITY]= $options{capacity};
    }
    if (exists($args{-delay})) {
      if (!exists($options{delay})) {
	show_error("missing option \"delay\" (record-route): \"$result\"");
	return undef;
      }
      $trace[CBGP_TR_DELAY]= $options{delay};
    }
    if (exists($args{-weight})) {
      if (!exists($options{weight})) {
	show_error("missing option \"weight\" (record-route): \"$result\"");
	return undef;
      }
      $trace[CBGP_TR_WEIGHT]= $options{weight};
    }

    return \@trace;
  }

# -----[ cbgp_net_ping ]---------------------------------------------
# Ping a destination from a source node.
#
# Arguments:
#   $cbgp : CBGP instance
#   $src  : IP address of source node
#   $dst  : IP address of destination node
#   %args:
#     -ttl=>TTL  : set time-to-live to TTL
# -------------------------------------------------------------------
sub cbgp_net_ping($$$;%)
  {
    my ($cbgp, $src, $dst, %args)= @_;
    my $options= "";
    (exists($args{-ttl})) and $options.= "--ttl=".$args{-ttl};

    cbgp_send($cbgp, "net node $src ping $options $dst");
    my $result= $cbgp->expect(1);
    if ($result =~ m/^ping\:\s+([a-z\-\(\) ]+)$/) {
      my @ping;
      $ping[CBGP_PING_ADDR]= undef;
      $ping[CBGP_PING_STATUS]= $1;
      return \@ping;
    } elsif ($result =~ m/^ping:\s+(.+)\s+from\s+([0-9.]+)$/) {
      my @ping;
      $ping[CBGP_PING_ADDR]= $2;
      $ping[CBGP_PING_STATUS]= $1;
      return \@ping;
    } else {
      show_error("incorrect format (net ping): \"$result\"");
      exit(-1);
    }
  }

# -----[ cbgp_net_traceroute ]---------------------------------------
# Traceroute a destination from a source node.
#
# Arguments:
#   $cbgp : CBGP instance
#   $src  : IP address of source node
#   $dst  : IP address of destination node
# -------------------------------------------------------------------
sub cbgp_net_traceroute($$$)
  {
    my ($cbgp, $src, $dst)= @_;
    my $hop= 1;
    my $status= CBGP_TRACEROUTE_STATUS_ERROR;
    my @traceroute;
    my @hops;

    cbgp_send($cbgp, "net node $src traceroute $dst");
    cbgp_send($cbgp, "print \"CHECKPOINT\\n\"");
    while ((my $result= $cbgp->expect(1)) ne "CHECKPOINT") {
      if ($result =~ m/^\s*([0-9]+)\s+([0-9.]+|\*)\s+(.+)$/) {
	if ($hop != $1) {
	  show_error("incorrect hop number \"$result\"");
	  exit(-1);
	}
	push @hops, ([$1, $2, $3]);
	if ($3 eq CBGP_PING_STATUS_REPLY) {
	  $status= CBGP_TRACEROUTE_STATUS_REPLY;
	} elsif ($3 eq CBGP_PING_STATUS_NO_REPLY) {
	  $status= CBGP_TRACEROUTE_STATUS_NO_REPLY;
	}
      } else {
	show_error("incorrect format (net traceroute): \"$result\"");
	exit(-1);
      }
      $hop++;
    }

    $traceroute[CBGP_TRACEROUTE_STATUS]= $status;
    $traceroute[CBGP_TRACEROUTE_NHOPS]= $hop-1;
    $traceroute[CBGP_TRACEROUTE_HOPS]= \@hops;
    return \@traceroute;
  }

# -----[ cbgp_bgp_record_route ]-------------------------------------
sub cbgp_bgp_record_route($$$)
  {
    my ($cbgp, $src, $dst)= @_;

    cbgp_send($cbgp, "bgp router $src record-route $dst\n");
    my $result= $cbgp->expect(1);
    if ($result =~ m/^([0-9.]+)\s+([0-9.\/]+)\s+([A-Z_]+)\s+([0-9\s]*)$/) {
      my @fields= split /\s+/, $result;
      my @trace;
      $trace[CBGP_TR_SRC]= shift @fields;
      $trace[CBGP_TR_DST]= shift @fields;
      $trace[CBGP_TR_STATUS]= shift @fields;
      $trace[CBGP_TR_PATH]= \@fields;
      return \@trace;
    } else {
      show_error("incorrect format (bgp record-route): \"$result\"");
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
	      [$link->[CBGP_LINK_DELAY],
	       $link->[CBGP_LINK_WEIGHT]];
	  }
	}
	if (!exists($nodes{$node2})) {
	  $nodes{$node2}= 1;
	  my $links_hash= cbgp_show_links($cbgp, $node2);
	  my @links= values %$links_hash;
	  foreach my $link (@links) {
	    $cbgp_topo{$node2}{$link->[CBGP_LINK_DST]}=
	      [$link->[CBGP_LINK_DELAY],
	       $link->[CBGP_LINK_WEIGHT]];
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
    foreach my $comm (@$comm1) {
      $comm1{$comm}= 1;
    }
    ;
    foreach my $comm (@$comm2) {
      $comm2{$comm}= 1;
    }
    ;
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
    cbgp_send($cbgp, "print \"DONE\\n\"");
    while (!((my $result= $cbgp->expect(1)) =~ m/^DONE/)) {
      if ($result =~ m/^([a-z]+)\s+version:\s+([0-9]+)\.([0-9]+)\.([0-9]+)(\-(.+))?(\s+.+)*$/) {
	my $component= $1;
	my $version= "$2.$3.$4";
	if (defined($5)) {
	  $version.= " ($5)";
	}
	if ($component eq "cbgp") {
	  (defined($validation->{'cbgp_version'})) and
	    return TEST_FAILURE;
	  if (($2 < CBGP_MAJOR_MIN()) ||
	      (($2 == CBGP_MAJOR_MIN()) && ($3 < CBGP_MINOR_MIN))) {
	    show_warning("this validation script was designed for version ".
			 CBGP_MAJOR_MIN().".".CBGP_MINOR_MIN().".x");
	  }
	  $validation->{'cbgp_version'}= $version;
	} elsif ($component eq "libgds") {
	  (defined($validation->{'libgds_version'})) and
	    return TEST_FAILURE;
	  $validation->{'libgds_version'}= $version;
	} else {
	  show_warning("unknown component \"$component\"");
	  return TEST_FAILURE;
	}
      } else {
	show_warning("syntax error");
	return TEST_FAILURE;
      }
    }
    return (defined($validation->{'cbgp_version'}) &&
	    defined($validation->{'libgds_version'}))?TEST_SUCCESS:TEST_FAILURE;
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

    my $msg= cbgp_check_error($cbgp, "net add node 1.0.0.1");
    if (defined($msg)) {
      return TEST_FAILURE;
    }

    $msg= cbgp_check_error($cbgp, "net add node 123.45.67.89");
    if (defined($msg)) {
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

    cbgp_send($cbgp, "net add node 1.0.0.1");

    my $msg= cbgp_check_error($cbgp, "net add node 1.0.0.1");
    if (!defined($msg) ||
	!($msg =~ m/$CBGP_ERROR_NODE_EXISTS/)) {
      return TEST_FAILURE;
    }
    return TEST_SUCCESS;
  }

# -----[ cbgp_valid_net_node_name ]----------------------------------
# Test ability to associate a name with a node.
#
# Scenario:
#   * Create a node 1.0.0.1
#   * Read its name (should be empty)
#   * Change its name to "R1"
#   * Read its name (should be "R1")
# -------------------------------------------------------------------
sub cbgp_valid_net_node_name($)
  {
    my ($cbgp)= @_;

    cbgp_send($cbgp, "net add node 1.0.0.1");
    my $info= cbgp_net_node_info($cbgp, "1.0.0.1");
    (!defined($info)) and return TEST_FAILURE;
    (exists($info->{'name'})) and return TEST_FAILURE;
    cbgp_send($cbgp, "net node 1.0.0.1 name R1");
    cbgp_send($cbgp, "net node 1.0.0.1 show info");
    $info= cbgp_net_node_info($cbgp, "1.0.0.1");
    (!defined($info)) and return TEST_FAILURE;
    (!exists($info->{'name'}) || ($info->{'name'} ne "R1")) and
      return TEST_FAILURE;

    return TEST_SUCCESS;
  }

# -----[ cbgp_valid_net_link ]---------------------------------------
# Create a topology composed of 3 nodes and 2 links. Check that the
# links are correctly created. Check the links attributes.
# -------------------------------------------------------------------
sub cbgp_valid_net_link($$)
  {
    my ($cbgp, $topo)= @_;
    cbgp_send($cbgp, "net add node 1.0.0.1\n");
    cbgp_send($cbgp, "net add node 1.0.0.2");
    cbgp_send($cbgp, "net add node 1.0.0.3");
    cbgp_send($cbgp, "net add link 1.0.0.1 1.0.0.2 10");
    cbgp_send($cbgp, "net link 1.0.0.1 1.0.0.2 igp-weight 123");
    cbgp_send($cbgp, "net link 1.0.0.2 1.0.0.1 igp-weight 123");
    cbgp_send($cbgp, "net add link 1.0.0.2 1.0.0.3 10");
    cbgp_send($cbgp, "net link 1.0.0.2 1.0.0.3 igp-weight 321");
    cbgp_send($cbgp, "net link 1.0.0.3 1.0.0.2 igp-weight 321");
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

    cbgp_send($cbgp, "net add node 1.0.0.1");

    my $msg= cbgp_check_error($cbgp, "net add link 1.0.0.1 1.0.0.1 10");
    if (!defined($msg) ||
	!($msg =~ m/$CBGP_ERROR_LINK_ENDPOINTS/)) {
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

    cbgp_send($cbgp, "net add node 1.0.0.1");
    cbgp_send($cbgp, "net add node 1.0.0.2");
    cbgp_send($cbgp, "net add link 1.0.0.1 1.0.0.2 10");

    my $msg= cbgp_check_error($cbgp, "net add link 1.0.0.1 1.0.0.2 10");
    if (!defined($msg) ||
	!($msg =~ m/$CBGP_ERROR_LINK_EXISTS/)) {
      return TEST_FAILURE;
    }
    return TEST_SUCCESS;
  }

# -----[ cbgp_valid_net_link_weight_bidir ]--------------------------
# Check that the IGP weight of a link can be defined in a single
# direction or in both directions at a time.
# -------------------------------------------------------------------
sub cbgp_valid_net_link_weight_bidir($)
  {
    my ($cbgp)= @_;
    my $links;

    cbgp_send($cbgp, "net add node 1.0.0.1");
    cbgp_send($cbgp, "net add node 1.0.0.2");
    cbgp_send($cbgp, "net add link 1.0.0.1 1.0.0.2 10");

    $links= cbgp_show_links($cbgp, "1.0.0.1");
    if (!exists($links->{'1.0.0.2'}) ||
	($links->{'1.0.0.2'}->[CBGP_LINK_WEIGHT] != MAX_IGP_WEIGHT)) {
      $tests->debug("Error: no link R1->R2 or wrong weight.\n");
      return TEST_FAILURE;
    }
    $links= cbgp_show_links($cbgp, "1.0.0.2");
    if (!exists($links->{'1.0.0.1'}) ||
	($links->{'1.0.0.1'}->[CBGP_LINK_WEIGHT] != MAX_IGP_WEIGHT)) {
      $tests->debug("Error: no link R1->R2 or wrong weight.\n");
      return TEST_FAILURE;
    }

    cbgp_send($cbgp, "net link 1.0.0.1 1.0.0.2 igp-weight 123");
    $links= cbgp_show_links($cbgp, "1.0.0.1");
    if (!exists($links->{'1.0.0.2'}) ||
	($links->{'1.0.0.2'}->[CBGP_LINK_WEIGHT] != 123)) {
      $tests->debug("Error: no link R1->R2 or wrong weight.\n");
      return TEST_FAILURE;
    }
    $links= cbgp_show_links($cbgp, "1.0.0.2");
    if (!exists($links->{'1.0.0.1'}) ||
	($links->{'1.0.0.1'}->[CBGP_LINK_WEIGHT] != MAX_IGP_WEIGHT)) {
      $tests->debug("Error: no link R1->R2 or wrong weight.\n");
      return TEST_FAILURE;
    }

    cbgp_send($cbgp, "net link 1.0.0.1 1.0.0.2 igp-weight --bidir 456");
    $links= cbgp_show_links($cbgp, "1.0.0.1");
    if (!exists($links->{'1.0.0.2'}) ||
	($links->{'1.0.0.2'}->[CBGP_LINK_WEIGHT] != 456)) {
      $tests->debug("Error: no link R1->R2 or wrong weight.\n");
      return TEST_FAILURE;
    }
    $links= cbgp_show_links($cbgp, "1.0.0.2");
    if (!exists($links->{'1.0.0.1'}) ||
	($links->{'1.0.0.1'}->[CBGP_LINK_WEIGHT] != 456)) {
      $tests->debug("Error: no link R1->R2 or wrong weight.\n");
      return TEST_FAILURE;
    }

    return TEST_SUCCESS;
  }

# -----[ cbgp_valid_net_link_capacity ]------------------------------
# Check that the capacity of a link can be defined during it's
# creation. The capacity is assigned for both directions.
#
# Setup:
#   - R1 (1.0.0.1)
#   - R2 (1.0.0.2)
#
# Topology:
#   R1 ----- R2
#
# Scenario:
#   * Create R1, R2 and add link with option --bw=12345678
#   * Check that the capacity is assigned for both directions.
#   * Assign a load to R1->R2 and another to R2->R1
#   * Check that the load is correctly assigned
#   * Clear the load in one direction
#   * Check that the load is cleared in only one direction
# -------------------------------------------------------------------
sub cbgp_valid_net_link_capacity($)
  {
    my ($cbgp)= @_;
    my $info;

    cbgp_send($cbgp, "net add node 1.0.0.1");
    cbgp_send($cbgp, "net add node 1.0.0.2");
    cbgp_send($cbgp, "net add link --bw=12345678 1.0.0.1 1.0.0.2 10");

    # Check load and capacity of R1->R2
    $info= cbgp_net_link_info($cbgp, '1.0.0.1', '1.0.0.2');
    if (!exists($info->{'capacity'}) || ($info->{'capacity'} != 12345678) ||
	!exists($info->{'load'}) || ($info->{'load'} != 0)) {
      $tests->debug("Error: no/invalid capacity/load for link R1->R2");
      return TEST_FAILURE;
    }

    # Check load and capacity of R2->R1
    $info= cbgp_net_link_info($cbgp, '1.0.0.2', '1.0.0.1');
    if (!exists($info->{'capacity'}) || ($info->{'capacity'} != 12345678) ||
	!exists($info->{'load'}) || ($info->{'load'} != 0)) {
      $tests->debug("Error: no/invalid capacity for link R2->R1");
      return TEST_FAILURE;
    }

    # Add load to R1->R2
    cbgp_send($cbgp, "net link 1.0.0.1 1.0.0.2 load add 589");
    $info= cbgp_net_link_info($cbgp, '1.0.0.1', '1.0.0.2');
    if (!exists($info->{'load'}) ||
	($info->{'load'} != 589)) {
      $tests->debug("Error: no/invalid load for link R1->R2");
      return TEST_FAILURE;
    }

    # Load of R1->R2 should be cleared
    cbgp_send($cbgp, "net link 1.0.0.1 1.0.0.2 load clear");
    $info= cbgp_net_link_info($cbgp, '1.0.0.1', '1.0.0.2');
    if (!exists($info->{'load'}) ||
	($info->{'load'} != 0)) {
      $tests->debug("Error: load of link R1->R2 should be cleared");
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
    my $msg;

    cbgp_send($cbgp, "net add node 1.0.0.1");
    cbgp_send($cbgp, "net add node 1.0.0.2");
    cbgp_send($cbgp, "net add subnet 192.168.0/24 transit");
    cbgp_send($cbgp, "net add subnet 192.168.1/24 stub");
    cbgp_send($cbgp, "net add link 1.0.0.1 192.168.0.1/24 10");
    cbgp_send($cbgp, "net link 1.0.0.1 192.168.0.1/24 igp-weight 123");
    cbgp_send($cbgp, "net add link 1.0.0.2 192.168.0.2/24 20");
    cbgp_send($cbgp, "net link 1.0.0.2 192.168.0.2/24 igp-weight 321");
    cbgp_send($cbgp, "net add link 1.0.0.2 192.168.1.2/24 30");
    cbgp_send($cbgp, "net link 1.0.0.2 192.168.1.2/24 igp-weight 456");
    cbgp_check_link($cbgp, '1.0.0.1', '192.168.0.0/24',
		    -type=>'TRANSIT',
		    -weight=>123) or return TEST_FAILURE;
    cbgp_check_link($cbgp, '1.0.0.2', '192.168.0.0/24',
		    -type=>'TRANSIT',
		    -weight=>321) or return TEST_FAILURE;
    cbgp_check_link($cbgp, '1.0.0.2', '192.168.1.0/24',
		    -type=>'STUB',
		    -weight=>456) or return TEST_FAILURE;

    # Create a /32 subnet: should fail
    $msg= cbgp_check_error($cbgp, "net add subnet 1.2.3.4/32 transit");
    if (!defined($msg) ||
	!($msg =~ m/$CBGP_ERROR_INVALID_SUBNET/)) {
      return TEST_FAILURE;
    }

    return TEST_SUCCESS;
  }

# -----[ cbgp_valid_net_subnet_duplicate ]---------------------------
#
# -------------------------------------------------------------------
sub cbgp_valid_net_subnet_duplicate($)
  {
    my ($cbgp)= @_;
    my $msg;

    cbgp_send($cbgp, "net add subnet 192.168.0/24 transit\n");
    $msg= cbgp_check_error($cbgp, "net add subnet 192.168.0/24 transit\n");
    if (!defined($msg) || !($msg =~m/$CBGP_ERROR_SUBNET_EXISTS/)) {
      return TEST_FAILURE;
    }
    return TEST_SUCCESS;
  }

# -----[ cbgp_valid_net_subnet_misc ]--------------------------------
#
# -------------------------------------------------------------------
sub cbgp_valid_net_subnet_misc($)
  {
    my ($cbgp)= @_;
    my $msg;
    my $ifaces;

    cbgp_send($cbgp, "net add node 1.0.0.1");
    cbgp_send($cbgp, "net add node 1.0.0.2");
    cbgp_send($cbgp, "net add node 1.0.0.3");
    cbgp_send($cbgp, "net add node 2.0.0.1");
    cbgp_send($cbgp, "net add node 192.168.0.1");
    cbgp_send($cbgp, "net add subnet 192.168.0/24 transit");
    cbgp_send($cbgp, "net add subnet 2.0.0/24 transit");
    cbgp_send($cbgp, "net add link 1.0.0.1 1.0.0.2 10");
    cbgp_send($cbgp, "net add link 1.0.0.1 1.0.0.3 10");
    cbgp_send($cbgp, "net add link 1.0.0.1 2.0.0.1 10");
    cbgp_send($cbgp, "net add link 1.0.0.1 192.168.0.1/24 10");
    cbgp_send($cbgp, "net add link 1.0.0.1 192.168.0.2/24 10");

    # Add a ptp link to 192.168.0.255: it should succeed
    $msg= cbgp_check_error($cbgp, "net add link 1.0.0.1 192.168.0.1 10");
    if (defined($msg)) {
      return TEST_FAILURE;
    }

    # Add a mtp link to 1.0.0/24: it should succeed
    $msg= cbgp_check_error($cbgp, "net add link 1.0.0.1 2.0.0.0/24 10");
    if (defined($msg)) {
      return TEST_FAILURE;
    }

    $ifaces= cbgp_show_ifaces($cbgp, "1.0.0.1");
    if (!exists($ifaces->{"1.0.0.1"}) ||
	($ifaces->{"1.0.0.1"}->[CBGP_IFACE_TYPE] ne "lo")) {
      $tests->debug("error with interface 1.0.0.1");
      return TEST_FAILURE;
    }
    if (!exists($ifaces->{"192.168.0.1_192.168.0.0/24"}) ||
	($ifaces->{"192.168.0.1_192.168.0.0/24"}->[CBGP_IFACE_TYPE] ne "ptmp")) {
      $tests->debug("error with interface 192.168.0.1_192.168.0.0/24");
      return TEST_FAILURE;
    }
    if (!exists($ifaces->{"192.168.0.2_192.168.0.0/24"}) ||
	($ifaces->{"192.168.0.2_192.168.0.0/24"}->[CBGP_IFACE_TYPE] ne "ptmp")) {
      $tests->debug("error with interface 192.168.0.2_192.168.0.0/24");
      return TEST_FAILURE;
    }
    if (!exists($ifaces->{"1.0.0.1_1.0.0.2"}) ||
	($ifaces->{"1.0.0.1_1.0.0.2"}->[CBGP_IFACE_TYPE] ne "ptp")) {
      $tests->debug("error with interface 1.0.0.1_1.0.0.2");
      return TEST_FAILURE;
    }
    if (!exists($ifaces->{"1.0.0.1_1.0.0.3"}) ||
	($ifaces->{"1.0.0.1_1.0.0.3"}->[CBGP_IFACE_TYPE] ne "ptp")) {
      $tests->debug("error with interface 1.0.0.1_1.0.0.3");
      return TEST_FAILURE;
    }
    if (!exists($ifaces->{"1.0.0.1_2.0.0.1"}) ||
	($ifaces->{"1.0.0.1_2.0.0.1"}->[CBGP_IFACE_TYPE] ne "ptp")) {
      $tests->debug("error with interface 1.0.0.1_2.0.0.1");
      return TEST_FAILURE;
    }
    if (!exists($ifaces->{"2.0.0.0_2.0.0.0/24"}) ||
	($ifaces->{"2.0.0.0_2.0.0.0/24"}->[CBGP_IFACE_TYPE] ne "ptmp")) {
      $tests->debug("error with interface 2.0.0.0_2.0.0.0/24");
      return TEST_FAILURE;
    }

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

# -----[ cbgp_valid_net_igp_overflow ]-------------------------------
# Create a small topology with huge weights. Check that the IGP
# computation will converge. Actually, if it does not converge, the
# validation process will be blocked...
# -------------------------------------------------------------------
sub cbgp_valid_net_igp_overflow($$)
  {
    my ($cbgp, $topo)= @_;
    my $rt;

    cbgp_send($cbgp, "net add domain 1 igp");
    cbgp_send($cbgp, "net add node 1.0.0.1");
    cbgp_send($cbgp, "net add node 1.0.0.2");
    cbgp_send($cbgp, "net add node 1.0.0.3");
    cbgp_send($cbgp, "net add link 1.0.0.1 1.0.0.2 4000000000");
    cbgp_send($cbgp, "net add link 1.0.0.2 1.0.0.3 4000000000");
    cbgp_send($cbgp, "net node 1.0.0.1 domain 1");
    cbgp_send($cbgp, "net node 1.0.0.2 domain 1");
    cbgp_send($cbgp, "net node 1.0.0.3 domain 1");
    cbgp_send($cbgp, "net domain 1 compute");
    $rt= cbgp_show_rt($cbgp, "1.0.0.1", undef);
    if (!exists($rt->{"1.0.0.3/32"}) ||
	($rt->{"1.0.0.3/32"}->[CBGP_RT_METRIC] != MAX_IGP_WEIGHT)) {
      return TEST_FAILURE;
    }
    return TEST_SUCCESS;
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

    cbgp_send($cbgp, "net add node 0.1.0.0");

    my $msg= cbgp_check_error($cbgp, "net node 0.1.0.0 domain 1");
    if (!defined($msg) ||
	!($msg =~ m/$CBGP_ERROR_DOMAIN_UNKNOWN/)) {
      return TEST_FAILURE;
    }
    return TEST_SUCCESS;
  }

# -----[ cbgp_valid_net_igp_subnet ]---------------------------------
# Check that the IGP routing protocol model handles subnets
# correctly.
#
# Setup:
#   - R1 (1.0.0.1)
#   - R2 (1.0.0.2)
#   - R3 (1.0.0.3)
#   - N1 (192.168.0/24, transit)
#   - N2 (192.168.1/24, stub)
#   - N3 (192.168.2/24, stub)
#
# Topology:
#        10        10        10
#   R1 ------{N1}------ R2 ------{N3}
#    |                  |
#  1 |                  | 1
#    |                  |
#   R3 ------{N2}-------*
#         1
#
# Scenario:
#   * create topology and compute IGP routes
#   * check that R1 joins R2 through R1->N1 (next-hop = R2.N1)
#   * check that R1 joins R3 through R1->R3
#   * check that R1 joins N1 throuth R1->N1
#   * check that R1 joins N2 through R1->R3
#   * check that R1 joins N3 through R1->N1 (next-hop = R2.N1)
# -------------------------------------------------------------------
sub cbgp_valid_net_igp_subnet($)
  {
    my ($cbgp)= @_;
    my $rt;

    cbgp_send($cbgp, "net add domain 1 igp");
    cbgp_send($cbgp, "net add subnet 192.168.0/24 transit");
    cbgp_send($cbgp, "net add subnet 192.168.1/24 stub");
    cbgp_send($cbgp, "net add subnet 192.168.2/24 stub");
    cbgp_send($cbgp, "net add node 1.0.0.1");
    cbgp_send($cbgp, "net node 1.0.0.1 domain 1");
    cbgp_send($cbgp, "net add node 1.0.0.2");
    cbgp_send($cbgp, "net node 1.0.0.2 domain 1");
    cbgp_send($cbgp, "net add node 1.0.0.3");
    cbgp_send($cbgp, "net node 1.0.0.3 domain 1");
    cbgp_send($cbgp, "net add link 1.0.0.1 1.0.0.3 1");
    cbgp_send($cbgp, "net link 1.0.0.1 1.0.0.3 igp-weight 1");
    cbgp_send($cbgp, "net link 1.0.0.3 1.0.0.1 igp-weight 1");
    cbgp_send($cbgp, "net add link 1.0.0.1 192.168.0.1/24 10");
    cbgp_send($cbgp, "net link 1.0.0.1 192.168.0.1/24 igp-weight 10");
    cbgp_send($cbgp, "net add link 1.0.0.2 192.168.0.2/24 10");
    cbgp_send($cbgp, "net link 1.0.0.2 192.168.0.2/24 igp-weight 10");
    cbgp_send($cbgp, "net add link 1.0.0.2 192.168.1.2/24 1");
    cbgp_send($cbgp, "net link 1.0.0.2 192.168.1.2/24 igp-weight 1");
    cbgp_send($cbgp, "net add link 1.0.0.2 192.168.2.2/24 10");
    cbgp_send($cbgp, "net link 1.0.0.2 192.168.2.2/24 igp-weight 10");
    cbgp_send($cbgp, "net add link 1.0.0.3 192.168.1.3/24 1");
    cbgp_send($cbgp, "net link 1.0.0.3 192.168.1.3/24 igp-weight 1");
    cbgp_send($cbgp, "net domain 1 compute");

    $rt= cbgp_show_rt($cbgp, "1.0.0.1");
    if (!exists($rt->{"1.0.0.2/32"}) ||
	($rt->{"1.0.0.2/32"}->[CBGP_RT_NEXTHOP] ne "192.168.0.2")) {
      $tests->debug("Error: should join 1.0.0.2/32 through 192.168.0.2");
      return TEST_FAILURE;
    }
    if (!exists($rt->{"1.0.0.3/32"})) {
      $tests->debug("Error: should have route towards 1.0.0.3/32");
      return TEST_FAILURE;
    }
    if (!exists($rt->{"192.168.0.0/24"})) {
      $tests->debug("Error: should have route towards 192.168.0.0/24");
      return TEST_FAILURE;
    }
    if (!exists($rt->{"192.168.1.0/24"})) {
      $tests->debug("Error: should have route towards 192.168.1.0/24");
      return TEST_FAILURE;
    }
    if (!exists($rt->{"192.168.2.0/24"}) ||
	($rt->{"192.168.2.0/24"}->[CBGP_RT_NEXTHOP] ne "192.168.0.2")) {
      $tests->debug("Error: should join 192.168.2.0/24 through 192.168.0.2");
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
    cbgp_send($cbgp, "net ntf load \"$ntf_file\"");
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

# -----[ cbgp_valid_net_record_route_delay ]-------------------------
# Create the given topology in C-BGP. Check that record-route
# correctly records the path's IGP weight and delay.
# -------------------------------------------------------------------
sub cbgp_valid_net_record_route_delay($$)
  {
    my ($cbgp, $topo)= @_;
    cbgp_topo($cbgp, $topo, 1);
    cbgp_topo_igp_compute($cbgp, $topo, 1);
    if (!cbgp_topo_check_record_route($cbgp, $topo, "SUCCESS", 1)) {
      return TEST_FAILURE;
    }
    return TEST_SUCCESS;
  }

# -----[ cbgp_valid_net_record_route_capacity ]----------------------
# Check that record-route is able to record the maximum bandwidth
# available along a path.
#
# Setup:
#   - R1 (1.0.0.4)
#   - R2 (1.0.0.2)
#   - R3 (1.0.0.3)
#   - R4 (1.0.0.4)
#   - R5 (1.0.0.5)
#
# Topology:
#       2k        2k
#   R1 ----- R2 ----- R3
#            |\  100
#            | \+---- R4
#            |    1k
#            +------- R5
#
# Scenario:
#   * Setup topology, link capacities and compute routes. Perform
#     record-route from R1 to all nodes.
#   * Check that capacity(R1->R2) = 2000
#   * Check that capacity(R1->R3) = 2000
#   * Check that capacity(R1->R4) = 100
#   * Check that capacity(R1->R5) = 1000
# -------------------------------------------------------------------
sub cbgp_valid_net_record_route_capacity($)
  {
    my ($cbgp)= @_;
    my $trace;

    cbgp_send($cbgp, "net add domain 1 igp");
    cbgp_send($cbgp, "net add node 1.0.0.1");
    cbgp_send($cbgp, "net node 1.0.0.1 domain 1");
    cbgp_send($cbgp, "net add node 1.0.0.2");
    cbgp_send($cbgp, "net node 1.0.0.2 domain 1");
    cbgp_send($cbgp, "net add node 1.0.0.3");
    cbgp_send($cbgp, "net node 1.0.0.3 domain 1");
    cbgp_send($cbgp, "net add node 1.0.0.4");
    cbgp_send($cbgp, "net node 1.0.0.4 domain 1");
    cbgp_send($cbgp, "net add node 1.0.0.5");
    cbgp_send($cbgp, "net node 1.0.0.5 domain 1");
    cbgp_send($cbgp, "net add link --bw=2000 1.0.0.1 1.0.0.2 1");
    cbgp_send($cbgp, "net add link --bw=3000 1.0.0.2 1.0.0.3 1");
    cbgp_send($cbgp, "net add link --bw=100 1.0.0.2 1.0.0.4 1");
    cbgp_send($cbgp, "net add link --bw=1000 1.0.0.2 1.0.0.5 1");
    cbgp_send($cbgp, "net domain 1 compute");

    $trace= cbgp_net_record_route($cbgp, "1.0.0.1", "1.0.0.2", -capacity=>1);
    if (($trace->[CBGP_TR_STATUS] ne "SUCCESS") ||
	($trace->[CBGP_TR_CAPACITY] != 2000)) {
      $tests->debug("Error: record-route R1 -> R2 failed");
      return TEST_FAILURE;
    }
    $trace= cbgp_net_record_route($cbgp, "1.0.0.1", "1.0.0.3", -capacity=>1);
    if (($trace->[CBGP_TR_STATUS] ne "SUCCESS") ||
	($trace->[CBGP_TR_CAPACITY] != 2000)) {
      $tests->debug("Error: record-route R1 -> R3 failed");
      return TEST_FAILURE;
    }
    $trace= cbgp_net_record_route($cbgp, "1.0.0.1", "1.0.0.4", -capacity=>1);
    if (($trace->[CBGP_TR_STATUS] ne "SUCCESS") ||
	($trace->[CBGP_TR_CAPACITY] != 100)) {
      $tests->debug("Error: record-route R1 -> R4 failed");
      return TEST_FAILURE;
    }
    $trace= cbgp_net_record_route($cbgp, "1.0.0.1", "1.0.0.5", -capacity=>1);
    if (($trace->[CBGP_TR_STATUS] ne "SUCCESS") ||
	($trace->[CBGP_TR_CAPACITY] != 1000)) {
      $tests->debug("Error: record-route R1 -> R5 failed");
      return TEST_FAILURE;
    }

    return TEST_SUCCESS;
  }

# -----[ cbgp_valid_net_record_route_load ]--------------------------
# Check that record-route is able to load the links along a path
# with the given volume of traffic.
#
# Setup:
#   - R1 (1.0.0.1)
#   - R2 (1.0.0.2)
#   - R3 (1.0.0.3)
#   - R4 (1.0.0.4)
#   - R5 (1.0.0.5)
#   - R6 (1.0.0.6)
#
# Topology:
#
#   R1 --(1)--*             *--(1)-- R5
#              \           /
#     *--(2)-- R2 --(1)-- R4 --(1)--*
#    /                               \
#   R3 -------------(2)------------- R6
#
#   (W) represent the link's IGP weight
#
# Scenario:
#   * Load R1 -> R5 with 1000
#   * Load R1 -> R6 with 500
#   * Load R1 -> R4 with 250
#   * Load R3 -> R5 with 125
#   * Load R3 -> R6 with 1000
#   * Check that capacity(R1->R2)=1750
#   * Check that capacity(R2->R4)=1875
#   * Check that capacity(R3->R2)=125
#   * Check that capacity(R3->R6)=1000
#   * Check that capacity(R4->R5)=1125
#   * Check that capacity(R4->R6)=500
# -------------------------------------------------------------------
sub cbgp_valid_net_record_route_load($)
  {
    my ($cbgp)= @_;
    my $info;

    cbgp_send($cbgp, "net add domain 1 igp");
    cbgp_send($cbgp, "net add node 1.0.0.1");
    cbgp_send($cbgp, "net node 1.0.0.1 domain 1");
    cbgp_send($cbgp, "net add node 1.0.0.2");
    cbgp_send($cbgp, "net node 1.0.0.2 domain 1");
    cbgp_send($cbgp, "net add node 1.0.0.3");
    cbgp_send($cbgp, "net node 1.0.0.3 domain 1");
    cbgp_send($cbgp, "net add node 1.0.0.4");
    cbgp_send($cbgp, "net node 1.0.0.4 domain 1");
    cbgp_send($cbgp, "net add node 1.0.0.5");
    cbgp_send($cbgp, "net node 1.0.0.5 domain 1");
    cbgp_send($cbgp, "net add node 1.0.0.6");
    cbgp_send($cbgp, "net node 1.0.0.6 domain 1");
    cbgp_send($cbgp, "net add link 1.0.0.1 1.0.0.2 1");
    cbgp_send($cbgp, "net link 1.0.0.1 1.0.0.2 igp-weight --bidir 1");
    cbgp_send($cbgp, "net add link 1.0.0.2 1.0.0.3 1");
    cbgp_send($cbgp, "net link 1.0.0.2 1.0.0.3 igp-weight --bidir 2");
    cbgp_send($cbgp, "net add link 1.0.0.2 1.0.0.4 1");
    cbgp_send($cbgp, "net link 1.0.0.2 1.0.0.4 igp-weight --bidir 1");
    cbgp_send($cbgp, "net add link 1.0.0.3 1.0.0.6 1");
    cbgp_send($cbgp, "net link 1.0.0.3 1.0.0.6 igp-weight --bidir 2");
    cbgp_send($cbgp, "net add link 1.0.0.4 1.0.0.5 1");
    cbgp_send($cbgp, "net link 1.0.0.4 1.0.0.5 igp-weight --bidir 1");
    cbgp_send($cbgp, "net add link 1.0.0.4 1.0.0.6 1");
    cbgp_send($cbgp, "net link 1.0.0.4 1.0.0.6 igp-weight --bidir 1");
    cbgp_send($cbgp, "net domain 1 compute");

    cbgp_net_record_route($cbgp, "1.0.0.1", "1.0.0.5", -load=>1000);
    cbgp_net_record_route($cbgp, "1.0.0.1", "1.0.0.6", -load=>500);
    cbgp_net_record_route($cbgp, "1.0.0.1", "1.0.0.4", -load=>250);
    cbgp_net_record_route($cbgp, "1.0.0.3", "1.0.0.5", -load=>125);
    cbgp_net_record_route($cbgp, "1.0.0.3", "1.0.0.6", -load=>1000);

    $info= cbgp_net_link_info($cbgp, "1.0.0.1", "1.0.0.2");
    if (!exists($info->{load}) || ($info->{load} != 1750)) {
      $tests->debug("Error: load of R1->R2 should be 1750");
      return TEST_FAILURE;
    }
    $info= cbgp_net_link_info($cbgp, "1.0.0.2", "1.0.0.4");
    if (!exists($info->{load}) || ($info->{load} != 1875)) {
      $tests->debug("Error: load of R2->R4 should be 1875");
      return TEST_FAILURE;
    }
    $info= cbgp_net_link_info($cbgp, "1.0.0.3", "1.0.0.2");
    if (!exists($info->{load}) || ($info->{load} != 125)) {
      $tests->debug("Error: load of R3->R2 should be 125");
      return TEST_FAILURE;
    }
    $info= cbgp_net_link_info($cbgp, "1.0.0.3", "1.0.0.6");
    if (!exists($info->{load}) || ($info->{load} != 1000)) {
      $tests->debug("Error: load of R3->R6 should be 1000");
      return TEST_FAILURE;
    }
    $info= cbgp_net_link_info($cbgp, "1.0.0.4", "1.0.0.5");
    if (!exists($info->{load}) || ($info->{load} != 1125)) {
      $tests->debug("Error: load of R4->R5 should be 1125");
      return TEST_FAILURE;
    }
    $info= cbgp_net_link_info($cbgp, "1.0.0.4", "1.0.0.6");
    if (!exists($info->{load}) || ($info->{load} != 500)) {
      $tests->debug("Error: load of R4->R6 should be 500");
      return TEST_FAILURE;
    }
    return TEST_SUCCESS;
  }


# -----[ cbgp_valid_net_record_route_loop ]--------------------------
# Test on the quick loop detection. A normal record-route does not
# detect loop. The basic behavior is to wait until TTL expiration.
#
# Setup:
#   - R1 (1.0.0.1, AS1), NH for 2.0.0.1 = 1.0.0.2
#   - R2 (1.0.0.2, AS1), NH for 2.0.0.1 = 1.0.0.1
#
# Topology:
#
#   R1 -- R2
#
# Scenario:
#   * Record route from node 1.0.0.1 towards 2.0.0.1
#   * Success if the result contains LOOP (instead of TOO_LONG)
# -------------------------------------------------------------------
sub cbgp_valid_net_record_route_loop($)
  {
    my ($cbgp)= @_;

    cbgp_send($cbgp, "net add domain 1 igp\n");
    cbgp_send($cbgp, "net add node 1.0.0.1\n");
    cbgp_send($cbgp, "net add node 1.0.0.2\n");
    cbgp_send($cbgp, "net node 1.0.0.1 domain 1\n");
    cbgp_send($cbgp, "net node 1.0.0.2 domain 1\n");

    cbgp_send($cbgp, "net add link 1.0.0.1 1.0.0.2 1\n");

    cbgp_send($cbgp, "net node 1.0.0.1 route add 2.0.0.1/32 * 1.0.0.2 1\n");
    cbgp_send($cbgp, "net node 1.0.0.2 route add 2.0.0.1/32 * 1.0.0.1 1\n");

    cbgp_send($cbgp, "net domain 1 compute\n");

    my $loop_trace= cbgp_net_record_route($cbgp, "1.0.0.1", "2.0.0.1",
					 -checkloop=>1);
    if ($loop_trace->[CBGP_TR_STATUS] ne "LOOP") {
      return TEST_FAILURE;
    }

    return TEST_SUCCESS;
  }

# -----[ cbgp_valid_net_record_route_prefix ]-----------------------
# Test that record-route works with a prefix destination.
# ------------------------------------------------------------------
sub cbgp_valid_net_record_route_prefix($)
  {
    my ($cbgp)= @_;
    return TEST_NOT_TESTED;
  }

# -----[ cbgp_valid_net_icmp_ping ]---------------------------------
# Test the behaviour of the ping command.
#
# Setup:
#   - R1 (1.0.0.1)
#   - R2 (1.0.0.2)
#   - R3 (1.0.0.3)
#   - R4 (1.0.0.4)
#   - [R5 (1.0.0.5)]
#   - [R6 (1.0.0.6)]
#   - [R7 (1.0.0.7)]
#   - [R8 (1.0.0.8)]
#   - N1 (192.168.0/24)
#   - N2 (192.168.1/24)
#
# Topology:
#
#   R1 ----- R2 ----- R3 --x-- R4
#    |        |
#   (N2)     (N1)
#
# Scenario:
#   * Compute IGP routes
#   * Break link R3-R4
#   * Add to R1 a static route towards R5 through N1 (obviously incorrect)
#   * Add to R1 a static route towards R6 through R2 (obviously incorrect)
#   * Add to R1 a static route towards R8 through N2 (obviously incorrect)
#   * Ping R1: should succeed
#   * Ping R2: should succeed
#   * Ping R3: should succeed
#   * Ping R3 with TTL=1: should fail with icmp-time-exceeded
#   * Ping R4: should fail with no-reply
#   * Ping R5: should fail with icmp-net-unreach
#   * Ping R6: should fail with icmp-dst-unreach
#   * Ping R7: should wail with net-unreach
#   * Ping R8: should fail with dst-unreach
# ------------------------------------------------------------------
sub cbgp_valid_net_icmp_ping($)
  {
    my ($cbgp)= @_;

    cbgp_send($cbgp, "net add domain 1 igp");
    cbgp_send($cbgp, "net add node 1.0.0.1");
    cbgp_send($cbgp, "net node 1.0.0.1 domain 1");
    cbgp_send($cbgp, "net add node 1.0.0.2");
    cbgp_send($cbgp, "net node 1.0.0.2 domain 1");
    cbgp_send($cbgp, "net add node 1.0.0.3");
    cbgp_send($cbgp, "net node 1.0.0.3 domain 1");
    cbgp_send($cbgp, "net add node 1.0.0.4");
    cbgp_send($cbgp, "net node 1.0.0.4 domain 1");
    cbgp_send($cbgp, "net add subnet 192.168.0/24 transit");
    cbgp_send($cbgp, "net add subnet 192.168.1/24 transit");
    cbgp_send($cbgp, "net add link 1.0.0.1 1.0.0.2 10");
    cbgp_send($cbgp, "net link 1.0.0.1 1.0.0.2 igp-weight --bidir 10");
    cbgp_send($cbgp, "net add link 1.0.0.1 192.168.1.1/24 10");
    cbgp_send($cbgp, "net link 1.0.0.1 192.168.1.1/24 igp-weight 10");
    cbgp_send($cbgp, "net add link 1.0.0.2 1.0.0.3 10");
    cbgp_send($cbgp, "net link 1.0.0.2 1.0.0.3 igp-weight --bidir 10");
    cbgp_send($cbgp, "net add link 1.0.0.2 192.168.0.1/24 10");
    cbgp_send($cbgp, "net link 1.0.0.2 192.168.0.1/24 igp-weight 10");
    cbgp_send($cbgp, "net add link 1.0.0.3 1.0.0.4 10");
    cbgp_send($cbgp, "net link 1.0.0.3 1.0.0.4 igp-weight --bidir 10");
    cbgp_send($cbgp, "net domain 1 compute");

    cbgp_send($cbgp, "net link 1.0.0.3 1.0.0.4 down");
    cbgp_send($cbgp, "net node 1.0.0.1 route add 1.0.0.5/32 * 1.0.0.2 1");
    cbgp_send($cbgp, "net node 1.0.0.1 route add 1.0.0.6/32 * 1.0.0.2 1");
    cbgp_send($cbgp, "net node 1.0.0.2 route add 1.0.0.6/32 * 192.168.0.1/24 1");
    cbgp_send($cbgp, "net node 1.0.0.1 route add 1.0.0.8/32 * 192.168.1.1/24 1");

    my $ping= cbgp_net_ping($cbgp, '1.0.0.1', '1.0.0.1');
    if (($ping->[CBGP_PING_STATUS] ne CBGP_PING_STATUS_REPLY) ||
	($ping->[CBGP_PING_ADDR] ne '1.0.0.1')) {
      return TEST_FAILURE;
    }

    $ping= cbgp_net_ping($cbgp, '1.0.0.1', '1.0.0.2');
    if (($ping->[CBGP_PING_STATUS] ne CBGP_PING_STATUS_REPLY) ||
	($ping->[CBGP_PING_ADDR] ne '1.0.0.2')) {
      return TEST_FAILURE;
    }

    $ping= cbgp_net_ping($cbgp, '1.0.0.1', '1.0.0.3');
    if (($ping->[CBGP_PING_STATUS] ne CBGP_PING_STATUS_REPLY) ||
	($ping->[CBGP_PING_ADDR] ne '1.0.0.3')) {
      return TEST_FAILURE;
    }

    $ping= cbgp_net_ping($cbgp, '1.0.0.1', '1.0.0.3', -ttl=>1);
    if (($ping->[CBGP_PING_STATUS] ne CBGP_PING_STATUS_ICMP_TTL) ||
	($ping->[CBGP_PING_ADDR] ne '1.0.0.2')) {
      return TEST_FAILURE;
    }

    $ping= cbgp_net_ping($cbgp, '1.0.0.1', '1.0.0.4');
    if ($ping->[CBGP_PING_STATUS] ne CBGP_PING_STATUS_NO_REPLY) {
      return TEST_FAILURE;
    }

    $ping= cbgp_net_ping($cbgp, '1.0.0.1', '1.0.0.5');
    if (($ping->[CBGP_PING_STATUS] ne CBGP_PING_STATUS_ICMP_NET) ||
	($ping->[CBGP_PING_ADDR] ne '1.0.0.2')) {
      return TEST_FAILURE;
    }

    $ping= cbgp_net_ping($cbgp, '1.0.0.1', '1.0.0.6');
    if (($ping->[CBGP_PING_STATUS] ne CBGP_PING_STATUS_ICMP_DST) ||
	($ping->[CBGP_PING_ADDR] ne '1.0.0.2')) {
      return TEST_FAILURE;
    }

    $ping= cbgp_net_ping($cbgp, '1.0.0.1', '1.0.0.7');
    if ($ping->[CBGP_PING_STATUS] ne CBGP_PING_STATUS_NET) {
      return TEST_FAILURE;
    }

    $ping= cbgp_net_ping($cbgp, '1.0.0.1', '1.0.0.8');
    if ($ping->[CBGP_PING_STATUS] ne CBGP_PING_STATUS_DST) {
      return TEST_FAILURE;
    }

    return TEST_SUCCESS;
  }

# -----[ cbgp_valid_net_icmp_traceroute ]---------------------------
# Test the behaviour of the traceroute command.
# ------------------------------------------------------------------
sub cbgp_valid_net_icmp_traceroute($)
  {
    my ($cbgp)= @_;

    cbgp_send($cbgp, "net add domain 1 igp");
    cbgp_send($cbgp, "net add node 1.0.0.1");
    cbgp_send($cbgp, "net node 1.0.0.1 domain 1");
    cbgp_send($cbgp, "net add node 1.0.0.2");
    cbgp_send($cbgp, "net node 1.0.0.2 domain 1");
    cbgp_send($cbgp, "net add node 1.0.0.3");
    cbgp_send($cbgp, "net node 1.0.0.3 domain 1");
    cbgp_send($cbgp, "net add node 1.0.0.4");
    cbgp_send($cbgp, "net node 1.0.0.4 domain 1");
    cbgp_send($cbgp, "net add subnet 192.168.0/24 transit");
    cbgp_send($cbgp, "net add subnet 192.168.1/24 transit");
    cbgp_send($cbgp, "net add link 1.0.0.1 1.0.0.2 10");
    cbgp_send($cbgp, "net link 1.0.0.1 1.0.0.2 igp-weight --bidir 10");
    cbgp_send($cbgp, "net add link 1.0.0.1 192.168.1.1/24 10");
    cbgp_send($cbgp, "net link 1.0.0.1 192.168.1.1/24 igp-weight 10");
    cbgp_send($cbgp, "net add link 1.0.0.2 1.0.0.3 10");
    cbgp_send($cbgp, "net link 1.0.0.2 1.0.0.3 igp-weight --bidir 10");
    cbgp_send($cbgp, "net add link 1.0.0.2 192.168.0.1/24 10");
    cbgp_send($cbgp, "net link 1.0.0.2 192.168.0.1/24 igp-weight 10");
    cbgp_send($cbgp, "net add link 1.0.0.3 1.0.0.4 10");
    cbgp_send($cbgp, "net link 1.0.0.3 1.0.0.4 igp-weight --bidir 10");
    cbgp_send($cbgp, "net domain 1 compute");

    cbgp_send($cbgp, "net link 1.0.0.3 1.0.0.4 down");
    cbgp_send($cbgp, "net node 1.0.0.1 route add 1.0.0.5/32 * 1.0.0.2 1");
    cbgp_send($cbgp, "net node 1.0.0.1 route add 1.0.0.6/32 * 1.0.0.2 1");
    cbgp_send($cbgp, "net node 1.0.0.2 route add 1.0.0.6/32 * 192.168.0.1/24 1");
    cbgp_send($cbgp, "net node 1.0.0.1 route add 1.0.0.8/32 * 192.168.1.1/24 1");

    my $tr= cbgp_net_traceroute($cbgp, '1.0.0.1', '1.0.0.1');
    if (($tr->[CBGP_TRACEROUTE_NHOPS] != 1) ||
	($tr->[CBGP_TRACEROUTE_HOPS]->[0]->[1] ne '1.0.0.1') ||
	($tr->[CBGP_TRACEROUTE_HOPS]->[0]->[2] ne CBGP_PING_STATUS_REPLY)) {
      return TEST_FAILURE;
    }

    $tr= cbgp_net_traceroute($cbgp, '1.0.0.1', '1.0.0.2');
    if (($tr->[CBGP_TRACEROUTE_NHOPS] != 1) ||
	($tr->[CBGP_TRACEROUTE_HOPS]->[0]->[1] ne '1.0.0.2') ||
	($tr->[CBGP_TRACEROUTE_HOPS]->[0]->[2] ne CBGP_PING_STATUS_REPLY)) {
      return TEST_FAILURE;
    }

    $tr= cbgp_net_traceroute($cbgp, '1.0.0.1', '1.0.0.3');
    if (($tr->[CBGP_TRACEROUTE_NHOPS] != 2) ||
	($tr->[CBGP_TRACEROUTE_HOPS]->[1]->[1] ne '1.0.0.3') ||
	($tr->[CBGP_TRACEROUTE_HOPS]->[1]->[2] ne CBGP_PING_STATUS_REPLY)) {
      return TEST_FAILURE;
    }

    $tr= cbgp_net_traceroute($cbgp, '1.0.0.1', '1.0.0.4');
    if (($tr->[CBGP_TRACEROUTE_NHOPS] != 3) ||
	($tr->[CBGP_TRACEROUTE_HOPS]->[2]->[1] ne '*') ||
	($tr->[CBGP_TRACEROUTE_HOPS]->[2]->[2] ne CBGP_PING_STATUS_NO_REPLY)) {
      return TEST_FAILURE;
    }

    $tr= cbgp_net_traceroute($cbgp, '1.0.0.1', '1.0.0.5');
    if (($tr->[CBGP_TRACEROUTE_NHOPS] != 2) ||
	($tr->[CBGP_TRACEROUTE_HOPS]->[1]->[1] ne '1.0.0.2') ||
	($tr->[CBGP_TRACEROUTE_HOPS]->[1]->[2] ne CBGP_PING_STATUS_ICMP_NET)) {
      return TEST_FAILURE;
    }

    $tr= cbgp_net_traceroute($cbgp, '1.0.0.1', '1.0.0.6');
    if (($tr->[CBGP_TRACEROUTE_NHOPS] != 2) ||
	($tr->[CBGP_TRACEROUTE_HOPS]->[1]->[1] ne '1.0.0.2') ||
	($tr->[CBGP_TRACEROUTE_HOPS]->[1]->[2] ne CBGP_PING_STATUS_ICMP_DST)) {
      return TEST_FAILURE;
    }

    $tr= cbgp_net_traceroute($cbgp, '1.0.0.1', '1.0.0.7');
    if (($tr->[CBGP_TRACEROUTE_NHOPS] != 1) ||
	($tr->[CBGP_TRACEROUTE_HOPS]->[0]->[1] ne '*') ||
	($tr->[CBGP_TRACEROUTE_HOPS]->[0]->[2] ne CBGP_PING_STATUS_NET)) {
      return TEST_FAILURE;
    }

    $tr= cbgp_net_traceroute($cbgp, '1.0.0.1', '1.0.0.8');
    if (($tr->[CBGP_TRACEROUTE_NHOPS] != 1) ||
	($tr->[CBGP_TRACEROUTE_HOPS]->[0]->[1] ne '*') ||
	($tr->[CBGP_TRACEROUTE_HOPS]->[0]->[2] ne CBGP_PING_STATUS_DST)) {
      return TEST_FAILURE;
    }

    return TEST_SUCCESS;
  }

# -----[ cbgp_valid_net_tunnel ]------------------------------------
# Check that a tunnel can be created. Check that forwarding through
# the tunnel works.
#
# Setup:
#   - R1 (1.0.0.0)
#   - R2 (1.0.0.1)
#   - R3 (1.0.0.2)
#   - R4 (1.0.0.3)
#   - R5 (1.0.0.4)
#   - R6 (2.0.0.0)
#   - N1 (192.168.0/24)
#
# Topology:
#
#    *---- R2 ----*    *---- R6
#   /              \  /
#  R1               R4
#   \          w=10/ |
#    *---- R3 ----*  |
#           \        /
#            *-(N1)-*
#                |
#                R5
#
# Scenario:
#
# -------------------------------------------------------------------
sub cbgp_valid_net_tunnel($)
  {
    my ($cbgp)= @_;

    # Domain 1
    cbgp_send($cbgp, "net add domain 1 igp");
    cbgp_send($cbgp, "net add node 1.0.0.0");
    cbgp_send($cbgp, "net node 1.0.0.0 domain 1");
    cbgp_send($cbgp, "net add node 1.0.0.1");
    cbgp_send($cbgp, "net node 1.0.0.1 domain 1");
    cbgp_send($cbgp, "net add node 1.0.0.2");
    cbgp_send($cbgp, "net node 1.0.0.2 domain 1");
    cbgp_send($cbgp, "net add node 1.0.0.3");
    cbgp_send($cbgp, "net node 1.0.0.3 domain 1");
    cbgp_send($cbgp, "net add node 1.0.0.4");
    cbgp_send($cbgp, "net node 1.0.0.4 domain 1");
    cbgp_send($cbgp, "net add subnet 192.168.0/24 transit");
    cbgp_send($cbgp, "net add link 1.0.0.0 1.0.0.1 1");
    cbgp_send($cbgp, "net link 1.0.0.0 1.0.0.1 igp-weight --bidir 1");
    cbgp_send($cbgp, "net add link 1.0.0.0 1.0.0.2 1");
    cbgp_send($cbgp, "net link 1.0.0.0 1.0.0.2 igp-weight --bidir 1");
    cbgp_send($cbgp, "net add link 1.0.0.1 1.0.0.3 1");
    cbgp_send($cbgp, "net link 1.0.0.1 1.0.0.3 igp-weight --bidir 1");
    cbgp_send($cbgp, "net add link 1.0.0.2 1.0.0.3 10");
    cbgp_send($cbgp, "net link 1.0.0.2 1.0.0.3 igp-weight --bidir 10");
    cbgp_send($cbgp, "net add link 1.0.0.2 192.168.0.1/24 1");
    cbgp_send($cbgp, "net link 1.0.0.2 192.168.0.1/24 igp-weight 1");
    cbgp_send($cbgp, "net add link 1.0.0.3 192.168.0.2/24 1");
    cbgp_send($cbgp, "net link 1.0.0.3 192.168.0.2/24 igp-weight 1");
    cbgp_send($cbgp, "net add link 1.0.0.4 192.168.0.3/24 1");
    cbgp_send($cbgp, "net link 1.0.0.4 192.168.0.3/24 igp-weight 1");
    cbgp_send($cbgp, "net domain 1 compute");

    # Domain 2
    cbgp_send($cbgp, "net add domain 2 igp");
    cbgp_send($cbgp, "net add node 2.0.0.0");

    # Interdomain link + static route
    cbgp_send($cbgp, "net add link 1.0.0.3 2.0.0.0 1");
    cbgp_send($cbgp, "net node 1.0.0.3 route add 2.0.0/24 * 2.0.0.0 1");

    # Default route from R1 to R4 should be R1 R2 R4
    # Default route from R1 to R5 should be R1 R2 R4 R5
    # Default route from R1 to R6 should be R1 R2 R4 R6

    # Create tunnels + static routes
    cbgp_send($cbgp, "net node 1.0.0.3 ipip-enable");
    cbgp_send($cbgp, "net node 1.0.0.0");
    cbgp_send($cbgp, "  tunnel add --oif=1.0.0.2 1.0.0.3 255.0.0.0");
    cbgp_send($cbgp, "  tunnel add 1.0.0.3 255.0.0.1");
    cbgp_send($cbgp, "  tunnel add 1.0.0.4 255.0.0.2");
    cbgp_send($cbgp, "  route add 1.0.0.3/32 * 255.0.0.0 1");
    cbgp_send($cbgp, "  route add 2.0.0/24 * 255.0.0.1 1");
    cbgp_send($cbgp, "  route add 1.0.0.1/32 * 255.0.0.2 1");

    # Path from R1 to R4 should be R1 R3 R4
    # Path from R1 to R5 should be R1 R3 R4 R5
    # Path from R1 to R6 should be R1 R3 R5 R6 (i.e. through N1)


    return TEST_SUCCESS;
  }

# -----[ cbgp_valid_net_tunnel_duplicate ]---------------------------
#
# -------------------------------------------------------------------
sub cbgp_valid_net_tunnel_duplicate($)
  {
    my ($cbgp)= @_;

    return TEST_NOT_TESTED;
  }

# -----[ cbgp_valid_net_record_route_tunnel ]------------------------
#
# -------------------------------------------------------------------
sub cbgp_valid_net_record_route_tunnel($)
  {
    my ($cbgp)= @_;

    return TEST_NOT_TESTED;
  }

# -----[ cbgp_valid_net_static_routes ]------------------------------
# Check ability to add and remove static routes.
#
# Setup:
#
# Scenario:
#
# -------------------------------------------------------------------
sub cbgp_valid_net_static_routes($)
  {
    my ($cbgp)= @_;
    my $trace;
    my $rt;

    cbgp_send($cbgp, "net add node 1.0.0.1");
    cbgp_send($cbgp, "net add node 1.0.0.2");
    cbgp_send($cbgp, "net add link 1.0.0.1 1.0.0.2 10");

    # Test without static route: should fail
    $trace= cbgp_net_record_route($cbgp, "1.0.0.1", "1.0.0.2");
    if ($trace->[CBGP_TR_STATUS] eq "SUCCESS") {
      return TEST_FAILURE;
    }

    # Test with static route: should succeed
    cbgp_send($cbgp, "net node 1.0.0.1 route add 1.0.0.2/32 1.0.0.2 1.0.0.2 10");
    $trace= cbgp_net_record_route($cbgp, "1.0.0.1", "1.0.0.2");
    if ($trace->[CBGP_TR_STATUS] ne "SUCCESS") {
      return TEST_FAILURE;
    }

    # Remove route and test: should fail
    cbgp_send($cbgp, "net node 1.0.0.1 route del 1.0.0.2/32 1.0.0.2 1.0.0.2");
    $trace= cbgp_net_record_route($cbgp, "1.0.0.1", "1.0.0.2");
    if ($trace->[CBGP_TR_STATUS] eq "SUCCESS") {
      return TEST_FAILURE;
    }

    return TEST_SUCCESS;
  }

# -----[ cbgp_valid_net_static_routes_wildcards ]--------------------
# Check ability to add and remove static routes using wildcards.
#
# Setup:
#
# Scenario:
#
# -------------------------------------------------------------------
sub cbgp_valid_net_static_routes_wildcards($)
  {
    my ($cbgp)= @_;
    my $trace;

    cbgp_send($cbgp, "net add node 1.0.0.1");
    cbgp_send($cbgp, "net add node 1.0.0.2");
    cbgp_send($cbgp, "net add node 1.0.0.3");
    cbgp_send($cbgp, "net add link 1.0.0.1 1.0.0.2 10");
    cbgp_send($cbgp, "net add link 1.0.0.1 1.0.0.3 20");
    cbgp_send($cbgp, "net add link 1.0.0.2 1.0.0.3 20");

    # Test with static towards each node: should succeed
    cbgp_send($cbgp, "net node 1.0.0.1 route add 1.0.0.2/32 * 1.0.0.2 10");
    cbgp_send($cbgp, "net node 1.0.0.1 route add 1.0.0.3/32 * 1.0.0.3 10");
    $trace= cbgp_net_record_route($cbgp, "1.0.0.1", "1.0.0.2");
    if ($trace->[CBGP_TR_STATUS] ne "SUCCESS") {
      $tests->debug("=> ERROR TEST(1)");
      return TEST_FAILURE;
    }
    $trace= cbgp_net_record_route($cbgp, "1.0.0.1", "1.0.0.3");
    if ($trace->[CBGP_TR_STATUS] ne "SUCCESS") {
      $tests->debug("=> ERROR TEST(2)");
      return TEST_FAILURE;
    }

    # Remove routes using wildcard: should succeed
    cbgp_send($cbgp, "net node 1.0.0.1 route del 1.0.0.2/32 * *");
    $trace= cbgp_net_record_route($cbgp, "1.0.0.1", "1.0.0.2");
    if ($trace->[CBGP_TR_STATUS] eq "SUCCESS") {
      $tests->debug("=> ERROR TEST(3)");
      return TEST_FAILURE;
    }
    $trace= cbgp_net_record_route($cbgp, "1.0.0.1", "1.0.0.3");
    if ($trace->[CBGP_TR_STATUS] ne "SUCCESS") {
      $tests->debug("=> ERROR TEST(4)");
      return TEST_FAILURE;
    }

    # Remove routes using wildcard: should succeed
    cbgp_send($cbgp, "net node 1.0.0.1 route del 1.0.0.3/32 * *");
    $trace= cbgp_net_record_route($cbgp, "1.0.0.1", "1.0.0.2");
    if ($trace->[CBGP_TR_STATUS] eq "SUCCESS") {
      $tests->debug("=> ERROR TEST(5)");
      return TEST_FAILURE;
    }
    $trace= cbgp_net_record_route($cbgp, "1.0.0.1", "1.0.0.3");
    if ($trace->[CBGP_TR_STATUS] eq "SUCCESS") {
      $tests->debug("=> ERROR TEST(6)");
      return TEST_FAILURE;
    }

    return TEST_SUCCESS;
  }

# -----[ cbgp_valid_net_static_routes_duplicate ]--------------------
# Check that adding multiple routes towards the same prefix generates
# an error.
#
# Setup:
#
# Scenario:
#
# -------------------------------------------------------------------
sub cbgp_valid_net_static_routes_duplicate($)
  {
    my ($cbgp)= @_;

    cbgp_send($cbgp, "net add node 1.0.0.1");
    cbgp_send($cbgp, "net add node 1.0.0.2");
    cbgp_send($cbgp, "net add node 1.0.0.3");
    cbgp_send($cbgp, "net add link 1.0.0.1 1.0.0.2 10");
    cbgp_send($cbgp, "net add link 1.0.0.1 1.0.0.3 10");
    cbgp_send($cbgp, "net add link 1.0.0.2 1.0.0.3 10");

    # Adding a single route: should succeed
    my $msg=
      cbgp_check_error($cbgp,
		       "net node 1.0.0.1 route add 1.0.0.2/32 * 1.0.0.2 10");
    if (defined($msg)) {
      return TEST_FAILURE;
    }

    # Adding a route through same interface: should fail
    $msg= cbgp_check_error($cbgp,
			   "net node 1.0.0.1 route add 1.0.0.2/32 * 1.0.0.2 10");
    if (!defined($msg) || !($msg =~ m/$CBGP_ERROR_ROUTE_EXISTS/)) {
      $tests->debug("=> no or incorrect error message generated");
      return TEST_FAILURE;
    }

    # Adding a route through other interface: should succeed
    $msg= cbgp_check_error($cbgp,
			   "net node 1.0.0.1 route add 1.0.0.2/32 * 1.0.0.3 10");
    if (defined($msg)) {
      $tests->debug("=> error message generated");
      return TEST_FAILURE;
    }

    return TEST_SUCCESS;
  }

# -----[ cbgp_valid_net_static_routes_errors ]-----------------------
# Check that adding a static route with a next-hop that cannot be
# joined through the interface generates an error.
#
# Setup:
#
# Scenario:
#   * try to add a static route towards R2 in R1 with R1's loopback
#     as outgoing interface.
#   * check that an error is raised
#   * try to add a static route towards R2
#   * check that an error is raised
# -------------------------------------------------------------------
sub cbgp_valid_net_static_routes_errors($)
  {
    my ($cbgp)= @_;
    my $msg;

    cbgp_send($cbgp, "net add node 1.0.0.1");
    cbgp_send($cbgp, "net add node 1.0.0.2");
    cbgp_send($cbgp, "net add subnet 192.168.0/24 transit");

    # Should not be able to add a route with R1's loopbask as outgoing
    # interface.
    $msg= cbgp_check_error($cbgp,
			   "net node 1.0.0.1 route add 1.0.0.2/32 * 1.0.0.1 10");
    if (!defined($msg) || !($msg =~ m/$CBGP_ERROR_ROUTE_BAD_IFACE/)) {
      $tests->debug("Error: no/invalid error for nh:*, if:1.0.0.1");
      return TEST_FAILURE;
    }

    # Should not be able to add a route through an unexisting interface
    $msg= cbgp_check_error($cbgp,
			   "net node 1.0.0.1 route add 1.0.0.2/32 * 1.0.0.2 10");
    if (!defined($msg) || !($msg =~ m/$CBGP_ERROR_ROUTE_BAD_IFACE/)) {
      $tests->debug("Error: no/invalid error for nh:*, if:1.0.0.2");
      return TEST_FAILURE;
    }

    # Should not be able to add a route with a next-hop that is not
    # reachable through the outgoing interface.
    cbgp_send($cbgp, "net add link 1.0.0.1 1.0.0.2 10");
    $msg= cbgp_check_error($cbgp,
			   "net node 1.0.0.1 route add 1.0.0.2/32 1.0.0.3 1.0.0.2 10");
    if (!defined($msg) || !($msg =~ m/$CBGP_ERROR_ROUTE_NH_UNREACH/)) {
      $tests->debug("Error: no/invalid error for nh:1.0.0.3, if:1.0.0.2");
      return TEST_FAILURE;
    }

    # Should not be able to add a route through a subnet, with a next-hop
    # equal to the outgoing interface
    cbgp_send($cbgp, "net add link 1.0.0.1 192.168.0.1/24 10");
    $msg= cbgp_check_error($cbgp,
			   "net node 1.0.0.1 route add 1.0.0.2/32 192.168.0.1 192.168.0.1/24 10");
    if (!defined($msg) || !($msg =~ m/$CBGP_ERROR_ROUTE_NH_UNREACH/)) {
      $tests->debug("Error: no/invalid error for nh:192.168.0.1, if:192.168.0.1/24");
      return TEST_FAILURE;
    }

    # Should not be able to add route through a subnet that cannot reach
    # the next-hop
    $msg= cbgp_check_error($cbgp,
			   "net node 1.0.0.1 route add 1.0.0.2/32 192.168.0.2 192.168.0.1/24 10");
    if (!defined($msg) || !($msg =~ m/$CBGP_ERROR_ROUTE_NH_UNREACH/)) {
      $tests->debug("Error: no/invalid error for nh:192.168.0.2, if:192.168.0.1/24");
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
    cbgp_send($cbgp, "net node 1.0.0.2 route add 1/8 * 1.0.0.1 10");
    cbgp_send($cbgp, "net node 1.0.0.2 route add 1.1/16 * 1.0.0.3 10");

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
    $trace= cbgp_net_record_route($cbgp, "1.0.0.2", "1.0.0.0");
    if ($trace->[CBGP_TR_PATH]->[1] ne "1.0.0.1") {
      return TEST_FAILURE;
    }
    $trace= cbgp_net_record_route($cbgp, "1.0.0.2", "1.1.0.0");
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
    cbgp_send($cbgp, "net node 1.0.0.2 route add 1.0.0.1/32 * 1.0.0.3 10");
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
    $trace= cbgp_net_record_route($cbgp, "1.0.0.2", "1.0.0.1");
    if ($trace->[CBGP_TR_PATH]->[1] ne "1.0.0.3") {
      return TEST_FAILURE;
    }
    $trace= cbgp_net_record_route($cbgp, "1.0.0.2", "1.0.0.3");
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

    cbgp_send($cbgp, "net add node 123.0.0.234");
    cbgp_send($cbgp, "bgp add router 255 123.0.0.234");
    cbgp_send($cbgp, "bgp router 123.0.0.234 add network 192.168.0/24");
    cbgp_send($cbgp, "bgp options show-mode cisco");
    cbgp_send($cbgp, "bgp router 123.0.0.234 show rib *");
    cbgp_send($cbgp, "print \"done\\n\"");

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

    cbgp_send($cbgp, "net add node 123.0.0.234");
    cbgp_send($cbgp, "bgp add router 255 123.0.0.234");
    cbgp_send($cbgp, "bgp router 123.0.0.234 add network 192.168.0/24");
    cbgp_send($cbgp, "bgp options show-mode mrt");
    cbgp_send($cbgp, "bgp router 123.0.0.234 show rib *");
    cbgp_send($cbgp, "print \"done\\n\"");

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

    cbgp_send($cbgp, "net add node 123.0.0.234");
    cbgp_send($cbgp, "bgp add router 255 123.0.0.234");
    cbgp_send($cbgp, "bgp router 123.0.0.234 add network 192.168.0/24");
    cbgp_send($cbgp, "bgp options show-mode custom \"ROUTE:%i;%a;%p;%P;%C;%O;%n\"");
    cbgp_send($cbgp, "bgp router 123.0.0.234 show rib *");
    cbgp_send($cbgp, "print \"done\\n\"");

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
    cbgp_send($cbgp, "bgp topology policies");
    cbgp_send($cbgp, "bgp topology run");
    if (!cbgp_topo_check_ebgp_sessions($cbgp, $topo, "OPENWAIT")) {
      return TEST_FAILURE;
    }
    cbgp_send($cbgp, "sim run");
    if (!cbgp_topo_check_ebgp_sessions($cbgp, $topo, "ESTABLISHED")) {
      return TEST_FAILURE;
    }

    return TEST_SUCCESS;
  }

# -----[ cbgp_valid_bgp_topology_subra2004 ]-------------------------
#
# -------------------------------------------------------------------
sub cbgp_valid_bgp_topology_subra2004($)
  {
    my ($cbgp)= @_;
    my $topo_file= $resources_path."data/subra-2004.txt";

    cbgp_send($cbgp, "bgp topology load \"$topo_file\"");
    cbgp_send($cbgp, "bgp topology policies");
    cbgp_send($cbgp, "bgp topology run");
    cbgp_send($cbgp, "sim run");
    # AS7018
    cbgp_send($cbgp, "bgp router 27.106.0.0 add network 255/8");
    # AS8709
    cbgp_send($cbgp, "bgp router 34.5.0.0 add network 245/8");
    cbgp_send($cbgp, "sim run");

    my $rib= cbgp_show_rib($cbgp, "27.106.0.0");
    $rib= cbgp_show_rib($cbgp, "34.5.0.0");
    return TEST_SUCCESS;
  }

# -----[ cbgp_valid_bgp_topology_smalltopo ]-------------------------
#
# -------------------------------------------------------------------
sub cbgp_valid_bgp_topology_smalltopo($)
  {
    my ($cbgp)= @_;
    my $topo_file= $resources_path."data/small.topology.txt";

    cbgp_send($cbgp, "bgp topology load \"$topo_file\"");
    cbgp_send($cbgp, "bgp topology policies");
    cbgp_send($cbgp, "bgp topology run");
    cbgp_send($cbgp, "sim run");
    # AS0
    cbgp_send($cbgp, "bgp router 0.0.0.0 add network 255/8");
    # AS11922
    cbgp_send($cbgp, "bgp router 46.146.0.0 add network 254/8");
    cbgp_send($cbgp, "sim run");

    my $rib= cbgp_show_rib($cbgp, "0.0.0.0");
    $rib= cbgp_show_rib($cbgp, "46.146.0.0");
    return TEST_SUCCESS;
  }

# -----[ cbgp_valid_bgp_topology_largetopo ]-------------------------
#
# -------------------------------------------------------------------
sub cbgp_valid_bgp_topology_largetopo($)
  {
    my ($cbgp)= @_;
    my $topo_file= $resources_path."data/large.topology.txt";

    cbgp_send($cbgp, "bgp topology load \"$topo_file\"");
    cbgp_send($cbgp, "bgp topology policies");
    cbgp_send($cbgp, "bgp topology run");
    cbgp_send($cbgp, "sim run");
    # AS0
    cbgp_send($cbgp, "bgp router 0.0.0.0 add network 255/8");
    # AS14694
    cbgp_send($cbgp, "bgp router 57.102.0.0 add network 254/8");
    cbgp_send($cbgp, "sim run");

    my $rib= cbgp_show_rib($cbgp, "0.0.0.0");
    $rib= cbgp_show_rib($cbgp, "57.102.0.0");
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

# -----[ cbgp_valid_bgp_session_nexthop_subnet ]---------------------
# Check that BGP can install a route with a next-hop reachable
# through a subnet.
#
# Setup:
#   - R1 (1.0.0.1, AS1)
#   - R2 (1.0.0.2, AS2)
#   - R3 (2.0.0.1, AS2, virtual)
#   - N1 (192.168.0/24)
#   - R1.N1 = 192.168.0.1
#   - R3.N1 = 192.168.0.2
#
# Topology:
#
#   R3 ---{N1}--- R1 ----- R2
#      .2      .1
#
# Scenario:
#   * Advertise a BGP route from R3 to R1, run the simulation
#   * Check that R1 has installed the route with nh:R3.N1 and if:R1->N1
#   * Check that R2 has installed the route with nh:0.0.0.0 and if:R2->R1
# -------------------------------------------------------------------
sub cbgp_valid_bgp_session_nexthop_subnet($)
  {
    my ($cbgp)= @_;
    my $rt;

    cbgp_send($cbgp, "net add domain 1 igp");
    cbgp_send($cbgp, "net add node 1.0.0.1");
    cbgp_send($cbgp, "net node 1.0.0.1 domain 1");
    cbgp_send($cbgp, "net add node 1.0.0.2");
    cbgp_send($cbgp, "net node 1.0.0.2 domain 1");
    cbgp_send($cbgp, "net add domain 2 igp");
    cbgp_send($cbgp, "net add node 2.0.0.1");
    cbgp_send($cbgp, "net node 2.0.0.1 domain 2");
    cbgp_send($cbgp, "net add subnet 192.168.0/24 transit");
    cbgp_send($cbgp, "net add link 1.0.0.1 1.0.0.2 10");
    cbgp_send($cbgp, "net add link 1.0.0.1 192.168.0.1/24 10");
    cbgp_send($cbgp, "net add link 2.0.0.1 192.168.0.2/24 10");
    cbgp_send($cbgp, "net domain 1 compute");
    cbgp_send($cbgp, "net domain 2 compute");
    cbgp_send($cbgp, "bgp add router 1 1.0.0.1");
    cbgp_send($cbgp, "bgp router 1.0.0.1");
    cbgp_send($cbgp, "\tadd peer 1 1.0.0.2");
    cbgp_send($cbgp, "\tpeer 1.0.0.2 up");
    cbgp_send($cbgp, "\tadd peer 2 192.168.0.2");
    cbgp_send($cbgp, "\tpeer 192.168.0.2 virtual");
    cbgp_send($cbgp, "\tpeer 192.168.0.2 up");
    cbgp_send($cbgp, "\texit");
    cbgp_send($cbgp, "bgp add router 1 1.0.0.2");
    cbgp_send($cbgp, "bgp router 1.0.0.2");
    cbgp_send($cbgp, "\tadd peer 1 1.0.0.1");
    cbgp_send($cbgp, "\tpeer 1.0.0.1 up");
    cbgp_send($cbgp, "\texit");
    cbgp_send($cbgp, "bgp router 1.0.0.1 peer 192.168.0.2 recv \"BGP4|0|A|1.0.0.1|1|255/8|2 3|IGP|192.168.0.2|12|34|\"");
    cbgp_send($cbgp, "sim run");

    $rt= cbgp_show_rt($cbgp, '1.0.0.2', '255/8');
    if (!exists($rt->{'255.0.0.0/8'}) ||
	($rt->{'255.0.0.0/8'}->[CBGP_RT_IFACE] ne '1.0.0.1') ||
	($rt->{'255.0.0.0/8'}->[CBGP_RT_NEXTHOP] ne '0.0.0.0')) {
      return TEST_FAILURE;
    }

    $rt= cbgp_show_rt($cbgp, '1.0.0.1', '255/8');
    if (!exists($rt->{'255.0.0.0/8'}) ||
	($rt->{'255.0.0.0/8'}->[CBGP_RT_IFACE] ne '192.168.0.0/24') ||
	($rt->{'255.0.0.0/8'}->[CBGP_RT_NEXTHOP] ne '192.168.0.2')) {
      return TEST_FAILURE;
    }

    return TEST_SUCCESS;
  }

# -----[ cbgp_valid_bgp_session_router_id ]--------------------------
#
# -------------------------------------------------------------------
sub cbgp_valid_bgp_session_router_id($)
  {
    my ($cbgp)= @_;

    cbgp_send($cbgp, "net add domain 1 igp");
    cbgp_send($cbgp, "net add node 1.0.0.1");
    cbgp_send($cbgp, "net add node 1.0.0.2");
    cbgp_send($cbgp, "net add node 1.0.0.3");
    cbgp_send($cbgp, "net add link 1.0.0.1 1.0.0.2 10");
    cbgp_send($cbgp, "net add link 1.0.0.2 1.0.0.3 10");
    cbgp_send($cbgp, "net domain 1 compute");
    cbgp_send($cbgp, "bgp add router 1 1.0.0.1");
    cbgp_send($cbgp, "bgp router 1.0.0.1");
    cbgp_send($cbgp, "\tadd peer 1 1.0.0.2");
    cbgp_send($cbgp, "\tpeer 1.0.0.2 up");
    cbgp_send($cbgp, "\texit");
    cbgp_send($cbgp, "bgp add router 3 1.0.0.3");
    cbgp_send($cbgp, "bgp router 1.0.0.3");
    cbgp_send($cbgp, "\tadd peer 1 1.0.0.2");
    cbgp_send($cbgp, "\tpeer 1.0.0.2 up");
    cbgp_send($cbgp, "\texit");

    return TEST_SUCCESS;
  }

# -----[ cbgp_valid_bgp_session_sm ]---------------------------------
# This test validate the BGP session state-machine. This is not an
# exhaustive test, however it covers most of the situations.
#
# Setup:
#   - R1 (1.0.0.1, AS1)
#   - R2 (1.0.0.2, AS1)
#
# Topology:
#   R1 ----- R2
#
# Scenario:
#   * create the topology, but do not compute the routes between
#     R1 and R2, the BGP session is configured but not running
#   * check that both peerings are in state IDLE
#   * set peering from R1 to R2 as running
#   * check that R1 -> R2 is in state ACTIVE (since route does not
#     exist)
#   * compute routes between R1 and R2 and rerun peering R1 -> R2
#   * check that R1 -> R2 is in state OPENWAIT
#   * set peering R2 -> R2 as running
#   * check that R2 -> R1 is in state OPENWAIT
#   * run the simulation (OPEN messages should be exchanged)
#   * check that both peerings are in state ESTABLISHED
#   * close peering R1 -> R2
#   * check that R1 -> R2 is in state IDLE
#   * run the simulation
#   * check that R2 -> R1 is in state ACTIVE
#   * re-run peering R1 -> R2 and run the simulation
#   * check that both peerings are in state ESTABLISHED
#   * close peering R1 -> R2, run the simulation, then close R2 -> R1
#   * check that peering R2 -> R1 is in state IDLE
# -------------------------------------------------------------------
sub cbgp_valid_bgp_session_sm()
  {
    my ($cbgp)= @_;
    cbgp_send($cbgp, "net add domain 1 igp");
    cbgp_send($cbgp, "net add node 1.0.0.1");
    cbgp_send($cbgp, "net node 1.0.0.1 domain 1");
    cbgp_send($cbgp, "net add node 1.0.0.2");
    cbgp_send($cbgp, "net node 1.0.0.2 domain 1");
    cbgp_send($cbgp, "net add link 1.0.0.1 1.0.0.2 10");
    cbgp_send($cbgp, "bgp add router 1 1.0.0.1");
    cbgp_send($cbgp, "bgp router 1.0.0.1 add peer 1 1.0.0.2");
    cbgp_send($cbgp, "bgp add router 1 1.0.0.2");
    cbgp_send($cbgp, "bgp router 1.0.0.2 add peer 1 1.0.0.1");

    # At this point, both peerings should be in state IDLE
    if (!cbgp_check_peering($cbgp, "1.0.0.1", "1.0.0.2", "IDLE") ||
	!cbgp_check_peering($cbgp, "1.0.0.2", "1.0.0.1", "IDLE")) {
      $tests->debug("Error: peerings should be in state IDLE");
      return TEST_FAILURE;
    }

    cbgp_send($cbgp, "bgp router 1.0.0.1 peer 1.0.0.2 up");
    # At this point, peering R1 -> R2 should be in state ACTIVE
    # because there is no route from R1 to R2
    if (!cbgp_check_peering($cbgp, "1.0.0.1", "1.0.0.2", "ACTIVE")) {
      $tests->debug("Error: peering R1 -> R2  should be in state ACTIVE");
      return TEST_FAILURE;
    }

    cbgp_send($cbgp, "net domain 1 compute");
    cbgp_send($cbgp, "bgp router 1.0.0.1 rescan");
    # At this point, peering R1 -> R2 should be in state OPENWAIT
    if (!cbgp_check_peering($cbgp, "1.0.0.1", "1.0.0.2", "OPENWAIT")) {
      $tests->debug("Error: peering R1 -> R2 should be in state OPENWAIT");
      return TEST_FAILURE;
    }

    cbgp_send($cbgp, "bgp router 1.0.0.2 peer 1.0.0.1 up");
    # At this point, peering R2 -> R1 should also be in state OPENWAIT
    if (!cbgp_check_peering($cbgp, "1.0.0.2", "1.0.0.1", "OPENWAIT")) {
      $tests->debug("Error: peering R2 -> R1 should be in state OPENWAIT");
      return TEST_FAILURE;
    }

    cbgp_send($cbgp, "sim run");
    # At this point, both peerings should be in state ESTABLISHED
    if (!cbgp_check_peering($cbgp, "1.0.0.1", "1.0.0.2", "ESTABLISHED") ||
	!cbgp_check_peering($cbgp, "1.0.0.2", "1.0.0.1", "ESTABLISHED")) {
      $tests->debug("Error: both peerings should be in state ESTABLISHED");
      return TEST_FAILURE;
    }

    cbgp_send($cbgp, "bgp router 1.0.0.1 peer 1.0.0.2 down");
    # At this point, peering R1 -> R2 should be in state IDLE
    if (!cbgp_check_peering($cbgp, "1.0.0.1", "1.0.0.2", "IDLE")) {
      $tests->debug("Error: peering R1 -> R2 should be in state IDLE");
      return TEST_FAILURE;
    }

    cbgp_send($cbgp, "sim run");
    # At this point, peering R2 -> R1 should be in state ACTIVE
    if (!cbgp_check_peering($cbgp, "1.0.0.2", "1.0.0.1", "ACTIVE")) {
      $tests->debug("Error: peering R2 -> R1 should be in state ACTIVE");
      return TEST_FAILURE;
    }

    cbgp_send($cbgp, "bgp router 1.0.0.1 peer 1.0.0.2 up");
    cbgp_send($cbgp, "sim run");
    # At this point, both peerings should be in state ESTABLISHED
    if (!cbgp_check_peering($cbgp, "1.0.0.1", "1.0.0.2", "ESTABLISHED") ||
	!cbgp_check_peering($cbgp, "1.0.0.2", "1.0.0.1", "ESTABLISHED")) {
      $tests->debug("Error: both peerings should be in state ESTABLISHED");
      return TEST_FAILURE;
    }

    cbgp_send($cbgp, "bgp router 1.0.0.1 peer 1.0.0.2 down");
    cbgp_send($cbgp, "sim run");
    cbgp_send($cbgp, "bgp router 1.0.0.2 peer 1.0.0.1 down");
    # At this point, peering R2 -> R1 should be in state IDLE
    if (!cbgp_check_peering($cbgp, "1.0.0.2", "1.0.0.1", "IDLE")) {
      $tests->debug("Error: peering R2 -> R1 should be in state IDLE");
      return TEST_FAILURE;
    }

    return TEST_SUCCESS;
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
#   - R1 (1.0.0.1, AS1)
#   - R2 (1.0.0.2, AS1)
#   - R3 (1.0.0.3, AS1)
#
# Topology:
#   R1 ----- R2 ----- R3
#
# Scenario:
#   * setup topology, routers and sessions
#   * check that all sessions are in state IDLE
#   * start all BGP routers
#   * check that all sessions are in state OPENWAIT
#   * run the simulation
#   * check that all sessions are in state ESTABLISHED
#   * stop all BGP routers, run the simulation
#   * check that all sessions are in state IDLE
# -------------------------------------------------------------------
sub cbgp_valid_bgp_router_up_down($)
  {
    my ($cbgp)= @_;

    cbgp_send($cbgp, "net add domain 1 igp");
    cbgp_send($cbgp, "net add node 1.0.0.1");
    cbgp_send($cbgp, "net node 1.0.0.1 domain 1");
    cbgp_send($cbgp, "net add node 1.0.0.2");
    cbgp_send($cbgp, "net node 1.0.0.2 domain 1");
    cbgp_send($cbgp, "net add node 1.0.0.3");
    cbgp_send($cbgp, "net node 1.0.0.3 domain 1");
    cbgp_send($cbgp, "net add link 1.0.0.1 1.0.0.2 10");
    cbgp_send($cbgp, "net add link 1.0.0.2 1.0.0.3 10");
    cbgp_send($cbgp, "net domain 1 compute");
    cbgp_send($cbgp, "bgp add router 1 1.0.0.1");
    cbgp_send($cbgp, "bgp router 1.0.0.1");
    cbgp_send($cbgp, "\tadd network 255/8");
    cbgp_send($cbgp, "\tadd peer 1 1.0.0.2");
    cbgp_send($cbgp, "\texit");
    cbgp_send($cbgp, "bgp add router 1 1.0.0.2");
    cbgp_send($cbgp, "bgp router 1.0.0.2");
    cbgp_send($cbgp, "\tadd peer 1 1.0.0.1");
    cbgp_send($cbgp, "\tadd peer 1 1.0.0.3");
    cbgp_send($cbgp, "\texit");
    cbgp_send($cbgp, "bgp add router 1 1.0.0.3");
    cbgp_send($cbgp, "bgp router 1.0.0.3");
    cbgp_send($cbgp, "\tadd network 255/8");
    cbgp_send($cbgp, "\tadd peer 1 1.0.0.2");
    cbgp_send($cbgp, "\texit");

    # All sessions should be in IDLE state
    if (!cbgp_check_peering($cbgp, "1.0.0.1", "1.0.0.2", "IDLE") ||
	!cbgp_check_peering($cbgp, "1.0.0.2", "1.0.0.1", "IDLE") ||
	!cbgp_check_peering($cbgp, "1.0.0.2", "1.0.0.3", "IDLE") ||
	!cbgp_check_peering($cbgp, "1.0.0.3", "1.0.0.2", "IDLE")) {
      $tests->debug("Error: all sessions should be in IDLE state");
      return TEST_FAILURE;
    }

    cbgp_send($cbgp, "bgp router 1.0.0.1 start");
    cbgp_send($cbgp, "bgp router 1.0.0.2 start");
    cbgp_send($cbgp, "bgp router 1.0.0.3 start");
    # All sessions should be in OPENWAIT state
    if (!cbgp_check_peering($cbgp, "1.0.0.1", "1.0.0.2", "OPENWAIT") ||
	!cbgp_check_peering($cbgp, "1.0.0.2", "1.0.0.1", "OPENWAIT") ||
	!cbgp_check_peering($cbgp, "1.0.0.2", "1.0.0.3", "OPENWAIT") ||
	!cbgp_check_peering($cbgp, "1.0.0.3", "1.0.0.2", "OPENWAIT")) {
      $tests->debug("Error: all sessions should be in OPENWAIT state");
      return TEST_FAILURE;
    }

    cbgp_send($cbgp, "sim run");
    # All sessions should be in ESTABLISHED state
    if (!cbgp_check_peering($cbgp, "1.0.0.1", "1.0.0.2", "ESTABLISHED") ||
	!cbgp_check_peering($cbgp, "1.0.0.2", "1.0.0.1", "ESTABLISHED") ||
	!cbgp_check_peering($cbgp, "1.0.0.2", "1.0.0.3", "ESTABLISHED") ||
	!cbgp_check_peering($cbgp, "1.0.0.3", "1.0.0.2", "ESTABLISHED")) {
      $tests->debug("Error: all sessions should be in ESTABLISHED state");
      return TEST_FAILURE;
    }

    cbgp_send($cbgp, "bgp router 1.0.0.1 stop");
    cbgp_send($cbgp, "bgp router 1.0.0.2 stop");
    cbgp_send($cbgp, "bgp router 1.0.0.3 stop");
    cbgp_send($cbgp, "sim run");
    # All sessions should be in IDLE state
    if (!cbgp_check_peering($cbgp, "1.0.0.1", "1.0.0.2", "IDLE") ||
	!cbgp_check_peering($cbgp, "1.0.0.2", "1.0.0.1", "IDLE") ||
	!cbgp_check_peering($cbgp, "1.0.0.2", "1.0.0.3", "IDLE") ||
	!cbgp_check_peering($cbgp, "1.0.0.3", "1.0.0.2", "IDLE")) {
      $tests->debug("Error: all sessions should be in IDLE state");
      return TEST_FAILURE;
    }

    return TEST_SUCCESS;
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

# -----[ cbgp_valid_bgp_record_route ]-------------------------------
# Create the given topology in C-BGP. Check that bgp record-route
# fails. Then, propagate the IGP routes. Check that record-route are
# successfull.
#
# TODO: check loop detection.
# -------------------------------------------------------------------
sub cbgp_valid_bgp_record_route($)
  {
    my ($cbgp)= @_;

    my $topo= topo_3nodes_line();
    cbgp_topo($cbgp, $topo, 1);
    cbgp_topo_igp_compute($cbgp, $topo, 1);
    cbgp_send($cbgp, "bgp add router 1 1.0.0.1");
    cbgp_send($cbgp, "bgp router 1.0.0.1");
    cbgp_send($cbgp, "  add network 255/8");
    cbgp_send($cbgp, "  add peer 2 1.0.0.2");
    cbgp_send($cbgp, "  peer 1.0.0.2 up");
    cbgp_send($cbgp, "  exit");
    cbgp_send($cbgp, "bgp add router 2 1.0.0.2");
    cbgp_send($cbgp, "bgp router 1.0.0.2");
    cbgp_send($cbgp, "  add peer 1 1.0.0.1");
    cbgp_send($cbgp, "  peer 1.0.0.1 up");
    cbgp_send($cbgp, "  add peer 3 1.0.0.3");
    cbgp_send($cbgp, "  peer 1.0.0.3 up");
    cbgp_send($cbgp, "  exit");
    cbgp_send($cbgp, "bgp add router 3 1.0.0.3");
    cbgp_send($cbgp, "bgp router 1.0.0.3");
    cbgp_send($cbgp, "  add peer 2 1.0.0.2");
    cbgp_send($cbgp, "  peer 1.0.0.2 up");
    cbgp_send($cbgp, "  exit");

    my $trace= cbgp_bgp_record_route($cbgp, "1.0.0.3", "255/8");
    if (!defined($trace) ||
	($trace->[CBGP_TR_STATUS] ne "UNREACHABLE")) {
      return TEST_FAILURE;
    }

    cbgp_send($cbgp, "sim run");

    $trace= cbgp_bgp_record_route($cbgp, "1.0.0.3", "255/8");
    if (!defined($trace) ||
	($trace->[CBGP_TR_STATUS] ne "SUCCESS") ||
	!aspath_equals($trace->[CBGP_TR_PATH], [3, 2, 1])) {
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
    die if $cbgp->send("net link 1.0.0.1 2.0.0.1 igp-weight 10\n");
    die if $cbgp->send("net link 2.0.0.1 1.0.0.1 igp-weight 10\n");
    die if $cbgp->send("net add link 1.0.0.1 3.0.0.1 10\n");
    die if $cbgp->send("net link 1.0.0.1 3.0.0.1 igp-weight 10\n");
    die if $cbgp->send("net link 3.0.0.1 1.0.0.1 igp-weight 10\n");
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
    while (<MRT_RECORD>) {
      $line_count++;
    }
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

    cbgp_send($cbgp, "bgp route-map RM_LP_100");
    cbgp_send($cbgp, "  exit");
    my $msg= cbgp_check_error($cbgp, "bgp route-map RM_LP_100");
    if (!defined($msg) ||
	!($msg =~ /$CBGP_ERROR_ROUTEMAP_EXISTS/)) {
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

# -----[ cbgp_valid_bgp_rr_set_cluster_id ]--------------------------
# Check that the cluster-ID can be defined correctly (including
# large cluster-ids).
# -------------------------------------------------------------------
sub cbgp_valid_bgp_rr_set_cluster_id($)
  {
    my ($cbgp)= @_;

    cbgp_send($cbgp, "net add node 0.1.0.0");
    cbgp_send($cbgp, "bgp add router 1 0.1.0.0");
    my $msg= cbgp_check_error($cbgp,
			      "bgp router 0.1.0.0 set cluster-id 1");
    if (defined($msg)) {
      return TEST_FAILURE;
    }

    # Maximum value (2^32)-1
    $msg= cbgp_check_error($cbgp,
			   "bgp router 0.1.0.0 set cluster-id 4294967295");
    if (defined($msg)) {
      return TEST_FAILURE;
    }

    # Larger than (2^32)-1
    $msg= cbgp_check_error($cbgp,
			   "bgp router 0.1.0.0 set cluster-id 4294967296");
    if (!defined($msg) ||
	!($msg =~ /$CBGP_ERROR_CLUSTERID_INVALID/)) {
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
    while (<MRT_RECORD>) {
      $line_count++;
    }
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
    while (<MRT_RECORD>) {
      $line_count++;
    }
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

# -----[ cbgp_valid_net_tm_load ]------------------------------------
# Check the ability to load a traffic matrix from a file and to load
# the links traversed by traffic.
#
# Setup:
#   - R1 (1.0.0.1)
#   - R2 (1.0.0.2)
#   - R3 (1.0.0.3)
#   - R4 (1.0.0.4)
#   - N1 (172.13.16/24)
#   - N2 (138.48/16)
#   - N3 (130.104/16)
#
# Topology:
#
#   R1 ----- R2 ----- R3 -----{N2}
#   |         \
#   |          *----- R4 -----{N1}
#  {N3}
#
# Scenario:
#   * Setup topology and load traffic matrix from file "valid-tm.txt"
#   * Check that load(R1->R1)=1500
#   * Check that load(R2->R3)=1800
#   * Check that load(R3->N2)=1600
#   * Check that load(R2->R4)=1200
#   * Check that load(R4->N1)=800
#   * Check that load(R1->N3)=3200
#   * Check that load(R2->R1)=3200
#
# Resources:
#   [valid-net-tm.tm]
# -------------------------------------------------------------------
sub cbgp_valid_net_tm_load($)
  {
    my ($cbgp)= @_;
    my $msg;
    my $info;

    cbgp_send($cbgp, "net add domain 1 igp");
    cbgp_send($cbgp, "net add subnet 172.13.16/24 stub");
    cbgp_send($cbgp, "net add subnet 138.48/16 stub");
    cbgp_send($cbgp, "net add subnet 130.104/16 stub");
    cbgp_send($cbgp, "net add node 1.0.0.1");
    cbgp_send($cbgp, "net node 1.0.0.1 domain 1");
    cbgp_send($cbgp, "net add node 1.0.0.2");
    cbgp_send($cbgp, "net node 1.0.0.2 domain 1");
    cbgp_send($cbgp, "net add node 1.0.0.3");
    cbgp_send($cbgp, "net node 1.0.0.3 domain 1");
    cbgp_send($cbgp, "net add node 1.0.0.4");
    cbgp_send($cbgp, "net node 1.0.0.4 domain 1");
    cbgp_send($cbgp, "net add link 1.0.0.1 1.0.0.2 10");
    cbgp_send($cbgp, "net link 1.0.0.1 1.0.0.2 igp-weight --bidir 10");
    cbgp_send($cbgp, "net add link 1.0.0.2 1.0.0.3 10");
    cbgp_send($cbgp, "net link 1.0.0.2 1.0.0.3 igp-weight --bidir 10");
    cbgp_send($cbgp, "net add link 1.0.0.2 1.0.0.4 10");
    cbgp_send($cbgp, "net link 1.0.0.2 1.0.0.4 igp-weight --bidir 10");
    cbgp_send($cbgp, "net add link 1.0.0.1 130.104.10.10/16 10");
    cbgp_send($cbgp, "net link 1.0.0.1 130.104.10.10/16 igp-weight 10");
    cbgp_send($cbgp, "net add link 1.0.0.3 138.48.4.4/16 10");
    cbgp_send($cbgp, "net link 1.0.0.3 138.48.4.4/16 igp-weight 10");
    cbgp_send($cbgp, "net add link 1.0.0.4 172.13.16.1/24 10");
    cbgp_send($cbgp, "net link 1.0.0.4 172.13.16.1/24 igp-weight 10");
    cbgp_send($cbgp, "net domain 1 compute");

    $msg= cbgp_check_error($cbgp, "net tm load \"".$data_resources."valid-net-tm.tm\"");
    if (defined($msg)) {
      $tests->debug("Error: \"net tm load\" generated an error.");
      return TEST_FAILURE;
    }

    $info= cbgp_net_link_info($cbgp, "1.0.0.1", "1.0.0.2");
    if (!exists($info->{load}) || ($info->{load} != 1500)) {
      $tests->debug("Error: no/invalid load for R1->R2");
      return TEST_FAILURE;
    }

    $info= cbgp_net_link_info($cbgp, "1.0.0.2", "1.0.0.3");
    if (!exists($info->{load}) || ($info->{load} != 1800)) {
      $tests->debug("Error: no/invalid load for R2->R3");
      return TEST_FAILURE;
    }

    $info= cbgp_net_link_info($cbgp, "1.0.0.3", "138.48.4.4/16");
    if (!exists($info->{load}) || ($info->{load} != 1600)) {
      $tests->debug("Error: no/invalid load for R3->N2");
      return TEST_FAILURE;
    }

    $info= cbgp_net_link_info($cbgp, "1.0.0.2", "1.0.0.4");
    if (!exists($info->{load}) || ($info->{load} != 1200)) {
      $tests->debug("Error: no/invalid load for R2->R4");
      return TEST_FAILURE;
    }

    $info= cbgp_net_link_info($cbgp, "1.0.0.4", "172.13.16.1/24");
    if (!exists($info->{load}) || ($info->{load} != 800)) {
      $tests->debug("Error: no/invalid load for R4->N1");
      return TEST_FAILURE;
    }

    $info= cbgp_net_link_info($cbgp, "1.0.0.1", "130.104.10.10/16");
    if (!exists($info->{load}) || ($info->{load} != 3200)) {
      $tests->debug("Error: no/invalid load for R1->N3");
      return TEST_FAILURE;
    }

    $info= cbgp_net_link_info($cbgp, "1.0.0.2", "1.0.0.1");
    if (!exists($info->{load}) || ($info->{load} != 3200)) {
      $tests->debug("Error: no/invalid load for R2->R1");
      return TEST_FAILURE;
    }

    return TEST_SUCCESS;
  }

# -----[ cbgp_valid_abilene ]----------------------------------------
#
# -------------------------------------------------------------------
sub cbgp_valid_abilene($)
  {
    my ($cbgp)= @_;

    cbgp_send($cbgp, "bgp options auto-create on");

    cbgp_send($cbgp, "net add domain 11537 igp");
    cbgp_send($cbgp, "net add node 198.32.12.9");
    cbgp_send($cbgp, "net node 198.32.12.9 domain 11537");
    cbgp_send($cbgp, "net node 198.32.12.9 name \"ATLA\"");
    cbgp_send($cbgp, "net add node 198.32.12.25");
    cbgp_send($cbgp, "net node 198.32.12.25 domain 11537");
    cbgp_send($cbgp, "net node 198.32.12.25 name \"CHIN\"");
    cbgp_send($cbgp, "net add node 198.32.12.41");
    cbgp_send($cbgp, "net node 198.32.12.41 domain 11537");
    cbgp_send($cbgp, "net node 198.32.12.41 name \"DNVR\"");
    cbgp_send($cbgp, "net add node 198.32.12.57");
    cbgp_send($cbgp, "net node 198.32.12.57 domain 11537");
    cbgp_send($cbgp, "net node 198.32.12.57 name \"HSTN\"");
    cbgp_send($cbgp, "net add node 198.32.12.177");
    cbgp_send($cbgp, "net node 198.32.12.177 domain 11537");
    cbgp_send($cbgp, "net node 198.32.12.177 name \"IPLS\"");
    cbgp_send($cbgp, "net add node 198.32.12.89");
    cbgp_send($cbgp, "net node 198.32.12.89 domain 11537");
    cbgp_send($cbgp, "net node 198.32.12.89 name \"KSCY\"");
    cbgp_send($cbgp, "net add node 198.32.12.105");
    cbgp_send($cbgp, "net node 198.32.12.105 domain 11537");
    cbgp_send($cbgp, "net node 198.32.12.105 name \"LOSA\"");
    cbgp_send($cbgp, "net add node 198.32.12.121");
    cbgp_send($cbgp, "net node 198.32.12.121 domain 11537");
    cbgp_send($cbgp, "net node 198.32.12.121 name \"NYCM\"");
    cbgp_send($cbgp, "net add node 198.32.12.137");
    cbgp_send($cbgp, "net node 198.32.12.137 domain 11537");
    cbgp_send($cbgp, "net node 198.32.12.137 name \"SNVA\"");
    cbgp_send($cbgp, "net add node 198.32.12.153");
    cbgp_send($cbgp, "net node 198.32.12.153 domain 11537");
    cbgp_send($cbgp, "net node 198.32.12.153 name \"STTL\"");
    cbgp_send($cbgp, "net add node 198.32.12.169");
    cbgp_send($cbgp, "net node 198.32.12.169 domain 11537");
    cbgp_send($cbgp, "net node 198.32.12.169 name \"WASH\"");

    # Internal links
    cbgp_send($cbgp, "net add link 198.32.12.137 198.32.12.41 1295");
    cbgp_send($cbgp, "net add link 198.32.12.137 198.32.12.105 366");
    cbgp_send($cbgp, "net add link 198.32.12.137 198.32.12.153 861");
    cbgp_send($cbgp, "net add link 198.32.12.41 198.32.12.153 2095");
    cbgp_send($cbgp, "net add link 198.32.12.153 198.32.12.89 639");
    cbgp_send($cbgp, "net add link 198.32.12.105 198.32.12.57 1893");
    cbgp_send($cbgp, "net add link 198.32.12.89 198.32.12.57 902");
    cbgp_send($cbgp, "net add link 198.32.12.89 198.32.12.177 548");
    cbgp_send($cbgp, "net add link 198.32.12.57 198.32.12.9 1176");
    cbgp_send($cbgp, "net add link 198.32.12.9 198.32.12.177 587");
    cbgp_send($cbgp, "net add link 198.32.12.9 198.32.12.169 846");
    cbgp_send($cbgp, "net add link 198.32.12.25 198.32.12.177 260");
    cbgp_send($cbgp, "net add link 198.32.12.25 198.32.12.121 700");
    cbgp_send($cbgp, "net add link 198.32.12.169 198.32.12.121 233");

    # Compute IGP routes
    cbgp_send($cbgp, "net domain 11537 compute");
    
    cbgp_send($cbgp, "bgp add router 11537 198.32.12.9");
    cbgp_send($cbgp, "bgp add router 11537 198.32.12.25");
    cbgp_send($cbgp, "bgp add router 11537 198.32.12.41");
    cbgp_send($cbgp, "bgp add router 11537 198.32.12.57");
    cbgp_send($cbgp, "bgp add router 11537 198.32.12.177");
    cbgp_send($cbgp, "bgp add router 11537 198.32.12.89");
    cbgp_send($cbgp, "bgp add router 11537 198.32.12.105");
    cbgp_send($cbgp, "bgp add router 11537 198.32.12.121");
    cbgp_send($cbgp, "bgp add router 11537 198.32.12.137");
    cbgp_send($cbgp, "bgp add router 11537 198.32.12.153");
    cbgp_send($cbgp, "bgp add router 11537 198.32.12.169");

    cbgp_send($cbgp, "bgp domain 11537 full-mesh");

    cbgp_send($cbgp, "sim run");

    cbgp_send($cbgp, "bgp router 198.32.12.9 load rib \"".$resources_path."abilene/ATLA/rib.20050801.0124-ebgp.ascii\"");
    cbgp_send($cbgp, "bgp router 198.32.12.25 load rib \"".$resources_path."abilene/CHIN/rib.20050801.0017-ebgp.ascii\"");
    cbgp_send($cbgp, "bgp router 198.32.12.41 load rib \"".$resources_path."abilene/DNVR/rib.20050801.0014-ebgp.ascii\"");
    cbgp_send($cbgp, "bgp router 198.32.12.57 load rib \"".$resources_path."abilene/HSTN/rib.20050801.0149-ebgp.ascii\"");
    cbgp_send($cbgp, "bgp router 198.32.12.177 load rib \"".$resources_path."abilene/IPLS/rib.20050801.0157-ebgp.ascii\"");
    cbgp_send($cbgp, "bgp router 198.32.12.89 load rib \"".$resources_path."abilene/KSCY/rib.20050801.0154-ebgp.ascii\"");
    cbgp_send($cbgp, "bgp router 198.32.12.105 load rib \"".$resources_path."abilene/LOSA/rib.20050801.0034-ebgp.ascii\"");
    cbgp_send($cbgp, "bgp router 198.32.12.121 load rib \"".$resources_path."abilene/NYCM/rib.20050801.0050-ebgp.ascii\"");
    cbgp_send($cbgp, "bgp router 198.32.12.137 load rib \"".$resources_path."abilene/SNVA/rib.20050801.0118-ebgp.ascii\"");
    cbgp_send($cbgp, "bgp router 198.32.12.153 load rib \"".$resources_path."abilene/STTL/rib.20050801.0124-ebgp.ascii\"");
    cbgp_send($cbgp, "bgp router 198.32.12.169 load rib \"".$resources_path."abilene/WASH/rib.20050801.0101-ebgp.ascii\"");

    cbgp_send($cbgp, "sim run");

    my $rib;
    $rib= cbgp_show_rib($cbgp, "198.32.12.9");
    $rib= cbgp_show_rib($cbgp, "198.32.12.25");
    $rib= cbgp_show_rib($cbgp, "198.32.12.41");
    $rib= cbgp_show_rib($cbgp, "198.32.12.57");
    $rib= cbgp_show_rib($cbgp, "198.32.12.177");
    $rib= cbgp_show_rib($cbgp, "198.32.12.89");
    $rib= cbgp_show_rib($cbgp, "198.32.12.105");
    $rib= cbgp_show_rib($cbgp, "198.32.12.121");
    $rib= cbgp_show_rib($cbgp, "198.32.12.137");
    $rib= cbgp_show_rib($cbgp, "198.32.12.153");
    $rib= cbgp_show_rib($cbgp, "198.32.12.169");

    return TEST_SUCCESS;
  }

# -----[ cbgp_valid_nasty_bgp_peering_1w_down ]----------------------
# Test that C-BGP detects that a session is down even when one
# direction still works.
#
# Setup:
#   - R1 (0.1.0.0, AS1)
#   - R2 (0.2.0.0, AS2)
#
# Topology:
#
#       eBGP
#   R1 ------ R2
#
# Scenario:
#   * Establish the BGP session
#   * Remove the route from R1 to R2
#   * Refresh BGP session state
#   * Check that the session is in ACTIVE state on both ends
# -------------------------------------------------------------------
sub cbgp_valid_nasty_bgp_peering_1w_down($)
  {
    my ($cbgp)= @_;

    cbgp_send($cbgp, "net add node 0.1.0.0");
    cbgp_send($cbgp, "net add node 0.2.0.0");
    cbgp_send($cbgp, "net add link 0.1.0.0 0.2.0.0 10");
    cbgp_send($cbgp, "net node 0.1.0.0 route add 0.2/16 * 0.2.0.0 10");
    cbgp_send($cbgp, "net node 0.2.0.0 route add 0.1/16 * 0.1.0.0 10");

    cbgp_send($cbgp, "bgp add router 1 0.1.0.0");
    cbgp_send($cbgp, "bgp router 0.1.0.0");
    cbgp_send($cbgp, "\tadd peer 2 0.2.0.0");
    cbgp_send($cbgp, "\tpeer 0.2.0.0 up");
    cbgp_send($cbgp, "\texit");

    cbgp_send($cbgp, "bgp add router 2 0.2.0.0");
    cbgp_send($cbgp, "bgp router 0.2.0.0");
    cbgp_send($cbgp, "\tadd peer 1 0.1.0.0");
    cbgp_send($cbgp, "\tpeer 0.1.0.0 up");
    cbgp_send($cbgp, "\texit");

    cbgp_send($cbgp, "sim run");

    my $peers= cbgp_show_peers($cbgp, "0.1.0.0");
    if (!exists($peers->{'0.2.0.0'}) ||
	($peers->{'0.2.0.0'}->[CBGP_PEER_STATE] ne 'ESTABLISHED')) {
      return TEST_FAILURE;
    }
    $peers= cbgp_show_peers($cbgp, "0.2.0.0");
    if (!exists($peers->{'0.1.0.0'}) ||
	($peers->{'0.1.0.0'}->[CBGP_PEER_STATE] ne 'ESTABLISHED')) {
      return TEST_FAILURE;
    }

    cbgp_send($cbgp, "net node 0.1.0.0 route del 0.2/16 * *");
    cbgp_send($cbgp, "bgp router 0.1.0.0 rescan");
    cbgp_send($cbgp, "bgp router 0.2.0.0 rescan");

    $peers= cbgp_show_peers($cbgp, "0.1.0.0");
    if (!exists($peers->{'0.2.0.0'}) ||
	($peers->{'0.2.0.0'}->[CBGP_PEER_STATE] ne 'ACTIVE')) {
      return TEST_FAILURE;
    }
    $peers= cbgp_show_peers($cbgp, "0.2.0.0");
    if (!exists($peers->{'0.1.0.0'}) ||
	($peers->{'0.1.0.0'}->[CBGP_PEER_STATE] ne 'ACTIVE')) {
      return TEST_FAILURE;
    }

    return TEST_SUCCESS;
  }

# -----[ cbgp_valid_nasty_bgp_session_segment_ordering ]-------------
#
# -------------------------------------------------------------------
sub cbgp_valid_nasty_bgp_session_segment_ordering($)
  {
    my ($cbgp)= @_;

    cbgp_send($cbgp, "net add domain 1 igp");
    cbgp_send($cbgp, "net add node 1.0.0.1");
    cbgp_send($cbgp, "net node 1.0.0.1 domain 1");
    cbgp_send($cbgp, "net add node 1.0.0.2");
    cbgp_send($cbgp, "net node 1.0.0.2 domain 1");
    cbgp_send($cbgp, "net add node 1.0.0.3");
    cbgp_send($cbgp, "net node 1.0.0.3 domain 1");
    cbgp_send($cbgp, "net add link 1.0.0.1 1.0.0.2 2");
    cbgp_send($cbgp, "net link 1.0.0.1 1.0.0.2 igp-weight --bidir 2");
    cbgp_send($cbgp, "net add link 1.0.0.1 1.0.0.3 10");
    cbgp_send($cbgp, "net link 1.0.0.1 1.0.0.3 igp-weight --bidir 10");
    cbgp_send($cbgp, "net add link 1.0.0.2 1.0.0.3 10");
    cbgp_send($cbgp, "net link 1.0.0.2 1.0.0.3 igp-weight --bidir 10");
    cbgp_send($cbgp, "net domain 1 compute");

    cbgp_send($cbgp, "bgp add router 1 1.0.0.1");
    cbgp_send($cbgp, "bgp router 1.0.0.1");
    cbgp_send($cbgp, "\tadd peer 1 1.0.0.2");
    cbgp_send($cbgp, "\tpeer 1.0.0.2 up");
    cbgp_send($cbgp, "\texit");

    cbgp_send($cbgp, "bgp add router 1 1.0.0.2");
    cbgp_send($cbgp, "bgp router 1.0.0.2");
    cbgp_send($cbgp, "\tadd peer 1 1.0.0.1");
    cbgp_send($cbgp, "\tpeer 1.0.0.1 up");
    cbgp_send($cbgp, "\texit");

    cbgp_send($cbgp, "bgp router 1.0.0.1 add network 254/8");
    cbgp_send($cbgp, "sim run");

    cbgp_send($cbgp, "net node 1.0.0.1 route add 1.0.0.2/32 * 1.0.0.3 1");

    cbgp_send($cbgp, "bgp router 1.0.0.1 del network 254/8");
    cbgp_send($cbgp, "net node 1.0.0.1 route del 1.0.0.2/32 * 1.0.0.3");
    cbgp_send($cbgp, "bgp router 1.0.0.1 add network 254/8");
    cbgp_send($cbgp, "sim run");

    return TEST_FAILURE;
  }

# -----[ cbgp_valid_nasty_bgp_med_ordering ]-------------------------
#
# -------------------------------------------------------------------
sub cbgp_valid_nasty_bgp_med_ordering($)
  {
    my ($cbgp)= @_;

    cbgp_send($cbgp, "net add domain 1 igp");
    cbgp_send($cbgp, "net add node 1.0.0.1");
    cbgp_send($cbgp, "net node 1.0.0.1 domain 1");
    cbgp_send($cbgp, "net add node 1.0.0.2");
    cbgp_send($cbgp, "net node 1.0.0.2 domain 1");
    cbgp_send($cbgp, "net add node 1.0.0.3");
    cbgp_send($cbgp, "net node 1.0.0.3 domain 1");
    cbgp_send($cbgp, "net add node 1.0.0.4");
    cbgp_send($cbgp, "net node 1.0.0.4 domain 1");
    cbgp_send($cbgp, "net add link 1.0.0.1 1.0.0.2 1");
    cbgp_send($cbgp, "net link 1.0.0.1 1.0.0.2 igp-weight --bidir 20");
    cbgp_send($cbgp, "net add link 1.0.0.1 1.0.0.3 1");
    cbgp_send($cbgp, "net link 1.0.0.1 1.0.0.3 igp-weight --bidir 5");
    cbgp_send($cbgp, "net add link 1.0.0.1 1.0.0.4 1");
    cbgp_send($cbgp, "net link 1.0.0.1 1.0.0.4 igp-weight --bidir 10");
    cbgp_send($cbgp, "net domain 1 compute");

    cbgp_send($cbgp, "bgp add router 1 1.0.0.1");
    cbgp_send($cbgp, "bgp router 1.0.0.1");
    cbgp_send($cbgp, "\tadd peer 2 1.0.0.2");
    cbgp_send($cbgp, "\tpeer 1.0.0.2 up");
    cbgp_send($cbgp, "\tadd peer 2 1.0.0.3");
    cbgp_send($cbgp, "\tpeer 1.0.0.3 up");
    cbgp_send($cbgp, "\tadd peer 3 1.0.0.4");
    cbgp_send($cbgp, "\tpeer 1.0.0.4 up");
    cbgp_send($cbgp, "\texit");

    cbgp_send($cbgp, "bgp add router 2 1.0.0.2");
    cbgp_send($cbgp, "bgp router 1.0.0.2");
    cbgp_send($cbgp, "\tadd network 255/8");
    cbgp_send($cbgp, "\tadd peer 1 1.0.0.1");
    cbgp_send($cbgp, "\tpeer 1.0.0.1");
    cbgp_send($cbgp, "\t\tfilter out");
    cbgp_send($cbgp, "\t\t\tadd-rule");
    cbgp_send($cbgp, "\t\t\t\tmatch any");
    cbgp_send($cbgp, "\t\t\t\taction \"metric 10\"");
    cbgp_send($cbgp, "\t\t\t\texit");
    cbgp_send($cbgp, "\t\t\texit");
    cbgp_send($cbgp, "\t\texit");
    cbgp_send($cbgp, "\tpeer 1.0.0.1 up");
    cbgp_send($cbgp, "\texit");

    cbgp_send($cbgp, "bgp add router 2 1.0.0.3");
    cbgp_send($cbgp, "bgp router 1.0.0.3");
    cbgp_send($cbgp, "\tadd network 255/8");
    cbgp_send($cbgp, "\tadd peer 1 1.0.0.1");
    cbgp_send($cbgp, "\tpeer 1.0.0.1");
    cbgp_send($cbgp, "\t\tfilter out");
    cbgp_send($cbgp, "\t\t\tadd-rule");
    cbgp_send($cbgp, "\t\t\t\tmatch any");
    cbgp_send($cbgp, "\t\t\t\taction \"metric 20\"");
    cbgp_send($cbgp, "\t\t\t\texit");
    cbgp_send($cbgp, "\t\t\texit");
    cbgp_send($cbgp, "\t\texit");
    cbgp_send($cbgp, "\tpeer 1.0.0.1 up");
    cbgp_send($cbgp, "\texit");

    cbgp_send($cbgp, "bgp add router 3 1.0.0.4");
    cbgp_send($cbgp, "bgp router 1.0.0.4");
    cbgp_send($cbgp, "\tadd network 255/8");
    cbgp_send($cbgp, "\tadd peer 1 1.0.0.1");
    cbgp_send($cbgp, "\tpeer 1.0.0.1 up");
    cbgp_send($cbgp, "\texit");

    cbgp_send($cbgp, "sim run");

    my $rib= cbgp_show_rib($cbgp, '1.0.0.1', '255/8');
    if (!exists($rib->{'255.0.0.0/8'}) ||
       ($rib->{'255.0.0.0/8'}->[CBGP_RIB_NEXTHOP] ne '1.0.0.4')) {
      return TEST_FAILURE;
    }

    cbgp_send($cbgp, "net link 1.0.0.1 1.0.0.2 down");
    cbgp_send($cbgp, "net domain 1 compute");
    cbgp_send($cbgp, "bgp router 1.0.0.1 rescan");
    cbgp_send($cbgp, "bgp router 1.0.0.2 rescan");
    cbgp_send($cbgp, "sim run");

    $rib= cbgp_show_rib($cbgp, '1.0.0.1', '255/8');
    if (!exists($rib->{'255.0.0.0/8'}) ||
       ($rib->{'255.0.0.0/8'}->[CBGP_RIB_NEXTHOP] ne '1.0.0.3')) {
      return TEST_FAILURE;
    }

    cbgp_send($cbgp, "net link 1.0.0.1 1.0.0.2 up");
    cbgp_send($cbgp, "net domain 1 compute");
    cbgp_send($cbgp, "bgp router 1.0.0.1 rescan");
    cbgp_send($cbgp, "bgp router 1.0.0.2 rescan");
    cbgp_send($cbgp, "sim run");

    $rib= cbgp_show_rib($cbgp, '1.0.0.1', '255/8');
    if (!exists($rib->{'255.0.0.0/8'}) ||
       ($rib->{'255.0.0.0/8'}->[CBGP_RIB_NEXTHOP] ne '1.0.0.4')) {
      return TEST_FAILURE;
    }

    cbgp_send($cbgp, "bgp router 1.0.0.2 del network 255/8");
    cbgp_send($cbgp, "sim run");

    $rib= cbgp_show_rib($cbgp, '1.0.0.1', '255/8');
    if (!exists($rib->{'255.0.0.0/8'}) ||
       ($rib->{'255.0.0.0/8'}->[CBGP_RIB_NEXTHOP] ne '1.0.0.3')) {
      return TEST_FAILURE;
    }

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
$tests->register("net node name", "cbgp_valid_net_node_name");
$tests->register("net link", "cbgp_valid_net_link", $topo);
$tests->register("net link loop", "cbgp_valid_net_link_loop");
$tests->register("net link duplicate", "cbgp_valid_net_link_duplicate");
$tests->register("net link weight bidir", "cbgp_valid_net_link_weight_bidir");
$tests->register("net link capacity", "cbgp_valid_net_link_capacity");
$tests->register("net subnet", "cbgp_valid_net_subnet", $topo);
$tests->register("net subnet duplicate", "cbgp_valid_net_subnet_duplicate");
$tests->register("net subnet misc", "cbgp_valid_net_subnet_misc", $topo);
$tests->register("net create", "cbgp_valid_net_create", $topo);
$tests->register("net igp", "cbgp_valid_net_igp", $topo);
$tests->register("net igp overflow", "cbgp_valid_net_igp_overflow");
$tests->register("net igp unknown domain",
		 "cbgp_valid_net_igp_unknown_domain");
$tests->register("net igp subnet", "cbgp_valid_net_igp_subnet");
$tests->register("net ntf load", "cbgp_valid_net_ntf_load",
		 $resources_path."valid-record-route.ntf");
$tests->register("net record-route", "cbgp_valid_net_record_route", $topo);
$tests->register("net record-route delay",
		 "cbgp_valid_net_record_route_delay", $topo);
$tests->register("net record-route capacity",
		 "cbgp_valid_net_record_route_capacity");
$tests->register("net record-route load",
		 "cbgp_valid_net_record_route_load");
$tests->register("net record-route loop", "cbgp_valid_net_record_route_loop");
$tests->register("net record-route prefix",
		 "cbgp_valid_net_record_route_prefix");
$tests->register("net icmp ping", "cbgp_valid_net_icmp_ping");
$tests->register("net icmp traceroute", "cbgp_valid_net_icmp_traceroute");
$tests->register("net tunnel", "cbgp_valid_net_tunnel");
$tests->register("net tunnel duplicate", "cbgp_valid_net_tunnel_duplicate");
$tests->register("net record-route tunnel",
		 "cbgp_valid_net_record_route_tunnel");
$tests->register("net static routes", "cbgp_valid_net_static_routes");
$tests->register("net static routes wildcards",
		 "cbgp_valid_net_static_routes_wildcards");
$tests->register("net static routes duplicate",
		 "cbgp_valid_net_static_routes_duplicate");
$tests->register("net static routes errors",
		 "cbgp_valid_net_static_routes_errors");
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
$tests->register("bgp topology subra-2004", "cbgp_valid_bgp_topology_subra2004");
$tests->register("bgp topology small-topo", "cbgp_valid_bgp_topology_smalltopo");
$tests->register("bgp topology large-topo", "cbgp_valid_bgp_topology_largetopo");
$tests->register("bgp session ibgp", "cbgp_valid_bgp_session_ibgp");
$tests->register("bgp session ebgp", "cbgp_valid_bgp_session_ebgp");
$tests->register("bgp session nexthop subnet",
		 "cbgp_valid_bgp_session_nexthop_subnet");
$tests->register("bgp session router-id", "cbgp_valid_bgp_session_router_id");
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
$tests->register("bgp record-route", "cbgp_valid_bgp_record_route");
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
$tests->register("net tm load", "cbgp_valid_net_tm_load");
$tests->register("abilene", "cbgp_valid_abilene");
$tests->register("nasty: bgp peering one-way down",
		 "cbgp_valid_nasty_bgp_peering_1w_down");
#$tests->register("nasty: bgp session segment ordering",
#		 "cbgp_valid_nasty_bgp_session_segment_ordering");
$tests->register("nasty: bgp med ordering",
		 "cbgp_valid_nasty_bgp_med_ordering");

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
  }
  if ($opts{report} eq "xml") {
    CBGPValid::XMLReport::report_write("$report_prefix",
				       $validation,
				       $tests);
    CBGPValid::BaseReport::save_resources("$report_prefix.resources");
  }
}

show_info("done.");
exit($return_value);

