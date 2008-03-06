#!/usr/bin/perl -w
# ===================================================================
# @(#)cbgp-validation.pl
#
# This script is used to validate C-BGP. It performs various tests in
# order to detect erroneous behaviour.
#
# @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
# @lastdate 25/02/2008
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
use CBGPValid::Checking;
use CBGPValid::Functions;
use CBGPValid::HTMLReport;
use CBGPValid::TestConstants;
use CBGPValid::Tests;
use CBGPValid::Topologies;
use CBGPValid::UI;
use CBGPValid::XMLReport;
use POSIX;

use constant CBGP_VALIDATION_VERSION => '1.12';

# -----[ Error messages ]-----
my $CBGP_ERROR_INVALID_SUBNET    = "invalid subnet";
my $CBGP_ERROR_CLUSTERID_INVALID = "invalid cluster-id";
my $CBGP_ERROR_DOMAIN_UNKNOWN    = "unknown domain";
my $CBGP_ERROR_LINK_EXISTS       = "could not add link";
my $CBGP_ERROR_LINK_ENDPOINTS    = "link endpoints are equal";
my $CBGP_ERROR_INVALID_LINK_TAIL = "tail-end .* does not exist";
my $CBGP_ERROR_NODE_EXISTS       = "node already exists";
my $CBGP_ERROR_ROUTE_BAD_IFACE   = "interface is unknown";
my $CBGP_ERROR_ROUTE_EXISTS      = "route already exists";
my $CBGP_ERROR_ROUTE_NH_UNREACH  = "next-hop is unreachable";
my $CBGP_ERROR_ROUTEMAP_EXISTS   = "route-map already exists";
my $CBGP_ERROR_SUBNET_EXISTS     = "already exists";

use constant MAX_IGP_WEIGHT => 4294967295;

use constant MRT_PREFIX => 5;
use constant MRT_NEXTHOP => 8;

# -----[ Expected C-BGP version ]-----
# These are the expected major/minor versions of the C-BGP instance
# we are going to test.If these versions are lower than reported by
# the C-BGP instance, a warning will be issued.
use constant CBGP_MAJOR_MIN => 1;
use constant CBGP_MINOR_MIN => 4;

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
show_info("(c) 2008, Bruno Quoitin (bruno.quoitin\@uclouvain.be)");

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
CBGPValid::Checking::set_tests($tests);
CBGPValid::Topologies::set_tests($tests);

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

# -----[ cbgp_checkpoint ]-------------------------------------------
# Wait until C-BGP has finished previous treatment...
# -------------------------------------------------------------------
sub cbgp_checkpoint($)
  {
    my ($cbgp)= @_;
    $cbgp->send_cmd("print \"CHECKPOINT\\n\"");
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
sub cbgp_check_error($$) {
  my ($cbgp, $cmd)= @_;
  my $result= "";

  $cbgp->send_cmd("set exit-on-error off");
  $cbgp->send_cmd($cmd);
  $cbgp->send_cmd("print \"CHECKPOINT\\n\"");
  while ((my $line= $cbgp->expect(1)) ne "CHECKPOINT") {
    $result.= $line;
  }
  if ($result =~ m/^Error\:(.*)$/) {
    return $1;
  }
  return undef;
}

# -----[ check_has_error ]-------------------------------------------
sub check_has_error($;$) {
  my ($msg, $error)= @_;

  $tests->debug("check_error(".(defined($error)?$error:"").")");
  if (!defined($error)) {
    $tests->debug("no error message found.");
    return 0;
  }
  if (defined($error) && !($msg =~ m/$error/)) {
    $tests->debug("error message does not match.");
    return 0;
  }
  return 1;
}

# -----[ cbgp_topo_domain ]------------------------------------------
sub cbgp_topo_domain($$$$)
  {
    my ($cbgp, $topo, $predicate, $domain)= @_;

    $cbgp->send_cmd("net add domain $domain igp");

    my $nodes= topo_get_nodes($topo);
    foreach my $node (keys %$nodes) {
      if ($node =~ m/$predicate/) {
	$cbgp->send_cmd("net node $node domain $domain");
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

    $cbgp->send_cmd("net add node 1.0.0.1");
    $cbgp->send_cmd("net add node 2.0.0.1");
    $cbgp->send_cmd("net add link 1.0.0.1 2.0.0.1 0");
    $cbgp->send_cmd("net node 1.0.0.1 route add 2.0.0.1/32 * 2.0.0.1 0");
    $cbgp->send_cmd("bgp add router 1 1.0.0.1");
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

    $cbgp->send_cmd("net add node 1.0.0.1");
    $cbgp->send_cmd("bgp add router 1 1.0.0.1");
    for (my $index= 0; $index < $num_peers; $index++) {
      my $asn= $index+2;
      my $peer= "$asn.0.0.1";
      $cbgp->send_cmd("net add node $peer");
      $cbgp->send_cmd("net add link 1.0.0.1 $peer 0");
      $cbgp->send_cmd("net node 1.0.0.1 route add $peer/32 * $peer 0");
      cbgp_peering($cbgp, "1.0.0.1", $peer, $asn, "virtual");
    }
  }

# -----[ cbgp_topo_dp2 ]---------------------------------------------
sub cbgp_topo_dp2($$)
  {
    my ($cbgp, $num_peers)= @_;

    $cbgp->send_cmd("net add node 1.0.0.1");
    $cbgp->send_cmd("bgp add router 1 1.0.0.1");
    for (my $index= 0; $index < $num_peers; $index++) {
      my $peer= "1.0.0.".($index+2);
      $cbgp->send_cmd("net add node $peer");
      $cbgp->send_cmd("net add link 1.0.0.1 $peer ".($index+1)."");
      $cbgp->send_cmd("net node 1.0.0.1 route add $peer/32 * $peer 0");
      cbgp_peering($cbgp, "1.0.0.1", $peer, "1", "virtual");
    }
  }

# -----[ cbgp_topo_dp3 ]---------------------------------------------
sub cbgp_topo_dp3($;@)
  {
    my ($cbgp, @peers)= @_;

    $cbgp->send_cmd("net add node 1.0.0.1");
    $cbgp->send_cmd("bgp add router 1 1.0.0.1");
    for (my $index= 0; $index < @peers; $index++) {
      my ($peer, $asn, $weight)= @{$peers[$index]};
      $cbgp->send_cmd("net add node $peer\n");
      $cbgp->send_cmd("net add link 1.0.0.1 $peer $weight\n");
      $cbgp->send_cmd("net node 1.0.0.1 route add $peer/32 * $peer $weight\n");
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

    $cbgp->send_cmd("net add node 1.0.0.1");
    $cbgp->send_cmd("bgp add router 1 1.0.0.1");
    for (my $index= 0; $index < @peers; $index++) {
      my @peer_record= @{$peers[$index]};
      my $peer= shift @peer_record;
      my $asn= shift @peer_record;
      my $weight= shift @peer_record;
      my $virtual= shift @peer_record;
      $cbgp->send_cmd("net add node $peer");
      $cbgp->send_cmd("net add link 1.0.0.1 $peer $weight");
      $cbgp->send_cmd("net node 1.0.0.1 route add $peer/32 * $peer $weight");
      if ($virtual) {
	cbgp_peering($cbgp, "1.0.0.1", $peer, $asn, "virtual", @peer_record);
      } else {
	$cbgp->send_cmd("net node $peer route add 1.0.0.1/32 * 1.0.0.1 $weight");
	$cbgp->send_cmd("bgp add router $asn $peer");
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

    $cbgp->send_cmd("net add domain 1 igp");
    $cbgp->send_cmd("net add node 1.0.0.1");
    $cbgp->send_cmd("net node 1.0.0.1 domain 1");

    foreach my $peer_spec (@_) {
      my $peer_ip= $peer_spec->[0];
      my $peer_weight= $peer_spec->[1];
      $cbgp->send_cmd("net add node $peer_ip");
      $cbgp->send_cmd("net node $peer_ip domain 1");
      $cbgp->send_cmd("net add link 1.0.0.1 $peer_ip $peer_weight");
      $cbgp->send_cmd("net link 1.0.0.1 $peer_ip igp-weight $peer_weight");
      $cbgp->send_cmd("net link $peer_ip 1.0.0.1 igp-weight $peer_weight");
    }
    $cbgp->send_cmd("net domain 1 compute");

    $cbgp->send_cmd("net add node 2.0.0.1");

    $cbgp->send_cmd("net add link 1.0.0.1 2.0.0.1 0");
    $cbgp->send_cmd("net node 1.0.0.1 route add 2.0.0.1/32 * 2.0.0.1 0");
    $cbgp->send_cmd("net node 2.0.0.1 route add 1.0.0.1/32 * 1.0.0.1 0");

    $cbgp->send_cmd("bgp add router 1 1.0.0.1");
    $cbgp->send_cmd("bgp router 1.0.0.1");
    foreach my $peer_spec (@_) {
      my $peer_ip= $peer_spec->[0];
      $cbgp->send_cmd("\tadd peer 1 $peer_ip");
      $cbgp->send_cmd("\tpeer $peer_ip virtual");
      for (my $index= 2; $index < scalar(@$peer_spec); $index++) {
	my $peer_option= $peer_spec->[$index];
	$cbgp->send_cmd("\tpeer $peer_ip $peer_option");
      }
      $cbgp->send_cmd("\tpeer $peer_ip up");
    }
    $cbgp->send_cmd("\tadd peer 2 2.0.0.1");
    $cbgp->send_cmd("\tpeer 2.0.0.1 up");
    $cbgp->send_cmd("\texit");

    $cbgp->send_cmd("bgp add router 2 2.0.0.1");
    $cbgp->send_cmd("bgp router 2.0.0.1");
    $cbgp->send_cmd("\tadd peer 1 1.0.0.1");
    $cbgp->send_cmd("\tpeer 1.0.0.1 up");
    $cbgp->send_cmd("\texit");
  }

# -----[ cbgp_topo_igp_compute ]-------------------------------------
sub cbgp_topo_igp_compute($$$)
  {
    my ($cbgp, $topo, $domain)= @_;
    $cbgp->send_cmd("net domain $domain compute");
  }

# -----[ cbgp_topo_bgp_routers ]-------------------------------------
sub cbgp_topo_bgp_routers($$$) {
  my ($cbgp, $topo, $asn)= @_;
  my $nodes= topo_get_nodes($topo);

  foreach my $node (keys %$nodes) {
    $cbgp->send_cmd("bgp add router $asn $node");
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
	$cbgp->send_cmd("bgp router $node1 add peer $asn $node2");
	$cbgp->send_cmd("bgp router $node1 peer $node2 up");
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
	$cbgp->send_cmd("bgp router $node1 add network $prefix");
	$cbgp->send_cmd("sim run");
      }
	
      # Check that the routes are present in each router's RIB
      foreach my $node2 (keys %$nodes) {
	my $rib= cbgp_get_rib($cbgp, $node2);
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
	  if ($rib->{$prefix}->[F_RIB_NEXTHOP] ne $node1) {
	    print "incorrect next-hop :-(\n";
	    return 0;
	  }

	  # Route should be marked as 'best' in all routers
	  if (!($rib->{$prefix}->[F_RIB_STATUS] =~ m/>/)) {
	    print "not best :-(\n";
	    return 0;
	  }

	  if ($node2 eq $node1) {
	    # Route should be marked as 'internal' in the origin router
	    if (!($rib->{$prefix}->[F_RIB_STATUS] =~ m/i/)) {
	      print "not internal :-(\n";
	      return 0;
	    }
	  } else {
	    # Route should be marked as 'reachable' in other routers
	    if (!($rib->{$prefix}->[F_RIB_STATUS] =~ m/\*/)) {
	      print "not reachable :-(\n";
	      return 0;
	    }
	  }
	  # Check that the route is installed into the IP
	  # routing table
	  if ($node1 ne $node2) {
	    my $rt= cbgp_get_rt($cbgp, $node2);
	    if (!exists($rt->{$prefix}) ||
		($rt->{$prefix}->[F_RT_PROTO] ne C_RT_PROTO_BGP)) {
	      print "route not installed in IP routing table\n";
	      return 0;
	    }
	  }
	}
      }	

      # Remove networks
      foreach my $prefix (@$prefixes) {	
	$cbgp->send_cmd("bgp router $node1 del network $prefix");
	$cbgp->send_cmd("sim run");

	# Check that the routes have been withdrawn
	foreach my $node2 (keys %$nodes) {
	  my $rib;
	  # Check that the route is not installed into the Loc-RIB
	  # (best)
	  $rib= cbgp_get_rib($cbgp, $node2);
	  if (scalar(keys %$rib) != 0) {
	    print "invalid number of routes in RIB (".
	      scalar(keys %$rib)."), should be 0\n";
	    return 0;
	  }
	  # Check that the route is not present in the Adj-RIB-in
	  $rib= cbgp_get_rib_in($cbgp, $node2);
	  if (scalar(keys %$rib) != 0) {
	    print "invalid number of routes in Adj-RIB-in (".
	      scalar(keys %$rib)."), should be 0\n";
	  }
	  # Check that the route has been uninstalled from the IP
	  # routing table
	  if ($node1 ne $node2) {
	    my $rt= cbgp_get_rt($cbgp, $node2);
	    if (exists($rt->{$prefix}) &&
		($rt->{$prefix}->[F_RT_PROTO] eq C_RT_PROTO_BGP)) {
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
      my $routes= cbgp_get_rt($cbgp, $node1);
      foreach my $node2 (keys %$nodes) {
	($node1 eq $node2) and next;
	if (!exists($routes->{"$node2/32"})) {
	  $tests->debug("no route towards destination ($node2/32)");
	  return 0;
	}
	my $type= $routes->{"$node2/32"}->[F_RT_PROTO];
	if ($type ne C_RT_PROTO_IGP) {
	  $tests->debug("route towards $node2/32 is not from IGP ($type)");
	  return 0;
	}
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
      my $peers= cbgp_get_peers($cbgp, $node1);
      foreach my $node2 (keys %$nodes) {
	($node1 eq $node2) and next;
	if (!exists($peers->{$node2})) {
	  print "missing session $node1 <-> $node2\n";
	  return 0;
	}
	if (defined($state) &&
	    ($peers->{$node2}->[F_PEER_STATE] ne $state)) {
	  print "$state\t$peers->{$node2}->[F_PEER_STATE]\n";
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
      my $peers1= cbgp_get_peers($cbgp, $node1);
      foreach my $node2 (keys %{$topo->{$node1}}) {
	my $peers2= cbgp_get_peers($cbgp, $node2);

	if (!exists($peers1->{$node2})) {
	  print "missing session $node1 --> $node2\n";
	  return 0;
	}
	if (defined($state) &&
	    ($peers1->{$node2}->[F_PEER_STATE] ne $state)) {
	  print "$state\t$peers1->{$node2}->[F_PEER_STATE]\n";
	  return 0;
	}

	if (!exists($peers2->{$node1})) {
	  print "missing session $node2 --> $node1\n";
	  return 0;
	}
	if (defined($state) &&
	    ($peers2->{$node1}->[F_PEER_STATE] ne $state)) {
	  print "$state\t$peers2->{$node1}->[F_PEER_STATE]\n";
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
	  $trace= cbgp_record_route($cbgp, $node1, $node2,
				    -delay=>1, -weight=>1);
	} else {
	  $trace= cbgp_record_route($cbgp, $node1, $node2);
	}

	return 0
	  if (!defined($trace));

	# Result should be equal to the given status
	if ($trace->[F_TR_STATUS] ne $status) {
	  $tests->debug("ERROR incorrect status \"".
			$trace->[F_TR_STATUS].
			"\" (expected=\"$status\")");
	  return 0;
	}

	# Check that destination matches in case of success
	my @path= @{$trace->[F_TR_PATH]};
	if ($trace->[F_TR_STATUS] eq "SUCCESS") {
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
	  $tests->debug("($delay, $weight) vs ($trace->[F_TR_DELAY], $trace->[F_TR_WEIGHT])"); 
	  if ($delay != $trace->[F_TR_DELAY]) {
	    $tests->debug("delay mismatch ($delay vs $trace->[F_TR_DELAY])");
	    return 0;
	  }
	  if ($weight != $trace->[F_TR_WEIGHT]) {
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
      my $links= cbgp_get_links($cbgp, $node1);

      # Add a static route over each link
      foreach my $link (values %$links) {
	$cbgp->send_cmd("net node $node1 route add ".
			$link->[F_LINK_DST]."/32 * ".
			$link->[F_LINK_DST]." ".
			$link->[F_LINK_WEIGHT]);
      }

      # Test static routes (presence)
      my $rt= cbgp_get_rt($cbgp, $node1);
      (scalar(keys %$rt) != scalar(keys %$links)) and return 0;
      foreach my $link (values %$links) {
	my $prefix= $link->[F_LINK_DST]."/32";
	# static route exists
	(!exists($rt->{$prefix})) and return 0;
	# next-hop is link's tail-end
	if ($rt->{$prefix}->[F_RT_IFACE] ne
	    $link->[F_LINK_DST]) {
	  print "next-hop != link's tail-end\n";
	  return 0;
	}
      }
	
      # Test static routes (forward)
      foreach my $link (values %$links) {
	my $trace= cbgp_record_route($cbgp, $node1,
					 $link->[F_LINK_DST]);
	# Destination was reached
	($trace->[F_TR_STATUS] ne "SUCCESS") and return 0;
	# Path is 2 hops long
	(@{$trace->[F_TR_PATH]} != 2) and return 0;
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

    $cbgp->send_cmd("bgp router $router");
    $cbgp->send_cmd("\tadd peer $asn $peer");
    foreach my $option (@options) {
      $cbgp->send_cmd("\tpeer $peer $option");
    }
    $cbgp->send_cmd("\tpeer $peer up");
    $cbgp->send_cmd("\texit");
  }

# -----[ cbgp_filter ]-----------------------------------------------
# Setup a filter on a BGP peering. The filter specification includes
# a direction (in/out), a predicate and a list of actions.
# -------------------------------------------------------------------
sub cbgp_filter($$$$$$)
  {
    my ($cbgp, $router, $peer, $direction, $predicate, $actions)= @_;

    $cbgp->send_cmd("bgp router $router");
    $cbgp->send_cmd("\tpeer $peer");
    $cbgp->send_cmd("\t\tfilter $direction");
    $cbgp->send_cmd("\t\t\tadd-rule");
    $cbgp->send_cmd("\t\t\t\tmatch \"$predicate\"");
    $cbgp->send_cmd("\t\t\t\taction \"$actions\"");
    $cbgp->send_cmd("\t\t\t\texit");
    $cbgp->send_cmd("\t\t\texit");
    $cbgp->send_cmd("\t\texit");
    $cbgp->send_cmd("\texit");
  }

# -----[ cbgp_recv_update ]------------------------------------------
sub cbgp_recv_update($$$$$)
  {
    my ($cbgp, $router, $asn, $peer, $msg)= @_;

    $cbgp->send_cmd("bgp router $router peer $peer recv ".
	      "\"BGP4|0|A|$router|$asn|$msg\"");
  }

# -----[ cbgp_recv_withdraw ]----------------------------------------
sub cbgp_recv_withdraw($$$$$)
  {
    my ($cbgp, $router, $asn, $peer, $prefix)= @_;

    $cbgp->send_cmd("bgp router $router peer $peer recv ".
	      "\"BGP4|0|W|$router|$asn|$prefix\"");
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
	  my $links_hash= cbgp_get_links($cbgp, $node1);
	  my @links= values %$links_hash;
	  foreach my $link (@links) {
	    $cbgp_topo{$node1}{$link->[F_LINK_DST]}=
	      [$link->[F_LINK_DELAY],
	       $link->[F_LINK_WEIGHT]];
	  }
	}
	if (!exists($nodes{$node2})) {
	  $nodes{$node2}= 1;
	  my $links_hash= cbgp_get_links($cbgp, $node2);
	  my @links= values %$links_hash;
	  foreach my $link (@links) {
	    $cbgp_topo{$node2}{$link->[F_LINK_DST]}=
	      [$link->[F_LINK_DELAY],
	       $link->[F_LINK_WEIGHT]];
	  }
	}
      }
    }

    # Compare these links to the initial topology
    return (topo_included($topo, \%cbgp_topo) &&
	    topo_included(\%cbgp_topo, $topo));
  }

# -----[ cbgp_check_bgp_route ]--------------------------------------
sub cbgp_check_bgp_route($$$)
  {
    my ($cbgp, $node, $prefix)= @_;

    my $rib= cbgp_get_rib($cbgp, $node);
    if (!exists($rib->{$prefix})) {
      return 0;
    } else {
      return 1;
    }
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

    $cbgp->send_cmd("show version");
    $cbgp->send_cmd("print \"DONE\\n\"");
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

# -----[ cbgp_valid_cli ]--------------------------------------------
#
# -------------------------------------------------------------------
sub cbgp_valid_cli($)
  {
    my ($cbgp)= @_;
    return TEST_NOT_TESTED;
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
    return TEST_FAILURE
      if (check_has_error($msg));
    $msg= cbgp_check_error($cbgp, "net add node 123.45.67.89");
    return TEST_FAILURE
      if (check_has_error($msg));
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

    $cbgp->send_cmd("net add node 1.0.0.1");

    my $msg= cbgp_check_error($cbgp, "net add node 1.0.0.1");
    return TEST_FAILURE
      if (!check_has_error($msg, $CBGP_ERROR_NODE_EXISTS));
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

    $cbgp->send_cmd("net add node 1.0.0.1");
    my $info= cbgp_node_info($cbgp, "1.0.0.1");
    (!defined($info)) and return TEST_FAILURE;
    (exists($info->{'name'})) and return TEST_FAILURE;
    $cbgp->send_cmd("net node 1.0.0.1 name R1");
    $cbgp->send_cmd("net node 1.0.0.1 show info");
    $info= cbgp_node_info($cbgp, "1.0.0.1");
    (!defined($info)) and return TEST_FAILURE;
    (!exists($info->{'name'}) || ($info->{'name'} ne "R1")) and
      return TEST_FAILURE;

    return TEST_SUCCESS;
  }

# -----[ cbgp_valid_net_link ]---------------------------------------
# Create a topology composed of 3 nodes and 2 links. Check that the
# links are correctly created. Check the links attributes.
# -------------------------------------------------------------------
sub cbgp_valid_net_link($$) {
  my ($cbgp, $topo)= @_;
  $cbgp->send_cmd("net add node 1.0.0.1\n");
  $cbgp->send_cmd("net add node 1.0.0.2");
  $cbgp->send_cmd("net add node 1.0.0.3");
  $cbgp->send_cmd("net add link 1.0.0.1 1.0.0.2 10");
  $cbgp->send_cmd("net link 1.0.0.1 1.0.0.2 igp-weight 123");
  $cbgp->send_cmd("net link 1.0.0.2 1.0.0.1 igp-weight 123");
  $cbgp->send_cmd("net add link 1.0.0.2 1.0.0.3 10");
  $cbgp->send_cmd("net link 1.0.0.2 1.0.0.3 igp-weight 321");
  $cbgp->send_cmd("net link 1.0.0.3 1.0.0.2 igp-weight 321");
  return TEST_FAILURE
    if (!cbgp_check_link($cbgp, '1.0.0.1', '1.0.0.2',
			 -type=>C_IFACE_RTR,
			 -weight=>123));
  return TEST_FAILURE
    if (!cbgp_check_link($cbgp, '1.0.0.2', '1.0.0.1',
			 -type=>C_IFACE_RTR,
			 -weight=>123));
  return TEST_FAILURE
    if (!cbgp_check_link($cbgp, '1.0.0.2', '1.0.0.3',
			 -type=>C_IFACE_RTR,
			 -weight=>321));
  return TEST_FAILURE
  if (!cbgp_check_link($cbgp, '1.0.0.3', '1.0.0.2',
		       -type=>C_IFACE_RTR,
		       -weight=>321));
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
sub cbgp_valid_net_link_loop($) {
  my ($cbgp)= @_;

  $cbgp->send_cmd("net add node 1.0.0.1");

  my $msg= cbgp_check_error($cbgp, "net add link 1.0.0.1 1.0.0.1 10");
  return TEST_FAILURE
    if (!check_has_error($msg, $CBGP_ERROR_LINK_ENDPOINTS));
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
sub cbgp_valid_net_link_duplicate($) {
  my ($cbgp)= @_;

  $cbgp->send_cmd("net add node 1.0.0.1");
  $cbgp->send_cmd("net add node 1.0.0.2");
  $cbgp->send_cmd("net add link 1.0.0.1 1.0.0.2 10");

  my $msg= cbgp_check_error($cbgp, "net add link 1.0.0.1 1.0.0.2 10");
  return TEST_FAILURE
    if (!check_has_error($msg, $CBGP_ERROR_LINK_EXISTS));
  return TEST_SUCCESS;
}

# -----[ cbgp_valid_net_link_missing ]-------------------------------
# Test ability to detect creation of link with missing destination.
#
# Scenario:
#   * Create a node 1.0.0.1
#   * Try to add link to 1.0.0.2 (missing)
#   * Check that an error is returned (and check error message)
# -------------------------------------------------------------------
sub cbgp_valid_net_link_missing($) {
  my ($cbgp)= @_;

  $cbgp->send_cmd("net add node 1.0.0.1");

  my $msg= cbgp_check_error($cbgp, "net add link 1.0.0.1 1.0.0.2 10");
  return TEST_FAILURE
    if (!check_has_error($msg, $CBGP_ERROR_INVALID_LINK_TAIL));
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

    $cbgp->send_cmd("net add node 1.0.0.1");
    $cbgp->send_cmd("net add node 1.0.0.2");
    $cbgp->send_cmd("net add link 1.0.0.1 1.0.0.2 10");

    $links= cbgp_get_links($cbgp, "1.0.0.1");
    return TEST_FAILURE
      if (!check_has_link($links, '1.0.0.2',
			  -weight=>MAX_IGP_WEIGHT));
    $links= cbgp_get_links($cbgp, "1.0.0.2");
    return TEST_FAILURE
      if (!check_has_link($links, "1.0.0.1",
			  -weight=>MAX_IGP_WEIGHT));

    $cbgp->send_cmd("net link 1.0.0.1 1.0.0.2 igp-weight 123");
    $links= cbgp_get_links($cbgp, "1.0.0.1");
    return TEST_FAILURE
      if (!check_has_link($links, "1.0.0.2",
			  -weight=>123));
    $links= cbgp_get_links($cbgp, "1.0.0.2");
    return TEST_FAILURE
      if (!check_has_link($links, "1.0.0.1",
			  -weight=>MAX_IGP_WEIGHT));

    $cbgp->send_cmd("net link 1.0.0.1 1.0.0.2 igp-weight --bidir 456");
    $links= cbgp_get_links($cbgp, "1.0.0.1");
    return TEST_FAILURE
      if (!check_has_link($links, "1.0.0.2",
			  -weight=>456));
    $links= cbgp_get_links($cbgp, "1.0.0.2");
    return TEST_FAILURE
      if (!check_has_link($links, "1.0.0.1",
			  -weight=>456));

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

    $cbgp->send_cmd("net add node 1.0.0.1");
    $cbgp->send_cmd("net add node 1.0.0.2");
    $cbgp->send_cmd("net add link --bw=12345678 1.0.0.1 1.0.0.2 10");

    # Check load and capacity of R1->R2
    $info= cbgp_link_info($cbgp, '1.0.0.1', '1.0.0.2');
    if (!exists($info->{'capacity'}) || ($info->{'capacity'} != 12345678) ||
	!exists($info->{'load'}) || ($info->{'load'} != 0)) {
      $tests->debug("Error: no/invalid capacity/load for link R1->R2");
      return TEST_FAILURE;
    }

    # Check load and capacity of R2->R1
    $info= cbgp_link_info($cbgp, '1.0.0.2', '1.0.0.1');
    if (!exists($info->{'capacity'}) || ($info->{'capacity'} != 12345678) ||
	!exists($info->{'load'}) || ($info->{'load'} != 0)) {
      $tests->debug("Error: no/invalid capacity for link R2->R1");
      return TEST_FAILURE;
    }

    # Add load to R1->R2
    $cbgp->send_cmd("net node 1.0.0.1 iface 1.0.0.2 load add 589");
    $info= cbgp_link_info($cbgp, '1.0.0.1', '1.0.0.2');
    if (!exists($info->{'load'}) ||
	($info->{'load'} != 589)) {
      $tests->debug("Error: no/invalid load for link R1->R2");
      return TEST_FAILURE;
    }

    # Load of R1->R2 should be cleared
    $cbgp->send_cmd("net node 1.0.0.1 iface 1.0.0.2 load clear");
    $info= cbgp_link_info($cbgp, '1.0.0.1', '1.0.0.2');
    if (!exists($info->{'load'}) ||
	($info->{'load'} != 0)) {
      $tests->debug("Error: load of link R1->R2 should be cleared");
      return TEST_FAILURE;
    }

    return TEST_SUCCESS;
  }

# -----[ cbgp_valid_net_link_ptp ]-----------------------------------
# -------------------------------------------------------------------
sub cbgp_valid_net_link_ptp($) {
  my ($cbgp)= @_;

  $cbgp->send_cmd("net add node 1.0.0.1");
  $cbgp->send_cmd("net add node 1.0.0.2");
  $cbgp->send_cmd("net add node 1.0.0.3");
  $cbgp->send_cmd("net add link-ptp 1.0.0.1 192.168.0.1/30 ".
		  "1.0.0.2 192.168.0.2/30");
  $cbgp->send_cmd("net add link-ptp 1.0.0.1 192.168.0.5/30 ".
		  "1.0.0.3 192.168.0.6/30");
  $cbgp->send_cmd("net add link-ptp 1.0.0.2 192.168.0.9/30 ".
		  "1.0.0.3 192.168.0.10/30");

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

    $cbgp->send_cmd("net add node 1.0.0.1");
    $cbgp->send_cmd("net add node 1.0.0.2");
    $cbgp->send_cmd("net add subnet 192.168.0/24 transit");
    $cbgp->send_cmd("net add subnet 192.168.1/24 stub");
    $cbgp->send_cmd("net add link 1.0.0.1 192.168.0.1/24 10");
    $cbgp->send_cmd("net link 1.0.0.1 192.168.0.1/24 igp-weight 123");
    $cbgp->send_cmd("net add link 1.0.0.2 192.168.0.2/24 20");
    $cbgp->send_cmd("net link 1.0.0.2 192.168.0.2/24 igp-weight 321");
    $cbgp->send_cmd("net add link 1.0.0.2 192.168.1.2/24 30");
    $cbgp->send_cmd("net link 1.0.0.2 192.168.1.2/24 igp-weight 456");

    return TEST_FAILURE
      if (!cbgp_check_link($cbgp, '1.0.0.1', '192.168.0.0/24',
			   -type=>C_IFACE_PTMP,
			   -weight=>123));
    return TEST_FAILURE
      if (!cbgp_check_link($cbgp, '1.0.0.2', '192.168.0.0/24',
			   -type=>C_IFACE_PTMP,
			   -weight=>321));
    return TEST_FAILURE
      if (!cbgp_check_link($cbgp, '1.0.0.2', '192.168.1.0/24',
			   -type=>C_IFACE_PTMP,
			   -weight=>456));

    # Create a /32 subnet: should fail
    $msg= cbgp_check_error($cbgp, "net add subnet 1.2.3.4/32 transit");
    return TEST_FAILURE
      if (!defined($msg) ||
	  !($msg =~ m/$CBGP_ERROR_INVALID_SUBNET/));

    return TEST_SUCCESS;
  }

# -----[ cbgp_valid_net_subnet_duplicate ]---------------------------
#
# -------------------------------------------------------------------
sub cbgp_valid_net_subnet_duplicate($)
  {
    my ($cbgp)= @_;
    my $msg;

    $cbgp->send_cmd("net add subnet 192.168.0/24 transit\n");
    $msg= cbgp_check_error($cbgp, "net add subnet 192.168.0/24 transit\n");
    if (!defined($msg) || !($msg =~m/$CBGP_ERROR_SUBNET_EXISTS/)) {
      return TEST_FAILURE;
    }

    $msg= cbgp_check_error($cbgp, "net add subnet 192.168.0.1/24 transit\n");
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

    $cbgp->send_cmd("net add node 1.0.0.1");
    $cbgp->send_cmd("net add node 1.0.0.2");
    $cbgp->send_cmd("net add node 1.0.0.3");
    $cbgp->send_cmd("net add node 2.0.0.1");
    $cbgp->send_cmd("net add node 192.168.0.1");
    $cbgp->send_cmd("net add subnet 192.168.0/24 transit");
    $cbgp->send_cmd("net add subnet 2.0.0/24 transit");
    $cbgp->send_cmd("net add link 1.0.0.1 1.0.0.2 10");
    $cbgp->send_cmd("net add link 1.0.0.1 1.0.0.3 10");
    $cbgp->send_cmd("net add link 1.0.0.1 2.0.0.1 10");
    $cbgp->send_cmd("net add link 1.0.0.1 192.168.0.1/24 10");
    $cbgp->send_cmd("net add link 1.0.0.1 192.168.0.2/24 10");

    # Add a mtp link to 1.0.0/24: it should succeed
    $msg= cbgp_check_error($cbgp, "net add link 1.0.0.1 2.0.0.0/24 10");
    return TEST_FAILURE
      if (defined($msg));

    $ifaces= cbgp_get_ifaces($cbgp, "1.0.0.1");
    return TEST_FAILURE
      if (!check_has_iface($ifaces, "1.0.0.1/32",
			   -type=>C_IFACE_LOOPBACK));
    return TEST_FAILURE
      if (!check_has_iface($ifaces, "192.168.0.1/24",
			   -type=>C_IFACE_PTMP));
    return TEST_FAILURE
      if (!check_has_iface($ifaces, "192.168.0.2/24",
			 -type=>C_IFACE_PTMP));
    return TEST_FAILURE
      if (!check_has_iface($ifaces, "1.0.0.2/32",
			  -type=>C_IFACE_RTR));
    return TEST_FAILURE
      if (!check_has_iface($ifaces, "1.0.0.3/32",
			   -type=>C_IFACE_RTR));
    return TEST_FAILURE
      if (!check_has_iface($ifaces, "2.0.0.1/32",
			 -type=>C_IFACE_RTR));
    return TEST_FAILURE
      if (!check_has_iface($ifaces, "2.0.0.0/24",
			 -type=>C_IFACE_PTMP));

    # Add a ptp link to 192.168.0.1:
    #  - it should fail (default)
    #  - it should succeed if (IFACE_ID_ADDR_MASK is set in src/link-list.c)
    $msg= cbgp_check_error($cbgp, "net add link 1.0.0.1 192.168.0.1 10");
    return TEST_FAILURE
      if (!defined($msg));

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

    $cbgp->send_cmd("net add domain 1 igp");
    $cbgp->send_cmd("net add node 1.0.0.1");
    $cbgp->send_cmd("net add node 1.0.0.2");
    $cbgp->send_cmd("net add node 1.0.0.3");
    $cbgp->send_cmd("net add link 1.0.0.1 1.0.0.2 0");
    $cbgp->send_cmd("net link 1.0.0.1 1.0.0.2 igp-weight --bidir 4000000000");
    $cbgp->send_cmd("net add link 1.0.0.2 1.0.0.3 0");
    $cbgp->send_cmd("net link 1.0.0.2 1.0.0.3 igp-weight --bidir 4000000000");
    $cbgp->send_cmd("net node 1.0.0.1 domain 1");
    $cbgp->send_cmd("net node 1.0.0.2 domain 1");
    $cbgp->send_cmd("net node 1.0.0.3 domain 1");
    $cbgp->send_cmd("net domain 1 compute");
    $rt= cbgp_get_rt($cbgp, "1.0.0.1", undef);
    if (!exists($rt->{"1.0.0.3/32"}) ||
	($rt->{"1.0.0.3/32"}->[F_RT_METRIC] != MAX_IGP_WEIGHT)) {
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

    $cbgp->send_cmd("net add node 0.1.0.0");

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
sub cbgp_valid_net_igp_subnet($) {
  my ($cbgp)= @_;
  my $rt;

  $cbgp->send_cmd("net add domain 1 igp");
  $cbgp->send_cmd("net add subnet 192.168.0/24 transit");
  $cbgp->send_cmd("net add subnet 192.168.1/24 stub");
  $cbgp->send_cmd("net add subnet 192.168.2/24 stub");
  $cbgp->send_cmd("net add node 1.0.0.1");
  $cbgp->send_cmd("net node 1.0.0.1 domain 1");
  $cbgp->send_cmd("net add node 1.0.0.2");
  $cbgp->send_cmd("net node 1.0.0.2 domain 1");
  $cbgp->send_cmd("net add node 1.0.0.3");
  $cbgp->send_cmd("net node 1.0.0.3 domain 1");
  $cbgp->send_cmd("net add link 1.0.0.1 1.0.0.3 1");
  $cbgp->send_cmd("net link 1.0.0.1 1.0.0.3 igp-weight 1");
  $cbgp->send_cmd("net link 1.0.0.3 1.0.0.1 igp-weight 1");
  $cbgp->send_cmd("net add link 1.0.0.1 192.168.0.1/24 10");
  $cbgp->send_cmd("net link 1.0.0.1 192.168.0.1/24 igp-weight 10");
  $cbgp->send_cmd("net add link 1.0.0.2 192.168.0.2/24 10");
  $cbgp->send_cmd("net link 1.0.0.2 192.168.0.2/24 igp-weight 10");
  $cbgp->send_cmd("net add link 1.0.0.2 192.168.1.2/24 1");
  $cbgp->send_cmd("net link 1.0.0.2 192.168.1.2/24 igp-weight 1");
  $cbgp->send_cmd("net add link 1.0.0.2 192.168.2.2/24 10");
  $cbgp->send_cmd("net link 1.0.0.2 192.168.2.2/24 igp-weight 10");
  $cbgp->send_cmd("net add link 1.0.0.3 192.168.1.3/24 1");
  $cbgp->send_cmd("net link 1.0.0.3 192.168.1.3/24 igp-weight 1");
  $cbgp->send_cmd("net domain 1 compute");

  $rt= cbgp_get_rt($cbgp, "1.0.0.1");
  return TEST_FAILURE
    if (!check_has_route($rt, "1.0.0.2/32",
			 -nexthop=>"192.168.0.2"));
  return TEST_FAILURE
    if (!check_has_route($rt, "1.0.0.3/32"));
  return TEST_FAILURE
    if (!check_has_route($rt, "192.168.0.0/24"));
  return TEST_FAILURE
    if (!check_has_route($rt, "192.168.1.0/24"));
  return TEST_FAILURE
    if (!check_has_route($rt, "192.168.2.0/24",
			 -nexthop=>"192.168.0.2"));
  return TEST_SUCCESS;
}

# -----[ cbgp_valid_net_ntf_load ]-----------------------------------
# Load a topology from an NTF file into C-BGP (using C-BGP 's "net
# ntf load" command). Check that the links are correctly setup.
#
# Resources:
#   [valid-record-route.ntf]
# -------------------------------------------------------------------
sub cbgp_valid_net_ntf_load($$) {
  my ($cbgp, $ntf_file)= @_;
  my $topo= topo_from_ntf($ntf_file);
  $cbgp->send_cmd("net ntf load \"$ntf_file\"");
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

    $cbgp->send_cmd("net add domain 1 igp");
    $cbgp->send_cmd("net add node 1.0.0.1");
    $cbgp->send_cmd("net node 1.0.0.1 domain 1");
    $cbgp->send_cmd("net add node 1.0.0.2");
    $cbgp->send_cmd("net node 1.0.0.2 domain 1");
    $cbgp->send_cmd("net add node 1.0.0.3");
    $cbgp->send_cmd("net node 1.0.0.3 domain 1");
    $cbgp->send_cmd("net add node 1.0.0.4");
    $cbgp->send_cmd("net node 1.0.0.4 domain 1");
    $cbgp->send_cmd("net add node 1.0.0.5");
    $cbgp->send_cmd("net node 1.0.0.5 domain 1");
    $cbgp->send_cmd("net add link --bw=2000 1.0.0.1 1.0.0.2 1");
    $cbgp->send_cmd("net link 1.0.0.1 1.0.0.2 igp-weight --bidir 1");
    $cbgp->send_cmd("net add link --bw=3000 1.0.0.2 1.0.0.3 1");
    $cbgp->send_cmd("net link 1.0.0.2 1.0.0.3 igp-weight --bidir 1");
    $cbgp->send_cmd("net add link --bw=100 1.0.0.2 1.0.0.4 1");
    $cbgp->send_cmd("net link 1.0.0.2 1.0.0.4 igp-weight --bidir 1");
    $cbgp->send_cmd("net add link --bw=1000 1.0.0.2 1.0.0.5 1");
    $cbgp->send_cmd("net link 1.0.0.2 1.0.0.5 igp-weight --bidir 1");
    $cbgp->send_cmd("net domain 1 compute");

    $trace= cbgp_record_route($cbgp, "1.0.0.1", "1.0.0.2", -capacity=>1);
    if (($trace->[F_TR_STATUS] ne "SUCCESS") ||
	($trace->[F_TR_CAPACITY] != 2000)) {
      $tests->debug("Error: record-route R1 -> R2 failed");
      return TEST_FAILURE;
    }
    $trace= cbgp_record_route($cbgp, "1.0.0.1", "1.0.0.3", -capacity=>1);
    if (($trace->[F_TR_STATUS] ne "SUCCESS") ||
	($trace->[F_TR_CAPACITY] != 2000)) {
      $tests->debug("Error: record-route R1 -> R3 failed");
      return TEST_FAILURE;
    }
    $trace= cbgp_record_route($cbgp, "1.0.0.1", "1.0.0.4", -capacity=>1);
    if (($trace->[F_TR_STATUS] ne "SUCCESS") ||
	($trace->[F_TR_CAPACITY] != 100)) {
      $tests->debug("Error: record-route R1 -> R4 failed");
      return TEST_FAILURE;
    }
    $trace= cbgp_record_route($cbgp, "1.0.0.1", "1.0.0.5", -capacity=>1);
    if (($trace->[F_TR_STATUS] ne "SUCCESS") ||
	($trace->[F_TR_CAPACITY] != 1000)) {
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

    $cbgp->send_cmd("net add domain 1 igp");
    $cbgp->send_cmd("net add node 1.0.0.1");
    $cbgp->send_cmd("net node 1.0.0.1 domain 1");
    $cbgp->send_cmd("net add node 1.0.0.2");
    $cbgp->send_cmd("net node 1.0.0.2 domain 1");
    $cbgp->send_cmd("net add node 1.0.0.3");
    $cbgp->send_cmd("net node 1.0.0.3 domain 1");
    $cbgp->send_cmd("net add node 1.0.0.4");
    $cbgp->send_cmd("net node 1.0.0.4 domain 1");
    $cbgp->send_cmd("net add node 1.0.0.5");
    $cbgp->send_cmd("net node 1.0.0.5 domain 1");
    $cbgp->send_cmd("net add node 1.0.0.6");
    $cbgp->send_cmd("net node 1.0.0.6 domain 1");
    $cbgp->send_cmd("net add link 1.0.0.1 1.0.0.2 1");
    $cbgp->send_cmd("net link 1.0.0.1 1.0.0.2 igp-weight --bidir 1");
    $cbgp->send_cmd("net add link 1.0.0.2 1.0.0.3 1");
    $cbgp->send_cmd("net link 1.0.0.2 1.0.0.3 igp-weight --bidir 2");
    $cbgp->send_cmd("net add link 1.0.0.2 1.0.0.4 1");
    $cbgp->send_cmd("net link 1.0.0.2 1.0.0.4 igp-weight --bidir 1");
    $cbgp->send_cmd("net add link 1.0.0.3 1.0.0.6 1");
    $cbgp->send_cmd("net link 1.0.0.3 1.0.0.6 igp-weight --bidir 2");
    $cbgp->send_cmd("net add link 1.0.0.4 1.0.0.5 1");
    $cbgp->send_cmd("net link 1.0.0.4 1.0.0.5 igp-weight --bidir 1");
    $cbgp->send_cmd("net add link 1.0.0.4 1.0.0.6 1");
    $cbgp->send_cmd("net link 1.0.0.4 1.0.0.6 igp-weight --bidir 1");
    $cbgp->send_cmd("net domain 1 compute");

    cbgp_record_route($cbgp, "1.0.0.1", "1.0.0.5", -load=>1000);
    cbgp_record_route($cbgp, "1.0.0.1", "1.0.0.6", -load=>500);
    cbgp_record_route($cbgp, "1.0.0.1", "1.0.0.4", -load=>250);
    cbgp_record_route($cbgp, "1.0.0.3", "1.0.0.5", -load=>125);
    cbgp_record_route($cbgp, "1.0.0.3", "1.0.0.6", -load=>1000);

    $info= cbgp_link_info($cbgp, "1.0.0.1", "1.0.0.2");
    if (!exists($info->{load}) || ($info->{load} != 1750)) {
      $tests->debug("Error: load of R1->R2 should be 1750");
      return TEST_FAILURE;
    }
    $info= cbgp_link_info($cbgp, "1.0.0.2", "1.0.0.4");
    if (!exists($info->{load}) || ($info->{load} != 1875)) {
      $tests->debug("Error: load of R2->R4 should be 1875");
      return TEST_FAILURE;
    }
    $info= cbgp_link_info($cbgp, "1.0.0.3", "1.0.0.2");
    if (!exists($info->{load}) || ($info->{load} != 125)) {
      $tests->debug("Error: load of R3->R2 should be 125");
      return TEST_FAILURE;
    }
    $info= cbgp_link_info($cbgp, "1.0.0.3", "1.0.0.6");
    if (!exists($info->{load}) || ($info->{load} != 1000)) {
      $tests->debug("Error: load of R3->R6 should be 1000");
      return TEST_FAILURE;
    }
    $info= cbgp_link_info($cbgp, "1.0.0.4", "1.0.0.5");
    if (!exists($info->{load}) || ($info->{load} != 1125)) {
      $tests->debug("Error: load of R4->R5 should be 1125");
      return TEST_FAILURE;
    }
    $info= cbgp_link_info($cbgp, "1.0.0.4", "1.0.0.6");
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

    $cbgp->send_cmd("net add domain 1 igp\n");
    $cbgp->send_cmd("net add node 1.0.0.1\n");
    $cbgp->send_cmd("net add node 1.0.0.2\n");
    $cbgp->send_cmd("net node 1.0.0.1 domain 1\n");
    $cbgp->send_cmd("net node 1.0.0.2 domain 1\n");

    $cbgp->send_cmd("net add link 1.0.0.1 1.0.0.2 1\n");

    $cbgp->send_cmd("net node 1.0.0.1 route add 2.0.0.1/32 * 1.0.0.2 1\n");
    $cbgp->send_cmd("net node 1.0.0.2 route add 2.0.0.1/32 * 1.0.0.1 1\n");

    $cbgp->send_cmd("net domain 1 compute\n");

    my $loop_trace= cbgp_record_route($cbgp, "1.0.0.1", "2.0.0.1",
					 -checkloop=>1);
    if ($loop_trace->[F_TR_STATUS] ne "LOOP") {
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
    $cbgp->send_cmd("net add domain 1 igp\n");
    $cbgp->send_cmd("net add node 1.0.0.1\n");
    $cbgp->send_cmd("net add node 1.0.0.2\n");
    $cbgp->send_cmd("net add subnet 192.168.0/24 stub\n");
    $cbgp->send_cmd("net node 1.0.0.1 domain 1\n");
    $cbgp->send_cmd("net node 1.0.0.2 domain 1\n");
    $cbgp->send_cmd("net add link 1.0.0.1 1.0.0.2 1\n");
    $cbgp->send_cmd("net link 1.0.0.1 1.0.0.2 igp-weight --bidir 1\n");
    $cbgp->send_cmd("net add link 1.0.0.2 192.168.0.2/24 1\n");
    $cbgp->send_cmd("net link 1.0.0.2 192.168.0.2/24 igp-weight 1\n");
    $cbgp->send_cmd("net domain 1 compute\n");

    my $trace= cbgp_record_route($cbgp, "1.0.0.1", "192.168.0/24");
    return TEST_FAILURE
      if (!check_recordroute($trace, -status=>"SUCCESS"));
    $trace= cbgp_record_route($cbgp, "1.0.0.1", "192.168.0.24");
    return TEST_FAILURE
      if (!check_recordroute($trace, -status=>"UNREACH"));
    $trace= cbgp_record_route($cbgp, "1.0.0.1", "192.168.0.2");
    return TEST_FAILURE
      if (!check_recordroute($trace, -status=>"SUCCESS"));
    return TEST_SUCCESS;
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

    $cbgp->send_cmd("net add domain 1 igp");
    $cbgp->send_cmd("net add node 1.0.0.1");
    $cbgp->send_cmd("net node 1.0.0.1 domain 1");
    $cbgp->send_cmd("net add node 1.0.0.2");
    $cbgp->send_cmd("net node 1.0.0.2 domain 1");
    $cbgp->send_cmd("net add node 1.0.0.3");
    $cbgp->send_cmd("net node 1.0.0.3 domain 1");
    $cbgp->send_cmd("net add node 1.0.0.4");
    $cbgp->send_cmd("net node 1.0.0.4 domain 1");
    $cbgp->send_cmd("net add subnet 192.168.0/24 transit");
    $cbgp->send_cmd("net add subnet 192.168.1/24 transit");
    $cbgp->send_cmd("net add link 1.0.0.1 1.0.0.2 10");
    $cbgp->send_cmd("net link 1.0.0.1 1.0.0.2 igp-weight --bidir 10");
    $cbgp->send_cmd("net add link 1.0.0.1 192.168.1.1/24 10");
    $cbgp->send_cmd("net link 1.0.0.1 192.168.1.1/24 igp-weight 10");
    $cbgp->send_cmd("net add link 1.0.0.2 1.0.0.3 10");
    $cbgp->send_cmd("net link 1.0.0.2 1.0.0.3 igp-weight --bidir 10");
    $cbgp->send_cmd("net add link 1.0.0.2 192.168.0.1/24 10");
    $cbgp->send_cmd("net link 1.0.0.2 192.168.0.1/24 igp-weight 10");
    $cbgp->send_cmd("net add link 1.0.0.3 1.0.0.4 10");
    $cbgp->send_cmd("net link 1.0.0.3 1.0.0.4 igp-weight --bidir 10");
    $cbgp->send_cmd("net domain 1 compute");

    $cbgp->send_cmd("net link 1.0.0.3 1.0.0.4 down");
    $cbgp->send_cmd("net node 1.0.0.1 route add 1.0.0.5/32 * 1.0.0.2 1");
    $cbgp->send_cmd("net node 1.0.0.1 route add 1.0.0.6/32 * 1.0.0.2 1");
    $cbgp->send_cmd("net node 1.0.0.2 route add 1.0.0.6/32 * 192.168.0.1/24 1");
    $cbgp->send_cmd("net node 1.0.0.1 route add 1.0.0.8/32 * 192.168.1.1/24 1");

    my $ping= cbgp_ping($cbgp, '1.0.0.1', '1.0.0.1');
    if (($ping->[F_PING_STATUS] ne C_PING_STATUS_REPLY) ||
	($ping->[F_PING_ADDR] ne '1.0.0.1')) {
      return TEST_FAILURE;
    }

    $ping= cbgp_ping($cbgp, '1.0.0.1', '1.0.0.2');
    if (($ping->[F_PING_STATUS] ne C_PING_STATUS_REPLY) ||
	($ping->[F_PING_ADDR] ne '1.0.0.2')) {
      return TEST_FAILURE;
    }

    $ping= cbgp_ping($cbgp, '1.0.0.1', '1.0.0.3');
    if (($ping->[F_PING_STATUS] ne C_PING_STATUS_REPLY) ||
	($ping->[F_PING_ADDR] ne '1.0.0.3')) {
      return TEST_FAILURE;
    }

    $ping= cbgp_ping($cbgp, '1.0.0.1', '1.0.0.3', -ttl=>1);
    if (($ping->[F_PING_STATUS] ne C_PING_STATUS_ICMP_TTL) ||
	($ping->[F_PING_ADDR] ne '1.0.0.2')) {
      return TEST_FAILURE;
    }

    $ping= cbgp_ping($cbgp, '1.0.0.1', '1.0.0.4');
    if ($ping->[F_PING_STATUS] ne C_PING_STATUS_NO_REPLY) {
      return TEST_FAILURE;
    }

    $ping= cbgp_ping($cbgp, '1.0.0.1', '1.0.0.5');
    if (($ping->[F_PING_STATUS] ne C_PING_STATUS_ICMP_NET) ||
	($ping->[F_PING_ADDR] ne '1.0.0.2')) {
      return TEST_FAILURE;
    }

    $ping= cbgp_ping($cbgp, '1.0.0.1', '1.0.0.6');
    if (($ping->[F_PING_STATUS] ne C_PING_STATUS_ICMP_HOST) ||
	($ping->[F_PING_ADDR] ne '1.0.0.2')) {
      return TEST_FAILURE;
    }

    $ping= cbgp_ping($cbgp, '1.0.0.1', '1.0.0.7');
    if ($ping->[F_PING_STATUS] ne C_PING_STATUS_NET) {
      return TEST_FAILURE;
    }

    $ping= cbgp_ping($cbgp, '1.0.0.1', '1.0.0.8');
    if ($ping->[F_PING_STATUS] ne C_PING_STATUS_HOST) {
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

    $cbgp->send_cmd("net add domain 1 igp");
    $cbgp->send_cmd("net add node 1.0.0.1");
    $cbgp->send_cmd("net node 1.0.0.1 domain 1");
    $cbgp->send_cmd("net add node 1.0.0.2");
    $cbgp->send_cmd("net node 1.0.0.2 domain 1");
    $cbgp->send_cmd("net add node 1.0.0.3");
    $cbgp->send_cmd("net node 1.0.0.3 domain 1");
    $cbgp->send_cmd("net add node 1.0.0.4");
    $cbgp->send_cmd("net node 1.0.0.4 domain 1");
    $cbgp->send_cmd("net add subnet 192.168.0/24 transit");
    $cbgp->send_cmd("net add subnet 192.168.1/24 transit");
    $cbgp->send_cmd("net add link 1.0.0.1 1.0.0.2 10");
    $cbgp->send_cmd("net link 1.0.0.1 1.0.0.2 igp-weight --bidir 10");
    $cbgp->send_cmd("net add link 1.0.0.1 192.168.1.1/24 10");
    $cbgp->send_cmd("net link 1.0.0.1 192.168.1.1/24 igp-weight 10");
    $cbgp->send_cmd("net add link 1.0.0.2 1.0.0.3 10");
    $cbgp->send_cmd("net link 1.0.0.2 1.0.0.3 igp-weight --bidir 10");
    $cbgp->send_cmd("net add link 1.0.0.2 192.168.0.1/24 10");
    $cbgp->send_cmd("net link 1.0.0.2 192.168.0.1/24 igp-weight 10");
    $cbgp->send_cmd("net add link 1.0.0.3 1.0.0.4 10");
    $cbgp->send_cmd("net link 1.0.0.3 1.0.0.4 igp-weight --bidir 10");
    $cbgp->send_cmd("net domain 1 compute");

    $cbgp->send_cmd("net link 1.0.0.3 1.0.0.4 down");
    $cbgp->send_cmd("net node 1.0.0.1 route add 1.0.0.5/32 * 1.0.0.2 1");
    $cbgp->send_cmd("net node 1.0.0.1 route add 1.0.0.6/32 * 1.0.0.2 1");
    $cbgp->send_cmd("net node 1.0.0.2 route add 1.0.0.6/32 * 192.168.0.1/24 1");
    $cbgp->send_cmd("net node 1.0.0.1 route add 1.0.0.8/32 * 192.168.1.1/24 1");

    my $tr= cbgp_traceroute($cbgp, '1.0.0.1', '1.0.0.1');
    return TEST_FAILURE
      if (!check_traceroute($tr, -nhops=>1,
			    -hop=>[0,'1.0.0.1',C_PING_STATUS_REPLY]));
    $tr= cbgp_traceroute($cbgp, '1.0.0.1', '1.0.0.2');
    return TEST_FAILURE
      if (!check_traceroute($tr, -nhops=>1,
			    -hop=>[0,'1.0.0.2',C_PING_STATUS_REPLY]));
    $tr= cbgp_traceroute($cbgp, '1.0.0.1', '1.0.0.3');
    return TEST_FAILURE
      if (!check_traceroute($tr, -nhops=>2,
			    -hop=>[1,'1.0.0.3',C_PING_STATUS_REPLY]));
    $tr= cbgp_traceroute($cbgp, '1.0.0.1', '1.0.0.4');
    return TEST_FAILURE
      if (!check_traceroute($tr, -nhops=>3,
			    -hop=>[2,'*',C_PING_STATUS_NO_REPLY]));
    $tr= cbgp_traceroute($cbgp, '1.0.0.1', '1.0.0.5');
    return TEST_FAILURE
      if (!check_traceroute($tr, -nhops=>2,
			    -hop=>[1,'1.0.0.2',C_PING_STATUS_ICMP_NET]));
    $tr= cbgp_traceroute($cbgp, '1.0.0.1', '1.0.0.6');
    return TEST_FAILURE
      if (!check_traceroute($tr, -nhops=>2,
			    -hop=>[1,'1.0.0.2',C_PING_STATUS_ICMP_HOST]));
    $tr= cbgp_traceroute($cbgp, '1.0.0.1', '1.0.0.7');
    return TEST_FAILURE
      if (!check_traceroute($tr, -nhops=>1,
			    -hop=>[0,'*',C_PING_STATUS_NET]));
    $tr= cbgp_traceroute($cbgp, '1.0.0.1', '1.0.0.8');
    return TEST_FAILURE
      if (!check_traceroute($tr, -nhops=>1,
			    -hop=>[0,'*',C_PING_STATUS_HOST]));
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
    $cbgp->send_cmd("net add domain 1 igp");
    $cbgp->send_cmd("net add node 1.0.0.0");
    $cbgp->send_cmd("net node 1.0.0.0 domain 1");
    $cbgp->send_cmd("net add node 1.0.0.1");
    $cbgp->send_cmd("net node 1.0.0.1 domain 1");
    $cbgp->send_cmd("net add node 1.0.0.2");
    $cbgp->send_cmd("net node 1.0.0.2 domain 1");
    $cbgp->send_cmd("net add node 1.0.0.3");
    $cbgp->send_cmd("net node 1.0.0.3 domain 1");
    $cbgp->send_cmd("net add node 1.0.0.4");
    $cbgp->send_cmd("net node 1.0.0.4 domain 1");
    $cbgp->send_cmd("net add subnet 192.168.0/24 transit");
    $cbgp->send_cmd("net add link 1.0.0.0 1.0.0.1 1");
    $cbgp->send_cmd("net link 1.0.0.0 1.0.0.1 igp-weight --bidir 1");
    $cbgp->send_cmd("net add link 1.0.0.0 1.0.0.2 1");
    $cbgp->send_cmd("net link 1.0.0.0 1.0.0.2 igp-weight --bidir 1");
    $cbgp->send_cmd("net add link 1.0.0.1 1.0.0.3 1");
    $cbgp->send_cmd("net link 1.0.0.1 1.0.0.3 igp-weight --bidir 1");
    $cbgp->send_cmd("net add link 1.0.0.2 1.0.0.3 10");
    $cbgp->send_cmd("net link 1.0.0.2 1.0.0.3 igp-weight --bidir 10");
    $cbgp->send_cmd("net add link 1.0.0.2 192.168.0.1/24 1");
    $cbgp->send_cmd("net link 1.0.0.2 192.168.0.1/24 igp-weight 1");
    $cbgp->send_cmd("net add link 1.0.0.3 192.168.0.2/24 1");
    $cbgp->send_cmd("net link 1.0.0.3 192.168.0.2/24 igp-weight 1");
    $cbgp->send_cmd("net add link 1.0.0.4 192.168.0.3/24 1");
    $cbgp->send_cmd("net link 1.0.0.4 192.168.0.3/24 igp-weight 1");
    $cbgp->send_cmd("net domain 1 compute");

    # Domain 2
    $cbgp->send_cmd("net add domain 2 igp");
    $cbgp->send_cmd("net add node 2.0.0.0");

    # Interdomain link + static route
    $cbgp->send_cmd("net add link 1.0.0.3 2.0.0.0 1");
    $cbgp->send_cmd("net node 1.0.0.3 route add 2.0.0/24 * 2.0.0.0 1");

    # Default route from R1 to R4 should be R1 R2 R4
    # Default route from R1 to R5 should be R1 R2 R4 R5
    # Default route from R1 to R6 should be R1 R2 R4 R6

    # Create tunnels + static routes
    $cbgp->send_cmd("net node 1.0.0.3 ipip-enable");
    $cbgp->send_cmd("net node 1.0.0.0");
    $cbgp->send_cmd("  tunnel add --oif=1.0.0.2 1.0.0.3 255.0.0.0");
    $cbgp->send_cmd("  tunnel add 1.0.0.3 255.0.0.1");
    $cbgp->send_cmd("  tunnel add 1.0.0.4 255.0.0.2");
    $cbgp->send_cmd("  route add 1.0.0.3/32 * 255.0.0.0 1");
    $cbgp->send_cmd("  route add 2.0.0/24 * 255.0.0.1 1");
    $cbgp->send_cmd("  route add 1.0.0.1/32 * 255.0.0.2 1");

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

    $cbgp->send_cmd("net add node 1.0.0.1");
    $cbgp->send_cmd("net add node 1.0.0.2");
    $cbgp->send_cmd("net add link 1.0.0.1 1.0.0.2 10");

    # Test without static route: should fail
    $trace= cbgp_record_route($cbgp, "1.0.0.1", "1.0.0.2");
    if ($trace->[F_TR_STATUS] eq "SUCCESS") {
      return TEST_FAILURE;
    }

    # Test with static route: should succeed
    $cbgp->send_cmd("net node 1.0.0.1 route add 1.0.0.2/32 1.0.0.2 1.0.0.2 10");
    $trace= cbgp_record_route($cbgp, "1.0.0.1", "1.0.0.2");
    if ($trace->[F_TR_STATUS] ne "SUCCESS") {
      return TEST_FAILURE;
    }

    # Remove route and test: should fail
    $cbgp->send_cmd("net node 1.0.0.1 route del 1.0.0.2/32 1.0.0.2 1.0.0.2");
    $trace= cbgp_record_route($cbgp, "1.0.0.1", "1.0.0.2");
    if ($trace->[F_TR_STATUS] eq "SUCCESS") {
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

    $cbgp->send_cmd("net add node 1.0.0.1");
    $cbgp->send_cmd("net add node 1.0.0.2");
    $cbgp->send_cmd("net add node 1.0.0.3");
    $cbgp->send_cmd("net add link 1.0.0.1 1.0.0.2 10");
    $cbgp->send_cmd("net add link 1.0.0.1 1.0.0.3 20");
    $cbgp->send_cmd("net add link 1.0.0.2 1.0.0.3 20");

    # Test with static towards each node: should succeed
    $cbgp->send_cmd("net node 1.0.0.1 route add 1.0.0.2/32 * 1.0.0.2 10");
    $cbgp->send_cmd("net node 1.0.0.1 route add 1.0.0.3/32 * 1.0.0.3 10");
    $trace= cbgp_record_route($cbgp, "1.0.0.1", "1.0.0.2");
    if ($trace->[F_TR_STATUS] ne "SUCCESS") {
      $tests->debug("=> ERROR TEST(1)");
      return TEST_FAILURE;
    }
    $trace= cbgp_record_route($cbgp, "1.0.0.1", "1.0.0.3");
    if ($trace->[F_TR_STATUS] ne "SUCCESS") {
      $tests->debug("=> ERROR TEST(2)");
      return TEST_FAILURE;
    }

    # Remove routes using wildcard: should succeed
    $cbgp->send_cmd("net node 1.0.0.1 route del 1.0.0.2/32 * *");
    $trace= cbgp_record_route($cbgp, "1.0.0.1", "1.0.0.2");
    if ($trace->[F_TR_STATUS] eq "SUCCESS") {
      $tests->debug("=> ERROR TEST(3)");
      return TEST_FAILURE;
    }
    $trace= cbgp_record_route($cbgp, "1.0.0.1", "1.0.0.3");
    if ($trace->[F_TR_STATUS] ne "SUCCESS") {
      $tests->debug("=> ERROR TEST(4)");
      return TEST_FAILURE;
    }

    # Remove routes using wildcard: should succeed
    $cbgp->send_cmd("net node 1.0.0.1 route del 1.0.0.3/32 * *");
    $trace= cbgp_record_route($cbgp, "1.0.0.1", "1.0.0.2");
    if ($trace->[F_TR_STATUS] eq "SUCCESS") {
      $tests->debug("=> ERROR TEST(5)");
      return TEST_FAILURE;
    }
    $trace= cbgp_record_route($cbgp, "1.0.0.1", "1.0.0.3");
    if ($trace->[F_TR_STATUS] eq "SUCCESS") {
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

    $cbgp->send_cmd("net add node 1.0.0.1");
    $cbgp->send_cmd("net add node 1.0.0.2");
    $cbgp->send_cmd("net add node 1.0.0.3");
    $cbgp->send_cmd("net add link 1.0.0.1 1.0.0.2 10");
    $cbgp->send_cmd("net add link 1.0.0.1 1.0.0.3 10");
    $cbgp->send_cmd("net add link 1.0.0.2 1.0.0.3 10");

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

    $cbgp->send_cmd("net add node 1.0.0.1");
    $cbgp->send_cmd("net add node 1.0.0.2");
    $cbgp->send_cmd("net add subnet 192.168.0/24 transit");

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
    $cbgp->send_cmd("net add link 1.0.0.1 1.0.0.2 10");
    $msg= cbgp_check_error($cbgp,
			   "net node 1.0.0.1 route add 1.0.0.2/32 1.0.0.3 1.0.0.2 10");
    if (!defined($msg) || !($msg =~ m/$CBGP_ERROR_ROUTE_NH_UNREACH/)) {
      $tests->debug("Error: no/invalid error for nh:1.0.0.3, if:1.0.0.2");
      return TEST_FAILURE;
    }

    # Should not be able to add a route through a subnet, with a next-hop
    # equal to the outgoing interface
    $cbgp->send_cmd("net add link 1.0.0.1 192.168.0.1/24 10");
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
    $cbgp->send_cmd("net node 1.0.0.2 route add 1/8 * 1.0.0.1 10");
    $cbgp->send_cmd("net node 1.0.0.2 route add 1.1/16 * 1.0.0.3 10");

    # Test longest-matching in show-rt
    my $rt;
    $rt= cbgp_get_rt($cbgp, "1.0.0.2", "1.0.0.0");
    if (!exists($rt->{"1.0.0.0/8"}) ||
	($rt->{"1.0.0.0/8"}->[F_RT_IFACE] ne "1.0.0.1")) {
      return TEST_FAILURE;
    }
    $rt= cbgp_get_rt($cbgp, "1.0.0.2", "1.1.0.0");
    if (!exists($rt->{"1.1.0.0/16"}) ||
	($rt->{"1.1.0.0/16"}->[F_RT_IFACE] ne "1.0.0.3")) {
      return TEST_FAILURE;
    }
    # Test longest-matching in record-route
    my $trace;
    $trace= cbgp_record_route($cbgp, "1.0.0.2", "1.0.0.0");
    if ($trace->[F_TR_PATH]->[1] ne "1.0.0.1") {
      return TEST_FAILURE;
    }
    $trace= cbgp_record_route($cbgp, "1.0.0.2", "1.1.0.0");
    if ($trace->[F_TR_PATH]->[1] ne "1.0.0.3") {
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
    $cbgp->send_cmd("net node 1.0.0.2 route add 1.0.0.1/32 * 1.0.0.3 10");
    # Test protocol-priority in show-rt
    my $rt;
    $rt= cbgp_get_rt($cbgp, "1.0.0.2", "1.0.0.1");
    if (!exists($rt->{"1.0.0.1/32"}) ||
	($rt->{"1.0.0.1/32"}->[F_RT_PROTO] ne C_RT_PROTO_STATIC) ||
	($rt->{"1.0.0.1/32"}->[F_RT_IFACE] ne "1.0.0.3")) {
      print "1.0.0.2 --> 1.0.0.1\n";
      return TEST_FAILURE;
    }
    $rt= cbgp_get_rt($cbgp, "1.0.0.2", "1.0.0.3");
    if (!exists($rt->{"1.0.0.3/32"}) ||
	($rt->{"1.0.0.3/32"}->[F_RT_PROTO] ne C_RT_PROTO_IGP) ||
	($rt->{"1.0.0.3/32"}->[F_RT_IFACE] ne "1.0.0.3")) {
      print "1.0.0.2 --> 1.0.0.3\n";
      return TEST_FAILURE;
    }
    # Test protocol-priority in record-route
    my $trace;
    $trace= cbgp_record_route($cbgp, "1.0.0.2", "1.0.0.1");
    if ($trace->[F_TR_PATH]->[1] ne "1.0.0.3") {
      return TEST_FAILURE;
    }
    $trace= cbgp_record_route($cbgp, "1.0.0.2", "1.0.0.3");
    if ($trace->[F_TR_PATH]->[1] ne "1.0.0.3") {
      return TEST_FAILURE;
    }    
    return TEST_SUCCESS;
  }

# -----[ cbgp_valid_bgp_add_router ]---------------------------------
# Test ability to create a BGP router.
# -------------------------------------------------------------------
sub cbgp_valid_bgp_add_router($)
  {
    my ($cbgp)= @_;

    $cbgp->send_cmd("net add node 1.0.0.0");

    # Check that router could be added
    my $msg= cbgp_check_error($cbgp, "bgp add router 1 1.0.0.0");
    if (defined($msg)) {
      return TEST_FAILURE;
    }

    return TEST_SUCCESS;
  }

# -----[ cbgp_valid_bgp_add_router_dup ]-----------------------------
# Test that an error is returned when we try to create a BGP router
# that already exists.
# -------------------------------------------------------------------
sub cbgp_valid_bgp_add_router_dup($)
  {
    my ($cbgp)= @_;

    $cbgp->send_cmd("net add node 1.0.0.0");
    $cbgp->send_cmd("bgp add router 1 1.0.0.0");

    # Check that adding the same router twice fails
    my $msg= cbgp_check_error($cbgp, "bgp add router 1 1.0.0.0");
    if (!defined($msg) ||
	!($msg =~ m/could not register BGP/)) {
      return TEST_FAILURE;
    }

    return TEST_SUCCESS;
  }

# -----[ cbgp_valid_bgp_router_peer ]--------------------------------
# Test ability to add a neighbor (peer) to a BGP router.
# -------------------------------------------------------------------
sub cbgp_valid_bgp_router_add_peer($)
  {
    my ($cbgp)= @_;

    $cbgp->send_cmd("net add node 1.0.0.0");
    $cbgp->send_cmd("bgp add router 1 1.0.0.0");

    my $msg= cbgp_check_error($cbgp, "bgp router 1.0.0.0 add peer 2 2.0.0.0");
    if (defined($msg)) {
      return TEST_FAILURE;
    }

    return TEST_SUCCESS;
  }

# -----[ cbgp_valid_bgp_router_peer_dup ]----------------------------
# Test that an error is returned when we try to add a BGP neighbor
# that already exists.
# -------------------------------------------------------------------
sub cbgp_valid_bgp_router_add_peer_dup($)
  {
    my ($cbgp)= @_;
    $cbgp->send_cmd("net add node 1.0.0.0");
    $cbgp->send_cmd("bgp add router 1 1.0.0.0");
    $cbgp->send_cmd("bgp router 1.0.0.0 add peer 2 2.0.0.0");

    my $msg= cbgp_check_error($cbgp, "bgp router 1.0.0.0 add peer 2 2.0.0.0");
    if (!defined($msg) ||
	!($msg =~ m/peer already exists/)) {
      return TEST_FAILURE;
    }

    return TEST_SUCCESS;
  }

# -----[ cbgp_valid_bgp_router_peer_self ]---------------------------
# Test that an error is returned when we try to add a BGP neighbor
# with an address that is owned by the target router..
# -------------------------------------------------------------------
sub cbgp_valid_bgp_router_add_peer_self($)
  {
    my ($cbgp)= @_;

    $cbgp->send_cmd("net add node 1.0.0.0");
    $cbgp->send_cmd("bgp add router 1 1.0.0.0");

    my $msg= cbgp_check_error($cbgp, "bgp router 1.0.0.0 add peer 1 1.0.0.0");
    if (!defined($msg) ||
	!($msg =~ m/peer already exists/)) {
      return TEST_FAILURE;
    }

    return TEST_SUCCESS;
  }

# -----[ cbgp_valid_bgp_show_routers ]-------------------------------
# Test the ability to list all the BGP routers defined in the
# simulation.
# -------------------------------------------------------------------
sub cbgp_valid_bgp_show_routers($)
  {
    my ($cbgp)= @_;

    $cbgp->send_cmd("net add node 1.0.0.1");
    $cbgp->send_cmd("net add node 1.0.0.2");
    $cbgp->send_cmd("net add node 2.0.0.1");
    $cbgp->send_cmd("net add node 2.0.0.2");
    $cbgp->send_cmd("net add node 2.0.0.3");

    $cbgp->send_cmd("bgp add router 1 1.0.0.1");
    $cbgp->send_cmd("bgp add router 1 1.0.0.2");
    $cbgp->send_cmd("bgp add router 2 2.0.0.1");
    $cbgp->send_cmd("bgp add router 2 2.0.0.2");
    $cbgp->send_cmd("bgp add router 2 2.0.0.3");

    my $rtrs= cbgp_get_routers($cbgp);
    my @routers= sort (keys %$rtrs);
    if ((scalar(@routers) != 5) || ($routers[0] ne '1.0.0.1') ||
	($routers[1] ne '1.0.0.2') || ($routers[2] ne '2.0.0.1') ||
	($routers[3] ne '2.0.0.2') || ($routers[4] ne '2.0.0.3')) {
      return TEST_FAILURE;
    }

    $rtrs= cbgp_get_routers($cbgp, "1");
    @routers= sort (keys %$rtrs);
    if ((scalar(@routers) != 2) || ($routers[0] ne '1.0.0.1') ||
	($routers[1] ne '1.0.0.2')) {
      return TEST_FAILURE;
    }

    $rtrs= cbgp_get_routers($cbgp, "2.0.0.2");
    @routers= sort (keys %$rtrs);
    if ((scalar(@routers) != 1) || ($routers[0] ne '2.0.0.2')) {
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

    $cbgp->send_cmd("net add node 123.0.0.234");
    $cbgp->send_cmd("bgp add router 255 123.0.0.234");
    $cbgp->send_cmd("bgp router 123.0.0.234 add network 192.168.0/24");
    $cbgp->send_cmd("bgp options show-mode cisco");
    $cbgp->send_cmd("bgp router 123.0.0.234 show rib *");
    $cbgp->send_cmd("print \"done\\n\"");

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

    $cbgp->send_cmd("net add node 123.0.0.234");
    $cbgp->send_cmd("bgp add router 255 123.0.0.234");
    $cbgp->send_cmd("bgp router 123.0.0.234 add network 192.168.0/24");
    $cbgp->send_cmd("bgp options show-mode mrt");
    $cbgp->send_cmd("bgp router 123.0.0.234 show rib *");
    $cbgp->send_cmd("print \"done\\n\"");

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

    $cbgp->send_cmd("net add node 123.0.0.234");
    $cbgp->send_cmd("bgp add router 255 123.0.0.234");
    $cbgp->send_cmd("bgp router 123.0.0.234 add network 192.168.0/24");
    $cbgp->send_cmd("bgp options show-mode custom \"ROUTE:%i;%a;%p;%P;%C;%O;%n\"");
   $cbgp->send_cmd("bgp router 123.0.0.234 show rib *");
    $cbgp->send_cmd("print \"done\\n\"");

    while ((my $result= $cbgp->expect(1)) ne "done") {
      chomp $result;
      my @fields= split /\:/, $result, 2;
      if ($fields[0] ne "ROUTE") {
	return TEST_FAILURE;
      }

      @fields= split /;/, $fields[1];

      my $peer_ip= shift @fields;
      my $peer_as= shift @fields;
      my $prefix= shift @fields;
      my $path= shift @fields;
      my $cluster_id_list= shift @fields;
      my $originator_id= shift @fields;
      my $nexthop= shift @fields;

      if ($peer_ip ne "LOCAL") {
	return TEST_FAILURE;
      }
      if ($peer_as ne "LOCAL") {
	return TEST_FAILURE;
      }
      if ($prefix ne "192.168.0.0/24") {
	return TEST_FAILURE;
      }
      if ($path ne "") {
	return TEST_FAILURE;
      }
      if ($nexthop ne "123.0.0.234") {
	return TEST_FAILURE;
      }
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

    $cbgp->send_cmd("bgp topology load \"$topo_file\"");
    $cbgp->send_cmd("bgp topology install");

    # Check that all links are created
    (!cbgp_topo_check_links($cbgp, $topo)) and
      return TEST_FAILURE;

    return TEST_SUCCESS;
  }

# -----[ cbgp_valid_bgp_topology_load_local ]------------------------
# Test different addressing scheme for "bgp topology load".
# -------------------------------------------------------------------
sub cbgp_valid_bgp_topology_load_local($)
  {
    my ($cbgp)= @_;

    my $topo_file= $resources_path."valid-bgp-topology.subramanian";
    my $topo= topo_from_subramanian($topo_file, "local");

    $cbgp->send_cmd("bgp topology load --addr-sch=local \"$topo_file\"");
    $cbgp->send_cmd("bgp topology install");

    # Check that all links are created
    (!cbgp_topo_check_links($cbgp, $topo)) and
      return TEST_FAILURE;

    return TEST_SUCCESS;
  }

# -----[ cbgp_valid_bgp_topology_load_caida ]------------------------
#
# -------------------------------------------------------------------
sub cbgp_valid_bgp_topology_load_caida($)
  {
    my ($cbgp)= @_;
    my $filename= $resources_path."as-rel.20070416.a0.01000.txt";
    my $topo= topo_from_caida($filename);
    my $options= "--format=caida";

    $cbgp->send_cmd("bgp topology load $options \"$filename\"");
    $cbgp->send_cmd("bgp topology install");

    # Check that all links are created
    (!cbgp_topo_check_links($cbgp, $topo)) and
      return TEST_FAILURE;

    return TEST_SUCCESS;
  }

# -----[ cbgp_valid_bgp_topology_load_caida ]------------------------
#
# -------------------------------------------------------------------
sub cbgp_valid_bgp_topology_load_meulle($)
  {
    my ($cbgp)= @_;
    my $filename= "/tmp/as-level-meulle.txt";
    my $options= "--format=meulle";
    my $error;

    die if !open(AS_LEVEL, ">$filename");
    print AS_LEVEL "AS1\tAS2\tP2C\n";
    print AS_LEVEL "AS2\tAS3\tPEER\n";
    print AS_LEVEL "AS2\tAS1\tC2P\n";
    print AS_LEVEL "AS3\tAS2\tPEER\n";
    close(AS_LEVEL);

    $error= cbgp_check_error($cbgp,"bgp topology load $options \"$filename\"");
    defined($error) and return TEST_FAILURE;
    $error= cbgp_check_error($cbgp, "bgp topology install");
    defined($error) and return TEST_FAILURE;

    return TEST_SUCCESS;
  }

# -----[ cbgp_valid_bgp_topology_load_consistency ]------------------
#
# -------------------------------------------------------------------
sub cbgp_valid_bgp_topology_load_consistency($)
  {
    my ($cbgp)= @_;
    my $filename= $resources_path."as-level-inconsistent.txt";
    my $options= "--addr-sch=local --format=caida";

    my $error= cbgp_check_error($cbgp, "bgp topology load $options \"$filename\"");
    if (!defined($error) || !($error =~ m/topology is not consistent/)) {
      return TEST_FAILURE;
    }

    return TEST_SUCCESS;
  }

# -----[ cbgp_valid_bgp_topology_load_duplicate ]--------------------
#
# -------------------------------------------------------------------
sub cbgp_valid_bgp_topology_load_duplicate($)
  {
    my ($cbgp)= @_;
    my $filename= "/tmp/as-level-duplicate.txt";
    my $options= "--addr-sch=local --format=caida";

    die if !open(AS_LEVEL, ">$filename");
    print AS_LEVEL "1 2 -1\n1 2 0\n";
    close(AS_LEVEL);

    my $error= cbgp_check_error($cbgp, "bgp topology load $options \"$filename\"");
    if (!defined($error) || !($error =~ m/duplicate link/)) {
      return TEST_FAILURE;
    }

    unlink($filename);

    return TEST_SUCCESS;
  }

# -----[ cbgp_valid_bgp_topology_load_loop ]-------------------------
#
# -------------------------------------------------------------------
sub cbgp_valid_bgp_topology_load_loop($)
  {
    my ($cbgp)= @_;
    my $filename= "/tmp/as-level-loop.txt";
    my $options= "--addr-sch=local --format=caida";

    die if !open(AS_LEVEL, ">$filename");
    print AS_LEVEL "123 123 1\n";
    close(AS_LEVEL);

    my $error= cbgp_check_error($cbgp, "bgp topology load $options \"$filename\"");
    if (!defined($error) || !($error =~ m/loop link/)) {
      return TEST_FAILURE;
    }

    unlink($filename);

    return TEST_SUCCESS;
  }

# -----[ cbgp_valid_bgp_topology_check_cycle ]-----------------------
#
# -------------------------------------------------------------------
sub cbgp_valid_bgp_topology_check_cycle($)
  {
    my ($cbgp)= @_;
    my $filename= $resources_path."as-level-cycle.txt";
    my $options= "--addr-sch=local";

    $cbgp->send_cmd("bgp topology load $options \"$filename\"");
    cbgp_checkpoint($cbgp);
    my $error= cbgp_check_error($cbgp, "bgp topology check");
    if (!defined($error) || !($error =~ m/topology contains cycle\(s\)/)) {
      return TEST_FAILURE;
    }

    return TEST_SUCCESS;
  }

# -----[ cbgp_valid_bgp_topology_check_connectedness ]---------------
#
# -------------------------------------------------------------------
sub cbgp_valid_bgp_topology_check_connectedness($)
  {
    my ($cbgp)= @_;
    my $filename= $resources_path."as-level-disconnected.txt";
    my $options= "--addr-sch=local";

    $cbgp->send_cmd("bgp topology load $options \"$filename\"");
    cbgp_checkpoint($cbgp);
    my $error= cbgp_check_error($cbgp, "bgp topology check");
    if (!defined($error) || !($error =~ m/topology is not connected/)) {
      return TEST_FAILURE;
    }

    return TEST_SUCCESS;
  }

# -----[ cbgp_valid_bgp_topology_policies_filters ]------------------
# Test that routing filters are setup correctly by the "bgp topology
# policies" statement.
#
# Setup:
#   - AS1 (0.1.0.0)
#   - AS2 (0.2.0.0) provider of AS1
#   - AS3 (0.3.0.0) provider of AS1
#   - AS4 (0.4.0.0) peer of AS1
#   - AS5 (0.5.0.0) peer of AS1
#   - AS6 (0.6.0.0) customer of AS1
#   - AS7 (0.7.0.0) customer of AS1
#
# Topology:
#
#   AS2 ----*    *----- AS3
#            \  /
#   AS4 ----- AS1 ----- AS5
#            /  \
#   AS6 ----*    *----- AS7
#
# Scenario:
#   * Originate 255/8 from AS6, 254/8 from AS4, 253/8 from AS2 and
#     252/8 from AS1
#   * Check that AS1 has all routes
#   * Check that AS7 has all routes
#   * Check that AS5 has only AS1 and AS6
#   * Check that AS3 has only AS1 and AS6
#   * Originate 251/8 from AS3, AS5, AS7, check that AS1 selects AS7
#   * Originate 250/8 from AS3, AS5, check that AS1 selects AS5
#   * Originate 249/8 from AS5, AS7, check that AS1 selects AS7
#   * Originate 248/8 from AS3, AS7, check that AS1 selects AS7
# -------------------------------------------------------------------
sub cbgp_valid_bgp_topology_policies_filters($)
  {
    my ($cbgp)= @_;
    my $filename= "/tmp/as-level-policies-filter.topo";

    # Create Subramanian AS-level file
    open(AS_LEVEL_TOPO, ">$filename") or die;
    print AS_LEVEL_TOPO "2 1 1\n";
    print AS_LEVEL_TOPO "3 1 1\n";
    print AS_LEVEL_TOPO "1 4 0\n";
    print AS_LEVEL_TOPO "1 6 1\n";
    print AS_LEVEL_TOPO "1 5 0\n";
    print AS_LEVEL_TOPO "1 7 1\n";
    close(AS_LEVEL_TOPO);

    # Load topology and setup routing filters
    $cbgp->send_cmd("bgp topology load \"$filename\"");
    $cbgp->send_cmd("bgp topology install");
    $cbgp->send_cmd("bgp topology policies");
    $cbgp->send_cmd("bgp topology run");
    $cbgp->send_cmd("sim run");

    # Originate prefixes and propagate
    $cbgp->send_cmd("bgp router 0.1.0.0 add network 252/8");
    $cbgp->send_cmd("bgp router 0.2.0.0 add network 253/8");
    $cbgp->send_cmd("bgp router 0.4.0.0 add network 254/8");
    $cbgp->send_cmd("bgp router 0.6.0.0 add network 255/8");
    $cbgp->send_cmd("sim run");

    # Check valley-free property for 252-255/8
    my $rib= cbgp_get_rib($cbgp, "0.1.0.0");
    if (!exists($rib->{"252.0.0.0/8"}) || !exists($rib->{"253.0.0.0/8"}) ||
	!exists($rib->{"254.0.0.0/8"}) || !exists($rib->{"255.0.0.0/8"})) {
      $tests->debug("AS1 should have all routes");
      return TEST_FAILURE;
    }
    $rib= cbgp_get_rib($cbgp, "0.3.0.0");
    if (!exists($rib->{"252.0.0.0/8"}) || exists($rib->{"253.0.0.0/8"}) ||
	exists($rib->{"254.0.0.0/8"}) || !exists($rib->{"255.0.0.0/8"})) {
      $tests->debug("AS3 should only receive route from AS1 and AS6");
      return TEST_FAILURE;
    }
    $rib= cbgp_get_rib($cbgp, "0.5.0.0");
    if (!exists($rib->{"252.0.0.0/8"}) || exists($rib->{"253.0.0.0/8"}) ||
	exists($rib->{"254.0.0.0/8"}) || !exists($rib->{"255.0.0.0/8"})) {
      $tests->debug("AS5 should only receive route from AS1 and AS6");
      return TEST_FAILURE;
    }
    $rib= cbgp_get_rib($cbgp, "0.7.0.0");
    if (!exists($rib->{"252.0.0.0/8"}) || !exists($rib->{"253.0.0.0/8"}) ||
	!exists($rib->{"254.0.0.0/8"}) || !exists($rib->{"255.0.0.0/8"})) {
      $tests->debug("AS7 should receive all routes");
      return TEST_FAILURE;
    }

    # Originate prefixes and propagate
    $cbgp->send_cmd("bgp router 0.3.0.0 add network 251/8");
    $cbgp->send_cmd("bgp router 0.5.0.0 add network 251/8");
    $cbgp->send_cmd("bgp router 0.7.0.0 add network 251/8");
    $cbgp->send_cmd("bgp router 0.3.0.0 add network 250/8");
    $cbgp->send_cmd("bgp router 0.5.0.0 add network 250/8");
    $cbgp->send_cmd("bgp router 0.5.0.0 add network 249/8");
    $cbgp->send_cmd("bgp router 0.7.0.0 add network 249/8");
    $cbgp->send_cmd("bgp router 0.3.0.0 add network 248/8");
    $cbgp->send_cmd("bgp router 0.7.0.0 add network 248/8");
    $cbgp->send_cmd("sim run");

    # Check preferences for 248-251/8
    $rib= cbgp_get_rib($cbgp, "0.1.0.0");
    if (!exists($rib->{"248.0.0.0/8"}) || !exists($rib->{"249.0.0.0/8"}) ||
	!exists($rib->{"250.0.0.0/8"}) || !exists($rib->{"251.0.0.0/8"})) {
      $tests->debug("AS1 should receive all routes");
      return TEST_FAILURE;
    }
    if (($rib->{"248.0.0.0/8"}->[F_RIB_NEXTHOP] ne '0.7.0.0') ||
	($rib->{"249.0.0.0/8"}->[F_RIB_NEXTHOP] ne '0.7.0.0') ||
	($rib->{"250.0.0.0/8"}->[F_RIB_NEXTHOP] ne '0.5.0.0') ||
	($rib->{"251.0.0.0/8"}->[F_RIB_NEXTHOP] ne '0.7.0.0')) {
      $tests->debug("AS1's route selection is incorrect");
      return TEST_FAILURE;
    }

    # Remove temporary file
    unlink($filename);

    return TEST_SUCCESS;
  }

# -----[ cbgp_valid_bgp_topology_policies_filters_caida ]------------
# Test that routing filters are setup correctly by the "bgp topology
# policies" statement, in case of the CAIDA format, i.e. including
# sibling-to-sibling relationships.
#
# Setup:
#   - AS1 (0.1.0.0)
#   - AS2 (0.2.0.0) provider of AS1
#   - AS3 (0.3.0.0) provider of AS1
#   - AS4 (0.4.0.0) peer of AS1
#   - AS5 (0.5.0.0) peer of AS1
#   - AS6 (0.6.0.0) customer of AS1
#   - AS7 (0.7.0.0) customer of AS1
#   - AS8 (0.8.0.0) sibling of AS1
#   - AS9 (0.9.0.0) sibling of AS1
#
# Topology:
#
#   AS2 ----*     *----- AS3
#            \   /
#   AS4 ----* \ / *----- AS5
#            \| |/
#             AS1
#            /| |\
#   AS8 ----* / \ *----- AS9
#            /   \
#   AS6 ----*     *----- AS7
#
# Scenario:
#   * Originate 255/8 from AS6, 254/8 from AS4, 253/8 from AS2,
#     252/8 from AS1, 240/8 from AS8
#   * Check that AS1 has all routes
#   * Check that AS7 has all routes
#   * Check that AS5 has only AS1, AS6 and AS8
#   * Check that AS3 has only AS1, AS6 and AS8
#   * Originate 251/8 from AS3, AS5, AS7, check that AS1 selects AS7
#   * Originate 250/8 from AS3, AS5, check that AS1 selects AS5
#   * Originate 249/8 from AS5, AS7, check that AS1 selects AS7
#   * Originate 248/8 from AS3, AS7, check that AS1 selects AS7
# -------------------------------------------------------------------
sub cbgp_valid_bgp_topology_policies_filters_caida($)
  {
    my ($cbgp)= @_;
    my $filename= "/tmp/as-level-policies-filter.topo";

    # Create CAIDA AS-level file
    open(AS_LEVEL_TOPO, ">$filename") or die;
    print AS_LEVEL_TOPO "2 1 1\n";
    print AS_LEVEL_TOPO "1 2 -1\n";
    print AS_LEVEL_TOPO "3 1 1\n";
    print AS_LEVEL_TOPO "1 3 -1\n";
    print AS_LEVEL_TOPO "1 4 0\n";
    print AS_LEVEL_TOPO "4 1 0\n";
    print AS_LEVEL_TOPO "1 6 1\n";
    print AS_LEVEL_TOPO "6 1 -1\n";
    print AS_LEVEL_TOPO "1 5 0\n";
    print AS_LEVEL_TOPO "5 1 0\n";
    print AS_LEVEL_TOPO "1 7 1\n";
    print AS_LEVEL_TOPO "7 1 -1\n";
    print AS_LEVEL_TOPO "8 1 2\n";
    print AS_LEVEL_TOPO "1 8 2\n";
    print AS_LEVEL_TOPO "9 1 2\n";
    print AS_LEVEL_TOPO "1 9 2\n";
    close(AS_LEVEL_TOPO);

    # Load topology and setup routing filters
    $cbgp->send_cmd("bgp topology load --format=caida \"$filename\"");
    $cbgp->send_cmd("bgp topology install");
    $cbgp->send_cmd("bgp topology policies");
    $cbgp->send_cmd("bgp topology run");
    $cbgp->send_cmd("sim run");

    # Originate prefixes and propagate
    $cbgp->send_cmd("bgp router 0.1.0.0 add network 252/8");
    $cbgp->send_cmd("bgp router 0.2.0.0 add network 253/8");
    $cbgp->send_cmd("bgp router 0.4.0.0 add network 254/8");
    $cbgp->send_cmd("bgp router 0.6.0.0 add network 255/8");
    $cbgp->send_cmd("sim run");

    # Check valley-free property for 252-255/8
    my $rib= cbgp_get_rib($cbgp, "0.1.0.0");
    if (!exists($rib->{"252.0.0.0/8"}) || !exists($rib->{"253.0.0.0/8"}) ||
	!exists($rib->{"254.0.0.0/8"}) || !exists($rib->{"255.0.0.0/8"})) {
      $tests->debug("AS1 should have all routes");
      return TEST_FAILURE;
    }
    $rib= cbgp_get_rib($cbgp, "0.3.0.0");
    if (!exists($rib->{"252.0.0.0/8"}) || exists($rib->{"253.0.0.0/8"}) ||
	exists($rib->{"254.0.0.0/8"}) || !exists($rib->{"255.0.0.0/8"})) {
      $tests->debug("AS3 should only receive route from AS1 and AS6");
      return TEST_FAILURE;
    }
    $rib= cbgp_get_rib($cbgp, "0.5.0.0");
    if (!exists($rib->{"252.0.0.0/8"}) || exists($rib->{"253.0.0.0/8"}) ||
	exists($rib->{"254.0.0.0/8"}) || !exists($rib->{"255.0.0.0/8"})) {
      $tests->debug("AS5 should only receive route from AS1 and AS6");
      return TEST_FAILURE;
    }
    $rib= cbgp_get_rib($cbgp, "0.7.0.0");
    if (!exists($rib->{"252.0.0.0/8"}) || !exists($rib->{"253.0.0.0/8"}) ||
	!exists($rib->{"254.0.0.0/8"}) || !exists($rib->{"255.0.0.0/8"})) {
      $tests->debug("AS7 should receive all routes");
      return TEST_FAILURE;
    }

    # Originate prefixes and propagate
    $cbgp->send_cmd("bgp router 0.3.0.0 add network 251/8");
    $cbgp->send_cmd("bgp router 0.5.0.0 add network 251/8");
    $cbgp->send_cmd("bgp router 0.7.0.0 add network 251/8");
    $cbgp->send_cmd("bgp router 0.3.0.0 add network 250/8");
    $cbgp->send_cmd("bgp router 0.5.0.0 add network 250/8");
    $cbgp->send_cmd("bgp router 0.5.0.0 add network 249/8");
    $cbgp->send_cmd("bgp router 0.7.0.0 add network 249/8");
    $cbgp->send_cmd("bgp router 0.3.0.0 add network 248/8");
    $cbgp->send_cmd("bgp router 0.7.0.0 add network 248/8");
    $cbgp->send_cmd("sim run");

    # Check preferences for 248-251/8
    $rib= cbgp_get_rib($cbgp, "0.1.0.0");
    if (!exists($rib->{"248.0.0.0/8"}) || !exists($rib->{"249.0.0.0/8"}) ||
	!exists($rib->{"250.0.0.0/8"}) || !exists($rib->{"251.0.0.0/8"})) {
      $tests->debug("AS1 should receive all routes");
      return TEST_FAILURE;
    }
    if (($rib->{"248.0.0.0/8"}->[F_RIB_NEXTHOP] ne '0.7.0.0') ||
	($rib->{"249.0.0.0/8"}->[F_RIB_NEXTHOP] ne '0.7.0.0') ||
	($rib->{"250.0.0.0/8"}->[F_RIB_NEXTHOP] ne '0.5.0.0') ||
	($rib->{"251.0.0.0/8"}->[F_RIB_NEXTHOP] ne '0.7.0.0')) {
      $tests->debug("AS1's route selection is incorrect");
      return TEST_FAILURE;
    }

    # Remove temporary file
    unlink($filename);

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
    $cbgp->send_cmd("bgp topology load \"$topo_file\"");
    $cbgp->send_cmd("bgp topology install");
    $cbgp->send_cmd("bgp topology policies");

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

    $cbgp->send_cmd("bgp topology load \"$topo_file\"");
    $cbgp->send_cmd("bgp topology install");
    $cbgp->send_cmd("bgp topology policies");
    $cbgp->send_cmd("bgp topology run");
    if (!cbgp_topo_check_ebgp_sessions($cbgp, $topo, C_PEER_OPENWAIT)) {
      return TEST_FAILURE;
    }
    $cbgp->send_cmd("sim run");
    if (!cbgp_topo_check_ebgp_sessions($cbgp, $topo, C_PEER_ESTABLISHED)) {
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
    my $topo= topo_from_subramanian($topo_file);

    $cbgp->send_cmd("bgp topology load \"$topo_file\"");
    $cbgp->send_cmd("bgp topology install");

    # Check that all links are created
    (!cbgp_topo_check_links($cbgp, $topo)) and
      return TEST_FAILURE;

    # Check the propagation of two prefixes
    $cbgp->send_cmd("bgp topology policies");
    $cbgp->send_cmd("bgp topology run");
    $cbgp->send_cmd("sim run");
    # AS7018
    $cbgp->send_cmd("bgp router 27.106.0.0 add network 255/8");
    # AS8709
    $cbgp->send_cmd("bgp router 34.5.0.0 add network 245/8");
    $cbgp->send_cmd("sim run");

    my $rib= cbgp_get_rib($cbgp, "27.106.0.0");
    $rib= cbgp_get_rib($cbgp, "34.5.0.0");
    return TEST_SUCCESS;
  }

# -----[ cbgp_valid_bgp_topology_smalltopo ]-------------------------
#
# -------------------------------------------------------------------
sub cbgp_valid_bgp_topology_smalltopo($)
  {
    my ($cbgp)= @_;
    my $topo_file= $resources_path."data/small.topology.txt";
    my $topo= topo_from_subramanian($topo_file);

    $cbgp->send_cmd("bgp topology load \"$topo_file\"");
    $cbgp->send_cmd("bgp topology install");

    # Check that all links are created
    (!cbgp_topo_check_links($cbgp, $topo)) and
      return TEST_FAILURE;

    # Check the propagation of two prefixes
    $cbgp->send_cmd("bgp topology policies");
    $cbgp->send_cmd("bgp topology run");
    $cbgp->send_cmd("sim run");
    # AS0
    $cbgp->send_cmd("bgp router 0.0.0.0 add network 255/8");
    # AS11922
    $cbgp->send_cmd("bgp router 46.146.0.0 add network 254/8");
    $cbgp->send_cmd("sim run");

    my $rib= cbgp_get_rib($cbgp, "0.0.0.0");
    $rib= cbgp_get_rib($cbgp, "46.146.0.0");
    return TEST_SUCCESS;
  }

# -----[ cbgp_valid_bgp_topology_largetopo ]-------------------------
#
# -------------------------------------------------------------------
sub cbgp_valid_bgp_topology_largetopo($)
  {
    my ($cbgp)= @_;
    my $topo_file= $resources_path."data/large.topology.txt";
    my $topo= topo_from_subramanian($topo_file);

    $cbgp->send_cmd("bgp topology load \"$topo_file\"");
    $cbgp->send_cmd("bgp topology install");

    # Check that all links are created
    (!cbgp_topo_check_links($cbgp, $topo)) and
      return TEST_FAILURE;

    # Check the propagation of two prefixes
    $cbgp->send_cmd("bgp topology policies");
    $cbgp->send_cmd("bgp topology run");
    $cbgp->send_cmd("sim run");
    # AS0
    $cbgp->send_cmd("bgp router 0.0.0.0 add network 255/8");
    # AS14694
    $cbgp->send_cmd("bgp router 57.102.0.0 add network 254/8");
    $cbgp->send_cmd("sim run");

    my $rib= cbgp_get_rib($cbgp, "0.0.0.0");
    $rib= cbgp_get_rib($cbgp, "57.102.0.0");
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
    return TEST_FAILURE
      if (!cbgp_check_peer($cbgp, "1.0.0.1", "1.0.0.2",
			   -state=>C_PEER_OPENWAIT) ||
	  !cbgp_check_peer($cbgp, "1.0.0.2", "1.0.0.1",
			   -state=>C_PEER_OPENWAIT) ||
	  !cbgp_check_peer($cbgp, "1.0.0.2", "1.0.0.3",
			   -state=>C_PEER_OPENWAIT) ||
	  !cbgp_check_peer($cbgp, "1.0.0.3", "1.0.0.2",
			   -state=>C_PEER_OPENWAIT));
    $cbgp->send_cmd("sim run");
    return TEST_FAILURE
      if (!cbgp_check_peer($cbgp, "1.0.0.1", "1.0.0.2",
			   -state=>C_PEER_ESTABLISHED) ||
	  !cbgp_check_peer($cbgp, "1.0.0.2", "1.0.0.1",
			   -state=>C_PEER_ESTABLISHED) ||
	  !cbgp_check_peer($cbgp, "1.0.0.2", "1.0.0.3",
			   -state=>C_PEER_ESTABLISHED) ||
	  !cbgp_check_peer($cbgp, "1.0.0.3", "1.0.0.2",
			   -state=>C_PEER_ESTABLISHED));
    $cbgp->send_cmd("bgp router 1.0.0.1 add network 255.255.0.0/16");
    $cbgp->send_cmd("sim run");

    my $rib;
    # Check that 1.0.0.2 has received the route, that the next-hop is
    # 1.0.0.1 and the AS-path is empty. 
    $rib= cbgp_get_rib($cbgp, "1.0.0.2");
    return TEST_FAILURE
      if (!check_has_bgp_route($rib, "255.255/16",
			       -nexthop=>"1.0.0.1",
			       -path=>undef));

    # Check that 1.0.0.3 has NOT received the route (1.0.0.2 did not
    # send it since it was learned through an iBGP session).
    $rib= cbgp_get_rib($cbgp, "1.0.0.3");
    return TEST_FAILURE
      if (check_has_bgp_route($rib, "255.255/16"));

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
    $cbgp->send_cmd("bgp add router 1 1.0.0.1");
    $cbgp->send_cmd("bgp add router 2 1.0.0.2");
    $cbgp->send_cmd("bgp add router 2 1.0.0.3");
    cbgp_peering($cbgp, "1.0.0.1", "1.0.0.2", 2);
    cbgp_peering($cbgp, "1.0.0.2", "1.0.0.1", 1);
    cbgp_peering($cbgp, "1.0.0.2", "1.0.0.3", 2);
    cbgp_peering($cbgp, "1.0.0.3", "1.0.0.2", 2);
    return TEST_FAILURE
      if (!cbgp_check_peer($cbgp, "1.0.0.1", "1.0.0.2",
			   -state=>C_PEER_OPENWAIT) ||
	  !cbgp_check_peer($cbgp, "1.0.0.2", "1.0.0.1",
			   -state=>C_PEER_OPENWAIT));
    $cbgp->send_cmd("sim run");
    return TEST_FAILURE
      if (!cbgp_check_peer($cbgp, "1.0.0.1", "1.0.0.2",
			   -state=>C_PEER_ESTABLISHED) ||
	  !cbgp_check_peer($cbgp, "1.0.0.2", "1.0.0.1",
			   -state=>C_PEER_ESTABLISHED));
    $cbgp->send_cmd("bgp router 1.0.0.1 add network 255.255.0.0/16");
    $cbgp->send_cmd("bgp router 1.0.0.3 add network 255.254.0.0/16");
    $cbgp->send_cmd("sim run");

    # Check that AS-path contains remote ASN and that the next-hop is
    # the last router in remote AS
    my $rib= cbgp_get_rib($cbgp, "1.0.0.1");
    return TEST_FAILURE
      if (!check_has_bgp_route($rib, "255.254/16",
			       -nexthop=>"1.0.0.2",
			       -path=>[2]));

    # Check that AS-path contains remote ASN
    $rib= cbgp_get_rib($cbgp, "1.0.0.2");
    return TEST_FAILURE
      if (!check_has_bgp_route($rib, "255.254/16",
			       -nexthop=>"1.0.0.3",
			       -path=>undef));
    return TEST_FAILURE
      if (!check_has_bgp_route($rib, "255.255/16",
			       -nexthop=>"1.0.0.1",
			       -path=>[1]));

    # Check that AS-path contains remote ASN
    $rib= cbgp_get_rib($cbgp, "1.0.0.3");
    return TEST_FAILURE
      if (!check_has_bgp_route($rib, "255.255/16",
			       -nexthop=>"1.0.0.1",
			       -path=>[1]));

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

    $cbgp->send_cmd("net add domain 1 igp");
    $cbgp->send_cmd("net add node 1.0.0.1");
    $cbgp->send_cmd("net node 1.0.0.1 domain 1");
    $cbgp->send_cmd("net add node 1.0.0.2");
    $cbgp->send_cmd("net node 1.0.0.2 domain 1");
    $cbgp->send_cmd("net add domain 2 igp");
    $cbgp->send_cmd("net add node 2.0.0.1");
    $cbgp->send_cmd("net node 2.0.0.1 domain 2");
    $cbgp->send_cmd("net add subnet 192.168.0/24 transit");
    $cbgp->send_cmd("net add link 1.0.0.1 1.0.0.2 0");
    $cbgp->send_cmd("net link 1.0.0.1 1.0.0.2 igp-weight --bidir 10");
    $cbgp->send_cmd("net add link 1.0.0.1 192.168.0.1/24 0");
    $cbgp->send_cmd("net link 1.0.0.1 192.168.0.1/24 igp-weight 10");
    $cbgp->send_cmd("net add link 2.0.0.1 192.168.0.2/24 0");
    $cbgp->send_cmd("net link 2.0.0.1 192.168.0.2/24 igp-weight 10");
    $cbgp->send_cmd("net domain 1 compute");
    $cbgp->send_cmd("net domain 2 compute");
    $cbgp->send_cmd("bgp add router 1 1.0.0.1");
    $cbgp->send_cmd("bgp router 1.0.0.1");
    $cbgp->send_cmd("\tadd peer 1 1.0.0.2");
    $cbgp->send_cmd("\tpeer 1.0.0.2 up");
    $cbgp->send_cmd("\tadd peer 2 192.168.0.2");
    $cbgp->send_cmd("\tpeer 192.168.0.2 virtual");
    $cbgp->send_cmd("\tpeer 192.168.0.2 up");
    $cbgp->send_cmd("\texit");
    $cbgp->send_cmd("bgp add router 1 1.0.0.2");
    $cbgp->send_cmd("bgp router 1.0.0.2");
    $cbgp->send_cmd("\tadd peer 1 1.0.0.1");
    $cbgp->send_cmd("\tpeer 1.0.0.1 up");
    $cbgp->send_cmd("\texit");
    $cbgp->send_cmd("bgp router 1.0.0.1 peer 192.168.0.2 recv \"BGP4|0|A|1.0.0.1|1|255/8|2 3|IGP|192.168.0.2|12|34|\"");
    $cbgp->send_cmd("sim run");

    return TEST_FAILURE
      if (!cbgp_check_route($cbgp, '1.0.0.2', '255.0.0.0/8',
			    -iface=>'1.0.0.1',
			    -nexthop=>'0.0.0.0'));
    return TEST_FAILURE
      if (!cbgp_check_route($cbgp, '1.0.0.1', '255.0.0.0/8',
			    -iface=>'192.168.0.1',
			    -nexthop=>'192.168.0.2'));

    return TEST_SUCCESS;
  }

# -----[ cbgp_valid_bgp_session_router_id ]--------------------------
#
# -------------------------------------------------------------------
sub cbgp_valid_bgp_session_router_id($)
  {
    my ($cbgp)= @_;

    $cbgp->send_cmd("net add domain 1 igp");
    $cbgp->send_cmd("net add node 1.0.0.1");
    $cbgp->send_cmd("net add node 1.0.0.2");
    $cbgp->send_cmd("net add node 1.0.0.3");
    $cbgp->send_cmd("net add link 1.0.0.1 1.0.0.2 10");
    $cbgp->send_cmd("net add link 1.0.0.2 1.0.0.3 10");
    $cbgp->send_cmd("net domain 1 compute");
    $cbgp->send_cmd("bgp add router 1 1.0.0.1");
    $cbgp->send_cmd("bgp router 1.0.0.1");
    $cbgp->send_cmd("\tadd peer 1 1.0.0.2");
    $cbgp->send_cmd("\tpeer 1.0.0.2 up");
    $cbgp->send_cmd("\texit");
    $cbgp->send_cmd("bgp add router 3 1.0.0.3");
    $cbgp->send_cmd("bgp router 1.0.0.3");
    $cbgp->send_cmd("\tadd peer 1 1.0.0.2");
    $cbgp->send_cmd("\tpeer 1.0.0.2 up");
    $cbgp->send_cmd("\texit");

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
    $cbgp->send_cmd("net add domain 1 igp");
    $cbgp->send_cmd("net add node 1.0.0.1");
    $cbgp->send_cmd("net node 1.0.0.1 domain 1");
    $cbgp->send_cmd("net add node 1.0.0.2");
    $cbgp->send_cmd("net node 1.0.0.2 domain 1");
    $cbgp->send_cmd("net add link 1.0.0.1 1.0.0.2 0");
    $cbgp->send_cmd("net link 1.0.0.1 1.0.0.2 igp-weight --bidir 10");
    $cbgp->send_cmd("bgp add router 1 1.0.0.1");
    $cbgp->send_cmd("bgp router 1.0.0.1 add peer 1 1.0.0.2");
    $cbgp->send_cmd("bgp add router 1 1.0.0.2");
    $cbgp->send_cmd("bgp router 1.0.0.2 add peer 1 1.0.0.1");

    # At this point, both peerings should be in state IDLE
    return TEST_FAILURE
      if (!cbgp_check_peer($cbgp, "1.0.0.1", "1.0.0.2",
			   -state=>C_PEER_IDLE) ||
	  !cbgp_check_peer($cbgp, "1.0.0.2", "1.0.0.1",
			   -state=>C_PEER_IDLE));

    $cbgp->send_cmd("bgp router 1.0.0.1 peer 1.0.0.2 up");
    # At this point, peering R1 -> R2 should be in state ACTIVE
    # because there is no route from R1 to R2
    return TEST_FAILURE
      if (!cbgp_check_peer($cbgp, "1.0.0.1", "1.0.0.2",
			   -state=>C_PEER_ACTIVE));

    $cbgp->send_cmd("net domain 1 compute");
    $cbgp->send_cmd("bgp router 1.0.0.1 rescan");
    # At this point, peering R1 -> R2 should be in state OPENWAIT
    return TEST_FAILURE
      if (!cbgp_check_peer($cbgp, "1.0.0.1", "1.0.0.2",
			   -state=>C_PEER_OPENWAIT));

    $cbgp->send_cmd("bgp router 1.0.0.2 peer 1.0.0.1 up");
    # At this point, peering R2 -> R1 should also be in state OPENWAIT
    return TEST_FAILURE
      if (!cbgp_check_peer($cbgp, "1.0.0.2", "1.0.0.1",
			   -state=>C_PEER_OPENWAIT));

    $cbgp->send_cmd("sim run");
    # At this point, both peerings should be in state ESTABLISHED
    return TEST_FAILURE
      if (!cbgp_check_peer($cbgp, "1.0.0.1", "1.0.0.2",
			   -state=>C_PEER_ESTABLISHED) ||
	  !cbgp_check_peer($cbgp, "1.0.0.2", "1.0.0.1",
			   -state=>C_PEER_ESTABLISHED));

    $cbgp->send_cmd("bgp router 1.0.0.1 peer 1.0.0.2 down");
    # At this point, peering R1 -> R2 should be in state IDLE
    return TEST_FAILURE
      if (!cbgp_check_peer($cbgp, "1.0.0.1", "1.0.0.2",
			   -state=>C_PEER_IDLE));

    $cbgp->send_cmd("sim run");
    # At this point, peering R2 -> R1 should be in state ACTIVE
    return TEST_FAILURE
      if (!cbgp_check_peer($cbgp, "1.0.0.2", "1.0.0.1",
			   -state=>C_PEER_ACTIVE));

    $cbgp->send_cmd("bgp router 1.0.0.1 peer 1.0.0.2 up");
    $cbgp->send_cmd("sim run");
    # At this point, both peerings should be in state ESTABLISHED
    return TEST_FAILURE
      if (!cbgp_check_peer($cbgp, "1.0.0.1", "1.0.0.2",
			   -state=>C_PEER_ESTABLISHED) ||
	  !cbgp_check_peer($cbgp, "1.0.0.2", "1.0.0.1",
			   -state=>C_PEER_ESTABLISHED));

    $cbgp->send_cmd("bgp router 1.0.0.1 peer 1.0.0.2 down");
    $cbgp->send_cmd("sim run");
    $cbgp->send_cmd("bgp router 1.0.0.2 peer 1.0.0.1 down");
    # At this point, peering R2 -> R1 should be in state IDLE
    return TEST_FAILURE
      if (!cbgp_check_peer($cbgp, "1.0.0.2", "1.0.0.1",
			   -state=>C_PEER_IDLE));

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
    $cbgp->send_cmd("bgp domain 1 full-mesh");
    if (!cbgp_topo_check_ibgp_sessions($cbgp, $topo, C_PEER_OPENWAIT)) {
      return TEST_FAILURE;
    }
    $cbgp->send_cmd("sim run");
    if (!cbgp_topo_check_ibgp_sessions($cbgp, $topo, C_PEER_ESTABLISHED)) {
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

    $cbgp->send_cmd("net add domain 1 igp");
    $cbgp->send_cmd("net add node 1.0.0.1");
    $cbgp->send_cmd("net node 1.0.0.1 domain 1");
    $cbgp->send_cmd("net add node 1.0.0.2");
    $cbgp->send_cmd("net node 1.0.0.2 domain 1");
    $cbgp->send_cmd("net add node 1.0.0.3");
    $cbgp->send_cmd("net node 1.0.0.3 domain 1");
    $cbgp->send_cmd("net add link 1.0.0.1 1.0.0.2 0");
    $cbgp->send_cmd("net link 1.0.0.1 1.0.0.2 igp-weight --bidir 10");
    $cbgp->send_cmd("net add link 1.0.0.2 1.0.0.3 10");
    $cbgp->send_cmd("net link 1.0.0.2 1.0.0.3 igp-weight --bidir 10");
    $cbgp->send_cmd("net domain 1 compute");
    $cbgp->send_cmd("bgp add router 1 1.0.0.1");
    $cbgp->send_cmd("bgp router 1.0.0.1");
    $cbgp->send_cmd("\tadd network 255/8");
    $cbgp->send_cmd("\tadd peer 1 1.0.0.2");
    $cbgp->send_cmd("\texit");
    $cbgp->send_cmd("bgp add router 1 1.0.0.2");
    $cbgp->send_cmd("bgp router 1.0.0.2");
    $cbgp->send_cmd("\tadd peer 1 1.0.0.1");
    $cbgp->send_cmd("\tadd peer 1 1.0.0.3");
    $cbgp->send_cmd("\texit");
    $cbgp->send_cmd("bgp add router 1 1.0.0.3");
    $cbgp->send_cmd("bgp router 1.0.0.3");
    $cbgp->send_cmd("\tadd network 255/8");
    $cbgp->send_cmd("\tadd peer 1 1.0.0.2");
    $cbgp->send_cmd("\texit");

    # All sessions should be in IDLE state
    return TEST_FAILURE
      if (!cbgp_check_peer($cbgp, "1.0.0.1", "1.0.0.2",
			   -state=>C_PEER_IDLE) ||
	  !cbgp_check_peer($cbgp, "1.0.0.2", "1.0.0.1",
			   -state=>C_PEER_IDLE) ||
	  !cbgp_check_peer($cbgp, "1.0.0.2", "1.0.0.3",
			   -state=>C_PEER_IDLE) ||
	  !cbgp_check_peer($cbgp, "1.0.0.3", "1.0.0.2",
			   -state=>C_PEER_IDLE));

    $cbgp->send_cmd("bgp router 1.0.0.1 start");
    $cbgp->send_cmd("bgp router 1.0.0.2 start");
    $cbgp->send_cmd("bgp router 1.0.0.3 start");
    # All sessions should be in OPENWAIT state
    return TEST_FAILURE
      if (!cbgp_check_peer($cbgp, "1.0.0.1", "1.0.0.2",
			   -state=>C_PEER_OPENWAIT) ||
	  !cbgp_check_peer($cbgp, "1.0.0.2", "1.0.0.1",
			   -state=>C_PEER_OPENWAIT) ||
	  !cbgp_check_peer($cbgp, "1.0.0.2", "1.0.0.3",
			   -state=>C_PEER_OPENWAIT) ||
	  !cbgp_check_peer($cbgp, "1.0.0.3", "1.0.0.2",
			   -state=>C_PEER_OPENWAIT));

    $cbgp->send_cmd("sim run");
    # All sessions should be in ESTABLISHED state
    return TEST_FAILURE
      if (!cbgp_check_peer($cbgp, "1.0.0.1", "1.0.0.2",
			   -state=>C_PEER_ESTABLISHED) ||
	  !cbgp_check_peer($cbgp, "1.0.0.2", "1.0.0.1",
			   -state=>C_PEER_ESTABLISHED) ||
	  !cbgp_check_peer($cbgp, "1.0.0.2", "1.0.0.3",
			   -state=>C_PEER_ESTABLISHED) ||
	  !cbgp_check_peer($cbgp, "1.0.0.3", "1.0.0.2",
			   -state=>C_PEER_ESTABLISHED));

    $cbgp->send_cmd("bgp router 1.0.0.1 stop");
    $cbgp->send_cmd("bgp router 1.0.0.2 stop");
    $cbgp->send_cmd("bgp router 1.0.0.3 stop");
    $cbgp->send_cmd("sim run");
    # All sessions should be in IDLE state
    return TEST_FAILURE
      if (!cbgp_check_peer($cbgp, "1.0.0.1", "1.0.0.2",
			   -state=>C_PEER_IDLE) ||
	  !cbgp_check_peer($cbgp, "1.0.0.2", "1.0.0.1",
			   -state=>C_PEER_IDLE) ||
	  !cbgp_check_peer($cbgp, "1.0.0.2", "1.0.0.3",
			   -state=>C_PEER_IDLE) ||
	  !cbgp_check_peer($cbgp, "1.0.0.3", "1.0.0.2",
			   -state=>C_PEER_IDLE));

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
    $cbgp->send_cmd("bgp domain 1 full-mesh");
    $cbgp->send_cmd("net add node 2.0.0.1");
    $cbgp->send_cmd("net add link 1.0.0.1 2.0.0.1 0");
    $cbgp->send_cmd("net node 1.0.0.1 route add 2.0.0.1/32 * 2.0.0.1 0");
    $cbgp->send_cmd("net node 2.0.0.1 route add 1.0.0.1/32 * 1.0.0.1 0");
    $cbgp->send_cmd("bgp add router 2 2.0.0.1");
    $cbgp->send_cmd("bgp router 2.0.0.1 add network 2.0.0.0/8");
    cbgp_peering($cbgp, "2.0.0.1", "1.0.0.1", 1);
    cbgp_peering($cbgp, "1.0.0.1", "2.0.0.1", 2, "next-hop-self");
    $cbgp->send_cmd("sim run");
    # Next-hop must be unchanged in 1.0.0.1
    my $rib= cbgp_get_rib($cbgp, "1.0.0.1");
    return TEST_FAILURE
      if (!check_has_bgp_route($rib, "2/8",
			       -nexthop=>"2.0.0.1"));

    # Next-hop must be updated with address of 1.0.0.1 in 1.0.0.2
    $rib= cbgp_get_rib($cbgp, "1.0.0.2");
    return TEST_FAILURE
      if (!check_has_bgp_route($rib, "2/8",
			       -nexthop=>"1.0.0.1"));

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
    $cbgp->send_cmd("sim run");
    return TEST_FAILURE
      if (!cbgp_check_peer($cbgp, "1.0.0.1", "2.0.0.1",
			   -state=>C_PEER_ESTABLISHED));
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
    $cbgp->send_cmd("bgp domain 1 full-mesh");
    $cbgp->send_cmd("net add node 2.0.0.1");
    $cbgp->send_cmd("net add link 1.0.0.1 2.0.0.1 0");
    $cbgp->send_cmd("net node 1.0.0.1 route add 2.0.0.1/32 * 2.0.0.1 0");
    cbgp_peering($cbgp, "1.0.0.1", "2.0.0.1", 2, "next-hop-self", "virtual");
    $cbgp->send_cmd("sim run");
    cbgp_recv_update($cbgp, "1.0.0.1", 1, "2.0.0.1",
		     "255.255/16|2|IGP|2.0.0.1|12|34");
    $cbgp->send_cmd("sim run");

    my $rib;
    # Check attributes of received route (note: local-pref has been
    # reset since the route is received over an eBGP session).
    $rib= cbgp_get_rib($cbgp, "1.0.0.1");
    return TEST_FAILURE
      if (!check_has_bgp_route($rib, "255.255/16",
			      -nexthop=>"2.0.0.1",
			      -pref=>0,
			      -med=>34,
			      -path=>[2]));

    $cbgp->send_cmd("bgp router 1.0.0.1 peer 2.0.0.1 recv ".
		    "\"BGP4|0|W|1.0.0.1|1|255.255.0.0/16\"");
    $cbgp->send_cmd("sim run");

    # Check that the route has been withdrawn
    $rib= cbgp_get_rib($cbgp, "1.0.0.1");
    return TEST_FAILURE
      if (check_has_bgp_route($rib, "255.255/16"));

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
    $cbgp->send_cmd("bgp add router 1 1.0.0.1");
    $cbgp->send_cmd("bgp router 1.0.0.1");
    $cbgp->send_cmd("  add network 255/8");
    $cbgp->send_cmd("  add peer 2 1.0.0.2");
    $cbgp->send_cmd("  peer 1.0.0.2 up");
    $cbgp->send_cmd("  exit");
    $cbgp->send_cmd("bgp add router 2 1.0.0.2");
    $cbgp->send_cmd("bgp router 1.0.0.2");
    $cbgp->send_cmd("  add peer 1 1.0.0.1");
    $cbgp->send_cmd("  peer 1.0.0.1 up");
    $cbgp->send_cmd("  add peer 3 1.0.0.3");
    $cbgp->send_cmd("  peer 1.0.0.3 up");
    $cbgp->send_cmd("  exit");
    $cbgp->send_cmd("bgp add router 3 1.0.0.3");
    $cbgp->send_cmd("bgp router 1.0.0.3");
    $cbgp->send_cmd("  add peer 2 1.0.0.2");
    $cbgp->send_cmd("  peer 1.0.0.2 up");
    $cbgp->send_cmd("  exit");

    my $trace= cbgp_bgp_record_route($cbgp, "1.0.0.3", "255/8");
    if (!defined($trace) ||
	($trace->[F_TR_STATUS] ne "UNREACHABLE")) {
      return TEST_FAILURE;
    }

    $cbgp->send_cmd("sim run");

    $trace= cbgp_bgp_record_route($cbgp, "1.0.0.3", "255/8");
    if (!defined($trace) ||
	($trace->[F_TR_STATUS] ne "SUCCESS") ||
	!aspath_equals($trace->[F_TR_PATH], [3, 2, 1])) {
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
    #    $rib= cbgp_get_rib($cbgp, "1.0.0.1");
    #    (!exists($rib->{"255.255.0.0/16"})) and
    #	return TEST_FAILURE;
    #    $route= $rib->{"255.255.0.0/16"};
    #    print "AS-Path: ".$route->[F_RIB_PATH]."\n";
    #    ($route->[F_RIB_PATH] =~ m/^2 3 \[([456 ]+)\]$/) or
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
    $cbgp->send_cmd("sim run");
    return TEST_FAILURE
      if (!cbgp_check_peer($cbgp, "1.0.0.1", "2.0.0.1",
			   -state=>C_PEER_ESTABLISHED));
    $cbgp->send_cmd("bgp router 1.0.0.1 peer 2.0.0.1 recv ".
		    "\"BGP4|0|A|1.0.0.1|1|255.255.0.0/16|2|IGP|2.0.0.1|0|0\"");
    $cbgp->send_cmd("sim run");

    my $rib;

    # Check that route is available
    $rib= cbgp_get_rib($cbgp, "1.0.0.1");
    return TEST_FAILURE
      if (!check_has_bgp_route($rib, "255.255/16"));

    $cbgp->send_cmd("bgp router 1.0.0.1 peer 2.0.0.1 down");
    $cbgp->send_cmd("sim run");

    # Check that route has been withdrawn
    $rib= cbgp_get_rib($cbgp, "1.0.0.1");
    return TEST_FAILURE
      if (check_has_bgp_route($rib, "255.255/16"));

    $cbgp->send_cmd("bgp router 1.0.0.1 peer 2.0.0.1 up");
    $cbgp->send_cmd("sim run");
    # Check that route is still announced after peering up
    $rib= cbgp_get_rib($cbgp, "1.0.0.1");
    return TEST_FAILURE
      if (!check_has_bgp_route($rib, "255.255/16"));

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
sub cbgp_valid_bgp_aspath_loop($) {
  my ($cbgp)= @_;
  cbgp_topo_dp3($cbgp, ["2.0.0.1", 2, 0]);
  cbgp_recv_update($cbgp, "1.0.0.1", 1, "2.0.0.1",
		   "255/8|2 1|IGP|2.0.0.1|0|0");
  $cbgp->send_cmd("sim run");

  my $rib= cbgp_get_rib($cbgp, "1.0.0.1");
  return TEST_FAILURE
    if (check_has_bgp_route($rib, "255/8"));

  return TEST_SUCCESS;
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
sub cbgp_valid_bgp_ssld($) {
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
  $cbgp->send_cmd("sim run");
  cbgp_recv_update($cbgp, "1.0.0.1", 1, "3.0.0.1",
		   "255/8|3 2 255|IGP|3.0.0.1|0|0");
  cbgp_recv_update($cbgp, "1.0.0.1", 1, "3.0.0.1",
		   "254/8|3 4 255|IGP|3.0.0.1|0|0");
  $cbgp->send_cmd("sim run");
  my $rib= cbgp_get_rib($cbgp, "2.0.0.1");
  return TEST_FAILURE
    if (!check_has_bgp_route($rib, "254/8"));
  return TEST_FAILURE
    if (check_has_bgp_route($rib, "255/8"));

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
sub cbgp_valid_bgp_implicit_withdraw($) {
  my ($cbgp)= @_;
  cbgp_topo_dp3($cbgp,
		["1.0.0.2", 1, 10],
		["2.0.0.1", 2, 0],
		["2.0.0.2", 2, 0]);
  cbgp_recv_update($cbgp, "1.0.0.1", 1, "2.0.0.1",
		   "255/8|2 3 4|IGP|2.0.0.1|0|0");
  cbgp_recv_update($cbgp, "1.0.0.1", 1, "2.0.0.2",
		   "255/8|2 4|IGP|2.0.0.2|0|0");
  $cbgp->send_cmd("sim run");

  my $rib= cbgp_get_rib($cbgp, "1.0.0.1");
  return TEST_FAILURE
    if (!check_has_bgp_route($rib, "255/8",
			     -nexthop=>"2.0.0.2"));
  cbgp_recv_update($cbgp, "1.0.0.1", 1, "2.0.0.1",
		   "255/8|2 4|IGP|2.0.0.1|0|0");
  $cbgp->send_cmd("sim run");
  $rib= cbgp_get_rib($cbgp, "1.0.0.1");
  return TEST_FAILURE
    if (!check_has_bgp_route($rib, "255/8",
			     -nexthop=>"2.0.0.1"));
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
    $rib= cbgp_get_rib($cbgp, "1.0.0.1", "255/16");
    @routes= values %$rib; 
    if (scalar(@routes) < 1) {
      return TEST_FAILURE;
    }
    if ($routes[0]->[F_RIB_PREF] != 100) {
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
    $rib= cbgp_get_rib($cbgp, "1.0.0.1", "255/16");
    @routes= values %$rib; 
    if (scalar(@routes) < 1) {
      return TEST_FAILURE;
    }
    if ($routes[0]->[F_RIB_PREF] != 100) {
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
    $rib= cbgp_get_rib($cbgp, "1.0.0.1", "255/16");
    @routes= values %$rib; 
    if (scalar(@routes) < 1) {
      return TEST_FAILURE;
    }
    if (!aspath_equals($routes[0]->[F_RIB_PATH], [3, 5])) {
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

    my $rib= cbgp_get_rib_mrt($cbgp, "1.0.0.1");
    return TEST_FAILURE
      if (!check_has_bgp_route($rib, "255/8",
			       -nexthop=>"1.0.0.3"));

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
    $rib= cbgp_get_rib($cbgp, "1.0.0.1", "255/16");
    @routes= values %$rib;
    if (scalar(@routes) < 1) {
      return TEST_FAILURE;
    }
    if ($routes[0]->[F_RIB_NEXTHOP] ne "2.0.0.1") {
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
    $rib= cbgp_get_rib($cbgp, "1.0.0.1", "255/16");
    @routes= values %$rib; 
    if (scalar(@routes) < 1) {
      return TEST_FAILURE;
    }
    if ($routes[0]->[F_RIB_NEXTHOP] ne "2.0.0.2") {
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
    $rib= cbgp_get_rib($cbgp, "1.0.0.1", "255/16");
    @routes= values %$rib; 
    if (scalar(@routes) < 1) {
      return TEST_FAILURE;
    }
    if ($routes[0]->[F_RIB_NEXTHOP] ne "3.0.0.2") {
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
    $rib= cbgp_get_rib($cbgp, "1.0.0.1", "255/16");
    @routes= values %$rib; 
    if (scalar(@routes) < 1) {
      return TEST_FAILURE;
    }
    if ($routes[0]->[F_RIB_NEXTHOP] ne "1.0.0.4") {
      return TEST_FAILURE;
    }
    cbgp_recv_withdraw($cbgp, "1.0.0.1", 1, "1.0.0.4", "255/16");
    die if $cbgp->send("sim run\n");
    $rib= cbgp_get_rib($cbgp, "1.0.0.1", "255/16");
    @routes= values %$rib; 
    if (scalar(@routes) < 1) {
      return TEST_FAILURE;
    }
    if ($routes[0]->[F_RIB_NEXTHOP] ne "1.0.0.3") {
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

    $cbgp->send_cmd("net add domain 1 igp");
    $cbgp->send_cmd("net add node 1.0.0.1");
    $cbgp->send_cmd("net node 1.0.0.1 domain 1");
    $cbgp->send_cmd("net add node 1.0.0.2");
    $cbgp->send_cmd("net node 1.0.0.2 domain 1");
    $cbgp->send_cmd("net add node 1.0.0.3");
    $cbgp->send_cmd("net node 1.0.0.3 domain 1");
    $cbgp->send_cmd("net add node 1.0.0.4");
    $cbgp->send_cmd("net node 1.0.0.4 domain 1");
    $cbgp->send_cmd("net add node 1.0.0.5");
    $cbgp->send_cmd("net node 1.0.0.5 domain 1");
    $cbgp->send_cmd("net add link 1.0.0.1 1.0.0.2 0");
    $cbgp->send_cmd("net link 1.0.0.1 1.0.0.2 igp-weight --bidir 1");
    $cbgp->send_cmd("net add link 1.0.0.1 1.0.0.3 0");
    $cbgp->send_cmd("net link 1.0.0.1 1.0.0.3 igp-weight --bidir 1");
    $cbgp->send_cmd("net add link 1.0.0.2 1.0.0.4 0");
    $cbgp->send_cmd("net link 1.0.0.2 1.0.0.4 igp-weight --bidir 1");
    $cbgp->send_cmd("net add link 1.0.0.3 1.0.0.5 0");
    $cbgp->send_cmd("net link 1.0.0.3 1.0.0.5 igp-weight --bidir 1");
    $cbgp->send_cmd("net add link 1.0.0.4 1.0.0.5 0");
    $cbgp->send_cmd("net link 1.0.0.4 1.0.0.5 igp-weight --bidir 1");
    $cbgp->send_cmd("net domain 1 compute");

    $cbgp->send_cmd("bgp add router 1 1.0.0.2");
    $cbgp->send_cmd("bgp router 1.0.0.2");
    $cbgp->send_cmd("\tadd peer 1 1.0.0.1");
    $cbgp->send_cmd("\tpeer 1.0.0.1 virtual");
    $cbgp->send_cmd("\tpeer 1.0.0.1 up");
    $cbgp->send_cmd("\tadd peer 1 1.0.0.4");
    $cbgp->send_cmd("\tpeer 1.0.0.4 rr-client");
    $cbgp->send_cmd("\tpeer 1.0.0.4 up");
    $cbgp->send_cmd("\texit");

    $cbgp->send_cmd("bgp add router 1 1.0.0.3");
    $cbgp->send_cmd("bgp router 1.0.0.3");
    $cbgp->send_cmd("\tadd peer 1 1.0.0.1");
    $cbgp->send_cmd("\tpeer 1.0.0.1 virtual");
    $cbgp->send_cmd("\tpeer 1.0.0.1 up");
    $cbgp->send_cmd("\tadd peer 1 1.0.0.5");
    $cbgp->send_cmd("\tpeer 1.0.0.5 rr-client");
    $cbgp->send_cmd("\tpeer 1.0.0.5 up");
    $cbgp->send_cmd("\texit");

    $cbgp->send_cmd("bgp add router 1 1.0.0.4");
    $cbgp->send_cmd("bgp router 1.0.0.4");
    $cbgp->send_cmd("\tadd peer 1 1.0.0.2");
    $cbgp->send_cmd("\tpeer 1.0.0.2 up");
    $cbgp->send_cmd("\tadd peer 1 1.0.0.5");
    $cbgp->send_cmd("\tpeer 1.0.0.5 rr-client");
    $cbgp->send_cmd("\tpeer 1.0.0.5 up");
    $cbgp->send_cmd("\texit");

    $cbgp->send_cmd("bgp add router 1 1.0.0.5");
    $cbgp->send_cmd("bgp router 1.0.0.5");
    $cbgp->send_cmd("\tadd peer 1 1.0.0.3");
    $cbgp->send_cmd("\tpeer 1.0.0.3 up");
    $cbgp->send_cmd("\tadd peer 1 1.0.0.4");
    $cbgp->send_cmd("\tpeer 1.0.0.4 up");
    $cbgp->send_cmd("\texit");

    cbgp_recv_update($cbgp, "1.0.0.2", 1, "1.0.0.1",
		     "255/8|2|IGP|1.0.0.1|0|0|255:1");
    cbgp_recv_update($cbgp, "1.0.0.3", 1, "1.0.0.1",
		     "255/8|2|IGP|1.0.0.1|0|0|255:2");

    $cbgp->send_cmd("sim run");

    my $rib= cbgp_get_rib_custom($cbgp, "1.0.0.5");
    return TEST_FAILURE
      if (!check_has_bgp_route($rib, "255/8",
			       -community=>["255:2"]));
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
    $rib= cbgp_get_rib($cbgp, "1.0.0.1", "255/16");
    @routes= values %$rib; 
    if (scalar(@routes) < 1) {
      return TEST_FAILURE;
    }
    if ($routes[0]->[F_RIB_NEXTHOP] ne "2.0.0.1") {
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
    $cbgp->send_cmd("net add domain 1 igp");
    $cbgp->send_cmd("net add node 1.0.0.1");
    $cbgp->send_cmd("net add node 1.0.0.2");
    $cbgp->send_cmd("net add node 1.0.0.3");
    $cbgp->send_cmd("net add node 1.0.0.4");
    $cbgp->send_cmd("net add node 1.0.0.5");
    $cbgp->send_cmd("net node 1.0.0.1 domain 1");
    $cbgp->send_cmd("net node 1.0.0.2 domain 1");
    $cbgp->send_cmd("net node 1.0.0.3 domain 1");
    $cbgp->send_cmd("net node 1.0.0.4 domain 1");
    $cbgp->send_cmd("net node 1.0.0.5 domain 1");
    $cbgp->send_cmd("net add link 1.0.0.1 1.0.0.4 0");
    $cbgp->send_cmd("net link 1.0.0.1 1.0.0.4 igp-weight --bidir 1");
    $cbgp->send_cmd("net add link 1.0.0.2 1.0.0.3 0");
    $cbgp->send_cmd("net link 1.0.0.2 1.0.0.3 igp-weight --bidir 1");
    $cbgp->send_cmd("net add link 1.0.0.3 1.0.0.5 0");
    $cbgp->send_cmd("net link 1.0.0.3 1.0.0.5 igp-weight --bidir 1");
    $cbgp->send_cmd("net add link 1.0.0.4 1.0.0.5 0");
    $cbgp->send_cmd("net link 1.0.0.4 1.0.0.5 igp-weight --bidir 1");
    $cbgp->send_cmd("net domain 1 compute");
    $cbgp->send_cmd("bgp add router 1 1.0.0.3");
    $cbgp->send_cmd("bgp router 1.0.0.3");
    $cbgp->send_cmd("\tadd peer 1 1.0.0.2");
    $cbgp->send_cmd("\tpeer 1.0.0.2 virtual");
    $cbgp->send_cmd("\tpeer 1.0.0.2 rr-client");
    $cbgp->send_cmd("\tpeer 1.0.0.2 up");
    $cbgp->send_cmd("\tadd peer 1 1.0.0.5");
    $cbgp->send_cmd("\tpeer 1.0.0.5 up");
    $cbgp->send_cmd("\texit");
    $cbgp->send_cmd("bgp add router 1 1.0.0.4");
    $cbgp->send_cmd("bgp router 1.0.0.4");
    $cbgp->send_cmd("\tadd peer 1 1.0.0.1");
    $cbgp->send_cmd("\tpeer 1.0.0.1 virtual");
    $cbgp->send_cmd("\tpeer 1.0.0.1 rr-client");
    $cbgp->send_cmd("\tpeer 1.0.0.1 up");
    $cbgp->send_cmd("\tadd peer 1 1.0.0.5");
    $cbgp->send_cmd("\tpeer 1.0.0.5 up");
    $cbgp->send_cmd("\texit");
    $cbgp->send_cmd("bgp add router 1 1.0.0.5");
    $cbgp->send_cmd("bgp router 1.0.0.5");
    $cbgp->send_cmd("\tadd peer 1 1.0.0.3");
    $cbgp->send_cmd("\tpeer 1.0.0.3 up");
    $cbgp->send_cmd("\tadd peer 1 1.0.0.4");
    $cbgp->send_cmd("\tpeer 1.0.0.4 up");
    $cbgp->send_cmd("\texit");

    cbgp_recv_update($cbgp, "1.0.0.4", 1, "1.0.0.1",
		     "255/8||IGP|1.0.0.1|0|0");
    cbgp_recv_update($cbgp, "1.0.0.3", 1, "1.0.0.2",
		     "255/8||IGP|1.0.0.2|0|0");
    $cbgp->send_cmd("sim run");
    my $rib;
    my @routes;
    $rib= cbgp_get_rib_mrt($cbgp, "1.0.0.5", "255/8");
    return TEST_FAILURE
      if (!check_has_bgp_route($rib, "255/8",
			       -originator=>"1.0.0.1"));

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

    $cbgp->send_cmd("net add domain 1 igp");
    $cbgp->send_cmd("net add node 1.0.0.1");
    $cbgp->send_cmd("net node 1.0.0.1 domain 1");
    $cbgp->send_cmd("net add node 1.0.0.2");
    $cbgp->send_cmd("net node 1.0.0.2 domain 1");
    $cbgp->send_cmd("net add node 2.0.0.1");
    $cbgp->send_cmd("net node 2.0.0.1 domain 1");
    $cbgp->send_cmd("net add subnet 192.168.0.0/31 transit");
    $cbgp->send_cmd("net add subnet 192.168.0.2/31 transit");
    $cbgp->send_cmd("net add link 1.0.0.1 192.168.0.0/31 0");
    $cbgp->send_cmd("net link 1.0.0.1 192.168.0.0/31 igp-weight 1");
    $cbgp->send_cmd("net add link 1.0.0.1 192.168.0.2/31 0");
    $cbgp->send_cmd("net link 1.0.0.1 192.168.0.2/31 igp-weight 1");
    $cbgp->send_cmd("net add link 1.0.0.2 192.168.0.1/31 0");
    $cbgp->send_cmd("net link 1.0.0.2 192.168.0.1/31 igp-weight 1");
    $cbgp->send_cmd("net add link 1.0.0.2 192.168.0.3/31 0");
    $cbgp->send_cmd("net link 1.0.0.2 192.168.0.3/31 igp-weight 1");
    $cbgp->send_cmd("net add link 1.0.0.1 2.0.0.1 0");
    $cbgp->send_cmd("net link 1.0.0.1 2.0.0.1 igp-weight 1");
    $cbgp->send_cmd("net domain 1 compute");

    $cbgp->send_cmd("bgp add router 1 1.0.0.1");
    $cbgp->send_cmd("bgp router 1.0.0.1");
    $cbgp->send_cmd("\tadd peer 1 192.168.0.1");
    $cbgp->send_cmd("\tpeer 192.168.0.1 next-hop 192.168.0.0");
    $cbgp->send_cmd("\tpeer 192.168.0.1 up");
    $cbgp->send_cmd("\tadd peer 1 192.168.0.3");
    $cbgp->send_cmd("\tpeer 192.168.0.3 next-hop 192.168.0.2");
    $cbgp->send_cmd("\tpeer 192.168.0.3 up");
    $cbgp->send_cmd("\tadd peer 2 2.0.0.1");
    $cbgp->send_cmd("\tpeer 2.0.0.1 virtual");
    $cbgp->send_cmd("\tpeer 2.0.0.1 up");
    $cbgp->send_cmd("\texit");

    $cbgp->send_cmd("bgp add router 1 1.0.0.2");
    $cbgp->send_cmd("bgp router 1.0.0.2");
    $cbgp->send_cmd("\tadd peer 1 192.168.0.0");
    $cbgp->send_cmd("\tpeer 192.168.0.0 up");
    $cbgp->send_cmd("\tadd peer 1 192.168.0.2");
    $cbgp->send_cmd("\tpeer 192.168.0.2 up");
    $cbgp->send_cmd("\texit");

    cbgp_recv_update($cbgp, "1.0.0.1", 1, "2.0.0.1",
		     "255/8|2|IGP|2.0.0.1|0|0");

    $cbgp->send_cmd("sim run");
    my $rib= cbgp_get_rib_mrt($cbgp, "1.0.0.2");
    return TEST_FAILURE
      if (!check_has_bgp_route($rib, "255/8",
			       -nexthop=>"192.168.0.0"));
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
    $cbgp->send_cmd("sim run");
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
    $rib= cbgp_get_rib_mrt($cbgp, "1.0.0.1");
    return TEST_FAILURE
      if (!check_has_bgp_route($rib, "255.0/16",
			       -community=>["1:2"]));
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

    my $rib= cbgp_get_rib_mrt($cbgp, "1.0.0.1");
    return TEST_FAILURE
      if (!check_has_bgp_route($rib, "253/8",
			       -community=>["2:1", "2:3"]));
    return TEST_FAILURE
      if (!check_has_bgp_route($rib, "254/8",
			       -community=>["3:5"]));
    return TEST_FAILURE
      if (!check_has_bgp_route($rib, "255/8",
			       -community=>["4:0"]));
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

    my $rib= cbgp_get_rib_mrt($cbgp, "1.0.0.1");
    return TEST_FAILURE
      if (!check_has_bgp_route($rib, "255/8",
			       -community=>[]));
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
    my $rib= cbgp_get_rib($cbgp, "1.0.0.1");
    return TEST_FAILURE
      if (check_has_bgp_route($rib, "255/16"));
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
    $cbgp->send_cmd("bgp router 1.0.0.1 peer 2.0.0.1 recv ".
		    "\"BGP4|0|A|1.0.0.1|1|255/16|2|IGP|2.0.0.1|0|0\"");
    $cbgp->send_cmd("sim run");
    my $rib= cbgp_get_rib($cbgp, "1.0.0.1");
    return TEST_FAILURE
      if (!check_has_bgp_route($rib, "255.0/16",
			       -pref=>57));
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
    $cbgp->send_cmd("bgp router 1.0.0.1 peer 2.0.0.1 recv ".
		    "\"BGP4|0|A|1.0.0.1|1|255/16|2|IGP|2.0.0.1|0|0\"");
    $cbgp->send_cmd("sim run");

    my $rib= cbgp_get_rib($cbgp, "1.0.0.1");
    return TEST_FAILURE
      if (!check_has_bgp_route($rib, "255.0/16",
			      -med=>57));
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
    $cbgp->send_cmd("sim run");

    my $rib= cbgp_get_rib($cbgp, "2.0.0.1");
    return TEST_FAILURE
      if (!check_has_bgp_route($rib, "254/8", -med=>5));
    return TEST_FAILURE
      if (!check_has_bgp_route($rib, "255/8", -med=>10));
    return TEST_SUCCESS;
  }

# -----[ cbgp_valid_bgp_filter_action_aspath ]-----------------------
# -------------------------------------------------------------------
sub cbgp_valid_bgp_filter_action_aspath()
  {
    my ($cbgp)= @_;
    cbgp_topo_filter($cbgp);
    cbgp_filter($cbgp, "1.0.0.1", "2.0.0.1", "in", "any", "as-path prepend 2");
    $cbgp->send_cmd("bgp router 1.0.0.1 peer 2.0.0.1 recv ".
		    "\"BGP4|0|A|1.0.0.1|1|255/16|2|IGP|2.0.0.1|0|0\"");
    $cbgp->send_cmd("sim run");
    my $rib= cbgp_get_rib($cbgp, "1.0.0.1");
    return TEST_FAILURE
      if (!check_has_bgp_route($rib, "255.0/16",
			      -path=>[1, 1, 2]));
    return TEST_SUCCESS;
  }

# -----[ cbgp_valid_bgp_filter_action_aspath_rem_private ]-----------
# -------------------------------------------------------------------
sub cbgp_valid_bgp_filter_action_aspath_rem_private($)
  {
    my ($cbgp)= @_;
    cbgp_topo_filter($cbgp);
    cbgp_filter($cbgp, "1.0.0.1", "2.0.0.1", "in", "any", "as-path remove-private");
    $cbgp->send_cmd("bgp router 1.0.0.1 peer 2.0.0.1 recv ".
		    "\"BGP4|0|A|1.0.0.1|1|255/16|2 65535 9 64511 64512|IGP|2.0.0.1|0|0\"");
    $cbgp->send_cmd("sim run");
    my $rib= cbgp_get_rib($cbgp, "1.0.0.1");
    return TEST_FAILURE
      if (!check_has_bgp_route($rib, "255.0/16",
			       -path=>[2, 9, 64511]));
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
    my $rib= cbgp_get_rib_mrt($cbgp, "1.0.0.1");
    return TEST_FAILURE
      if (!check_has_bgp_route($rib, "255/8",
			       -community=>["1:2", "1:2000"]));
    return TEST_FAILURE
      if (!check_has_bgp_route($rib, "254/8",
			       -community=>["1:3", "1:2000"]));
    return TEST_FAILURE
      if (!check_has_bgp_route($rib, "253/8",
			       -community=>["1:2001"]));
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
    my $rib= cbgp_get_rib_mrt($cbgp, "1.0.0.1");
    return TEST_FAILURE
      if (!check_has_bgp_route($rib, "123.234/16",
			       -community=>["1:1"]));
    return TEST_FAILURE
      if (!check_has_bgp_route($rib, "123.235/16",
			       -community=>["1:2"]));
    return TEST_FAILURE
      if (!check_has_bgp_route($rib, "123.236/16",
			       -community=>[]));
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
    my $rib= cbgp_get_rib_mrt($cbgp, "1.0.0.1");
    return TEST_FAILURE
      if (!check_has_bgp_route($rib, "123.234.1/24",
			       -community=>["1:1"]));
    return TEST_FAILURE
      if (!check_has_bgp_route($rib, "123.235.1/24",
			       -community=>["1:2"]));
    return TEST_FAILURE
      if (!check_has_bgp_route($rib, "123.236.1/24",
			       -community=>[]));
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

    my $rib= cbgp_get_rib_mrt($cbgp, "1.0.0.1");
    return TEST_FAILURE
      if (!check_has_bgp_route($rib, "253.2/16",
			       -community=>["1:1"]));
    return TEST_FAILURE
      if (!check_has_bgp_route($rib, "254.2/16",
			       -community=>["1:1", "1:2"]));
    return TEST_FAILURE
      if (!check_has_bgp_route($rib, "255.2/16",
			       -community=>["1:1", "1:3"]));
    return TEST_FAILURE
      if (!check_has_bgp_route($rib, "253.3/16",
			       -community=>[]));
    return TEST_FAILURE
      if (!check_has_bgp_route($rib, "254.3/16",
			       -community=>["1:2"]));
    return TEST_FAILURE
      if (!check_has_bgp_route($rib, "255.3/16",
			       -community=>["1:3"]));
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
    my $rib= cbgp_get_rib_mrt($cbgp, "1.0.0.1");
    return TEST_FAILURE
      if (!check_has_bgp_route($rib, "255/8", -pref=>100));
    return TEST_FAILURE
      if (!check_has_bgp_route($rib, "254/8", -pref=>80));
    return TEST_FAILURE
      if (!check_has_bgp_route($rib, "253/8", -pref=>60));

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
    $cbgp->send_cmd("bgp route-map RM_LP_100");
    $cbgp->send_cmd("  add-rule");
    $cbgp->send_cmd("    match \"prefix in 128/8\"");
    $cbgp->send_cmd("    action \"local-pref 100\"");
    $cbgp->send_cmd("    exit");
    $cbgp->send_cmd("  exit");
    $cbgp->send_cmd("bgp route-map RM_LP_200");
    $cbgp->send_cmd("  add-rule");
    $cbgp->send_cmd("    match \"prefix in 0/8\"");
    $cbgp->send_cmd("    action \"local-pref 200\"");
    $cbgp->send_cmd("    exit");
    $cbgp->send_cmd("  exit");

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
    my $rib= cbgp_get_rib_mrt($cbgp, "1.0.0.1");
    return TEST_FAILURE
      if (!check_has_bgp_route($rib, "128/8", -pref=>100));
    return TEST_FAILURE
      if (!check_has_bgp_route($rib, "0/8", -pref=>200));

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

    $cbgp->send_cmd("bgp route-map RM_LP_100");
    $cbgp->send_cmd("  exit");
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
    $cbgp->send_cmd("sim run");

    my $rib= cbgp_get_rib_mrt($cbgp, "2.0.0.1");
    return TEST_FAILURE
      if (!check_has_bgp_route($rib, "255/8",
			       -community=>["255:1"]));

    # Increase weight of link R1-R2
    $cbgp->send_cmd("net link 1.0.0.1 1.0.0.2 igp-weight 100");
    $cbgp->send_cmd("net link 1.0.0.2 1.0.0.1 igp-weight 100");
    $cbgp->send_cmd("net domain 1 compute");
    $cbgp->send_cmd("bgp domain 1 rescan");
    $cbgp->send_cmd("sim run");

    $rib= cbgp_get_rib_mrt($cbgp, "2.0.0.1");
    return TEST_FAILURE
      if (!check_has_bgp_route($rib, "255/8",
			       -community=>["255:2"]));

    # Restore weight of link R1-R2
    $cbgp->send_cmd("net link 1.0.0.1 1.0.0.2 igp-weight 5");
    $cbgp->send_cmd("net link 1.0.0.2 1.0.0.1 igp-weight 5");
    $cbgp->send_cmd("net domain 1 compute");
    $cbgp->send_cmd("bgp domain 1 rescan");
    $cbgp->send_cmd("sim run");


    $rib= cbgp_get_rib_mrt($cbgp, "2.0.0.1");
    return TEST_FAILURE
      if (!check_has_bgp_route($rib, "255/8",
			       -community=>["255:1"]));
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
    $cbgp->send_cmd("sim run");

    my $rib= cbgp_get_rib_mrt($cbgp, "2.0.0.1");
    return TEST_FAILURE
      if(!check_has_bgp_route($rib, "255/8",
			     -community=>["255:1"]));

    # Increase weight of link R1-R2
    $cbgp->send_cmd("net link 1.0.0.1 1.0.0.2 down");
    $cbgp->send_cmd("net domain 1 compute");
    $cbgp->send_cmd("bgp domain 1 rescan");
    $cbgp->send_cmd("sim run");

    $rib= cbgp_get_rib_mrt($cbgp, "2.0.0.1");
    return TEST_FAILURE
      if (!check_has_bgp_route($rib, "255/8",
			       -community=>["255:2"]));

    # Restore weight of link R1-R2
    $cbgp->send_cmd("net link 1.0.0.1 1.0.0.2 up");
    $cbgp->send_cmd("net domain 1 compute");
    $cbgp->send_cmd("bgp domain 1 rescan");
    $cbgp->send_cmd("sim run");

    $rib= cbgp_get_rib_mrt($cbgp, "2.0.0.1");
    return TEST_FAILURE
      if (!check_has_bgp_route($rib, "255/8",
			       -community=>["255:1"]));
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
    $cbgp->send_cmd("sim run");

    my $rib= cbgp_get_rib_mrt($cbgp, "2.0.0.1");
    return TEST_FAILURE
      if (!check_has_bgp_route($rib, "255/8",
			       -community=>["255:1"],
			       -med=>5));

    # Increase weight of link R1-R2
    $cbgp->send_cmd("net link 1.0.0.1 1.0.0.2 igp-weight 100");
    $cbgp->send_cmd("net link 1.0.0.2 1.0.0.1 igp-weight 100");
    $cbgp->send_cmd("net domain 1 compute");
    $cbgp->send_cmd("bgp domain 1 rescan");
    $cbgp->send_cmd("sim run");

    $rib= cbgp_get_rib_mrt($cbgp, "2.0.0.1");
    return TEST_FAILURE
      if (!check_has_bgp_route($rib, "255/8",
			       -community=>["255:2"],
			       -med=>10));

    # Restore weight of link R1-R2
    $cbgp->send_cmd("net link 1.0.0.1 1.0.0.2 igp-weight 5");
    $cbgp->send_cmd("net link 1.0.0.2 1.0.0.1 igp-weight 5");
    $cbgp->send_cmd("net domain 1 compute");
    $cbgp->send_cmd("bgp domain 1 rescan");
    $cbgp->send_cmd("sim run");

    $rib= cbgp_get_rib_mrt($cbgp, "2.0.0.1");
    return TEST_FAILURE
      if (!check_has_bgp_route($rib, "255/8",
			       -community=>["255:1"],
			       -med=>5));
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
    my $rib_file= $resources_path."simple-rib.ascii";
    $cbgp->send_cmd("net add domain 1 igp");
    $cbgp->send_cmd("net add node 1.0.0.1");
    $cbgp->send_cmd("net node 1.0.0.1 domain 1");
    $cbgp->send_cmd("net add node 2.0.0.1");
    $cbgp->send_cmd("net node 2.0.0.1 domain 1");
    $cbgp->send_cmd("net add link 1.0.0.1 2.0.0.1 10");
    $cbgp->send_cmd("net link 1.0.0.1 2.0.0.1 igp-weight --bidir 10");
    $cbgp->send_cmd("net domain 1 compute");
    $cbgp->send_cmd("bgp add router 1 1.0.0.1");
    $cbgp->send_cmd("bgp router 1.0.0.1");
    $cbgp->send_cmd("\tadd peer 2 2.0.0.1");
    $cbgp->send_cmd("\tpeer 2.0.0.1 virtual");
    $cbgp->send_cmd("\tpeer 2.0.0.1 up");
    $cbgp->send_cmd("\texit");
    $cbgp->send_cmd("bgp router 1.0.0.1 load rib $rib_file");
    my $rib;
    $rib= cbgp_get_rib($cbgp, "1.0.0.1");
    if (scalar(keys %$rib) != `cat $rib_file | wc -l`) {
      $tests->debug("number of prefixes mismatch");
      return TEST_FAILURE;
    }
    open(RIB, "<$rib_file") or die;
    while (<RIB>) {
      chomp;
      my @fields= split /\|/;
      $rib= cbgp_get_rib($cbgp, "1.0.0.1", $fields[MRT_PREFIX]);
      if (scalar(keys %$rib) != 1) {
	print "no route\n";
	return TEST_FAILURE;
      }
      my $canonic= canonic_prefix($fields[MRT_PREFIX]);
      if (!exists($rib->{$canonic})) {
	print "could not find route towards prefix $fields[MRT_PREFIX]\n";
	return TEST_FAILURE;
      }
      if ($fields[MRT_NEXTHOP] ne
	  $rib->{$canonic}->[F_RIB_NEXTHOP]) {
	print "incorrect next-hop for $fields[MRT_PREFIX]\n";
	return TEST_FAILURE;
      }
    }
    close(RIB);
    return TEST_SUCCESS;
  }

# -----[ cbgp_valid_bgp_load_rib_full ]------------------------------
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
sub cbgp_valid_bgp_load_rib_full($)
  {
    my ($cbgp)= @_;
    my $rib_file= $resources_path."abilene-rib.ascii";
    $cbgp->send_cmd("net add node 198.32.12.9");
    $cbgp->send_cmd("bgp add router 11537 198.32.12.9");
    $cbgp->send_cmd("bgp router 198.32.12.9 load rib --autoconf $rib_file");
    my $rib;
    $rib= cbgp_get_rib($cbgp, "198.32.12.9");
    if (scalar(keys %$rib) != `cat $rib_file | wc -l`) {
      $tests->debug("number of prefixes mismatch");
      return TEST_FAILURE;
    }
    open(RIB, "<$rib_file") or die;
    while (<RIB>) {
      chomp;
      my @fields= split /\|/;
      $rib= cbgp_get_rib($cbgp, "198.32.12.9", $fields[MRT_PREFIX]);
      if (scalar(keys %$rib) != 1) {
	print "no route\n";
	return TEST_FAILURE;
      }
      my $canonic= canonic_prefix($fields[MRT_PREFIX]);
      if (!exists($rib->{$canonic})) {
	print "could not find route towards prefix $fields[MRT_PREFIX]\n";
	return TEST_FAILURE;
      }
      if ($fields[MRT_NEXTHOP] ne
	  $rib->{$canonic}->[F_RIB_NEXTHOP]) {
	print "incorrect next-hop for $fields[MRT_PREFIX]\n";
	return TEST_FAILURE;
      }
    }
    close(RIB);
    return TEST_SUCCESS;
  }

# -----[ cbgp_valid_bgp_load_rib_full_binary ]-----------------------
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
sub cbgp_valid_bgp_load_rib_full_binary($)
  {
    my ($cbgp)= @_;
    my $rib_file= $resources_path."abilene-rib.gz";
    my $rib_file_ascii= $resources_path."abilene-rib.marco";
    $cbgp->send_cmd("net add node 198.32.12.9");
    $cbgp->send_cmd("bgp add router 11537 198.32.12.9");
    $cbgp->send_cmd("bgp router 198.32.12.9 load rib --autoconf --format=mrt-binary $rib_file");

    # Use Marco d'Itri's zebra-dump-parser to compare content
    # File obtained using
    #   zcat abilene-rib.gz | ./zebra-dump-parser.pl > abilene-rib.marco
    my $num_routes= 0;
    open(ZEBRA_DUMP_PARSER, "$rib_file_ascii") or die;
    while (<ZEBRA_DUMP_PARSER>) {
      chomp;
      my @fields= split /\s+/, $_, 2;
      $num_routes++;
      my $rib= cbgp_get_rib($cbgp, "198.32.12.9", $fields[0]);
      if (!defined($rib) || !exists($rib->{$fields[0]})) {
	return TEST_FAILURE;
      }
    }
    close(ZEBRA_DUMP_PARSER);

    # Check that number of routes reported by Marco's script is
    # equal to number of routes in RIB.
    my $rib;
    $rib= cbgp_get_rib($cbgp, "198.32.12.9");
    if (scalar(keys %$rib) != $num_routes) {
      return TEST_FAILURE;
    }
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

    $cbgp->send_cmd("net add domain 1 igp");
    $cbgp->send_cmd("net add node 1.0.0.1");
    $cbgp->send_cmd("net node 1.0.0.1 domain 1");
    $cbgp->send_cmd("net add node 1.0.0.2");
    $cbgp->send_cmd("net node 1.0.0.2 domain 1");
    $cbgp->send_cmd("net add node 1.0.0.3");
    $cbgp->send_cmd("net node 1.0.0.3 domain 1");
    $cbgp->send_cmd("net add node 1.0.0.4");
    $cbgp->send_cmd("net node 1.0.0.4 domain 1");
    $cbgp->send_cmd("net add node 1.0.0.5");
    $cbgp->send_cmd("net node 1.0.0.5 domain 1");
    $cbgp->send_cmd("net add node 2.0.0.1");
    $cbgp->send_cmd("net node 2.0.0.1 domain 1");
    $cbgp->send_cmd("net add node 3.0.0.1");
    $cbgp->send_cmd("net node 3.0.0.1 domain 1");
    $cbgp->send_cmd("net add node 4.0.0.1");
    $cbgp->send_cmd("net node 4.0.0.1 domain 1");
    $cbgp->send_cmd("net add link 1.0.0.1 1.0.0.2 0");
    $cbgp->send_cmd("net link 1.0.0.1 1.0.0.2 igp-weight --bidir 1");
    $cbgp->send_cmd("net add link 1.0.0.1 2.0.0.1 0");
    $cbgp->send_cmd("net link 1.0.0.1 2.0.0.1 igp-weight --bidir 1");
    $cbgp->send_cmd("net add link 1.0.0.2 1.0.0.3 0");
    $cbgp->send_cmd("net link 1.0.0.2 1.0.0.3 igp-weight --bidir 1");
    $cbgp->send_cmd("net add link 1.0.0.2 1.0.0.5 0");
    $cbgp->send_cmd("net link 1.0.0.2 1.0.0.5 igp-weight --bidir 1");
    $cbgp->send_cmd("net add link 1.0.0.2 3.0.0.1 0");
    $cbgp->send_cmd("net link 1.0.0.2 3.0.0.1 igp-weight --bidir 1");
    $cbgp->send_cmd("net add link 1.0.0.3 1.0.0.4 0");
    $cbgp->send_cmd("net link 1.0.0.3 1.0.0.4 igp-weight --bidir 1");
    $cbgp->send_cmd("net add link 1.0.0.3 4.0.0.1 0");
    $cbgp->send_cmd("net link 1.0.0.3 4.0.0.1 igp-weight --bidir 1");
    $cbgp->send_cmd("net domain 1 compute");

    $cbgp->send_cmd("bgp add router 1 1.0.0.1");
    $cbgp->send_cmd("bgp router 1.0.0.1");
    $cbgp->send_cmd("\tadd peer 1 1.0.0.2");
    $cbgp->send_cmd("\tpeer 1.0.0.2 up");
    $cbgp->send_cmd("\tadd peer 2 2.0.0.1");
    $cbgp->send_cmd("\tpeer 2.0.0.1 virtual");
    $cbgp->send_cmd("\tpeer 2.0.0.1 up");
    $cbgp->send_cmd("\texit");

    $cbgp->send_cmd("bgp add router 1 1.0.0.2");
    $cbgp->send_cmd("bgp router 1.0.0.2");
    $cbgp->send_cmd("\tadd peer 1 1.0.0.1");
    $cbgp->send_cmd("\tpeer 1.0.0.1 up");
    $cbgp->send_cmd("\tadd peer 1 1.0.0.3");
    $cbgp->send_cmd("\tpeer 1.0.0.3 rr-client");
    $cbgp->send_cmd("\tpeer 1.0.0.3 up");
    $cbgp->send_cmd("\tadd peer 1 1.0.0.5");
    $cbgp->send_cmd("\tpeer 1.0.0.5 up");
    $cbgp->send_cmd("\tadd peer 3 3.0.0.1");
    $cbgp->send_cmd("\tpeer 3.0.0.1 up");
    $cbgp->send_cmd("\texit");

    $cbgp->send_cmd("bgp add router 1 1.0.0.3");
    $cbgp->send_cmd("bgp router 1.0.0.3");
    $cbgp->send_cmd("\tadd peer 1 1.0.0.2");
    $cbgp->send_cmd("\tpeer 1.0.0.2 up");
    $cbgp->send_cmd("\tadd peer 1 1.0.0.4");
    $cbgp->send_cmd("\tpeer 1.0.0.4 up");
    $cbgp->send_cmd("\tadd peer 4 4.0.0.1");
    $cbgp->send_cmd("\tpeer 4.0.0.1 up");
    $cbgp->send_cmd("\texit");

    $cbgp->send_cmd("bgp add router 1 1.0.0.4");
    $cbgp->send_cmd("bgp router 1.0.0.4");
    $cbgp->send_cmd("\tadd peer 1 1.0.0.3");
    $cbgp->send_cmd("\tpeer 1.0.0.3 up");
    $cbgp->send_cmd("\texit");

    $cbgp->send_cmd("bgp add router 1 1.0.0.5");
    $cbgp->send_cmd("bgp router 1.0.0.5");
    $cbgp->send_cmd("\tadd peer 1 1.0.0.2");
    $cbgp->send_cmd("\tpeer 1.0.0.2 up");
    $cbgp->send_cmd("\texit");

    $cbgp->send_cmd("bgp add router 3 3.0.0.1");
    $cbgp->send_cmd("bgp router 3.0.0.1");
    $cbgp->send_cmd("\tadd peer 1 1.0.0.2");
    $cbgp->send_cmd("\tpeer 1.0.0.2 up");
    $cbgp->send_cmd("\texit");

    $cbgp->send_cmd("bgp add router 4 4.0.0.1");
    $cbgp->send_cmd("bgp router 4.0.0.1");
    $cbgp->send_cmd("\tadd peer 1 1.0.0.3");
    $cbgp->send_cmd("\tpeer 1.0.0.3 up");
    $cbgp->send_cmd("\texit");

    cbgp_recv_update($cbgp, "1.0.0.1", 1, "2.0.0.1",
		     "255/8|2|IGP|2.0.0.1|0|0");

    $cbgp->send_cmd("sim run");

    my $rib;
    $rib= cbgp_get_rib_mrt($cbgp, "1.0.0.1");
    return TEST_FAILURE
      if (!check_has_bgp_route($rib, "255/8"));
    $rib= cbgp_get_rib_mrt($cbgp, "1.0.0.2");
    return TEST_FAILURE
      if (!check_has_bgp_route($rib, "255/8"));
    $rib= cbgp_get_rib_custom($cbgp, "1.0.0.3");
    return TEST_FAILURE
      if (!check_has_bgp_route($rib, "255/8",
			       -originator=>"1.0.0.1",
			       -clusterlist=>["1.0.0.2"]));
    $rib= cbgp_get_rib_mrt($cbgp, "1.0.0.4");
    return TEST_FAILURE
      if (check_has_bgp_route($rib, "255/8"));
    $rib= cbgp_get_rib_mrt($cbgp, "1.0.0.5");
    return TEST_FAILURE
      if (check_has_bgp_route($rib, "255/8"));
    $rib= cbgp_get_rib_custom($cbgp, "3.0.0.1");
    return TEST_FAILURE
      if (!check_has_bgp_route($rib, "255/8",
			       -clusterlist=>undef,
			       -originator=>undef));
    $rib= cbgp_get_rib_custom($cbgp, "4.0.0.1");
    return TEST_FAILURE
      if (!check_has_bgp_route($rib, "255/8",
			       -clusterlist=>undef,
			       -originator=>undef));
    return TEST_SUCCESS;
  }

# -----[ cbgp_valid_bgp_rr_set_cluster_id ]--------------------------
# Check that the cluster-ID can be defined correctly (including
# large cluster-ids).
# -------------------------------------------------------------------
sub cbgp_valid_bgp_rr_set_cluster_id($)
  {
    my ($cbgp)= @_;

    $cbgp->send_cmd("net add node 0.1.0.0");
    $cbgp->send_cmd("bgp add router 1 0.1.0.0");
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

    $cbgp->send_cmd("net add domain 1 igp");
    $cbgp->send_cmd("net add node 1.0.0.1");
    $cbgp->send_cmd("net add node 1.0.0.2");
    $cbgp->send_cmd("net add node 1.0.0.3");
    $cbgp->send_cmd("net add node 1.0.0.4");
    $cbgp->send_cmd("net add node 1.0.0.5");
    $cbgp->send_cmd("net node 1.0.0.1 domain 1");
    $cbgp->send_cmd("net node 1.0.0.2 domain 1");
    $cbgp->send_cmd("net node 1.0.0.3 domain 1");
    $cbgp->send_cmd("net node 1.0.0.4 domain 1");
    $cbgp->send_cmd("net node 1.0.0.5 domain 1");
    $cbgp->send_cmd("net add link 1.0.0.1 1.0.0.2 0");
    $cbgp->send_cmd("net link 1.0.0.1 1.0.0.2 igp-weight --bidir 1");
    $cbgp->send_cmd("net add link 1.0.0.1 1.0.0.3 0");
    $cbgp->send_cmd("net link 1.0.0.1 1.0.0.3 igp-weight --bidir 1");
    $cbgp->send_cmd("net add link 1.0.0.2 1.0.0.4 0");
    $cbgp->send_cmd("net link 1.0.0.2 1.0.0.4 igp-weight --bidir 1");
    $cbgp->send_cmd("net add link 1.0.0.3 1.0.0.4 0");
    $cbgp->send_cmd("net link 1.0.0.3 1.0.0.4 igp-weight --bidir 1");
    $cbgp->send_cmd("net add link 1.0.0.4 1.0.0.5 1");
    $cbgp->send_cmd("net link 1.0.0.4 1.0.0.5 igp-weight --bidir 1");
    $cbgp->send_cmd("net domain 1 compute");

    $cbgp->send_cmd("bgp add router 1 1.0.0.2");
    $cbgp->send_cmd("bgp router 1.0.0.2");
    $cbgp->send_cmd("\tset cluster-id 1");
    $cbgp->send_cmd("\tadd peer 1 1.0.0.1");
    $cbgp->send_cmd("\tpeer 1.0.0.1 rr-client");
    $cbgp->send_cmd("\tpeer 1.0.0.1 virtual");
    $cbgp->send_cmd("\tpeer 1.0.0.1 up");
    $cbgp->send_cmd("\tadd peer 1 1.0.0.4");
    $cbgp->send_cmd("\tpeer 1.0.0.4 up");
    $cbgp->send_cmd("\texit");

    $cbgp->send_cmd("bgp add router 1 1.0.0.3");
    $cbgp->send_cmd("bgp router 1.0.0.3");
    $cbgp->send_cmd("\tset cluster-id 1");
    $cbgp->send_cmd("\tadd peer 1 1.0.0.1");
    $cbgp->send_cmd("\tpeer 1.0.0.1 rr-client");
    $cbgp->send_cmd("\tpeer 1.0.0.1 virtual");
    $cbgp->send_cmd("\tpeer 1.0.0.1 up");
    $cbgp->send_cmd("\tadd peer 1 1.0.0.4");
    $cbgp->send_cmd("\tpeer 1.0.0.4 up");
    $cbgp->send_cmd("\texit");

    $cbgp->send_cmd("bgp add router 1 1.0.0.4");
    $cbgp->send_cmd("bgp router 1.0.0.4");
    $cbgp->send_cmd("\tadd peer 1 1.0.0.2");
    $cbgp->send_cmd("\tpeer 1.0.0.2 up");
    $cbgp->send_cmd("\tadd peer 1 1.0.0.3");
    $cbgp->send_cmd("\tpeer 1.0.0.3 up");
    $cbgp->send_cmd("\tadd peer 1 1.0.0.5");
    $cbgp->send_cmd("\tpeer 1.0.0.5 rr-client");
    $cbgp->send_cmd("\tpeer 1.0.0.5 virtual");
    $cbgp->send_cmd("\tpeer 1.0.0.5 record $mrt_record_file");
    $cbgp->send_cmd("\tpeer 1.0.0.5 up");
    $cbgp->send_cmd("\texit");

    cbgp_recv_update($cbgp, "1.0.0.3", 1, "1.0.0.1",
		     "255/8||IGP|1.0.0.1|0|0");
    $cbgp->send_cmd("sim run");
    cbgp_recv_update($cbgp, "1.0.0.2", 1, "1.0.0.1",
		     "255/8||IGP|1.0.0.1|0|0");
    $cbgp->send_cmd("sim run");
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

    $cbgp->send_cmd("net add domain 1 igp");
    $cbgp->send_cmd("net add node 1.0.0.1");
    $cbgp->send_cmd("net add node 1.0.0.2");
    $cbgp->send_cmd("net add node 1.0.0.3");
    $cbgp->send_cmd("net node 1.0.0.1 domain 1");
    $cbgp->send_cmd("net node 1.0.0.2 domain 1");
    $cbgp->send_cmd("net node 1.0.0.3 domain 1");
    $cbgp->send_cmd("net add link 1.0.0.1 1.0.0.2 0");
    $cbgp->send_cmd("net link 1.0.0.1 1.0.0.2 igp-weight --bidir 1");
    $cbgp->send_cmd("net add link 1.0.0.1 1.0.0.3 0");
    $cbgp->send_cmd("net link 1.0.0.1 1.0.0.3 igp-weight --bidir 1");
    $cbgp->send_cmd("net add link 1.0.0.2 1.0.0.3 0");
    $cbgp->send_cmd("net link 1.0.0.2 1.0.0.3 igp-weight --bidir 1");
    $cbgp->send_cmd("net domain 1 compute");

    $cbgp->send_cmd("bgp add router 1 1.0.0.2");
    $cbgp->send_cmd("bgp router 1.0.0.2");
    $cbgp->send_cmd("\tadd peer 1 1.0.0.1");
    $cbgp->send_cmd("\tpeer 1.0.0.1 rr-client");
    $cbgp->send_cmd("\tpeer 1.0.0.1 virtual");
    $cbgp->send_cmd("\tpeer 1.0.0.1 up");
    $cbgp->send_cmd("\tadd peer 1 1.0.0.3");
    $cbgp->send_cmd("\tpeer 1.0.0.3 up");
    $cbgp->send_cmd("\texit");

    $cbgp->send_cmd("bgp add router 1 1.0.0.3");
    $cbgp->send_cmd("bgp router 1.0.0.3");
    $cbgp->send_cmd("\tadd peer 1 1.0.0.1");
    $cbgp->send_cmd("\tpeer 1.0.0.1 rr-client");
    $cbgp->send_cmd("\tpeer 1.0.0.1 virtual");
    $cbgp->send_cmd("\tpeer 1.0.0.1 record $mrt_record_file");
    $cbgp->send_cmd("\tpeer 1.0.0.1 up");
    $cbgp->send_cmd("\tadd peer 1 1.0.0.2");
    $cbgp->send_cmd("\tpeer 1.0.0.2 up");
    $cbgp->send_cmd("\texit");

    cbgp_recv_update($cbgp, "1.0.0.2", 1, "1.0.0.1",
		     "255/8||IGP|1.0.0.1|0|0");
    $cbgp->send_cmd("sim run");

    my $rib= cbgp_get_rib_mrt($cbgp, "1.0.0.3");
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
    $cbgp->send_cmd("bgp add router 1 1.0.0.1");
    $cbgp->send_cmd("bgp router 1.0.0.1");
    $cbgp->send_cmd("\tadd network 255/8");
    $cbgp->send_cmd("\tadd peer 1 1.0.0.2");
    $cbgp->send_cmd("\tpeer 1.0.0.2 up");
    $cbgp->send_cmd("\texit");
    $cbgp->send_cmd("bgp add router 1 1.0.0.2");
    $cbgp->send_cmd("bgp router 1.0.0.2");
    $cbgp->send_cmd("\tadd peer 1 1.0.0.1");
    $cbgp->send_cmd("\tpeer 1.0.0.1 up");
    $cbgp->send_cmd("\tadd peer 1 1.0.0.3");
    $cbgp->send_cmd("\tpeer 1.0.0.3 rr-client");
    $cbgp->send_cmd("\tpeer 1.0.0.3 up");
    $cbgp->send_cmd("\tadd peer 1 1.0.0.4");
    $cbgp->send_cmd("\tpeer 1.0.0.4 up");
    $cbgp->send_cmd("\tadd peer 2 1.0.0.5");
    $cbgp->send_cmd("\tpeer 1.0.0.5 up");
    $cbgp->send_cmd("\tadd peer 1 1.0.0.6");
    $cbgp->send_cmd("\tpeer 1.0.0.6 rr-client");
    $cbgp->send_cmd("\tpeer 1.0.0.6 up");
    $cbgp->send_cmd("\texit");
    $cbgp->send_cmd("bgp add router 1 1.0.0.3");
    $cbgp->send_cmd("bgp router 1.0.0.3");
    $cbgp->send_cmd("\tadd peer 1 1.0.0.2");
    $cbgp->send_cmd("\tpeer 1.0.0.2 up");
    $cbgp->send_cmd("\texit");
    $cbgp->send_cmd("bgp add router 1 1.0.0.4");
    $cbgp->send_cmd("bgp router 1.0.0.4");
    $cbgp->send_cmd("\tadd peer 1 1.0.0.2");
    $cbgp->send_cmd("\tpeer 1.0.0.2 up");
    $cbgp->send_cmd("\texit");
    $cbgp->send_cmd("bgp add router 2 1.0.0.5");
    $cbgp->send_cmd("bgp router 1.0.0.5");
    $cbgp->send_cmd("\tadd network 254/8");
    $cbgp->send_cmd("\tadd peer 1 1.0.0.2");
    $cbgp->send_cmd("\tpeer 1.0.0.2 up");
    $cbgp->send_cmd("\texit");
    $cbgp->send_cmd("bgp add router 1 1.0.0.6");
    $cbgp->send_cmd("bgp router 1.0.0.6");
    $cbgp->send_cmd("\tadd network 253/8");
    $cbgp->send_cmd("\tadd peer 1 1.0.0.2");
    $cbgp->send_cmd("\tpeer 1.0.0.2 up");
    $cbgp->send_cmd("\texit");
    $cbgp->send_cmd("sim run");

    # Peer 1.0.0.1 must have received all routes
    my $rib= cbgp_get_rib_mrt($cbgp, "1.0.0.1");
    (cbgp_rib_check($rib, "255/8") &&
     cbgp_rib_check($rib, "254/8") &&
     cbgp_rib_check($rib, "253/8")) or return TEST_FAILURE;
    # RR must have received all routes
    $rib= cbgp_show_rib_mrt($cbgp, "1.0.0.2");
    (cbgp_rib_check($rib, "255/8") &&
     cbgp_rib_check($rib, "254/8") &&
     cbgp_rib_check($rib, "253/8")) or return TEST_FAILURE;
    # RR-client 1.0.0.3 must have received all routes
    $rib= cbgp_get_rib_mrt($cbgp, "1.0.0.3");
    (cbgp_rib_check($rib, "255/8") &&
     cbgp_rib_check($rib, "254/8") &&
     cbgp_rib_check($rib, "253/8")) or return TEST_FAILURE;
    # Peer 1.0.0.4 must only have received the 254/8 and 253/8 routes
    $rib= cbgp_get_rib_mrt($cbgp, "1.0.0.4");
    (!cbgp_rib_check($rib, "255/8") &&
     cbgp_rib_check($rib, "254/8") &&
     cbgp_rib_check($rib, "253/8")) or return TEST_FAILURE;
    # External peer 1.0.0.5 must have received all routes
    $rib= cbgp_get_rib_mrt($cbgp, "1.0.0.5");
    (cbgp_rib_check($rib, "255/8") &&
     cbgp_rib_check($rib, "254/8") &&
     cbgp_rib_check($rib, "253/8")) or return TEST_FAILURE;
    # RR-client 1.0.0.6 must have received all routes
    $rib= cbgp_get_rib_mrt($cbgp, "1.0.0.6");
    (cbgp_rib_check($rib, "255/8") &&
     cbgp_rib_check($rib, "254/8") &&
     cbgp_rib_check($rib, "253/8")) or return TEST_FAILURE;
    return TEST_SUCCESS;
  }

# -----[ cbgp_valid_net_traffic_load ]-------------------------------
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
sub cbgp_valid_net_traffic_load($)
  {
    my ($cbgp)= @_;
    my $msg;
    my $info;

    $cbgp->send_cmd("net add domain 1 igp");
    $cbgp->send_cmd("net add subnet 172.13.16/24 stub");
    $cbgp->send_cmd("net add subnet 138.48/16 stub");
    $cbgp->send_cmd("net add subnet 130.104/16 stub");
    $cbgp->send_cmd("net add node 1.0.0.1");
    $cbgp->send_cmd("net node 1.0.0.1 domain 1");
    $cbgp->send_cmd("net add node 1.0.0.2");
    $cbgp->send_cmd("net node 1.0.0.2 domain 1");
    $cbgp->send_cmd("net add node 1.0.0.3");
    $cbgp->send_cmd("net node 1.0.0.3 domain 1");
    $cbgp->send_cmd("net add node 1.0.0.4");
    $cbgp->send_cmd("net node 1.0.0.4 domain 1");
    $cbgp->send_cmd("net add link 1.0.0.1 1.0.0.2 10");
    $cbgp->send_cmd("net link 1.0.0.1 1.0.0.2 igp-weight --bidir 10");
    $cbgp->send_cmd("net add link 1.0.0.2 1.0.0.3 10");
    $cbgp->send_cmd("net link 1.0.0.2 1.0.0.3 igp-weight --bidir 10");
    $cbgp->send_cmd("net add link 1.0.0.2 1.0.0.4 10");
    $cbgp->send_cmd("net link 1.0.0.2 1.0.0.4 igp-weight --bidir 10");
    $cbgp->send_cmd("net add link 1.0.0.1 130.104.10.10/16 10");
    $cbgp->send_cmd("net link 1.0.0.1 130.104.10.10/16 igp-weight 10");
    $cbgp->send_cmd("net add link 1.0.0.3 138.48.4.4/16 10");
    $cbgp->send_cmd("net link 1.0.0.3 138.48.4.4/16 igp-weight 10");
    $cbgp->send_cmd("net add link 1.0.0.4 172.13.16.1/24 10");
    $cbgp->send_cmd("net link 1.0.0.4 172.13.16.1/24 igp-weight 10");
    $cbgp->send_cmd("net domain 1 compute");

    $msg= cbgp_check_error($cbgp, "net traffic load \"".$resources_path."valid-net-tm.tm\"");
    if (defined($msg)) {
      $tests->debug("Error: \"net traffic load\" generated an error.");
      return TEST_FAILURE;
    }

    $info= cbgp_link_info($cbgp, "1.0.0.1", "1.0.0.2");
    if (!exists($info->{load}) || ($info->{load} != 1500)) {
      $tests->debug("Error: no/invalid load for R1->R2");
      return TEST_FAILURE;
    }

    $info= cbgp_link_info($cbgp, "1.0.0.2", "1.0.0.3");
    if (!exists($info->{load}) || ($info->{load} != 1800)) {
      $tests->debug("Error: no/invalid load for R2->R3");
      return TEST_FAILURE;
    }

    $info= cbgp_link_info($cbgp, "1.0.0.3", "138.48.4.4/16");
    if (!exists($info->{load}) || ($info->{load} != 1600)) {
      $tests->debug("Error: no/invalid load for R3->N2");
      return TEST_FAILURE;
    }

    $info= cbgp_link_info($cbgp, "1.0.0.2", "1.0.0.4");
    if (!exists($info->{load}) || ($info->{load} != 1200)) {
      $tests->debug("Error: no/invalid load for R2->R4");
      return TEST_FAILURE;
    }

    $info= cbgp_link_info($cbgp, "1.0.0.4", "172.13.16.1/24");
    if (!exists($info->{load}) || ($info->{load} != 800)) {
      $tests->debug("Error: no/invalid load for R4->N1");
      return TEST_FAILURE;
    }

    $info= cbgp_link_info($cbgp, "1.0.0.1", "130.104.10.10/16");
    if (!exists($info->{load}) || ($info->{load} != 3200)) {
      $tests->debug("Error: no/invalid load for R1->N3");
      return TEST_FAILURE;
    }

    $info= cbgp_link_info($cbgp, "1.0.0.2", "1.0.0.1");
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

    my %nodes= (
		"198.32.12.9" => "ATLA",
		"198.32.12.25" => "CHIN",
		"198.32.12.41" => "DNVR",
		"198.32.12.57" => "HSTN",
		"198.32.12.177" => "IPLS",
		"198.32.12.89" => "KSCY",
		"198.32.12.105" => "LOSA",
		"198.32.12.121" => "NYCM",
		"198.32.12.137" => "SNVA",
		"198.32.12.153" => "STTL",
		"198.32.12.169" => "WASH",
	       );

      $cbgp->send_cmd("net add domain 11537 igp");
    foreach my $r (keys %nodes) {
      my $name= $nodes{$r};
      $cbgp->send_cmd("net add node $r");
      $cbgp->send_cmd("net node $r domain 11537");
      $cbgp->send_cmd("net node $r name \"$name\"");
    }

    # Internal links
    $cbgp->send_cmd("net add link 198.32.12.137 198.32.12.41 1295");
    $cbgp->send_cmd("net add link 198.32.12.137 198.32.12.105 366");
    $cbgp->send_cmd("net add link 198.32.12.137 198.32.12.153 861");
    $cbgp->send_cmd("net add link 198.32.12.41 198.32.12.153 2095");
    $cbgp->send_cmd("net add link 198.32.12.153 198.32.12.89 639");
    $cbgp->send_cmd("net add link 198.32.12.105 198.32.12.57 1893");
    $cbgp->send_cmd("net add link 198.32.12.89 198.32.12.57 902");
    $cbgp->send_cmd("net add link 198.32.12.89 198.32.12.177 548");
    $cbgp->send_cmd("net add link 198.32.12.57 198.32.12.9 1176");
    $cbgp->send_cmd("net add link 198.32.12.9 198.32.12.177 587");
    $cbgp->send_cmd("net add link 198.32.12.9 198.32.12.169 846");
    $cbgp->send_cmd("net add link 198.32.12.25 198.32.12.177 260");
    $cbgp->send_cmd("net add link 198.32.12.25 198.32.12.121 700");
    $cbgp->send_cmd("net add link 198.32.12.169 198.32.12.121 233");

    # Compute IGP routes
    $cbgp->send_cmd("net link 198.32.12.137 198.32.12.41 igp-weight --bidir 1295");
    $cbgp->send_cmd("net link 198.32.12.137 198.32.12.105 igp-weight --bidir 366");
    $cbgp->send_cmd("net link 198.32.12.137 198.32.12.153 igp-weight --bidir 861");
    $cbgp->send_cmd("net link 198.32.12.41 198.32.12.153 igp-weight --bidir 2095");
    $cbgp->send_cmd("net link 198.32.12.153 198.32.12.89 igp-weight --bidir 639");
    $cbgp->send_cmd("net link 198.32.12.105 198.32.12.57 igp-weight --bidir 1893");
    $cbgp->send_cmd("net link 198.32.12.89 198.32.12.57 igp-weight --bidir 902");
    $cbgp->send_cmd("net link 198.32.12.89 198.32.12.177 igp-weight --bidir 548");
    $cbgp->send_cmd("net link 198.32.12.57 198.32.12.9 igp-weight --bidir 1176");
    $cbgp->send_cmd("net link 198.32.12.9 198.32.12.177 igp-weight --bidir 587");
    $cbgp->send_cmd("net link 198.32.12.9 198.32.12.169 igp-weight --bidir 846");
    $cbgp->send_cmd("net link 198.32.12.25 198.32.12.177 igp-weight --bidir 260");
    $cbgp->send_cmd("net link 198.32.12.25 198.32.12.121 igp-weight --bidir 700");
    $cbgp->send_cmd("net link 198.32.12.169 198.32.12.121 igp-weight --bidir 233");
    $cbgp->send_cmd("net domain 11537 compute");
    
    $cbgp->send_cmd("bgp add router 11537 198.32.12.9");
    $cbgp->send_cmd("bgp add router 11537 198.32.12.25");
    $cbgp->send_cmd("bgp add router 11537 198.32.12.41");
    $cbgp->send_cmd("bgp add router 11537 198.32.12.57");
    $cbgp->send_cmd("bgp add router 11537 198.32.12.177");
    $cbgp->send_cmd("bgp add router 11537 198.32.12.89");
    $cbgp->send_cmd("bgp add router 11537 198.32.12.105");
    $cbgp->send_cmd("bgp add router 11537 198.32.12.121");
    $cbgp->send_cmd("bgp add router 11537 198.32.12.137");
    $cbgp->send_cmd("bgp add router 11537 198.32.12.153");
    $cbgp->send_cmd("bgp add router 11537 198.32.12.169");

    $cbgp->send_cmd("bgp domain 11537 full-mesh");

    $cbgp->send_cmd("sim run");

    my %ribs= (
	       "198.32.12.9" => "ATLA/rib.20050801.0124-ebgp.ascii",
	       "198.32.12.25" => "CHIN/rib.20050801.0017-ebgp.ascii",
	       "198.32.12.41" => "DNVR/rib.20050801.0014-ebgp.ascii",
	       "198.32.12.57" => "HSTN/rib.20050801.0149-ebgp.ascii",
	       "198.32.12.177" => "IPLS/rib.20050801.0157-ebgp.ascii",
	       "198.32.12.89" => "KSCY/rib.20050801.0154-ebgp.ascii",
	       "198.32.12.105" => "LOSA/rib.20050801.0034-ebgp.ascii",
	       "198.32.12.121" => "NYCM/rib.20050801.0050-ebgp.ascii",
	       "198.32.12.137" => "SNVA/rib.20050801.0118-ebgp.ascii",
	       "198.32.12.153" => "STTL/rib.20050801.0124-ebgp.ascii",
	       "198.32.12.169" => "WASH/rib.20050801.0101-ebgp.ascii",
	      );

    foreach my $r (keys %ribs) {
      my $rib= $resources_path."abilene/".$ribs{$r};
      $cbgp->send_cmd("bgp router $r load rib --autoconf \"$rib\"");
    }
    $cbgp->send_cmd("sim run");

    foreach my $r (keys %ribs) {
      cbgp_get_rib($cbgp, $r);
    }

    return TEST_SUCCESS;
  }

# -----[ cbgp_valid_enst ]-------------------------------------------
#
# -------------------------------------------------------------------
sub cbgp_valid_enst($)
  {
    my ($cbgp)= @_;

    $cbgp->send_cmd("net add node 137.194.1.1");
    $cbgp->send_cmd("net add node 212.27.32.1");
    $cbgp->send_cmd("net add node 134.157.1.1");
    $cbgp->send_cmd("net add node 4.1.1.1");
    $cbgp->send_cmd("net add node 62.229.1.1");

    $cbgp->send_cmd("net add link 137.194.1.1 212.27.32.1 0");
    $cbgp->send_cmd("net add link 137.194.1.1 134.157.1.1 0");
    $cbgp->send_cmd("net add link 212.27.32.1 4.1.1.1 0");
    $cbgp->send_cmd("net add link 62.229.1.1 134.157.1.1 0");
    $cbgp->send_cmd("net add link 62.229.1.1 4.1.1.1 0");

    $cbgp->send_cmd("net node 137.194.1.1 route add 134.157.0.0/14 134.157.1.1 134.157.1.1 5");
    $cbgp->send_cmd("net node 137.194.1.1 route add 212.27.32.0/19 212.27.32.1 212.27.32.1 5");
    $cbgp->send_cmd("net node 212.27.32.1 route add 4.0.0.0/8 4.1.1.1 4.1.1.1 5");
    $cbgp->send_cmd("net node 212.27.32.1 route add 137.194.0.0/16 137.194.1.1 137.194.1.1 5");
    $cbgp->send_cmd("net node 4.1.1.1 route add 212.27.32.0/19 212.27.32.1 212.27.32.1 5");
    $cbgp->send_cmd("net node 134.157.1.1 route add 137.194.0.0/16 137.194.1.1 137.194.1.1 5");
    $cbgp->send_cmd("net node 62.229.1.1 route add 134.157.0.0/14 134.157.1.1 134.157.1.1 5");
    $cbgp->send_cmd("net node 62.229.1.1 route add 4.0.0.0/8 4.1.1.1 4.1.1.1 5");
    $cbgp->send_cmd("net node 134.157.1.1 route add 62.229.0.0/16 62.229.1.1 62.229.1.1 5");
    $cbgp->send_cmd("net node 4.1.1.1 route add 62.229.0.0/16 62.229.1.1 62.229.1.1 5");

    $cbgp->send_cmd("bgp add router 1712 137.194.1.1");
    $cbgp->send_cmd("bgp add router 12322 212.27.32.1");
    $cbgp->send_cmd("bgp add router 2200 134.157.1.1");
    $cbgp->send_cmd("bgp add router 3356 4.1.1.1");
    $cbgp->send_cmd("bgp add router 5511 62.229.1.1");

    $cbgp->send_cmd("bgp router 62.229.1.1");
    $cbgp->send_cmd("\tadd network 62.229.0.0/16");
    $cbgp->send_cmd("\tadd peer 3356 4.1.1.1");
    $cbgp->send_cmd("\tadd peer 2200 134.157.1.1");
    $cbgp->send_cmd("\tpeer 4.1.1.1 up");
    $cbgp->send_cmd("\tpeer 134.157.1.1 up");
    $cbgp->send_cmd("\texit");
    $cbgp->send_cmd("bgp router 137.194.1.1");
    $cbgp->send_cmd("\tadd network 137.194.0.0/16");
    $cbgp->send_cmd("\tadd peer 12322 212.27.32.1");
    $cbgp->send_cmd("\tadd peer 2200 134.157.1.1");
    $cbgp->send_cmd("\tpeer 212.27.32.1 up");
    $cbgp->send_cmd("\tpeer 134.157.1.1 up");
    $cbgp->send_cmd("\tpeer 212.27.32.1");
    $cbgp->send_cmd("\t\tfilter in");
    $cbgp->send_cmd("\t\t\tadd-rule");
    $cbgp->send_cmd("\t\t\t\tmatch any");
    $cbgp->send_cmd("\t\t\t\taction \"community add 1\"");
    $cbgp->send_cmd("\t\t\t\texit");
    $cbgp->send_cmd("\t\t\texit");
    $cbgp->send_cmd("\t\tfilter out");
    $cbgp->send_cmd("\t\t\tadd-rule");
    $cbgp->send_cmd("\t\t\t\tmatch \"community is 1\"");
    $cbgp->send_cmd("\t\t\t\taction deny");
    $cbgp->send_cmd("\t\t\t\texit");
    $cbgp->send_cmd("\t\t\texit");
    $cbgp->send_cmd("\t\texit");
    $cbgp->send_cmd("\tpeer 134.157.1.1");
    $cbgp->send_cmd("\t\tfilter in");
    $cbgp->send_cmd("\t\t\tadd-rule");
    $cbgp->send_cmd("\t\t\t\tmatch any");
    $cbgp->send_cmd("\t\t\t\taction \"community add 1\"");
    $cbgp->send_cmd("\t\t\t\texit");
    $cbgp->send_cmd("\t\t\texit");
    $cbgp->send_cmd("\t\tfilter out");
    $cbgp->send_cmd("\t\t\tadd-rule");
    $cbgp->send_cmd("\t\t\t\tmatch \"community is 1\"");
    $cbgp->send_cmd("\t\t\t\taction deny");
    $cbgp->send_cmd("\t\t\t\texit");
    $cbgp->send_cmd("\t\t\texit");
    $cbgp->send_cmd("\t\texit");
    $cbgp->send_cmd("\texit");

    $cbgp->send_cmd("bgp router 212.27.32.1");
    $cbgp->send_cmd("\tadd network 212.27.32.0/19");
    $cbgp->send_cmd("\tadd network 88.160.0.0/11");
    $cbgp->send_cmd("\tadd peer 1712 137.194.1.1");
    $cbgp->send_cmd("\tadd peer 3356 4.1.1.1");
    $cbgp->send_cmd("\tpeer 137.194.1.1 up");
    $cbgp->send_cmd("\tpeer 4.1.1.1 up");
    $cbgp->send_cmd("\texit");

    $cbgp->send_cmd("bgp router 134.157.1.1");
    $cbgp->send_cmd("\tadd network 134.157.0.0/16");
    $cbgp->send_cmd("\tadd peer 5511 62.229.1.1");
    $cbgp->send_cmd("\tadd peer 1712 137.194.1.1");
    $cbgp->send_cmd("\tpeer 62.229.1.1 up");
    $cbgp->send_cmd("\tpeer 137.194.1.1 up");
    $cbgp->send_cmd("\texit");

    $cbgp->send_cmd("bgp router 4.1.1.1");
    $cbgp->send_cmd("\tadd network 4.0.0.0/8");
    $cbgp->send_cmd("\tadd network 212.73.192.0/18");
    $cbgp->send_cmd("\tadd peer 12322 212.27.32.1");
    $cbgp->send_cmd("\tadd peer 5511 62.229.1.1");
    $cbgp->send_cmd("\tpeer 212.27.32.1 up");
    $cbgp->send_cmd("\tpeer 62.229.1.1 up");
    $cbgp->send_cmd("\texit");

    $cbgp->send_cmd("sim run");
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

    $cbgp->send_cmd("net add node 0.1.0.0");
    $cbgp->send_cmd("net add node 0.2.0.0");
    $cbgp->send_cmd("net add link 0.1.0.0 0.2.0.0 10");
    $cbgp->send_cmd("net node 0.1.0.0 route add 0.2/16 * 0.2.0.0 10");
    $cbgp->send_cmd("net node 0.2.0.0 route add 0.1/16 * 0.1.0.0 10");

    $cbgp->send_cmd("bgp add router 1 0.1.0.0");
    $cbgp->send_cmd("bgp router 0.1.0.0");
    $cbgp->send_cmd("\tadd peer 2 0.2.0.0");
    $cbgp->send_cmd("\tpeer 0.2.0.0 up");
    $cbgp->send_cmd("\texit");

    $cbgp->send_cmd("bgp add router 2 0.2.0.0");
    $cbgp->send_cmd("bgp router 0.2.0.0");
    $cbgp->send_cmd("\tadd peer 1 0.1.0.0");
    $cbgp->send_cmd("\tpeer 0.1.0.0 up");
    $cbgp->send_cmd("\texit");

    $cbgp->send_cmd("sim run");

    my $peers= cbgp_get_peers($cbgp, "0.1.0.0");
    return TEST_FAILURE
      if (!check_has_peer($peers, "0.2.0.0",
			  -state=>C_PEER_ESTABLISHED));
    $peers= cbgp_get_peers($cbgp, "0.2.0.0");
    return TEST_FAILURE
      if (!check_has_peer($peers, "0.1.0.0",
			  -state=>C_PEER_ESTABLISHED));

    $cbgp->send_cmd("net node 0.1.0.0 route del 0.2/16 * *");
    $cbgp->send_cmd("bgp router 0.1.0.0 rescan");
    $cbgp->send_cmd("bgp router 0.2.0.0 rescan");

    $peers= cbgp_get_peers($cbgp, "0.1.0.0");
    return TEST_FAILURE
      if (!check_has_peer($peers, "0.2.0.0",
			  -state=>C_PEER_ACTIVE));
    $peers= cbgp_get_peers($cbgp, "0.2.0.0");
    return TEST_FAILURE
      if (!check_has_peer($peers, "0.1.0.0",
			  -state=>C_PEER_ACTIVE));
    return TEST_SUCCESS;
  }

# -----[ cbgp_valid_nasty_bgp_session_segment_ordering ]-------------
#
# -------------------------------------------------------------------
sub cbgp_valid_nasty_bgp_session_segment_ordering($)
  {
    my ($cbgp)= @_;

    $cbgp->send_cmd("net add domain 1 igp");
    $cbgp->send_cmd("net add node 1.0.0.1");
    $cbgp->send_cmd("net node 1.0.0.1 domain 1");
    $cbgp->send_cmd("net add node 1.0.0.2");
    $cbgp->send_cmd("net node 1.0.0.2 domain 1");
    $cbgp->send_cmd("net add node 1.0.0.3");
    $cbgp->send_cmd("net node 1.0.0.3 domain 1");
    $cbgp->send_cmd("net add link 1.0.0.1 1.0.0.2 2");
    $cbgp->send_cmd("net link 1.0.0.1 1.0.0.2 igp-weight --bidir 2");
    $cbgp->send_cmd("net add link 1.0.0.1 1.0.0.3 10");
    $cbgp->send_cmd("net link 1.0.0.1 1.0.0.3 igp-weight --bidir 10");
    $cbgp->send_cmd("net add link 1.0.0.2 1.0.0.3 10");
    $cbgp->send_cmd("net link 1.0.0.2 1.0.0.3 igp-weight --bidir 10");
    $cbgp->send_cmd("net domain 1 compute");

    $cbgp->send_cmd("bgp add router 1 1.0.0.1");
    $cbgp->send_cmd("bgp router 1.0.0.1");
    $cbgp->send_cmd("\tadd peer 1 1.0.0.2");
    $cbgp->send_cmd("\tpeer 1.0.0.2 up");
    $cbgp->send_cmd("\texit");

    $cbgp->send_cmd("bgp add router 1 1.0.0.2");
    $cbgp->send_cmd("bgp router 1.0.0.2");
    $cbgp->send_cmd("\tadd peer 1 1.0.0.1");
    $cbgp->send_cmd("\tpeer 1.0.0.1 up");
    $cbgp->send_cmd("\texit");

    $cbgp->send_cmd("bgp router 1.0.0.1 add network 254/8");
    $cbgp->send_cmd("sim run");

    $cbgp->send_cmd("net node 1.0.0.1 route add 1.0.0.2/32 * 1.0.0.3 1");

    $cbgp->send_cmd("bgp router 1.0.0.1 del network 254/8");
    $cbgp->send_cmd("net node 1.0.0.1 route del 1.0.0.2/32 * 1.0.0.3");
    $cbgp->send_cmd("bgp router 1.0.0.1 add network 254/8");
    $cbgp->send_cmd("sim run");

    return TEST_FAILURE;
  }

# -----[ cbgp_valid_nasty_bgp_med_ordering ]-------------------------
#
# -------------------------------------------------------------------
sub cbgp_valid_nasty_bgp_med_ordering($)
  {
    my ($cbgp)= @_;

    $cbgp->send_cmd("net add domain 1 igp");
    $cbgp->send_cmd("net add node 1.0.0.1");
    $cbgp->send_cmd("net node 1.0.0.1 domain 1");
    $cbgp->send_cmd("net add node 1.0.0.2");
    $cbgp->send_cmd("net node 1.0.0.2 domain 1");
    $cbgp->send_cmd("net add node 1.0.0.3");
    $cbgp->send_cmd("net node 1.0.0.3 domain 1");
    $cbgp->send_cmd("net add node 1.0.0.4");
    $cbgp->send_cmd("net node 1.0.0.4 domain 1");
    $cbgp->send_cmd("net add link 1.0.0.1 1.0.0.2 1");
    $cbgp->send_cmd("net link 1.0.0.1 1.0.0.2 igp-weight --bidir 20");
    $cbgp->send_cmd("net add link 1.0.0.1 1.0.0.3 1");
    $cbgp->send_cmd("net link 1.0.0.1 1.0.0.3 igp-weight --bidir 5");
    $cbgp->send_cmd("net add link 1.0.0.1 1.0.0.4 1");
    $cbgp->send_cmd("net link 1.0.0.1 1.0.0.4 igp-weight --bidir 10");
    $cbgp->send_cmd("net domain 1 compute");

    $cbgp->send_cmd("bgp add router 1 1.0.0.1");
    $cbgp->send_cmd("bgp router 1.0.0.1");
    $cbgp->send_cmd("\tadd peer 2 1.0.0.2");
    $cbgp->send_cmd("\tpeer 1.0.0.2 up");
    $cbgp->send_cmd("\tadd peer 2 1.0.0.3");
    $cbgp->send_cmd("\tpeer 1.0.0.3 up");
    $cbgp->send_cmd("\tadd peer 3 1.0.0.4");
    $cbgp->send_cmd("\tpeer 1.0.0.4 up");
    $cbgp->send_cmd("\texit");

    $cbgp->send_cmd("bgp add router 2 1.0.0.2");
    $cbgp->send_cmd("bgp router 1.0.0.2");
    $cbgp->send_cmd("\tadd network 255/8");
    $cbgp->send_cmd("\tadd peer 1 1.0.0.1");
    $cbgp->send_cmd("\tpeer 1.0.0.1");
    $cbgp->send_cmd("\t\tfilter out");
    $cbgp->send_cmd("\t\t\tadd-rule");
    $cbgp->send_cmd("\t\t\t\tmatch any");
    $cbgp->send_cmd("\t\t\t\taction \"metric 10\"");
    $cbgp->send_cmd("\t\t\t\texit");
    $cbgp->send_cmd("\t\t\texit");
    $cbgp->send_cmd("\t\texit");
    $cbgp->send_cmd("\tpeer 1.0.0.1 up");
    $cbgp->send_cmd("\texit");

    $cbgp->send_cmd("bgp add router 2 1.0.0.3");
    $cbgp->send_cmd("bgp router 1.0.0.3");
    $cbgp->send_cmd("\tadd network 255/8");
    $cbgp->send_cmd("\tadd peer 1 1.0.0.1");
    $cbgp->send_cmd("\tpeer 1.0.0.1");
    $cbgp->send_cmd("\t\tfilter out");
    $cbgp->send_cmd("\t\t\tadd-rule");
    $cbgp->send_cmd("\t\t\t\tmatch any");
    $cbgp->send_cmd("\t\t\t\taction \"metric 20\"");
    $cbgp->send_cmd("\t\t\t\texit");
    $cbgp->send_cmd("\t\t\texit");
    $cbgp->send_cmd("\t\texit");
    $cbgp->send_cmd("\tpeer 1.0.0.1 up");
    $cbgp->send_cmd("\texit");

    $cbgp->send_cmd("bgp add router 3 1.0.0.4");
    $cbgp->send_cmd("bgp router 1.0.0.4");
    $cbgp->send_cmd("\tadd network 255/8");
    $cbgp->send_cmd("\tadd peer 1 1.0.0.1");
    $cbgp->send_cmd("\tpeer 1.0.0.1 up");
    $cbgp->send_cmd("\texit");

    $cbgp->send_cmd("sim run");

    my $rib= cbgp_get_rib($cbgp, '1.0.0.1', '255/8');
    return TEST_FAILURE
      if (!check_has_bgp_route($rib, "255/8",
			       -nexthop=>'1.0.0.4'));

    $cbgp->send_cmd("net link 1.0.0.1 1.0.0.2 down");
    $cbgp->send_cmd("net domain 1 compute");
    $cbgp->send_cmd("bgp router 1.0.0.1 rescan");
    $cbgp->send_cmd("bgp router 1.0.0.2 rescan");
    $cbgp->send_cmd("sim run");

    $rib= cbgp_get_rib($cbgp, '1.0.0.1', '255/8');
    return TEST_FAILURE
      if (!check_has_bgp_route($rib, "255/8",
			       -nexthop=>'1.0.0.3'));

    $cbgp->send_cmd("net link 1.0.0.1 1.0.0.2 up");
    $cbgp->send_cmd("net domain 1 compute");
    $cbgp->send_cmd("bgp router 1.0.0.1 rescan");
    $cbgp->send_cmd("bgp router 1.0.0.2 rescan");
    $cbgp->send_cmd("sim run");

    $rib= cbgp_get_rib($cbgp, '1.0.0.1', '255/8');
    return TEST_FAILURE
      if (!check_has_bgp_route($rib, "255/8",
			       -nexthop=>'1.0.0.4'));

    $cbgp->send_cmd("bgp router 1.0.0.2 del network 255/8");
    $cbgp->send_cmd("sim run");

    $rib= cbgp_get_rib($cbgp, '1.0.0.1', '255/8');
    return TEST_FAILURE
      if (!check_has_bgp_route($rib, "255/8",
			       -nexthop=>'1.0.0.3'));
    return TEST_SUCCESS;
  }

# -----[ cbgp_valid_bgp_clearadjrib ]--------------------------------
#
# -------------------------------------------------------------------
sub cbgp_valid_bgp_clearadjrib($)
  {
    my ($cbgp)= @_;
    return TEST_NOT_TESTED;
  }

# -----[ cbgp_valid_bgp_router_show_filter ]-------------------------
#
# -------------------------------------------------------------------
sub cbgp_valid_bgp_router_show_filter($)
  {
    my ($cbgp)= @_;
    return TEST_NOT_TESTED;
  }

# -----[ cbgp_valid_bgp_options_autocreate ]-------------------------
#
# -------------------------------------------------------------------
sub cbgp_valid_bgp_options_autocreate($)
  {
    my ($cbgp)= @_;
    return TEST_NOT_TESTED;
  }

# -----[ cbgp_valid_net_node_traffic_load ]--------------------------
#
# -------------------------------------------------------------------
sub cbgp_valid_net_node_traffic_load($)
  {
    my ($cbgp)= @_;
    return TEST_NOT_TESTED;
  }

# -----[ cbgp_valid_net_node_traffic_load_netflow ]------------------
#
# -------------------------------------------------------------------
sub cbgp_valid_net_node_traffic_load_netflow($)
  {
    my ($cbgp)= @_;
    return TEST_NOT_TESTED;
  }

# -----[ cbgp_valid_bgp_router_load_cisco ]--------------------------
#
# -------------------------------------------------------------------
sub cbgp_valid_bgp_router_load_cisco($)
  {
    my ($cbgp)= @_;
    return TEST_NOT_TESTED;
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
$tests->register("cli", "cbgp_valid_cli");
$tests->register("net node", "cbgp_valid_net_node");
$tests->register("net node duplicate", "cbgp_valid_net_node_duplicate");
$tests->register("net node name", "cbgp_valid_net_node_name");
$tests->register("net link", "cbgp_valid_net_link", $topo);
$tests->register("net link loop", "cbgp_valid_net_link_loop");
$tests->register("net link duplicate", "cbgp_valid_net_link_duplicate");
$tests->register("net link missing", "cbgp_valid_net_link_missing");
$tests->register("net link weight bidir", "cbgp_valid_net_link_weight_bidir");
$tests->register("net link capacity", "cbgp_valid_net_link_capacity");
$tests->register("net link-ptp", "cbgp_valid_net_link_ptp");
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
$tests->register("bgp add router", "cbgp_valid_bgp_add_router");
$tests->register("bgp add router (duplicate)", "cbgp_valid_bgp_add_router_dup");
$tests->register("bgp router add peer", "cbgp_valid_bgp_router_add_peer");
$tests->register("bgp router add peer (duplicate)", "cbgp_valid_bgp_router_add_peer_dup");
$tests->register("bgp router add peer (self)", "cbgp_valid_bgp_router_add_peer_self");
$tests->register("bgp show routers", "cbgp_valid_bgp_show_routers");
$tests->register("bgp options show-mode cisco",
		 "cbgp_valid_bgp_options_showmode_cisco");
$tests->register("bgp options show-mode mrt",
		 "cbgp_valid_bgp_options_showmode_mrt");
$tests->register("bgp options show-mode custom",
		 "cbgp_valid_bgp_options_showmode_custom");
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
$tests->register("bgp filter action as-path remove-private",
	      "cbgp_valid_bgp_filter_action_aspath_rem_private");
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
$tests->register("bgp load rib (full)", "cbgp_valid_bgp_load_rib_full");
$tests->register("bgp load rib (full,binary)", "cbgp_valid_bgp_load_rib_full_binary");
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
$tests->register("bgp topology load", "cbgp_valid_bgp_topology_load");
$tests->register("bgp topology load (addr-sch=local)",
		 "cbgp_valid_bgp_topology_load_local");
$tests->register("bgp topology load (format=caida)",
		 "cbgp_valid_bgp_topology_load_caida");
$tests->register("bgp topology load (format=meulle)",
		 "cbgp_valid_bgp_topology_load_meulle");
$tests->register("bgp topology load (consistency)",
		 "cbgp_valid_bgp_topology_load_consistency");
$tests->register("bgp topology load (duplicate)",
		 "cbgp_valid_bgp_topology_load_duplicate");
$tests->register("bgp topology load (loop)",
		 "cbgp_valid_bgp_topology_load_loop");
$tests->register("bgp topology check (cycle)",
		 "cbgp_valid_bgp_topology_check_cycle");
$tests->register("bgp topology check (connectedness)",
		 "cbgp_valid_bgp_topology_check_connectedness");
$tests->register("bgp topology policies (filters)",
		 "cbgp_valid_bgp_topology_policies_filters");
$tests->register("bgp topology policies (filters-caida)",
		 "cbgp_valid_bgp_topology_policies_filters_caida");
$tests->register("bgp topology policies", "cbgp_valid_bgp_topology_policies");
$tests->register("bgp topology run", "cbgp_valid_bgp_topology_run");
$tests->register("bgp topology subra-2004", "cbgp_valid_bgp_topology_subra2004");
$tests->register("bgp topology small-topo", "cbgp_valid_bgp_topology_smalltopo");
$tests->register("bgp topology large-topo", "cbgp_valid_bgp_topology_largetopo");
$tests->register("net traffic load", "cbgp_valid_net_traffic_load");
$tests->register("abilene", "cbgp_valid_abilene");
$tests->register("enst", "cbgp_valid_enst");
$tests->register("nasty: bgp peering one-way down",
		 "cbgp_valid_nasty_bgp_peering_1w_down");
#$tests->register("nasty: bgp session segment ordering",
#		 "cbgp_valid_nasty_bgp_session_segment_ordering");
$tests->register("nasty: bgp med ordering",
		 "cbgp_valid_nasty_bgp_med_ordering");
$tests->register("experimental: bgp clear-adj-rib",
		 "cbgp_valid_bgp_clearadjrib");
$tests->register("bgp router show filter",
		 "cbgp_valid_bgp_router_show_filter");
$tests->register("bgp options auto-create",
		 "cbgp_valid_bgp_options_autocreate");
$tests->register("net node traffic load",
		 "cbgp_valid_net_node_traffic_load");
$tests->register("net node traffic load (netflow)",
		 "cbgp_valid_net_node_traffic_load_netflow");
$tests->register("bgp router load cisco",
		 "cbgp_valid_bgp_router_load_cisco");

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

