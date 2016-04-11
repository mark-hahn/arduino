
fs = require 'fs'

radians = (degrees) =>
  degrees * Math.PI / 180
degrees = (radians) =>
  radians * 180 / Math.PI;
  
normalize = (deg) =>
  while deg  <   0 then deg += 360
  while deg >= 360 then deg -= 360
  deg
  
delta = 3
rOfsB = -62
rB    = 60
tOfsB = 70
rms   = 5.7726

delta = 1
rOfsB = -61
rB    = 59
tOfsB = 70
rms   = 5.7707

delta = 0.5
rOfsB = -62
rB    = 59.5
tOfsB = 70
rms   = 5.7534

delta = 0.25
rOfsB = -61.75
rB    = 59.5
tOfsB = 70
rms   = 5.7506

delta = 0.125
rOfsB = -61.75
rB    = 59.375
tOfsB = 69.875
rms   = 5.7495

delta = 0.0625
rOfsB = -61.75
rB    = 59.375
tOfsB = 69.9375
rms   = 5.7494

delta = 0.03125
rOfsB = -61.71875
rB    = 59.375
tOfsB = 69.9375
rms   = 5.7493

rOfs = rOfsB
r    = rB
tOfs = tOfsB
func = (t) -> rOfs + r * (1 - Math.cos radians(t + tOfs))

map = []
txt = fs.readFileSync 'samples'
regex = new RegExp '([^\\n]*)\\n', 'g'
while (parts = regex.exec txt)
  [mag,diff,plus,reading] = parts[1].split /\s+/
  map.push [+reading, +diff]

minDiff = Math.min()
for rd in map
  minDiff = Math.min minDiff, rd[1]
for rd in map
  x  = rd[0]
  fx = rd[1]
  console.log  (fx-minDiff).toFixed(2), x.toFixed(2)
  console.log  (func(x)-minDiff).toFixed(2), x.toFixed(2)

# best = ''
# minSsq = Math.min()
# for tOfs in [tOfsB-delta*3..tOfsB+delta*3] by delta
#   for    r in  [rB-delta*3..rB+delta*3] by delta
#     for rOfs in [rOfsB-delta*3..rOfsB+delta*3] by delta
#       sumSq = 0
#       for rd in map
#         x  = rd[0]
#         fx = rd[1]
#         sumSq += Math.pow rd[1] - func(rd[0]), 2
#         # console.log (fx-minDiff).toFixed(2), x.toFixed(2)
#         # console.log (func(x)-minDiff+40).toFixed(2), x.toFixed(2)
#       if sumSq < minSsq
#         minSsq = sumSq
#         best = """
#           delta = #{delta}
#           rOfsB = #{rOfs}
#           rB    = #{r}
#           tOfsB = #{tOfs}
#           rms   = #{Math.sqrt(sumSq/map.length).toFixed(4)}
#         """
#       console.log rOfs, r, tOfs, +Math.sqrt(sumSq/map.length).toFixed(2)
# console.log best
