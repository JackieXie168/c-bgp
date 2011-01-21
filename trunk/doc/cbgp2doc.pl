# ===================================================================
# @(#)sync-cmd-nodes.pl
#
# Generate documentation nodes for C-BGP.
#
# @author Bruno Quoitin (bruno.quoitin@umons.ac.be)
# @lastdate 21/01/2011
# ===================================================================
# Must be run from directory src/doc, as follows
#
#   ../src/cbgp -e "show commands" | perl cbgp2doc.pl
#
# will produce output in src/doc/xml and src/doc/xml/nodes
#
# Can be used to check generated documentation (does it need updates ?)
#
#  ../src/cbgp -e "show commands" | perl cbgp2doc.pl --check
#
# ===================================================================
use strict;

use Getopt::Long;
use Symbol;

use constant COLOR_DEFAULT => "\033[0m";
use constant COLOR_RED => "\033[1;31m";
use constant COLOR_WHITE => "\033[37;1m";
use constant COLOR_YELLOW => "\033[1;33m";

# -----[ configuration parameters ]----------------------------------
my $cfg_prefix= "xml";
my $cfg_nodes_prefix= "$cfg_prefix/nodes";

# -----[ show_error ]------------------------------------------------
sub show_error($)
{
  my ($msg)= @_;

  print STDERR "Error: ".COLOR_RED()."$msg".COLOR_DEFAULT()."\n";
  exit(-1);
}

# -----[ show_warning ]----------------------------------------------
sub show_warning($)
{
  my ($msg)= @_;

  print STDERR "Warning: ".COLOR_YELLOW."$msg".COLOR_DEFAULT."\n";
}

# -----[ show_info ]-------------------------------------------------
sub show_info($)
{
  my ($msg)= @_;

  print STDERR "Info: ".COLOR_WHITE."$msg".COLOR_DEFAULT."\n";
}

# -----[ node_to_filename ]------------------------------------------
# ARGUMENTS:
#   node
#   file extension
#
# RETURNS:
#   file name
# -------------------------------------------------------------------
sub node_to_filename($;$)
  {
    my ($node, $ext)= @_;
    (!exists($node->{full_name})) and
      show_error("node has no full_name attribute");
    if (defined($ext)) {
      return $node->{full_name}.'.'.$ext;
    } else {
      return $node->{full_name};
    }
  }

# -----[ node_file_exists ]------------------------------------------
# Check that a node file exists.
#
# ARGUMENTS:
#   node id (complete command)
#
# RETURNS:
#   1 file exists
#   0 file does not exist
# -------------------------------------------------------------------
sub node_file_exists($)
  {
    my ($node)= @_;

    my $filename= $cfg_nodes_prefix.'/'.node_to_filename($node, 'xml');

    return ( -e "$filename" );
  }

# -----[ node_file_update ]------------------------------------------
#
# -------------------------------------------------------------------
sub node_file_update($) {
  my ($node)= @_;

  my $filename= $cfg_nodes_prefix.'/'.node_to_filename($node, 'xml');

  ( ! -e "$filename" ) and return;

  print "comparing template for node \"$_\"...\n";
}

# -----[ template_params ]-------------------------------------------
#
# -------------------------------------------------------------------
sub template_params($$) {
  my ($stream, $node)= @_;

    if (scalar(@{$node->{params}}) > 0) {
      print $stream "  <parameters>\n";
      foreach (@{$node->{params}}) {
	if (m/^\[(.+)\]$/) {
	  print $stream "    <option>\n";
	  print $stream "      <name>$1</name>\n";
	  print $stream "      <description></description>\n";
	  print $stream "    </option>\n";
	} elsif (m/^\[(.+)\]\?\((.+)\)$/) {
	  print $stream "    <vararg>\n";
	  print $stream "      <name>$1</name>\n";
	  print $stream "      <cardinality>$2</cardinality>\n";
	  print $stream "      <description></description>\n";
	  print $stream "    </vararg>\n";
	} elsif (m/^\<(.+)\>$/) {
	  print $stream "    <parameter>\n";
	  print $stream "      <name>$1</name>\n";
	  print $stream "      <description></description>\n";
	  print $stream "    </parameter>\n";
	} else {
	  show_error("unknown parameter type \"$_\"");
	}

      }
      print $stream "  </parameters>\n";
    }
}

# -----[ node_file_create_template ]---------------------------------
# Create a template for a node file.
#
# ARGUMENTS:
#   node id (complete command)
#
# RETURNS:
#   ---
# -------------------------------------------------------------------
sub node_file_create_template($$) {
  my ($node, $path)= @_;

  my $filename= $path.'/'.node_to_filename($node, 'xml');

  ( -e "$filename" ) and return;

  my $stream= gensym();
  (!open($stream, ">$filename")) and
    show_error("could not create \"$filename\": $!");
  #print $stream "<!-- This is a template. You will need to complete and rename it !-->\n";
  print $stream "<?xml version=\"1.0\"?>\n";
  print $stream "<command>\n";
  print $stream "  <name>$node->{name}</name>\n";
  print $stream "  <id>$node->{full_name}</id>\n";
  print $stream "  <context>$node->{context}</context>\n";

  template_params($stream, $node);

  print $stream "  <abstract></abstract>\n";
  print $stream "  <description></description>\n";

  if (defined($node->{father})) {
    print $stream "  <father>\n";
    print $stream "    <name>$node->{father}->{name}</name>\n";
    print $stream "    <id>$node->{father}->{full_name}</id>\n";
    print $stream "  </father>\n";
  }

  if (scalar(%{$node->{childs}} > 0)) {
    print $stream "  <childs>\n";
    foreach (sort keys %{$node->{childs}}) {
      print $stream "    <child>\n";
      print $stream "      <name>$node->{childs}->{$_}->{name}</name>\n";
      print $stream "      <id>$node->{childs}->{$_}->{full_name}</id>\n";
      print $stream "    </child>\n";
    }
    print $stream "  </childs>\n";
  }

  if (defined($node->{prev})) {
    print $stream "  <prev>\n";
    print $stream "    <name>$node->{prev}->{name}</name>\n";
    print $stream "    <id>$node->{prev}->{full_name}</id>\n";
    print $stream "  </prev>\n";
  }

  if (defined($node->{next})) {
    print $stream "  <next>\n";
    print $stream "    <name>$node->{next}->{name}</name>\n";
    print $stream "    <id>$node->{next}->{full_name}</id>\n";
    print $stream "  </next>\n";
  }

  if (exists($node->{version})) {
    print $stream "  <version></version>\n";
  }

  if (exists($node->{bugs})) {
    print $stream "  <!--<bugs>\n";
    print $stream "    <bug>\n";
    print $stream "      <id></id>\n";
    print $stream "      <date></date>\n";
    print $stream "      <status></status>\n";
    print $stream "      <description></description>\n";
    print $stream "    </bug>\n";
    print $stream "  </bugs>!-->\n";
  }

  print $stream "</command>\n";

  close($stream);

  return;
}

# -----[ node_file_create_links ]------------------------------------
# Create a links XML file for a node file.
#
# ARGUMENTS:
#   node id (complete command)
#
# RETURNS:
#   ---
# -------------------------------------------------------------------
sub node_file_create_links($)
  {
    my ($node)= @_;

    my $filename= $cfg_nodes_prefix.'/LINKS_'.node_to_filename($node, 'xml');

    print "creating links for node \"$_\"...\n";

    (!open(NODE_FILE, ">$filename")) and
      show_error("could not create \"$filename\": $!");
    #print NODE_FILE "<!-- This is a template. You will need to complete and rename it !-->\n";
    print NODE_FILE "<?xml version=\"1.0\"?>\n";
    print NODE_FILE "<command>\n";
    if (defined($node->{father})) {
      print NODE_FILE "  <father>\n";
      print NODE_FILE "    <name>$node->{father}->{name}</name>\n";
      print NODE_FILE "    <id>$node->{father}->{full_name}</id>\n";
      print NODE_FILE "  </father>\n";
    }

    if (scalar(%{$node->{childs}} > 0)) {
      print NODE_FILE "  <childs>\n";
      foreach (sort keys %{$node->{childs}}) {
	print NODE_FILE "    <child>\n";
	print NODE_FILE "      <name>$node->{childs}->{$_}->{name}</name>\n";
	print NODE_FILE "      <id>$node->{childs}->{$_}->{full_name}</id>\n";
	print NODE_FILE "    </child>\n";
      }
      print NODE_FILE "  </childs>\n";
    }

    if (defined($node->{prev})) {
      print NODE_FILE "  <prev>\n";
      print NODE_FILE "    <name>$node->{prev}->{name}</name>\n";
      print NODE_FILE "    <id>$node->{prev}->{full_name}</id>\n";
      print NODE_FILE "  </prev>\n";
    }

    if (defined($node->{next})) {
      print NODE_FILE "  <next>\n";
      print NODE_FILE "    <name>$node->{next}->{name}</name>\n";
      print NODE_FILE "    <id>$node->{next}->{full_name}</id>\n";
      print NODE_FILE "  </next>\n";
    }

    print NODE_FILE "</command>\n";

    close(NODE_FILE);

    return;
  }

# -----[ _parse_cmd_full_name ]--------------------------------------
sub _parse_cmd_full_name($$)
  {
    my ($stack_ref, $name)= @_;
    my $full= '';
    (scalar(@$stack_ref) > 0) and
      $full.= (join "_", @$stack_ref)."_";
    return $full.$name;
  }

# -----[ parse_show_commands ]---------------------------------------
# Parse the result of C-BGP's "show commands" and build the list of
# documentation nodes.
#
# ARGUMENTS:
#   input stream
#
# RETURNS:
#   nodes (hash table reference)
# -------------------------------------------------------------------
sub parse_show_commands($) {
  my ($stream)= @_;

  my %nodes;
  my @stack;
  my $line_number= 0;
  my $init_spaces= undef;
  my $cmd_current= undef;
  my $cmd_father= undef;
  while (<$stream>) {
    chomp;
    $line_number++;
    my @fields;
    my $diff_space;

    if ($_ =~ m/^(\s+)(.+)$/) {
      my $spaces= $1;
      my $raw_command= $2;
      @fields= split /\s+/, $2;
      (scalar(@fields) < 1) and next;
      (!defined($init_spaces)) and
	$init_spaces= length($spaces);
      (length($spaces) < $init_spaces) and
	show_error("invalid syntax (spaces) at line $line_number");
      $diff_space= (length($spaces)-$init_spaces)/2-scalar(@stack);
      (($diff_space > 1) || ($diff_space < -scalar(@stack))) and
	show_error("invalid syntax ($diff_space) at line $line_number");
	
    } else {
      show_error("invalid syntax at line $line_number");
    }

    if ($diff_space > 0) {
      # Create CHILD
      $cmd_father= _parse_cmd_full_name(\@stack, $cmd_current);
      push @stack, ($cmd_current);
    } elsif ($diff_space < 0) {
      # Create PARENT
      while ($diff_space++ < 0) {
	pop(@stack);
      }
      if (scalar(@stack) > 0) {
	my @tmp_stack= @stack;
	pop(@tmp_stack);
	$cmd_father= _parse_cmd_full_name(\@tmp_stack, $stack[$#stack]);
      } else {
	$cmd_father= undef;
      }
    }
    $cmd_current= shift @fields;

    my $cmd_full= _parse_cmd_full_name(\@stack, $cmd_current);
    my $cmd_params= \@fields;
    my $cmd_struct=
      {
       full_name => $cmd_full,
       context   => (join " ", @stack),
       name      => $cmd_current,
       params    => $cmd_params,
       father    => undef,
       childs    => {},
       next      => undef,
       pref      => undef,
      };
    (exists($nodes{$cmd_full})) and
      show_error("duplicate command \"$cmd_full\" at line $line_number");
    $nodes{$cmd_full}= $cmd_struct;

    if (defined($cmd_father)) {
      (!exists($nodes{$cmd_father})) and
	show_error("command's father doesn't exist \"$cmd_father\" at line $line_number");
      $cmd_struct->{father}= $nodes{$cmd_father};
      $nodes{$cmd_father}->{childs}->{$cmd_current}= $cmd_struct;
    }
  }

  foreach my $node (values %nodes) {
    if (scalar(%{$node->{childs}}) > 0) {
      my $prev= undef;
      foreach my $child_id (sort keys %{$node->{childs}}) {
	my $child= $node->{childs}->{$child_id};
	if (defined($prev)) {
	  $child->{prev}= $prev;
	  $prev->{next}= $child;
	}
	$prev= $child;
      }
    }
  }

  return \%nodes;
}

# -----[ main_run ]--------------------------------------------------
#
# -------------------------------------------------------------------
sub main_run($) {
  my ($nodes)= @_;

  show_info("Check existing nodes");
  foreach (sort keys %$nodes) {
    if (!node_file_exists($nodes->{$_})) {
      print "creating template for node \"$_\"...\n";
      node_file_create_template($nodes->{$_}, $cfg_nodes_prefix);
    }
    node_file_create_links($nodes->{$_});
  }


  show_info("Creating Index");
  my %index;
  (open(INDEX, ">$cfg_prefix/index.xml")) or
    show_error("could not create index");
  print INDEX "<?xml version=\"1.0\"?>\n";
  print INDEX "<docindex>\n";
  foreach (sort keys %$nodes) {
    #if (node_file_exists($nodes->{$_})) {
    print INDEX "  <index>\n";
    print INDEX "    <name>";
    if ($nodes->{$_}->{context} ne '') {
      print INDEX $nodes->{$_}->{context}." ";
    }
    print INDEX $nodes->{$_}->{name};
    print INDEX "</name>\n";
    print INDEX "    <id>".node_to_filename($nodes->{$_})."</id>\n";
    print INDEX "  </index>\n";
    #}
  }
  print INDEX "</docindex>\n";
  close(INDEX);

  show_info("Creating TOC");
  my %categories=
    (
     'bgp' => 'BGP-related commands',
     'net' => 'Network- and topology-related commands',
     'sim' => 'Simulation-related commands',
     '*'   => 'Miscellaneaous commands'
    );
  open(TOC, ">$cfg_prefix/toc.xml") or
    show_error("cound not create table-of-content");
  print TOC "<?xml version=\"1.0\"?>\n";
  print TOC "<toc>";
  print TOC "  <categories>\n";
  foreach (keys %categories) {
    print TOC "    <category>\n";
    print TOC "      <name>$_</name>\n";
    print TOC "      <description>$categories{$_}</description>\n";
    print TOC "      <id>$_</id>\n";
    print TOC "    </category>\n";
  }
  print TOC "  </categories>\n";
  print TOC "</toc>";
  close(TOC);
}

# -----[ main_check ]------------------------------------------------
#
# -------------------------------------------------------------------
sub main_check($) {
  my ($nodes)= @_;

  show_info("Checking documentation");
  foreach (sort keys %$nodes) {
    node_file_create_template($nodes->{$_}, "/tmp");
    my $node= $nodes->{$_};
    my $cmd= "xsltproc xml/nodes_src/extract-params.xsl /tmp/".node_to_filename($node, 'xml')." > /tmp/".node_to_filename($node, 'params');
    (system($cmd) == 0) or die "could not run \"$cmd\"";

    $cmd= "xsltproc xml/nodes_src/extract-params.xsl xml/nodes/".node_to_filename($node, 'xml')." > /tmp/".node_to_filename($node, 'params2');
    (system($cmd) == 0) or die "could not run \"$cmd\"";

    $cmd= "diff /tmp/".node_to_filename($node, 'params')." /tmp/".node_to_filename($node, 'params2');

    my $result= `$cmd`;
    if (length($result > 0)) {
      show_warning("parameters outdated in \"".node_to_filename($node)."\"");
      template_params(*STDERR, $node);
    }
  }
}

# -----[ main ]------------------------------------------------------
#
# -------------------------------------------------------------------
my %opts;
if (!GetOptions(\%opts,
		"check!")) {
  show_error("Invalid command-line option(s)");
  exit(-1);
}

show_info("Parsing C-BGP's list of commands");
my $nodes= parse_show_commands(*STDIN);
if (exists($opts{'check'})) {
  main_check($nodes);
} else {
  main_run($nodes);
}
