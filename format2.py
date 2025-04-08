import numpy as np

def pack64(row_bytes):
    """
    Pack successive groups of 8 bytes into uint64_t integers.
    Little-endian order: byte0 is LSB.
    Returns array length 8 of uint64_t for one row of 64.
    """
    row_bytes = np.array(row_bytes, dtype=np.uint8).reshape(-1,8)
    result = []
    for chunk in row_bytes:
        val = 0
        for i in range(8):
            val |= int(chunk[i]) << (8*i)
        result.append(val)
    return result

def emit_packed(name, arr):
    print(f'// {name}: packed uint64_t version of 64x64 byte table')
    print(f'constexpr uint64_t {name}_packed[64][8] = {{')
    for row in arr:
        packed = pack64(row)
        print('    { ' + ', '.join(f'0x{val:016X}' for val in packed) + ' },')
    print('};\n')

    # cast to bytes
    print(f'constexpr auto& {name} = reinterpret_cast<const uint8_t(&)[64][64]>({name}_packed);\n')

# --------
# Demo data: replace this by load/your downsampling
minB = np.load('MinB.npy')
maxB = np.load('MaxB.npy')


def downsample(arr, method='min'):
    reshaped = arr.reshape(64,4,64,4)
    if method == 'min':
        return reshaped.min(axis=(1,3))
    elif method == 'max':
        return reshaped.max(axis=(1,3))
    elif method == 'mean':
        return reshaped.mean(axis=(1,3)).astype(np.uint8)
    else:
        raise ValueError

minB_coarse = downsample(minB, 'min')
maxB_coarse = downsample(maxB, 'max')

print('// Generated C++ constexpr LUTs packed into uint64_t and cast to byte arrays')
emit_packed('minB_coarse', minB_coarse)
emit_packed('maxB_coarse', maxB_coarse)

