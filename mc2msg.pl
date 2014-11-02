#!/usr/bin/perl

# .mc to .msg converter
# .msg is input format for gencat(1)
# (c) Ufasoft 2010 http://ufasoft.com mailto:support@ufasoft.com
# Version 2010
# This software is Public Domain

use warnings;
use strict;

use English;
use File::Basename;
use Getopt::Std;

sub str2msgid {
	$_ = shift;
	return undef if !defined || $_ eq "";
	return /^0/ ? oct : 0+$_;
}

sub readLine {
	$_ = <F>;
	return undef unless defined;
	s/\n|\r//g;
	/(^[^;]*)(;(.*))?$/;
	my ($cont, $comment) = ($1, $3);
	if ($cont =~ /^\s*$/) {
		print H "$comment\n" if $comment;
	} else {
		print H "// $_\n" if $_ ne ".";
	}
	return $cont;
}

sub processList {
 	my ($fun, $line) = @_;
	while (1) {
		if ($line =~ /(\w+)\s*=\s*(\w+)\s*/) {
			$$fun->($1, $2);
		}
		last if $line =~ /\)/;
		$line = readLine;
	}
}

sub VERSION_MESSAGE {
	my $fh = shift;
	print $fh ".mc to .msg converter\n";
	print $fh "(c) Ufasoft, 2010, http://ufasoft.com, mailto:support\@ufasoft.com, Public Domain\n";
}

sub HELP_MESSAGE {
	my $fh = shift;
	print $fh "  Usage: mc2msg.pl [-h dir] <file.mc>\n";
	print $fh "    -h dir: gives the dir, where to create .h and .msg, by default same as dir of file.mc\n";
	print $fh "  Outputs: file.msg file.h\n";
}

$Getopt::Std::STANDARD_HELP_VERSION = 1;
my %opts = ();
exit 1 if !getopts("h:k:", \%opts);

if (@ARGV < 1) {
	main::VERSION_MESSAGE STDERR;
	main::HELP_MESSAGE STDERR;
	exit 3;
}

my $mcfile = $ARGV[0];

sub main {

    open F, "< $mcfile" or die "Couldn't open `$mcfile': $!";

    my (%name2fac, %name2sev);

    my $id = 0;
    my $prevfac = -1;
    my $fac = 0;
    my $sev = 0;
	my $typ = "int";

    while (defined(my $line = readLine)) {
    	next if $line =~ /^\s*$/;
    	if ($line =~ /^\s*(\w+)\s*=\s*(.*)/) {
    		my ($name, $val) = ($1, $2);
    		if ($name eq "FacilityNames") {
    			processList(\sub {
						my ($n, $v) = @_;
    					$name2fac{$n} = str2msgid($v);
    				}, $val);
    		} elsif ($name eq "SeverityNames") {
    			processList(\sub {
						my ($n, $v) = @_;
    					$name2sev{$n} = str2msgid($v)
    				}, $val);
    		} elsif ($name eq "LanguageNames") {
    			processList(\sub {}, $val);
  			} elsif ($name eq "MessageIdTypedef") {
				$typ = $val;
    		} else {
    			my $msg = "";
    			my $bfields = 1;
    			my $sym;

MESSAGE:   		while (1) {
    				if ($name eq "MessageId") {
						$_ = str2msgid($val);						
    					$id = $_ if defined;
    				} elsif ($name eq "Facility") {
						$_ = $name2fac{$val};
    					$fac = $_ if defined;
    				} elsif ($name eq "SymbolicName") {
    					$sym = $val;
    				} elsif ($name eq "Severity") {
						$_ = $name2sev{$val};
    					$sev = $_ if defined;
    				} elsif ($name eq "Language") {
    				} else {
    			 		die "Unexpected format in the line $.";
    				}
					$line = readLine;
			    	next if $line =~ /^\s*$/;
			    	if ($line =~ /^\s*(\w+)\s*=\s*(.*)/) {
    					($name, $val) = ($1, $2);
					} else {
						while (1) {
							$msg .= $line;
							$line = readLine;
							last MESSAGE if $line eq ".";
						}
					}    			
    			}
            	my $hr = ($sev << 30) | ($fac << 16) | $id;
            	print H "#define $sym (($typ)0x" .  sprintf("%08x", $hr) . ")\n" if $sym;

            	if ($fac != $prevfac) {
            		my $f = $fac & 255;			# NL_SETMAX==255 in BSD
            		print M "\$set $f\n";		
            		$prevfac = $fac;
            	}
            	print M "$id $msg\n";

    			++$id;
    		}
    	} else {
    		die "Unexpected format in the line $.";
    	}
    }
}

my ($file,	$dir,	$ext) = fileparse($mcfile, ".mc");
$dir = $opts{h} if defined $opts{h};

my ($hfn, $mfn) = ("$dir/$file.h", "$dir/$file.msg");

open H, "> $hfn" or die "Couldn't create `$hfn': $!";
open M, "> $mfn" or die "Couldn't create `$mfn': $!";

eval { main() };
if ($EVAL_ERROR) {
    close H;
	close M;
	unlink $hfn, $mfn;
	die $EVAL_ERROR;
}

