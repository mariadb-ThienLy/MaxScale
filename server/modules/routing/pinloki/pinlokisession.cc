/*
 * Copyright (c) 2018 MariaDB Corporation Ab
 *
 * Use of this software is governed by the Business Source License included
 * in the LICENSE.TXT file and at www.mariadb.com/bsl11.
 *
 * Change Date: 2023-11-05
 *
 * On the date above, in accordance with the Business Source License, use
 * of this software will be governed by version 2 or later of the General
 * Public License.
 */

#include "pinlokisession.hh"

#include <maxscale/modutil.hh>
#include <maxscale//protocol/mariadb/resultset.hh>
#include <maxscale/protocol/mariadb/mysql.hh>

namespace
{
GWBUF* create_resultset(const std::initializer_list<std::string>& columns,
                        const std::initializer_list<std::string>& row)
{
    auto rset = ResultSet::create(columns);
    rset->add_row(row);
    return rset->as_buffer().release();
}
}

namespace pinloki
{
PinlokiSession::PinlokiSession(MXS_SESSION* pSession)
    : mxs::RouterSession(pSession)
{
}

void PinlokiSession::close()
{
}

int32_t PinlokiSession::routeQuery(GWBUF* pPacket)
{
    int rval = 0;
    GWBUF* response = nullptr;
    mxs::Buffer buf(pPacket);
    auto cmd = mxs_mysql_get_command(buf.get());

    switch (cmd)
    {
    case MXS_COM_REGISTER_SLAVE:
        // Register slave (maybe grab the slave's server_id if we need it)
        MXS_INFO("COM_REGISTER_SLAVE");
        response = modutil_create_ok();
        break;

    case MXS_COM_BINLOG_DUMP:
        // Start dumping binlogs (not yet implemented)
        MXS_INFO("COM_BINLOG_DUMP");
        rval = 1;
        break;

    case MXS_COM_QUERY:
        {
            auto sql = mxs::extract_sql(buf.get());
            MXS_INFO("COM_QUERY: %s", sql.c_str());
            parser::parse(sql, this);
        }
        break;
    }

    if (response)
    {
        const mxs::ReplyRoute down;
        const mxs::Reply reply;
        mxs::RouterSession::clientReply(response, down, reply);
        rval = 1;
    }
    else
    {
        mxb_assert(rval == 1);
    }

    return rval;
}

void PinlokiSession::clientReply(GWBUF* pPacket, const mxs::ReplyRoute& down, const mxs::Reply& reply)
{
    mxb_assert_message(!true, "This should not happen");
}

bool PinlokiSession::handleError(mxs::ErrorType type, GWBUF* pMessage,
                                 mxs::Endpoint* pProblem, const mxs::Reply& pReply)
{
    mxb_assert_message(!true, "This should not happen");
    return false;
}

bool PinlokiSession::send_event(const maxsql::RplEvent& event)
{
    mxs::Buffer buffer(5 + event.data().size());

    // Wrap the events in a protocol packet with a command byte of 0x0
    mariadb::set_byte3(buffer.data(), event.data().size() + 1);
    buffer.data()[3] = m_seq++;
    buffer.data()[4] = 0x0;
    mempcpy(buffer.data() + 5, event.data().data(), event.data().size());

    send(buffer.release());

    // TODO: Stop sending events when the network buffer gets full
    return true;
}

void PinlokiSession::send(GWBUF* buffer)
{
    const mxs::ReplyRoute down;
    const mxs::Reply reply;
    mxs::RouterSession::clientReply(buffer, down, reply);
}

void PinlokiSession::select(const std::vector<std::string>& values)
{
    send(create_resultset({"1"}, {"1"}));
}

void PinlokiSession::set(const std::string& key, const std::string& value)
{
    send(modutil_create_ok());
}

void PinlokiSession::change_master_to(const parser::MasterConfig& config)
{
    send(modutil_create_ok());
}

void PinlokiSession::start_slave()
{
    send(modutil_create_ok());
}

void PinlokiSession::stop_slave()
{
    send(modutil_create_ok());
}

void PinlokiSession::reset_slave()
{
    send(modutil_create_ok());
}

void PinlokiSession::show_slave_status()
{
    send(create_resultset({"1"}, {"1"}));
}

void PinlokiSession::flush_logs()
{
    send(modutil_create_ok());
}

void PinlokiSession::purge_logs()
{
    send(modutil_create_ok());
}

void PinlokiSession::error(const std::string& err)
{
    send(modutil_create_mysql_err_msg(1, 0, 1064, "42000", err.c_str()));
}
}