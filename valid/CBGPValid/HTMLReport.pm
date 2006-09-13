# ===================================================================
# CBGPValid::HTMLReport.pm
#
# author Bruno Quoitin (bqu@info.ucl.ac.be)
# lastdate 13/09/2006
# ===================================================================

package CBGPValid::HTMLReport;

require Exporter;
@ISA= qw(Exporter);

use strict;
use Symbol;
use CBGPValid::TestConstants;

# -----[ doc_write_section_title ]-----------------------------------
sub doc_write_section_title($$)
{
  my ($stream, $title)= @_;

  print $stream "<h3>$title</h3>\n";
}

# -----[ doc_write_section ]-----------------------------------------
#
# -------------------------------------------------------------------
sub doc_write_section($$)
{
  my ($stream, $section)= @_;

  my $state= 0;

  foreach my $line (@$section) {
    #print "line(state:$state) [$line]\n";
    if ($state == 0) {
      if ($line =~ m/^\s*-(.*)$/) {
	$state= 1;
	print $stream "<ul>\n";
	print $stream "<li>$1\n";
      } elsif ($line =~ m/^\s*\*(.*)$/) {
	$state= 2;
	print $stream "<ol>\n";
	print $stream "<li>$1\n";
      } else {
	print $stream "$line\n";
      }
    } elsif ($state > 0) {
      if ($line =~ m/^\s*$/) {
	if ($state == 1) {
	  print $stream "</ul>\n";
	} elsif ($state == 2) {
	  print $stream "</ol>\n";
	}
	$state= 0;
      } elsif ($line =~ m/^\s*[-*](.*)$/) {
	print $stream "<li>$1\n";
      } else {
	print $stream "$line\n";
      }
    }
  }
  if ($state > 0) {
    if ($state == 1) {
      print $stream "</ul>\n";
    } elsif ($state == 2) {
      print $stream "</ol>\n";
    }
  }
}

# -----[ doc_write_preformated ]-------------------------------------
#
# -------------------------------------------------------------------
sub doc_write_preformated($$)
{
  my ($stream, $section)= @_;

  print $stream "<br>\n";
  print $stream "<pre>\n";
  print $stream "".(join "\n", @{$section});
  print $stream "</pre>\n";
  print $stream "<br>\n";
}

# -----[ doc_write ]-------------------------------------------------
# Generates the documentation file.
# -------------------------------------------------------------------
sub doc_write($$)
{
  my ($filename, $doc)= @_;

  my $stream= gensym();
  open($stream, ">$filename") or
    die "unable to generate \"$filename\": $!";
  print $stream "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\">\n";
  print $stream "<html>\n";
  print $stream "<head>\n";
  print $stream "<title>C-BGP validation documentation</title>\n";
  print $stream "</head>\n";
  print $stream "<body>\n";
  print $stream "<h1>C-BGP validation: tests documentation</h1>\n";
  foreach my $item (sort keys %$doc) {
    print $stream "<hr>\n";
    print $stream "<a name=\"$item\"><h2>$doc->{$item}->{Name}</h2></a>\n";
    # -- Description --
    doc_write_section_title($stream, "Description:");
    if (exists($doc->{$item}->{Description})) {
      doc_write_section($stream, $doc->{$item}->{Description});
    }
    # -- Setup --
    doc_write_section_title($stream, "Setup:");
    if (exists($doc->{$item}->{Setup})) {
      doc_write_section($stream, $doc->{$item}->{Setup});
    }
    # -- Topology --
    if (exists($doc->{$item}->{Topology})) {
      doc_write_preformated($stream, $doc->{$item}->{Topology});
    }
    # -- Scenario --
    doc_write_section_title($stream, "Scenario:");
    if (exists($doc->{$item}->{Scenario})) {
      doc_write_section($stream, $doc->{$item}->{Scenario});
    }
  }
  print $stream "<hr>\n";
  print $stream "(C) 2006, B. Quoitin\n";
  print $stream "</body>\n";
  print $stream "<html>\n";
  close($stream);
}

# -----[ doc_from_script ]-------------------------------------------
# Parses the given Perl script in order to document the validation
# tests.
#
# The function looks for the following sections:
#   - Description: general description of the test
#   - Setup: how C-BGP is configured for the test (topology and config)
#   - Topology: 
#   - Scenario: what is announced and what is tested
# -------------------------------------------------------------------
sub doc_from_script($$)
{
  my ($script_name, $tests_list)= @_;

  my %doc= ();

  # Build index of tests function names
  my %tests_index= ();
  foreach my $test_record (@$tests_list) {
    $tests_index{$test_record->[TEST_FIELD_FUNC]}= 1;
  }

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
	if (exists($tests_index{$func_name})) {
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
      } else {
	if (m/^#(.*$)/) {
	  push @{$record->{$section}}, ($1);
	} else {
	  print "warning: i don't know how to handle \"$_\"\n";
	}
      }
    }
  }
  close(MYSELF);

  return \%doc;
}

# -----[ report_write ]----------------------------------------------
# Build an HTML report containing the test results.
# -------------------------------------------------------------------
sub report_write($$$)
{
  my ($report_prefix,
      $validation,
      $tests)= @_;

  my $cbgp_path= $tests->{'cbgp-path'};
  my $cbgp_version= $validation->{'cbgp_version'};
  my $program_args= $validation->{'program_args'};
  my $program_name= $validation->{'program_name'};
  my $program_version= $validation->{'program_version'};
  my $num_failures= $tests->{'num-failures'};
  my $num_warnings= $tests->{'num-warnings'};

  my $doc= doc_from_script($program_name, $tests->{'list'});
  if (defined($doc)) {
    doc_write("$report_prefix-doc.html", $doc);
  }

  my $report_file_name= "$report_prefix.html";
  open(REPORT, ">$report_file_name") or
    die "could not create HTML report in \"$report_file_name\": $!";
  print REPORT "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\">\n";
  print REPORT "<html>\n";
  print REPORT "<head>\n";
  print REPORT "<title>C-BGP validation test report (v$program_version)</title>\n";
  print REPORT "</head>\n";
  print REPORT "<body>\n";
  print REPORT "<h2>C-BGP validation test report (v$program_version)</h2>\n";
  print REPORT "<h3>Configuration</h3>\n";
  print REPORT "<ul>\n";
  print REPORT "<li>Arguments: $program_args</li>\n";
  print REPORT "<li>C-BGP version: ".(defined($cbgp_version)?$cbgp_version:"undef")."</li>\n";
  print REPORT "<li>C-BGP path: $cbgp_path</li>\n";
  print REPORT "<li>Number of failures: $num_failures";
  ($tests->{'max-failures'} > 0) and
    print REPORT " (max: ".$tests->{'max-failures'}.")";
  print REPORT "</li>\n";
  print REPORT "<li>Number of warnings: $num_warnings";
  ($tests->{'max-warnings'} > 0) and
    print REPORT " (max: ".$tests->{'max-warnings'}.")";
  print REPORT "</li>\n";
  print REPORT "<li>From cache: ".
    (defined($tests->{'cache-file'})?"yes":"no").
      "</li>\n";
  print REPORT "<li>Test duration: ".$tests->{'duration'}." secs.</li>\n";
  my $time= POSIX::strftime("%Y/%m/%d %H:%M", localtime(time()));
  print REPORT "<li>Finished: $time</li>\n";
  print REPORT "<li>System: ".`uname -m -p -s -r`."</li>\n";
  print REPORT "</ul>\n";
  print REPORT "<h3>Results</h3>\n";
  if ($num_failures == 0) {
    print REPORT "&nbsp;&nbsp;<font color=\"green\">ALL TESTS PASSED :-)</font>\n";
  } else {
    print REPORT "&nbsp;&nbsp;<font color=\"red\">SOME TESTS FAILED :-(</font>\n";
  }
  print REPORT "<br><br>\n";
  print REPORT "<table border=\"1\">\n";
  print REPORT "<tr>\n";
  print REPORT "<th>Test ID</th>\n";
  print REPORT "<th>Test description</th>\n";
  print REPORT "<th>Result</th>\n";
  print REPORT "<th>Duration (s.)</th>\n";
  print REPORT "</tr>\n";
  foreach my $test_record (@{$tests->{'list'}}) {
    my $test_id= $test_record->[TEST_FIELD_ID];
    my $test_name= $test_record->[TEST_FIELD_NAME];
    my $test_func= $test_record->[TEST_FIELD_FUNC];
    my $test_result= $test_record->[TEST_FIELD_RESULT];
    my $test_duration= $test_record->[TEST_FIELD_DURATION];

    my $color= "black";

    if ($test_result == TEST_DISABLED) {
      $color= "gray";
    } elsif (($test_result == TEST_FAILURE) ||
	     ($test_result == TEST_NOT_TESTED)) {
      $color= "red";
    } elsif ($test_result == TEST_SUCCESS) {
      $color= "green";
    }
    $test_result= TEST_RESULT_MSG->{$test_result};

    my $doc_ref= undef;
    if (defined($doc) && exists($doc->{$test_func})) {
      $doc_ref= "$report_prefix-doc.html#$test_func";
    } else {
      show_warning("no documentation for $test_name ($test_func)");
    }

    (!defined($test_duration)) and
      $test_duration= "---";

    print REPORT "<tr>\n";
    print REPORT "<td align=\"center\">$test_id</td>\n";
    print REPORT "<td>\n";
    (defined($doc_ref)) and
      print REPORT "<a href=\"$doc_ref\">\n";
    print REPORT "$test_name\n";
    (defined($doc_ref)) and
      print REPORT "</a>\n";
    print REPORT "</td>\n";
    print REPORT "<td align=\"center\"><font color=\"$color\">$test_result</font></td>\n";
    print REPORT "<td align=\"center\">$test_duration</td>\n";
    print REPORT "</tr>\n";
  }
  print REPORT "</table>\n";
  print REPORT "<hr>\n";
  print REPORT "(C) 2006, B. Quoitin\n";
  print REPORT "</body>\n";
  print REPORT "</html>\n";
  close(REPORT);
}
