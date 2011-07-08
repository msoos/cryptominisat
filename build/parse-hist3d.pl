# gnuplot-style graph generation for CryptoMiniSat by Vegard Nossum, 2011
# Minor edits by Mate Soos, 2011
# 
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

use Getopt::Long;
use Pod::Usage;
use strict;
use warnings;

my $granularity = 3000;

my $help = 0;
my @i = (0,0,0,0,0,0);
my @j = (0,0,0,0,0,0);
my @sizes = ({}, {}, {}, {}, {}, {});
my @filenames = ("sizedist", "gluedist", "propsizedist", "propgluedist", "conflsizedist", "conflgluedist");

GetOptions ("gran=i" => \$granularity, # numeric
   "sizedist=s" => \$filenames[0], # string
   "gluedist=s" => \$filenames[1], # string
   "propsizedist=s" => \$filenames[2], # string
   "propgluedist=s" => \$filenames[3], # string
   "conflsizedist=s" => \$filenames[4], # string
   "conflgluedist=s" => \$filenames[5], # string
   "help" => \$help
   #"verbose" => \$verbose  # flag
);

pod2usage(1) if $help;
#pod2usage(-verbose => 2) if $man;


#open my $fd, '>', $filename or die $!;

my %fd;
open $fd{"sizedist"}, ">", sprintf("%s.txt", $filenames[0]) or die $!;
open $fd{"gluedist"}, ">", sprintf("%s.txt", $filenames[1]) or die $!;

open $fd{"propsizedist"}, ">", sprintf("%s.txt", $filenames[2]) or die $!;
open $fd{"propgluedist"}, ">", sprintf("%s.txt", $filenames[3]) or die $!;

open $fd{"conflsizedist"}, ">", sprintf("%s.txt", $filenames[4]) or die $!;
open $fd{"conflgluedist"}, ">", sprintf("%s.txt", $filenames[5]) or die $!;

while ($_ = <>) {
    chomp;

    if (m/^Learnt clause size: (.*)$/) {
        my @clause = split m/ /, $1;
        my $val = scalar @clause;
        dostuff($val, 0, $fd{"sizedist"})
    } elsif (m/^Learnt clause glue: (\d+)$/) {
        dostuff($1, 1, $fd{"gluedist"})
    } elsif (m/^Prop by learnt size: (\d+)$/) {
        dostuff($1, 2, $fd{"propsizedist"})
    } elsif (m/^Prop by learnt glue: (\d+)$/) {
        dostuff($1, 3, $fd{"propgluedist"})
    } elsif (m/^Confl by learnt size: (\d+)$/) {
        dostuff($1, 4, $fd{"conflsizedist"})
    } elsif (m/^Confl by learnt glue: (\d+)$/) {
        dostuff($1, 5, $fd{"conflgluedist"})
    } else {
            printf "%s\n", $_;
    }
}

for (keys %fd)
{
    close $fd{$_};
}

# sub helpmessage
# {
#     print
#     "3D Graph generator by Vegard Nossum and Mate Soos
#      You can adjust the following:
#      --sizedist  -> 
#     "
# }

sub dostuff {
    my $val = shift;
    my $index = shift;
    my $myfile = shift;

    ++$sizes[$index]->{$val};

    if (++$i[$index] == $granularity) {
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


__END__

=head1 NAME

3D time-histogram generator by Vegard Nossum and Mate Soos

=head1 SYNOPSIS

1) ./cryptominisat --plain --maxconfl=30000 myfile.cnf > data

2) cat data | perl parse-hist3d.pl [options]

3) launch gnuplot

4) gnuplot > set pm3d interpolate 3,3

5) gnuplot > splot "./sizedist.txt" with pm3d notitle

=head1 OPTIONS

=over 8

=item B<--help>

Print a brief help message and exits.

=item B<--gran=X>

adjust granularity
default: 3000

=item B<--sizedist=X>

filename of size histogram
(default: sizedist.txt)

=item B<--gluedist=X>

filename of glue histogram
(default: gluedist.txt)

=item B<--propsizedist=X>

filename ofsize histogram of propagation by learnt clause
(default: propsizedist.txt)

=item B<--propgluedist=X>

filename of glue histogram of propagation by learnt clause
(default: propgluedist.txt)

=item B<--conflsizedist=X>

filename of size histogram of conflict by learnt clause
(default: conflsizedist.txt)

=item B<--conflgluedist=X>

filename of glue histogram of conflict by learnt clause
(default: conflgluedist.txt)

=back

=head1 DESCRIPTION

B<This program> reads the standard input and do something
useful with the contents thereof.

=cut

