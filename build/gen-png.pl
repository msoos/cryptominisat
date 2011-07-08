# PNG graph generation for CryptoMiniSat by Vegard Nossum, 2011
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

use strict;
use warnings;

use GD;

my $slices;

my $min_y = 0;
my $max_y = 0;

my $min_z = 0;
my $max_z = 0;

while ($_ = <>) {
	chomp;

	next if m/^$/;

	my ($x, $y, $z) = split m/ /;
	push @{$slices->[$x]}, [$y, $z];

	$min_y = $y if $y < $min_y;
	$max_y = $y if $y > $max_y;

	$min_z = $z if $z < $min_z;
	$max_z = $z if $z > $max_z;
}

my $width = $max_y - $min_y + 1;
my $height = scalar @$slices;

my $im = GD::Image->new($width, $height, 1);

my $y = 0;
for my $slice (@$slices) {
	#$im->line(0, $y, $width - 1, $y, $black);

	for my $point (@$slice) {
		my $i = ($point->[1] - $min_z) / ($max_z - $min_z);
		die unless $i >= 0 && $i <= 1;

		$im->setPixel($point->[0], $y, $im->colorResolve(255 * r($i), 255 * g($i), 255 * b($i)))
	}

	++$y;
}

binmode STDOUT;
print $im->png;

sub r {
	my $x = shift;
	$x = 4 * $x / 6;

	return 1 if ($x >= 0 / 6 && $x <= 1 / 6);
	return 1 - ($x - 1 / 6) * 6 if ($x >= 1 / 6 && $x <= 2 / 6);
	return 0;
}

sub g {
	my $x = shift;
	$x = 4 * $x / 6;

	return $x * 6 if ($x >= 0 / 6 && $x <= 1 / 6);
	return 1 - ($x - 3 / 6) * 6 if ($x >= 3 / 6 && $x <= 4 / 6);
	return 1;
}

sub b {
	my $x = shift;
	$x = 4 * $x / 6;

	return 0 if ($x >= 0 / 6 && $x <= 1 / 6);
	return 1 if ($x >= 3 / 6 && $x <= 4 / 6);
	return ($x - 2 / 6) * 6;
}
