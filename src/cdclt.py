print("Init module now")
print("--------- INIT ----------")
pbs = [
    #v1 + v2 <= 1
    [[1, 2], 1],
    [[-1, 2], 1],
    [[-1, -2], 1],
    [[1, 3], 1],
    [[-1, -3], 1]
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

    for pb, maxval in pbs:
        #print("pb  :", pb)
        #print("card:", maxval)
        val = 0
        reason = []
        prop_lit = 0
        for p in pb:
            if ass[abs(p)] != 0:
                reason.append(-p)

            if ass[abs(p)] == 0:
                prop_lit = -p

            if p > 0:
                val += int(ass[p] > 0)
            else:
                val += int(-ass[abs(p)] > 0)

        print("val   : ", val)
        print("maxval: ", maxval)
        if val > maxval:
            print("Conflict! Reason clause:", reason)
            return 2, [], reason

        if val == maxval and prop_lit != 0:
            #print("prop lit:", prop_lit)
            #print("reason:", reason)
            prop = [prop_lit] + reason
            print("Propagation! Reason clause:", prop)
            return 1, [prop]

    return 0
