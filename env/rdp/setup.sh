# 1. 停止服务并清理旧的损坏配置
systemctl --user stop gnome-remote-desktop.service
rm -rf ~/.local/share/gnome-remote-desktop/

# 2. 重新创建配置目录
mkdir -p ~/.local/share/gnome-remote-desktop/certificates

# 3. 生成新的自签名证书 (解决证书无效报错)
openssl req -x509 -newkey rsa:4096 -keyout ~/.local/share/gnome-remote-desktop/tls.key -out ~/.local/share/gnome-remote-desktop/certificates/rdp-tls.crt -nodes -days 3650 -subj "/CN=Ubuntu-RDP"

# 4. 通过 gsettings 开启桌面共享模式
gsettings set org.gnome.desktop.remote-desktop.rdp enable true
gsettings set org.gnome.desktop.remote-desktop.rdp view-only false
gsettings set org.gnome.desktop.remote-desktop.rdp screen-share-mode 'mirror-primary'

# 5. 设置你的远程连接密码 (请将下面的 P@ssw0rd 换成你想要的密码)
grdctl rdp set-password "P@ssw0rd"

# 6. 重启服务
systemctl --user restart gnome-remote-desktop.service

# 7. 查看结果
sleep 2
grdctl status