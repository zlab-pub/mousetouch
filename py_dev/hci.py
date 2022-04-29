import sys
import time
import win32file
from multiprocessing import Queue, Process
from comm_handler import TypeID, data_packing, tagging_index
from PIL import Image

def pipe_client_read(pipe_name, q):
    print("pipe client for reading")
    handle = win32file.CreateFile(pipe_name,
        win32file.GENERIC_READ | win32file.GENERIC_WRITE,
        0, None, win32file.OPEN_EXISTING, 0, None)

    while True:
        data = win32file.ReadFile(handle, 1024)
        q.put(data[1])

def pipe_client_write(pipe_name, q, idx):
    print("pipe client for writing")
    handle = win32file.CreateFile(pipe_name,
        win32file.GENERIC_READ | win32file.GENERIC_WRITE,
        win32file.FILE_SHARE_WRITE, None,
        win32file.OPEN_EXISTING, 0, None)
    
    # try:
    while True:
        packet = q.get()
        packet_tagged = tagging_index(packet, int(idx))
        win32file.WriteFile(handle, packet_tagged)
        print('[' + str(time.time()) + ']send data: ',end='')
        print(packet_tagged)
    # except:
    #     print('Outpipe Broken')
    #     win32file.CloseHandle(handle)


if __name__ == '__main__':
    # hci.py idx inpipe outpipe
    assert len(sys.argv) == 4

    MOUSE_INDEX = sys.argv[1]
    INPIPE_NAME = sys.argv[2]
    OUTPIPE_NAME = sys.argv[3]
    print('Detected arguments:', end=' ')
    print(MOUSE_INDEX, INPIPE_NAME, OUTPIPE_NAME)
    
    read_queue = Queue()
    write_queue = Queue()
    # angl_queue = Queue()
    # loc_queue = Queue()

    # create four processes
    read_process = Process(target=pipe_client_read, args=(INPIPE_NAME, read_queue))
    write_process = Process(target=pipe_client_write, args=(OUTPIPE_NAME, write_queue, MOUSE_INDEX))
    # angl_process = Process(target=angl_detect, args=(angl_queue, write_queue))
    # loc_process = Process(target=loc_recog, args=(loc_queue, write_queue))
    for p in [read_process, write_process]: #, angl_process, loc_process]:
        p.start()


    import angle, location
    measurer = angle.AngleMeasurer()
    localizer = location.Localizer()
    last_pos_ts = time.time()
    last_agl_ts = time.time()
    import matplotlib.pyplot as plt
    import numpy as np
    
    # f = open('pos_data.csv', 'w')
    # f2 = open('pos_data2.csv','w')
    update_time = time.time()
    stt = time.time()
    count = 0
    mode = 'ANGLE'
    debug_time = time.time()
    while True:
        # if read_queue.empty():
        #     continue
        read_data = read_queue.get()
        func = read_data[0]
        # print(read_data)
        # print(int.from_bytes(read_data[-8:],'little'))
        if func == TypeID.ANGLE or func == TypeID.ANGLE_WHITE:
            # if time.time() - debug_time > 1:
            #     print('IN AGNLE')
            #     debug_time = time.time()
            if mode == 'LOCATION':
                localizer.reset()
                mode = 'ANGLE'
            # if time.time() - last_agl_ts < 0.4:
            #     continue
                
            # try:
            img_np = np.zeros(900)
            tmp = read_data[1:]
            for i in range(len(tmp)):
                img_np[i] = read_data[i+1]
            img_np = img_np.reshape((30, 30))
            img = Image.fromarray(img_np)
            img = img.convert('L')
            ret = measurer.update_(img, func)
            #ret = measurer.update(read_data[1:], func)
            # except:
            #     continue
            if ret is not None:
                # if time.time() - last_agl_ts > 1:
                write_queue.put(data_packing(ret[0], ret[1]))
                last_agl_ts = time.time()
        elif func == TypeID.POSITION:
            if mode == 'ANGLE':
                measurer.reset()
                mode = 'LOCATION'
            count += 1

            ts = int.from_bytes(read_data[-8:],'little') / (1e6)
            
            ret = localizer.update(ts, read_data[1:-8])
            
            if ret is not None:
                if time.time() - last_pos_ts > 0.3:
                    write_queue.put(data_packing(TypeID.POSITION, ret[0]))
                    print(ret[0])
                    last_pos_ts = time.time()
                    

