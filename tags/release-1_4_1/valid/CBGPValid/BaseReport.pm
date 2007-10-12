# ===================================================================
# CBGPValid::BaseReport.pm
#
# author Bruno Quoitin (bqu@info.ucl.ac.be)
# lastdate 14/09/2006
# ===================================================================

package CBGPValid::BaseReport;

require Exporter;
@ISA= qw(Exporter);

use strict;
use Symbol;
use CBGPValid::TestConstants;
use CBGPValid::UI;

my %resources= ();
1;

# -----[ doc_from_script ]-------------------------------------------
# Parses the given Perl script in order to document the validation
# tests.
#
# The function looks for the following sections:
#   - Description: general description of the test
#   - Setup: how C-BGP is configured for the test (topology and config)
#   - Topology: an optional "ASCII-art" topology
#   - Scenario: what is announced and what is tested
#   - Resources: an optional list of resources (files/URLs)
# -------------------------------------------------------------------
sub doc_from_script($$)
{
  my ($script_name, $tests_index)= @_;

  my %doc= ();

  open(MYSELF, "<$script_name") or
    die "unable to open \"$script_name\"";
  my $state= 0;
  my $section= undef;
  my $name= undef;
  my $record;
  while (<MYSELF>) {
    if ($state == 0) {
      if (m/^# -----\[\s*cbgp_valid_([^ ]*)\s*\]/) {
	my $func_name= "cbgp_valid_$1";
	if (exists($tests_index->{$func_name})) {
	  $state= 1;
	  $section= 'Description';
	  $name= $func_name;
	  $record= { 'Name' => $1 };
	}
      }
    } elsif ($state >= 1) {
      if ((!m/^#/) || (m/^# ----------.*/)) {
	$state= 0;
	$doc{$name}= $record;
      } elsif (m/^# Description\:/) {
	$section= 'Description';
	$record->{$section}= [];
      } elsif (m/^# Setup\:/) {
	$section= 'Setup';
	$record->{$section}= [];
      } elsif (m/^# Scenario\:/) {
	$section= 'Scenario';
	$record->{$section}= [];
      } elsif (m/^# Topology\:/) {
	$section= 'Topology';
	$record->{$section}= [];
      } elsif (m/^# Resources\:/) {
	$section= 'Resources';
	$record->{$section}= [];
      } else {
	if (m/^#(.*$)/) {
	  push @{$record->{$section}}, ($1);
	} else {
	  show_warning("I don't know how to handle \"$_\"\n");
	}
      }
    }
  }
  close(MYSELF);

  # Post-processing
  for my $doc_item (values %doc) {
    (exists($doc_item->{'Resources'})) and
      $doc_item->{'Resources'}=
	parse_resources_section($doc_item->{'Resources'});
  }

  return \%doc;
}

# -----[ parse_resources_section ]-----------------------------------
#
# -------------------------------------------------------------------
sub parse_resources_section($)
{
  my ($section)= @_;

  my @section_resources= ();

  foreach my $line (@$section) {
    if ($line =~ m/^\s*\[(\S+)(\s+[^\]])?\]\s*$/) {
      my $ref= $1;
      my $name= $2;
      (!defined($2)) and $name= $ref;
      $resources{$ref}= 1;
      push @section_resources, ([$ref, $name]);
    } else {
      show_warning("invalid resource \"$line\" (skipped)");
    }
  }


  return \@section_resources;
}

# -----[ save_resources ]--------------------------------------------
#
# -------------------------------------------------------------------
sub save_resources($)
{
  my ($file)= @_;

  open(RESOURCES, ">$file") or
    die "unable to create resources file \"$file\": $!";
  foreach my $ref (sort keys %resources) {
    print RESOURCES "$ref\n";
  }
  close(RESOURCES);
}
