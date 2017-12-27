//*****************************************************************************
//*****************************************************************************

#ifndef XBRIDGEAPP_H
#define XBRIDGEAPP_H

#include "xbridge.h"
#include "xbridgesession.h"
#include "xbridgepacket.h"
#include "uint256.h"
#include "xbridgetransactiondescr.h"
#include "util/xbridgeerror.h"
#include <thread>
#include <atomic>
#include <vector>
#include <map>
#include <tuple>
#include <set>
#include <queue>

#ifdef WIN32
// #include <Ws2tcpip.h>
#endif

namespace rpc
{
class AcceptedConnection;
}

//*****************************************************************************
//*****************************************************************************
class XBridgeApp
{
    typedef std::vector<unsigned char> UcharVector;
    typedef std::shared_ptr<boost::asio::io_service>       IoServicePtr;
    typedef std::shared_ptr<boost::asio::io_service::work> WorkPtr;

    friend void callback(void * closure, int event,
                         const unsigned char * info_hash,
                         const void * data, size_t data_len);

private:
    XBridgeApp();
    virtual ~XBridgeApp();

public:
    static XBridgeApp &instance();

    static std::string version();

    static bool isEnabled();

    bool init(int argc, char *argv[]);
    bool start();

    xbridge::Error sendXBridgeTransaction(const std::string &from,
                                          const std::string &fromCurrency,
                                          const uint64_t &fromAmount,
                                          const std::string &to,
                                          const std::string &toCurrency,
                                          const uint64_t &toAmount,
                                          uint256 &id);

    bool sendPendingTransaction(XBridgeTransactionDescrPtr &ptr);

    xbridge::Error acceptXBridgeTransaction(const uint256 &id,
                                     const std::string &from,
                                     const std::string &to, uint256 &result);
    bool sendAcceptingTransaction(XBridgeTransactionDescrPtr &ptr);

    xbridge::Error cancelXBridgeTransaction(const uint256 &id, const TxCancelReason &reason);
    bool sendCancelTransaction(const uint256 &txid, const TxCancelReason &reason);

    xbridge::Error rollbackXBridgeTransaction(const uint256 &id);
    bool sendRollbackTransaction(const uint256 &txid);

    /**
     * @brief isValidAddress checks the correctness of the address
     * @param address checked address
     * @return true, if address valid
     */
    bool isValidAddress(const std::string &address) const;

    /**
     * @brief checkAcceptParams checks the correctness of the parameters
     * @param id - id accepted transaction
     * @param ptr - smart pointer to accepted transaction
     * @return xbridge::SUCCESS, if all parameters valid
     */
    xbridge::Error checkAcceptParams(const uint256 &id, XBridgeTransactionDescrPtr &ptr);

    /**
     * @brief checkCreateParams - checks parameter needs to success created transaction
     * @param fromCurrency - from currency
     * @param toCurrency - to currency
     * @param fromAmount -  amount
     * @return xbridge::SUCCES, if all parameters valid
     */
    xbridge::Error checkCreateParams(const std::string &fromCurrency, const std::string &toCurrency, const uint64_t &fromAmount);

    /**
     * @brief checkAmount - checks wallet balance
     * @param currency - currency name
     * @param amount - amount
     * @return xbridge::SUCCES, if  the session currency is open and
     * on account has sufficient funds for operations
     */
    xbridge::Error checkAmount(const std::string &currency, const uint64_t &amount);
public:
    bool stop();

    XBridgeSessionPtr sessionByCurrency(const std::string & currency) const;
    std::vector<std::string> sessionsCurrencies() const;

    // store session
    void addSession(XBridgeSessionPtr session);
    // store session addresses in local table
    void storageStore(XBridgeSessionPtr session, const std::vector<unsigned char> & id);

    bool isLocalAddress(const std::vector<unsigned char> & id);
    bool isKnownMessage(const std::vector<unsigned char> & message);
    void addToKnown(const std::vector<unsigned char> & message);

    XBridgeSessionPtr serviceSession();

    void storeAddressBookEntry(const std::string & currency,
                               const std::string & name,
                               const std::string & address);
    void getAddressBook();


    /**
     * @brief isHistoricState - checks the state of the transaction
     * @param state - current state of transaction
     * @return true, if the transaction is historical
     */
    bool isHistoricState(const XBridgeTransactionDescr::State state);
public:// slots:
    // send messave via xbridge
    void onSend(const XBridgePacketPtr & packet);
    void onSend(const UcharVector & id, const XBridgePacketPtr & packet);

    // call when message from xbridge network received
    void onMessageReceived(const std::vector<unsigned char> & id, const std::vector<unsigned char> & message);
    // broadcast message
    void onBroadcastReceived(const std::vector<unsigned char> & message);

private:
    void onSend(const UcharVector & id, const UcharVector & message);

public:
    static void sleep(const unsigned int umilliseconds);

private:
    boost::thread_group m_threads;

    XBridgePtr        m_bridge;

    mutable boost::mutex m_sessionsLock;
    typedef std::map<std::vector<unsigned char>, XBridgeSessionPtr> SessionAddrMap;
    SessionAddrMap m_sessionAddrs;
    typedef std::map<std::string, XBridgeSessionPtr> SessionIdMap;
    SessionIdMap m_sessionIds;
    typedef std::queue<XBridgeSessionPtr> SessionQueue;
    SessionQueue m_sessionQueue;

    // service session
    XBridgeSessionPtr m_serviceSession;

    boost::mutex m_messagesLock;
    typedef std::set<uint256> ProcessedMessages;
    ProcessedMessages m_processedMessages;

    boost::mutex m_addressBookLock;
    typedef std::tuple<std::string, std::string, std::string> AddressBookEntry;
    typedef std::vector<AddressBookEntry> AddressBook;
    AddressBook m_addressBook;
    std::set<std::string> m_addresses;

public:
    static boost::mutex                                  m_txLocker;
    static std::map<uint256, XBridgeTransactionDescrPtr> m_pendingTransactions;
    static std::map<uint256, XBridgeTransactionDescrPtr> m_transactions;
    static std::map<uint256, XBridgeTransactionDescrPtr> m_historicTransactions;

    static boost::mutex                                  m_txUnconfirmedLocker;
    static std::map<uint256, XBridgeTransactionDescrPtr> m_unconfirmed;

    static boost::mutex                                  m_ppLocker;
    static std::map<uint256, std::pair<std::string, XBridgePacketPtr> > m_pendingPackets;

  private:
    /**
     * @brief m_historicTransactionsStates - transaction state, in the historical list
     */
    std::list<XBridgeTransactionDescr::State>       m_historicTransactionsStates;

};

#endif // XBRIDGEAPP_H
