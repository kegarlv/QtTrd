//  This file is part of Qt Bitcoin Trader
//      https://github.com/JulyIGHOR/QtBitcoinTrader
//  Copyright (C) 2013-2018 July IGHOR <julyighor@gmail.com>
//
//  This program is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  In addition, as a special exception, the copyright holders give
//  permission to link the code of portions of this program with the
//  OpenSSL library under certain conditions as described in each
//  individual source file, and distribute linked combinations including
//  the two.
//
//  You must obey the GNU General Public License in all respects for all
//  of the code used other than OpenSSL. If you modify file(s) with this
//  exception, you may extend this exception to your version of the
//  file(s), but you are not obligated to do so. If you do not wish to do
//  so, delete this exception statement from your version. If you delete
//  this exception statement from all source files in the program, then
//  also delete it here.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include <QUrl>
#include <QNetworkProxyFactory>
#include <QNetworkProxy>
#include <QDir>
#include "main.h"
#include <QLoggingCategory>
#include <QApplication>
#include <QFileInfo>
#include <QSettings>
#include "updaterdialog.h"
#include "login/passworddialog.h"
#include "login/newpassworddialog.h"
#include "julyaes256.h"
#include "translationdialog.h"
#include <QMessageBox>
#include <QDateTime>
#include <QStyleFactory>
#include "datafolderchusedialog.h"
#include <QMetaEnum>
#include <QUuid>
#include <QTranslator>
#include <QLibraryInfo>
#include "julylockfile.h"
#include "login/featuredexchangesdialog.h"
#include "login/allexchangesdialog.h"
#include "config/config_manager.h"
#include "utils/utils.h"
#include "timesync.h"
#include "iniengine.h"

#include <QStyle>
#include <QStandardPaths>

BaseValues* baseValues_;

void selectSystemLanguage()
{
    QString sysLocale = QLocale().name().toLower();

    if (sysLocale.startsWith("de"))
        baseValues.defaultLangFile = ":/Resources/Language/German.lng";
    else if (sysLocale.startsWith("fr"))
        baseValues.defaultLangFile = ":/Resources/Language/French.lng";
    else if (sysLocale.startsWith("zh"))
        baseValues.defaultLangFile = ":/Resources/Language/Chinese.lng";
    else if (sysLocale.startsWith("ru"))
        baseValues.defaultLangFile = ":/Resources/Language/Russian.lng";
    else if (sysLocale.startsWith("uk"))
        baseValues.defaultLangFile = ":/Resources/Language/Ukrainian.lng";
    else if (sysLocale.startsWith("pl"))
        baseValues.defaultLangFile = ":/Resources/Language/Polish.lng";
    else if (sysLocale.startsWith("nl"))
        baseValues.defaultLangFile = ":/Resources/Language/Dutch.lng";
    else if (sysLocale.startsWith("es"))
        baseValues.defaultLangFile = ":/Resources/Language/Spanish.lng";
    else if (sysLocale.startsWith("nb"))
        baseValues.defaultLangFile = ":/Resources/Language/Norwegian.lng";
    else if (sysLocale.startsWith("bg"))
        baseValues.defaultLangFile = ":/Resources/Language/Bulgarian.lng";
    else if (sysLocale.startsWith("cs"))
        baseValues.defaultLangFile = ":/Resources/Language/Czech.lng";
    else if (sysLocale.startsWith("tr"))
        baseValues.defaultLangFile = ":/Resources/Language/Turkish.lng";
    else if (sysLocale.startsWith("it"))
        baseValues.defaultLangFile = ":/Resources/Language/Italiano.lng";
    else
        baseValues.defaultLangFile = ":/Resources/Language/English.lng";
}

void BaseValues::Construct()
{
    forceDotInSpinBoxes = true;
    scriptsThatUseOrderBookCount = 0;
    trafficSpeed = 0;
    trafficTotal = 0;
    trafficTotalType = 0;
    currentExchange_ = nullptr;
    currentTheme = 0;
    gzipEnabled = true;
    appVerIsBeta = false;
    jlScriptVersion = 1.0;
    appVerStr = "1.40130";
    appVerReal = appVerStr.toDouble();

    if (appVerStr.size() > 4)
    {
        if (appVerStr.size() == 7)
            appVerStr.remove(6, 1);

        appVerStr.insert(4, ".");
    }

    appVerLastReal = appVerReal;

    logThread = nullptr;

    highResolutionDisplay = true;
    timeFormat = QLocale().timeFormat(QLocale::LongFormat).replace(" ", "").replace("t", "");
    dateTimeFormat = QLocale().dateFormat(QLocale::ShortFormat) + " " + timeFormat;
    depthCountLimit = 100;
    depthCountLimitStr = "100";
    uiUpdateInterval = 100;
    supportsUtfUI = true;
    debugLevel_ = 0;

#ifdef Q_WS_WIN

    if (QSysInfo::windowsVersion() <= QSysInfo::WV_XP)
        supportsUtfUI = false;

#endif

    upArrow = QByteArray::fromBase64("4oaR");
    downArrow = QByteArray::fromBase64("4oaT");

    if (baseValues.supportsUtfUI)
    {
        upArrowNoUtf8 = upArrow;
        downArrowNoUtf8 = downArrow;
    }
    else
    {
        upArrowNoUtf8 = ">";
        downArrowNoUtf8 = "<";
    }

    httpRequestInterval = 400;
    httpRequestTimeout = 5000;
    httpRetryCount = 5;
    apiDownCount = 0;
    groupPriceValue = 0.0;
    defaultHeightForRow_ = 22;

    selectSystemLanguage();
}

int main(int argc, char* argv[])
{
    QScopedPointer<JulyLockFile> julyLock(nullptr);
#if QT_VERSION < 0x050000
    QTextCodec::setCodecForCStrings(QTextCodec::codecForName("utf8"));
    QTextCodec::setCodecForTr(QTextCodec::codecForName("utf8"));
#endif

    QLoggingCategory::setFilterRules("qt.network.ssl.warning=false");

    baseValues_ = new BaseValues;
    baseValues_->Construct();

#if QT_VERSION >= 0x050600
    QSettings hiDpiSettings("Centrabit", "Qt Bitcoin Trader");

    if (hiDpiSettings.value("HiDPI", true).toBool())
    {
        QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
        QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    }

    else
        QApplication::setAttribute(Qt::AA_Use96Dpi);

#endif

    QApplication a(argc, argv);

    QTranslator qTranslator;
    qTranslator.load("qt_" + QLocale::system().name(), QLibraryInfo::location(QLibraryInfo::TranslationsPath));
    a.installTranslator(&qTranslator);

    TimeSync::global();

#ifdef Q_OS_WIN//DPI Fix
    QFont font = a.font();
    font.setPixelSize(11);
    a.setFont(font);
#endif

    a.setStyle(QStyleFactory::create("Fusion"));

    a.setApplicationName("QtBitcoinTrader");
    a.setApplicationVersion(baseValues.appVerStr);

    baseValues_->fontMetrics_ = new QFontMetrics(a.font());

#if QT_VERSION < 0x050000
    baseValues.tempLocation = QDesktopServices::storageLocation(QDesktopServices::TempLocation).replace('\\', '/') + "/";
    baseValues.desktopLocation = QDesktopServices::storageLocation(QDesktopServices::DesktopLocation).replace('\\', '/') + "/";
#else
    baseValues.tempLocation = QStandardPaths::standardLocations(QStandardPaths::TempLocation).first().replace('\\', '/') + "/";
    baseValues.desktopLocation = QStandardPaths::standardLocations(QStandardPaths::DesktopLocation).first().replace('\\', '/') + "/";
#endif

#ifdef Q_OS_WIN
    {
        QString appLocalStorageDir = a.applicationDirPath() + QLatin1String("/QtBitcoinTrader/");

#if QT_VERSION < 0x050000
        appDataDir = QDesktopServices::storageLocation(QDesktopServices::DataLocation).replace('\\', '/') + "/";
#else
        appDataDir = QStandardPaths::standardLocations(QStandardPaths::DataLocation).first().replace('\\', '/') + "/";
#endif

        if (!QFile::exists(appLocalStorageDir) && !QFile::exists(appDataDir))
        {
            julyTranslator.loadFromFile(baseValues.defaultLangFile);
            DataFolderChuseDialog chuseStorageLocation(appDataDir, appLocalStorageDir);

            if (chuseStorageLocation.exec() == QDialog::Rejected)
                return 0;

            if (chuseStorageLocation.isPortable)
                QDir().mkdir(appLocalStorageDir);
        }

        if (QFile::exists(appLocalStorageDir))
        {
            appDataDir = appLocalStorageDir;
            QDir().mkpath(appDataDir + "Language");

            if (!QFile::exists(appDataDir + "Language"))
                appDataDir.clear();
        }

        if (!QFile::exists(appDataDir + "Language"))
            QDir().mkpath(appDataDir + "Language");

        if (!QFile::exists(appDataDir))
        {
            QMessageBox::warning(0, "Qt Bitcoin Trader",
                                 julyTr("CAN_NOT_WRITE_TO_FOLDER", "Can not write to folder") +
                                 ": \"" + appDataDir + "\"");
            return 0;
        }
    }
#else

#if QT_VERSION < 0x050000
    appDataDir = QDesktopServices::storageLocation(QDesktopServices::DataLocation).replace('\\', '/') + "/";
    QString oldAppDataDir = QDesktopServices::storageLocation(QDesktopServices::HomeLocation) + "/.config/QtBitcoinTrader/";
#else
    appDataDir = QStandardPaths::standardLocations(QStandardPaths::DataLocation).first().replace('\\', '/') + "/";
    QString oldAppDataDir = QStandardPaths::standardLocations(QStandardPaths::HomeLocation).first() + "/.config/QtBitcoinTrader/";
#endif

    if (!QFile::exists(appDataDir) && oldAppDataDir != appDataDir && QFile::exists(oldAppDataDir))
    {
        QFile::rename(oldAppDataDir, appDataDir);

        if (QFile::exists(oldAppDataDir))
        {
            if (!QFile::exists(appDataDir))
                QDir().mkpath(appDataDir);

            QStringList fileList = QDir(oldAppDataDir).entryList();

            for (int n = 0; n < fileList.count(); n++)
                if (fileList.at(n).length() > 2)
                {
                    QFile::copy(oldAppDataDir + fileList.at(n), appDataDir + fileList.at(n));

                    if (QFile::exists(oldAppDataDir + fileList.at(n)))
                        QFile::remove(oldAppDataDir + fileList.at(n));
                }
        }
    }

    if (!QFile::exists(appDataDir))
        QDir().mkpath(appDataDir);

    if (!QFile::exists(appDataDir))
    {
        QMessageBox::warning(0, "Qt Bitcoin Trader",
                             julyTr("CAN_NOT_WRITE_TO_FOLDER", "Can not write to folder") +
                             ": \"" + appDataDir + "\"");
        return 0;
    }

#endif

    if (baseValues.appVerLastReal < 1.0763)
    {
        QFile::rename(appDataDir + "/Settings.set", appDataDir + "/QtBitcoinTrader.cfg");
    }

    if (QFile::exists(appDataDir + "Themes/"))
    {
        baseValues.themeFolder = appDataDir + "Themes/";

        if (!QFile::exists(baseValues.themeFolder + "Dark.thm"))
            QFile::copy("://Resources/Themes/Dark.thm", baseValues.themeFolder + "Dark.thm");

        if (!QFile::exists(baseValues.themeFolder + "Light.thm"))
            QFile::copy("://Resources/Themes/Light.thm", baseValues.themeFolder + "Light.thm");

        if (!QFile::exists(baseValues.themeFolder + "Gray.thm"))
            QFile::copy("://Resources/Themes/Gray.thm", baseValues.themeFolder + "Gray.thm");
    }
    else
        baseValues.themeFolder = "://Resources/Themes/";

    baseValues.appThemeLight.palette = a.palette();
    baseValues.appThemeDark.palette = a.palette();
    baseValues.appThemeGray.palette = a.palette();

    baseValues.appThemeLight.loadTheme("Light");
    baseValues.appThemeDark.loadTheme("Dark");
    baseValues.appThemeGray.loadTheme("Gray");
    baseValues.appTheme = baseValues.appThemeLight;

    QSettings settingsMain(appDataDir + "/QtBitcoinTrader.cfg", QSettings::IniFormat);
    settingsMain.beginGroup("Proxy");

    bool proxyEnabled = settingsMain.value("Enabled", true).toBool();
    bool proxyAuto = settingsMain.value("Auto", true).toBool();
    QString proxyHost = settingsMain.value("Host", "127.0.0.1").toString();
    quint16 proxyPort = settingsMain.value("Port", 1234).toInt();
    QString proxyUser = settingsMain.value("User", "username").toString();
    QString proxyPassword = settingsMain.value("Password", "password").toString();

    QNetworkProxy::ProxyType proxyType;

    if (settingsMain.value("Type", "HttpProxy").toString() == "Socks5Proxy")
        proxyType = QNetworkProxy::Socks5Proxy;
    else
        proxyType = QNetworkProxy::HttpProxy;

    settingsMain.setValue("Enabled", proxyEnabled);
    settingsMain.setValue("Auto", proxyAuto);
    settingsMain.setValue("Host", proxyHost);
    settingsMain.setValue("Port", proxyPort);
    settingsMain.setValue("User", proxyUser);
    settingsMain.setValue("Password", proxyPassword);

    settingsMain.endGroup();

    QNetworkProxy proxy;

    if (proxyEnabled)
    {
        if (proxyAuto)
        {
            QList<QNetworkProxy> proxyList = QNetworkProxyFactory::systemProxyForQuery(QNetworkProxyQuery(QUrl("https://")));

            if (proxyList.count())
                proxy = proxyList.first();
        }
        else
        {
            proxy.setHostName(proxyHost);
            proxy.setUser(proxyUser);
            proxy.setPort(proxyPort);
            proxy.setPassword(proxyPassword);
            proxy.setType(proxyType);
        }

        QNetworkProxy::setApplicationProxy(proxy);
    }

    if (argc > 1)
    {
        if (a.arguments().last().startsWith("/checkupdate"))
        {
            a.setStyleSheet(baseValues.appTheme.styleSheet);

            QSettings settings(appDataDir + "/QtBitcoinTrader.cfg", QSettings::IniFormat);
            QString langFile = settings.value("LanguageFile", "").toString();

            if (langFile.isEmpty() || !QFile::exists(langFile))
                langFile = baseValues.defaultLangFile;

            julyTranslator.loadFromFile(langFile);
            QTimer::singleShot(3600000, &a, &QCoreApplication::quit);
            UpdaterDialog updater(a.arguments().last() != "/checkupdate");
            return a.exec();
        }
    }

#ifdef  Q_OS_WIN

    if (QFile::exists(a.applicationFilePath() + ".upd"))
        QFile::remove(a.applicationFilePath() + ".upd");

    if (QFile::exists(a.applicationFilePath() + ".bkp"))
        QFile::remove(a.applicationFilePath() + ".bkp");

#endif

#ifdef  Q_OS_MAC

    if (QFile::exists(a.applicationFilePath() + ".upd"))
        QFile::remove(a.applicationFilePath() + ".upd");

    if (QFile::exists(a.applicationFilePath() + ".bkp"))
        QFile::remove(a.applicationFilePath() + ".bkp");

#endif

    a.setWindowIcon(QIcon(":/Resources/QtBitcoinTrader.png"));
    {
        baseValues.appVerLastReal = settingsMain.value("Version", 1.0).toDouble();

        if (baseValues.appVerLastReal != baseValues.appVerReal)
        {
            settingsMain.setValue("Version", baseValues.appVerReal);
            QStringList cacheFiles = QDir(appDataDir + "cache").entryList(QStringList("*.cache"), QDir::Files);

            for (int i = 0; i < cacheFiles.count(); ++i)
                QFile(appDataDir + "cache/" + cacheFiles.at(i)).remove();
        }

        IniEngine::global();

        baseValues_->appVerIsBeta = settingsMain.value("CheckForUpdatesBeta", false).toBool();
        baseValues_->use24HourTimeFormat = settingsMain.value("Use24HourTimeFormat", true).toBool();

#if QT_VERSION < 0x050000
        bool plastiqueStyle = true;
        plastiqueStyle = settingsMain.value("PlastiqueStyle", plastiqueStyle).toBool();
        settingsMain.setValue("PlastiqueStyle", plastiqueStyle);

        if (plastiqueStyle)
            a.setStyle(new QPlastiqueStyle);

#endif
        baseValues.scriptFolder = appDataDir + "Scripts/";
        baseValues.osStyle = a.style()->objectName();

        a.setPalette(baseValues.appTheme.palette);
        a.setStyleSheet(baseValues.appTheme.styleSheet);

        settingsMain.beginGroup("Decimals");
        baseValues.decimalsAmountMyTransactions = settingsMain.value("AmountMyTransactions", 8).toInt();
        baseValues.decimalsPriceMyTransactions = settingsMain.value("PriceMyTransactions", 8).toInt();
        baseValues.decimalsTotalMyTransactions = settingsMain.value("TotalMyTransactions", 8).toInt();
        baseValues.decimalsAmountOrderBook = settingsMain.value("AmountOrderBook", 8).toInt();
        baseValues.decimalsPriceOrderBook = settingsMain.value("PriceOrderBook", 8).toInt();
        baseValues.decimalsTotalOrderBook = settingsMain.value("TotalOrderBook", 8).toInt();
        baseValues.decimalsAmountLastTrades = settingsMain.value("AmountLastTrades", 8).toInt();
        baseValues.decimalsPriceLastTrades = settingsMain.value("PriceLastTrades", 8).toInt();
        baseValues.decimalsTotalLastTrades = settingsMain.value("TotalLastTrades", 8).toInt();
        /*baseValues.decimalsAmountMyTransactions=settingsMain.value("AmountMyTransactions",baseValues.currentPair.currADecimals).toInt();
        baseValues.decimalsPriceMyTransactions=settingsMain.value("PriceMyTransactions",8).toInt();
        baseValues.decimalsTotalMyTransactions=settingsMain.value("TotalMyTransactions",baseValues.currentPair.currADecimals).toInt();
        baseValues.decimalsAmountOrderBook=settingsMain.value("AmountOrderBook",baseValues.currentPair.currADecimals).toInt();
        baseValues.decimalsPriceOrderBook=settingsMain.value("PriceOrderBook",baseValues.currentPair.priceDecimals).toInt();
        baseValues.decimalsTotalOrderBook=settingsMain.value("TotalOrderBook",baseValues.currentPair.currADecimals).toInt();
        baseValues.decimalsAmountLastTrades=settingsMain.value("AmountLastTrades",baseValues.currentPair.currADecimals).toInt();
        baseValues.decimalsPriceLastTrades=settingsMain.value("PriceLastTrades",baseValues.currentPair.priceDecimals).toInt();
        baseValues.decimalsTotalLastTrades=settingsMain.value("TotalLastTrades",baseValues.currentPair.currADecimals).toInt();*/
        settingsMain.endGroup();

        baseValues.logFileName = QLatin1String("QtBitcoinTrader.log");
        baseValues.iniFileName = QLatin1String("QtBitcoinTrader.ini");

        {
            if (!QFile::exists(appDataDir + "Language"))
                QDir().mkpath(appDataDir + "Language");

            QString langFile = settingsMain.value("LanguageFile", "").toString();

            if (langFile.isEmpty() || !QFile::exists(langFile))
                langFile = baseValues.defaultLangFile;

            julyTranslator.loadFromFile(langFile);
        }

        bool tryDecrypt = true;
        bool showNewPasswordDialog = false;

        while (tryDecrypt)
        {
            QString tryPassword;
            baseValues.restKey.clear();
            baseValues.restSign.clear();
            bool noProfiles = QDir(appDataDir, "*.ini").entryList().isEmpty();

            if (noProfiles || showNewPasswordDialog)
            {
                FeaturedExchangesDialog featuredExchanges;

                if (featuredExchanges.exchangeNum != -3)
                {
                    int execResult = featuredExchanges.exec();

                    if (noProfiles && execResult == QDialog::Rejected)
                        return 0;

                    if (featuredExchanges.exchangeNum != -2)
                        if (execResult == QDialog::Rejected || featuredExchanges.exchangeNum == -1)
                        {
                            showNewPasswordDialog = false;
                            continue;
                        }
                }

                qint32 exchangeNumber = 0;

                if (featuredExchanges.exchangeNum >= 0)
                    exchangeNumber = featuredExchanges.exchangeNum;
                else
                {
                    AllExchangesDialog allExchanges(featuredExchanges.exchangeNum);

                    if (allExchanges.exec() == QDialog::Rejected)
                    {
                        showNewPasswordDialog = false;
                        continue;
                    }

                    if (allExchanges.exchangeNum == -1)
                    {
                        showNewPasswordDialog = false;
                        continue;
                    }

                    if (allExchanges.exchangeNum == -2)
                        continue;

                    exchangeNumber = allExchanges.exchangeNum;
                }

                NewPasswordDialog newPassword(exchangeNumber);

                if (newPassword.exec() == QDialog::Accepted)
                {
                    tryPassword = newPassword.getPassword();
                    newPassword.updateIniFileName();
                    baseValues.restKey = newPassword.getRestKey().toLatin1();
                    QSettings settings(baseValues.iniFileName, QSettings::IniFormat);
                    settings.setValue("Profile/ExchangeId", newPassword.getExchangeId());
                    settings.sync();

                    if (!QFile::exists(baseValues.iniFileName))
                        QMessageBox::warning(0, "Qt Bitcoin Trader", "Can't write file: \"" + baseValues.iniFileName + "\"");

                    QByteArray encryptedData;

                    switch (newPassword.getExchangeId())
                    {
                        case 0:
                            {
                                //Secret Exchange
                                baseValues.restSign = newPassword.getRestSign().toLatin1();
                                encryptedData = JulyAES256::encrypt("Qt Bitcoin Trader\r\n" + baseValues.restKey + "\r\n" +
                                                                    baseValues.restSign.toBase64() + "\r\n" +
                                                                    QUuid::createUuid().toString().toLatin1(), tryPassword.toUtf8());
                            }
                            break;

                        case 1:
                            {
                                //WEX
                                baseValues.restSign = newPassword.getRestSign().toLatin1();
                                encryptedData = JulyAES256::encrypt("Qt Bitcoin Trader\r\n" + baseValues.restKey + "\r\n" +
                                                                    baseValues.restSign.toBase64() + "\r\n" +
                                                                    QUuid::createUuid().toString().toLatin1(), tryPassword.toUtf8());
                            }
                            break;

                        case 2:
                            {
                                //Bitstamp
                                baseValues.restSign = newPassword.getRestSign().toLatin1();
                                encryptedData = JulyAES256::encrypt("Qt Bitcoin Trader\r\n" + baseValues.restKey + "\r\n" +
                                                                    baseValues.restSign.toBase64() + "\r\n" +
                                                                    QUuid::createUuid().toString().toLatin1(), tryPassword.toUtf8());
                            }
                            break;

                        case 3:
                            {
                                //BTC China
                                baseValues.restSign = newPassword.getRestSign().toLatin1();
                                encryptedData = JulyAES256::encrypt("Qt Bitcoin Trader\r\n" + baseValues.restKey + "\r\n" +
                                                                    baseValues.restSign.toBase64() + "\r\n" +
                                                                    QUuid::createUuid().toString().toLatin1(), tryPassword.toUtf8());
                            }
                            break;

                        case 4:
                            {
                                //Bitfinex
                                baseValues.restSign = newPassword.getRestSign().toLatin1();
                                encryptedData = JulyAES256::encrypt("Qt Bitcoin Trader\r\n" + baseValues.restKey + "\r\n" +
                                                                    baseValues.restSign.toBase64() + "\r\n" +
                                                                    QUuid::createUuid().toString().toLatin1(), tryPassword.toUtf8());
                            }
                            break;

                        case 5:
                            {
                                //GOCio
                                baseValues.restSign = newPassword.getRestSign().toLatin1();
                                encryptedData = JulyAES256::encrypt("Qt Bitcoin Trader\r\n" + baseValues.restKey + "\r\n" +
                                                                    baseValues.restSign.toBase64() + "\r\n" +
                                                                    QUuid::createUuid().toString().toLatin1(), tryPassword.toUtf8());
                            }
                            break;

                        case 6:
                            {
                                //Indacoin
                                baseValues.restSign = newPassword.getRestSign().toLatin1();
                                encryptedData = JulyAES256::encrypt("Qt Bitcoin Trader\r\n" + baseValues.restKey + "\r\n" +
                                                                    baseValues.restSign.toBase64() + "\r\n" +
                                                                    QUuid::createUuid().toString().toLatin1(), tryPassword.toUtf8());
                            }
                            break;

                        case 8:
                            {
                                //BitMarket
                                baseValues.restSign = newPassword.getRestSign().toLatin1();
                                encryptedData = JulyAES256::encrypt("Qt Bitcoin Trader\r\n" + baseValues.restKey + "\r\n" +
                                                                    baseValues.restSign.toBase64() + "\r\n" +
                                                                    QUuid::createUuid().toString().toLatin1(), tryPassword.toUtf8());
                            }
                            break;

                        case 9:
                            {
                                //OKCoin
                                baseValues.restSign = newPassword.getRestSign().toLatin1();
                                encryptedData = JulyAES256::encrypt("Qt Bitcoin Trader\r\n" + baseValues.restKey + "\r\n" +
                                                                    baseValues.restSign.toBase64() + "\r\n" +
                                                                    QUuid::createUuid().toString().toLatin1(), tryPassword.toUtf8());
                            }
                            break;

                        case 10:
                            {
                                //YObit
                                baseValues.restSign = newPassword.getRestSign().toLatin1();
                                encryptedData = JulyAES256::encrypt("Qt Bitcoin Trader\r\n" + baseValues.restKey + "\r\n" +
                                                                    baseValues.restSign.toBase64() + "\r\n" +
                                                                    QUuid::createUuid().toString().toLatin1(), tryPassword.toUtf8());
                            }
                            break;

                        //TODO add custom exchange here
                    case 11:
                        baseValues.restSign = newPassword.getRestSign().toLatin1();
                        encryptedData = JulyAES256::encrypt("Qt Bitcoin Trader\r\n" + baseValues.restKey + "\r\n" +
                                                            baseValues.restSign.toBase64() + "\r\n" +
                                                            QUuid::createUuid().toString().toLatin1(), tryPassword.toUtf8());
                        default:
                            break;
                    }

                    settings.setValue("EncryptedData/ApiKeySign", QString(encryptedData.toBase64()));
                    settings.setValue("Profile/Name", newPassword.selectedProfileName());
                    settings.sync();

                    showNewPasswordDialog = false;
                }
            }

            PasswordDialog enterPassword;

            if (enterPassword.exec() == QDialog::Rejected)
                return 0;

            if (enterPassword.resetData)
            {
                QString iniToRemove = enterPassword.getIniFilePath();

                if (QFile::exists(iniToRemove))
                {
                    QFile::remove(iniToRemove);
                    QString scriptFolder = baseValues.scriptFolder + "/" + QFileInfo(iniToRemove).completeBaseName() + "/";

                    if (QFile::exists(scriptFolder))
                    {
                        QStringList filesToRemove = QDir(scriptFolder).entryList(QStringList() << "*.JLS" << "*.JLR");

                        Q_FOREACH (QString curFile, filesToRemove)
                            QFile::remove(scriptFolder + curFile);

                        QDir().rmdir(scriptFolder);
                    }
                }

                continue;
            }

            if (enterPassword.newProfile)
            {
                showNewPasswordDialog = true;
                continue;
            }

            tryPassword = enterPassword.getPassword();

            if (!tryPassword.isEmpty())
            {
                baseValues.iniFileName = enterPassword.getIniFilePath();
                baseValues.logFileName = baseValues.iniFileName;
                baseValues.logFileName.replace(".ini", ".log", Qt::CaseInsensitive);

                if (julyLock)
                    julyLock->free();

                julyLock.reset(new JulyLockFile(baseValues.iniFileName, appDataDir));
                bool profileLocked = julyLock->isLocked();

                if (profileLocked)
                {
                    QMessageBox msgBox(0);
                    msgBox.setIcon(QMessageBox::Question);
                    msgBox.setWindowTitle("Qt Bitcoin Trader");
                    msgBox.setText(julyTr("THIS_PROFILE_ALREADY_USED",
                                          "This profile is already used by another instance.<br>API does not allow to run two instances with same key sign pair.<br>Please create new profile if you want to use two instances."));
                    msgBox.setStandardButtons(QMessageBox::Ok);
                    msgBox.setDefaultButton(QMessageBox::Ok);
                    msgBox.exec();

                    if (profileLocked)
                        tryPassword.clear();
                }

                if (!profileLocked)
                {
                    qDebug() << baseValues.iniFileName;
                    QSettings settings(baseValues.iniFileName, QSettings::IniFormat);
                    QStringList decryptedList = QString(JulyAES256::decrypt(QByteArray::fromBase64(
                                                            settings.value("EncryptedData/ApiKeySign", "").toString().toLatin1()),
                                                        tryPassword.toUtf8())).split("\r\n");

                    if (decryptedList.count() < 3 || decryptedList.first() != "Qt Bitcoin Trader")
                    {
                        decryptedList = QString(JulyAES256::decrypt(QByteArray::fromBase64(settings.value("EncryptedData/ApiKeySign",
                                                "").toString().toLatin1()), tryPassword.toLatin1())).split("\r\n");
                    }
                    if (decryptedList.count() >= 3 && decryptedList.first() == "Qt Bitcoin Trader")
                    {
                        baseValues.restKey = decryptedList.at(1).toLatin1();
                        baseValues.restSign = QByteArray::fromBase64(decryptedList.at(2).toLatin1());

                        if (decryptedList.count() == 3)
                        {
                            decryptedList << QUuid::createUuid().toString().toLatin1();
                            settings.setValue("EncryptedData/ApiKeySign", QString(JulyAES256::encrypt("Qt Bitcoin Trader\r\n" +
                                              decryptedList.at(1).toLatin1() + "\r\n" + decryptedList.at(2).toLatin1() + "\r\n" +
                                              decryptedList.at(3).toLatin1(), tryPassword.toUtf8()).toBase64()));
                            settings.sync();
                        }

                        baseValues.randomPassword = decryptedList.at(3).toLatin1();
                        tryDecrypt = false;
                    }
                }
            }
        }

        baseValues.scriptFolder += QFileInfo(baseValues.iniFileName).completeBaseName() + "/";

        QSettings iniSettings(baseValues.iniFileName, QSettings::IniFormat);
        if (iniSettings.value("Debug/LogEnabled", false).toBool())
            debugLevel = 1;


        iniSettings.setValue("Debug/LogEnabled", debugLevel > 0);
        baseValues.logThread_ = 0;

        if (debugLevel)
        {
            baseValues.logThread_ = new LogThread;
            logThread->writeLog("Proxy settings: " + proxy.hostName().toUtf8() + ":" + QByteArray::number(
                                    proxy.port()) + " " + proxy.user().toUtf8());
        }

        ::config = new ConfigManager(slash(appDataDir, "QtBitcoinTrader.ws.cfg"), &a);
        baseValues.mainWindow_ = new QtBitcoinTrader;
    }

    baseValues.mainWindow_->setupClass();
    a.exec();

    //delete baseValues_->fontMetrics_;
    //delete baseValues_;

    return 0;
}