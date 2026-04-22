<template>
    <div class="login-page">
        <div class="bg-orb bg-orb-left"></div>
        <div class="bg-orb bg-orb-right"></div>

        <section class="login-panel">
            <header class="login-header">
                <h1>自动装车系统</h1>
                <p>欢迎登录控制台</p>
            </header>

            <el-form class="login-form" @submit.prevent>
                <el-form-item>
                    <el-input v-model="form.username" placeholder="请输入用户名" autocomplete="username" size="large"
                        clearable>
                        <template #prefix>
                            <el-icon>
                                <User />
                            </el-icon>
                        </template>
                    </el-input>
                </el-form-item>

                <el-form-item>
                    <el-input v-model="form.password" placeholder="请输入密码" type="password"
                        autocomplete="current-password" show-password size="large" clearable>
                        <template #prefix>
                            <el-icon>
                                <Lock />
                            </el-icon>
                        </template>
                    </el-input>
                </el-form-item>

                <el-button class="login-btn" type="primary" size="large" @click="onLogin">
                    登录
                </el-button>
            </el-form>
        </section>
    </div>
</template>

<script setup>
import { reactive } from 'vue';
import { useStorage } from '@vueuse/core'

const loginInfo = useStorage('al_login_info', {
    username: '',
    password: '',
    expire_time: 0,
})
const form = reactive({
    username: '',
    password: '',
});

function onLogin() {
    // 模拟登录成功，保存登录信息到 localStorage
    loginInfo.value = {
        username: form.username,
        password: form.password,
        expire_time: Date.now() + 7 * 24 * 60 * 60 * 1000, // 设置过期时间为7天
    };
    window.location.href = '/';
}
</script>

<style scoped>
.login-page {
    position: relative;
    min-height: 100vh;
    display: flex;
    align-items: center;
    justify-content: center;
    padding: 24px;
    background:
        radial-gradient(circle at 20% 20%, rgba(32, 98, 240, 0.25), transparent 38%),
        radial-gradient(circle at 85% 18%, rgba(16, 176, 212, 0.25), transparent 42%),
        linear-gradient(135deg, #f2f6ff 0%, #eef8ff 46%, #f9fcff 100%);
    overflow: hidden;
}

.bg-orb {
    position: absolute;
    width: 360px;
    height: 360px;
    border-radius: 50%;
    filter: blur(4px);
    animation: float 8s ease-in-out infinite;
    pointer-events: none;
}

.bg-orb-left {
    left: -110px;
    bottom: -90px;
    background: radial-gradient(circle, rgba(46, 115, 255, 0.35), rgba(46, 115, 255, 0));
}

.bg-orb-right {
    right: -110px;
    top: -80px;
    animation-delay: 1.6s;
    background: radial-gradient(circle, rgba(10, 164, 190, 0.34), rgba(10, 164, 190, 0));
}

.login-panel {
    position: relative;
    z-index: 1;
    width: min(100%, 460px);
    border-radius: 24px;
    padding: 34px 30px 30px;
    background: rgba(255, 255, 255, 0.78);
    backdrop-filter: blur(10px);
    box-shadow: 0 20px 48px rgba(35, 72, 142, 0.16);
    border: 1px solid rgba(255, 255, 255, 0.8);
    animation: panel-in 460ms ease-out;
}

.login-header h1 {
    margin: 0;
    color: #113f8a;
    font-size: clamp(24px, 4vw, 30px);
    line-height: 1.2;
    font-family: "HarmonyOS Sans SC", "PingFang SC", "Noto Sans SC", sans-serif;
    letter-spacing: 0.03em;
}

.login-header p {
    margin: 10px 0 26px;
    color: #4f6386;
    font-size: 15px;
    font-family: "HarmonyOS Sans SC", "PingFang SC", "Noto Sans SC", sans-serif;
}

.login-form {
    display: grid;
    gap: 6px;
}

.login-btn {
    width: 100%;
    margin-top: 8px;
    height: 46px;
    border: none;
    border-radius: 12px;
    font-size: 16px;
    font-weight: 700;
    letter-spacing: 0.08em;
    background: linear-gradient(90deg, #1f67ea 0%, #0eaec6 100%);
}

.login-btn:hover {
    filter: brightness(1.04);
}

@keyframes panel-in {
    from {
        transform: translateY(16px) scale(0.98);
        opacity: 0;
    }

    to {
        transform: translateY(0) scale(1);
        opacity: 1;
    }
}

@keyframes float {

    0%,
    100% {
        transform: translateY(0);
    }

    50% {
        transform: translateY(-14px);
    }
}

@media (max-width: 768px) {
    .login-page {
        padding: 18px;
    }

    .login-panel {
        width: 100%;
        border-radius: 20px;
        padding: 26px 20px 22px;
    }

    .login-header p {
        margin-bottom: 20px;
    }
}
</style>