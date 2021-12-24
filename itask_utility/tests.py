import pexpect
import os

HTTP_RESPONSE = '<!DOCTYPE html>\n<html><body><h1>Hello from JOS!</h1></body></html>'
UDP_HELLO = 'HELLO'


def test_arp():
    template: str = 'test arp - {verdict}'
    result: str = pexpect.run('arping -c 1 -I br0 192.168.123.2').decode('utf-8')
    if 'Received 1 response' in result:
        print(template.format(verdict='OK'))
    else:
        print(template.format(verdict='FAIL'))


def test_ping():
    template: str = 'test ping - {verdict}'
    result: str = pexpect.run('ping -c 3 192.168.123.2').decode('utf-8')
    if '3 packets transmitted, 3 received' in result:
        print(template.format(verdict='OK'))
    else:
        print(template.format(verdict='FAIL'))


def test_udp():
    result: str = pexpect.run('./udp_send_recv').decode('utf-8')
    template: str = 'test udp - {verdict}'
    if result == UDP_HELLO:
        print(template.format(verdict='OK'))
    else:
        print(template.format(verdict='FAIL'))


def test_http_response():
    try:
        os.remove('index.html')
        os.remove('wget-log')
    except FileNotFoundError:
        pass
    template: str = 'test http response - {verdict}'
    pexpect.run('wget 192.168.123.2')
    try:
        with open('index.html') as f:
            content: str = f.read()
            if content == HTTP_RESPONSE:
                print(template.format(verdict='OK'))
            else:
                print(template.format(verdict='FAIL'))
    except:
        print(template.format(verdict='FAIL'))
    try:
        os.remove('index.html')
        os.remove('wget-log')
    except FileNotFoundError:
        pass


def main():
    test_arp()
    test_ping()
    test_udp()
    test_http_response()


if __name__ == '__main__':
    main()