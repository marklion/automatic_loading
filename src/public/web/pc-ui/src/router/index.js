import { createRouter, createWebHashHistory } from 'vue-router'

const router = createRouter({
  history: createWebHashHistory(import.meta.env.BASE_URL),
  routes: [{
    path: '/',
    component: () => import('@/pages/Home.vue')
  }, {
    path:'/video_cast/',
    component: () => import('@/pages/VideCast.vue')
  }],
})

export default router
