/*****************************************************************************
Graph generation for CryptoMiniSat by Vegard Nossum, 2011
Minor edits by Mate Soos, 2011

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/

use strict;
use warnings;

my $granularity1 = 3000;
my $granularity2 = 3000;

my @i = (0,0,0,0,0,0);
my @j = (0,0,0,0,0,0);
my @sizes = ({}, {}, {}, {}, {}, {});

#open my $fd, '>', $filename or die $!;

my %fd;
open $fd{"sizedist"}, ">sizedist.txt" or die $!;
open $fd{"gluedist"}, ">gluedist.txt" or die $!;

open $fd{"propsizedist"}, ">propsizedist.txt" or die $!;
open $fd{"propgluedist"}, ">propgluedist.txt" or die $!;

open $fd{"conflsizedist"}, ">conflsizedist.txt" or die $!;
open $fd{"conflgluedist"}, ">conflgluedist.txt" or die $!;

while ($_ = <>) {
    chomp;

    if (m/^Learnt clause size: (.*)$/) {
        my @clause = split m/ /, $1;
        my $val = scalar @clause;
        dostuff($val, 0, $granularity1, $fd{"sizedist"})
    } elsif (m/^Learnt clause glue: (\d+)$/) {
        dostuff($1, 1, $granularity1, $fd{"gluedist"})
    } elsif (m/^Prop by learnt size: (\d+)$/) {
        dostuff($1, 2, $granularity2, $fd{"propsizedist"})
    } elsif (m/^Prop by learnt glue: (\d+)$/) {
        dostuff($1, 3, $granularity2, $fd{"propgluedist"})
    } elsif (m/^Confl by learnt size: (\d+)$/) {
        dostuff($1, 4, $granularity2, $fd{"conflsizedist"})
    } elsif (m/^Confl by learnt glue: (\d+)$/) {
        dostuff($1, 5, $granularity2, $fd{"conflgluedist"})
    } else {
            printf "%s\n", $_;
    }
}

for (keys %fd)
{
    close $fd{$_};
}

sub dostuff {
    my $val = shift;
    my $index = shift;
    my $mygranularity = shift;
    my $myfile = shift;

    ++$sizes[$index]->{$val};

    if (++$i[$index] == $mygranularity) {
        hist3d($myfile, $j[$index], $sizes[$index]);

        $i[$index] = 0;
        ++$j[$index];
        $sizes[$index] = ();
    }
}


sub hist3d {
        my $myfile = shift;
	my $index = shift;
	my $hist = shift;

	#printf STDERR "*** %s\n", $index;

	printf $myfile "%u %u %u\n", $index, $_, $hist->{$_} for sort { $a <=> $b } keys %$hist;
	printf $myfile "\n";
}

