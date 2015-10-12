# coding: iso-8859-1
require 'socket'
require_relative 'fts_packets'
require_relative 'fts_connection'

#$debug = 1
#$preint_raw = 1

def dbgprint? bit
  ($debug & bit) > 0 if $debug
end

#$hostname = '192.168.1.12'
$hostname = 'localhost'

$nClients = 20

$statMsgSend = Hash.new( 0 )
$statMsgRecv = Hash.new( 0 )

$kindNames = Hash.new
$kindNames[MsgType::LOGIN] = 'Login'
$kindNames[MsgType::LOGOUT] = 'Logout'
$kindNames[MsgType::CHAT_SEND_MSG] = 'ChatSend'
$kindNames[MsgType::CHAT_GET_MSG] = 'ChatGet'
$kindNames[MsgType::JOIN_CHAT] = 'JoinChat'
$kindNames[MsgType::SOMEONE_JOINS_THE_CHAT] = 'Joined'
$kindNames[MsgType::QUIT_CHAT] = 'QuitChat'
$kindNames[MsgType::GET_CHAT_USER] = 'GetChatUser'
$kindNames[MsgType::GET_CHAT_LIST] = 'GetChatList'
$kindNames[MsgType::CHAT_KICKED] = 'ChatKicked'
$kindNames[MsgType::GET_CHAT_USER] = 'GetChatUser'
$kindNames[MsgType::DESTROY_CHAN] = 'ChatDestroy'
$kindNames[MsgType::GET_CHAT_PUBLICS] = 'GetPublics'

class Client
  attr_reader :failedHdr
  attr_reader :failedMsg
  attr_reader :port
  attr_reader :user
  attr_reader :statMsgSend
  attr_reader :statMsgRecv
  
  def initialize host, port, user, pwd
    @host = host
    @port = port
    @user = user
    @pwd = pwd
    @con = Connection.new host, port
    @isDone = false
    @failedHdr = Array.new
    @failedMsg = Array.new
    @statMsgSend = Hash.new( 0 )
    @statMsgRecv = Hash.new( 0 )
  end

  def start
    @treceiver = Thread.start do
      failed = 0
      until @isDone and failed > 3 do
        hdr = @con.recvdata(9)
        ba = hdr.unpack('H*') if $preint_raw
        puts "#r hdr #{hdr} #{ba}" if dbgprint? 0x02
        if hdr.size != 9
          failed += 1
          @failedHdr << hdr.size
          next
        end
        rhdr = PacketHeader.new
        rhdr.read(hdr)
        puts "#r rhdr #{rhdr}" if dbgprint? 0x02
        ans = @con.recvdata(rhdr.len)
        puts "#r ans #{ans.size}  #{ans}" if dbgprint? 0x02
        if ans.size != rhdr.len
          @failedMsg << ans.size
          next
        end
        answer = hdr + ans
        rp = PacketResp.new
        puts "#r asnwer #{answer}" if dbgprint? 0x02
        rp.read(answer)
        ba = answer.unpack('H*') if $preint_raw
        puts "#r #{rp} #{ba}" if dbgprint? 0x01
        failed = 0
        $statMsgRecv[rp.kind.snapshot] += 1
        @statMsgRecv[rp.kind.snapshot] += 1
      end
    end
  end

  def sender msg
    sleep(0.1) # make it some how realistic
    sndlen = @con.senddata msg.to_binary_s
    puts "#s #{msg} #{sndlen} bytes" if $debug
    $statMsgSend[msg.kind.snapshot] += 1 # snapshot returns the value as FixNum ruby object
    @statMsgSend[msg.kind.snapshot] += 1
  end
  
  def login
    msg = Packet.new( :ident => 'FTSS', :kind => MsgType::LOGIN, :user => @user, :pwd => @pwd )
    msg.len = msg.user.size + msg.pwd.size + 2 # 2 end string delimiter
    sender msg
  end

  def join
    msg = Packet.new( :ident => 'FTSS', :kind => MsgType::JOIN_CHAT, :pwd => @pwd, :room => "UselessChan" )
    msg.len = msg.pwd.size + "UselessChan".size + 2 
    sender msg
  end

  def listChatUsers
    msg = Packet.new( :ident => 'FTSS', :kind => MsgType::GET_CHAT_LIST, :pwd => @pwd )
    msg.len = msg.pwd.size + 1 # 1 end string delimiter
    sender msg
  end

  def chatMessage
    msg = Packet.new( :ident => 'FTSS', :kind => MsgType::CHAT_SEND_MSG, :pwd => @pwd )
    msg.text = "Hello you. Here John Doe."
    msg.len = 2 + msg.pwd.size + msg.text.size + 2 # chat_type + flags+ 2 * end string delimiter
    sender msg
  end

  def getUserState
    msg = Packet.new( :ident => 'FTSS', :kind => MsgType::GET_CHAT_USER, :pwd => @pwd )
    msg.user = @user
    msg.len = msg.pwd.size + msg.user.size + 2 # chat_type + flags+ 2 * end string delimiter
    sender msg
  end

  def getPublicChannels
    msg = Packet.new( :ident => 'FTSS', :kind => MsgType::GET_CHAT_PUBLICS, :pwd => @pwd )
    msg.len = msg.pwd.size + 1
    sender msg
  end

  def listMyChans
    msg = Packet.new( :ident => 'FTSS', :kind => MsgType::CHAT_LIST_MY_CHANS, :pwd => @pwd )
    msg.len = msg.pwd.size + 1 # 1 end string delimiter
    sender msg
  end

  def destroyChan
    msg = Packet.new  :ident => 'FTSS', :kind => MsgType::DESTROY_CHAN, :pwd => @pwd, :room => "UselessChan"
    msg.len = msg.pwd.size + "UselessChan".size + 2
    sender msg
  end

  def logout
    msg = Packet.new( :ident => 'FTSS', :kind => MsgType::LOGOUT, :pwd => @pwd )
    msg.len = msg.pwd.size + 1 # 1 end string delimiter
    sender msg
  end

  def quit
    @isDone = true
    @treceiver.join
    @con.close
  end
end

def testCase1( client )
  client.start
  client.login
  client.join
  client.listChatUsers
  client.getUserState
  client.getPublicChannels
  client.listMyChans
  loops = 19
  for i in 0..loops
    client.destroyChan if i == 10
    client.chatMessage
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
