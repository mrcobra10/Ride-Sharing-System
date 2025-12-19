import { AnimatePresence, motion } from 'framer-motion'

export function Toast(props: { message: string | null }) {
  return (
    <AnimatePresence>
      {props.message ? (
        <motion.div
          initial={{ y: -12, opacity: 0 }}
          animate={{ y: 0, opacity: 1 }}
          exit={{ y: -12, opacity: 0 }}
          transition={{ type: 'spring', stiffness: 400, damping: 30 }}
          className="pointer-events-none fixed left-1/2 top-4 z-[1000] -translate-x-1/2 rounded-full border border-white/10 bg-black/70 px-4 py-2 text-sm text-white shadow-xl backdrop-blur-xl"
        >
          {props.message}
        </motion.div>
      ) : null}
    </AnimatePresence>
  )
}


