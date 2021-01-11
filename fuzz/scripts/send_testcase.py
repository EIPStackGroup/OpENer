import sys
import socket
import struct

if len(sys.argv) != 3:
	print("python {} IP TESTCASE_PATH".format(sys.argv[0]))
	sys.exit(1)

HOST_IP = sys.argv[1]
HOST_PORT = 44818
TESTCASE_PATH = sys.argv[2]

ENIP_SESSION_CONTEXT = b"\x92\x83J\x0b=\x9e\x0cW"
ENIP_INIT_SESSION_PACKET = b"e\x00\x04\x00\x00\x00\x00\x00\x00\x00\x00\x00" + ENIP_SESSION_CONTEXT + b"\x00\x00\x00\x00\x01\x00\x00\x00"


print("[-] Connecting to {}:{}".format(HOST_IP, HOST_PORT))
s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.connect((HOST_IP, HOST_PORT))

print("[-] Init ENIP session")
s.sendall(ENIP_INIT_SESSION_PACKET)
enip_session = s.recv(1024)
session_handle = enip_session[4:8]
print("[-] Got ENIP Session Handle: {}".format(struct.unpack("<I", session_handle)[0]))
print("[-] Reading testcase from: '{}'".format(TESTCASE_PATH))
with open(TESTCASE_PATH, "rb") as f:
	testcase_data = f.read()

print("[-] Patching sender context and session handle")
testcase = testcase_data[:4]		# command, len
testcase += session_handle 		# session handle
testcase += testcase_data[8:12] 	# status
testcase += ENIP_SESSION_CONTEXT 	# session context
testcase += testcase_data[20:]		# options and payload
print("[-] Sending testcase of {} bytes".format(len(testcase)))
s.send(testcase)
s.close()