#include <QApplication>
#include <QStyle>
#include <QTimer>
#include "AkvisTextControl.h"
#include <limits>

#ifdef __APPLE__
# undef qDebug
#endif

QT_BEGIN_NAMESPACE
Q_GUI_EXPORT extern int qt_defaultDpiY(); // in qfont.cpp
QT_END_NAMESPACE

#include <private/qtextdocumentfragment_p.h>
class TextDocumentFragmentHack { // !!! ОЧЕНЬ ГРЯЗНЫЙ ХАК !!!
public:
  TextDocumentFragmentHack( QTextDocumentFragment* fragment ) :
    d( ( (TextDocumentFragmentHack*)fragment )->d )
  {}
  QTextDocumentFragmentPrivate *d;
};
static QTextDocumentFragmentPrivate* hack_d_func( QTextDocumentFragment* p ) {
  TextDocumentFragmentHack tmp( p );
  return tmp.d;
}

#if QT_VERSION < 0x050000
  #include "private/qtextcontrol_p_p.h"
  typedef QTextControlPrivate _QWidgetTextControlPrivate;
  class TextControlHack : public _QWidgetTextControl {
  public:
    Q_DECLARE_PRIVATE( QTextControl )
    _QWidgetTextControlPrivate* hack_d_func() { return d_func(); }
    const _QWidgetTextControlPrivate* const_hack_d_func() const { return d_func(); }
  };
#else
  #include "private/qwidgettextcontrol_p_p.h"
  typedef QWidgetTextControlPrivate _QWidgetTextControlPrivate;

  class TextControlHack : public _QWidgetTextControl {
  public:
    Q_DECLARE_PRIVATE( QWidgetTextControl )

    _QWidgetTextControlPrivate* hack_d_func() { return d_func(); }
    const _QWidgetTextControlPrivate* const_hack_d_func() const { return d_func(); }
  };
#endif

namespace Akvis {

static _QWidgetTextControlPrivate* hack_d_func( AkvisTextControl* p ) {
  return ( ( TextControlHack*) p )->hack_d_func();
}

static const _QWidgetTextControlPrivate* hack_d_func( const AkvisTextControl* p ) {
  return ( ( const TextControlHack*) p )->const_hack_d_func();
}


//-----------------------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------------
AkvisTextControl::AkvisTextControl( QTextDocument *doc, QObject *parent, bool antialias ) :
  _QWidgetTextControl( doc, parent ), m_Antialias( antialias )
{}

bool AkvisTextControl::IsCaretOn() const {
  return hack_d_func( this )->cursorOn;
}

bool AkvisTextControl::hasSelection() const {
  return hack_d_func( this )->cursor.hasSelection();
}

void AkvisTextControl::clearSelection() {
  _QWidgetTextControlPrivate *d = hack_d_func( this );
  Q_ASSERT( d->cursor.hasSelection() );
  QTextCursor oldSelection( d->cursor );
  d->cursor.clearSelection();
  d->selectionChanged( true );
  d->repaintOldAndNewSelection( oldSelection );
}

void AkvisTextControl::setHasFocus() {
  hack_d_func( this )->hasFocus = true;
}

void AkvisTextControl::setBlinkingCaretEnabled( bool enable ) {
#if QT_VERSION < 0x050000
  hack_d_func( this )->setBlinkingCursorEnabled( enable );
#else
  hack_d_func( this )->setCursorVisible( enable );
#endif
}

void AkvisTextControl::setCursorPosition( const QPointF &pos ) {
  hack_d_func( this )->setCursorPosition( pos );
}

void AkvisTextControl::timerEvent( QTimerEvent* e ) {
  _QWidgetTextControlPrivate *d = hack_d_func( this );
  if( e->timerId() == d->cursorBlinkTimer.timerId() ) {
    d->cursorOn = !d->cursorOn;
    if( d->cursor.hasSelection() && !QApplication::style()->styleHint( QStyle::SH_BlinkCursorWhenTextSelected ) ) {
      d->cursorOn = false;
    }
    emit blinkCaret( d->cursorOn );
    return;
  }
  _QWidgetTextControl::timerEvent( e );
}

static bool Validate( QTextCharFormat& charFmt, bool antialias ) {
  bool changed = false;
  if( charFmt.fontCapitalization() == QFont::SmallCaps ) {
    charFmt.setFontCapitalization( QFont::MixedCase );
    changed = true;
  }
  charFmt.setFontStyleStrategy( QFont::StyleStrategy( QFont::PreferQuality | ( antialias ? QFont::PreferAntialias : QFont::PreferDefault ) ) );
  if( charFmt.hasProperty( QTextFormat::FontPointSize ) ) {
    return antialias || changed;
  }
  double pointSize = 0; int pixelSize = 0;
  if( charFmt.hasProperty( QTextFormat::FontSizeAdjustment ) ) {
    charFmt.clearProperty( QTextFormat::FontSizeAdjustment );
  }
  if( charFmt.hasProperty( QTextFormat::FontPixelSize ) ) {
    pixelSize = charFmt.intProperty( QTextFormat::FontPixelSize );
    charFmt.clearProperty( QTextFormat::FontPixelSize );
  }
  else {
    pointSize = charFmt.font().pointSizeF();
    if( pointSize < 0 ) {
      pixelSize = charFmt.font().pixelSize();
    }
  }
  if( pointSize <= 0 ) {
    pointSize = ( pixelSize <= 0 ? 12.0 : pixelSize * 72.0 / qt_defaultDpiY() );
  }
  charFmt.setFontPointSize( pointSize );
  return true;
}

void AkvisTextControl::insertFromMimeData( const QMimeData *source ) {
  _QWidgetTextControlPrivate *d = hack_d_func( this );
  
  if(!( d->interactionFlags & Qt::TextEditable ) || !source ) return;

  bool hasData = false;
  QTextDocumentFragment fragment;

  if( source->hasFormat( QLatin1String( "application/x-qrichtext" ) ) && d->acceptRichText ) {
    // x-qrichtext is always UTF-8 (taken from Qt3 since we don't use it anymore).
    QString richtext = QString::fromUtf8( source->data( QLatin1String( "application/x-qrichtext" ) ) );
    richtext.prepend( QLatin1String( "<meta name=\"qrichtext\" content=\"1\" />" ) );

    fragment = QTextDocumentFragment::fromHtml( richtext, d->doc );
    hasData  = true;
  } 
  else if( source->hasHtml() && d->acceptRichText ) {
    fragment = QTextDocumentFragment::fromHtml( source->html(), d->doc );
    hasData  = true;
  }
  else {
    QString text = source->text();
    if( !text.isNull() ) {
      fragment = QTextDocumentFragment::fromPlainText( text );
      hasData = true;
    }
  }

  if( hasData ) {
    QTextDocument *tmpDoc = hack_d_func( &fragment )->doc;
    {
      QTextCharFormat charFmt( currentCharFormat() );
      if( charFmt.fontCapitalization() == QFont::SmallCaps ) {
        charFmt.setFontCapitalization( QFont::MixedCase );
      }
      QFont defFont( 
        charFmt.fontFamily(),
        charFmt.fontPointSize(),
        charFmt.fontWeight(),
        charFmt.fontItalic()
      );
      if( QFontInfo( defFont ).family().isEmpty() ) {
        defFont = QApplication::font();
      }
      QFont::StyleStrategy ss = QFont::StyleStrategy( QFont::PreferQuality | ( m_Antialias ? QFont::PreferAntialias : QFont::PreferDefault ) );
      if( defFont.styleStrategy() != ss ) {
        defFont.setStyleStrategy( ss );
      }
      tmpDoc->setDefaultFont( defFont );
    }

    {
      QTextCursor cursor( tmpDoc );
      cursor.beginEditBlock();

      cursor.select( QTextCursor::Document );
      int pos = cursor.selectionStart(), endPos = cursor.selectionEnd();
      Q_ASSERT( pos < endPos );
      
      QTextDocumentPrivate *src = tmpDoc->docHandle(); bool firstBlock = true;
      while( pos < endPos ) {
        QTextDocumentPrivate::FragmentIterator FI( src->find( pos ) );
        const QTextFragmentData * const frag = FI.value();

        const int inFragmentOffset = std::max( 0, pos - FI.position() );
        int charsToCopy = std::min( int( frag->size_array[0] - inFragmentOffset ), endPos - pos );

        QTextFormat ffmt( src->formatCollection()->format( frag->format ) );
        if( ffmt.isCharFormat() ) {
          QTextCharFormat charFmt( ffmt.toCharFormat() );
          if( Validate( charFmt, m_Antialias ) ) {
            src->setCharFormat( pos, charsToCopy, charFmt );
          }
          if( firstBlock ) {
            QTextBlock block( src->blocksFind( pos ) );
            if( block.position() == 0 ) {
              firstBlock = false;
              if( !block.charFormat().hasProperty( QTextFormat::FontPointSize ) ) {
                src->setCharFormat( -1, 1, charFmt, QTextDocumentPrivate::SetFormatAndPreserveObjectIndices );
              }
            }
          }
        } 
        pos+=charsToCopy;
      }
      cursor.endEditBlock();
    }
    d->cursor.beginEditBlock();
    d->cursor.insertFragment( fragment );
    d->cursor.endEditBlock();
  }
  
  ensureCursorVisible();
}

} // namespace Akvis
