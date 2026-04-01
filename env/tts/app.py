from flask import Flask, jsonify, request
import subprocess
import signal
import os
import sys
import json
import threading
import time
import queue
from collections import deque
import atexit

app = Flask(__name__)

# 全局变量
current_process = None
play_queue = deque(maxlen=1)  # 使用双端队列作为缓存，最大长度为1
is_playing = False
queue_lock = threading.Lock()

# 这里设置您的固定命令前缀
COMMAND_PREFIX = "ekho -v Mandarin --speed=10"

def kill_process():
    """终止当前正在运行的进程"""
    global current_process, is_playing
    if current_process is not None:
        try:
            if sys.platform == "win32":
                # Windows 系统
                current_process.terminate()
            else:
                # Linux/Mac 系统
                os.killpg(os.getpgid(current_process.pid), signal.SIGTERM)
            current_process.wait(timeout=5)
        except Exception as e:
            # 如果终止失败，尝试强制杀死
            try:
                current_process.kill()
                current_process.wait(timeout=5)
            except:
                pass
        finally:
            current_process = None
            is_playing = False
    return True

def start_process(content):
    """启动新进程，将 content 参数添加到命令中"""
    global current_process, is_playing
    try:
        is_playing = True
        if sys.platform == "win32":
            # Windows 下，确保参数用引号括起来
            full_command = f'{COMMAND_PREFIX} "{content}"'
            current_process = subprocess.Popen(full_command, shell=True, creationflags=subprocess.CREATE_NEW_PROCESS_GROUP)
        else:
            # Linux/Mac 下
            import shlex
            cmd_list = COMMAND_PREFIX.split(' ')
            cmd_list.append(content)
            current_process = subprocess.Popen(cmd_list, shell=False, preexec_fn=os.setsid)

        # 启动线程监控进程结束
        monitor_thread = threading.Thread(target=monitor_process, args=(current_process, content))
        monitor_thread.daemon = True
        monitor_thread.start()

        return True
    except Exception as e:
        print(f"启动进程失败: {e}")
        is_playing = False
        return False

def monitor_process(process, current_content):
    """监控进程状态，播完后检查缓存队列"""
    global current_process, is_playing, play_queue

    # 等待进程结束
    process.wait()

    with queue_lock:
        # 进程结束后，更新状态
        if current_process == process:
            current_process = None
            is_playing = False

        # 检查缓存队列中是否有待播放的内容
        if play_queue:
            next_content = play_queue.popleft()
            print(f"正在播放的内容已结束，开始播放缓存内容: {next_content}")
            # 启动播放缓存内容
            time.sleep(0.1)  # 短暂延迟，避免冲突
            start_process(next_content)

@app.route('/cast', methods=['GET'])
def cast():
    """处理 /cast 接口：判断当前是否有正在运行命令，若有则缓存，若没有则运行命令"""
    global is_playing, play_queue

    # 获取 content 参数
    content = request.args.get('content', '')

    # 设置 CORS 头以支持跨域调用
    headers = {
        'Access-Control-Allow-Origin': '*',
        'Access-Control-Allow-Methods': 'GET',
        'Access-Control-Allow-Headers': 'Content-Type'
    }

    # 检查 content 参数是否为空
    if not content:
        response = {
            "status": "error",
            "message": "content 参数不能为空"
        }
        return jsonify(response), 400, headers

    with queue_lock:
        if is_playing:
            # 当前有内容正在播放，缓存新请求
            if play_queue:
                old_content = play_queue.pop()  # 移除旧缓存
                print(f"缓存队列已满，移除旧缓存: {old_content}")

            play_queue.append(content)
            print(f"正在播放中，已将内容加入缓存: {content}")

            response = {
                "status": "cached",
                "message": f"正在播放中，已缓存新内容: {content}",
                "cached_content": content
            }
            return jsonify(response), 200, headers
        else:
            # 当前没有内容播放，直接启动
            success = start_process(content)

            if success:
                response = {
                    "status": "success",
                    "message": f"已开始播放: {content}"
                }
                return jsonify(response), 200, headers
            else:
                response = {
                    "status": "error",
                    "message": "启动播放失败"
                }
                return jsonify(response), 500, headers

@app.route('/status', methods=['GET'])
def get_status():
    """获取当前播放状态和缓存队列"""
    headers = {
        'Access-Control-Allow-Origin': '*',
        'Access-Control-Allow-Methods': 'GET',
        'Access-Control-Allow-Headers': 'Content-Type'
    }

    with queue_lock:
        response = {
            "status": "success",
            "is_playing": is_playing,
            "cached_content": list(play_queue) if play_queue else [],
            "has_cache": len(play_queue) > 0
        }
        return jsonify(response), 200, headers

@app.route('/clear', methods=['GET'])
def clear_cache():
    """清除缓存队列"""
    headers = {
        'Access-Control-Allow-Origin': '*',
        'Access-Control-Allow-Methods': 'GET',
        'Access-Control-Allow-Headers': 'Content-Type'
    }

    with queue_lock:
        if play_queue:
            cleared = list(play_queue)
            play_queue.clear()
            response = {
                "status": "success",
                "message": "缓存已清除",
                "cleared_content": cleared
            }
        else:
            response = {
                "status": "success",
                "message": "缓存队列已为空",
                "cleared_content": []
            }
        return jsonify(response), 200, headers

@app.route('/stop', methods=['GET'])
def stop_playback():
    """停止当前播放并清除缓存"""
    headers = {
        'Access-Control-Allow-Origin': '*',
        'Access-Control-Allow-Methods': 'GET',
        'Access-Control-Allow-Headers': 'Content-Type'
    }

    with queue_lock:
        # 停止当前播放
        kill_process()

        # 清除缓存
        if play_queue:
            cleared = list(play_queue)
            play_queue.clear()
            response = {
                "status": "success",
                "message": "已停止播放并清除缓存",
                "cleared_content": cleared
            }
        else:
            response = {
                "status": "success",
                "message": "已停止播放，缓存队列为空"
            }
        return jsonify(response), 200, headers

@app.after_request
def after_request(response):
    """添加 CORS 头到所有响应"""
    response.headers.add('Access-Control-Allow-Origin', '*')
    response.headers.add('Access-Control-Allow-Headers', 'Content-Type,Authorization')
    response.headers.add('Access-Control-Allow-Methods', 'GET,PUT,POST,DELETE,OPTIONS')
    return response

# 程序退出时清理
def cleanup():
    """程序退出时清理进程"""
    kill_process()

# 注册退出处理
atexit.register(cleanup)

if __name__ == '__main__':
    # 运行 Flask 应用
    app.run(host='localhost', port=5000, debug=False)