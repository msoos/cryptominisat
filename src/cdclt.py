print("Init module now")
print("--------- INIT ----------")
pbs = [
    #v1 + v2 <= 1
    [[1, 2], 1],
    [[1, 3], 1],
    [[-1, -3], 1],
    [[-1, 2], 1],
    [[1, -2], 1],
    [[-2, -3], 1]
    ]

# Propagates
def propagate(ass):
    print("propagate() call here")
    print("values: ")
    for l, i in zip(ass, range(1000000)):
        if i > 0:
            print ("%d: %d" % (i, l))

    for pb, maxval in pbs:
        print("pb  :", pb)
        print("card:", maxval)
        val = 0
        reason = []
        for p in pb:
            if ass[abs(p)] != 0:
                reason.append(-p)

            if p > 0:
                val += ass[p] > 0
            else:
                val += -ass[-p] > 0

        print("val   : ", val)
        print("maxval: ", maxval)
        if val > maxval:
            print("Conflict! Reason clause:", reason)
            return 2, [], reason

    return 0
