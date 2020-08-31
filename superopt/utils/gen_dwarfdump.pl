#!/usr/bin/perl -w
use strict;
use warnings;
use config_host;
use Fcntl qw(SEEK_SET SEEK_CUR SEEK_END);

if ($#ARGV != 0) {
  print "Usage: gen_dwarfdump.pl <elf-filename>\n";
  exit(1);
}
our $dwarfdump;

my $exec = $ARGV[0];

system("$dwarfdump $exec > $exec.dd");
