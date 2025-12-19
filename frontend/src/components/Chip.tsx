import { motion } from 'framer-motion'
import type { ReactNode } from 'react'

export function Chip(props: { children: ReactNode; onClick?: () => void; className?: string }) {
  return (
    <motion.button
      whileTap={{ scale: 0.98 }}
      whileHover={{ y: -1 }}
      onClick={props.onClick}
      className={`rounded-full border bg-white/10 px-3 py-1.5 text-sm font-medium text-white backdrop-blur-md transition hover:bg-white/15 ${props.className || 'border-white/10'}`}
    >
      {props.children}
    </motion.button>
  )
}


