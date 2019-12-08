print("Init module now")
print("--------- INIT ----------")
cards = [
    #1*v1 + 1*v2 + 1*v3 <= 1
    [[1, 2, 3, 4, 5, 6, 7], 1],
    #1*(NOT v1) + 1*v2 <= 1
    [[-1, 10], 1],
    #1*(NOT v1) + 1*(NOT v3) <= 1
    [[-1, -30], 1],
    ]
verb = False

# Propagates
def propagate(ass):
    if verb:
        print("propagate() called")
        print("variable values: ")
        for l, i in zip(ass, range(100000)):
            if i > 0:
                print ("%d: %d" % (i, l))

    #Let's go through all the cardinality constraints and see what's up
    for lhs, rhs in cards:
        print("lhs: %s <= %s" % (lhs, rhs))
        val = 0
        reason = []
        prop_lits = []
        for p in lhs:
            #assigned variable in cardinality constraint
            #could be part of a reason!
            if ass[abs(p)] != 0:
                var = abs(p)
                if p < 0:
                    neg = -1
                else:
                    neg = 1

                #Only put FALSIFIED values into the reason
                if ass[var]*neg > 0:
                    reason.append(-p)

            #unassigned variable in cardinality constraint
            if ass[abs(p)] == 0:
                #potentially propagated literals
                prop_lits.append(-p)

            #Let's update the current value
            if p > 0:
                val += int(ass[p] > 0)
            else:
                val += int(-ass[abs(p)] > 0)

        print("val   : ", val)
        print("rhs   : ", rhs)
        if val > rhs:
            print("Conflict! Reason clause:", reason)
            return 2, [], reason

        if val == rhs and len(prop_lits) != 0:
            props = []
            for prop_lit in prop_lits:
                prop = [prop_lit] + reason
                print("Propagation! Reason clause:", prop)
                props.append(prop)

            return 1, props

    return 0
