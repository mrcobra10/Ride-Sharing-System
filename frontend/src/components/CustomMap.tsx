import { useEffect, useRef } from 'react'
import type { GraphPlace, GraphRoad } from '../lib/api'

type CustomMapProps = {
  places: GraphPlace[]
  roads: GraphRoad[]
  routePath?: string[]
  width: number
  height: number
}

export function CustomMap({ places, roads, routePath = [], width, height }: CustomMapProps) {
  const canvasRef = useRef<HTMLCanvasElement>(null)

  useEffect(() => {
    const canvas = canvasRef.current
    if (!canvas) return

    const ctx = canvas.getContext('2d')
    if (!ctx) return

    // Clear
    ctx.fillStyle = '#0a0a0a'
    ctx.fillRect(0, 0, width, height)

    // Simple force-directed layout (or grid if too many nodes)
    const nodePositions = new Map<string, { x: number; y: number }>()
    const padding = 40

    if (places.length <= 20) {
      // Force-directed layout for small graphs
      const iterations = 100
      const k = Math.sqrt((width * height) / places.length)
      const repulsion = k * k
      const attraction = k

      // Initialize positions randomly
      places.forEach((p) => {
        nodePositions.set(p.name, {
          x: padding + Math.random() * (width - 2 * padding),
          y: padding + Math.random() * (height - 2 * padding),
        })
      })

      // Iterate force-directed algorithm
      for (let iter = 0; iter < iterations; iter++) {
        const forces = new Map<string, { fx: number; fy: number }>()
        places.forEach((p) => {
          forces.set(p.name, { fx: 0, fy: 0 })
        })

        // Repulsion between all nodes
        for (let i = 0; i < places.length; i++) {
          for (let j = i + 1; j < places.length; j++) {
            const a = nodePositions.get(places[i].name)!
            const b = nodePositions.get(places[j].name)!
            const dx = b.x - a.x
            const dy = b.y - a.y
            const dist = Math.sqrt(dx * dx + dy * dy) || 1
            const force = repulsion / dist
            const fx = (dx / dist) * force
            const fy = (dy / dist) * force

            const fa = forces.get(places[i].name)!
            const fb = forces.get(places[j].name)!
            fa.fx -= fx
            fa.fy -= fy
            fb.fx += fx
            fb.fy += fy
          }
        }

        // Attraction along edges
        roads.forEach((r) => {
          const a = nodePositions.get(r.from)
          const b = nodePositions.get(r.to)
          if (!a || !b) return

          const dx = b.x - a.x
          const dy = b.y - a.y
          const dist = Math.sqrt(dx * dx + dy * dy) || 1
          const force = (dist / k) * attraction
          const fx = (dx / dist) * force
          const fy = (dy / dist) * force

          const fa = forces.get(r.from)!
          const fb = forces.get(r.to)!
          fa.fx += fx
          fa.fy += fy
          fb.fx -= fx
          fb.fy -= fy
        })

        // Apply forces with damping
        const damping = 0.9
        places.forEach((p) => {
          const pos = nodePositions.get(p.name)!
          const force = forces.get(p.name)!
          pos.x += force.fx * damping
          pos.y += force.fy * damping

          // Keep within bounds
          pos.x = Math.max(padding, Math.min(width - padding, pos.x))
          pos.y = Math.max(padding, Math.min(height - padding, pos.y))
        })
      }
    } else {
      // Grid layout for large graphs
      const cols = Math.ceil(Math.sqrt(places.length))
      const cellW = (width - 2 * padding) / cols
      const cellH = (height - 2 * padding) / Math.ceil(places.length / cols)

      places.forEach((p, idx) => {
        const row = Math.floor(idx / cols)
        const col = idx % cols
        nodePositions.set(p.name, {
          x: padding + col * cellW + cellW / 2,
          y: padding + row * cellH + cellH / 2,
        })
      })
    }

    // Draw edges (roads) - grey
    ctx.strokeStyle = 'rgba(255, 255, 255, 0.2)'
    ctx.lineWidth = 2
    roads.forEach((r) => {
      const a = nodePositions.get(r.from)
      const b = nodePositions.get(r.to)
      if (!a || !b) return

      ctx.beginPath()
      ctx.moveTo(a.x, a.y)
      ctx.lineTo(b.x, b.y)
      ctx.stroke()

      // Draw cost label
      const midX = (a.x + b.x) / 2
      const midY = (a.y + b.y) / 2
      ctx.fillStyle = 'rgba(255, 255, 255, 0.6)'
      ctx.font = '11px monospace'
      ctx.textAlign = 'center'
      ctx.textBaseline = 'middle'
      ctx.fillText(String(r.cost), midX, midY - 2)
    })

    // Draw route path (blue) if provided
    if (routePath.length >= 2) {
      ctx.strokeStyle = '#2F6BFF'
      ctx.lineWidth = 5
      ctx.lineCap = 'round'
      ctx.lineJoin = 'round'

      for (let i = 0; i < routePath.length - 1; i++) {
        const a = nodePositions.get(routePath[i])
        const b = nodePositions.get(routePath[i + 1])
        if (!a || !b) continue

        ctx.beginPath()
        ctx.moveTo(a.x, a.y)
        ctx.lineTo(b.x, b.y)
        ctx.stroke()
      }
    }

    // Draw nodes (places)
    places.forEach((p) => {
      const pos = nodePositions.get(p.name)
      if (!pos) return

      // Node circle
      ctx.fillStyle = 'rgba(0, 0, 0, 0.7)'
      ctx.beginPath()
      ctx.arc(pos.x, pos.y, 12, 0, Math.PI * 2)
      ctx.fill()

      ctx.strokeStyle = 'rgba(255, 255, 255, 0.8)'
      ctx.lineWidth = 2
      ctx.stroke()

      // Place name label
      ctx.fillStyle = 'rgba(255, 255, 255, 0.9)'
      ctx.font = '12px sans-serif'
      ctx.textAlign = 'center'
      ctx.textBaseline = 'top'
      ctx.fillText(p.name, pos.x, pos.y + 18)
    })
  }, [places, roads, routePath, width, height])

  return (
    <canvas
      ref={canvasRef}
      width={width}
      height={height}
      className="absolute inset-0 h-full w-full"
      style={{ imageRendering: 'crisp-edges' }}
    />
  )
}

