# coding: iso-8859-1
require 'socket'
require_relative 'fts_packets'
require_relative 'fts_connection'

#$debug = 2
#$print_raw = 1

def dbgprint? bit
  ($debug & bit) > 0 if $debug
end

#$hostname = '192.168.1.12'
$hostname = 'localhost'


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
      begin
        until @isDone do
          hdr = @con.recvdata(9)
          ba = hdr.unpack('H*') if $print_raw
          puts "#r hdr #{hdr} #{ba}" if dbgprint? 0x02
          if hdr.size != 9
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
          #the read never returns correct if an answer status is given...
          #rp.read(answer)
          ba = answer.unpack('H*') if $print_raw
          puts "#r #{rp} #{ba}" if dbgprint? 0x02
          @statMsgRecv[rp.kind.snapshot] += 1
        end
      rescue EOFError
        # Ok we except that.
        puts "End of receiving reached" if dbgprint? 0x2
      end
    end
  end

  def sender msg
    sleep(0.01) # make it some how realistic
    sndlen = @con.senddata msg.to_binary_s
    puts "#s #{msg} #{sndlen} bytes" if dbgprint? 0x1
    @statMsgSend[msg.kind.snapshot] += 1
  end
  
  def chatMessage
    msg = Packet.new( :ident => 'FTSS', :kind => MsgType::CHAT_SEND_MSG, :pwd => @pwd )
    msg.text = "spam0r messag0r."
    msg.len = 2 + msg.pwd.size + msg.text.size + 2 # chat_type + flags+ 2 * end string delimiter
    sender msg
  end

  def chatgetMessage
    msg = Packet.new( :ident => 'FTSS', :kind => MsgType::CHAT_GET_MSG, :pwd => @pwd )
    msg.text = "spam0r messag0r."
    msg.len = 2 + msg.pwd.size + msg.text.size + 2 # chat_type + flags+ 2 * end string delimiter
    sender msg
  end

  def quit
    @isDone = true
    @treceiver.join
    @con.close
  end
end

port = 44917
passwd = 44917
userpwd = "Test_spammer"
myClient = Client.new $hostname, port, userpwd, userpwd
myClient.start
timeStart = Time.now
(1..1000).each do |x| 
  myClient.chatgetMessage
end
timeDiff = Time.now - timeStart
puts "Quiting #{timeDiff}"
myClient.quit

puts "send #{myClient.statMsgSend} recv #{myClient.statMsgRecv} failed #{myClient.failedMsg} fHdr #{myClient.failedHdr}"
puts "Avg per send #{(timeDiff/1000)}"

