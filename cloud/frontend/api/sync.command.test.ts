/**
 * Story 6.8：postSettingsCommand 真实路径薄测（不被 store mock 吞掉）。
 */
import { beforeEach, describe, expect, it, vi } from 'vitest'

const post = vi.fn()

vi.mock('./request', () => ({
  get: vi.fn(),
  post: (...args: unknown[]) => post(...args),
}))

describe('api/sync postSettingsCommand', () => {
  beforeEach(() => {
    post.mockReset()
    post.mockResolvedValue({ code: 0, message: 'ok', data: null })
  })

  it('POST /sync/command 且 body 含 volume/brightness/timeout/action_id', async () => {
    const { postSettingsCommand } = await import('./sync')
    await postSettingsCommand({
      volume: 50,
      brightness: 'mid',
      timeout: 15,
      action_id: 'ewf-cmd-1',
      base_revision: 2,
    })
    expect(post).toHaveBeenCalledWith(
      '/sync/command',
      {
        volume: 50,
        brightness: 'mid',
        timeout: 15,
        action_id: 'ewf-cmd-1',
        base_revision: 2,
      },
      expect.objectContaining({ showError: false }),
    )
  })
})
