return ["net subnet duplicate", "cbgp_valid_net_subnet_duplicate"];

# -----[ cbgp_valid_net_subnet_duplicate ]---------------------------
#
# -------------------------------------------------------------------
sub cbgp_valid_net_subnet_duplicate($) {
  my ($cbgp)= @_;
  my $msg;

  $cbgp->send_cmd("net add subnet 192.168.0/24 transit\n");
  $msg= cbgp_check_error($cbgp, "net add subnet 192.168.0/24 transit\n");
  return TEST_FAILURE
    if (!check_has_error($msg, $CBGP_ERROR_SUBNET_EXISTS));
  $msg= cbgp_check_error($cbgp, "net add subnet 192.168.0.1/24 transit\n");
  return TEST_FAILURE
    if (!check_has_error($msg, $CBGP_ERROR_SUBNET_EXISTS));
  return TEST_SUCCESS;
}

