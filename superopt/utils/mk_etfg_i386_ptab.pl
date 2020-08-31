#!/usr/bin/perl -w
#use strict;
use warnings;
use config_host;

my $abuild_dir = "$srcdir/build/etfg_i386";
my $out_filename = "$abuild_dir/etfg-i386.ptab";
my $cmpl_insns = "$srcdir/utils/etfg-i386.cmpl_insns.ptab";
my $bconds = "$srcdir/utils/etfg-i386.bconds.ptab";
my $inverted_bconds = "$srcdir/utils/etfg-i386.inverted_bconds.ptab";
my $other = "$srcdir/utils/etfg-i386.other.ptab";
my $fcalls = "$srcdir/utils/etfg-i386.fcalls.ptab";
open(my $out_fp, ">$out_filename");
output_cmpl_insns($cmpl_insns, $out_fp);
output_bconds($bconds, $out_fp);
output_inverted_bconds($inverted_bconds, $out_fp);
output_other($other, $out_fp);
output_fcalls($fcalls, $out_fp);
close($out_fp);

sub output_cmpl_insns
{
  my %pairs = (
	  "eq", $i386_exreg_group_eflags_eq,
	  #"ne", $i386_exreg_group_eflags_ne,
	  "bvult", $i386_exreg_group_eflags_ult,
	  "bvslt", $i386_exreg_group_eflags_slt,
	  "bvugt", $i386_exreg_group_eflags_ugt,
	  "bvsgt", $i386_exreg_group_eflags_sgt,
	  "bvule", $i386_exreg_group_eflags_ule,
	  "bvsle", $i386_exreg_group_eflags_sle,
	  "bvuge", $i386_exreg_group_eflags_uge,
	  "bvsge", $i386_exreg_group_eflags_sge,
  );
  my $in_filename = shift;
  my $out_fp = shift;
  for my $opcode (keys %pairs) {
    my $i386_exreg_group_name = $pairs{$opcode};
    my %submap = ("PATTERN_OPCODE", $opcode, "PATTERN_GROUPNAME", $i386_exreg_group_name);
    append_input_after_substituting($in_filename, $out_fp, \%submap);
  }
}

sub output_bconds
{
  my %pairs = (
	  "je",  $i386_exreg_group_eflags_eq,
	  #"je", $i386_exreg_group_eflags_ne,
	  "jb",  $i386_exreg_group_eflags_ult,
	  "jl",  $i386_exreg_group_eflags_slt,
	  "ja",  $i386_exreg_group_eflags_ugt,
	  "jg",  $i386_exreg_group_eflags_sgt,
	  "jbe", $i386_exreg_group_eflags_ule,
	  "jle", $i386_exreg_group_eflags_sle,
	  "jae", $i386_exreg_group_eflags_uge,
	  "jge", $i386_exreg_group_eflags_sge,
  );
  my $in_filename = shift;
  my $out_fp = shift;
  for my $opcode (keys %pairs) {
    my $i386_exreg_group_name = $pairs{$opcode};
    my %submap = ("PATTERN_OPCODE", $opcode, "PATTERN_GROUPNAME", $i386_exreg_group_name);
    append_input_after_substituting($in_filename, $out_fp, \%submap);
  }
}

sub output_inverted_bconds
{
  my %pairs = (
	  "je",  $i386_exreg_group_eflags_eq,
	  #"je", $i386_exreg_group_eflags_ne,
	  "jb",  $i386_exreg_group_eflags_ult,
	  "jl",  $i386_exreg_group_eflags_slt,
	  "ja",  $i386_exreg_group_eflags_ugt,
	  "jg",  $i386_exreg_group_eflags_sgt,
	  "jbe", $i386_exreg_group_eflags_ule,
	  "jle", $i386_exreg_group_eflags_sle,
	  "jae", $i386_exreg_group_eflags_uge,
	  "jge", $i386_exreg_group_eflags_sge,
  );

  my %inv_pairs = (
	  "je",  $i386_exreg_group_eflags_ne,
	  #"je", $i386_exreg_group_eflags_eq,
	  "jb",  $i386_exreg_group_eflags_uge,
	  "jl",  $i386_exreg_group_eflags_sge,
	  "ja",  $i386_exreg_group_eflags_ule,
	  "jg",  $i386_exreg_group_eflags_sle,
	  "jbe", $i386_exreg_group_eflags_ugt,
	  "jle", $i386_exreg_group_eflags_sgt,
	  "jae", $i386_exreg_group_eflags_ult,
	  "jge", $i386_exreg_group_eflags_slt,
  );
  my $in_filename = shift;
  my $out_fp = shift;
  for my $opcode (keys %pairs) {
    my $i386_exreg_group_name = $pairs{$opcode};
    my $i386_exreg_group_name_inv = $inv_pairs{$opcode};
    my %submap = ("PATTERN_OPCODE", $opcode, "PATTERN_GROUPNAME", $i386_exreg_group_name, "PATTERN_INVERTED_GROUPNAME", $i386_exreg_group_name_inv);
    append_input_after_substituting($in_filename, $out_fp, \%submap);
  }
}

sub output_other
{
  my $in_filename = shift;
  my $out_fp = shift;
  my %submap; #empty substitution map
  append_input_after_substituting($in_filename, $out_fp, \%submap);
}

sub output_fcalls
{
  my $in_filename = shift;
  my $out_fp = shift;
  my %submap; #empty substitution map
  append_input_after_substituting($in_filename, $out_fp, \%submap);
}



sub append_input_after_substituting
{
  my $in_filename = shift;
  my $out_fp = shift;
  my $submap_ref = shift;
  my %submap = %{$submap_ref};

  open(my $in_fp, "<$in_filename");
  while (my $line = <$in_fp>) {
    foreach my $skey (keys %submap) {
      my $sval = $submap{$skey};
      #print "substituting $sval for $skey\n";
      #print "line was $line\n";
      $line =~ s/$skey/$sval/g;
      #print "line is $line\n";
    }
    #$line =~ s/PATTERN_OPCODE/$opcode/g;
    #$line =~ s/PATTERN_GROUPNAME/$i386_exreg_group_name/g;
    print $out_fp $line;
  }
  close($in_fp);
}

sub output_fcalls_old
{
  my $output_fp = shift;
  my $max_num_fcall_args = 4;
  for (my $num_fcall_args = 0; $num_fcall_args <= $max_num_fcall_args; $num_fcall_args++)
  {
    my @arg_mappings = @{get_all_arg_mappings($num_fcall_args)};
    foreach my $arg_mapping (@arg_mappings) {
      output_fcall_entry_for_arg_mapping($output_fp, $num_fcall_args, $arg_mapping);
    }
  }
}

sub get_all_arg_mappings
{
  my $num_args = shift;
  if ($num_args <= 0) {
    my @arr;
    return \@arr;
  } elsif ($num_args == 1) {
    my @arr;
    push(@arr, "0");
    push(@arr, "1");
    return \@arr;
  } else {
    my @sub_ret = @{get_all_arg_mappings($num_args - 1)};
    my @ret;
    foreach my $r (@sub_ret) {
      my $new_r = "0$r";
      push(@ret, $new_r);
      $new_r = "1$r";
      push(@ret, $new_r);
    }
    return \@ret;
  }
}

sub output_fcall_entry_for_arg_mapping
{
  my $out_fp = shift;
  my $num_fcall_args = shift;
  my $arg_mapping = shift;
  my $infile = "etfg-i386.fcall.template";
  open(my $in_fp, $infile);
  while (my $line = <$in_fp>) {
    if ($line =~ /PATTERN_FUNCTION_CALL_LINES_REG(\d)/) {
      my $start_argnum = $1;
      output_function_call_lines($out_fp, $num_fcall_args, $start_argnum, 0);
    } elsif ($line =~ /PATTERN_FUNCTION_CALL_LINES_MEM(\d)/) {
      my $start_argnum = $1;
      output_function_call_lines($out_fp, $num_fcall_args, $start_argnum, 1);
    } elsif ($line =~ /PATTERN_LIVE_REGS_EXCLUDING_R0/) {
      output_live_regs_line($out_fp, $num_fcall_args, 0);
    } elsif ($line =~ /PATTERN_LIVE_REGS_INCLUDING_R0/) {
      output_live_regs_line($out_fp, $num_fcall_args, 1);
    } elsif ($line =~ /PATTERN_TRANSMAP_ENTRIES(\d)/) {
      my $start_argnum = $1;
      output_transmap_entries($out_fp, $start_argnum, $num_fcall_args, $arg_mapping)
    } elsif ($line =~ /PATTERN_PUSH_ARGS_LINES/) {
      output_push_args_lines($out_fp, $num_fcall_args, $arg_mapping);
    } elsif ($line =~ /PATTERN_POP_ARGS_LINES/) {
      output_pop_args_lines($out_fp, $num_fcall_args);
    } else {
      print $out_fp $line;
    }
  }
  close($in_fp);
}

sub output_function_call_lines
{
  my $out_fp = shift;
  my $num_fcall_args = shift;
  my $start_argnum = shift;
  my $is_mem = shift;

  my $cur_argnum = $start_argnum;
  my $cur_expr_num = 7;
  for (my $i = 0; $i < $num_fcall_args; $i++) {
    print $out_fp "$cur_expr_num : input.src.exreg.0.$cur_argnum : BV:32\n";
    $cur_argnum++;
    $cur_expr_num++;
  }
  $cur_argnum = $start_argnum;
  print $out_fp "$cur_expr_num : function_call(1, 2, 3, 4, 5, 6, 4";
  for (my $i = 0; $i < $num_fcall_args; $i++) {
    print $out_fp ", $cur_argnum";
    $cur_argnum++;
  }
  print $out_fp ") : ";
  if ($is_mem == 1) {
   print $out_fp "ARRAY[BV:32 -> BV:8]\n";
  } else {
   $is_mem == 0 or die;
   print $out_fp "BV:32\n";
  }
}

sub output_transmap_entries
{
  my $out_fp = shift;
  my $start_argnum = shift;
  my $num_fcall_args = shift;
  my $arg_mapping = shift;
  die "not-implemented";
}

sub output_live_regs_line
{
  my $out_fp = shift;
  my $num_fcall_args = shift;
  my $include_r0 = shift;
  die "not-implemented";
}

sub output_push_args_lines
{
  my $out_fp = shift;
  my $num_fcall_args = shift;
  my $arg_mapping = shift;
  die "not-implemented";
}

sub output_pop_args_lines
{
  my $out_fp = shift;
  my $num_fcall_args = shift;
  die "not-implemented";
}
