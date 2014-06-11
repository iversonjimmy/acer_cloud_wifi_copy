#!/usr/bin/perl -w
# annotate the output of the test by substituting source file name and line number if they are missing

while (<>) {
    if (/TestDatasetDB\(\)/) {
	if (/\[(.+)\]/) {
	    open A2L, "addr2line -e TestDatasetDB $1 |" or die "FATAL: failed to run addr2line: $!";
	    my $line = <A2L>;
	    chomp $line;
	    if ($line =~ /\/([^\/]+:\d+)$/) {
		$line = $1;
	    }
	    close A2L;
	    s/\(\)/($line)/;
	}
    }
    print;
}


