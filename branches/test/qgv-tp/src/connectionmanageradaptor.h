/*
 * This file was generated by dbusxml2cpp version 0.6
 * Command line was: dbusxml2cpp -i QtTelepathy/Common/BaseTypes -i QtTelepathy/Common/ConnectionManagerTypes -a include/QtTelepathy/Core/connectionmanageradaptor.h: xml/tp-connmgr.xml
 *
 * dbusxml2cpp is Copyright (C) 2006 Trolltech ASA. All rights reserved.
 *
 * This is an auto-generated file.
 * This file may have been hand-edited. Look for HAND-EDIT comments
 * before re-generating it.
 */

#ifndef CONNECTIONMANAGERADAPTOR_H_1172489892
#define CONNECTIONMANAGERADAPTOR_H_1172489892

#include <QtCore/QObject>
#include <QtDBus/QtDBus>
#include "connectionmanagertypes.h"
class QByteArray;
template<class T> class QList;
template<class Key, class Value> class QMap;
class QString;
class QStringList;
class QVariant;

/*
 * Adaptor class for interface org.freedesktop.Telepathy.ConnectionManager
 */
class ConnectionManagerAdaptor: public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.freedesktop.Telepathy.ConnectionManager")
    Q_CLASSINFO("D-Bus Introspection", ""
"  <interface name=\"org.freedesktop.Telepathy.ConnectionManager\" >\n"
"    <method name=\"GetParameters\" >\n"
"      <annotation value=\"org::freedesktop::Telepathy::ParameterDefinitionList\" name=\"com.trolltech.QtDBus.QtTypeName.Out0\" />\n"
"      <arg direction=\"in\" type=\"s\" name=\"proto\" />\n"
"      <arg direction=\"out\" type=\"a(susv)\" />\n"
"    </method>\n"
"    <method name=\"ListProtocols\" >\n"
"      <arg direction=\"out\" type=\"as\" />\n"
"    </method>\n"
"    <method name=\"RequestConnection\" >\n"
"      <annotation value=\"QVariantMap\" name=\"com.trolltech.QtDBus.QtTypeName.In1\" />\n"
"      <arg direction=\"in\" type=\"s\" name=\"proto\" />\n"
"      <arg direction=\"in\" type=\"a{sv}\" name=\"parameters\" />\n"
"      <arg direction=\"out\" type=\"s\" name=\"bus_name\" />\n"
"      <arg direction=\"out\" type=\"o\" name=\"object_path\" />\n"
"    </method>\n"
"    <signal name=\"NewConnection\" >\n"
"      <arg type=\"s\" name=\"bus_name\" />\n"
"      <arg type=\"o\" name=\"object_path\" />\n"
"      <arg type=\"s\" name=\"proto\" />\n"
"    </signal>\n"
"  </interface>\n"
        "")
public:
    ConnectionManagerAdaptor(QObject *parent);
    virtual ~ConnectionManagerAdaptor();

public: // PROPERTIES
public Q_SLOTS: // METHODS
    org::freedesktop::Telepathy::ParameterDefinitionList GetParameters(const QString &proto);
    QStringList ListProtocols();
    QString RequestConnection(const QString &proto, QVariantMap parameters, QDBusObjectPath &object_path);
Q_SIGNALS: // SIGNALS
    void NewConnection(const QString &bus_name, const QDBusObjectPath &object_path, const QString &proto);
};

#endif
