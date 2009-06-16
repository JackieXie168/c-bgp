return ["net igp external", "cbgp_valid_net_igp_external"];

# -----[ cbgp_valid_net_igp_external ]-------------------------------
# Description:
# This test checks how links accross the boundaries of an IGP domain
# are handled. For this purpose, a node R1 is placed at the border of
# IGP domain 1. Inside domain 1, R1 is connected to R2 with a simple
# link. Outside domain 1, R1 is connected to R3 through 3 different
# links: an rtr link, a ptp link and a ptmp link (through a subnet).
#
# Setup:
#   - R1 (1.0.0.0)
#   - R2 (1.0.0.1)
#   - R3 (2.0.0.0)
#   - N1 (10.0.0.0/24, transit)
#   - N2 (192.168.0/30)
#   - Domain 1 = {R1, R2}
#
# Topology:
#                 .1  N2  .2
#                 *--------*
#                /          \
#     R2 ----- R1 ---------- R3
#                \          /
#                 *-- N1 --*
#
# -------------------------------------------------------------------
sub cbgp_valid_net_igp_external($) {
  my ($cbgp)= @_;

  $cbgp->send_cmd("net add node 1.0.0.0");
  $cbgp->send_cmd("net add node 1.0.0.1");
  $cbgp->send_cmd("net add node 2.0.0.0");
  $cbgp->send_cmd("net add subnet 10.0.0.0/24 transit");
  $cbgp->send_cmd("net add link 1.0.0.0 1.0.0.1");
  $cbgp->send_cmd("net add link 1.0.0.0 2.0.0.0");
  $cbgp->send_cmd("net add link-ptp 1.0.0.0 192.168.0.1/30 2.0.0.0 192.168.0.2/30");
  $cbgp->send_cmd("net add link 1.0.0.0 10.0.0.1/24");
  $cbgp->send_cmd("net add link 2.0.0.0 10.0.0.2/24");

  $cbgp->send_cmd("net add domain 1 igp");
  $cbgp->send_cmd("net node 1.0.0.0 domain 1");
  $cbgp->send_cmd("net node 1.0.0.1 domain 1");
  $cbgp->send_cmd("net link 1.0.0.0 1.0.0.1 igp-weight --bidir 1");
  $cbgp->send_cmd("net link 1.0.0.0 2.0.0.0 igp-weight 1");
  $cbgp->send_cmd("net node 1.0.0.0 iface 192.168.0.1/30 igp-weight 1");
  $cbgp->send_cmd("net node 2.0.0.0 iface 192.168.0.2/30 igp-weight 1");
  $cbgp->send_cmd("net link 1.0.0.0 10.0.0.1/24 igp-weight 1");
  $cbgp->send_cmd("net link 2.0.0.0 10.0.0.2/24 igp-weight 1");
  $cbgp->send_cmd("net domain 1 compute");

  my $rt= cbgp_get_rt($cbgp, '1.0.0.0');

  $rt= cbgp_get_rt($cbgp, '1.0.0.1');

  return TEST_SUCCESS;
}
