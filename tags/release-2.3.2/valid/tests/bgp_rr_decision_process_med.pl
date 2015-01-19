return ["bgp RR decision process (med)",
	"cbgp_valid_bgp_rr_dp_med"];

# -----[ cbgp_valid_bgp_rr_dp_med ]----------------------------------
# Check that a BGP route-reflector that receives two equal routes
# will break ties based on MED.
#
# Setup:
#   - R1 (1.0.0.1, AS3), virtual
#   - R2 (1.0.0.2, AS3), virtual
#   - R3 (1.0.0.3, AS3), virtual
#   - R4 (1.0.0.4, AS3, RR)
#
# Topology:
#
#    <AS1,med=30> --- R1 --*
#                           \
#    <AS1,med=20> --- R2 --- R4
#                           /
#    <AS2,med=10> --- R3 --*
#
# Scenario:
#   * Inject 255/8 to R4, from R1, with MED=30
#   * Inject 255/8 to R4, from R2, with MED=20
#   * Inject 255/8 to R4, from R3, with MED=10
#   * Check that route selected is from R2
# -------------------------------------------------------------------
sub cbgp_valid_bgp_rr_dp_med($) {
  my ($cbgp)= @_;
  $cbgp->send_cmd("net add domain 1 igp");
  $cbgp->send_cmd("net add node 1.0.0.1");
  $cbgp->send_cmd("net add node 1.0.0.2");
  $cbgp->send_cmd("net add node 1.0.0.3");
  $cbgp->send_cmd("net add node 1.0.0.4");
  $cbgp->send_cmd("net node 1.0.0.1 domain 1");
  $cbgp->send_cmd("net node 1.0.0.2 domain 1");
  $cbgp->send_cmd("net node 1.0.0.3 domain 1");
  $cbgp->send_cmd("net node 1.0.0.4 domain 1");
  $cbgp->send_cmd("net add link 1.0.0.1 1.0.0.4");
  $cbgp->send_cmd("net link 1.0.0.1 1.0.0.4 igp-weight --bidir 5");
  $cbgp->send_cmd("net add link 1.0.0.2 1.0.0.4");
  $cbgp->send_cmd("net link 1.0.0.2 1.0.0.4 igp-weight --bidir 10");
  $cbgp->send_cmd("net add link 1.0.0.3 1.0.0.4");
  $cbgp->send_cmd("net link 1.0.0.3 1.0.0.4 igp-weight --bidir 7");
  $cbgp->send_cmd("net domain 1 compute");
  $cbgp->send_cmd("bgp add router 3 1.0.0.4");
  $cbgp->send_cmd("bgp router 1.0.0.4");
  $cbgp->send_cmd("\tadd peer 3 1.0.0.1");
  $cbgp->send_cmd("\tpeer 1.0.0.1 rr-client");
  $cbgp->send_cmd("\tpeer 1.0.0.1 virtual");
  $cbgp->send_cmd("\tpeer 1.0.0.1 up");
  $cbgp->send_cmd("\tadd peer 3 1.0.0.2");
  $cbgp->send_cmd("\tpeer 1.0.0.2 rr-client");
  $cbgp->send_cmd("\tpeer 1.0.0.2 virtual");
  $cbgp->send_cmd("\tpeer 1.0.0.2 up");
  $cbgp->send_cmd("\tadd peer 3 1.0.0.3");
  $cbgp->send_cmd("\tpeer 1.0.0.3 rr-client");
  $cbgp->send_cmd("\tpeer 1.0.0.3 virtual");
  $cbgp->send_cmd("\tpeer 1.0.0.3 up");
  $cbgp->send_cmd("\texit");

  cbgp_recv_update($cbgp, "1.0.0.4", 3, "1.0.0.1",
		   "255/8|1 127|IGP|1.0.0.1|0|30");
  cbgp_recv_update($cbgp, "1.0.0.4", 3, "1.0.0.3",
		   "255/8|2 127|IGP|1.0.0.3|0|10");
  $cbgp->send_cmd("sim run");
  my $rib;
  $rib= cbgp_get_rib_mrt($cbgp, "1.0.0.4", "255/8");
  return TEST_FAILURE
    if (!check_has_bgp_route($rib, "255/8",
			     -nexthop=>"1.0.0.1"));

  cbgp_recv_update($cbgp, "1.0.0.4", 3, "1.0.0.2",
		   "255/8|1 127|IGP|1.0.0.2|0|20");
  $cbgp->send_cmd("sim run");
  $rib= cbgp_get_rib_mrt($cbgp, "1.0.0.4", "255/8");
  return TEST_FAILURE
    if (!check_has_bgp_route($rib, "255/8",
			     -nexthop=>"1.0.0.3"));


  return TEST_SUCCESS;
}

