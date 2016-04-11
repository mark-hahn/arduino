
fs		   	= require 'fs'
url 		  = require 'url'
net       = require 'net'
http    	= require 'http'

{render, doctype, html, head, title, body, div, form,
 img, raw, text, script, input, button} = require 'teacup'

client = null
error = null
reading = ''

http.createServer (reqIn, resIn) ->
  console.log 'incoming req:', reqIn.url
  if reading is null
    resIn.writeHead 200, "Content-Type": "text/html"
    resIn.end 'waiting'
    console.log 'ignoring req while waiting', reqIn.url
    return
    
  req = reqIn
  res = resIn
  query = url.parse(req.url,true).query
  magHeading = ''
  
  if req.url[1] in [undefined, '?']
    
    magHeading = +query.head
    if magHeading  
      console.log 'magHeading', magHeading
      if not client 
        client = net.connect 23, '192.168.1.194', =>
          console.log 'connected to bot'
        
        client.on 'data', (data) =>
          if reading is null
            reading = data.toString()
          
        client.on 'error', (err) =>
          console.log 'bot err:', err.message
          error = err.message
          
        client.on 'end', =>
          error = 'disconnected'
          console.log 'disconnected from bot'
          
      else
        console.log 'getting sample'
        client?.write "r"
        setTimeout =>
          reading = null
          intvl = setInterval =>
            if reading isnt null
              ofs1 = +reading - magHeading
              ofs2 = ofs1 + 360
              ofs3 = ofs1 - 360
              minAbs = Math.abs(ofs1)
              ofs = ofs1
              if Math.abs(ofs2) < minAbs
                minAbs = Math.abs(ofs2)
                ofs = ofs2
              if Math.abs(ofs3) < minAbs
                ofs = ofs3
              line = magHeading + ' ' + 
                     ofs.toFixed(2) + ' ' + 
                     (ofs+100).toFixed(2) + ' ' +
                     reading
              console.log 'saved sample', line[0..-2]
              fs.appendFileSync 'samples', line
              res.writeHead 200, "Content-Type": "text/html"
              res.end render ->
                doctype()
                html ->
                  head ->
                    title 'forecast - bath'
                  body style:'', ->
                    form target: '/', ->
                      input autofocus:yes, name: 'head', value: ''
              reading = ''
              clearInterval intvl
          , 100
        , 1000
        
    if error
      res.writeHead 200, "Content-Type": "text/html"
      res.end error
      return
    
    if reading isnt null
      # console.log 'sending blank result'
      res.writeHead 200, "Content-Type": "text/html"
      res.end render ->
        doctype()
        html ->
          head ->
            title 'forecast - bath'
          body style:'', ->
            form target: '/', ->
              input autofocus:yes, name: 'head', value: ''
    return
              
.listen 8082

console.log 'listening on port 8082'
