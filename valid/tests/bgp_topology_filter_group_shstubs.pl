use strict;

use CBGPValid::Topologies;

return ["bgp topology filter group (single-homed-stubs)",
	"cbgp_valid_bgp_topology_filter_group_shstubs"];

# -----[ cbgp_valid_bgp_topology_filter_group_shstubs ]--------------
# This test checks that it is possible to remove all the stubs from
# an AS-level topology.
#
# Setup:
#   - AS1 provider of {AS3, AS4}
#   - AS2 provider of {AS3, AS4, AS7}
#   - AS3 provider of {AS5, AS6}
#   - AS4 provider of {AS6, AS7}
#   - AS5
#   - AS6
#   - AS7
#
# Topology:
#
#     1 --- 2
#     | \ / |\
#     |  \  | \
#     | / \ |  |
#     3 --- 4  |
#    /\    /\  |
#   /  \  /  \ |
#  /    \/    \|
#  5     6     7
#
# Scenario:
#   * Load the whole topology
#   * Check presence of AS1..AS7
#   * Filter single-homed stubs
#   * Check presence of AS1..AS4 and AS6..AS7
#   * Check absence of AS5
# -------------------------------------------------------------------
sub cbgp_valid_bgp_topology_filter_group_shstubs($) {
  my ($cbgp)= @_;

  my $filename= get_resource("as-level-topo-small.txt");
  (-e $filename) or return TEST_DISABLED;

  my $options= "--addr-sch=local";

  $cbgp->send_cmd("bgp topology load $options \"$filename\"");

  # Obtain topology from cbgp
  my $topo= cbgp_bgp_topology($cbgp, C_TOPO_ADDR_SCH_ASN());

  # Check that AS1-7 exist
  foreach my $asn ((1, 2, 3, 4, 5, 6, 7)) {
    return TEST_FAILURE
      if (!topo_check_asn($topo, $asn));
  }

  # Filter single-homed stub domains
  my $msg=cbgp_check_error($cbgp, "bgp topology filter group single-homed-stubs");
  return TEST_FAILURE if ($msg);

  # Obtain topology from cbgp
  $topo= cbgp_bgp_topology($cbgp, C_TOPO_ADDR_SCH_ASN);

  # Check that AS1-4, AS6 exist
  foreach my $asn ((1, 2, 3, 4, 6, 7)) {
    return TEST_FAILURE
      if (!topo_check_asn($topo, $asn));
  }
  # Check that AS5 and AS7 do not exist
  foreach my $asn ((5)) {
    return TEST_FAILURE
      if (topo_check_asn($topo, $asn));
  }

  return TEST_SUCCESS;
}
