# coding: iso-8859-1
require 'socket'
require_relative 'fts_packets'
require_relative 'fts_connection'
require_relative 'client'

#$debug = 1
#$print_raw = 1

def dbgprint? bit
  ($debug & bit) > 0 if $debug
end

#$hostname = '192.168.1.12'
$hostname = 'localhost'

$nClients = 5

def testCase1( client )
  client.start
  client.login
  client.join "UselessChan"
  client.listChatUsers
  client.getUserState
  client.getPublicChannels
  client.listMyChans
  loops = 10
  for i in 0..loops
#    client.destroyChan if i == 10
    client.chatMessage "Hello you. Here John Doe."
  end
  sleep(0.100)
  client.logout
  client.quit
end

myClients = Array.new

startTime = Time.now

thr = Array.new
(0..($nClients-1)).each do |x|
  port = 44917#  + x
  passwd = 44917 + x 
  userpwd = "Test#{passwd}"
  myClients[x] = Client.new $hostname, port, userpwd, userpwd
  thr << Thread.start do
    testCase1 myClients[x] #Client.new $hostname, port, userpwd, userpwd
  end

end

thr.each do |t|
  t.join
end
endTime = Time.now

myClients.each do | client |
  txt = "Result: "
  $kindNames.each do |k,v|
    txt += "#{v}(#{client.statMsgSend[k]}/#{client.statMsgRecv[k]}) " if client.statMsgSend[k] != 0 or client.statMsgRecv[k] != 0 
  end
  puts txt
end

txt = "Total : "
$kindNames.each do |k,v|
  txt += "#{v}(#{$statMsgSend[k]}/#{$statMsgRecv[k]}) " if $statMsgSend[k] != 0 or $statMsgRecv[k] != 0
end
puts txt

txt = "Missed : "
myClients.each do | client |
  failedHdr = client.failedHdr - [0]
  if (failedHdr.length > 0) or (client.failedMsg.length) > 0 then
    txt += "#{client.user}: #{failedHdr} / #{client.failedMsg} | "
  end
end
puts txt

sumSended = $statMsgSend.values.reduce :+
sumReceived = $statMsgRecv.values.reduce :+
puts "Total Send #{sumSended} Recv #{sumReceived}"
puts "Total Execution Time #{endTime - startTime} avg #{(endTime - startTime)/(sumSended+sumReceived)}"
