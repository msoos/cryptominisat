#!/usr/bin/zsh

#Copyright Vegard Nossum, 2013
#
#This program is free software: you can redistribute it and/or modify
#it under the terms of the GNU General Public License as published by
#the Free Software Foundation, either version 3 of the License, or
#(at your option) any later version.

#This program is distributed in the hope that it will be useful,
#but WITHOUT ANY WARRANTY; without even the implied warranty of
#MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#GNU General Public License for more details.

#You should have received a copy of the GNU General Public License
#along with this program.  If not, see <http://www.gnu.org/licenses/>.


set -e
set -u

function choice()
{
	for x in "$@"
	do
		echo "$x"
	done | shuf -n 1
}

function int()
{
	shuf -i $1-$2 -n 1
}

function bool()
{
	shuf -i 0-1 -n 1
}

for i in {1..20000}
do
	# generate the instance
	../../sha1-sat/build/sha1-gen \
		--seed $RANDOM \
		--attack preimage \
		--rounds 18 \
		--hash-bits $(int 1 80) \
		--cnf \
		> instance.cnf

		#--lhbr $(bool) \
	# solve it
                
        #--simplify $(bool) \
        #--stamp $(bool) \
        #--cache $(bool) \
        #--scc $(bool) \
        #--presimp $(bool) \
        ../build/cryptominisat \
		--random $RANDOM \
		--clbtwsimp $(int 0 3) \
		--restart $(choice geom agility glue glueagility) \
		--agilviollim $(int 0 40) \
		--gluehist $(int 1 100) \
		--updateglue $(bool) \
		--binpri $(bool) \
		--otfhyper $(bool) \
		--clean $(choice size glue activity propconfl) \
		--preclean $(bool) \
		--precleanlim $(int 0 10) \
		--precleantime $(int 0 20000) \
		--clearstat $(bool) \
		--startclean $(int 0 16000) \
		--maxredratio $(int 2 20) \
		--dompickf $(int 1 20) \
		--flippolf $(int 1 3000) \
		--moreminim $(bool) \
		--alwaysmoremin $(bool) \
		--otfsubsume $(bool) \
		--rewardotfsubsume $(int 0 100) \
		--bothprop $(bool) \
		--probe $(bool) \
		--probemultip $(int 0 10) \
		--cachesize $(int 10 100) \
		--calcreach $(bool) \
		--cachecutoff $(int 0 2000) \
		--varelim $(bool) \
		--elimstrategy $(bool) \
		--elimcomplexupdate $(bool) \
		--subsume1 $(bool) \
		--block $(bool) \
		--asymmte $(bool) \
		--noextbinsubs $(bool) \
		--extscc $(bool) \
		--vivif $(bool) \
		--sortwatched $(bool) \
		--renumber $(bool) \
                --recur $(bool) \
		--verb 1 \
		--comps 1 \
        --compsfrom $(int 0 2) \
        --compsvar $(int 20000 500000) \
        --compslimit $(int 0 3000) \
		instance.cnf \
		| tee solution.out | grep -v '^v'

	# verify
	perl ../../sha1-sat/verify-preimage.pl instance.cnf solution.out | ../../sha1-sat/build/sha1-verify
done
