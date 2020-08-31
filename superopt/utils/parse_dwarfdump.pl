#!/usr/bin/perl -w
use strict;
use warnings;
use config_host;
use Fcntl qw(SEEK_SET SEEK_CUR SEEK_END);

our $Union_type_None = 0;
our $Union_type_Union = 1;
our $Union_type_Structure = 2;
our $Union_type_Class = 3;

our $Pointer_type_None = 0;
our $Pointer_type_Pointer = 1;
our $Pointer_type_Reference = 2;

our $dwarfdump;

if ($#ARGV != 0) {
  print "Usage: parse_dwarfdump.pl <elf-filename>\n";
  exit(1);
}

my $exec = $ARGV[0];
my $inexec = $exec;

#XXX: these are hacks.
#$inexec =~ s/ccomp/gcc48/g; #replace ccomp with gcc48; ccomp does not emit useful dwarf headers
#$inexec =~ s/clang36/gcc48/g; #replace all execs with gcc48-O2, to ensure uniformity
#$inexec =~ s/-O0-/-O2-/g; #replace all execs with gcc48-O2, to ensure uniformity

my $out = "$exec.ddsum";
my %types;

#system("$dwarfdump $inexec > $exec.dd");
open my($fh), "<$exec.dd" or die "Could not open $exec.dd: $!\n";
open my($ofh), ">$out" or die "Could not open $out: $!\n";

my $changed = 1;

while ($changed) {
  $changed = 0;
  $changed = init_base_types(\%types, $fh) || $changed;
  $changed = init_enumeration_types(\%types, $fh) || $changed;
  $changed = init_subroutine_types(\%types, $fh) || $changed;
  $changed = init_pointer_types(\%types, $fh) || $changed;
  $changed = init_typedefs(\%types, $fh) || $changed;
  $changed = init_array_types(\%types, $fh) || $changed;
  $changed = init_union_types(\%types, $fh) || $changed;
  #print "After full iteration: changed=$changed\n";
}

open my($oth), ">$exec.ddtypes" or die "Could not open $exec.ddtypes: $!\n";
print_types(\%types, $oth);
close $oth;

get_fun_type_info($ofh, $fh, \%types);
  
close($fh);
close($ofh);

sub clear_context_stack_till {
  my $context_stack_ref = shift;
  my $depth = shift;

  my $ret = 0;
  while (scalar(@$context_stack_ref) > $depth) {
    pop(@$context_stack_ref);
    $ret = 1;
  }
  my $len = scalar(@$context_stack_ref);
  #print "after clear_context_stack_till $depth: $len: @$context_stack_ref\n";
  $len <= $depth or die;
  return $ret;
}

sub update_context_stack {
#update_context_stack returns true if the new line pops (and potentially pushes) anything from the current context stack
  my $line = shift;
  my $context_stack_ref = shift;

  my ($is_new_compile_unit, $compile_unit_offset) = check_new_compile_unit($line);
  if ($is_new_compile_unit) {
    #$cur_compile_unit = $compile_unit_offset;
    #pop(@$context_stack_ref);
    my $ret;
    $ret = clear_context_stack_till($context_stack_ref, 0);
    push(@$context_stack_ref, $compile_unit_offset);
    scalar(@$context_stack_ref) > 0 or die;
    return $ret;
  }

  if ($line =~ /^\.debug_line/) {
    my $ret = clear_context_stack_till($context_stack_ref, 1);
    scalar(@$context_stack_ref) > 0 or die;
    return $ret;
  }

  if ($line =~ /^<\s*(\d+)>.*DW_TAG/) {
    my $tag_depth = $1;
    my $ret = 0;
    if ($tag_depth == 0) {
      $tag_depth = 1;
    }
    $ret = clear_context_stack_till($context_stack_ref, $tag_depth);
    scalar(@$context_stack_ref) > 0 or die;
    return $ret;
  }
  return 0;
}

sub push_context_stack {
  my $context_stack_ref = shift;
  my $depth = shift;

  $depth >= scalar(@$context_stack_ref) or die;
  my $i = scalar(@$context_stack_ref);
  while (scalar(@$context_stack_ref) <= $depth) {
    push(@$context_stack_ref, "$i");
    $i++;
  }
}

sub check_new_compile_unit {
  my $line = shift;
  if ($line =~ /COMPILE_UNIT.*offset = (.*)>/) {
    return (1, $1);
  }
  return (0,0x0);
}

sub get_fun_type_info {
  my $ofh = shift;
  my $fh = shift;
  my $types_ref = shift;
  my %types = %$types_ref;

  my ($subprogram_context, $subprogram_name, $subprogram_type);
  my ($subprogram_vars);
  my (@subprogram_var_names, @subprogram_var_types);
  my ($subprogram_params);
  my (@subprogram_param_names, @subprogram_param_types);
  my ($subprogram_param_context, $subprogram_var_context);
  my $cur_compile_unit = "COMPILE_UNIT_UNDEF";
  my @context_stack;
  push(@context_stack, $cur_compile_unit);
  my $fnum = 0;
  $subprogram_context = 0;
  $subprogram_param_context = 0;
  $subprogram_var_context = 0;
  seek($fh, 0, SEEK_SET) or die "seek failed while getting subprogram info: $!\n";
  while (my $line = <$fh>) {
    #my ($is_new_compile_unit, $compile_unit_offset) = check_new_compile_unit($line);
    #if ($is_new_compile_unit) {
    #  #$cur_compile_unit = $compile_unit_offset;
    #  pop(@context_stack);
    #  push(@context_stack, $compile_unit_offset);
    #  next;
    #}
    #print "line = $line\n";
    if (update_context_stack($line, \@context_stack)) {
      my $context_stack_str = context_stack_to_string(@context_stack);
      if ($subprogram_context && (defined $subprogram_name)) {
        if (defined $subprogram_type) {
          #print "1. subprogram_type $subprogram_type\n";
          my $subprogram_typesize = get_type_size(\%types, $context_stack_str, $subprogram_type);
          my $subprogram_typename = get_type_name(\%types, $context_stack_str, $subprogram_type);
          $subprogram_typesize >= 0 or die "Could not get size for type $context_stack_str.$subprogram_type proc $subprogram_name\n";
          print $ofh "FUNCTION$fnum: ";
          print $ofh "$subprogram_name $subprogram_typesize \"$subprogram_typename\" :";
        } else {
          print $ofh "FUNCTION$fnum: ";
          print $ofh "$subprogram_name 0 \"void\" :";
        }
        $fnum++;
        for (my $i = 0; $i <= $subprogram_params; $i++) {
          #print "2. subprogram_param_type $subprogram_param_types[$i]\n";
          my $param_typesize = get_type_size(\%types, $context_stack_str, $subprogram_param_types[$i]);
          my $param_typename = get_type_name(\%types, $context_stack_str, $subprogram_param_types[$i]);
          defined $param_typesize or die "param_typesize not defined\n";
          defined $param_typename or die "param_typename not defined\n";
          if (!(defined $subprogram_param_names[$i])) {
            $subprogram_param_names[$i] = "noname";
          }
          print $ofh " { $param_typesize \"$param_typename\" \"$subprogram_param_names[$i]\" }";
        }
        print $ofh " :";
        for (my $i = 0; $i <= $subprogram_vars; $i++) {
          #print "3. subprogram_param_type $subprogram_var_types[$i]\n";
          my $var_typesize = get_type_size(\%types, $context_stack_str, $subprogram_var_types[$i]);
          my $var_typename = get_type_name(\%types, $context_stack_str, $subprogram_var_types[$i]);
          print $ofh " { $var_typesize \"$var_typename\" \"$subprogram_var_names[$i]\" }";
        }
        print $ofh "\n";
      }
      undef $subprogram_name;
      undef $subprogram_type;
      $subprogram_context = 0;
    }

    if (   $subprogram_context == 0
        && $line =~ /^<\s*(\d+)>.*DW_TAG_subprogram/) {
      my $tag_depth = $1;
      #print "subprogram calling push_context_stack\n";
      push_context_stack(\@context_stack, $tag_depth);
      $subprogram_context = 1;
      $subprogram_params = -1;
      $subprogram_vars = -1;
      $subprogram_var_context = 0;
      $subprogram_param_context = 0;
    } elsif (   $subprogram_context
             && $subprogram_params == -1
             && $subprogram_vars == -1
             && $line =~ /DW_AT_name\s+(\S+)$/) {
      if (!(defined $subprogram_name)) {
        $subprogram_name = $1;
      }
      #print "subprogram_name = $subprogram_name\n";
    } elsif (   $subprogram_context
             && $subprogram_params == -1
             && $subprogram_vars == -1
             && (   $line =~ /DW_AT_linkage_name\s+(\S+)$/
                 || $line =~ /DW_AT_MIPS_linkage_name\s+(\S+)$/)) {
      $subprogram_name = $1;
      #print "subprogram_name = $subprogram_name\n";
    } elsif (   $subprogram_context
             && $subprogram_params == -1
             && $subprogram_vars == -1
             && $line =~ /DW_AT_type\s+<(\S+)>$/) {
      $subprogram_type = $1;
      #print "subprogram_type = $subprogram_type\n";
    } elsif (   $subprogram_context
             && $line =~ /< 2>.*DW_TAG_formal_parameter/) {
      $subprogram_params++;
      $subprogram_param_context = 1;
      $subprogram_var_context = 0;
      #print "found param $subprogram_params\n";
    } elsif (   $subprogram_param_context
             && $subprogram_params >= 0
             && $line =~ /DW_AT_type\s+<(\S+)>$/) {
      $subprogram_param_types[$subprogram_params] = $1;
      #print "found param type $subprogram_param_types[$subprogram_params]\n";
    } elsif (   $subprogram_param_context
             && $subprogram_params >= 0
             && $line =~ /DW_AT_name\s+(\S+)$/) {
      $subprogram_param_names[$subprogram_params] = $1;
      #print "found param name $subprogram_param_names[$subprogram_params]\n";
    } elsif (   $subprogram_context
             && $line =~ /DW_TAG_variable/) {
      $subprogram_vars++;
      $subprogram_var_context = 1;
      $subprogram_param_context = 0;
      #print "found var $subprogram_vars\n";
    } elsif (   $subprogram_var_context
             && $line =~ /< 2>.*DW_TAG/) {
      #print "found end of subcontext (vars,params)\n";
      $subprogram_var_context = 0;
      $subprogram_param_context = 0;
    } elsif (   $subprogram_var_context
             && $line =~ /DW_AT_type\s+<(\S+)>$/) {
      $subprogram_var_types[$subprogram_vars] = $1;
      #print "found var type $subprogram_var_types[$subprogram_vars]\n";
    } elsif (   $subprogram_var_context
             && $line =~ /DW_AT_name\s+(\S+)$/) {
      $subprogram_var_names[$subprogram_vars] = $1;
      #print "found var name $subprogram_var_names[$subprogram_vars]\n";
    }
  }
}

sub init_base_types {
  my $types = shift;
  my $fh = shift;
  my @context_stack;
  my ($base_type_context, $base_type_name, $base_type_size);
  my $cur_compile_unit = "COMPILE_UNIT_UNDEF";
  push(@context_stack, $cur_compile_unit);
  my $changed = 0;
  undef $base_type_context;
  seek($fh, 0, SEEK_SET) or die "seek failed inside init_base_types: $!\n";
  while (my $line = <$fh>) {
    #my ($is_new_compile_unit, $compile_unit_offset) = check_new_compile_unit($line);
    #if ($is_new_compile_unit) {
    #  #$cur_compile_unit = $compile_unit_offset;
    #  pop(@context_stack);
    #  push(@context_stack, $compile_unit_offset);
    #  next;
    #}
    if (update_context_stack($line, \@context_stack)) {
      if ((defined $base_type_context) && (defined $base_type_name)) {
        if (   defined($base_type_size)
            && !defined($$types{$base_type_context})) {
          my @typeval = ($base_type_name, $base_type_size);
          #print "adding $base_type_context base $base_type_name\n";
          $$types{$base_type_context} = \@typeval;
          $changed = 1;
        }
      }
      undef $base_type_name;
      undef $base_type_size;
      undef $base_type_context;
    }

    if (   !(defined $base_type_context)
        && $line =~ /^<\s*(\d+)>.*<([^>]*)>\s+DW_TAG_base_type/) {
      my $tag_depth = $1;
      my $tag_id = $2;
      my $context_stack_str = context_stack_to_string(@context_stack);
      #print "base calling push_context_stack\n";
      push_context_stack(\@context_stack, $tag_depth);
      $base_type_context = "$context_stack_str.$tag_id";
      #print "found $base_type_context base_type\n";
    } elsif (   (defined $base_type_context)
             && $line =~ /DW_AT_name\s+(.+)$/) {
      $base_type_name = $1;
    } elsif (   (defined $base_type_context)
             && $line =~ /DW_AT_byte_size\s+(\S+)$/) {
      $base_type_size = (hex($1) * 8);
    } elsif (   (defined $base_type_context)
             && $line =~ /DW_AT_bit_size\s+(\S+)$/) {
      $base_type_size = (hex($1));

    }
  }
  return $changed;
}

sub init_enumeration_types {
  my $types = shift;
  my $fh = shift;
  my ($enum_type_context, $enum_type_size);
  my $cur_compile_unit = "COMPILE_UNIT_UNDEF";
  my @context_stack;
  push(@context_stack, $cur_compile_unit);
  my $changed = 0;
  undef $enum_type_context;
  seek($fh, 0, SEEK_SET) or die "seek failed inside init_enum_types: $!\n";
  while (my $line = <$fh>) {
    #my ($is_new_compile_unit, $compile_unit_offset) = check_new_compile_unit($line);
    #if ($is_new_compile_unit) {
    #  #$cur_compile_unit = $compile_unit_offset;
    #  pop(@context_stack);
    #  push(@context_stack, $compile_unit_offset);
    #  next;
    #}
    if (update_context_stack($line, \@context_stack)) {
      if (defined $enum_type_context) {
        if (   defined($enum_type_size)
            && !defined($$types{$enum_type_context})) {
          my @typeval = ($enum_type_context, $enum_type_size);
          #print "adding $enum_type_context enum $enum_type_name\n";
          $$types{$enum_type_context} = \@typeval;
          $changed = 1;
        }
      }
      #undef $enum_type_name;
      undef $enum_type_size;
      undef $enum_type_context;
    }

    if (   !(defined $enum_type_context)
        && $line =~ /^<\s*(\d+)>.*<([^>]*)>\s+DW_TAG_enumeration_type/) {
      my $tag_depth = $1;
      my $tag_id = $2;
      #$enum_type_context = "$cur_compile_unit.$1";
      my $context_stack_str = context_stack_to_string(@context_stack);
      #print "enum calling push_context_stack\n";
      push_context_stack(\@context_stack, $tag_depth);
      $enum_type_context = "$context_stack_str.$tag_id";
      #print "found $enum_type_context enum_type\n";
    #} elsif (   (defined $enum_type_context)
    #         && $line =~ /DW_AT_name\s+(.+)$/) {
    #  #$enum_type_name = $1;
    } elsif (   (defined $enum_type_context)
             && $line =~ /DW_AT_byte_size\s+(\S+)$/) {
      $enum_type_size = (hex($1) * 8);
    } elsif (   (defined $enum_type_context)
             && $line =~ /DW_AT_bit_size\s+(\S+)$/) {
      $enum_type_size = (hex($1));

    }
  }
  return $changed;
}

sub init_subroutine_types {
  my $types = shift;
  my $fh = shift;
  my ($subr_type_context);
  my $cur_compile_unit = "COMPILE_UNIT_UNDEF";
  my @context_stack;
  push(@context_stack, $cur_compile_unit);
  my $changed = 0;
  undef $subr_type_context;
  seek($fh, 0, SEEK_SET) or die "seek failed inside init_subr_types: $!\n";
  while (my $line = <$fh>) {
    #my ($is_new_compile_unit, $compile_unit_offset) = check_new_compile_unit($line);
    #if ($is_new_compile_unit) {
    #  #$cur_compile_unit = $compile_unit_offset;
    #  pop(@context_stack);
    #  push(@context_stack, $compile_unit_offset);
    #  next;
    #}
    if (update_context_stack($line, \@context_stack)) {
      if (defined $subr_type_context) {
        if (!defined($$types{$subr_type_context})) {
          my @typeval = ($subr_type_context, 0);
          #print "adding $subr_type_context subr $subr_type_name\n";
          $$types{$subr_type_context} = \@typeval;
          $changed = 1;
        }
      }
      #undef $subr_type_name;
      #undef $subr_type_size;
      undef $subr_type_context;
    }
    if (   !(defined $subr_type_context)
        && $line =~ /^<\s*(\d+)>.*<([^>]*)>\s+DW_TAG_subroutine_type/) {
      my $tag_depth = $1;
      my $tag_id = $2;
      #$subr_type_context = "$cur_compile_unit.$1";
      my $context_stack_str = context_stack_to_string(@context_stack);
      #print "subr calling push_context_stack\n";
      push_context_stack(\@context_stack, $tag_depth);
      $subr_type_context = "$context_stack_str.$tag_id";
      #print "found $subr_type_context subr_type\n";
    #} elsif (   (defined $subr_type_context)
    #         && $line =~ /DW_AT_byte_size\s+(\S+)$/) {
    #  $subr_type_size = (hex($1) * 8);
    #} elsif (   (defined $subr_type_context)
    #         && $line =~ /DW_AT_bit_size\s+(\S+)$/) {
    #  $subr_type_size = (hex($1));
    }
  }
  return $changed;
}



sub init_pointer_types {
  my $types = shift;
  my $fh = shift;
  my ($pointer_type_context, $pointer_type_pointed_to_type, $pointer_type_size);
  my $is_pointer = $Pointer_type_None;
  my $changed = 0;
  my $default_pointer_type_size = 32;
  my $cur_compile_unit = "COMPILE_UNIT_UNDEF";
  my @context_stack;
  push(@context_stack, $cur_compile_unit);
  undef $pointer_type_context;
  seek($fh, 0, SEEK_SET) or die "seek failed inside init_pointer_types: $!\n";
  while (my $line = <$fh>) {
    #my ($is_new_compile_unit, $compile_unit_offset) = check_new_compile_unit($line);
    #if ($is_new_compile_unit) {
    #  #$cur_compile_unit = $compile_unit_offset;
    #  pop(@context_stack);
    #  push(@context_stack, $compile_unit_offset);
    #  next;
    #}

    if (update_context_stack($line, \@context_stack)) {
      if (defined $pointer_type_context) {
        if (!(defined $pointer_type_size)) {
          $pointer_type_size = $default_pointer_type_size;
        }
        my $pointer_type_pointed_to_typename;
        if (!(defined $pointer_type_pointed_to_type)) {
          $pointer_type_pointed_to_typename = "void";
        } else {
          $pointer_type_pointed_to_type =~ /<(.*)>/ or die;
          #my $type_id = "$cur_compile_unit.$1";
          my $context_stack_str = context_stack_to_string(@context_stack);
          my $type_id = "$context_stack_str.$1";
          #print "$pointer_type_context pointer points to type_id $type_id\n";
          if (defined $$types{$type_id}) {
            my $pointed_to_typeval = $$types{$type_id};
            $pointer_type_pointed_to_typename = $$pointed_to_typeval[0];
            #print "pointer_type_pointed_to_typename $pointer_type_pointed_to_typename\n";
          } else {
            #print "types for $type_id not defined\n";
            $pointer_type_pointed_to_typename = "unknown";
          }
          #$pointer_type_pointed_to_typename = $pointer_type_pointed_to_type; #XXX: this should be replaced by commented out code before this, after adding support for struct types
        }
        if (defined $pointer_type_pointed_to_typename) {
          my $pname;
          if ($is_pointer == $Pointer_type_Pointer) {
            $pname = "$pointer_type_pointed_to_typename *";
          } elsif ($is_pointer == $Pointer_type_Reference) {
            $pname = "$pointer_type_pointed_to_typename &";
          } else {
            die "not-reached";
          }
          my @typeval = ($pname, $pointer_type_size);
          if (!(defined($$types{$pointer_type_context}))) {
            $$types{$pointer_type_context} = \@typeval;
            #print "adding $pointer_type_context pointer $pname\n";
            $changed = 1;
          } else {
            my @tmp = @$types{$pointer_type_context};
            #print "$pointer_type_context already assigned to @tmp\n";
          }
        }
      }
      undef $pointer_type_pointed_to_type;
      undef $pointer_type_size;
      undef $pointer_type_context;
    }

    my $context_stack_str = context_stack_to_string(@context_stack);
    if (   !(defined $pointer_type_context)
        && $line =~ /^<\s*(\d+)>.*<([^>]*)>\s+DW_TAG_pointer_type/) {
      my $tag_depth = $1;
      my $tag_id = $2;
      #print "pointer calling push_context_stack\n";
      push_context_stack(\@context_stack, $tag_depth);
      $pointer_type_context = "$context_stack_str.$tag_id";
      $is_pointer = $Pointer_type_Pointer;
      #print "found $pointer_type_context pointer_type\n";
    } elsif (   !(defined $pointer_type_context)
             && $line =~ /^<\s*(\d+)>.*<([^>]*)>\s+DW_TAG_reference_type/) {
      my $tag_depth = $1;
      my $tag_id = $2;
      #print "reference calling push_context_stack\n";
      push_context_stack(\@context_stack, $tag_depth);
      $pointer_type_context = "$context_stack_str.$tag_id";
      $is_pointer = $Pointer_type_Reference;
    } elsif (   (defined $pointer_type_context)
             && $line =~ /DW_AT_type\s+(.+)$/) {
      $pointer_type_pointed_to_type = $1;
    } elsif (   (defined $pointer_type_context)
             && $line =~ /DW_AT_byte_size\s+(\S+)$/) {
      $pointer_type_size = (hex($1) * 8);
      #print "pointer_type_size = $pointer_type_size\n";
    } elsif (   (defined $pointer_type_context)
             && $line =~ /DW_AT_bit_size\s+(\S+)$/) {
      $pointer_type_size = (hex($1));

    }
  }
  return $changed;
}

sub init_typedefs {
  my $types = shift;
  my $fh = shift;
  my ($typedefs_context, $typedefs_name, $typedefs_type);
  my $changed = 0;
  my $is_const = 0;
  my $cur_compile_unit = "COMPILE_UNIT_UNDEF";
  my @context_stack;
  push(@context_stack, $cur_compile_unit);
  undef $typedefs_context;
  #print "init_typedefs\n";
  seek($fh, 0, SEEK_SET) or die "seek failed inside init_typedefss: $!\n";
  while (my $line = <$fh>) {
    #my ($is_new_compile_unit, $compile_unit_offset) = check_new_compile_unit($line);
    #if ($is_new_compile_unit) {
    #  #$cur_compile_unit = $compile_unit_offset;
    #  pop(@context_stack);
    #  push(@context_stack, $compile_unit_offset);
    #  next;
    #}
    if (update_context_stack($line, \@context_stack)) {
      if (defined $typedefs_context) {
        if (defined $typedefs_type) {
          #print "3. typedefs_type $cur_compile_unit.$typedefs_type\n";
          #my $type_size = get_type_size(\%types, $cur_compile_unit, $typedefs_type);
          my $context_stack_str = context_stack_to_string(@context_stack);
          my $type_size = get_type_size(\%types, $context_stack_str, $typedefs_type);
          if (   !(defined $typedefs_name)
              && $is_const == 1) {
            #$typedefs_name = get_type_name(\%types, $cur_compile_unit, $typedefs_type);
            $typedefs_name = get_type_name(\%types, $context_stack_str, $typedefs_type);
            $typedefs_name = "$typedefs_name const";
          }
          if ($type_size >= 0) {
            my @typeval = ($typedefs_name, $type_size);
            if (!(defined $$types{$typedefs_context})) {
              #print "adding $typedefs_context typedefs $typedefs_name\n";
              $$types{$typedefs_context} = \@typeval;
              $changed = 1;
            }
          } else {
            #print "type_size = $type_size\n";
          }
        }
      }
      undef $typedefs_name;
      undef $typedefs_type;
      undef $typedefs_context;
    }
    my $context_stack_str = context_stack_to_string(@context_stack);
    if (   !(defined $typedefs_context)
        && $line =~ /^<\s*(\d+)>.*<([^>]*)>\s+DW_TAG_typedef/) {
      my $tag_depth = $1;
      my $tag_id = $2;
      #print "typedef calling push_context_stack\n";
      push_context_stack(\@context_stack, $tag_depth);
      $typedefs_context = "$context_stack_str.$tag_id";
      $is_const = 0;
      #print "found $typedefs_context typedefs\n";
    } elsif (   !(defined $typedefs_context)
             && $line =~ /^<\s*(\d+)>.*<([^>]*)>\s+DW_TAG_const_type/) {
      my $tag_depth = $1;
      my $tag_id = $2;
      #print "const_type calling push_context_stack\n";
      push_context_stack(\@context_stack, $tag_depth);
      $typedefs_context = "$context_stack_str.$tag_id";
      #print "found $typedefs_context const\n";
      $is_const = 1;
    } elsif (   (defined $typedefs_context)
             && $line =~ /DW_AT_name\s+(.+)$/) {
      $typedefs_name = $1;
      #print "typedefs_context=$typedefs_context, typedefs_name=$typedefs_name\n";
    } elsif (   (defined $typedefs_context)
             && $line =~ /DW_AT_type\s+<(\S+)>$/) {
      $typedefs_type = $1;
      #print "typedefs_context=$typedefs_context, typedefs_type=$typedefs_type\n";
    }
  }
  #print "init_typedefs done. changed $changed\n";
  return $changed;
}

sub init_array_types {
  my $types = shift;
  my $fh = shift;
  my ($array_type_context, $array_elem_type, @array_upper_bound);
  my $array_subrange_context = 0;
  my $changed = 0;
  my $cur_compile_unit = "COMPILE_UNIT_UNDEF";
  my @context_stack;
  push(@context_stack, $cur_compile_unit);
  undef $array_type_context;
  #print "init_array_type\n";
  seek($fh, 0, SEEK_SET) or die "seek failed inside init_array_type: $!\n";
  while (my $line = <$fh>) {
    #my ($is_new_compile_unit, $compile_unit_offset) = check_new_compile_unit($line);
    #if ($is_new_compile_unit) {
    #  #$cur_compile_unit = $compile_unit_offset;
    #  pop(@context_stack);
    #  push(@context_stack, $compile_unit_offset);
    #  next;
    #}
    if (update_context_stack($line, \@context_stack)) {
      my $context_stack_str = context_stack_to_string(@context_stack);
      if (   (defined $array_type_context)
          && (defined $array_elem_type)
          && (scalar @array_upper_bound)) {
	#print "exiting array type context\n";
        #print " array_elem_type $array_elem_type\n";
	#my $array_elem_type_size = get_type_size(\%types, $cur_compile_unit, $array_elem_type);
	my $array_elem_type_size = get_type_size(\%types, $context_stack_str, $array_elem_type);
        #print "array_elem_type_size $array_elem_type_size\n";
        if ($array_elem_type_size >= 0 && !(defined($$types{$array_type_context}))) {
	  #my $array_elem_type_name = get_type_name(\%types, $cur_compile_unit, $array_elem_type);
	  my $array_elem_type_name = get_type_name(\%types, $context_stack_str, $array_elem_type);
          my $array_num_elems_str = "";
          my $array_num_elems = 1;
          foreach my $u (@array_upper_bound) {
            my $n = $u + 1;
            $array_num_elems_str .= "[$n]";
            $array_num_elems *= $n;
          }
          my $aname = "$array_elem_type_name$array_num_elems_str";
	  my @typeval = ($aname, $array_num_elems * $array_elem_type_size);
          #print "adding $array_type_context array $aname\n";
	  $$types{$array_type_context} = \@typeval;
          $changed = 1;
        }
      }
      undef $array_elem_type;
      @array_upper_bound = ();
      undef $array_type_context;
      $array_subrange_context = 0;
    }

    if (   !(defined $array_type_context)
        && $line =~ /^<\s*(\d+)>.*<([^>]*)>\s+DW_TAG_array_type/) {
      my $tag_depth = $1;
      my $tag_id = $2;
      #print "array_type calling push_context_stack\n";
      push_context_stack(\@context_stack, $tag_depth);
      #$array_type_context = "$cur_compile_unit.$1";
      my $context_stack_str = context_stack_to_string(@context_stack);
      $array_type_context = "$context_stack_str.$tag_id";
      #print "found array type context $array_type_context\n";
    } elsif (   (defined $array_type_context)
	     && $line =~ /DW_AT_type\s+<(.+)>$/) {
      if ($array_subrange_context == 0) {
        $array_elem_type = "$1";
	#print "found array elem type $array_elem_type\n";
      }
    } elsif (   (defined $array_type_context)
	     && $line =~ /DW_TAG_subrange_type$/) {
      $array_subrange_context++;
      #print "entered subrange context\n";
    } elsif (   (defined $array_type_context)
             && ($array_subrange_context > 0)
	     && $line =~ /DW_AT_upper_bound\s+(\d+)/) {
      scalar(@array_upper_bound) == ($array_subrange_context - 1) or die "Inconsistency found.\n";
      @array_upper_bound = (@array_upper_bound, $1);
    }
  }
  #print "init_array_type done\n";
  return $changed;
}

sub init_union_types {
  my $types = shift;
  my $fh = shift;
  my ($union_type_context, $union_type_name, $union_type_size);
  my $changed = 0;
  my $is_union = $Union_type_None;
  my $cur_compile_unit = "COMPILE_UNIT_UNDEF";
  my @context_stack;
  push(@context_stack, $cur_compile_unit);
  undef $union_type_context;
  my $default_union_type_size = 0; #useful for declarations only (no definitions)
  seek($fh, 0, SEEK_SET) or die "seek failed inside init_union_types: $!\n";
  while (my $line = <$fh>) {
    #my ($is_new_compile_unit, $compile_unit_offset) = check_new_compile_unit($line);
    #if ($is_new_compile_unit) {
    #  #$cur_compile_unit = $compile_unit_offset;
    #  pop(@context_stack);
    #  push(@context_stack, $compile_unit_offset);
    #  next;
    #}
    if (update_context_stack($line, \@context_stack)) {
      if (   (defined $union_type_context)
          && (defined $union_type_name)) {
        if (!(defined $union_type_size)) {
          $union_type_size = $default_union_type_size;
        }
        if (!defined($$types{$union_type_context})) {
          my @typeval = ("$union_type_name}", $union_type_size);
          #print "adding $union_type_context union_type_name $union_type_name\n";
          $$types{$union_type_context} = \@typeval;
          $changed = 1;
        }
      }
      undef $union_type_name;
      undef $union_type_size;
      undef $union_type_context;
    }
    my $context_stack_str = context_stack_to_string(@context_stack);
    if (   !(defined $union_type_context)
        && $line =~ /^<\s*(\d+)>.*<([^>]*)>\s+DW_TAG_union_type/) {
      my $tag_depth = $1;
      my $tag_id = $2;
      #print "union_type calling push_context_stack\n";
      push_context_stack(\@context_stack, $tag_depth);
      $union_type_context = "$context_stack_str.$tag_id";
      $is_union = $Union_type_Union;
      #print "found $union_type_context union_type\n";
    } elsif (   !(defined $union_type_context)
             && $line =~ /^<\s*(\d+)>.*<([^>]*)>\s+DW_TAG_structure_type/) {
      my $tag_depth = $1;
      my $tag_id = $2;
      #print "struct_type calling push_context_stack\n";
      push_context_stack(\@context_stack, $tag_depth);
      $union_type_context = "$context_stack_str.$tag_id";
      #print "found structure type $union_type_context\n";
      $is_union = $Union_type_Structure;
    } elsif (   !(defined $union_type_context)
             && $line =~ /^<\s*(\d+)>.*<([^>]*)>\s+DW_TAG_class_type/) {
      my $tag_depth = $1;
      my $tag_id = $2;
      #print "class_type calling push_context_stack\n";
      push_context_stack(\@context_stack, $tag_depth);
      $union_type_context = "$context_stack_str.$tag_id";
      $is_union = $Union_type_Class;
    } elsif (   (defined $union_type_context)
             && $line =~ /DW_AT_name\s+(.+)$/) {
      if (defined $union_type_name) {
        $union_type_name = "$union_type_name, $1";
      } else {
        if ($is_union == $Union_type_Union) {
          $union_type_name = "union { $1";
        } elsif ($is_union == $Union_type_Structure) {
          $union_type_name = "struct { $1";
        } elsif ($is_union == $Union_type_Class) {
          $union_type_name = "class { $1";
        } else {
          die "not-reached";
        }
      } 
      #print "union_type_name $union_type_name\n";
    } elsif (   (defined $union_type_context)
             && (defined $union_type_name)
             && $line =~ /DW_AT_type\s+(.+)$/) {
      my $type_id = $1;
      #my $type_name = get_type_name($types, $cur_compile_unit, $type_id);
      my $context_stack_str = context_stack_to_string(@context_stack);
      my $type_name = get_type_name($types, $context_stack_str, $type_id);
      if ($type_name ne "") {
        $union_type_name = "$union_type_name:$type_name";
        #print "union_type_name $union_type_name\n";
      } else {
        #print "Warning: could not get type_name for $type_id\n";
      }
    } elsif (   (defined $union_type_context)
             && $line =~ /DW_AT_byte_size\s+(\S+)$/) {
      $union_type_size = (hex($1) * 8);
    } elsif (   (defined $union_type_context)
             && $line =~ /DW_AT_bit_size\s+(\S+)$/) {
      $union_type_size = (hex($1));
    }
  }
  return $changed;
}

sub get_type_name {
  my $types = shift;
  my $context_stack_str = shift;
  my $type_id = shift;

  my $full_type_id = "$context_stack_str.$type_id";
  if (!defined($$types{$full_type_id})) {
    return "";
    #die "fatal: type $full_type_id not found.\n";
  }
  my $typeval = $$types{$full_type_id};
  my $typename = $$typeval[0];
  my $typesize = $$typeval[1];
  #print "typename $typename\n";
  #print "typesize $typesize\n";
  return $typename;
}

sub get_type_size {
  my $types = shift;
  my $context_stack_str = shift;
  my $type_id = shift;

  #print "context_stack_str = $context_stack_str.\n";
  #print "type_id = $type_id.\n";
  my $full_type_id = "$context_stack_str.$type_id";
  #print "querying type size for $full_type_id\n";
  if (!defined($$types{$full_type_id})) {
    return -1;
    #die "fatal: type $full_type_id not found.\n";
  }
  my $typeval = $$types{$full_type_id};
  my $typename = $$typeval[0];
  my $typesize = $$typeval[1];
  #print "typename $typename\n";
  #print "typesize $typesize\n";
  return $typesize;
}

sub print_types {
  my $types = shift;
  my $fh = shift;
  foreach my $key (keys %$types) {
    my $name = ${$$types{$key}}[0];
    my $size = ${$$types{$key}}[1];
    print $fh "$key : $size : $name\n";
  }
}

sub context_stack_to_string {
  my @arr = shift;
  return $arr[0];
}
