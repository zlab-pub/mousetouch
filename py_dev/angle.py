import numpy as np
import math
import time
from PIL import Image
from skimage.transform import hough_line, hough_line_peaks
from PIL import Image, ImageEnhance
from comm_handler import TypeID
import matplotlib.pyplot as plt

ShowHoughTransformResult = False

# old version: red -> background1, white -> background2
BACKGROUND1 = 'red'
BACKGROUND2 = 'blue'

FRAME_WIDTH = 30
FRAME_HEIGHT = 30
RAW_FRAME_SIZE = 900

RESOLUTION = (FRAME_HEIGHT, FRAME_WIDTH)
COLOR_CHANGE_INTERVAL = 1.5 # seconds

comName = 'COM5'
baudRate = 115200
posisionHeader = b'\x00\xfe\xfe\x00\xfe'
angleHeader = b'\x00\xff\x00\xff\xff'

fig = plt.figure('1')

def get_threshold(im, ratio):
    threshold = 0.5
    im = np.array(im)
    while sum(sum(im > threshold)) > sum(sum(im >= 0)) * (1-ratio):
        threshold += 1
    return threshold

def weight_image(im):
    width = np.size(im, 0)
    height = np.size(im, 1)
    im_tmp = np.zeros((width+2, height+2))
    im_tmp[1:width+1,1:height+1] = im
    ret = np.zeros((width, height))
    for i in range(width):
        for j in range(height):
            if im_tmp[1+j,1+i] != 0:
                ret[j,i] = max(0, sum(sum(im_tmp[j:j+3,i:i+3])) - 1) * 32
                if ret[j,i] > 255:
                    ret[j,i] = 255
    return ret


class AngleMeasurer:
    def __init__(self):
        self.reset()
        self.w_count = 0
    
    def reset(self):
        self.img = Image.new("RGB", RESOLUTION)
        self.red_img = None
        self.white_img = None
        self.collect = 0
        self.cur = BACKGROUND1
        self.red_angle = None
        self.red_lines = None
        self.r_ang = None
        self.w_ang = None
        self.raw_rx = None
        self.raw_wx = None
        self.r_rhos = None
        self.w_rhos = None
        self.r_votes = None
        self.w_votes = None
        # self.w_count = 0

    def calc_angle(self, im_raw, mode, ref=None):
        BORDER = 6
        im = ImageEnhance.Contrast(im_raw).enhance(4)
        GAUSSIAN_STRENGTH = 3
        MIN_DISTANCE = 4
        MIN_ANGLE = 0
        NUM_PEAKS = 4
        TOLERATE_DIFF = 2

        im = np.array(im.convert('L'))
        
        th = get_threshold(im, ratio=0.70)
        BW_ = im > th
        BW_temp = weight_image(BW_)

        BW = np.zeros((FRAME_HEIGHT+BORDER, FRAME_WIDTH+BORDER))
        BW[int(BORDER/2):FRAME_HEIGHT+BORDER-int(BORDER/2),int(BORDER/2):FRAME_WIDTH+BORDER-int(BORDER/2)] = BW_temp

        tested_angles = np.linspace(-np.pi / 2, np.pi / 2, 360, endpoint=False)
        H, T, R = hough_line(BW, theta=tested_angles)

        if ShowHoughTransformResult:
            fig2, (ax1, ax2, ax3) = plt.subplots(1,3)
            fig2.suptitle(mode)
            ax1.set_title("im")
            ax1.imshow(im, cmap ='gray')

            ax2.set_title("Bin")
            ax2.imshow(BW_, cmap ='gray')

            ax3.imshow(BW, cmap='gray')
            ax3.set_ylim((BW.shape[0], 0))
            ax3.set_axis_off()
            ax3.set_title('Detected lines')
            for _, angle, dist in zip(*hough_line_peaks(H, T, R, num_peaks=NUM_PEAKS, min_distance=MIN_DISTANCE)):
                (x0, y0) = dist * np.array([np.cos(angle), np.sin(angle)])
                ax3.axline((x0, y0), slope=np.tan(angle + np.pi/2))
            plt.pause(0.1)
            input("enter to continue")
            plt.close(fig2)

        hspace, angles, dists = hough_line_peaks(H, T, R, min_distance=MIN_DISTANCE, num_peaks=NUM_PEAKS)
        H_weight = hspace.copy()
        top = min(max(5, sum(H_weight==max(H_weight))), len(angles))
        for i in range(top):
            for j in range(len(hspace)):
                if j == i:
                    continue
                a1 = math.degrees(angles[i])
                a2 = math.degrees(angles[j])
                if mode == BACKGROUND1:
                    if abs(a1 - a2) < TOLERATE_DIFF:
                        H_weight[i] = H_weight[i] + hspace[j]
                elif mode == BACKGROUND2:
                    if abs(a1 - a2) < TOLERATE_DIFF or abs(abs(a1 - a2) - 90) < TOLERATE_DIFF:
                        H_weight[i] = H_weight[i] + hspace[j]
        tmp = H_weight.tolist()
        index = tmp.index(max(tmp))
        x = math.degrees(angles[index])
        rho = dists[index]
        theta = angles[index]
        ang = - x + 90

        ag = theta
        if ref is not None:
            ag = ref
        rows, cols = H.shape
        rhos = [[], []]
        votes = [[], []]
        for i in range(rows):
            for j in range(cols):
                if math.degrees(abs(T[j] - ag)) < 1 and H[i][j] > 4:
                    rhos[0].append(R[i])
                    votes[0].append(H[i][j])
                if abs(180 - math.degrees(abs(T[j] - ag))) < 1 and H[i][j] > 4:
                    rhos[0].append(R[i])
                    votes[0].append(H[i][j])
                if abs(90 - math.degrees(abs(T[j] - ag))) < 2 and H[i][j] > 4:
                    rhos[1].append(R[i])
                    votes[1].append(H[i][j])
        # rhos[0]: rhos of the similar angle as returned one
        # rhos[1]: rhos of the verticle angle as returned one
        return ang, rhos, votes, x, BW, H, rho

    # update with raw values (FRAME_WIDTH*FRAME_HEIGHT 1D list)
    def update(self, vals, typeID):
        if typeID == TypeID.ANGLE_WHITE: self.cur = BACKGROUND2
        elif typeID == TypeID.ANGLE: self.cur = BACKGROUND1
        count = 0
        print('[' + str(time.time()) + ']angle: received values', self.cur)
        for v in vals:
            i = math.floor(count / RESOLUTION[0])
            j = count % RESOLUTION[1]
            self.img.putpixel((i, j), (v, ) * 3)
            count += 1

        if self.collect != 0:
            self.collect -= 1
            return

        self.img.save(self.cur +str(time.time()) + '.png')

        if self.cur == BACKGROUND1:
            # ang, rhos, votes, x, BW, H, rho
            self.r_ang, self.r_rhos, self.r_votes, self.raw_rx, \
                _, _, _ = self.calc_angle(self.img, BACKGROUND1)
            self.red_img = self.img.copy()
            self.collect = 0
            self.cur = BACKGROUND2
            return TypeID.ANGLE_COLOR2WHITE, 0
        else:
            self.w_ang, self.w_rhos, self.w_votes, self.raw_wx, \
                _, _, _ = self.calc_angle(self.img, BACKGROUND2)
            ang = self.get_angle()
            if ang is None:
                return
            self.white_img = self.img.copy()
            print('[' + str(time.time()) + ']angle:', ang)
            self.reset()
            return TypeID.ANGLE, ang

    # update with Image type
    def update_(self, img, typeID):
        if typeID == TypeID.ANGLE_WHITE: self.cur = BACKGROUND2
        elif typeID == TypeID.ANGLE: self.cur = BACKGROUND1
        
        self.img = img

        if self.collect != 0:
            self.collect -= 1
            return
        
        if self.cur == BACKGROUND1:
            # ang, rhos, votes, x, BW, H, rho
            self.r_ang, self.r_rhos, self.r_votes, self.raw_rx, \
                _, _, _ = self.calc_angle(self.img, BACKGROUND1)
            self.red_img = self.img.copy()
            self.collect = 0
            self.cur = BACKGROUND2
            return TypeID.ANGLE_COLOR2WHITE, 0
        else:
            self.w_ang, self.w_rhos, self.w_votes, self.raw_wx, \
                _, _, _ = self.calc_angle(self.img, BACKGROUND2)
            ang = self.get_angle()
            if ang is None:
                return
            self.white_img = self.img.copy()
            print('[' + str(time.time()) + ']angle:', ang)
            self.reset()
            return TypeID.ANGLE, ang
            # return TypeID.ANGLE, 30

    def get_angle(self):
        r_ang=self.r_ang
        w_ang=self.w_ang
        raw_wx = self.raw_wx
        r_rhos=self.r_rhos
        w_rhos=self.w_rhos
        w_votes = self.w_votes
        r_votes = self.r_votes
        rhos = [] # 0: white 1: red
        votes = [] # 0: white 1: red
        if abs(r_ang - w_ang) <= 45 or abs(180 - abs(r_ang - w_ang)) <= 45:
            ang = w_ang
            rhos = [w_rhos[0]]
            votes = [w_votes[0]]
            ref = math.radians(raw_wx)
        else:
            ang = w_ang + 90
            raw_wx = raw_wx + 90 if raw_wx < 0 else raw_wx - 90
            rhos = [w_rhos[1]]
            votes = [w_votes[1]]
            ref = raw_wx 
            ref = math.radians(ref)
        r_ang, r_rhos, r_votes, raw_rx, _, _, _ = self.calc_angle(self.red_img, BACKGROUND1, ref=ref)
        rhos = rhos + [r_rhos[0]]
        votes = votes + [r_votes[0]]

        if not (rhos[0] != [] and rhos[1] != []):
            print('angle detecting fail 1')
            return

        white_rho_blocks = []
        tmp = []
        for rho_i in range(len(rhos[0])):
            r = rhos[0][rho_i]
            v = votes[0][rho_i]
            if v > 8:
                if len(tmp) > 0:
                    if abs(r -tmp[-1]) < 2:
                        tmp.append(r)
                    else:
                        white_rho_blocks.append(tmp)
                        tmp = []
                else:
                    tmp.append(r)
            elif tmp != []:
                white_rho_blocks.append(tmp)
                tmp = []

        red_rho_blocks = []
        tmp = []
        for rho_i in range(len(rhos[1])):
            r = rhos[1][rho_i]
            v = votes[1][rho_i]
            if v > 8:
                if len(tmp) > 0:
                    if abs(r -tmp[-1]) < 2:
                        tmp.append(r)
                    else:
                        if red_rho_blocks != []:
                            if np.mean(tmp) != np.mean(red_rho_blocks[-1]):
                                red_rho_blocks.append(tmp)
                        else:
                            red_rho_blocks.append(tmp)
                        tmp = []
                else:
                    tmp.append(r)
            elif tmp != []:
                if red_rho_blocks != []:
                    if np.mean(tmp) != np.mean(red_rho_blocks[-1]):
                        red_rho_blocks.append(tmp)
                else:
                    red_rho_blocks.append(tmp)
                tmp = []
    
        min_dists = []
        for w_block in white_rho_blocks:
            for r_block in red_rho_blocks:
                mid_w = (max(w_block)+min(w_block)) / 2
                mid_r = (max(r_block) + min(r_block)) / 2
                if abs(mid_w - mid_r) < 3 and mid_w != mid_r:
                    min_dists.append(mid_w - mid_r)
        
        if min_dists == []:
            for w_block in white_rho_blocks:
                for r_block in red_rho_blocks:
                    mid_w = min(w_block)
                    mid_r = max(r_block)
                    if abs(mid_w - mid_r) < 3 and mid_w != mid_r:
                        min_dists.append(mid_w - mid_r)

        if min_dists == []:
            print('angle detecting fail 2')
            return
        positive_num, negative_num = 0, 0
        for num in min_dists:
            if num > 0:
                positive_num += 1
            else:
                negative_num += 1
        positive = np.mean(min_dists) > 0
        if ang > 180:
            ang -= 180
        ang -= 90
        if ang < 0:
            ang += 180
        theta_positive = raw_wx > 0
        if theta_positive and positive:
            return ang + 180
        elif theta_positive and not positive:
            return ang
        elif not theta_positive and not positive:
            return ang + 180
        elif not theta_positive and positive:
            return ang
