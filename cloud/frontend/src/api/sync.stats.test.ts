/**
 * Story 6.6：getHistoryStats 真实路径薄测。
 */
import { beforeEach, describe, expect, it, vi } from 'vitest'

const get = vi.fn()

vi.mock('./request', () => ({
  get: (...args: unknown[]) => get(...args),
  post: vi.fn(),
}))

describe('api/sync getHistoryStats', () => {
  beforeEach(() => {
    get.mockReset()
    get.mockResolvedValue({ code: 0, message: 'ok', data: null })
  })

  it('GET /sync/stats 且 showError:false', async () => {
    const { getHistoryStats } = await import('./sync')
    await getHistoryStats()
    expect(get).toHaveBeenCalledWith(
      '/sync/stats',
      expect.objectContaining({ showError: false }),
    )
  })
})
