/**
 * Story 6.5：postRoundAction 真实路径薄测（不被 store mock 吞掉）。
 */
import { beforeEach, describe, expect, it, vi } from 'vitest'

const post = vi.fn()

vi.mock('./request', () => ({
  get: vi.fn(),
  post: (...args: unknown[]) => post(...args),
}))

describe('api/sync postRoundAction', () => {
  beforeEach(() => {
    post.mockReset()
    post.mockResolvedValue({ code: 0, message: 'ok', data: null })
  })

  it('POST /sync/round-action 且 body 仅 action + action_id', async () => {
    const { postRoundAction } = await import('./sync')
    await postRoundAction({ action: 'restart', action_id: 'ewf_ra_restart_1' })
    expect(post).toHaveBeenCalledWith(
      '/sync/round-action',
      { action: 'restart', action_id: 'ewf_ra_restart_1' },
      expect.objectContaining({ showError: false }),
    )
  })
})
