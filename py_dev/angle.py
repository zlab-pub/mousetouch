import numpy as np
import math
import time
from collections import Counter
from cv2 import cv2
from PIL import Image
from skimage.transform import hough_line, hough_line_peaks
from PIL import Image, ImageEnhance
from scipy.ndimage.filters import gaussian_filter
from comm_handler import TypeName, TypeID


RESOLUTION = (19, 19)
COLOR_CHANGE_INTERVAL = 1.5 # seconds


def pil2cv(im):
    im = np.array(im)
    return cv2.cvtColor(im, cv2.COLOR_RGB2BGR)


def cv2pil(im):
    im = cv2.cvtColor(im, cv2.COLOR_BGR2RGB)
    return Image.fromarray(im)


def detect_line(im, threshold, mode, ref=None, vote=1):
    img = pil2cv(im)
    gray = cv2.cvtColor(img,cv2.COLOR_BGR2GRAY)
    edges = gray.copy()
    edges[gray<threshold] = 255
    edges[gray>threshold] = 0
    
    lines = cv2.HoughLines(edges, 1, np.pi/180, vote)
    if lines is None:
        return im, [], None

    remained_lines, angles = _detect_line(lines, ref)
    if remained_lines is None and mode=='red':
        return detect_line(im, threshold, mode, ref, vote+1)

    for l in remained_lines:
        (x1,y1), (x2,y2) = l
        cv2.line(img, (x1,y1), (x2,y2), (0,0,255), 1) # red line

    return cv2pil(img), angles, remained_lines


def _detect_line(lines, ref=None):
    angles = []
    ls = []
    for line in lines[:10]:
        for rho,theta in line:
            a = np.cos(theta)
            b = np.sin(theta)
            x0 = a * rho
            y0 = b * rho
            x1 = int(x0 + 1000*(-b))
            y1 = int(y0 + 1000*(a))
            x2 = int(x0 - 1000*(-b))
            y2 = int(y0 - 1000*(a))
            angle = math.degrees(math.atan2(y2 - y1, x2 - x1))
            angle += 90
            if angle < 0:
                angle += 360
            if angle > 360:
                angle -= 360
            if angle > 180:
                angle -= 180
            angles.append(angle)
            ls.append(((x1,y1), (x2,y2)))
            
    if ref is not None:
        # print('ref: ' + str(ref))
        valid_angle = []
        remained_index = []
        for i in range(len(angles)):
            ag = angles[i]
            if abs(ag - ref) < 6:
                valid_angle.append(ag)
                remained_index.append(i)
        if valid_angle != []:
            angles = valid_angle
        remained_lines = [ls[i] for i in remained_index]
        return remained_lines, angles
        
    most_comm_ele = Counter(angles).most_common()[0][0]
    remained_index = [[], []]
    two_set_angles = [[], []]
    for i in range(len(angles)):
        ag = angles[i]
        if abs(ag - most_comm_ele) < 6:
            remained_index[0].append(i)
            two_set_angles[0].append(ag)
        if abs(90 - abs(ag - most_comm_ele)) < 6:
            remained_index[1].append(i)
            two_set_angles[1].append(ag)

    if ref is not None:
        remained_lines = [ls[i] for i in remained_index[0]]
        return remained_lines, two_set_angles[0]
    elif two_set_angles[0] == []:
        remained_lines = [ls[i] for i in remained_index[1]]
        return remained_lines, two_set_angles[1]
    elif two_set_angles[1] == []:
        remained_lines = [ls[i] for i in remained_index[0]]
        return remained_lines, two_set_angles[0]
    else:
        return None, []


def change_cur(cur):
    if cur == 'red':
        # print('changing to white...')
        time.sleep(COLOR_CHANGE_INTERVAL)
        # print('done')
        return 'white'
    else:
        # print('changing to red...')
        time.sleep(COLOR_CHANGE_INTERVAL)
        # print('done')
        return 'red'


# def get_threshold(im, mode):
#     ratio = 3 / 2 if mode == 'white' else 2.5 / 2
#     threshold = 10
#     im = np.array(im)
#     while sum(sum(sum(im > threshold))) > sum(sum(sum(im >= 0))) / ratio:
#         threshold += 10
#     return threshold
def get_threshold(im, ratio=0.8):
    # ratio = 0.5 if mode == 'white' else 0.7
    threshold = 10
    im = np.array(im)
    while sum(sum(im > threshold)) > sum(sum(im >= 0)) * ratio:
        threshold += 1
    return threshold

class AngleMeasurer:
    def __init__(self):
        self.img = Image.new("RGB", RESOLUTION)
        self.red_img = None
        self.white_img = None
        self.collect = 0
        self.cur = 'red'
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
        self.w_count = 0

        # self.white_angle = None
        # self.white_lines = None
    
    def reset(self):
        self.img = Image.new("RGB", RESOLUTION)
        self.red_img = None
        self.white_img = None
        self.collect = 0
        self.cur = 'red'
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
        # mode = self.cur
        BORDER = 6
        if mode == 'white':
            im = im_raw
            GAUSSIAN_STRENGTH = 1.5
            MIN_DISTANCE = 0
            MIN_ANGLE = 0
            NUM_PEAKS = 20
            TOLERATE_DIFF = 2
        elif mode == 'red':
            im = ImageEnhance.Contrast(im_raw).enhance(4)
            GAUSSIAN_STRENGTH = 3
            MIN_DISTANCE = 2
            MIN_ANGLE = 1
            NUM_PEAKS = 10
            TOLERATE_DIFF = 3

        im = np.array(im.convert('L'))
        I_DC = gaussian_filter(im, GAUSSIAN_STRENGTH)
        I_DC = I_DC - np.min(I_DC)
        I_copy = np.zeros((19+BORDER,19+BORDER), np.uint8)
        I_copy[int(BORDER/2):19+BORDER-int(BORDER/2),int(BORDER/2):19+BORDER-int(BORDER/2)] = im
        I_DCremove = im - I_DC
        
        if mode == 'white':
            th = np.median(I_DCremove) - 6
        elif mode == 'red':
            th = get_threshold(I_DCremove)

        BW_temp = I_DCremove < th
        BW = np.zeros((19+BORDER, 19+BORDER))
        BW[int(BORDER/2):19+BORDER-int(BORDER/2),int(BORDER/2):19+BORDER-int(BORDER/2)] = BW_temp
        H, T, R = hough_line(BW * 255)
        hspace, angles, dists = hough_line_peaks(H, T, R, min_distance=MIN_DISTANCE, min_angle=MIN_ANGLE, num_peaks=NUM_PEAKS)
        H_weight = hspace.copy()
        top = min(max(5, sum(H_weight==max(H_weight))), len(angles))
        for i in range(top):
            for j in range(len(hspace)):
                if j == i:
                    continue
                a1 = math.degrees(angles[i])
                a2 = math.degrees(angles[j])
                if mode == 'red':
                    if abs(a1 - a2) < TOLERATE_DIFF:
                        H_weight[i] = H_weight[i] + hspace[j]
                elif mode == 'white':
                    if abs(a1 - a2) < TOLERATE_DIFF or abs(abs(a1 - a2) - 90) < TOLERATE_DIFF:
                        H_weight[i] = H_weight[i] + hspace[j]
        tmp = H_weight.tolist()
        index = tmp.index(max(tmp))
        x = math.degrees(angles[index])
        rho = dists[index]
        theta = angles[index]
        ang = - x + 90
        # points = calc_points(rho, theta)
        # rtn = I_copy
        # rtn = draw_lines(I_copy, (((points[0], points[1]), (points[2], points[3])),))

        ag = theta
        if ref is not None:
            ag = ref
        rows, cols = H.shape
        rhos = [[], []]
        votes = [[], []]
        for i in range(rows):
            for j in range(cols):
                if math.degrees(abs(T[j] - ag)) < 1 and H[i][j] > 4:
                    # print('same angle')
                    rhos[0].append(R[i])
                    votes[0].append(H[i][j])
                if abs(180 - math.degrees(abs(T[j] - ag))) < 1 and H[i][j] > 4:
                    # print('same angle')
                    rhos[0].append(R[i])
                    votes[0].append(H[i][j])
                if abs(90 - math.degrees(abs(T[j] - ag))) < 2 and H[i][j] > 4:
                    # print('verticle angle')
                    rhos[1].append(R[i])
                    votes[1].append(H[i][j])
        # rhos[0]: rhos of the similar angle as returned one
        # rhos[1]: rhos of the verticle angle as returned one
        return ang, rhos, votes, x, BW, H, rho


    def update(self, vals, typeID):
        if typeID == TypeID.ANGLE_WHITE: self.cur = 'white'
        elif typeID == TypeID.ANGLE: self.cur = 'red'
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
        # if self.cur == 'white': self.w_count += 1
        # self.img.save(self.cur+str(self.w_count) + '.png')
        # print('decoding ' + self.cur)
        # if self.cur == 'white': return
        
        # threshold = get_threshold(self.img, self.cur)
        
        if self.cur == 'red':
            # ang, rhos, votes, x, BW, H, rho
            self.r_ang, self.r_rhos, self.r_votes, self.raw_rx, \
                _, _, _ = self.calc_angle(self.img, 'red')
            self.red_img = self.img.copy()
            self.collect = 0
            self.cur = 'white'
            return TypeID.ANGLE_COLOR2WHITE, 0
        else:
            self.w_ang, self.w_rhos, self.w_votes, self.raw_wx, \
                _, _, _ = self.calc_angle(self.img, 'white')
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
            ref = raw_wx # + 90 if raw_wx < 0 else raw_wx - 90
            ref = math.radians(ref)
        r_ang, r_rhos, r_votes, raw_rx, _, _, _ = self.calc_angle(self.red_img, 'red', ref=ref)
        rhos = rhos + [r_rhos[0]]
        votes = votes + [r_votes[0]]

        if not (rhos[0] != [] and rhos[1] != []):
            print('angle detecting fail')
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
            print('angle detecting fail')
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
