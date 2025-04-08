import numpy as np

# Either load your fine LUTs:
minB = np.load('MinB.npy')
maxB = np.load('MaxB.npy')

# Downsample to 64x64 block (taking e.g. average or min or max)
coarse_shape = (64,64)

def downsample(arr, method='mean'):
    h, w = arr.shape
    reshaped = arr.reshape(coarse_shape[0], h//coarse_shape[0], 
                           coarse_shape[1], w//coarse_shape[1])
    if method == 'mean':
        return reshaped.mean(axis=(1,3)).astype(np.uint8)
    elif method == 'min':
        return reshaped.min(axis=(1,3))
    elif method == 'max':
        return reshaped.max(axis=(1,3))
    else:
        raise ValueError

minB_coarse = downsample(minB, method='min')
maxB_coarse = downsample(maxB, method='max')

def emit_array(name, arr):
    print(f'constexpr uint8_t {name}[64][64] = {{')
    for row in arr:
        print('    { ', end='')
        for i, val in enumerate(row):
            print(f'0x{val:02X}, ', end='' if i != 63 else '')
        print('},')
    print('};\n')

print('// Generated coarse LUTs')
emit_array("minB_coarse", minB_coarse)
emit_array("maxB_coarse", maxB_coarse)

