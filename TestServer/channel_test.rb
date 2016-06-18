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

def testCaseKick( client )
  client.start
  client.login
  client.join "UselessChan"
  loops = 5
  for i in 1..loops
    client.chatMessage "Bond. James Bond."
  end
  client.chatMessageTo "Test44918", "I'll kick you!"
  sleep(0.05)
  client.kick "Test44918"
  sleep(0.100)
  client.logout
  client.quit
end

def testCaseChat( client )
  client.start
  client.login
  client.join "UselessChan"
  loops = 10
  for i in 1..loops
    client.chatMessage "Hello you. Here John Doe."
  end
  sleep(0.500)
  client.logout
  client.quit
end

myClients = Array.new

startTime = Time.now

AdminClient = Client.new $hostname, 44917, "Test44933", "Test44933"
UserClient = Client.new $hostname, 44918, "Test44918", "Test44918"
myClients << AdminClient
myClients << UserClient

thr = Array.new

thr << Thread.start do
  testCaseChat UserClient
end

thr << Thread.start do
  testCaseKick AdminClient
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
