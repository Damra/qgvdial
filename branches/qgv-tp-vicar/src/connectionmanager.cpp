/*
@version: 0.5
@author: Sudheer K. <scifi1947 at gmail.com>
@license: GNU General Public License

Based on Telepathy-SNOM with copyright notice below.
*/

/*
 * Telepathy SNOM VoIP phone connection manager
 * Copyright (C) 2006 by basyskom GmbH
 *  @author Tobias Hunger <info@basyskom.de>
 *
 * This library is free software; you can redisQObject::tribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * This library is disQObject::tributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc.,
 * 51 Franklin SQObject::treet, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "connectionmanager.h"
#include "connectionmanageradaptor.h"
#include "connection.h"

#include <QtCore/QDebug>

namespace
{
static const QString protocol_name("tel");
}

class ConnectionManagerPrivate
{
public:
    ConnectionManagerPrivate(ConnectionManager * parent) :
        adaptor(new ConnectionManagerAdaptor(parent))
    { Q_ASSERT(0 != adaptor); }

    ~ConnectionManagerPrivate() { delete(adaptor);}

    ConnectionManagerAdaptor * const adaptor;
};

// ---------------------------------------------------------------------------

ConnectionManager::ConnectionManager(QObject * parent) :
    QObject(parent),
    d(new ConnectionManagerPrivate(this))
{ Q_ASSERT(0 != d); }

ConnectionManager::~ConnectionManager()
{ delete(d); }

org::freedesktop::Telepathy::ParameterDefinitionList
ConnectionManager::GetParameters(const QString &proto)
{
    Q_ASSERT(!proto.isEmpty());
    qDebug() << "VICAR: ConnectionManager::GetParameters(const QString &prot)";
    org::freedesktop::Telepathy::ParameterDefinitionList result;
    org::freedesktop::Telepathy::ParameterDefinition param;

    // Attention! Default constructed QDBusVariants cause havok on the D-Bus!
    param.name = "com.nokia.Telepathy.Connection.Interface.GSM.IMSI";
    param.flags = Register;
    param.signature = "s";
    //param.defaultValue = QDBusVariant(QString());
    result.append(param);

    param.name = "com.nokia.Telepathy.Connection.Interface.GSM.Privacy";
    param.flags = Register|hasDefault;
    param.signature = "s";
    param.defaultValue = QDBusVariant(QString());
    result.append(param);

    param.name = "com.nokia.Telepathy.Connection.Interface.GSM.SMSServiceCentre";
    param.flags = Register;
    param.signature = "s";
    //param.defaultValue = QDBusVariant(QString());
    result.append(param);

    param.name = "com.nokia.Telepathy.Connection.Interface.GSM.SMSValidityPeriod";
    param.flags = Register|hasDefault;
    param.signature = "u";
    param.defaultValue = QDBusVariant(0);
    result.append(param);

    param.name = "account";
    param.flags = None;
    param.signature = "s";
    //param.defaultValue = QDBusVariant(QString());
    result.append(param);

    param.name = "password";
    param.flags = None;
    param.signature = "s";
    //param.defaultValue = QDBusVariant(QString());
    result.append(param);

    return result;
}

QStringList ConnectionManager::ListProtocols()
{
    qDebug() << "VICaR ConnectionManager::ListProtocols()";
    return QStringList(protocol_name);
}

QString ConnectionManager::RequestConnection(const QString & proto,
                                             QVariantMap parameters,
                                             QDBusObjectPath & object_path)
{
    qDebug() << "VICaR CM: Connection Requested...";
    QString connection_service;
    object_path = QDBusObjectPath();

    if (proto != protocol_name)
    {
        /*
        sendErrorReply("org.freedesktop.Telepathy.Error.NotImplemented",
                       "VICaR - Unable to create Connection. Requested protocol is not implemented.");
        */
        qDebug() << "VICaR CM::RequestConnection: proto mismatch.";
        return connection_service;
    }


    QString imsi;
    QString privacy;
    QString smsServiceCenter;
    uint smsValidityPeriod(0);
    QString account;
    QString password;

    // read parameters:
    QString param;
    foreach (param, parameters.keys())
    {
        if ("com.nokia.Telepathy.Connection.Interface.GSM.IMSI" == param)
        { imsi = parameters[param].toString(); }
        else if ("com.nokia.Telepathy.Connection.Interface.GSM.Privacy" == param)
        { privacy = parameters[param].toString(); }
        else if ("com.nokia.Telepathy.Connection.Interface.GSM.SMSServiceCentre" == param)
        { smsServiceCenter = parameters[param].toString(); }
        else if ("com.nokia.Telepathy.Connection.Interface.GSM.SMSValidityPeriod" == param)
        { smsValidityPeriod = parameters[param].toInt(); }
        else if ("account" == param)
        { account = parameters[param].toString(); }
        else if ("password" == param)
        { password = parameters[param].toString(); }
        else
        {
            /*
            sendErrorReply("org.freedesktop.Telepathy.Error.InvalidArgument",
                           "VICaR - Unable to create Connection. Invalid parameters specified.");
            */
            qDebug() << "VICaR CM::RequestConnection: invalid parameter" << param << "found.";
            return connection_service;
        }
    }

    Connection * new_connection = new Connection(account, this);
    Q_ASSERT(0 != new_connection);

    if (!new_connection->registerObject())
    {
        qDebug() << "VICaR CM: Error while registering Connection object with DBus.";
        delete new_connection;
        return QString();
    }

    qDebug() << "VICaR CM: New Connection Created. Status is "<< new_connection->GetStatus();

    object_path = new_connection->objectPath();
    connection_service = new_connection->serviceName();

    emit NewConnection(connection_service, object_path, "tel");

    return new_connection->serviceName();
}
