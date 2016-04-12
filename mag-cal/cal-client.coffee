

# 2D measurement of robot heading

fs		   	= require 'fs'
net       = require 'net'

line = ''

client = net.connect 23, '192.168.1.194', ->
  console.log 'connected to bot'
  client.write "\n" # needed to start robot net conn

client.on 'data', (data) ->
  line += data.toString()
  if line[-1..-1] is '\n'
    line = line[0..-2]
    [x,y,z] = line.split /\s+/
    x = -y; y = -z
    adjX = x +  52 # (see compass docs for values)
    adjY = y + 205
    heading = 360 * Math.atan(adjY / adjX) / (2 * Math.PI)
    heading += (if adjX > 0 then 270 else 90)
    console.log Math.round heading
    line = ''
  
client.on 'error', (err) ->
  console.log 'bot err:', err.message
  
client.on 'end', ->
  console.log 'disconnected from bot'
