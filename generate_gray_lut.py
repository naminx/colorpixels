import numpy as np
from skimage import color
from tqdm import tqdm

THRESHOLD = 11  # chroma threshold for near-gray
W, H = 256, 256

minB = np.zeros((W, H), dtype=np.uint8)
maxB = np.zeros((W, H), dtype=np.uint8)

print("Generating gray LUT... (this may take a few minutes)")

for r in tqdm(range(W), desc='Processing R'):
    for g in range(H):
        chromas = []
        for b in range(256):
            rgb = np.array([[[r/255.0, g/255.0, b/255.0]]])  # shape (1,1,3)
            lab = color.rgb2lab(rgb)[0,0]
            a, b_lab = lab[1], lab[2]
            chroma = np.sqrt(a**2 + b_lab**2)
            chromas.append(chroma)
        chromas = np.array(chromas)

        # Indices of B with chroma below threshold
        low_chroma = np.where(chromas < THRESHOLD)[0]
        if len(low_chroma) == 0:
            # No grayish B for this (R,G)
            minB[r,g] = 255
            maxB[r,g] = 0
        else:
            minB[r,g] = low_chroma[0]
            maxB[r,g] = low_chroma[-1]

print("Done.")

# Save the arrays
np.save('MinB.npy', minB)
np.save('MaxB.npy', maxB)

print("\n// ---------- Generated constexpr lookup tables ---------")
print("constexpr uint8_t MinB[256][256] = {")
for r in range(W):
    line = ', '.join(f"{minB[r,g]:3}" for g in range(H))
    print(f"    {{{line}}},")
print("};\n")

print("constexpr uint8_t MaxB[256][256] = {")
for r in range(W):
    line = ', '.join(f"{maxB[r,g]:3}" for g in range(H))
    print(f"    {{{line}}},")
print("};")

