require 'bindata'

class MsgType
  LOGIN=1
  LOGOUT=2
  CHAT_SEND_MSG=0x30          #48
  CHAT_GET_MSG=0x31           #49
  JOIN_CHAT=0x33              #51
  SOMEONE_JOINS_THE_CHAT=0x34 #52
  QUIT_CHAT=0x35              #53
  GET_CHAT_MOTTO=0x36
  SET_CHAT_MOTTO=0x37
  CHAT_MOTTO_CHANGED=0x38
  GET_CHAT_LIST=0x39          #57  Returns the list of users in the channel
  GET_CHAT_USER=0x3A          #58  Returns the state of the user
  GET_CHAT_PUBLICS=0x3B       #59  returns a list of public channels
  CHAT_KICK=0x3C              #60  kicks a user out of the current channel
  CHAT_KICKED=0x3D            #61  Info of kicked user
  CHAT_OPERATOR=0x3E          #62  Add to the operator list of the current channel
  CHAT_OP_ADDED=0x3F          #63  Info user added as operator
  CHAT_REMOVE_OPERATOR=0x40   #64  Remove user from lost of operators
  CAHT_OP_REMOVED=0x41        #65  Info user removed from operator list
  CHAT_LIST_MY_CHANS=0x42     #66  List all channels where I'm in.
  DESTROY_CHAN=0x43           #67
end

class ChatType
  NORMAL=1
  WHISPER=2
  SYSTEM=4
end

class PacketHeader < BinData::Record
  endian :little
  string  :ident, :length => 4
  uint8   :kind
  uint32  :len
end

class Packet < PacketHeader
  endian :little
  stringz :user, :onlyif => :isUserRequired?
  stringz :pwd
  stringz :room, :onlyif => lambda{ kind == MsgType::JOIN_CHAT or kind == MsgType::DESTROY_CHAN }
  uint8   :chat_type, :value => 1, :onlyif => lambda{ kind == MsgType::CHAT_SEND_MSG }
  uint8   :flags, :value => 0, :onlyif => lambda{ kind == MsgType::CHAT_SEND_MSG }
  stringz :toUser, :onlyif => lambda{ chat_type == ChatType::WHISPER and kind == MsgType::CHAT_SEND_MSG }
  stringz :text, :onlyif => lambda{ kind == MsgType::CHAT_SEND_MSG }
  def isUserRequired?
    kind == MsgType::LOGIN or kind == MsgType::GET_CHAT_USER
  end
end

class PacketResp < PacketHeader
  endian  :little
  uint8   :result, :onlyif => :hasResult?
  uint8   :chat_type, :value => 1, :onlyif => lambda{ kind == MsgType::CHAT_GET_MSG }
  uint8   :flags, :value => 0, :onlyif => lambda{ kind == MsgType::CHAT_GET_MSG }
  uint32  :elemets, :onlyif => :isList?
  array   :list_names, :type => :stringz, :onlyif => :isList?, :initial_length => lambda { elemets }
  array   :data, :type => :uint8, :onlyif => :hasData?, :initial_length => lambda { len - 1 }
  stringz :name, :onlyif => :hasName? 
  stringz :text, :onlyif => :hasText?

  def hasData?
    not isList? and hasResult? and len > 1 and result == 0
  end
  def isList?
    kind == MsgType::GET_CHAT_LIST or kind == MsgType::GET_CHAT_PUBLICS or kind == MsgType::CHAT_LIST_MY_CHANS
  end
  def hasResult?
    kind != MsgType::QUIT_CHAT and kind != MsgType::SOMEONE_JOINS_THE_CHAT and kind != MsgType::CHAT_GET_MSG
  end
  def hasName?
    kind == MsgType::QUIT_CHAT or kind == MsgType::SOMEONE_JOINS_THE_CHAT or kind == MsgType::CHAT_GET_MSG or kind == MsgType::CHAT_KICKED
  end
  def hasText?
    kind == MsgType::CHAT_GET_MSG or kind == MsgType::CHAT_KICKED
  end
end
