# ===================================================================
# CBGPValid::Tests.pm
#
# author Bruno Quoitin (bqu@info.ucl.ac.be)
# lastdate 13/09/2006
# ===================================================================

package CBGPValid::Tests;

require Exporter;
@ISA= qw(Exporter);
@EXPORT= qw(show_testing
	    show_testing_success
	    show_testing_cache
	    show_testing_failure
	    show_testing_skipped
	    show_testing_disabled
	   );

use strict;
use CBGP;
use CBGPValid::TestConstants;
use CBGPValid::UI;

my $tests_id;
1;

# -----[ show_testing ]----------------------------------------------
sub show_testing($)
{
  my ($msg)= @_;

  print STDERR "Testing: \033[37;1m$msg\033[0m";
}

# -----[ show_testing_success ]--------------------------------------
sub show_testing_success()
{
  print STDERR "\033[70G\033[32;1mSUCCESS\033[0m\n";
}

# -----[ show_testing_cache ]----------------------------------------
sub show_testing_cache()
{
  print STDERR "\033[70G\033[32;1m(CACHE)\033[0m\n";
}

# -----[ show_testing_failure ]--------------------------------------
sub show_testing_failure()
{
  my ($self)= @_;

  print STDERR "\033[70G\033[31;1mFAILURE\033[0m\n";
}

# -----[ show_testing_skipped ]--------------------------------------
sub show_testing_skipped()
{
  print STDERR "\033[70G\033[31;1mNOT-TESTED\033[0m\n";
}

# -----[ show_testing_skipped ]--------------------------------------
sub show_testing_disabled()
{
  print STDERR "\033[70G\033[33;1mDISABLED\033[0m\n";
}

# -----[ debug ]-----------------------------------------------------
sub debug($$)
{
  my ($self, $msg)= @_;

  ($self->{'debug'}) and
    print STDERR "debug: $msg\n";
}


# -----[ new ]-------------------------------------------------------
sub new($%)
{
  my ($class, %args)= @_;
  my $self= {
	     'cache' => {},
	     'cache-file' => undef,
	     'cbgp-path' => undef,
	     'debug' => 0,
	     'duration' => 0,
	     'list' => [],
	     'id' => 0,
	     'include' => undef,
	     'max-failures' => 0,
	     'max-warnings' => 0,
	     'num-failures' => 0,
	     'num-warnings' => 0,
	    };
  (exists($args{'-cache'})) and
    $self->{'cache-file'}= $args{'-cache'};
  (exists($args{'-cbgppath'})) and
    $self->{'cbgp-path'}= $args{'-cbgppath'};
  (exists($args{'-debug'})) and
    $self->{'debug'}= $args{'-debug'};
  (exists($args{'-include'})) and
    $self->{'include'}= $args{'-include'};
  bless $self, $class;

  (defined($self->{'cache-file'})) and
    $self->cache_read();

  return $self;
}

# -----[ register ]--------------------------------------------------
sub register($$$;@)
{
  my $self= shift(@_);
  my $name= shift(@_);
  my $func= shift(@_);

  my $test_record= [$tests_id++, $name, $func, TEST_NOT_TESTED, undef];
  while (@_) {
    push @$test_record, (shift(@_));
  }

  push @{$self->{'list'}}, ($test_record);
}

# -----[ set_result ]------------------------------------------------
sub set_result($$;$)
{
  my ($self, $test, $result, $duration)= @_;

  $test->[TEST_FIELD_RESULT]= $result;
  (defined($duration)) and
    $test->[TEST_FIELD_DURATION]= $duration;
}

# -----[ run ]-------------------------------------------------------
sub run($)
{
  my ($self)= @_;

  my $cache= $self->{'cache'};

  my $time_start= time();
  foreach my $test_record (@{$self->{'list'}}) {
    my $test_id= $test_record->[TEST_FIELD_ID];
    my $test_name= $test_record->[TEST_FIELD_NAME];
    my $test_func= "::".$test_record->[TEST_FIELD_FUNC];
    my $test_args= $test_record->[TEST_FIELD_ARGS];

    if (!defined($test_func) ||
	(defined($self->{'include'}) &&
	 !exists($self->{'include'}->{$test_name}))) {
      $self->set_result($test_record, TEST_DISABLED);
      show_testing("$test_name");
      show_testing_disabled();
      $cache->{$test_name}= TEST_DISABLED;
    } else {
      if (!exists($cache->{$test_name}) ||
	  ($cache->{$test_name} != TEST_SUCCESS)) {
	my $result;
	my $cbgp= CBGP->new($self->{'cbgp-path'});
	my $log_file= ".$test_name.log";
	($log_file =~ tr[\ -][___]);
	unlink $log_file;
	$cbgp->{log_file}= $log_file;
	$cbgp->{log}= 1;
	$cbgp->spawn();
	die if $cbgp->send("set autoflush on\n");
	$self->debug("testing $test_name");
	my $test_time_start= time();

	# Call the function (with a symbolic reference)
	# We need to temporarily disable strict references checking...
	no strict 'refs';
	$result= &$test_func($cbgp, $test_args);
	use strict 'refs';

	my $test_time_end= time();
	my $test_time_duration= $test_time_end-$test_time_start;
	$cbgp->finalize();
	$self->set_result($test_record, $result, $test_time_duration);
	show_testing("$test_name");
	if ($result == TEST_SUCCESS) {
	  show_testing_success();
	} elsif ($result == TEST_NOT_TESTED) {
	  show_testing_skipped();
	} else {
	  $self->{'num-failures'}++;
	  show_testing_failure();
	}
	$cache->{$test_name}= $result;
      } else {
	$self->set_result($test_record, $cache->{$test_name});
	show_testing($test_name);
	show_testing_cache();
      }
    }
    if (($self->{'max-failures'} > 0) &&
	($self->{'num-failures'} >= $self->{'max-failures'})) {
      show_error("Error: too many failures.");
      last;
    }
  }
  $self->{'duration'}= time()-$time_start;

  (defined($self->{'cache-file'})) and
    $self->cache_write();

  return $self->{'num-failures'};
}

# -----[ cache_read ]------------------------------------------------
sub cache_read()
{
  my ($self)= @_;

  if (open(CACHE, "<".$self->{'cache-file'})) {
    while (<CACHE>) {
      chomp;
      my ($test_result, $test_name)= split /\t/;
      (exists($self->{'cache'}->{$test_name})) and
	die "duplicate test in cache file";
      $self->{'cache'}->{$test_name}= $test_result;
    }
    close(CACHE);
  }
}

# -----[ cache_write ]-----------------------------------------------
sub cache_write()
{
  my ($self)= @_;

  if (open(CACHE, ">".$self->{'cache-file'})) {
    my $cache= $self->{'cache'};
    foreach my $test (keys %$cache)
      {
	my $test_result= -1;
	(exists($cache->{$test})) and
	  $test_result= $cache->{$test};
	print CACHE "$test_result\t$test\n";
      }
    close(CACHE);
  }
}

