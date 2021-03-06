#include <qipython/error.hpp>

//http://stackoverflow.com/questions/1418015/how-to-get-python-exception-text
std::string PyFormatError()
{
  try
  {
    PyObject *exc,*val,*tb;
    boost::python::object formatted_list, formatted;
    PyErr_Fetch(&exc,&val,&tb);
    if (!exc)
    {
      static const char err[] = "Bug: no error after call to PyFormatError";
      qiLogError("python") << err;
      return err;
    }
    boost::python::handle<> hexc(exc),hval(boost::python::allow_null(val)),htb(boost::python::allow_null(tb));
    boost::python::object traceback(boost::python::import("traceback"));
    boost::python::object format_exception_only(traceback.attr("format_exception_only"));
    formatted_list = format_exception_only(hexc,hval);
    formatted = boost::python::str("\n").join(formatted_list);
    return boost::python::extract<std::string>(formatted);
  }
  catch (...)
  {
    static const char err[] = "Bug: error while getting python error";
    qiLogError("python") << err;
    return err;
  }
}
