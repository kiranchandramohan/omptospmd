#!/usr/bin/perl
use Getopt::Long;

my $benchmark ;
GetOptions ("bench=s"  => \$benchmark) or die("Error in command line arguments\n");
die "Specify atleast option : (bench=<value>)" unless $benchmark ;

print "benchmark = $benchmark\n" ;

my $appl ;
if($benchmark eq "mxm") {
	$appl = "matmul" ;
} elsif($benchmark eq "dotp") {
	$appl = "dotproduct" ;
} elsif($benchmark eq "edge") {
	$appl = "edgedetect" ;
} elsif($benchmark eq "hist") {
	$appl = "histo" ;
} elsif($benchmark eq "dgen") {
	$appl = "doitgen" ;
} elsif($benchmark eq "regd") {
	$appl = "regdetect" ;
} elsif($benchmark eq "flwl") {
	$appl = "floydwarshall" ;
} else {
	printf "$benchmark : Currently not supported\n" ;
}

my $panda_ip = "129.215.91.125" ;
my $board_home = "/home/kiran/automatic/" ;
my $sysbios_home = "/home/kiran/Downloads/smpbios/automatic/" ;
my $sysbios_location = $sysbios_home."sysbios-rpmsg_".$appl ;
my $a9_location = $board_home."phd_project_".$appl ;
my $m3_file = $sysbios_location."/test_m3.c" ;
my $dsp_file = $sysbios_location."/test_dsp.c" ;
my $a9_file = $a9_location."/test_a9.c" ;

system("make $benchmark") ;

system("rm -f $m3_file") ;
system("cp output/test_m3.c $m3_file") ;
system("rm -f $dsp_file") ;
system("cp output/test_dsp.c $dsp_file") ;
system("ssh $panda_ip rm -f $a9_file") ;
system("scp output/test_a9.c $panda_ip:$a9_file") ;
