import sys
import time
import win32pipe
import win32file
from multiprocessing import Queue, Process
from comm_handler import TypeName, TypeID, data_packing, tagging_index

def pipe_client_read(pipe_name, q):
    print("pipe client for reading")
    handle = win32file.CreateFile(pipe_name,
        win32file.GENERIC_READ | win32file.GENERIC_WRITE,
        0, None, win32file.OPEN_EXISTING, 0, None)

    # try:
    while True:
        data = win32file.ReadFile(handle, 1024)
        q.put(data[1])
        # type_id = data[1][0]
        # if type_id == TypeID.POSITION:
        #     t1 = (time.time(), data[1][1])
        #     t2 = (time.time(), data[1][2])
        #     q.put((TypeName.POSITION, t1, t2))
        # elif type_id == TypeID.ANGLE:
        #     resp = win32file.ReadFile(handle, 19 * 19)
        #     q.put((TypeName.ANGLE, bytes_arr2_int_arr(resp)))

    # except:
    #     print('Inpipe Broken')
    #     win32file.CloseHandle(handle)


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
            ret = measurer.update(read_data[1:], func)
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
            ts = int.from_bytes(read_data[-16:-8],'little') / (1e6)
            ts2 = int.from_bytes(read_data[-8:],'little') / (1e6)
            try:
                ret = localizer.update(((ts, read_data[1]), (ts2, read_data[2])))
            # except Exception as e:
            except:
            #     print(e)
                continue
            if ret is not None:
                if time.time() - last_pos_ts > 1:
                    write_queue.put(data_packing(TypeID.POSITION, ret[0]))
                    print(ret[0])
                    last_pos_ts = time.time()
                    

