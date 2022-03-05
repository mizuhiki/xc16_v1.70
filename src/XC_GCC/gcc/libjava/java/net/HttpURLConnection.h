
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __java_net_HttpURLConnection__
#define __java_net_HttpURLConnection__

#pragma interface

#include <java/net/URLConnection.h>
extern "Java"
{
  namespace java
  {
    namespace net
    {
        class HttpURLConnection;
        class URL;
    }
    namespace security
    {
        class Permission;
    }
  }
}

class java::net::HttpURLConnection : public ::java::net::URLConnection
{

public: // actually protected
  HttpURLConnection(::java::net::URL *);
public:
  virtual void disconnect() = 0;
  virtual jboolean usingProxy() = 0;
  static void setFollowRedirects(jboolean);
  static jboolean getFollowRedirects();
  virtual jboolean getInstanceFollowRedirects();
  virtual void setInstanceFollowRedirects(jboolean);
  virtual void setRequestMethod(::java::lang::String *);
  virtual ::java::lang::String * getRequestMethod();
  virtual jint getResponseCode();
  virtual ::java::lang::String * getResponseMessage();
private:
  void getResponseVals();
public:
  virtual ::java::security::Permission * getPermission();
  virtual ::java::io::InputStream * getErrorStream();
  virtual jlong getHeaderFieldDate(::java::lang::String *, jlong);
public: // actually package-private
  static const jint HTTP_CONTINUE = 100;
public:
  static const jint HTTP_OK = 200;
  static const jint HTTP_CREATED = 201;
  static const jint HTTP_ACCEPTED = 202;
  static const jint HTTP_NOT_AUTHORITATIVE = 203;
  static const jint HTTP_NO_CONTENT = 204;
  static const jint HTTP_RESET = 205;
  static const jint HTTP_PARTIAL = 206;
  static const jint HTTP_MULT_CHOICE = 300;
  static const jint HTTP_MOVED_PERM = 301;
  static const jint HTTP_MOVED_TEMP = 302;
  static const jint HTTP_SEE_OTHER = 303;
  static const jint HTTP_NOT_MODIFIED = 304;
  static const jint HTTP_USE_PROXY = 305;
  static const jint HTTP_BAD_REQUEST = 400;
  static const jint HTTP_UNAUTHORIZED = 401;
  static const jint HTTP_PAYMENT_REQUIRED = 402;
  static const jint HTTP_FORBIDDEN = 403;
  static const jint HTTP_NOT_FOUND = 404;
  static const jint HTTP_BAD_METHOD = 405;
  static const jint HTTP_NOT_ACCEPTABLE = 406;
  static const jint HTTP_PROXY_AUTH = 407;
  static const jint HTTP_CLIENT_TIMEOUT = 408;
  static const jint HTTP_CONFLICT = 409;
  static const jint HTTP_GONE = 410;
  static const jint HTTP_LENGTH_REQUIRED = 411;
  static const jint HTTP_PRECON_FAILED = 412;
  static const jint HTTP_ENTITY_TOO_LARGE = 413;
  static const jint HTTP_REQ_TOO_LONG = 414;
  static const jint HTTP_UNSUPPORTED_TYPE = 415;
  static const jint HTTP_SERVER_ERROR = 500;
  static const jint HTTP_INTERNAL_ERROR = 500;
  static const jint HTTP_NOT_IMPLEMENTED = 501;
  static const jint HTTP_BAD_GATEWAY = 502;
  static const jint HTTP_UNAVAILABLE = 503;
  static const jint HTTP_GATEWAY_TIMEOUT = 504;
  static const jint HTTP_VERSION = 505;
private:
  static jboolean followRedirects;
  static ::java::lang::String * valid_methods;
public: // actually protected
  ::java::lang::String * __attribute__((aligned(__alignof__( ::java::net::URLConnection)))) method;
  jint responseCode;
  ::java::lang::String * responseMessage;
  jboolean instanceFollowRedirects;
private:
  jboolean gotResponseVals;
public:
  static ::java::lang::Class class$;
};

#endif // __java_net_HttpURLConnection__
