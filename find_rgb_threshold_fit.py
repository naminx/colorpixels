import numpy as np
from sklearn.linear_model import LinearRegression
import matplotlib.pyplot as plt

# Load LUTs and convert to integer to avoid overflow
minB = np.load("MinB.npy").astype(np.int16)
maxB = np.load("MaxB.npy").astype(np.int16)

max_sum = 510  # max of r+g for bytes
max_diff = -np.ones(max_sum + 1, dtype=np.float32)

# Accumulate *max* diffSum observed for each (r+g) pair on gray band edge
inds = np.where(minB < maxB)

for r, g in zip(*inds):
    sumRG = r + g
    mid_b = (minB[r, g] + maxB[r, g]) // 2
    diffSum = abs(r - g) + abs(g - mid_b) + abs(mid_b - r)
    if diffSum > max_diff[sumRG]:
        max_diff[sumRG] = diffSum

# X = sumRG values with observed band
X = []
Y = []
for s in range(max_sum + 1):
    if max_diff[s] >= 0:
        X.append(s)
        Y.append(max_diff[s] + 2.0)  # add small fudge margin

X = np.array(X).reshape(-1,1)
Y = np.array(Y)

# Fit linear regression
model = LinearRegression()
model.fit(X, Y)

a = model.intercept_
b = model.coef_[0]

print(f"Fitted diffSum threshold ≈ {a:.2f} + {b:.4f} * (r+g)\n")

print("// C code check:")
print("bool isBrightlyGray(unsigned char r, unsigned char g, unsigned char b) {")
print("    int diffSum = abs((int)r - (int)g) + abs((int)g - (int)b) + abs((int)b - (int)r);")
print("    int sumRG = r + g;")
scale = 10000
intercept_scaled = int(a * scale)
slope_scaled = int(b * scale)
print(f"    return (diffSum * {scale}) > ({intercept_scaled} + {slope_scaled} * sumRG);")
print("}")

# Optional: plot for insight  
plt.scatter(X, Y, s=5, label='max diffSum + margin')
xline = np.linspace(0, max_sum)
plt.plot(xline, model.predict(xline.reshape(-1,1)), 'r-', label='linear fit')
plt.xlabel('R + G sum')
plt.ylabel('diffSum threshold')
plt.legend()
plt.grid()
plt.title('Max diffSum on gray band edge + margin and linear fit')
plt.show()

