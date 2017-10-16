import pickle
import pandas as pd
import matplotlib.pyplot as plt
df = None
with open("pandasdata.dat", "rb") as f:
    df = pickle.load(f)

l = df.columns.values.flatten().tolist()

# print(df.describe())
plt.style.use = "default"

for x in l:
    if x == "good":
        continue
    df2 = df.filter(items=["good", x])
    df2.groupby("good").hist()
