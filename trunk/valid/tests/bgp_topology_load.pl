return ["bgp topology load", "cbgp_valid_bgp_topology_load"];

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
sub cbgp_valid_bgp_topology_load($$) {
  my ($cbgp)= @_;

  my $topo_file= get_resource("valid-bgp-topology.subramanian");

  (-e $topo_file) or return TEST_SKIPPED;

  # If 32-bits ASN are enabled, the default scheme is LOCAL
  my ($cbgp_version, $libgds_version)= cbgp_version($cbgp);
  my $scheme= C_TOPO_ADDR_SCH_DEFAULT;
  if ($cbgp_version->{'options'} =~ m/asn32/) {
      $scheme= C_TOPO_ADDR_SCH_LOCAL;
  }
  my $topo= topo_from_subramanian_file($topo_file, $scheme);

  $cbgp->send_cmd("bgp topology load \"$topo_file\"");
  $cbgp->send_cmd("bgp topology install");

  # Check that all links are created
  (!cbgp_topo_check_links($cbgp, $topo)) and
    return TEST_FAILURE;

  return TEST_SUCCESS;
}

