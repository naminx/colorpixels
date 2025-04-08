import numpy as np
from sklearn.linear_model import LinearRegression

minB = np.load("MinB.npy")
maxB = np.load("MaxB.npy")

# For valid pixels (grayish range exists)
inds = np.where(minB < maxB)

rg_diff = []
diffsum_edge = []
sumRGB = []

minB = minB.astype(np.int32)
maxB = maxB.astype(np.int32)

for r, g in zip(*inds):
    mid_b = (minB[r,g] + maxB[r,g]) // 2
    rg_sum = r + g
    # Typical mid-grayish brightness sum (ignoring B component)
    sumRGB.append( rg_sum )

    # “Critical” case: edge of grayish band, diffSum just below threshold
    rg_diff.append(abs(r-g))
    diffsum_edge.append(abs(r-g) + abs(g-mid_b) + abs(mid_b - r))

sumRGB = np.array(sumRGB)
diffsum_edge = np.array(diffsum_edge)

# Fit a simple affine least squares:
X = sumRGB.reshape(-1,1)
y = diffsum_edge

model = LinearRegression().fit(X, y)

a = model.intercept_
b = model.coef_[0]

print(f"Fitted diffSum threshold ≈ {a:.2f} + {b:.4f} * (r+g)")

print(f"\n// C code check:")
print(f"bool isBrightlyGray(unsigned char r, unsigned char g, unsigned char b) {{")
print(f"    int diffSum = abs((int)r - (int)g) + abs((int)g - (int)b) + abs((int)b - (int)r);")
print(f"    int sumRG = r + g;")
print(f"    return diffSum > ({a:.2f} + {b:.4f} * sumRG);")
print(f"}}")

