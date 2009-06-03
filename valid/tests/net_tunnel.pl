return ["net tunnel", "cbgp_valid_net_tunnel"];

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
    $cbgp->send_cmd("net add link 1.0.0.0 1.0.0.1");
    $cbgp->send_cmd("net link 1.0.0.0 1.0.0.1 igp-weight --bidir 1");
    $cbgp->send_cmd("net add link 1.0.0.0 1.0.0.2");
    $cbgp->send_cmd("net link 1.0.0.0 1.0.0.2 igp-weight --bidir 1");
    $cbgp->send_cmd("net add link 1.0.0.1 1.0.0.3");
    $cbgp->send_cmd("net link 1.0.0.1 1.0.0.3 igp-weight --bidir 1");
    $cbgp->send_cmd("net add link 1.0.0.2 1.0.0.3");
    $cbgp->send_cmd("net link 1.0.0.2 1.0.0.3 igp-weight --bidir 10");
    $cbgp->send_cmd("net add link 1.0.0.2 192.168.0.1/24");
    $cbgp->send_cmd("net link 1.0.0.2 192.168.0.1/24 igp-weight 1");
    $cbgp->send_cmd("net add link 1.0.0.3 192.168.0.2/24");
    $cbgp->send_cmd("net link 1.0.0.3 192.168.0.2/24 igp-weight 1");
    $cbgp->send_cmd("net add link 1.0.0.4 192.168.0.3/24");
    $cbgp->send_cmd("net link 1.0.0.4 192.168.0.3/24 igp-weight 1");

    # Domain 2
    $cbgp->send_cmd("net add domain 2 igp");
    $cbgp->send_cmd("net add node 2.0.0.0");

    # Interdomain link (+ static route in reverse direction)
    $cbgp->send_cmd("net add link 1.0.0.3 2.0.0.0");
    $cbgp->send_cmd("net link 1.0.0.3 2.0.0.0 igp-weight 1");
    $cbgp->send_cmd("net node 2.0.0.0 route add --oif=1.0.0.3 1.0.0/24 1");

    $cbgp->send_cmd("net domain 1 compute");

    # Default route from R1 to R4 should be R1 R2 R4
    my $trace= cbgp_traceroute($cbgp, "1.0.0.0", "1.0.0.3");
    return TEST_FAILURE
      if (!check_traceroute($trace, -status=>C_TRACEROUTE_STATUS_REPLY,
			    -nhops=>2,
			    -hops=>[[0,"1.0.0.1",C_PING_STATUS_ICMP_TTL],
				    [1,"1.0.0.3",C_PING_STATUS_REPLY]]));
    # Default route from R1 to R5 should be R1 R3 R5
    $trace= cbgp_traceroute($cbgp, "1.0.0.0", "1.0.0.4");
    return TEST_FAILURE
      if (!check_traceroute($trace, -status=>C_TRACEROUTE_STATUS_REPLY,
			    -nhops=>2,
			    -hops=>[[0,"1.0.0.2",C_PING_STATUS_ICMP_TTL],
				    [1,"1.0.0.4",C_PING_STATUS_REPLY]]));
    # Default route from R1 to R6 should be R1 R2 R4 R6
    $trace= cbgp_traceroute($cbgp, "1.0.0.0", "2.0.0.0");
    return TEST_FAILURE
      if (!check_traceroute($trace, -status=>C_TRACEROUTE_STATUS_REPLY,
			    -nhops=>3,
			   -hops=>[[0,"1.0.0.1",C_PING_STATUS_ICMP_TTL],
				   [1,"1.0.0.3",C_PING_STATUS_ICMP_TTL],
				   [2,"2.0.0.0",C_PING_STATUS_REPLY]]));

    # Create tunnels + static routes
    $cbgp->send_cmd("net node 1.0.0.2 ipip-enable");
    $cbgp->send_cmd("net node 1.0.0.0");
    $cbgp->send_cmd("  tunnel add 1.0.0.2 255.0.0.0");
    $cbgp->send_cmd("  tunnel add 1.0.0.2 255.0.0.1");
    $cbgp->send_cmd("  tunnel add 1.0.0.4 255.0.0.2");
    $cbgp->send_cmd("  route add --oif=255.0.0.0 1.0.0.3/32 1");
    $cbgp->send_cmd("  route add --oif=255.0.0.1 2.0.0/24 1");
    $cbgp->send_cmd("  route add --oif=255.0.0.2 1.0.0.1/32 1");
    $cbgp->send_cmd("  exit");

    # Path from R1 to R4 should be R1 R3 R4
    $trace= cbgp_traceroute($cbgp, "1.0.0.0", "1.0.0.3");
    return TEST_FAILURE
      if (!check_traceroute($trace, -status=>C_TRACEROUTE_STATUS_REPLY,
			    -nhops=>5));
    # Path from R1 to R5 should be R1 R3 R4 R5
    $trace= cbgp_traceroute($cbgp, "1.0.0.0", "1.0.0.4");
    # Path from R1 to R6 should be R1 R3 R5 R6 (i.e. through N1)
    $trace= cbgp_traceroute($cbgp, "1.0.0.0", "2.0.0.0");

    return TEST_SUCCESS;
  }

