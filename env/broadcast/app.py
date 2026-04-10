import asyncio
import websockets
import pyaudio
import numpy as np
import struct
import json
from datetime import datetime
import threading
from queue import Queue
import time

class AudioStreamServer:
    def __init__(self, host='0.0.0.0', port=17712):
        # 音频参数
        self.sample_rate = 16000
        self.channels = 1
        self.format = pyaudio.paInt16
        self.chunk_size = 1024
        self.host = host
        self.port = port

        # 音频播放
        self.p = pyaudio.PyAudio()
        self.stream = None
        self.clients = set()

        # 音频缓冲
        self.audio_buffer = Queue(maxsize=20)

        # 启动播放线程
        self.playback_thread = threading.Thread(target=self._playback_loop, daemon=True)
        self.playback_running = True
        self.playback_thread.start()

    def _playback_loop(self):
        """音频播放循环"""
        while self.playback_running:
            if not self.audio_buffer.empty():
                try:
                    audio_data = self.audio_buffer.get(timeout=0.1)

                    if self.stream is None:
                        # 初始化音频输出流
                        self.stream = self.p.open(
                            format=self.format,
                            channels=self.channels,
                            rate=self.sample_rate,
                            output=True,
                            frames_per_buffer=self.chunk_size
                        )

                    # 播放音频
                    if audio_data is not None and len(audio_data) > 0:
                        # 确保数据长度是2的倍数
                        if len(audio_data) % 2 != 0:
                            # 如果不是2的倍数，补零
                            audio_data = audio_data + b'\x00'

                        # 将字节数据转换为numpy数组
                        audio_array = np.frombuffer(audio_data, dtype=np.int16)

                        # 播放音频
                        self.stream.write(audio_array.tobytes())

                except Exception as e:
                    print(f"播放音频时出错: {e}")
            else:
                time.sleep(0.01)  # 避免CPU占用过高

    async def handle_audio_stream(self, websocket):
        """处理音频流连接"""
        client_id = f"{websocket.remote_address[0]}:{websocket.remote_address[1]}"
        print(f"[{datetime.now().strftime('%H:%M:%S')}] 客户端连接: {client_id}")

        self.clients.add(websocket)

        try:
            async for message in websocket:
                if isinstance(message, bytes):
                    # 将音频数据放入缓冲队列
                    if not self.audio_buffer.full():
                        self.audio_buffer.put(message)

                    # 计算音频时长
                    duration_ms = (len(message) * 1000) / (self.sample_rate * 2)  # 2 bytes per sample

                    # 调试信息
                    if hasattr(websocket, 'last_log') and (time.time() - websocket.last_log) > 1:
                        print(f"[{client_id}] 收到音频数据: {len(message)} 字节, 时长: {duration_ms:.1f}ms")
                        websocket.last_log = time.time()
                    elif not hasattr(websocket, 'last_log'):
                        websocket.last_log = time.time()

        except websockets.exceptions.ConnectionClosed as e:
            print(f"[{datetime.now().strftime('%H:%M:%S')}] 客户端断开: {client_id}, 原因: {e}")
        except Exception as e:
            print(f"[{datetime.now().strftime('%H:%M:%S')}] 处理客户端 {client_id} 时出错: {e}")
        finally:
            self.clients.discard(websocket)

    async def start_server(self):
        """启动WebSocket服务器"""
        server = await websockets.serve(
            self.handle_audio_stream,
            self.host,
            self.port,
            ping_interval=None
        )

        print(f"=" * 50)
        print(f"音频广播服务器已启动")
        print(f"监听地址: ws://{self.host}:{self.port}")
        print(f"采样率: {self.sample_rate}Hz, 声道数: {self.channels}")
        print(f"按 Ctrl+C 停止服务器")
        await server.wait_closed()

    def cleanup(self):
        """清理资源"""
        self.playback_running = False
        if self.playback_thread.is_alive():
            self.playback_thread.join(timeout=2)

        if self.stream:
            self.stream.stop_stream()
            self.stream.close()

        self.p.terminate()

        print("服务器资源已清理")

async def main():
    server = AudioStreamServer()

    try:
        await server.start_server()
    except KeyboardInterrupt:
        print("\n收到停止信号，正在关闭服务器...")
    except Exception as e:
        print(f"服务器运行出错: {e}")
    finally:
        server.cleanup()

if __name__ == "__main__":
    asyncio.run(main())