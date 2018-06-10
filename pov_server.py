#!/usr/bin/python

from time import sleep
from traceback import format_exc
from socket import socket, error, AF_INET, SOCK_STREAM, SOL_SOCKET, SO_REUSEADDR
from select import select
from threading import Thread
from re import match


class Server(Thread):
    def __init__(self, port):
        Thread.__init__(self)
        self.daemon = True
        self.port = port
        self.srvsock = socket(AF_INET, SOCK_STREAM)
        self.srvsock.setsockopt(SOL_SOCKET, SO_REUSEADDR, 1)
        self.srvsock.bind(("", port))
        self.srvsock.listen(5)
        self.descriptors = [self.srvsock]
        print('Server started on port {}'.format(port))

    def run(self):
        while 1:
            # Await an event on a readable socket descriptor
            (sread, swrite, sexc) = select(self.descriptors, [], [])
            # Iterate through the tagged read descriptors
            for sock in sread:
                # Received a connect to the server (listening) socket
                if sock == self.srvsock:
                    self.accept_new_connection()
                else:
                    try:
                        # Received something on a client socket
                        try:
                            string = str(sock.recv(2500).decode('utf-8'))
                        except UnicodeDecodeError:
                            string = "invalid character transmitted"
                            pass
                        host, port = sock.getpeername()
                        # Check to see if the peer socket closed
                        if string == '' or string == 'q':
                            print('Client left {0}:{1}'.format(host, port))
                            sock.close()
                            self.descriptors.remove(sock)
                        else:
                            print('{0} says: "{1}"'.format(host, string.rstrip()))
                            fwd_sock = None
                            if self.validate(string):
                                dest_ip = string.split(',')[1]
                                fwd_sock = self.get_sock(dest_ip)
                                if fwd_sock is not None:
                                    try:
                                        fwd_sock.send(string.encode())
                                        print('  Forward successful')
                                    except error:
                                        host, port = fwd_sock.getpeername()
                                        print('Socket Error - Client left {0}:{1}'.format(host, port))
                                        fwd_sock.close()
                                        self.descriptors.remove(fwd_sock)
                                else:
                                    print('  No target found, message dismissed')
                                    resp = 'Error: No target found, message dismissed\n'
                                    sock.send(resp.encode())
                            else:
                                print('  Invalid command, message dismissed')
                                resp = 'Error: Invalid command, message dismissed\n'
                                sock.send(resp.encode())
                    except (error, OSError):
                        host, port = sock.getpeername()
                        print('Socket Error - Client left {0}:{1}'.format(host, port))
                        sock.close()
                        self.descriptors.remove(sock)

    def get_sock(self, ip):
        ret_sock = None
        for sock in self.descriptors:
            try:
                host, port = sock.getpeername()
                if host == ip:
                    ret_sock = sock
                    break
            except (error, OSError):
                pass
        return ret_sock

    def validate(self, string):
        m = match(r"^\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3}\,\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3}\,(test|display|response|lock|unlock)\,.+\n", string)
        return m is not None

    def accept_new_connection(self):
        newsock, (remhost, remport) = self.srvsock.accept()
        self.descriptors.append(newsock)
        print('Client joined {0}:{1}'.format(remhost, remport))


def main():
    myServer = Server(10000)
    myServer.start()
    try:
        while 1:
            # Do nothing...
            sleep(1.0)
    except KeyboardInterrupt:
        print('Exiting...')


if __name__ == '__main__':
    main()
