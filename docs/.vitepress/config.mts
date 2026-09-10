import { defineConfig } from 'vitepress'

export default defineConfig({
  lang: 'ja-JP',
  title: '蚊取りシステム',
  description: 'ESP32-S3 と Arduino による蚊取りシステムの開発記録',
  base: process.env.PAGES_BASE_PATH || '/',
  themeConfig: {
    nav: [{ text: '開発を始める', link: '/getting-started' }, { text: '開発計画', link: '/roadmap' }],
    sidebar: [
      { text: 'プロジェクト', items: [
        { text: '概要', link: '/' },
        { text: 'macOS / Arduino IDE', link: '/getting-started' },
        { text: 'ハードウェア', link: '/hardware' },
        { text: 'GitHub と公開手順', link: '/workflow' },
        { text: '開発計画・実験記録', link: '/roadmap' }
      ] }
    ],
    search: { provider: 'local' }
  }
})
