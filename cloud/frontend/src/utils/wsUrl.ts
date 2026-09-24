/**
 * Story 6.3 裁决 D / AD-20：从 REST base 派生单一 WS URL。
 * http→ws，https→wss；path = REST base + `/ws`；token 仅经 query。
 */
import { API_BASE_URL } from './constants'

/**
 * 由 VITE_API_BASE_URL（含 /api/v1）派生 `ws(s)://{origin}{REST}/ws?token=`。
 */
export function buildRealtimeWsUrl(accessToken: string, apiBase = API_BASE_URL): string {
  const base = apiBase.replace(/\/+$/, '')
  let wsBase: string
  if (base.startsWith('https://')) {
    wsBase = `wss://${base.slice('https://'.length)}`
  } else if (base.startsWith('http://')) {
    wsBase = `ws://${base.slice('http://'.length)}`
  } else {
    wsBase = base
  }
  const path = wsBase.endsWith('/ws') ? wsBase : `${wsBase}/ws`
  const token = encodeURIComponent(accessToken)
  return `${path}?token=${token}`
}
