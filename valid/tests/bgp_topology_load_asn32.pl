return ["bgp topology load (asn32)", "cbgp_valid_bgp_topology_load_asn32"];

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
#   [valid-bgp-topology-asn32.subramanian]
# -------------------------------------------------------------------
sub cbgp_valid_bgp_topology_load_asn32($) {
  my ($cbgp)= @_;

  my $topo_file= get_resource("valid-bgp-topology-asn32.subramanian");

  (-e $topo_file) or return TEST_SKIPPED;

  my $topo= topo_from_subramanian_file($topo_file, C_TOPO_ADDR_SCH_LOCAL);

  cbgp_check_error($cbgp, "bgp topology load \"$topo_file\"")
      and return TEST_FAILURE;
  $cbgp->send_cmd("bgp topology install");

  # Check that all links are created
  (!cbgp_topo_check_links($cbgp, $topo)) and
    return TEST_FAILURE;

  return TEST_SUCCESS;
}

