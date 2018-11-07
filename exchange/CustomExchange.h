//
// Created by kegar on 9/3/18.
//

#ifndef QTBICOINTRADER_CUSTOMEXCHANGE_H
#define QTBICOINTRADER_CUSTOMEXCHANGE_H

#include "exchange.h"
#include <QJsonArray>

class CustomExchange : public Exchange {
    Q_OBJECT
public:

    CustomExchange(QByteArray pRestSign, QByteArray pRestKey);
    ~CustomExchange() override;


    void clearVariables() override;

    void secondSlot() override;

    void dataReceivedAuth(QByteArray array, int i) override;

    void reloadDepth() override;

    void clearValues() override;

    void getHistory(bool b) override;

    void buy(QString string, double d, double d1) override;

    void sell(QString string, double d, double d1) override;

    void cancelOrder(QString string, QByteArray array) override;

    void sslErrors(const QList<QSslError>& errors);

    void sendToApi(int reqType, QByteArray method, bool auth, bool sendNow, QByteArray commands = "");

private:
    JulyHttp* julyHttp=  nullptr;

    QByteArray ecdsaSha1(QByteArray shaKey, QByteArray& data);
    bool isReplayPending(int reqType);
    void depthUpdateOrder(QString symbol, double price, double amount, bool isAsk);
    void depthSubmitOrder(QString symbol, QMap<double, double>* currentMap, double priceDouble,
                          double amount, bool isAsk);
    QList<DepthItem> *depthAsks, *depthBids;
    QMap<double,double> lastDepthAsksMap;
    QMap<double,double> lastDepthBidsMap;
    QList<TradesItem> m_tradesCache;

};


#endif //QTBICOINTRADER_CUSTOMEXCHANGE_H
