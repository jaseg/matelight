from socketserver import *
import zlib
import struct

class ThreadingTCPServer(ThreadingMixIn, TCPServer): pass
class ThreadingUDPServer(ThreadingMixIn, UDPServer): pass

class MateLightUDPHandler(BaseRequestHandler):
    def handle(self):
        data = self.request[0].strip()
        if len(data) != FRAME_SIZE+4:
            raise ValueError('Invalid frame size: Expected {}, got {}'.format(FRAME_SIZE+4, len(frame)))
        frame = data[:-4]
        crc1, = struct.unpack('!I', data[-4:])
        crc2 = zlib.crc32(frame),
        if crc1 != crc2:
            raise ValueError('Invalid frame CRC checksum: Expected {}, got {}'.format(crc2, crc1))
        socket.sendto(b'ACK', self.client_address)

