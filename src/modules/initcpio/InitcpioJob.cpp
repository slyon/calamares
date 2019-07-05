/* === This file is part of Calamares - <https://github.com/calamares> ===
 *
 *   Copyright 2019, Adriaan de Groot <groot@kde.org>
 *
 *   Calamares is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   Calamares is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with Calamares. If not, see <http://www.gnu.org/licenses/>.
 */

#include "InitcpioJob.h"

#include "utils/CalamaresUtilsSystem.h"
#include "utils/Logger.h"
#include "utils/UMask.h"
#include "utils/Variant.h"

#include <QDir>
#include <QFile>

InitcpioJob::InitcpioJob( QObject* parent )
    : Calamares::CppJob( parent )
{
}

InitcpioJob::~InitcpioJob() {}


QString
InitcpioJob::prettyName() const
{
    return tr( "Creating initramfs with mkinitcpio." );
}

static void
fixPermissions( const QDir& d )
{
    for ( const auto& fi : d.entryInfoList( { "initramfs*" }, QDir::Files ) )
    {
        QFile f( fi.absoluteFilePath() );
        if ( f.exists() )
        {
            cDebug() << "initcpio fixing permissions for" << f.fileName();
            f.setPermissions( QFileDevice::ReadOwner | QFileDevice::WriteOwner );
        }
    }
}

Calamares::JobResult
InitcpioJob::exec()
{
    CalamaresUtils::UMask m( CalamaresUtils::UMask::Safe );

    QDir d( CalamaresUtils::System::instance()->targetPath( "/boot" ) );
    if ( d.exists() )
    {
        fixPermissions( d );
    }
    
    cDebug() << "Updating initramfs with kernel" << m_kernel;
    auto r = CalamaresUtils::System::instance()->targetEnvCommand(
        { "mkinitcpio", "-p", m_kernel }, QString(), QString(), 0 );
    return r.explainProcess( "mkinitcpio", 10 );
}

void
InitcpioJob::setConfigurationMap( const QVariantMap& configurationMap )
{
    m_kernel = CalamaresUtils::getString( configurationMap, "kernel" );
    if ( m_kernel.isEmpty() )
    {
        m_kernel = QStringLiteral( "all" );
    }
    else if ( m_kernel == "$uname" )
    {
        auto r = CalamaresUtils::System::runCommand(
            CalamaresUtils::System::RunLocation::RunInHost, { "/bin/uname", "-r" }, QString(), QString(), 3 );
        if ( r.getExitCode() == 0 )
        {
            m_kernel = r.getOutput();
            cDebug() << "*initcpio* using running kernel" << m_kernel;
        }
        else
        {
            cWarning() << "*initcpio* could not determine running kernel, using 'all'." << Logger::Continuation
                       << r.getExitCode() << r.getOutput();
        }
    }
}

CALAMARES_PLUGIN_FACTORY_DEFINITION( InitcpioJobFactory, registerPlugin< InitcpioJob >(); )
