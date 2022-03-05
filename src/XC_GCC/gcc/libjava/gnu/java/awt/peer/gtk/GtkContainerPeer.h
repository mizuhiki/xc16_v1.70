
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __gnu_java_awt_peer_gtk_GtkContainerPeer__
#define __gnu_java_awt_peer_gtk_GtkContainerPeer__

#pragma interface

#include <gnu/java/awt/peer/gtk/GtkComponentPeer.h>
extern "Java"
{
  namespace gnu
  {
    namespace java
    {
      namespace awt
      {
        namespace peer
        {
          namespace gtk
          {
              class GtkContainerPeer;
          }
        }
      }
    }
  }
  namespace java
  {
    namespace awt
    {
        class Color;
        class Container;
        class Font;
        class Insets;
    }
  }
}

class gnu::java::awt::peer::gtk::GtkContainerPeer : public ::gnu::java::awt::peer::gtk::GtkComponentPeer
{

public:
  GtkContainerPeer(::java::awt::Container *);
  virtual void beginValidate();
  virtual void endValidate();
  virtual ::java::awt::Insets * getInsets();
  virtual ::java::awt::Insets * insets();
  virtual void setBounds(jint, jint, jint, jint);
  virtual void setFont(::java::awt::Font *);
  virtual void beginLayout();
  virtual void endLayout();
  virtual jboolean isPaintPending();
  virtual void setBackground(::java::awt::Color *);
  virtual jboolean isRestackSupported();
  virtual void cancelPendingPaint(jint, jint, jint, jint);
  virtual void restack();
public: // actually package-private
  ::java::awt::Container * __attribute__((aligned(__alignof__( ::gnu::java::awt::peer::gtk::GtkComponentPeer)))) c;
public:
  static ::java::lang::Class class$;
};

#endif // __gnu_java_awt_peer_gtk_GtkContainerPeer__