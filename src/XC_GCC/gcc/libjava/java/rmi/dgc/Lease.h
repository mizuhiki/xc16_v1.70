
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __java_rmi_dgc_Lease__
#define __java_rmi_dgc_Lease__

#pragma interface

#include <java/lang/Object.h>
extern "Java"
{
  namespace java
  {
    namespace rmi
    {
      namespace dgc
      {
          class Lease;
          class VMID;
      }
    }
  }
}

class java::rmi::dgc::Lease : public ::java::lang::Object
{

public:
  Lease(::java::rmi::dgc::VMID *, jlong);
  ::java::rmi::dgc::VMID * getVMID();
  jlong getValue();
  ::java::lang::String * toString();
public: // actually package-private
  static const jlong serialVersionUID = -5713411624328831948LL;
private:
  ::java::rmi::dgc::VMID * __attribute__((aligned(__alignof__( ::java::lang::Object)))) vmid;
  jlong value;
public:
  static ::java::lang::Class class$;
};

#endif // __java_rmi_dgc_Lease__
