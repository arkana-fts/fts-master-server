# coding: iso-8859-1
require 'socket'
require_relative 'fts_packets'
require_relative 'fts_connection'

$debug = 1
#$print_raw = 1

def dbgprint? bit
  ($debug & bit) > 0 if $debug
end

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
$kindNames[MsgType::CHAT_KICK] = 'ChatKick'
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
        ba = hdr.unpack('H*') if $print_raw
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
        ba = answer.unpack('H*') if $print_raw
        puts "#r #{rp} #{ba}" if dbgprint? 0x01
        failed = 0
        if rp.kind == 1 and rp.result != 0
          puts "#{@con.get_port}:Failure on #{$kindNames[rp.kind]} with result=#{rp.result}"
          #puts "Failure on #{$kindNames[rp.kind]} with result=#{rp.result}"
        end
        $statMsgRecv[rp.kind.snapshot] += 1
        @statMsgRecv[rp.kind.snapshot] += 1
      end
    end
  end

  def sender msg
    sleep(0.05) # make it some how realistic
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

  def join channel_name
    msg = Packet.new( :ident => 'FTSS', :kind => MsgType::JOIN_CHAT, :pwd => @pwd, :room => channel_name )
    msg.len = msg.pwd.size + "UselessChan".size + 2 
    sender msg
  end

  def listChatUsers
    msg = Packet.new( :ident => 'FTSS', :kind => MsgType::GET_CHAT_LIST, :pwd => @pwd )
    msg.len = msg.pwd.size + 1 # 1 end string delimiter
    sender msg
  end

  def chatMessage text
    msg = Packet.new( :ident => 'FTSS', :kind => MsgType::CHAT_SEND_MSG, :chat_type => ChatType::NORMAL , :pwd => @pwd )
    msg.text = text
    msg.len = 2 + msg.pwd.size + msg.text.size + 2 # chat_type + flags+ 2 * end string delimiter
    sender msg
  end

  def chatMessageTo user, text
    msg = Packet.new( :ident => 'FTSS', :kind => MsgType::CHAT_SEND_MSG, :chat_type => ChatType::WHISPER, :pwd => @pwd )
    msg.text = text
    msg.toUser = user
    msg.len = 2 + msg.pwd.size + msg.text.size + msg.toUser.size + 3 # chat_type + flags+ 3 * end string delimiter
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

  def kick user
   msg = Packet.new( :ident => 'FTSS', :kind => MsgType::CHAT_KICK, :pwd => @pwd, :toUser => user )
   msg.len = msg.pwd.size + 1 + msg.toUser.size + 1 # 1 end string delimiter
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

