print("Init module now")
print("--------- INIT ----------")


# Propagates
def propagate(a):
    print("propagate here!")
    print("values: ")
    for x in a:
        print (x)

    reason = [1,2, 3]
    prop = (1, reason)
    return 0, [prop]
