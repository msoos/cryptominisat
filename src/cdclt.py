print("Init module now")
print("--------- INIT ----------")
cards = [
    #v1 + v2 <= 1
    [[1, 2], 1],
    [[1, 3], 1],
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

    for pb, card in cards:
        val = 0
        reason = []
        for p in pb:
            if ass[abs(p)] != 0:
                reason.append(-p)

            if p > 0:
                val += ass[p] > 0
            else:
                val += -ass[-p] > 0

        print("val       : ", val)
        print("card (max): ", card)
        if val > card:
            print("Conflict! Reason clause:", reason)
            return 2, [], reason

    reason = [1,2, 3]
    prop = (1, reason)
    return 0
