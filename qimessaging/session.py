##
## Author(s):
##  - Pierre Roullon <proullon@aldebaran-robotics.com>
##
## Copyright (C) 2010, 2011, 2012 Aldebaran Robotics
##

""" QiMessaging session system

Sessions are basis of QiMessaging application.
There is two different manners to use them :

- Service style :

 With objects bound on it, listening for call of other applications.

- Client style :

Only connected to service directory, get services from other applications.
"""

# NOTE: moved to collections module in python 3.
import UserDict

import _qimessagingswig as _qim
import qimessaging.binder as binder

from qimessaging.genericobject import GenericObject

class ConnectionError(Exception):
    """ Raised by Session constructor and Session.connect
    """
    def __init__(self, value):
        """ ConnectionError constructor
        Args:
        value : Error message.
        """
        self._value = value

    def __str__(self):
        """ Error message getter, Python style.
        """
        return str(self._value)


class RegisterError(Exception):
    """Raised by Session when it can't register a service."""

    def __init__(self, value):
        self._value = value

    def __str__(self):
        return str(self._value)


class Session(UserDict.DictMixin):
    """ Package all function needed to create and connect to QiMessage services.
    """

    def __init__(self, address = None):
        """ Session constructor, if address is set, try to connect.
        """
        self._session = _qim.qi_session_create()
        if address:
            self.connect(address)

    def __del__(self):
        """Session destructor, also destroy C++ session."""
        _qim.qi_session_destroy(self._session)

    def connect(self, address):
        """ Connect to service directory.

        .. Raises::
            ConnectionError exception.
        """
        if not _qim.qi_session_connect(self._session, address):
            raise ConnectionError('Cannot connect to ' + address)

    def listen(self, address):
        """ Listen on given address.

        Uppon connection, return service asked.
        """
        if _qim.qi_session_listen(self._session, address):
            return True
        return False

    def register_object(self, name, obj):
        """ Register given Python class instance
        """
        functionsList = binder.buildFunctionListFromObject(obj)
        return _qim.py_session_register_object(self._session, name, obj, functionsList)


    def register_service(self, name, obj):
        """ Register given service and expose it to the world.
        """
        return _qim.qi_session_register_service(self._session, name, obj._obj)

    def unregister_service(self, idx):
        """ Unregister service, it is not visible anymore.
        """
        _qim.qi_session_unregister_service(self._session, idx)

    def service(self, name, default=None):
        try:
            return self.__getitem__(name)
        except KeyError:
            return default

    def services(self):
        return _qim.qi_session_get_services(self._session)

    def close(self):
        """Disconnect from service directory."""
        _qim.qi_session_close(self._session)

    # Iteration over services functions.
    def __getitem__(self, name):
        """Ask to service directory for a service."""
        if not isinstance(name, str):
            raise TypeError('keys must be strings.')

        # Get C object.
        obj_c = _qim.qi_session_get_service(self._session, name)

        # One failure, return None.
        if not obj_c:
            raise KeyError("unknow service '%s'" % name)

        # Create Python object from C object.
        return GenericObject(obj_c)

    def keys(self):
        return self.services()
