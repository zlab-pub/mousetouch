# Trial
# CODING_METHOD = 'FREQ'
CODING_METHOD = 'MANCHESTER_MODE'
# CODING_METHOD = 'INFRAME++'
# CODING_METHOD = 'DESIGNED_CODE'
# print('Choose Coding Method: (current is ' + CODING_METHOD + ')')
# print('\t1. FREQ')
# print('\t2. MANCHESTER_MODE')
# print('\t3. 10B12B')
# print('\t4. Designed Code')
# chose_num = input()
# if chose_num != '':
#     if int(chose_num) == 4:
#         CODING_METHOD = 'DESIGNED_CODE'
#     elif int(chose_num) == 2:
#         CODING_METHOD = 'MANCHESTER_MODE'
#     elif int(chose_num) == 3:
#         CODING_METHOD = 'fiveBsixB'
#     elif int(chose_num) == 1:
#         CODING_METHOD = 'FREQ'
    # CODING_METHOD = 'MANCHESTER_MODE' if int(chose_num) == 2 else 'FREQ'
LOOP = False# True
DETAILS = False
INTERPOLATION_DEBUG = True
MANCHESTER_MODE = CODING_METHOD == 'MANCHESTER_MODE'
CRC4 = CODING_METHOD == 'CRC4'
fiveBsixB = CODING_METHOD == 'fiveBsixB'
FREQ = CODING_METHOD == 'FREQ'
DESIGNED_CODE = CODING_METHOD == 'DESIGNED_CODE'
INFRAMEPP = CODING_METHOD == 'INFRAME++'

if MANCHESTER_MODE:
    EXPEND = 4 # 3
elif DESIGNED_CODE:
    EXPEND = 3
elif FREQ:
    EXPEND = 4
elif INFRAMEPP:
    EXPEND = 4
# FILTER_sure = input('Turn on FILTER? ')
# FILTER = False if FILTER_sure == '' else True
FILTER = True
# FILTER = False

FILTER_H = False
GRAPHICS = False #True
TESTING_MODE = True
FORCED_EXIT = False

if TESTING_MODE:
    FORCED_EXIT = True
    GRAPHICS = False
    DETAILS = True

# if not GRAPHICS:
#     FORCED_EXIT = True

LOCATION_SLIDE_WINDOW_SIZE = 10
BITS_NUM = 10
FRAME_RATE = 240
MOUSE_FRAME_RATE = 1600 #2800 # 2400

if MANCHESTER_MODE:
    PREAMBLE_STR = '100110' # '10101010101010' # '000'
elif CRC4:
    PREAMBLE_STR = ''
elif fiveBsixB:
    PREAMBLE_STR = '100001' # ver.1
elif FREQ:
    PREAMBLE_STR = '1001'
else:
    PREAMBLE_STR = '10101010101010' #'10001' # '10101010101010' # '000'

#PREAMBLE_STR = '11011'
PREAMBLE_LIST = list(PREAMBLE_STR) # used by np.array(preamble_list)
if DESIGNED_CODE:
    PREAMBLE_LIST = [1, -1, 1, -1]
    PREAMBLE_PATTERN = '1010'
    # PREAMBLE_LIST = []
import numpy as np
PREAMBLE_NP = np.array(PREAMBLE_LIST)
SIZE = (32, 32) # (256, 256)
CRC_LEN = 4


# setting for decoder

CHECK_BIT = 'BY_TIME' # 'BY_PERCENTAGE'
TIMES_INTERPOLATE = 10 # 10 choice one from two
FRAMES_PER_SECOND_AFTER_INTERPOLATE = 8400 # choice one from two
if FRAME_RATE == 240:
    FRAMES_PER_SECOND_AFTER_INTERPOLATE = 9600
INTERPOLATION_INTERVAL = 0.08 # 0.5
END_INTERVAL = 0.1
# EXTRA_LEN = 0.02
EXTRA_LEN = END_INTERVAL - INTERPOLATION_INTERVAL

#POINTS_PER_FRAME = 30 # 30 # to combine
MEAN_WIDTH = 40 # 50
#MIN_LEN_PER_BIT = 15 # 15

POINTS_TO_COMBINE = FRAMES_PER_SECOND_AFTER_INTERPOLATE / FRAME_RATE / 10 # 5 for advanced interpolation, 40 for 20hz , 14 for 60hz -> maintain 10 points for one bit
assert int(POINTS_TO_COMBINE) == POINTS_TO_COMBINE


# setting for encoder
ZOOM = 10
SHARE_PATH = 'render/share_data'
SHARE_PATH_LOCATION = 'render/share_data_location'
