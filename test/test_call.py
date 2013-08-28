#!/usr/bin/env python2
##
## Author(s):
##  - Cedric GESTES <gestes@aldebaran-robotics.com>
##
## Copyright (C) 2013 Aldebaran Robotics

import qi
import time
import threading
import pytest

def setValue(p):
    time.sleep(0.2)
    p.setValue(42)

class FooService:
    def __init__(self):
        pass

    def simple(self):
        return 42

    def vargs(self, *args):
        return args

    def vargsdrop(self, titi, *args):
        return args

    @qi.nobind
    def hidden(self):
        pass

    @qi.bind(qi.Dynamic, qi.AnyArguments)
    def bind_vargs(self, *args):
        return args

    @qi.bind(qi.Dynamic, (), "renamed")
    def oldname(self):
        return 42

    @qi.bind(qi.Int, (qi.Int, qi.Int))
    def add(self, a, b):
        return a + b

    @qi.bind(None, None)
    def reta(self, a):
        return a

    def retfutint(self):
        p = qi.Promise()
        t = threading.Thread(target=setValue, args=(p,))
        t.start()
        return p.future()

    @qi.bind(qi.Int)
    def bind_retfutint(self):
        p = qi.Promise("(i)")
        t = threading.Thread(target=setValue, args=(p,))
        t.start()
        return p.future()

    @qi.bind(qi.Int(1, True))
    def retc(self, name, index):
        return name[index]

def docalls(sserver, sclient):
    m = FooService()
    sserver.registerService("FooService", m)
    s = sclient.service("FooService")

    assert s.simple() == 42

    #TODO: missing support in python
    assert s.vargs(42) == (42,)
    assert s.vargs("titi", "toto") == ("titi", "toto",)

    assert s.vargsdrop(4, 42) == (42,)


    try:
        s.hidden()
        assert False
    except:
        pass

    assert s.bind_vargs(42) == (42,)
    assert s.bind_vargs("titi", "toto") == ("titi", "toto",)

    assert s.renamed() == 42

    assert s.add(40, 2) == 42
    try:
        s.add("40", "2")
        assert False
    except:
        pass

    assert s.retfutint() == 42
    assert s.bind_retfutint() == 42



def test_calldirect():
    sd = qi.ServiceDirectory()
    try:
        sd.listen("tcp://127.0.0.1:0")
        local = sd.endpoints()[0]

        #MODE DIRECT
        print "## DIRECT MODE"
        ses = qi.Session()
        try:
            ses.connect(local)
            docalls(ses, ses)
        finally:
            ses.close()
    finally:
        sd.close()

def test_callsd():
    sd = qi.ServiceDirectory()
    try:
        sd.listen("tcp://127.0.0.1:0")
        local = sd.endpoints()[0]

        #MODE NETWORK
        print "## NETWORK MODE"
        ses = qi.Session()
        ses2 = qi.Session()
        try:
            ses.connect(local)
            ses2.connect(local)
            docalls(ses, ses2)
        finally:
            ses.close()
            ses2.close()
    finally:
        sd.close()



class Invalid1:
    def titi():
        pass

def test_missingself():
    sd = qi.ServiceDirectory()
    try:
        sd.listen("tcp://127.0.0.1:9559")
        local = sd.endpoints()[0]

        print "## TestInvalid (missing self)"
        ses = qi.Session()
        ses.connect(local)
        i = Invalid1()
        with pytest.raises(Exception):
            ses.registerService("Invalid1", i)
    finally:
        ses.close()
        sd.close()

class Invalid2:
    @qi.bind(42)
    def titi(self, a):
        pass

def test_badbind():
    sd = qi.ServiceDirectory()
    try:
        sd.listen("tcp://127.0.0.1:9559")
        local = sd.endpoints()[0]

        print "## TestInvalid (bind: bad return value)"
        ses = qi.Session()
        ses.connect(local)
        i = Invalid2()
        with pytest.raises(Exception):
            ses.registerService("Invalid2", i)
    finally:
        ses.close()
        sd.close()

class Invalid3:
    @qi.bind(qi.Int, [42])
    def titi(self, a):
        pass

def test_badbind2():
    sd = qi.ServiceDirectory()
    try:
        sd.listen("tcp://127.0.0.1:9559")
        local = sd.endpoints()[0]

        print "## TestInvalid (bind: bad params value)"
        ses = qi.Session()
        ses.connect(local)
        i = Invalid3()
        with pytest.raises(Exception):
            ses.registerService("Invalid3", i)
    finally:
        ses.close()
        sd.close()

def main():
    test_callsd()
    test_calldirect()
    test_missingself()
    test_badbind()
    test_badbind2()

if __name__ == "__main__":
    main()