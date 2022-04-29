import os
from crccheck.crc import Crc4Itu
from scipy.fftpack import fft,ifft
from setting import *
import numpy as np
import math
import random
import time
import re
from scipy.signal import savgol_filter


def chunk_decode(np_chunk, flip=False):
    chunk = [str(i) for i in np_chunk]
    chunk = ''.join(chunk)
    preamble = PREAMBLE_STR
    if flip:
        pat = list(preamble)
        for i in range(len(preamble)):
            pat[i] = '0' if pat[i] == '1' else '1'
        pat = ''.join(pat)
    else:
        pat = preamble
    # print(pat)
    rtn = []

    # print('try to decode')
    for i in range(len(chunk)):
        if chunk[i:i+len(pat)] != pat:
            continue
        # print("Dectec preamble: ", i)
        wait2decode = chunk[i+len(pat):i+len(pat)+(BITS_NUM+4) * EXPEND]
        if len(wait2decode) != (BITS_NUM + 4)* EXPEND:
            continue
        bit_str = Manchester_decode(chunk[i+len(pat):i+len(pat)+(BITS_NUM+4) * EXPEND], flip=flip)
        # bit_str = designed_decode(chunk[i+len(pat):i+len(pat)+BITS_NUM * EXPEND], flip=flip)
        if not bit_str:
            # print(wait2decode)
            continue
        if len(bit_str) != BITS_NUM + 4:
            continue
        decoded_num = bit_str2num(bit_str[:-4])
        crc_val = list(bit_str[-4:])
        crc_val = ''.join(crc_val)
        crc_check = crc_validate(bit_str[:-4], crc_val)
        # print(crc_cal(bit_str[:-4]), crc_val)
        if crc_check:
            
            # print(flip)
            rtn.append([decoded_num, bit_str, naive_location(decoded_num, (32,32))])
        else:
            print('crc fail')
        # else:
        #     for index in range(len(bit_str)):
        #         bit_str = list(bit_str)
        #         bit_str[index] = '1' if bit_str[index] == '0' else '0'
        #         if crc_validate(''.join(bit_str), chunk[i+34:i+38]):
                    
        #             rtn.append([decoded_num, ''.join(bit_str), naive_location(decoded_num, (32,32))])
        #             break
        #         bit_str[index] = '1' if bit_str[index] == '0' else '0'
        #         bit_str = ''.join(bit_str)

    if rtn != []:
        return rtn

def naive_location(data, SIZE):
    return (int(data / SIZE[0]), data % SIZE[0])
    # return ()

def manhattan_dist(str1, str2):
    assert len(str1) == len(str2)
    count = 0
    for i in range(len(str1)):
        if str1[i] != str2[i]:
            count += 1
    return count

def designed_decode(received, recurse=True, flip=False):
    if flip:
        one = '011'
        zero = '001'
    else:
        one = '100'
        zero = '110'
    decoded = ''
    for i in range(0, len(received), 3):
        sub_data = received[i:i+3]
        if sub_data == one:
            decoded += '1'
        elif sub_data == zero:
            decoded += '0'
        # elif manhattan_dist(one, sub_data) == manhattan_dist(zero, sub_data):
        #     return
        # else:
        #     decoded = decoded + '0' \
        #         if manhattan_dist(one, sub_data) > manhattan_dist(zero, sub_data) \
        #         else decoded + '1'
        else:
            return
    return decoded

def designed_code(raw):
    new_code = []
    for i in raw:
        if int(i) == 1:
            new_code += [1, -1, -1]
        else:
            new_code += [1, 1, -1]
    return PREAMBLE_LIST + new_code

def add_NRZI(tenBtwlB, fixed_len=False):
    last_bit = '0'
    new_code = ''
    for i in tenBtwlB:
        if i == '0':
            new_code += last_bit * 2
        elif last_bit == '0':
            new_code = new_code + '01' 
        else:
            new_code = new_code + '10'
        last_bit = new_code[-1]
    if fixed_len:
        while len(new_code) < BITS_NUM:
            new_code = '0' + new_code
        if len(new_code) > BITS_NUM:
            print('overflow')
        return new_code[-BITS_NUM:]
    return new_code
    # last_bit = '0'
    # new_code = ''
    # for i in tenBtwlB:
    #     if i == '0':
    #         new_code += last_bit
    #     elif last_bit == '0':
    #         new_code = new_code + '1' 
    #     else:
    #         new_code = new_code + '0'
    #     last_bit = new_code[-1]
    # if fixed_len:
    #     while len(new_code) < BITS_NUM:
    #         new_code = '0' + new_code
    #     if len(new_code) > BITS_NUM:
    #         print('overflow')
    #     return new_code[-BITS_NUM:]
    # return new_code

# def add_NRZ(tenBtwlB):
#     last_bit = '0'
#     new_code = ''
#     for i in tenBtwlB:
#         if i == last_bit:
#             new_code += last_bit
#         elif last_bit == '1':
#             new_code = new_code + '0'
#         else:
#             new_code = new_code + '1'
#         last_bit = new_code[-1]
#     return new_code

# def smooth(y):
#     if y.size < 15:
#         return y
#     return savgol_filter(y, 15, 3)
from scipy.interpolate import interp1d
def interpl(x, y, x_sample, method='nearest'):
    inter = interp1d(x, y, kind=method)
    return inter(x_sample)

def smooth(a,WSZ):
    # a: NumPy 1-D array containing the data to be smoothed
    # WSZ: smoothing window size needs, which must be odd number,
    # as in the original MATLAB implementation
    out0 = np.convolve(a,np.ones(WSZ,dtype=int),'valid')/WSZ    
    r = np.arange(1,WSZ-1,2)
    start = np.cumsum(a[:WSZ-1])[::2]/r
    stop = (np.cumsum(a[:-WSZ:-1])[::2]/r)[::-1]
    return np.concatenate((  start , out0, stop  ))

# def smooth(y, box_pts):
#     box = np.ones(box_pts)/box_pts
#     y_smooth = np.convolve(y, box, mode='same')
#     return y_smooth

# ceil based on 0.5
def half_ceil(raw):
    flt, dcm = math.modf(raw)
    if flt <= 0.5:
        return dcm + 0.5
    else:
        return dcm + 1


def half_floor(raw):
    flt, dcm = math.modf(raw)
    if flt < 0.5:
        return dcm
    else:
        return dcm + 0.5


def sim_fix(raw_str):
    new_str = ''
    continue_num = 1
    last_ele = ''
    for i in range(len(raw_str)):
        assert raw_str[i] == '0' or raw_str[i] == '1'
        if raw_str[i] != last_ele:
            continue_num = 1
            last_ele = raw_str[i]
        elif continue_num == 2:
            continue
        else:
            continue_num += 1
        new_str += raw_str[i]
    return new_str

def Manchester_encode(raw_bit_str): # input: str, output: str
    new_bit_str = ['Unassigned'] * len(raw_bit_str)
    for i in range(len(raw_bit_str)):
        bit = raw_bit_str[i]
        # new_bit_str[i] = '01' if bit == '0' else '10'
        new_bit_str[i] = '01' if bit == '0' else '10'
    return ''.join(new_bit_str)

def Manchester_decode(raw_bit_str, flip=False): # input: str, output: str
    assert len(raw_bit_str) % 4 == 0
    new_bit_str = ['Unassigned']  * int(len(raw_bit_str) / 4)
    for i in range(len(raw_bit_str)):
        if i % 4 != 0:
            continue
        bits = raw_bit_str[i:i+4]

        # if manhattan_dist(bits, '1010') >= 3:
        #     bits = '1010'
        #     # print('fix1')
        # elif manhattan_dist(bits, '0101') >= 3:
        #     bits = '0101'
        #     # print('fix2')
        # else:
        if bits != '1010' and bits != '0101':
            # print(bits, raw_bit_str)
            return None
        
        if flip:
            new_bit_str[int(i / 4)] = '1' if bits == '0101' else '0'
        else:
            new_bit_str[int(i / 4)] = '0' if bits == '0101' else '1'
        # print(new_bit_str)
    return ''.join(new_bit_str)

def get_coordinate(x, y):
    return np.concatenate((x.reshape(x.size, 1), \
            y.reshape(y.size, 1)), axis=1)

def divide_coordinate(xy):
    return xy[:, 0], xy[:, 1]

# def get_screen_size():
#     import re
#     output = os.popen('xdpyinfo | grep dimensions').readlines()[0]
#     nums = re.findall("\d+",output)
#     return (int(nums[0]), int(nums[1]))


def bit_str2num(bits_str):
    num = 0
    for i in range(len(bits_str) - 1, -1, -1):
        bit_str = bits_str[i]
        multiplier = 0 if bit_str == '0' else 1
        num += multiplier * pow(2, len(bits_str) - 1 - i)
    return num 


def num2bin(num, bit_num): # return str
    current = "{0:b}".format(num)
    while len(current) < bit_num:
        current = '0' + current
    return current[-bit_num:]


def crc_cal(num, binary=True, bit_num=10):
    if type(num) == str:
        num = bit_str2num(num)
    byte_arr = bytearray(num.to_bytes(2, 'big'))
    crc = Crc4Itu.calc(byte_arr)
    if binary:
        return num2bin(crc, 4)
    else:
        return crc


def crc_validate(num, crc, binary=True, bit_num=10):
    if binary:
        num = bit_str2num(num)
        crc = bit_str2num(crc)
    hex_byte = bytes([crc])
    byte_arr = bytearray(num.to_bytes(2, 'big')) + hex_byte
    new_crc = Crc4Itu.calc(byte_arr)
    if new_crc == 0:
        return True
    return False

def hle(size):
    assert math.log(size[0], 2) % 1 == 0
    assert math.log(size[1], 2) % 1 == 0

    width_divided = size[0]
    height_divided = size[1]

    imgs_arr = np.zeros((BITS_NUM, size[0], size[1]), dtype=np.int16)
    [im_id, row, col] = imgs_arr.shape

    turn = True
    for n in range(im_id):
        mod = width_divided / 2 if turn else height_divided / 2
        if turn:
            width_divided /= 2
        else:
            height_divided /= 2

        if turn:
            for j in range(col):
                imgs_arr[n, :, j] = '0' if math.floor(j / mod) % 2 == 0 else '1'
        else:
            for i in range(row):
                imgs_arr[n, i, :] = '0' if math.floor(i / mod) % 2 == 0 else '1'
                
        turn = not turn
    return imgs_arr

# per x, y, per bit
# without preamble
def hle_raw(size):
    imgs_arr = np.zeros((size[0], size[1], BITS_NUM), dtype=np.int16)
    raw_imgs_arr = hle(size)
    for i in range(size[0]):
        for j in range(size[1]):
            imgs_arr[i,j,:] = raw_imgs_arr[:,i,j]
    return imgs_arr

def freq_encode(s):
    d = ''
    for i in s:
        d = d + '1010' if i == '0' else d + '1100'
    return d

# per x, y, per bit      
def raw_random_location(size):
    bits_pool = np.zeros((pow(2, BITS_NUM), BITS_NUM))
    index_pool = list(range(pow(2, BITS_NUM)))
    for i in range(pow(2, BITS_NUM)):
        bits = num2bin(i, BITS_NUM)
        bits_pool[i] = np.array(list(bits))
    data = np.zeros((size[0], size[1], BITS_NUM * EXPEND + PREAMBLE_NP.size + 4), dtype=np.int16)
    # print(index_pool)
    for i in range(size[0]):
        for j in range(size[1]):
            # random_index = random.choice(index_pool)
            random_index = i * size[0] + j
            # random_index = random_index % 32 # tobe comment
            # random_index = max(341, random_index)
            # random_index = min(342, random_index)
            temp = bits_pool[random_index]
            temp = [str(int(i)) for i in temp]
            # temp = temp[5:]
            if MANCHESTER_MODE:
                encoded_str ='0001' +Manchester_encode(''.join(temp))
            elif DESIGNED_CODE:
                encoded_str = designed_code(''.join(temp))
            elif FREQ:
                # encoded_str = '1001' + freq_encode(''.join(temp))
                encoded_str = '1001' + freq_encode(Manchester_encode(''.join(temp)))
            # encoded_str = encoded_str + list(crc_cal(bits_pool[random_index]))
            r = [str(i) for i in bits_pool[random_index].astype(int)]
            # print(''.join(r))
            data[i,j,:] = list(encoded_str) + list(crc_cal(''.join(r)))
            # index_pool.remove(random_index)
    return data


# with preamble
# per x,y, per bit
def fiveBsixB_encode(size):
    from fiveBsixB_coding import CODING_DIC
    assert BITS_NUM == 10
    raw_arr = hle_raw(size)
    imgs_arr = np.zeros((size[0], size[1], len(PREAMBLE_STR) + 12), dtype=np.int16)
    for i in range(size[0]):
        for j in range(size[1]):
            temp_list = raw_arr[i,j,:].tolist()
            temp_list = [str(i) for i in temp_list]
            encoded_str = CODING_DIC[''.join(temp_list)]
            encoded_list = PREAMBLE_STR + encoded_str
            imgs_arr[i,j] = list(encoded_list)
    return imgs_arr

# without preamble
# per x,y, per bit
def designed_location_encode(size):
    assert BITS_NUM == 10
    raw_arr = hle_raw(size)
    imgs_arr = np.zeros((size[0], size[1], BITS_NUM * EXPEND + PREAMBLE_NP.size), dtype=np.int16)
    for i in range(size[0]):
        for j in range(size[1]):
            temp_list = raw_arr[i,j,:].tolist()
            temp_list = [str(i) for i in temp_list]
            if EXPEND == 6:
                encoded_str = designed_code(Manchester_encode(''.join(temp_list)))
            else:
                encoded_str = designed_code(''.join(temp_list))
            imgs_arr[i,j] = list(encoded_str)
    return imgs_arr

def hld(bit_arr, size, bit_one, bit_zero):
    # print(bit_arr)
    assert len(bit_arr) == BITS_NUM

    init_location_range = [[0, size[0]], [0, size[1]]]
    width_divided = size[0]
    height_divided = size[1]

    turn = True
    for bit in bit_arr:
        if turn:
            if width_divided <= 1:
                continue
            dis = init_location_range[1][1] - init_location_range[1][0]
            init_location_range[1] = [init_location_range[1][0], init_location_range[1][1]  - dis / 2] if bit == bit_zero \
                    else [init_location_range[1][0] + dis / 2, init_location_range[1][1]]
            width_divided /= 2
        else:
            if height_divided <= 1:
                continue
            dis = init_location_range[0][1] - init_location_range[0][0]
            init_location_range[0] = [init_location_range[0][0], init_location_range[0][1] - dis / 2] if bit == bit_zero \
                    else [init_location_range[0][0] + dis / 2, init_location_range[0][1]]
            height_divided /= 2
        turn = not turn

    return init_location_range

def mid_one_larger_than(x, compare_num):
    mid_one = None
    for i in x:
        if i >= compare_num:
            mid_one = i
            break
    return (mid_one + x[-1])/2

def first_one_larger_than(x, compare_num):
    for i in x:
        if i >= compare_num:
            return i

############################################
################## DSP #####################
############################################
import matplotlib.pyplot as plt
def filter_normalize(complex_arr, quiet=False, nothing=False):
    complex_arr = np.array(complex_arr)
    quiet = True
    if not quiet:
        print('Default length is 8 in FREQ and 4 for others')
        show = input('If show figures? Default is off. ')
        print(complex_arr)
        show = True if show != '' else False
    else:
        show = False
    if show:
        plt.figure()
        plt.plot(list(range(len(complex_arr))), complex_arr, marker='o')
        plt.figure()
        plt.plot(list(range(len(complex_arr))), abs(fft(complex_arr)), marker='o')
        plt.show()
    if not quiet:
        l = input('cut length: ')
    else:
        l = ''
    show = True
    l = ''
    
    if l != '':
        while l != '':
            l = int(l)
            a1 = fft(complex_arr)
            a1[0:1 + l]=0
            a1[complex_arr.size - l:complex_arr.size]=0
            a2 = ifft(a1).real
            if show:
                plt.subplot(1,2,1)
                plt.plot(list(range(len(complex_arr))), complex_arr, marker='x')
                plt.plot(list(range(len(a2))), a2, marker='x')
                plt.subplot(1,2,2)
                plt.plot(list(range(len(a1))), abs(fft(complex_arr)), marker='x')
                plt.plot(list(range(len(a2))),abs(fft(a2)), marker='x')
                plt.show()
            l = input('update cut length? ')
    else:
        l = 3
        l2 = 0
        a1 = fft(complex_arr)
        a1[0:1 + l]=0
        a1[l+1:l+1+l2] /= 2

        a1[complex_arr.size - l:complex_arr.size]=0
        a1[complex_arr.size - l - l2:complex_arr.size - l] /= 2
        # a1[11] *=0.99
        # a1[-11] *=0.99
        # a1[22] -=0.08
        a2 = ifft(a1).real
        if show:
            # plt.subplot(1,2,1)
            # plt.plot(list(range(len(complex_arr))), complex_arr, marker='x')
            # plt.plot(list(range(len(a2))), a2, marker='x')
            # plt.subplot(1,2,2)
            # plt.plot(list(range(len(a1))), abs(fft(complex_arr)), marker='x')
            plt.plot(list(range(len(a2))),abs(fft(a2)), marker='x')
            # plt.show()
    
    # if show:
    #     new_arr = np.concatenate((a2, a2))
    #     plt.plot(list(range(len(new_arr))),abs(fft(new_arr)), marker='x')
    #     plt.show()
    if not quiet:
        print(ifft(a1))
    # a2 = a2 - a2.mean()
    # a2 = a2 / 2 + 0.5
    if nothing:
        a2 = complex_arr
    amax = max(abs(a2.max()), abs(a2.min()))
    amin = -amax

    min_val = 0.7 #0.7
    max_val = 1.3 # 1.3
    a2 = [(0.7 + (1.3-0.7) * (i - amin)/(amax - amin)) for i in a2]
    # print('a2 ', end='')
    # print(a2)
    return a2

def bound(arr, min_val, max_val):
    amax = max(abs(arr.max()), abs(arr.min()))
    amin = -amax
    return [(min_val + (max_val - min_val) * (i - amin)/(amax - amin)) / 2 for i in arr]


##########################################
######## Report Utils ###################
###########################################
def test_report(one_bit, possible_dataB, possible_dataD, fixed_bit_arr, fixed_val):
    pass
