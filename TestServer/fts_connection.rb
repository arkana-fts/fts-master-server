require 'socket'

class Connection
  def initialize host, port
    @sock = TCPSocket.open host, port
  end

  def recvdata( len )
    msg = ""
    left = len
    counter = 0 
    while (left != 0) and (counter < 10) do
      result = IO.select([@sock],nil,nil,0.05)
      if result != nil and result[0] != nil
        data = @sock.recv(left)
        if data.size == 0
          counter += 1
          next
        end
        msg = msg + data
        left -= data.size
      else
        counter += 1
      end
    end
    msg
  end

  def senddata(  msg )
    @sock.send msg, 0
  end

  def close
    @sock.close
  end
end
