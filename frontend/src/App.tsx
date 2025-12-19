import { useEffect, useState } from 'react'
import { api, type GraphResponse } from './lib/api'
import { BottomSheet } from './components/BottomSheet'
import { Chip } from './components/Chip'
import { Toast } from './components/Toast'
import { CustomMap } from './components/CustomMap'

type Mode = 'register' | 'offer' | 'request' | 'top' | 'offers' | 'reachable'

type UserState = { isRegistered: boolean; userId: number; role: 'driver' | 'passenger' | null }

export default function App() {
  const [userState, setUserState] = useState<UserState>({ isRegistered: false, userId: 0, role: null })
  const [mode, setMode] = useState<Mode>(userState.isRegistered ? 'top' : 'register')
  const [toast, setToast] = useState<string | null>(null)

  const [graph, setGraph] = useState<GraphResponse | null>(null)
  const [routeNames, setRouteNames] = useState<string[]>([])


  useEffect(() => {
    let alive = true
    api
      .graph()
      .then((g) => alive && setGraph(g))
      .catch(() => alive && setToast('Backend not reachable. Start ./server on :8080'))
    return () => {
      alive = false
    }
  }, [])

  useEffect(() => {
    if (!toast) return
    const t = setTimeout(() => setToast(null), 2600)
    return () => clearTimeout(t)
  }, [toast])


  return (
    <div className="relative h-full w-full">
      <Toast message={toast} />

      {/* Custom Graph Map */}
      <div className="absolute inset-0 z-0">
        {graph ? (
          <CustomMap
            places={graph.places}
            roads={graph.roads}
            routePath={routeNames}
            width={window.innerWidth}
            height={window.innerHeight}
          />
        ) : (
          <div className="flex h-full w-full items-center justify-center bg-black text-white">
            Loading graph...
          </div>
        )}
      </div>

      {/* Top action bar - only show after registration */}
      {userState.isRegistered && (
        <div className="pointer-events-none absolute left-0 right-0 top-0 z-50 p-3">
          <div className="pointer-events-auto mx-auto flex max-w-2xl flex-wrap items-center justify-center gap-2">
            {userState.role === 'driver' && (
              <Chip onClick={() => setMode('offer')} className="bg-blue-500/20 border-blue-400/30">
                Create Offer
              </Chip>
            )}
            {userState.role === 'passenger' && (
              <>
                <Chip onClick={() => setMode('request')} className="bg-green-500/20 border-green-400/30">
                  Create Request
                </Chip>
                <Chip onClick={() => setMode('offers')} className="bg-purple-500/20 border-purple-400/30">
                  View Offers
                </Chip>
              </>
            )}
            <Chip
              onClick={async () => {
                try {
                  const r = await api.matchNext()
                  if (r.matched && r.driverName) {
                    const msg = `✓ Matched! Driver: ${r.driverName} (Offer #${r.offerId}) - ${r.start} → ${r.end} at ${r.departTime}`
                    setToast(msg)
                  } else {
                    setToast('No match found')
                  }
                } catch {
                  setToast('Match failed')
                }
              }}
              className="bg-yellow-500/20 border-yellow-400/30"
            >
              Match Next
            </Chip>
          </div>
        </div>
      )}

      {/* Bottom action bar - always visible after registration */}
      {userState.isRegistered && (
        <div className="pointer-events-none absolute bottom-20 left-0 right-0 z-50 p-3">
          <div className="pointer-events-auto mx-auto flex max-w-2xl flex-wrap items-center justify-center gap-2">
            <Chip onClick={() => setMode('top')} className="bg-orange-500/20 border-orange-400/30">
              Top Drivers
            </Chip>
            <Chip onClick={() => setMode('reachable')} className="bg-cyan-500/20 border-cyan-400/30">
              Reachable Areas
            </Chip>
            <Chip
              onClick={async () => {
                try {
                  const r = await api.save()
                  setToast(r.ok ? '✓ Saved to .dat files' : 'Save failed')
                } catch {
                  setToast('Save failed')
                }
              }}
              className="bg-gray-500/20 border-gray-400/30"
            >
              Save
            </Chip>
            <Chip
              onClick={async () => {
                try {
                  const r = await api.load()
                  if (!r.ok) {
                    setToast('Load failed')
                    return
                  }
                  const g = await api.graph()
                  setGraph(g)
                  setRouteNames([])
                  setToast('✓ Loaded from .dat files')
                } catch {
                  setToast('Load failed')
                }
              }}
              className="bg-gray-500/20 border-gray-400/30"
            >
              Load
            </Chip>
          </div>
        </div>
      )}

      {/* Bottom sheet - always visible, positioned above map */}
      <div className="pointer-events-none absolute bottom-0 left-0 right-0 z-[100] p-4">
        <div className="mx-auto flex max-w-md justify-center">
          <Sheet
            mode={mode}
            userState={userState}
            onRegistered={(userId, role) => {
              setUserState({ isRegistered: true, userId, role })
              setMode(role === 'driver' ? 'offer' : 'request')
            }}
            onClose={() => {
              if (userState.isRegistered) {
                setMode(userState.role === 'driver' ? 'offer' : 'request')
              }
            }}
            onToast={setToast}
            onRoute={(from, to) => {
              api
                .route(from, to)
                .then((r) => {
                  if (r.path.length === 0) {
                    setToast(`No path found from ${from} to ${to}. Make sure places exist in the graph.`)
                  } else {
                    setRouteNames(r.path)
                    setToast(`Route found: ${r.path.join(' → ')} (cost: ${r.totalCost})`)
                  }
                })
                .catch((err) => {
                  const msg = err.message || 'No route found'
                  if (msg.includes('404') || msg.includes('no_path')) {
                    setToast(`No path exists from "${from}" to "${to}". Check if both places are connected in the graph.`)
                  } else {
                    setToast(`Route error: ${msg}`)
                  }
                })
            }}
            graph={graph}
          />
        </div>
      </div>
    </div>
  )
}

function Sheet(props: {
  mode: Mode
  userState: UserState
  onRegistered: (userId: number, role: 'driver' | 'passenger') => void
  onClose: () => void
  onToast: (m: string) => void
  onRoute: (from: string, to: string) => void
  graph: GraphResponse | null
}) {
  const [userId, setUserId] = useState(1)
  const [name, setName] = useState('User')
  const [role, setRole] = useState<'driver' | 'passenger'>('passenger')

  const [offerId, setOfferId] = useState(1)
  const [driverId, setDriverId] = useState(props.userState.userId || 101)
  const [start, setStart] = useState('')
  const [end, setEnd] = useState('')
  const [departTime, setDepartTime] = useState(10)
  const [capacity, setCapacity] = useState(2)

  const [requestId, setRequestId] = useState(1)
  const [passengerId, setPassengerId] = useState(props.userState.userId || 201)

  // Update IDs when user registers
  useEffect(() => {
    if (props.userState.isRegistered && props.userState.userId) {
      setDriverId(props.userState.userId)
      setPassengerId(props.userState.userId)
    }
  }, [props.userState.isRegistered, props.userState.userId])

  const [from, setFrom] = useState('')
  const [to, setTo] = useState('')

  // Set default places when graph loads
  useEffect(() => {
    if (props.graph && props.graph.places.length > 0) {
      if (start === '') setStart(props.graph.places[0].name)
      if (end === '' && props.graph.places.length > 1) setEnd(props.graph.places[1].name)
      if (from === '') setFrom(props.graph.places[0].name)
      if (to === '' && props.graph.places.length > 1) setTo(props.graph.places[1].name)
    }
  }, [props.graph])
  const [earliest, setEarliest] = useState(9)
  const [latest, setLatest] = useState(12)

  const [topK, setTopK] = useState(5)
  const [drivers, setDrivers] = useState<
    { userId: number; name: string; rating: number; completedRides: number }[]
  >([])

  const [offers, setOffers] = useState<
    { offerId: number; driverId: number; start: string; end: string; departTime: number; capacity: number; seatsLeft: number }[]
  >([])

  const [reachableOfferId, setReachableOfferId] = useState(1)
  const [costBound, setCostBound] = useState(15)
  const [reachable, setReachable] = useState<{ place: string; cost: number }[]>([])

  // Force registration first if not registered
  if (!props.userState.isRegistered || props.mode === 'register') {
    return (
      <BottomSheet title="Register">
        <div className="space-y-3 text-white/80">
          <Field label="User ID">
            <input
              className="w-full rounded-xl border border-white/10 bg-black/30 px-3 py-2"
              value={userId}
              onChange={(e) => setUserId(Number(e.target.value))}
            />
          </Field>
          <Field label="Name (no spaces)">
            <input
              className="w-full rounded-xl border border-white/10 bg-black/30 px-3 py-2"
              value={name}
              onChange={(e) => setName(e.target.value)}
            />
          </Field>
          <Field label="Role">
            <div className="flex gap-2">
              <button
                className={`flex-1 rounded-xl px-3 py-2 text-sm font-semibold ${
                  role === 'passenger' ? 'bg-white text-black' : 'bg-white/10 text-white'
                }`}
                onClick={() => setRole('passenger')}
              >
                Passenger
              </button>
              <button
                className={`flex-1 rounded-xl px-3 py-2 text-sm font-semibold ${
                  role === 'driver' ? 'bg-white text-black' : 'bg-white/10 text-white'
                }`}
                onClick={() => setRole('driver')}
              >
                Driver
              </button>
            </div>
          </Field>
          <div className="flex gap-2">
            <button
              className="flex-1 rounded-2xl bg-white px-4 py-2 text-sm font-semibold text-black"
              onClick={async () => {
                try {
                  await api.register({ userId, name, role })
                  props.onToast(`Registered as ${role}`)
                  props.onRegistered(userId, role)
                } catch {
                  props.onToast('Register failed')
                }
              }}
            >
              Register
            </button>
            {props.userState.isRegistered && (
              <button
                className="rounded-2xl border border-white/10 bg-white/10 px-4 py-2 text-sm font-semibold text-white"
                onClick={props.onClose}
              >
                Close
              </button>
            )}
          </div>
        </div>
      </BottomSheet>
    )
  }

  if (props.mode === 'offer' && props.userState.isRegistered && props.userState.role === 'driver') {
    return (
      <BottomSheet title="Create ride offer">
        <div className="space-y-3 text-white/80">
          <Grid2>
            <Field label="Offer ID">
              <input className="w-full rounded-xl border border-white/10 bg-black/30 px-3 py-2" value={offerId} onChange={(e) => setOfferId(Number(e.target.value))} />
            </Field>
            <Field label="Driver ID">
              <input className="w-full rounded-xl border border-white/10 bg-black/30 px-3 py-2" value={driverId} onChange={(e) => setDriverId(Number(e.target.value))} />
            </Field>
          </Grid2>
          <Grid2>
            <Field label="Start Place">
              <select
                className="w-full rounded-xl border border-white/10 bg-black/30 px-3 py-2 text-white"
                value={start}
                onChange={(e) => setStart(e.target.value)}
              >
                <option value="">Select place...</option>
                {props.graph?.places.map((p) => (
                  <option key={p.name} value={p.name} className="bg-black">
                    {p.name}
                  </option>
                ))}
              </select>
            </Field>
            <Field label="End Place">
              <select
                className="w-full rounded-xl border border-white/10 bg-black/30 px-3 py-2 text-white"
                value={end}
                onChange={(e) => setEnd(e.target.value)}
              >
                <option value="">Select place...</option>
                {props.graph?.places.map((p) => (
                  <option key={p.name} value={p.name} className="bg-black">
                    {p.name}
                  </option>
                ))}
              </select>
            </Field>
          </Grid2>
          <Grid2>
            <Field label="Depart time">
              <input className="w-full rounded-xl border border-white/10 bg-black/30 px-3 py-2" value={departTime} onChange={(e) => setDepartTime(Number(e.target.value))} />
            </Field>
            <Field label="Capacity">
              <input className="w-full rounded-xl border border-white/10 bg-black/30 px-3 py-2" value={capacity} onChange={(e) => setCapacity(Number(e.target.value))} />
            </Field>
          </Grid2>
          <div className="flex gap-2">
            <button
              className="flex-1 rounded-2xl bg-white px-4 py-2 text-sm font-semibold text-black"
              onClick={async () => {
                if (!start || !end) {
                  props.onToast('Please select both start and end places')
                  return
                }
                try {
                  await api.createOffer({ offerId, driverId, start, end, departTime, capacity })
                  props.onRoute(start, end)
                  props.onToast('Offer created! Route highlighted on map.')
                  props.onClose()
                } catch (err: any) {
                  props.onToast(`Offer failed: ${err.message || 'Unknown error'}`)
                }
              }}
            >
              Create
            </button>
            <button
              className="rounded-2xl border border-white/10 bg-white/10 px-4 py-2 text-sm font-semibold text-white"
              onClick={props.onClose}
            >
              Close
            </button>
          </div>
        </div>
      </BottomSheet>
    )
  }

  if (props.mode === 'request' && props.userState.isRegistered && props.userState.role === 'passenger') {
    return (
      <BottomSheet title="Create ride request">
        <div className="space-y-3 text-white/80">
          <Grid2>
            <Field label="Request ID">
              <input className="w-full rounded-xl border border-white/10 bg-black/30 px-3 py-2" value={requestId} onChange={(e) => setRequestId(Number(e.target.value))} />
            </Field>
            <Field label="Passenger ID">
              <input className="w-full rounded-xl border border-white/10 bg-black/30 px-3 py-2" value={passengerId} onChange={(e) => setPassengerId(Number(e.target.value))} />
            </Field>
          </Grid2>
          <Grid2>
            <Field label="From Place">
              <select
                className="w-full rounded-xl border border-white/10 bg-black/30 px-3 py-2 text-white"
                value={from}
                onChange={(e) => setFrom(e.target.value)}
              >
                <option value="">Select place...</option>
                {props.graph?.places.map((p) => (
                  <option key={p.name} value={p.name} className="bg-black">
                    {p.name}
                  </option>
                ))}
              </select>
            </Field>
            <Field label="To Place">
              <select
                className="w-full rounded-xl border border-white/10 bg-black/30 px-3 py-2 text-white"
                value={to}
                onChange={(e) => setTo(e.target.value)}
              >
                <option value="">Select place...</option>
                {props.graph?.places.map((p) => (
                  <option key={p.name} value={p.name} className="bg-black">
                    {p.name}
                  </option>
                ))}
              </select>
            </Field>
          </Grid2>
          <Grid2>
            <Field label="Earliest">
              <input className="w-full rounded-xl border border-white/10 bg-black/30 px-3 py-2" value={earliest} onChange={(e) => setEarliest(Number(e.target.value))} />
            </Field>
            <Field label="Latest">
              <input className="w-full rounded-xl border border-white/10 bg-black/30 px-3 py-2" value={latest} onChange={(e) => setLatest(Number(e.target.value))} />
            </Field>
          </Grid2>
          <div className="flex gap-2">
            <button
              className="flex-1 rounded-2xl bg-white px-4 py-2 text-sm font-semibold text-black"
              onClick={async () => {
                if (!from || !to) {
                  props.onToast('Please select both from and to places')
                  return
                }
                try {
                  await api.createRequest({ requestId, passengerId, from, to, earliest, latest })
                  props.onRoute(from, to)
                  
                  // Automatically try to match the request
                  try {
                    const matchResult = await api.matchNext()
                    if (matchResult.matched && matchResult.driverName) {
                      const msg = `✓ Matched! Driver: ${matchResult.driverName} (Offer #${matchResult.offerId}) - ${matchResult.start} → ${matchResult.end} at ${matchResult.departTime}`
                      props.onToast(msg)
                    } else {
                      props.onToast('Request created! No match found yet. Route highlighted on map.')
                    }
                  } catch {
                    props.onToast('Request created! Route highlighted on map.')
                  }
                  
                  props.onClose()
                } catch (err: any) {
                  props.onToast(`Request failed: ${err.message || 'Passenger must exist'}`)
                }
              }}
            >
              Create
            </button>
            <button
              className="rounded-2xl border border-white/10 bg-white/10 px-4 py-2 text-sm font-semibold text-white"
              onClick={props.onClose}
            >
              Close
            </button>
          </div>
        </div>
      </BottomSheet>
    )
  }

  // Load offers when mode changes to 'offers'
  useEffect(() => {
    if (props.mode === 'offers' && props.userState.isRegistered) {
      api
        .getAllOffers()
        .then((r) => {
          setOffers(r.offers || [])
        })
        .catch((err) => {
          console.error('Failed to load offers:', err)
          props.onToast('Failed to load offers')
          setOffers([])
        })
    } else {
      setOffers([]) // Clear when mode changes away
    }
  }, [props.mode, props.userState.isRegistered])

  if (props.mode === 'offers' && props.userState.isRegistered) {
    return (
      <BottomSheet title="All Ride Offers">
        <div className="space-y-3 text-white/80">
          <button
            className="w-full rounded-xl bg-white/10 px-3 py-2 text-sm font-medium text-white hover:bg-white/15 transition"
            onClick={() => {
              api
                .getAllOffers()
                .then((r) => {
                  setOffers(r.offers)
                  props.onToast(`Loaded ${r.offers.length} offer(s)`)
                })
                .catch(() => props.onToast('Failed to load offers'))
            }}
          >
            🔄 Refresh Offers
          </button>
          <div className="max-h-96 space-y-2 overflow-y-auto">
            {offers.length === 0 ? (
              <div className="rounded-2xl border border-white/10 bg-white/5 p-4 text-center text-sm">
                <div className="text-white/60">No offers available</div>
                <div className="mt-1 text-xs text-white/40">Drivers can create offers using "Create Offer"</div>
              </div>
            ) : (
              offers.map((o) => (
                <div key={o.offerId} className="rounded-2xl border border-white/10 bg-white/5 p-3">
                  <div className="flex items-center justify-between">
                    <div className="font-semibold text-white">Offer #{o.offerId}</div>
                    <div className="text-xs text-white/60">Driver {o.driverId}</div>
                  </div>
                  <div className="mt-1 text-sm text-white/70">
                    {o.start} → {o.end}
                  </div>
                  <div className="mt-1 text-xs text-white/60">
                    Depart: {o.departTime} · Seats: {o.seatsLeft}/{o.capacity}
                  </div>
                </div>
              ))
            )}
          </div>
          <button
            className="w-full rounded-2xl border border-white/10 bg-white/10 px-4 py-2 text-sm font-semibold text-white"
            onClick={props.onClose}
          >
            Close
          </button>
        </div>
      </BottomSheet>
    )
  }

  if (props.mode === 'reachable' && props.userState.isRegistered) {
    return (
      <BottomSheet title="Reachable Areas">
        <div className="space-y-3 text-white/80">
          <Grid2>
            <Field label="Offer ID">
              <input
                className="w-full rounded-xl border border-white/10 bg-black/30 px-3 py-2"
                value={reachableOfferId}
                onChange={(e) => setReachableOfferId(Number(e.target.value))}
              />
            </Field>
            <Field label="Cost Bound">
              <input
                className="w-full rounded-xl border border-white/10 bg-black/30 px-3 py-2"
                value={costBound}
                onChange={(e) => setCostBound(Number(e.target.value))}
              />
            </Field>
          </Grid2>
          <button
            className="w-full rounded-2xl bg-white px-4 py-2 text-sm font-semibold text-black"
            onClick={async () => {
              try {
                const r = await api.getReachable(reachableOfferId, costBound)
                setReachable(r.reachable)
                props.onToast(`Found ${r.reachable.length} reachable places`)
              } catch {
                props.onToast('Failed to get reachable areas')
              }
            }}
          >
            Find Reachable
          </button>
          <div className="max-h-64 space-y-2 overflow-y-auto">
            {reachable.map((r) => (
              <div key={r.place} className="rounded-2xl border border-white/10 bg-white/5 p-3">
                <div className="flex items-center justify-between">
                  <div className="font-semibold text-white">{r.place}</div>
                  <div className="text-xs text-white/60">Cost: {r.cost}</div>
                </div>
              </div>
            ))}
            {reachable.length === 0 && (
              <div className="rounded-2xl border border-white/10 bg-white/5 p-3 text-sm">Press "Find Reachable" to search</div>
            )}
          </div>
          <button
            className="w-full rounded-2xl border border-white/10 bg-white/10 px-4 py-2 text-sm font-semibold text-white"
            onClick={props.onClose}
          >
            Close
          </button>
        </div>
      </BottomSheet>
    )
  }

  // top drivers
  return (
    <BottomSheet title="Top drivers">
      <div className="space-y-3 text-white/80">
        <div className="flex items-end gap-2">
          <Field label="K">
            <input
              className="w-full rounded-xl border border-white/10 bg-black/30 px-3 py-2"
              value={topK}
              onChange={(e) => setTopK(Number(e.target.value))}
            />
          </Field>
          <button
            className="h-10 rounded-2xl bg-white px-4 text-sm font-semibold text-black"
            onClick={async () => {
              try {
                const r = await api.topDrivers(topK)
                setDrivers(r.drivers)
              } catch {
                props.onToast('Failed to fetch top drivers')
              }
            }}
          >
            Refresh
          </button>
        </div>

        <div className="space-y-2">
          {drivers.map((d) => (
            <div key={d.userId} className="rounded-2xl border border-white/10 bg-white/5 p-3">
              <div className="flex items-center justify-between">
                <div className="font-semibold text-white">{d.name}</div>
                <div className="text-xs text-white/60">ID {d.userId}</div>
              </div>
              <div className="mt-1 text-sm text-white/70">
                Completed rides: <span className="text-white">{d.completedRides}</span> · Rating:{' '}
                <span className="text-white">{d.rating}</span>
              </div>
            </div>
          ))}
          {drivers.length === 0 ? (
            <div className="rounded-2xl border border-white/10 bg-white/5 p-3 text-sm">
              Press Refresh to load rankings.
            </div>
          ) : null}
        </div>

        <button
          className="w-full rounded-2xl border border-white/10 bg-white/10 px-4 py-2 text-sm font-semibold text-white"
          onClick={props.onClose}
        >
          Close
        </button>
      </div>
    </BottomSheet>
  )
}

function Field(props: { label: string; children: React.ReactNode }) {
  return (
    <label className="block">
      <div className="mb-1 text-xs text-white/60">{props.label}</div>
      {props.children}
    </label>
  )
}

function Grid2(props: { children: React.ReactNode }) {
  return <div className="grid grid-cols-2 gap-2">{props.children}</div>
}
