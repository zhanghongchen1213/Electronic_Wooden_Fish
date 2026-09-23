import { defineConfig } from 'vitest/config'

export default defineConfig({
  test: {
    environment: 'node',
    include: ['src/**/*.test.ts'],
  },
  define: {
    'import.meta.env.VITE_API_BASE_URL': JSON.stringify('http://localhost:9218/api/v1'),
    'import.meta.env.VITE_WX_APPID': JSON.stringify('test-wx-appid-not-a-snapshot-gold'),
  },
})
