import numpy as np
from collections import deque
from setting import MOUSE_FRAME_RATE, PREAMBLE_LIST, BITS_NUM, FRAME_RATE
from utils import smooth, interpl, chunk_decode

NDATA = 5

class Localizer:
    def __init__(self):
        self.reset()
        self.len_e = 3000
        self.last_succ = None

    def reset(self):
        self.buffers = []
        for i in range(NDATA):
            self.buffers.append(deque(maxlen=MOUSE_FRAME_RATE * 2))
        # self.frames = deque(maxlen=MOUSE_FRAME_RATE * 2)
        # self.frames_accum = deque(maxlen=MOUSE_FRAME_RATE * 2)
        self.last_ts = [None]*NDATA
        # self.last_ts_accum = None
    
    def update(self, ts, data):
        rlt = None
        for buffer_id in range(len(data)):
            val = data[buffer_id]
            tmp = self.resolve((ts,val), buffer_id)
            if tmp is not None:
                #print("Buffer", buffer_id)
                #print(tmp)
                if rlt is None:
                    rlt = tmp
        if rlt is not None:
            self.reset()
        return rlt

    def resolve(self, val_tuple, buffer_id):
        # print("Decode Buffer", buffer_id)
        # print(self.buffers[buffer_id])
        timestamp, val = val_tuple
        val_fixed = val
        if len(self.buffers[buffer_id])!=0 and abs(val_fixed - self.buffers[buffer_id][-1][1]) > 20:
            val_fixed = self.buffers[buffer_id][-1][1]
        # if val_fixed < 128:
        #     val_fixed += 128
        # if val_fixed > 240:
        #     return
        self.buffers[buffer_id].append((timestamp, val_fixed))
        frames = self.buffers[buffer_id]
        if self.last_ts[buffer_id] is None:
            self.last_ts[buffer_id] = frames[0][0]
        last_ts = self.last_ts[buffer_id]
        
        if frames[-1][0] - last_ts < 0.6:
            return

        M = np.array(frames)
        M = M[np.logical_and(M[:,0] > last_ts - 0.5, M[:,0] < last_ts + 0.5)]
        if M.size == 0:
            M = np.array(frames)
            M = M[M[:,0] > last_ts - 0.5]
        Mtime = M[:,0]
        value = M[:,1]
        self.last_ts[buffer_id] = Mtime[-1]
        if Mtime[-1] - Mtime[0] < 0.3:
            return
        sample_time = np.arange(Mtime[0], Mtime[-1], 1 / 2400)
        sample_value = interpl(Mtime, value, sample_time[:-1], 'nearest')
        sample_time = sample_time[:-1]
        sample_value_smooth = smooth(sample_value, 41)
        sample_value_DCremove = smooth(sample_value - sample_value_smooth, 5)

        value = np.zeros((10, 1))
        step = 10
        for i in range(10):
            temp_sample = sample_value_DCremove[i:self.len_e:step]
            value[i] = max(temp_sample) - min(temp_sample)
        std_min = max(value)
        shift_index = np.where(value==std_min)[0][0]
        sample_wave = myResampler(sample_value_DCremove, shift_index, step)
        temp_sample = sample_wave
        # sample_wave = sample_value_DCremove[shift_index:self.len_e:step]
        # temp_sample = sample_value_DCremove[shift_index:self.len_e:step]

        bit_stream = sample_wave <= np.mean(temp_sample)
        bit_stream = bit_stream.astype(int)
        result = chunk_decode(bit_stream)
        if result is None:
            result = chunk_decode(bit_stream, flip=True)

        if result is not None:
            return result

def myResampler(data, startIndex, step):
    data_resampled = []
    index = startIndex
    dataLen = len(data)
    while index < dataLen:
        if len(data_resampled) == 0:
            data_resampled.append(data[index])
            index += step
            continue

        tmp = data[index]
        search_range = 3
        if tmp*data_resampled[-1] < 0:
            local_region = data[max(0,index-search_range):min(dataLen,index+search_range)]
            if tmp > data_resampled[-1]:
                tmp = max(local_region)
            elif tmp < data_resampled[-1]:
                tmp = min(local_region)
            tmp_index = np.where(local_region == tmp)
            tmp_index = tmp_index[0][0]
            data_resampled.append(tmp)
            index = index + (tmp_index-4) + step
            continue

        data_resampled.append(tmp)
        index += step
    return data_resampled