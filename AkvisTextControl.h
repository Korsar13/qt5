#ifndef _DEF_AKVIS_TEXTCONTROL
#define _DEF_AKVIS_TEXTCONTROL

#include <QtGlobal>

#ifdef __APPLE__
# undef qDebug
#endif

#if QT_VERSION < 0x050000
  #include "private/qtextcontrol_p.h"
  typedef QTextControl _QWidgetTextControl;
  #define _Q_WIDGETS_EXPORT
#else
  #include "private/qwidgettextcontrol_p.h"
  typedef QWidgetTextControl _QWidgetTextControl;
  #define _Q_WIDGETS_EXPORT Q_WIDGETS_EXPORT 
#endif

namespace Akvis {

class _Q_WIDGETS_EXPORT AkvisTextControl : public _QWidgetTextControl {
  Q_OBJECT
public:
  AkvisTextControl( QTextDocument *doc, QObject *parent, bool antialias );

  virtual void timerEvent( QTimerEvent* e );

  bool hasSelection() const;
  void clearSelection();
  
  bool IsCaretOn() const;

  void setHasFocus();
  void setBlinkingCaretEnabled( bool enable );

  void setCursorPosition( const QPointF &pos );

  virtual void insertFromMimeData( const QMimeData *source );

signals:
  void blinkCaret( bool on );

public:
  bool m_Antialias;
};

} // namespace Akvis

#endif //_DEF_AKVIS_TEXTCONTROL
