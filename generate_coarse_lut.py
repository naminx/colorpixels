import numpy as np

# Load your previously saved high-res chroma masking limits
minB = np.load('MinB.npy')  # shape (256,256)
maxB = np.load('MaxB.npy')

W_HR, H_HR = 256, 256   # input size
W_CR, H_CR = 64, 64     # coarse size (factor 4 downscale)

stride = W_HR // W_CR  # must divide evenly (=4)

coarse_minB = np.zeros((W_CR, H_CR), dtype=np.uint8)
coarse_maxB = np.zeros((W_CR, H_CR), dtype=np.uint8)

for rbin in range(W_CR):
    for gbin in range(H_CR):
        r0, r1 = rbin*stride, (rbin+1)*stride
        g0, g1 = gbin*stride, (gbin+1)*stride

        tile_min = minB[r0:r1, g0:g1]
        tile_max = maxB[r0:r1, g0:g1]

        # Conservative merge: expand chroma < thresh range
        block_min = tile_min.min()
        block_max = tile_max.max()

        # Optional: tighten by ignoring impossibles (full block empty)
        # if block_min == 255 and block_max == 0:
        #     block_min, block_max = 255, 0

        coarse_minB[rbin, gbin] = block_min
        coarse_maxB[rbin, gbin] = block_max

np.save('coarse_minB.npy', coarse_minB)
np.save('coarse_maxB.npy', coarse_maxB)

# Print constexpr-formatted C++ output
print('constexpr uint8_t MinB_coarse[64][64] = {')
for r in range(W_CR):
    values = ', '.join(f'{coarse_minB[r,g]}' for g in range(H_CR))
    print(f'    {{{values}}},')
print('};\n')

print('constexpr uint8_t MaxB_coarse[64][64] = {')
for r in range(W_CR):
    values = ', '.join(f'{coarse_maxB[r,g]}' for g in range(H_CR))
    print(f'    {{{values}}},')
print('};')
