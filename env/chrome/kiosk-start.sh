#!/bin/bash
# 等待系统完全启动
sleep 2

# 启动Chrome信息亭模式
google-chrome --check-for-update-interval=31536000 --disable-background-networking --noerrdialogs --kiosk http://localhost/#/video_cast