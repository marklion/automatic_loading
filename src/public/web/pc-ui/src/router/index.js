import { createRouter, createWebHashHistory } from 'vue-router'
import { useStorage } from '@vueuse/core'
const router = createRouter({
  history: createWebHashHistory(import.meta.env.BASE_URL),
  routes: [
    {
      path: '/login/',
      component: () => import('@/components/Login.vue')
    },
    {
      path: '/',
      component: () => import('@/pages/Home.vue')
    },
    {
      path: '/video_cast/',
      component: () => import('@/pages/VideCast.vue')
    }
  ],
})

const loginInfo = useStorage('al_login_info', {
  username: '',
  password: '',
  expire_time: 0,
})

router.beforeEach(async (to, from) => {
  if (to.path === '/login/') {
    return true;
  }

  let ret = { path: '/login/' };
  let host = window.location.host
  if (host.includes('localhost') || host.includes('127.0.0.1')) {
    ret = true;
  }
  else {
    if (Date.now() < loginInfo.value.expire_time) {
      let resp = await router.call_cli(`process verify_user '${loginInfo.value.username}' '${loginInfo.value.password}'`);
      if (resp.verified) {
        ret = true;
      }
    }
  }

  return ret;
})

export default router
