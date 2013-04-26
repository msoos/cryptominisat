#include "stamp.h"
#include "varreplacer.h"

using namespace forl;

bool Stamp::stampBasedClRem(
    const vector<Lit>& lits
) const {
    StampSorter sortNorm(tstamp, STAMP_IRRED, false);
    StampSorterInv sortInv(tstamp, STAMP_IRRED, false);

    stampNorm = lits;
    stampInv = lits;

    std::sort(stampNorm.begin(), stampNorm.end(), sortNorm);
    std::sort(stampInv.begin(), stampInv.end(), sortInv);

    assert(lits.size() > 0);
    vector<Lit>::const_iterator lpos = stampNorm.begin();
    vector<Lit>::const_iterator lneg = stampInv.begin();

    while(true) {
        if (tstamp[(~*lneg).toInt()].start[STAMP_IRRED]
            >= tstamp[lpos->toInt()].start[STAMP_IRRED]
        ) {
            lpos++;

            if (lpos == stampNorm.end())
                return false;
        } else if (tstamp[(~*lneg).toInt()].end[STAMP_IRRED]
            <= tstamp[lpos->toInt()].end[STAMP_IRRED]
        ) {
            lneg++;

            if (lneg == stampInv.end())
                return false;
        } else {
            return true;
        }
    }

    return false;
}

std::pair<size_t, size_t> Stamp::stampBasedLitRem(
    vector<Lit>& lits
    , StampType stampType
) const {
    size_t remLitTimeStamp = 0;
    StampSorter sorter(tstamp, stampType, true);
    std::sort(lits.begin(), lits.end(), sorter);

    #ifdef DEBUG_STAMPING
    cout << "Timestamps: ";
    for(size_t i = 0; i < lits.size(); i++) {
        cout
        << " " << tstamp[lits[i].toInt()].start[stampType]
        << "," << tstamp[lits[i].toInt()].end[stampType];
    }
    cout << endl;
    cout << "Ori clause: " << lits << endl;
    #endif

    assert(!lits.empty());
    Lit lastLit = lits[0];
    for(size_t i = 1; i < lits.size(); i++) {
        if (tstamp[lastLit.toInt()].end[stampType]
            < tstamp[lits[i].toInt()].end[stampType]
        ) {
            lits[i] = lit_Undef;
            remLitTimeStamp++;
        } else {
            lastLit = lits[i];
        }
    }

    if (remLitTimeStamp) {
        //First literal cannot be removed
        assert(lits.front() != lit_Undef);

        //At least 1 literal must remain
        assert(remLitTimeStamp < lits.size());

        //Remove lit_Undef-s
        size_t at = 0;
        for(size_t i = 0; i < lits.size(); i++) {
            if (lits[i] != lit_Undef) {
                lits[at++] = lits[i];
            }
        }
        lits.resize(lits.size()-remLitTimeStamp);

        #ifdef DEBUG_STAMPING
        cout << "New clause: " << lits << endl;
        #endif
    }

    size_t remLitTimeStampInv = 0;
    StampSorterInv sorterInv(tstamp, stampType, false);
    std::sort(lits.begin(), lits.end(), sorterInv);
    assert(!lits.empty());
    lastLit = lits[0];

    for(size_t i = 1; i < lits.size(); i++) {
        if (tstamp[(~lastLit).toInt()].end[stampType]
            > tstamp[(~lits[i]).toInt()].end[stampType]
        ) {
            lits[i] = lit_Undef;
            remLitTimeStampInv++;
        } else {
            lastLit = lits[i];
        }
    }

    if (remLitTimeStampInv) {
        //First literal cannot be removed
        assert(lits.front() != lit_Undef);

        //At least 1 literal must remain
        assert(remLitTimeStampInv < lits.size());

        //Remove lit_Undef-s
        size_t at = 0;
        for(size_t i = 0; i < lits.size(); i++) {
            if (lits[i] != lit_Undef) {
                lits[at++] = lits[i];
            }
        }
        lits.resize(lits.size()-remLitTimeStampInv);

        #ifdef DEBUG_STAMPING
        cout << "New clause: " << lits << endl;
        #endif
    }


    return std::make_pair(remLitTimeStamp, remLitTimeStampInv);
}

void Stamp::remove_from_stamps(const Var var)
{
    int types[] = {STAMP_IRRED, STAMP_RED};
    for(int i = 0; i < 2; i++) {
        tstamp[Lit(var, false).toInt()].dominator[types[i]] = lit_Undef;
        tstamp[Lit(var, true).toInt()].dominator[types[i]] = lit_Undef;
    }
    for(size_t i = 0; i < tstamp.size(); i++) {
        for(int i2 = 0; i2 < 2; i2++) {
            if (tstamp[i].dominator[types[i2]].var() == var) {
                tstamp[i].dominator[types[i2]] = lit_Undef;
            }
        }
    }
}

void Stamp::updateDominators(const VarReplacer* replacer)
{
    for(size_t i = 0; i < tstamp.size(); i++) {
        tstamp[i] = tstamp[replacer->getLitReplacedWith(Lit::toLit(i)).toInt()];
        if (tstamp[i].dominator[STAMP_IRRED] != lit_Undef) {
            tstamp[i].dominator[STAMP_IRRED]
                = replacer->getLitReplacedWith(tstamp[i].dominator[STAMP_IRRED]);
        }

        if (tstamp[i].dominator[STAMP_RED] != lit_Undef) {
            tstamp[i].dominator[STAMP_RED]
                = replacer->getLitReplacedWith(tstamp[i].dominator[STAMP_RED]);
        }
    }
}

void Stamp::clearStamps()
{
    for(vector<Timestamp>::iterator
        it = tstamp.begin(), end = tstamp.end()
        ; it != end
        ; it++
    ) {
        *it = Timestamp();
    }
}
