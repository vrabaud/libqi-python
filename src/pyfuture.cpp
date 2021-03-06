/*
**  Copyright (C) 2013 Aldebaran Robotics
**  See COPYING for the license
*/
#include <qipython/pyfuture.hpp>
#include <qi/future.hpp>
#include <qipython/gil.hpp>
#include <boost/python.hpp>
#include <qipython/error.hpp>
#include <qipython/pythreadsafeobject.hpp>
#include "pystrand.hpp"

qiLogCategory("qipy.future");

namespace qi {
  namespace py {

    void pyFutureCb(const qi::Future<qi::AnyValue>& fut, const PyThreadSafeObject& callable) {
      GILScopedLock _lock;
      //reconstruct a pyfuture from the c++ future (the c++ one is always valid here)
      //both pypromise and pyfuture could have disappeared here.
      PY_CATCH_ERROR(callable.object()(PyFuture(fut)));
    }

    class PyPromise;
    static void pyFutureCbProm(const PyThreadSafeObject &callable, PyPromise *pp) {
      GILScopedLock _lock;
      PY_CATCH_ERROR(callable.object()(pp));
    }


    PyFuture::PyFuture()
    {}


    PyFuture::PyFuture(const qi::Future<qi::AnyValue>& fut)
      : qi::Future<qi::AnyValue>(fut)
    {}

    boost::python::object PyFuture::value(int msecs) const {
      qi::AnyValue gv;
      {
        GILScopedUnlock _unlock;
        //throw in case of error
        gv = qi::Future<qi::AnyValue>::value(msecs);
      }
      return gv.to<boost::python::object>();
    }

    std::string PyFuture::error(int msecs) const {
      GILScopedUnlock _unlock;
      return qi::Future<qi::AnyValue>::error(msecs);
    }

    FutureState PyFuture::wait(int msecs) const {
      GILScopedUnlock _unlock;
      return qi::Future<qi::AnyValue>::wait(msecs);
    }

    bool PyFuture::hasError(int msecs) const{
      GILScopedUnlock _unlock;
      return qi::Future<qi::AnyValue>::hasError(msecs);
    }

    bool PyFuture::hasValue(int msecs) const {
      GILScopedUnlock _unlock;
      return qi::Future<qi::AnyValue>::hasValue(msecs);
    }

    void PyFuture::addCallback(const boost::python::object &callable) {
      if (!PyCallable_Check(callable.ptr()))
        throw std::runtime_error("Not a callable");

      PyThreadSafeObject obj(callable);

      qi::Strand* strand = extractStrand(callable);
      if (strand)
      {
        GILScopedUnlock _unlock;
        connectWithStrand(strand, boost::bind<void>(&pyFutureCb, _1, obj));
      }
      else
      {
        GILScopedUnlock _unlock;
        connect(boost::bind<void>(&pyFutureCb, _1, obj));
      }
    }

    PyPromise::PyPromise()
    {
    }

    PyPromise::PyPromise(const qi::Promise<qi::AnyValue> &ref)
      : qi::Promise<qi::AnyValue>(ref)
    {
    }

    PyPromise::PyPromise(boost::python::object callable)
      : qi::Promise<qi::AnyValue> (boost::bind<void>(&pyFutureCbProm, PyThreadSafeObject(callable), this))
    {
    }

    void PyPromise::setValue(const boost::python::object &pyval) {
      //TODO: remove the useless copy here.
      qi::AnyValue gvr = qi::AnyValue::from(pyval);
      {
        GILScopedUnlock _unlock;
        qi::Promise<qi::AnyValue>::setValue(gvr);
      }
    }

    PyFuture PyPromise::future() {
      return qi::Promise<qi::AnyValue>::future();
    }

    void export_pyfuture() {
      boost::python::enum_<qi::FutureState>("FutureState")
          .value("None", qi::FutureState_None)
          .value("Running", qi::FutureState_Running)
          .value("Canceled", qi::FutureState_Canceled)
          .value("FinishedWithError", qi::FutureState_FinishedWithError)
          .value("FinishedWithValue", qi::FutureState_FinishedWithValue);


      boost::python::enum_<qi::FutureTimeout>("FutureTimeout")
          .value("None", qi::FutureTimeout_None)
          .value("Infinite", qi::FutureTimeout_Infinite);


      boost::python::class_<PyPromise>("Promise")
          .def(boost::python::init<boost::python::object>())

          .def("setCanceled", &PyPromise::setCanceled,
               "setCanceled() -> None\n"
               "Set the state of the promise to Canceled")

          .def("setError", &PyPromise::setError,
               "setError(error) -> None\n"
               "Set the error of the promise")

          .def("setValue", &PyPromise::setValue,
               "setValue(value) -> None\n"
               "Set the value of the promise")

          .def("future", &PyPromise::future,
               "future() -> qi.Future\n"
               "Get a qi.Future from the promise, you can get multiple from the same promise.")

          .def("isCancelRequested", &PyPromise::isCancelRequested,
               "isCancelRequested() -> bool\n"
               "Return true if the future associated with the promise asked for cancelation");

      boost::python::class_<PyFuture>("Future", boost::python::no_init)
          .def("value", &PyFuture::value, (boost::python::args("timeout") = qi::FutureTimeout_Infinite),
               "value(timeout) -> value\n"
               ":param timeout: a time in milliseconds. Optional.\n"
               ":return: the value of the future.\n"
               ":raise: a RuntimeError if the timeout is reached or the future has error.\n"
               "\n"
               "Block until the future is ready.")

          .def("error", &PyFuture::error, (boost::python::args("timeout") = qi::FutureTimeout_Infinite),
               "error(timeout) -> string\n"
               ":param timeout: a time in milliseconds. Optional.\n"
               ":return: the error of the future.\n"
               ":raise: a RuntimeError if the timeout is reached or the future has no error.\n"
               "\n"
               "Block until the future is ready.")

          .def("wait", &PyFuture::wait, (boost::python::args("timeout") = qi::FutureTimeout_Infinite),
               "wait(timeout) -> qi.FutureState\n"
               ":param timeout: a time in milliseconds. Optional.\n"
               ":return: a :py:data:`qi.FutureState`.\n"
               "\n"
               "Wait for the future to be ready." )

          .def("hasError", &PyFuture::hasError, (boost::python::args("timeout") = qi::FutureTimeout_Infinite),
               "hasError(timeout) -> bool\n"
               ":param timeout: a time in milliseconds. Optional.\n"
               ":return: true if the future has an error.\n"
               ":raise: a RuntimeError if the timeout is reached.\n"
               "\n"
               "Return true or false depending on the future having an error.")

          .def("hasValue", &PyFuture::hasValue, (boost::python::args("timeout") = qi::FutureTimeout_Infinite),
               "hasValue(timeout) -> bool\n"
               ":param timeout: a time in milliseconds. Optional.\n"
               ":return: true if the future has a value.\n"
               ":raise: a RuntimeError if the timeout is reached.\n"
               "\n"
               "Return true or false depending on the future having a value.")

          .def("cancel", &PyFuture::cancel,
               "cancel() -> None\n"
               "If the future is cancelable, ask for cancelation.")

          .def("isFinished", &PyFuture::isFinished,
               "isFinished() -> bool\n"
               ":return: true if the future is not running anymore (if hasError or hasValue or isCanceled).")

          .def("isRunning", &PyFuture::isRunning,
               "isRunning() -> bool\n"
               ":return: true if the future is still running.\n")

          .def("isCanceled", &PyFuture::isCanceled,
               "isCanceled() -> bool\n"
               ":return: true if the future is canceled.\n")

          .def("isCancelable", &PyFuture::isCancelable,
               "isCancelable() -> bool\n"
               ":return: true if the future is cancelable. (not all future are cancelable)\n")

          .def("addCallback", &PyFuture::addCallback,
               "addCallback(cb) -> None\n"
               ":param cb: a python callable, could be a method or a function.\n"
               "\n"
               "Add a callback that will be called when the future becomes ready.\n"
               "The callback will be called even if the future is already ready.\n"
               "The first argument of the callback is the future itself.");
    }
  }
}
