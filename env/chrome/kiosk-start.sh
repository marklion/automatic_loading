#!/bin/bash
# 等待系统完全启动
sleep 2

# 启动Chrome信息亭模式
#microsoft-edge --kiosk http://localhost/#/video_cast --edge-kiosk-type=fullscreen
google-chrome --kiosk http://localhost/#/video_cast