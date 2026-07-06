#!/usr/bin/env python3
# -*- coding: utf-8 -*-
import cv2

DEVICE = 0 # device 선택 필요
WIDTH, HEIGHT = 640, 480
OUTPUT = "frame.jpg"

cap = cv2.VideoCapture(DEVICE, cv2.CAP_V4L2)
if not cap.isOpened():
    raise SystemExit("카메라를 열 수 없습니다 (/dev/video%d 확인)" % DEVICE)

# FPGA 설정에 맞는 해상도 
cap.set(cv2.CAP_PROP_FRAME_WIDTH, WIDTH)
cap.set(cv2.CAP_PROP_FRAME_HEIGHT, HEIGHT)

ok, frame = cap.read()          
cap.release()

if not ok:
    raise SystemExit("프레임을 받아오지 못했습니다")

cv2.imwrite(OUTPUT, frame)      
print("saved:", OUTPUT, frame.shape)
