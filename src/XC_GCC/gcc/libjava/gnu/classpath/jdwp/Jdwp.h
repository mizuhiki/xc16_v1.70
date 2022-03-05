
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __gnu_classpath_jdwp_Jdwp__
#define __gnu_classpath_jdwp_Jdwp__

#pragma interface

#include <java/lang/Thread.h>
#include <gcj/array.h>

extern "Java"
{
  namespace gnu
  {
    namespace classpath
    {
      namespace jdwp
      {
          class Jdwp;
        namespace event
        {
            class Event;
            class EventRequest;
        }
        namespace processor
        {
            class PacketProcessor;
        }
        namespace transport
        {
            class JdwpConnection;
        }
      }
    }
  }
}

class gnu::classpath::jdwp::Jdwp : public ::java::lang::Thread
{

public:
  Jdwp();
  static ::gnu::classpath::jdwp::Jdwp * getDefault();
  virtual ::java::lang::ThreadGroup * getJdwpThreadGroup();
  static jboolean suspendOnStartup();
  virtual void configure(::java::lang::String *);
private:
  void _doInitialization();
public:
  virtual void shutdown();
  static void notify(::gnu::classpath::jdwp::event::Event *);
  static void notify(JArray< ::gnu::classpath::jdwp::event::Event * > *);
  static void sendEvent(::gnu::classpath::jdwp::event::EventRequest *, ::gnu::classpath::jdwp::event::Event *);
  static void sendEvents(JArray< ::gnu::classpath::jdwp::event::EventRequest * > *, JArray< ::gnu::classpath::jdwp::event::Event * > *, jbyte);
private:
  void _enforceSuspendPolicy(jbyte);
public:
  virtual void subcomponentInitialized();
  virtual void run();
private:
  void _processConfigury(::java::lang::String *);
public: // actually package-private
  static ::gnu::classpath::jdwp::processor::PacketProcessor * access$0(::gnu::classpath::jdwp::Jdwp *);
private:
  static ::gnu::classpath::jdwp::Jdwp * _instance;
public:
  static jboolean isDebugging;
private:
  ::gnu::classpath::jdwp::processor::PacketProcessor * __attribute__((aligned(__alignof__( ::java::lang::Thread)))) _packetProcessor;
  ::java::lang::Thread * _ppThread;
  ::java::util::HashMap * _properties;
  static ::java::lang::String * _PROPERTY_SUSPEND;
  ::gnu::classpath::jdwp::transport::JdwpConnection * _connection;
  jboolean _shutdown;
  ::java::lang::ThreadGroup * _group;
  ::java::lang::Object * _initLock;
  jint _initCount;
public:
  static ::java::lang::Class class$;
};

#endif // __gnu_classpath_jdwp_Jdwp__
