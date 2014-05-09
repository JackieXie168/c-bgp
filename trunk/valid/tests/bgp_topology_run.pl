return ["bgp topology run", "cbgp_valid_bgp_topology_run"];

# -----[ cbgp_valid_bgp_topology_run ]-------------------------------
# Setup:
#   see 'valid bgp topology load'
#
# Scenario:
#
# Resources:
#   [valid-bgp-topology.subramanian]
# -------------------------------------------------------------------
sub cbgp_valid_bgp_topology_run($) {
  my ($cbgp)= @_;

  my $topo_file= get_resource("valid-bgp-topology.subramanian");
  (-e $topo_file) or return TEST_DISABLED;

  # If 32-bits ASN are enabled, the default scheme is LOCAL
  my ($cbgp_version, $libgds_version)= cbgp_version($cbgp);
  my $scheme= C_TOPO_ADDR_SCH_DEFAULT;
  if ($cbgp_version->{'options'} =~ m/asn32/) {
      $scheme= C_TOPO_ADDR_SCH_LOCAL;
  }
  my $topo= topo_from_subramanian_file($topo_file, $scheme);

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

