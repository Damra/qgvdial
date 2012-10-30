#ifndef __GLOBAL_H__
#define __GLOBAL_H__

#ifdef __cplusplus

#include <QtCore>
#include "shared_data_types.h"

#define LOGS_SERVER "http://www.yuvraaj.net"

#if defined(Q_WS_X11)
#define __FULLFUNC__ __PRETTY_FUNCTION__
#else
#define __FULLFUNC__ __FUNCTION__
#endif

#define Q_DEBUG(_s) qDebug() << QString("%1(%2): %3").arg(__FULLFUNC__).arg(__LINE__).arg(_s)
#define Q_WARN(_s) qWarning() << QString("%1(%2): %3").arg(__FULLFUNC__).arg(__LINE__).arg(_s)
#define Q_CRIT(_s) qCritical() << QString("%1(%2): %3").arg(__FULLFUNC__).arg(__LINE__).arg(_s)

#endif //__cplusplus
#endif //__GLOBAL_H__