/**
 * 信封 message 清洗：滤掉默认 "success"，避免白 toast。
 */

export function normalizeToastErrorMessage(message: unknown): string {
  if (typeof message !== 'string') {
    return ''
  }
  const trimmed = message.trim()
  if (!trimmed) {
    return ''
  }
  if (trimmed.toLowerCase() === 'success') {
    return ''
  }
  return trimmed
}
