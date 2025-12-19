export type GraphPlace = { name: string; lat: number; lng: number }
export type GraphRoad = { from: string; to: string; cost: number }
export type GraphResponse = { places: GraphPlace[]; roads: GraphRoad[] }

export type RouteResponse = { path: string[]; totalCost: number }

const API_BASE = import.meta.env.VITE_API_BASE ?? 'http://localhost:8080'

async function http<T>(path: string, init?: RequestInit): Promise<T> {
  const res = await fetch(`${API_BASE}${path}`, {
    ...init,
    headers: { 'Content-Type': 'application/json', ...(init?.headers ?? {}) },
  })
  if (!res.ok) {
    const text = await res.text().catch(() => '')
    throw new Error(`HTTP ${res.status}: ${text}`)
  }
  return (await res.json()) as T
}

export const api = {
  health: () => http<{ ok: boolean }>('/api/health'),
  graph: () => http<GraphResponse>('/api/graph'),
  route: (from: string, to: string) =>
    http<RouteResponse>(`/api/route?from=${encodeURIComponent(from)}&to=${encodeURIComponent(to)}`),

  register: (body: { userId: number; name: string; role: 'driver' | 'passenger' }) =>
    http<{ ok: boolean }>('/api/users/register', { method: 'POST', body: JSON.stringify(body) }),

  topDrivers: (k: number) => http<{ drivers: { userId: number; name: string; rating: number; completedRides: number }[] }>(`/api/users/top?k=${k}`),

  createOffer: (body: {
    offerId: number
    driverId: number
    start: string
    end: string
    departTime: number
    capacity: number
  }) => http<{ ok: boolean }>('/api/offers', { method: 'POST', body: JSON.stringify(body) }),

  createRequest: (body: {
    requestId: number
    passengerId: number
    from: string
    to: string
    earliest: number
    latest: number
  }) => http<{ ok: boolean }>('/api/requests', { method: 'POST', body: JSON.stringify(body) }),

  matchNext: () => http<{ matched: boolean; driverName?: string; driverId?: number; offerId?: number; start?: string; end?: string; departTime?: number }>('/api/match/next', { method: 'POST', body: '{}' }),

  getAllOffers: () => http<{ offers: { offerId: number; driverId: number; start: string; end: string; departTime: number; capacity: number; seatsLeft: number }[] }>('/api/offers'),

  getReachable: (offerId: number, costBound: number) =>
    http<{ reachable: { place: string; cost: number }[] }>(`/api/reachable?offerId=${offerId}&costBound=${costBound}`),

  save: () => http<{ ok: boolean }>('/api/storage/save', { method: 'POST', body: '{}' }),
  load: () => http<{ ok: boolean }>('/api/storage/load', { method: 'POST', body: '{}' }),
}


