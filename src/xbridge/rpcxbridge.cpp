#include "json/json_spirit_reader_template.h"
#include "json/json_spirit_writer_template.h"
#include "json/json_spirit_utils.h"

#include <boost/asio.hpp>
#include <boost/filesystem.hpp>
#include <boost/iostreams/concepts.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/signals2.hpp>
#include <boost/date_time/posix_time/ptime.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <stdio.h>
#include <atomic>

#include "util/settings.h"
#include "util/logger.h"
#include "util/xbridgeerror.h"
#include "xbridgeapp.h"
#include "xbridgeexchange.h"
#include "xbridgetransaction.h"
#include "rpcserver.h"

using namespace json_spirit;
using namespace std;
using namespace boost;
using namespace boost::asio;


Value xBridgeValueFromAmount(uint64_t amount)
{
    return static_cast<double>(amount / XBridgeTransactionDescr::COIN);
}

uint64_t xBridgeAmountFromReal(double val)
{
    // TODO: should we check amount ranges and throw JSONRPCError like they do in rpcserver.cpp ?
    return static_cast<uint64_t>(val * XBridgeTransactionDescr::COIN + 0.5);
}

/** \brief Returns the list of open and pending transactions
  * \param params A list of input params.
  * \param fHelp For debug purposes, throw the exception describing parameters.
  * \return A list of open(they go first) and pending transactions.
  *
  * Returns the list of open and pending transactions as JSON structures.
  * The open transactions go first.
  */

Value dxGetTransactionList(const Array & params, bool fHelp)
{
    if (fHelp || params.size() > 0)
    {
        throw runtime_error("dxGetTransactionList\nList transactions.");
    }

    Array arr;

    boost::mutex::scoped_lock l(XBridgeApp::m_txLocker);

    // pending tx
    {
        std::map<uint256, XBridgeTransactionDescrPtr> trlist = XBridgeApp::m_pendingTransactions;
        for (const auto & trEntry : trlist)
        {
            Object jtr;
            const auto tr = trEntry.second;
//            tr->state
            jtr.push_back(Pair("id", tr->id.GetHex()));
            jtr.push_back(Pair("from", tr->fromCurrency));
            jtr.push_back(Pair("from address", tr->from));
            jtr.push_back(Pair("fromAmount", xBridgeValueFromAmount(tr->fromAmount)));
            jtr.push_back(Pair("to", tr->toCurrency));
            jtr.push_back(Pair("to address", tr->to));
            jtr.push_back(Pair("toAmount", xBridgeValueFromAmount(tr->toAmount)));
            jtr.push_back(Pair("state", tr->strState()));
            arr.push_back(jtr);
        }
    }

    // active tx
    {
        std::map<uint256, XBridgeTransactionDescrPtr> trlist = XBridgeApp::m_transactions;
        for (const auto & trEntry : trlist)
        {
            Object jtr;
            const auto tr = trEntry.second;
            jtr.push_back(Pair("id", tr->id.GetHex()));
            jtr.push_back(Pair("from", tr->fromCurrency));
            jtr.push_back(Pair("from address", tr->from));
            jtr.push_back(Pair("fromAmount", xBridgeValueFromAmount(tr->fromAmount)));
            jtr.push_back(Pair("to", tr->toCurrency));
            jtr.push_back(Pair("to address", tr->to));
            jtr.push_back(Pair("toAmount", xBridgeValueFromAmount(tr->toAmount)));
            jtr.push_back(Pair("state", tr->strState()));
            arr.push_back(jtr);
        }
    }
    return arr;
}

//*****************************************************************************
//*****************************************************************************

Value dxGetTransactionsHistoryList(const Array & params, bool fHelp)
{
    if (fHelp || params.size() > 0)
    {
        throw runtime_error("dxGetTransactionsHistoryList\nHistoric list transactions.");
    }
    Array arr;
    boost::mutex::scoped_lock l(XBridgeApp::m_txLocker);
    {
        std::map<uint256, XBridgeTransactionDescrPtr> trlist = XBridgeApp::m_historicTransactions;
        if(trlist.empty())
        {
            LOG() << "empty history transactions list ";
            return arr;
        }

        for (const auto &trEntry : trlist)
        {
            Object buy;            
            const auto tr = trEntry.second;
            double fromAmount = static_cast<double>(tr->fromAmount);
            double toAmount = static_cast<double>(tr->toAmount);
            double price = fromAmount / toAmount;
            std::string buyTime = to_simple_string(tr->created);
            buy.push_back(Pair("time", buyTime));
            buy.push_back(Pair("traid_id", tr->id.GetHex()));
            buy.push_back(Pair("price", price));
            buy.push_back(Pair("size", tr->toAmount));
            buy.push_back(Pair("side", "buy"));
            arr.push_back(buy);
        }
    }
    return arr;
}

//*****************************************************************************
//*****************************************************************************

Value dxGetTransactionInfo(const Array & params, bool fHelp)
{
    if (fHelp || params.size() != 1)
    {
        throw runtime_error("dxGetTransactionInfo (id)\nTransaction info.");
    }

    std::string id = params[0].get_str();

    Array arr;

    boost::mutex::scoped_lock l(XBridgeApp::m_txLocker);

    // pending tx
    {
        std::map<uint256, XBridgeTransactionDescrPtr> trlist = XBridgeApp::m_pendingTransactions;
        for (const auto & trEntry : trlist)
        {
            const auto tr = trEntry.second;
            if(id != tr->id.GetHex())
            {
                continue;
            }
            Object jtr;
            jtr.push_back(Pair("id", tr->id.GetHex()));
            jtr.push_back(Pair("from", tr->fromCurrency));
            jtr.push_back(Pair("from address", tr->from));
            jtr.push_back(Pair("fromAmount", xBridgeValueFromAmount(tr->fromAmount)));
            jtr.push_back(Pair("to", tr->toCurrency));
            jtr.push_back(Pair("to address", tr->to));
            jtr.push_back(Pair("toAmount", xBridgeValueFromAmount(tr->toAmount)));
            jtr.push_back(Pair("state", tr->strState()));
            arr.push_back(jtr);
        }
    }

    // active tx
    {
        std::map<uint256, XBridgeTransactionDescrPtr> trlist = XBridgeApp::m_transactions;
        for (const auto & trEntry : trlist)
        {
            const auto tr = trEntry.second;

            if(id != tr->id.GetHex())
                continue;

            Object jtr;
            jtr.push_back(Pair("id", tr->id.GetHex()));
            jtr.push_back(Pair("from", tr->fromCurrency));
            jtr.push_back(Pair("from address", tr->from));
            jtr.push_back(Pair("fromAmount", xBridgeValueFromAmount(tr->fromAmount)));
            jtr.push_back(Pair("to", tr->toCurrency));
            jtr.push_back(Pair("to address", tr->to));
            jtr.push_back(Pair("toAmount", xBridgeValueFromAmount(tr->toAmount)));
            jtr.push_back(Pair("state", tr->strState()));
            arr.push_back(jtr);
        }
    }

    // historic tx
    {
        std::map<uint256, XBridgeTransactionDescrPtr> trlist = XBridgeApp::m_historicTransactions;
        if(trlist.empty())
        {
            LOG() << "history transaction list empty " << __FUNCTION__;
        }
        for (const auto & trEntry : trlist)
        {
            const auto tr = trEntry.second;

            if(id != tr->id.GetHex())
                continue;

            Object jtr;
            jtr.push_back(Pair("id", tr->id.GetHex()));
            jtr.push_back(Pair("from", tr->fromCurrency));
            jtr.push_back(Pair("from address", tr->from));
            jtr.push_back(Pair("fromAmount", xBridgeValueFromAmount(tr->fromAmount)));
            jtr.push_back(Pair("to", tr->toCurrency));
            jtr.push_back(Pair("to address", tr->to));
            jtr.push_back(Pair("toAmount", xBridgeValueFromAmount(tr->toAmount)));
            jtr.push_back(Pair("state", tr->strState()));
            arr.push_back(jtr);
        }
    }
    return arr;
}


//******************************************************************************
//******************************************************************************
Value dxGetCurrencyList(const Array & params, bool fHelp)
{
    if (fHelp || params.size() > 0)
    {
        throw runtime_error("dxGetCurrencyList\nList currencies.");
    }

    Object obj;

    std::vector<std::string> currencies = XBridgeApp::instance().sessionsCurrencies();
    for (std::string currency : currencies)
    {
        obj.push_back(Pair(currency, ""));
    }

    return obj;
}

//******************************************************************************
//******************************************************************************
Value dxCreateTransaction(const Array & params, bool fHelp)
{
    if (fHelp || params.size() != 6)
    {
        throw runtime_error("dxCreateTransaction "
                            "(address from) (currency from) (amount from) "
                            "(address to) (currency to) (amount to)\n"
                            "Create xbridge transaction.");
    }

    std::string from            = params[0].get_str();
    std::string fromCurrency    = params[1].get_str();
    double      fromAmount      = params[2].get_real();
    std::string to              = params[3].get_str();
    std::string toCurrency      = params[4].get_str();
    double      toAmount        = params[5].get_real();

    if ((from.size() < 32 && from.size() > 36) ||
            (to.size() < 32 && to.size() > 36))
    {
        throw runtime_error("incorrect address");
    }

    uint256 id = uint256();
    const auto res = XBridgeApp::instance().sendXBridgeTransaction
          (from, fromCurrency, xBridgeAmountFromReal(fromAmount),
           to, toCurrency, xBridgeAmountFromReal(toAmount), id);

    if(res == xbridge::NO_ERROR)
    {
        Object obj;
        obj.push_back(Pair("from", from));
        obj.push_back(Pair("from currency", fromCurrency));
        obj.push_back(Pair("from amount", fromAmount));
        obj.push_back(Pair("to", to));
        obj.push_back(Pair("to currency", toCurrency));
        obj.push_back(Pair("to amount", toAmount));
        return obj;
    }
    Object obj;
    obj.push_back(Pair("id", id.GetHex()));
    return obj;
}

//******************************************************************************
//******************************************************************************
Value dxAcceptTransaction(const Array & params, bool fHelp)
{
    if (fHelp || params.size() != 3)
    {
        throw runtime_error("dxAcceptTransaction (id) "
                            "(address from) (address to)\n"
                            "Accept xbridge transaction.");
    }

    uint256 id(params[0].get_str());
    std::string from    = params[1].get_str();
    std::string to      = params[2].get_str();

    if ((from.size() != 33 && from.size() != 34) ||
            (to.size() != 33 && to.size() != 34))
    {
        throw runtime_error("incorrect address");
    }


    uint256 idResult;
    const auto error= XBridgeApp::instance().acceptXBridgeTransaction(id, from, to, idResult);
    if(error == xbridge::NO_ERROR)
    {
        Object obj;
        obj.push_back(Pair("id", id.GetHex()));
        obj.push_back(Pair("from", from));
        obj.push_back(Pair("to", to));
        return obj;
    }

    Object obj;
    obj.push_back(Pair("id", idResult.GetHex()));
    return obj;
}

//******************************************************************************
//******************************************************************************
Value dxCancelTransaction(const Array & params, bool fHelp)
{
    if (fHelp || params.size() != 1)
    {
        throw runtime_error("dxCancelTransaction (id)\n"
                            "Cancel xbridge transaction.");
    }
    LOG() << "rpc cancel transaction " << __FUNCTION__;
    uint256 id(params[0].get_str());
    if(XBridgeApp::instance().cancelXBridgeTransaction(id, crRpcRequest) == xbridge::NO_ERROR)
    {
        Object obj;
        obj.push_back(Pair("id",id.GetHex()));
        return  obj;
    }
    Object obj;
    obj.push_back(Pair("id", id.GetHex()));
    return obj;
}
