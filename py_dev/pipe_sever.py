## for test
## ignored

from multiprocessing import Queue, Process
import time
import random
import win32file
import win32pipe

def func_write():
    PIPE_NAME = r'\\.\pipe\test_pipe'
    PIPE_BUFFER_SIZE = 1
    named_pipe = win32pipe.CreateNamedPipe(PIPE_NAME,
                    win32pipe.PIPE_ACCESS_DUPLEX,
                    win32pipe.PIPE_TYPE_MESSAGE | win32pipe.PIPE_WAIT | win32pipe.PIPE_READMODE_MESSAGE,
                    win32pipe.PIPE_UNLIMITED_INSTANCES,
                    PIPE_BUFFER_SIZE,
                    PIPE_BUFFER_SIZE, 500, None)
    win32pipe.ConnectNamedPipe(named_pipe, None)
    while True:
        win32file.WriteFile('1'.encode())

def func_read():
    PIPE_NAME = r'\\.\pipe\test_pipe2'
    PIPE_BUFFER_SIZE = 1
    named_pipe = win32pipe.CreateNamedPipe(PIPE_NAME,
                    win32pipe.PIPE_ACCESS_DUPLEX,
                    win32pipe.PIPE_TYPE_MESSAGE | win32pipe.PIPE_WAIT | win32pipe.PIPE_READMODE_MESSAGE,
                    win32pipe.PIPE_UNLIMITED_INSTANCES,
                    PIPE_BUFFER_SIZE,
                    PIPE_BUFFER_SIZE, 500, None)
    win32pipe.ConnectNamedPipe(named_pipe, None)
    while True:
        res = win32file.ReadFile(named_pipe, 1)
        

if __name__ == '__main__':
    p = Process(target=func_read)
    p2 = Process(target=func_write)
    p.start()
    p2.start()