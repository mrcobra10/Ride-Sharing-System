import { motion } from 'framer-motion'
import type { ReactNode } from 'react'

export function BottomSheet(props: { title: string; children: ReactNode }) {
  return (
    <motion.div
      initial={{ y: 40, opacity: 0 }}
      animate={{ y: 0, opacity: 1 }}
      transition={{ type: 'spring', stiffness: 260, damping: 24 }}
      className="pointer-events-auto w-full max-w-md rounded-3xl border border-white/10 bg-black/80 p-4 shadow-2xl backdrop-blur-xl"
    >
      <div className="mb-3 flex items-center justify-between gap-3">
        <div className="min-w-0">
          <div className="text-xs text-white/60">Ride Sharing</div>
          <div className="truncate text-lg font-semibold text-white">{props.title}</div>
        </div>
        <div className="h-1.5 w-12 rounded-full bg-white/15" />
      </div>
      {props.children}
    </motion.div>
  )
}


