//
// Created by kegar on 9/3/18.
//

#include "StexExchange.h"
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <openssl/ecdsa.h>
#include <openssl/hmac.h>

void StexExchange::clearVariables() {
    Exchange::clearVariables();
}

StexExchange::~StexExchange() {}

void StexExchange::secondSlot() {

    static int sendCounter = 0;

    switch (sendCounter) {
    case 0:
        if (!isReplayPending(103))
            sendToApi(103,
                      "grafic_public?pair=" + baseValues.currentPair.currRequestPair +
                          "&interval=1D&order=ASC&count=100",
                      false, true);

        break;
    case 1:
        if (!isReplayPending(202))
            sendToApi(202, "GetInfo", true, true, "");

        break;

    case 2:
        if (!isReplayPending(109))
            sendToApi(109, "trades?pair=" + baseValues.currentPair.currRequestPair, false, true);

        break;
    case 3:
        if (!tickerOnly && !isReplayPending(204))
            sendToApi(204, "ActiveOrders", true, true, "");

        break;
    case 4:
        if (isDepthEnabled() && (forceDepthLoad || !isReplayPending(111))) {
            emit depthRequested();
            sendToApi(111,
                      "orderbook?pair=" +
                          baseValues.currentPair
                              .currRequestPair /*+"?limit="+baseValues.depthCountLimitStr*/,
                      false, true);
            forceDepthLoad = false;
        }

        break;

    case 5:
        if (lastHistory.isEmpty())
            getHistory(false);

        break;

    case 6:
        sendToApi(112, "ticker", false, true);
        break;

    default:
        break;
    }

    if (sendCounter++ >= 6)
        sendCounter = 0;

    Exchange::secondSlot();
}

void StexExchange::dataReceivedAuth(QByteArray data, int i) {
    if (debugLevel) {
        logThread->writeLog("RCV: " + data);
    }

    if (data.size() == 0) {
        return;
    }

    switch (i) {
    case 112: {
        QJsonArray root = QJsonDocument::fromJson(data).array();
        for (auto ticker : root) {
            if (ticker.toObject().value("market_name").toString() ==
                baseValues.currentPair.currRequestPair) {
                auto tickerObj = ticker.toObject();
                double tickerHigh = tickerObj.value("ask").toString().toDouble();
                double tickerLow = tickerObj.value("bid").toString().toDouble();
                double tickerLast = tickerObj.value("last").toString().toDouble();
                double tickerVol = tickerObj.value("vol").toString().toDouble();

                IndicatorEngine::setValue(baseValues.exchangeName, baseValues.currentPair.symbol,
                                          "Sell", tickerHigh);
                IndicatorEngine::setValue(baseValues.exchangeName, baseValues.currentPair.symbol,
                                          "Buy", tickerLow);
                IndicatorEngine::setValue(baseValues.exchangeName, baseValues.currentPair.symbol,
                                          "Last", tickerLast);
                IndicatorEngine::setValue(baseValues.exchangeName, baseValues.currentPair.symbol,
                                          "Volume", tickerVol);
            }
        }

    } break;
    case 103:
        {
            QJsonObject rootObj = QJsonDocument::fromJson(data).object();

            QJsonArray graphArray = rootObj.value("data").toObject().value("graf").toArray();
            QJsonObject graphValue = graphArray.last().toObject();

            double newTickerHigh = graphValue.value("high").toString().toDouble();
            double newTickerLow = graphValue.value("low").toString().toDouble();
            //                double newTickerLast = graphValue.value("last").toString().toDouble();
            double newTickerVolume = graphValue.value("volume").toString().toDouble();

            IndicatorEngine::setValue(baseValues.exchangeName, baseValues.currentPair.symbol,
                                      "High", newTickerHigh);
            IndicatorEngine::setValue(baseValues.exchangeName, baseValues.currentPair.symbol, "Low",
                                      newTickerLow);
        }
        break;
    case 109: {
        auto list = new QList<TradesItem>();
        QJsonObject root = QJsonDocument::fromJson(data).object();
        QJsonArray arr = root.value("result").toArray();
        for (auto transaction : arr) {
            TradesItem item;
            QString type = transaction.toObject().value("type").toString();
            item.date = transaction.toObject().value("timestamp").toInt();
            item.orderType = type == "BUY" ? 1 : -1;
            item.amount = transaction.toObject().value("quantity").toString().toDouble();
            item.price = transaction.toObject().value("price").toString().toDouble();

            item.symbol = baseValues.currentPair.symbol;
            if (item.isValid()) {
                (*list) << item;
            }
        }

        QVariant lastItem = (std::max_element(arr.begin(), arr.end(),
                                              [](const QJsonValue &a, const QJsonValue &b) {
                                                  return a.toObject().value("timestamp").toString().toLongLong() <
                                                         b.toObject().value("timestamp").toString().toLongLong();
                                              }))
                                ->toVariant();
//        IndicatorEngine::setValue(baseValues.exchangeName, baseValues.currentPair.symbol, "Last",
//                                  lastItem.toMap().value("price").toString().toDouble());

        std::sort(list->begin(), list->end(), [](const TradesItem &i1, const TradesItem &i2) {
            return i1.date < i2.date;
        });
        emit this->addLastTrades(baseValues.currentPair.symbol, list);
    } break;
    case 111:
        emit depthRequestReceived();
        {

            if (lastDepthData != data) {
                lastDepthData = data;
                depthAsks = new QList<DepthItem>;
                depthBids = new QList<DepthItem>;

                QMap<double, double> currentAsksMap;
                QJsonObject response = QJsonDocument::fromJson(data).object();
                QJsonArray asksArray = response.value("result").toObject().value("buy").toArray();
                QJsonArray bidsArray = response.value("result").toObject().value("sell").toArray();

                double groupedPrice = 0.0;
                double groupedVolume = 0.0;
                int rowCounter = 0;

                if (asksArray.count() == 0)
                    IndicatorEngine::setValue(baseValues.exchangeName,
                                              baseValues.currentPair.symbol, "Buy", 0);

                for (int n = 0; n < asksArray.count(); n++) {
                    if (baseValues.depthCountLimit && rowCounter >= baseValues.depthCountLimit)
                        break;

                    QJsonObject currentPair = asksArray.at(n).toObject();

                    double amount = currentPair.value("Quantity").toString().toDouble();
                    double priceDouble = currentPair.value("Rate").toString().toDouble();

                                        if (n == 0) {
                                            if (priceDouble != lastTickerBuy)
                                                IndicatorEngine::setValue(baseValues.exchangeName,
                                                                          baseValues.currentPair.symbol,
                                                                          "Buy", priceDouble);

                                            lastTickerBuy = priceDouble;
                                        }

                    if (baseValues.groupPriceValue > 0.0) {
                        if (n == 0) {
                            emit depthFirstOrder(baseValues.currentPair.symbol, priceDouble, amount,
                                                 true);
                            groupedPrice = baseValues.groupPriceValue *
                                           (int)(priceDouble / baseValues.groupPriceValue);
                            groupedVolume = amount;
                        } else {
                            bool matchCurrentGroup =
                                priceDouble < groupedPrice + baseValues.groupPriceValue;

                            if (matchCurrentGroup)
                                groupedVolume += amount;

                            if (!matchCurrentGroup || n == asksArray.count() - 1) {
                                depthSubmitOrder(baseValues.currentPair.symbol, &currentAsksMap,
                                                 groupedPrice + baseValues.groupPriceValue,
                                                 groupedVolume, true);
                                rowCounter++;
                                groupedVolume = amount;
                                groupedPrice += baseValues.groupPriceValue;
                            }
                        }
                    } else {
                        depthSubmitOrder(baseValues.currentPair.symbol, &currentAsksMap,
                                         priceDouble, amount, true);
                        rowCounter++;
                    }
                }

                QList<double> currentAsksList = lastDepthAsksMap.keys();

                for (int n = 0; n < currentAsksList.count(); n++) {
                    if (currentAsksMap.value(currentAsksList.at(n), 0) == 0) {
                        depthUpdateOrder(baseValues.currentPair.symbol, currentAsksList.at(n), 0.0,
                                         true);
                    }
                }

                lastDepthAsksMap = currentAsksMap;

                QMap<double, double> currentBidsMap;
                groupedPrice = 0.0;
                groupedVolume = 0.0;
                rowCounter = 0;

                                if (bidsArray.count() == 0)
                                    IndicatorEngine::setValue(baseValues.exchangeName,
                                                              baseValues.currentPair.symbol,
                                                              "Sell", 0);

                for (int n = 0; n < bidsArray.count(); n++) {
                    if (baseValues.depthCountLimit && rowCounter >= baseValues.depthCountLimit)
                        break;

                    QJsonObject currentPair = bidsArray.at(n).toObject();

                    if (currentPair.count() != 2)
                        continue;

                    double priceDouble = currentPair.value("Rate").toString().toDouble();
                    double amount = currentPair.value("Quantity").toString().toDouble();

                                        if (n == 0) {
                                            if (priceDouble != lastTickerSell)
                                                IndicatorEngine::setValue(baseValues.exchangeName,
                                                                          baseValues.currentPair.symbol,
                                                                          "Sell", priceDouble);

                                            lastTickerSell = priceDouble;
                                        }

                    if (baseValues.groupPriceValue > 0.0) {
                        if (n == 0) {
                            emit depthFirstOrder(baseValues.currentPair.symbol, priceDouble, amount,
                                                 false);
                            groupedPrice = baseValues.groupPriceValue *
                                           (int)(priceDouble / baseValues.groupPriceValue);
                            groupedVolume = amount;
                        } else {
                            bool matchCurrentGroup =
                                priceDouble > groupedPrice - baseValues.groupPriceValue;

                            if (matchCurrentGroup)
                                groupedVolume += amount;

                            if (!matchCurrentGroup || n == asksArray.count() - 1) {
                                depthSubmitOrder(baseValues.currentPair.symbol, &currentBidsMap,
                                                 groupedPrice - baseValues.groupPriceValue,
                                                 groupedVolume, false);
                                rowCounter++;
                                groupedVolume = amount;
                                groupedPrice -= baseValues.groupPriceValue;
                            }
                        }
                    } else {
                        depthSubmitOrder(baseValues.currentPair.symbol, &currentBidsMap,
                                         priceDouble, amount, false);
                        rowCounter++;
                    }
                }

                QList<double> currentBidsList = lastDepthBidsMap.keys();

                for (int n = 0; n < currentBidsList.count(); n++)
                    if (currentBidsMap.value(currentBidsList.at(n), 0) == 0)
                        depthUpdateOrder(baseValues.currentPair.symbol, currentBidsList.at(n), 0.0,
                                         false);

                lastDepthBidsMap = currentBidsMap;

                emit depthSubmitOrders(baseValues.currentPair.symbol, depthAsks, depthBids);
                depthAsks = 0;
                depthBids = 0;
            } else if (debugLevel) {
                logThread->writeLog("Invalid depth data:" + data, 2);
            }
        }
        break;

    case 202: {
        QJsonObject root = QJsonDocument::fromJson(data).object();
        QJsonObject funds = root.value(data).toObject().value("funds").toObject();
        // A in baseValues.currentPair
        double newBtcBalance =
            funds.value(baseValues.currentPair.currAStr.toUpper()).toString().toDouble();
        emit accBtcBalanceChanged(baseValues.currentPair.symbol, newBtcBalance);

        double newUsdBalance =
            funds.value(baseValues.currentPair.currBStr.toUpper()).toString().toDouble();
        emit accUsdBalanceChanged(baseValues.currentPair.symbol, newUsdBalance);
    } break;
    case 204:
        {
            QJsonObject root = QJsonDocument::fromJson(data).object();
            QJsonObject ordersObj = root.value("data").toObject();
            QList<OrderItem> *orders = new QList<OrderItem>;
            for (auto &&x : ordersObj.keys()) {
                QJsonObject order = ordersObj.value(x).toObject();
                OrderItem currentOrder;

                currentOrder.oid = x.toUtf8();
                currentOrder.date = order.value("timestamp").toInt();
                currentOrder.type = (order.value("type").toString() == "sell");
                currentOrder.status = order.value("is_your_order").toInt() + 1;
                currentOrder.amount = order.value("amount").toString().toDouble();
                currentOrder.price = order.value("rate").toString().toDouble();
                currentOrder.symbol = order.value("pair").toString().replace('_', "/");

                if (currentOrder.isValid())
                    (*orders) << currentOrder;
            }

            emit orderBookChanged(baseValues.currentPair.symbol, orders);
        }
        break;
    case 305:
        break;
    case 306:
        break;
    case 307:
        break;
    case 208:
        {
            auto list = new QList<HistoryItem>();
            QJsonObject root = QJsonDocument::fromJson(data).object();
            for (auto transaction : root.value("result").toArray()) {
                HistoryItem item;
                item.dateTimeInt = transaction.toObject().value("timestamp").toInt();
                QString type = transaction.toObject().value("type").toString();
                if (type == "BUY") {
                    item.type = 2;
                } else if (type == "SELL") {
                    item.type = 1;
                }

                item.volume = transaction.toObject().value("quantity").toString().toDouble();
                item.price = transaction.toObject().value("price").toString().toDouble();
                item.symbol = baseValues.currentPair.symbol;
                (*list) << item;
            }
            emit this->historyChanged(list);
        }
        break;

    default:
        break;
    }
}

void StexExchange::reloadDepth() {
    Exchange::reloadDepth();
}

void StexExchange::clearValues() {
    if (julyHttp)
        julyHttp->clearPendingData();

    m_tradesCache.clear();
}

void StexExchange::getHistory(bool force) {

    if (tickerOnly)
        return;

    if (force)
        lastHistory.clear();

    if (!isReplayPending(208))
        sendToApi(208, "TradeHistory", true, true, "");
}

void StexExchange::buy(QString symbol, double apiBtcToBuy, double apiPriceToBuy) {
    if (tickerOnly)
        return;

    CurrencyPairItem pairItem;
    pairItem = baseValues.currencyPairMap.value(symbol, pairItem);

    if (pairItem.symbol.isEmpty())
        return;

    QByteArray payload = "type:\'BUY\', pair:\'" + pairItem.currRequestPair.toUpper() +
                         "\', amount:\'" +

                         JulyMath::byteArrayFromDouble(apiBtcToBuy, 8, 0) + "\', rate:\'" +
                         JulyMath::byteArrayFromDouble(apiPriceToBuy, 8, 0) + "\'";
    QByteArray data = "pair:'" + pairItem.currRequestPair.toUpper() + "',price:'" +
                      JulyMath::byteArrayFromDouble(apiPriceToBuy, pairItem.priceDecimals, 0) +
                      "',amount:'" +
                      JulyMath::byteArrayFromDouble(apiBtcToBuy, pairItem.currADecimals, 0) + "'";

    if (debugLevel)
        logThread->writeLog("Buy: " + data, 2);

    sendToApi(306, "Trade", true, true, data);
}

void StexExchange::sell(QString symbol, double apiBtcToSell, double apiPriceToSell) {
    if (tickerOnly)
        return;

    CurrencyPairItem pairItem;
    pairItem = baseValues.currencyPairMap.value(symbol, pairItem);

    if (pairItem.symbol.isEmpty())
        return;

    QByteArray payload = "type:\'SELL\', pair:\'" + pairItem.currRequestPair.toUpper() +
                         "\', amount:\'" + JulyMath::byteArrayFromDouble(apiBtcToSell, 8, 0) +
                         "\', rate:\'" + JulyMath::byteArrayFromDouble(apiPriceToSell, 8, 0) + "\'";

    if (debugLevel)
        logThread->writeLog("Sell: " + payload, 2);

    sendToApi(307, "Trade", true, true, payload);
}

void StexExchange::cancelOrder(QString string, QByteArray order) {
    if (this->tickerOnly)
        return;

    if (debugLevel)
        logThread->writeLog("Cancel order: " + order, 2);

    sendToApi(305, "cancelOrder", true, true, "order_id:" + order);
}

StexExchange::StexExchange(QByteArray pRestSign, QByteArray pRestKey)
    : Exchange() {
    orderBookItemIsDedicatedOrder = true;
    clearHistoryOnCurrencyChanged = true;
    isLastTradesTypeSupported = false;
    calculatingFeeMode = 1;
    baseValues.exchangeName = "StocksExchange";

    setApiKeySecret(pRestKey, pRestSign);

    forceDepthLoad = false;
    tickerOnly = false;

    currencyMapFile = "StocksExchange";
    baseValues.currentPair.name = "LTC_BTC";
    baseValues.currentPair.setSymbol("LTC_BTC");
    baseValues.currentPair.currRequestPair = "LTC_BTC";
    baseValues.currentPair.priceDecimals = 5;
    minimumRequestIntervalAllowed = 1500;
    baseValues.currentPair.priceMin = qPow(0.1, baseValues.currentPair.priceDecimals);
    baseValues.currentPair.tradeVolumeMin = 0.01;
    baseValues.currentPair.tradePriceMin = 0.1;
    defaultCurrencyParams.currADecimals = 8;
    defaultCurrencyParams.currBDecimals = 5;
    defaultCurrencyParams.currABalanceDecimals = 8;
    defaultCurrencyParams.currBBalanceDecimals = 5;
    defaultCurrencyParams.priceDecimals = 5;
    defaultCurrencyParams.priceMin = qPow(0.1, baseValues.currentPair.priceDecimals);

    supportsLoginIndicator = false;
    supportsAccountVolume = false;

    julyHttp = new JulyHttp("app.stocks.exchange", "", this, true, true,
                            "application/json; charset=UTF-8");
    connect(julyHttp, SIGNAL(anyDataReceived()), baseValues_->mainWindow_, SLOT(anyDataReceived()));
    connect(julyHttp, SIGNAL(apiDown(bool)), baseValues_->mainWindow_, SLOT(setApiDown(bool)));
    connect(julyHttp, SIGNAL(setDataPending(bool)), baseValues_->mainWindow_,
            SLOT(setDataPending(bool)));
    connect(julyHttp, SIGNAL(errorSignal(QString)), baseValues_->mainWindow_,
            SLOT(showErrorMessage(QString)));
    connect(julyHttp, SIGNAL(sslErrorSignal(const QList<QSslError> &)), this,
            SLOT(sslErrors(const QList<QSslError> &)));
    connect(julyHttp, SIGNAL(dataReceived(QByteArray, int)), this,
            SLOT(dataReceivedAuth(QByteArray, int)));
}

void StexExchange::sslErrors(const QList<QSslError> &errors) {
    QStringList errorList;

    for (int n = 0; n < errors.count(); n++)
        errorList << errors.at(n).errorString();

    if (debugLevel)
        logThread->writeLog(errorList.join(" ").toLatin1(), 2);

    emit showErrorMessage("SSL Error: " + errorList.join(" "));
}

void StexExchange::sendToApi(int reqType, QByteArray method, bool auth, bool sendNow,
                               QByteArray commands) {
    if (julyHttp == 0) {
        julyHttp = new JulyHttp("app.stex.com", "", this, true, true,
                                "application/json; charset=UTF-8");
        connect(julyHttp, SIGNAL(anyDataReceived()), baseValues_->mainWindow_,
                SLOT(anyDataReceived()));
        connect(julyHttp, SIGNAL(apiDown(bool)), baseValues_->mainWindow_, SLOT(setApiDown(bool)));
        connect(julyHttp, SIGNAL(setDataPending(bool)), baseValues_->mainWindow_,
                SLOT(setDataPending(bool)));
        connect(julyHttp, SIGNAL(errorSignal(QString)), baseValues_->mainWindow_,
                SLOT(showErrorMessage(QString)));
        connect(julyHttp, SIGNAL(sslErrorSignal(const QList<QSslError> &)), this,
                SLOT(sslErrors(const QList<QSslError> &)));
        connect(julyHttp, SIGNAL(dataReceived(QByteArray, int)), this,
                SLOT(dataReceivedAuth(QByteArray, int)));
    }

    if (auth) {
        QByteArray nonceStr = QByteArray::number((long long)time(nullptr));
        QByteArray postData = '{' + commands + '}';

        QByteArray appendHeaders = "Key: " + getApiKey() +
                                   "\r\n"
                                   "Nonce: " +
                                   nonceStr + "\r\n";
        nonceStr.prepend(method);
        nonceStr.append(getApiKey());
        appendHeaders += "Sign: " + ecdsaSha1(getApiSign(), nonceStr).toBase64() + "\r\n";

        if (sendNow)
            julyHttp->sendData(reqType, "POST /api2/" + method, postData, appendHeaders);
        else
            julyHttp->prepareData(reqType, "POST /ap2i/" + method, postData, appendHeaders);
    } else {
        if (commands.isEmpty()) {
            if (sendNow)
                julyHttp->sendData(reqType, "GET /api2/" + method);
            else
                julyHttp->prepareData(reqType, "GET /api2/" + method);
        } else {
            if (sendNow)
                julyHttp->sendData(reqType, "POST /api2/" + method, commands);
            else
                julyHttp->prepareData(reqType, "POST /api2/" + method, commands);
        }
    }
}

QByteArray StexExchange::ecdsaSha1(QByteArray shaKey, QByteArray &data) {
    EC_KEY *eckey = EC_KEY_new_by_curve_name(NID_secp256k1);
    EC_KEY_generate_key(eckey);
    BIGNUM *tempPrivateKey = BN_new();

    BN_bin2bn((unsigned char *)shaKey.data(), shaKey.length(), tempPrivateKey);
    EC_KEY_set_private_key(eckey, tempPrivateKey);

    QByteArray rezult;
    rezult.resize(ECDSA_size(eckey));
    quint32 len = rezult.size();
    ECDSA_sign(0, (unsigned char *)QCryptographicHash::hash(data, QCryptographicHash::Sha1).data(),
               20, (unsigned char *)rezult.data(), &len, eckey);

    BN_free(tempPrivateKey);
    return rezult;
}

bool StexExchange::isReplayPending(int reqType) {
    if (julyHttp == 0)
        return false;

    return julyHttp->isReqTypePending(reqType);
}

void StexExchange::depthUpdateOrder(QString symbol, double price, double amount, bool isAsk) {
    if (symbol != baseValues.currentPair.symbol)
        return;

    if (isAsk) {
        if (depthAsks == 0)
            return;

        DepthItem newItem;
        newItem.price = price;
        newItem.volume = amount;

        if (newItem.isValid())
            (*depthAsks) << newItem;
    } else {
        if (depthBids == 0)
            return;

        DepthItem newItem;
        newItem.price = price;
        newItem.volume = amount;

        if (newItem.isValid())
            (*depthBids) << newItem;
    }
}

void StexExchange::depthSubmitOrder(QString symbol, QMap<double, double> *currentMap,
                                      double priceDouble, double amount, bool isAsk) {
    if (symbol != baseValues.currentPair.symbol)
        return;

    if (priceDouble == 0.0 || amount == 0.0)
        return;

    if (isAsk) {
        (*currentMap)[priceDouble] = amount;

        if (lastDepthAsksMap.value(priceDouble, 0.0) != amount)
            depthUpdateOrder(symbol, priceDouble, amount, true);
    } else {
        (*currentMap)[priceDouble] = amount;

        if (lastDepthBidsMap.value(priceDouble, 0.0) != amount)
            depthUpdateOrder(symbol, priceDouble, amount, false);
    }
}
