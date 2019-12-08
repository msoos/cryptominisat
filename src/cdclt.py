print("--------- INIT ----------")
cards = [
    #1*v1 + 1*v2 + 1*v3 [....] <= 1
    [[1, 2, 3, 4, 5, 6, 7], 1],
    ]

cards2 = [
    #1*v1 + 1*(NOT v2) + 1*v3 + 1*v4 ...<= 1
    [[1, 2, 3, 4, 5, 6, 7], 1],
    ]

# try adding the line "2 0" to the CNF and see what happens
cards3 = [
    #1*v1 + 1*v2 + 1*v3 <= 1
    [[1, 2, 3, 4, 5, 6, 7], 2],
    ]

cards4 = [
    #1*v10 + 1*v11 <= 1
    [[10, 11], 1],
    #1*(NOT v10) + 1*v11 <= 1
    [[-10, 11], 1],
    #1*v10 + 1*(NOT v11) <= 1
    [[10, -11], 1],
    #1*(NOT v10) + 1*(NOT v11) <= 1
    [[-10, -11], 1]
]

print("--------- INIT FINISH ----------")

# Propagate or conflict based on the cardinality constraints
# 'ass' contains the assignements:
# -> ass[1] = 0  means variable 1 is UNDEFINED
# -> ass[1] = 1  means variable 1 is TRUE
# -> ass[1] = -1 means variable 1 is FALSE
def propagate(ass):
    print("propagate() called")
    print("variable assignments: ")
    for val, i in zip(ass, range(100000)):
        if i > 0:
            print ("%d: %d" % (i, val))

    # nothing propagated or conflicted, return 0
    return 0

    # to return a conflict do:
    #return 2, [], conflict_reason
    # where conflict_reason is a list of literals that are all falsified
    # e.g. [1, 2, -3] where:
    # -> ass[1] = -1
    # -> ass[2] = -1
    # -> ass[3] = 1

    # to return an invdividual propagation, do:
    #return 1, [propagating_reason]
    # where propagating_reason is a list of literals that are all falsified
    # except the first literal which is undefined, e.g.
    # e.g. [4, 2, -3] where:
    # -> ass[1] = 0
    # -> ass[2] = -1
    # -> ass[3] = 1

    # to return multiple proapgations, do:
    #return 1, propagating_reasons
    # where "propagating_reasons" is a list of "propagation_reason"-s
