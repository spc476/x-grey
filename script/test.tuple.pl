#!/usr/bin/perl
use strict;

my $line;
my @tuple;
$| = 1;

while($line = <STDIN>)
{
  chomp $line;
  @tuple = split / /,$line;
  print <<EOF;
request=smtpd_access_policy
sender=$tuple[1]
recipient=$tuple[2]
client_address=$tuple[0]

EOF
#  sleep 1;
}
