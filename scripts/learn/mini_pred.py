import pandas
import pickle
from sklearn.model_selection import train_test_split
with open("test_predict2/data.sqlite-pandasdata.dat", "rb") as f:
  df = pickle.load(f)

features = df.columns.values.flatten().tolist()
features.remove("x.num_used")
features.remove("x.class")
features.remove("x.lifetime")
features.remove("fname")
features.remove("x.lifetime_cut")
features.remove("cl.cur_restart_type")
features.remove("cl2.cur_restart_type")

train, test = train_test_split(df, test_size=0.33)
X_train = train[features]
y_train = train["x.lifetime_cut"]
X_test = test[features]
y_test = test["x.lifetime_cut"]

import sklearn
import sklearn.svm
clf = sklearn.svm.SVC()
clf.fit(X_train, y_train)

import sklearn.metrics
y_pred = clf.predict(X_test)
accuracy = sklearn.metrics.accuracy_score(y_test, y_pred)
precision = sklearn.metrics.precision_score(y_test, y_pred, average="micro")
recall = sklearn.metrics.recall_score(y_test, y_pred, average="micro")
print("accuracy:", accuracy)
print("precision:", precision)
print("recall:", recall)
sklearn.metrics.confusion_matrix(y_test, y_pred)


