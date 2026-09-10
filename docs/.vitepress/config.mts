import { defineConfig } from 'vitepress'

export default defineConfig({
  lang: 'ja-JP',
  title: 'Deneuve 2',
  description: 'Deneuve 2 — ESP32-S3 と Arduino による蚊取りシステムの開発記録',
  base: process.env.PAGES_BASE_PATH || '/',
  themeConfig: {
    socialLinks: [{ icon: 'github', link: 'https://github.com/ShigeichiroYamasaki/mosquito-trap-esp32s3' }],
    nav: [{ text: '開発を始める', link: '/getting-started' }, { text: '開発計画', link: '/roadmap' }],
    sidebar: [
      { text: 'プロジェクト', items: [
        { text: '概要', link: '/' },
        { text: '誘引・検出・吸引の機構', link: '/mechanism' },
        { text: 'macOS / Arduino IDE', link: '/getting-started' },
        { text: 'ハードウェア', link: '/hardware' },
        { text: 'ハードウェアテスト版', link: '/hardware-test' },
        { text: '自動検出・Webログ実行版', link: '/runtime' },
        { text: 'GitHub と公開手順', link: '/workflow' },
        { text: '開発計画・実験記録', link: '/roadmap' }
      ] }
    ],
    search: { provider: 'local' }
  }
})
