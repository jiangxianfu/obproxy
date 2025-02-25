/**
 * Copyright (c) 2021 OceanBase
 * OceanBase Database Proxy(ODP) is licensed under Mulan PubL v2.
 * You can use this software according to the terms and conditions of the Mulan PubL v2.
 * You may obtain a copy of Mulan PubL v2 at:
 *          http://license.coscl.org.cn/MulanPubL-2.0
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PubL v2 for more details.
 */

#define USING_LOG_PREFIX PROXY
#include "obutils/ob_connection_diagnosis_trace.h"
#include "lib/utility/ob_print_utils.h"
#include "lib/oblog/ob_log_module.h"
#include "proxy/mysqllib/ob_proxy_parser_utils.h"
#include "iocore/eventsystem/ob_vconnection.h"
#include "lib/objectpool/ob_concurrency_objpool.h"

namespace oceanbase
{
namespace obproxy
{
namespace obutils
{

using namespace oceanbase::obproxy::proxy;
using namespace oceanbase::common;
using namespace oceanbase::obproxy::obutils;

static const char *get_vc_disconnect_initiator_str(const ObVCDisconnectInitiator initiator)
{
  const char *str = NULL;
  switch(initiator) {
    case OB_CLIENT:
      str = "CLIENT";
      break;
    case OB_SERVER:
      str = "SERVER";
      break;
    case OB_PROXY:
      str = "PROXY";
      break;
    case OB_MAX_VC_INITIATOR:
      str = "MAX_VC_INITIATOR";
      break;
    case OB_UNKNOWN:
    default:
      str = "OB_UNKOWN";
      break;
  }
  return str;
}

static const char *get_inactivity_timeout_event_str(const ObInactivityTimeoutEvent event)
{
  const char *str = NULL;
  switch(event) {
    case OB_CLIENT_DELETE_CLUSTER_RESOURCE:
      str = "CLIENT_DELETE_CLUSTER_RESOURCE";
      break;
    case OB_CLIENT_INTERNAL_CMD_TIMEOUT:
      str = "CLIENT_INTERNAL_CMD_TIMEOUT";
      break;
    case OB_CLIENT_CONNECT_TIMEOUT:
      str = "CLIENT_CONNECT_TIMEOUT";
      break;
    case OB_CLIENT_NET_READ_TIMEOUT:
      str = "CLIENT_NET_READ_TIMEOUT";
      break;
    case OB_CLIENT_EXECEUTE_PLAN_TIMEOUT:
      str = "CLIENT_EXECUTE_PLAN_TIMEOUT";
      break;
    case OB_CLIENT_WAIT_TIMEOUT:
      str = "CLIENT_WAIT_TIMEOUT";
      break;
    case OB_CLIENT_NET_WRITE_TIMEOUT:
      str = "CLIENT_NET_WRITE_TIMEOUT";
      break;
    case OB_SERVER_QUERY_TIMEOUT:
      str = "SERVER_QUERY_TIMEOUT";
      break;
    case OB_SERVER_TRX_TIMEOUT:
      str = "SERVER_TRX_TIMEOUT";
      break;
    case OB_SERVER_WAIT_TIMEOUT:
      str = "SERVER_WAIT_TIMEOUT";
      break;
    case OB_MAX_TIMEOUT_EVENT:
      str = "MAX_TIMEOUT_EVENT";
      break;
    case OB_TIMEOUT_UNKNOWN_EVENT:
    default:
      str = "OB_TIMEOUT_UNKNOWN_EVENT";
      break;
  }
  return str;
}

static const char *get_connection_diagnosis_trace_str(const ObConnectionDiagnosisTraceType trace)
{
  const char *str = NULL;
  switch(trace) {
    case OB_UNKNOWN_TRACE:
      str = "UNKOWN_TRACE";
      break;
    case OB_LOGIN_DISCONNECT_TRACE:
      str = "LOGIN_TRACE";
      break;
    case OB_PROXY_INTERNAL_TRACE:
      str = "PROXY_INTERNAL_TRACE";
      break;
    case OB_CLIENT_VC_TRACE:
      str = "CLIENT_VC_TRACE";
      break;
    case OB_SERVER_VC_TRACE:
      str = "SERVER_VC_TRACE";
      break;
    case OB_INTERNAL_REQUEST_VC_TRACE:
      str = "INTERNAL_REQUEST_VC_TRACE";
      break;
    case OB_VC_DISCONNECT_TRACE:
      str = "VC_TRACE";
      break;
    case OB_TIMEOUT_DISCONNECT_TRACE:
      str = "TIMEOUT_TRACE";
      break;
    case OB_MAX_TRACE_TYPE:
      str = "MAX_TRACE_TYPE";
      break;
    default:
      str = "UNKNOWN";
      break;
  }
  return str;
}

DEF_TO_STRING(ObConnectionDiagnosisInfo)
{
  int64_t pos = 0;
  J_OBJ_START();
  J_KV(K_(cs_id), K_(ss_id), K_(proxy_session_id), K_(server_session_id), K_(client_addr),
       K_(server_addr), K_(cluster_name), K_(tenant_name), K_(user_name), K_(error_code), K_(error_msg));
  J_OBJ_END();
  return pos;
}

DEF_TO_STRING(ObVCDiagnosisInfo)
{
  int64_t pos = 0;
  pos = ObConnectionDiagnosisInfo::to_string(buf, buf_len);
  J_OBJ_START();
  const char *vc_event = event::get_vc_event_name(vc_event_);
  const char *initiator = get_vc_disconnect_initiator_str(disconnect_initiator_);
  J_KV(K(vc_event), K(initiator));
  J_OBJ_END();
  return pos;
}

void ObVCDiagnosisInfo::reset() {
  ObConnectionDiagnosisInfo::reset();
  vc_event_ = 0;
  disconnect_initiator_ = OB_UNKNOWN;
}

DEF_TO_STRING(ObInactivityTimeoutDiagnosisInfo)
{
  int64_t pos = 0;
  pos = ObConnectionDiagnosisInfo::to_string(buf, buf_len);
  J_OBJ_START();
  const char *timeout_event = get_inactivity_timeout_event_str(inactivity_timeout_event_);
  J_KV(K_(timeout), K(timeout_event));
  J_OBJ_END();
  return pos;
}

void ObInactivityTimeoutDiagnosisInfo::reset()
{
  ObConnectionDiagnosisInfo::reset();
  inactivity_timeout_event_ = OB_TIMEOUT_UNKNOWN_EVENT;
  timeout_ = 0;
}

DEF_TO_STRING(ObProxyInternalDiagnosisInfo)
{
  int64_t pos = 0;
  pos = ObConnectionDiagnosisInfo::to_string(buf, buf_len);
  J_OBJ_START();
  const char *request_cmd = ObProxyParserUtils::get_sql_cmd_name(request_cmd_);
  const char *sql_cmd = ObProxyParserUtils::get_sql_cmd_name(sql_cmd_);
  J_KV(K_(sql), K(request_cmd), K(sql_cmd));
  J_OBJ_END();
  return pos;
}

void ObProxyInternalDiagnosisInfo::reset()
{
  ObConnectionDiagnosisInfo::reset();
  sql_.reset();
  sql_cmd_ = obmysql::OB_MYSQL_COM_MAX_NUM;
  request_cmd_ = obmysql::OB_MYSQL_COM_MAX_NUM;
}

DEF_TO_STRING(ObLoginDiagnosisInfo)
{
  int64_t pos = 0;
  pos = ObConnectionDiagnosisInfo::to_string(buf, buf_len);
  J_OBJ_START();
  J_KV(K_(internal_sql));
  J_OBJ_END();
  return pos;
}

void ObLoginDiagnosisInfo::reset() {
  ObConnectionDiagnosisInfo::reset();
  MEMSET(internal_sql_, '\0', OB_SHORT_SQL_LENGTH + 1);
}

void ObConnectionDiagnosisTrace::reset()
{
  if (OB_NOT_NULL(diagnosis_info_)) {
    diagnosis_info_->destroy();
    diagnosis_info_ = NULL;
  }
  trace_type_ = OB_UNKNOWN_TRACE;
  is_user_client_ = true;
  is_first_packet_received_ = false;
  is_com_quit_ = false;
}

int ObConnectionDiagnosisTrace::record_vc_disconnection(int vc_event, ObVCDisconnectInitiator initiator, int error_code, ObString error_msg)
{
  int ret = OB_SUCCESS;
  if (OB_NOT_NULL(diagnosis_info_)) {
    diagnosis_info_->destroy();
    diagnosis_info_ = NULL;
  }
  ObVCDiagnosisInfo *vc_diagnosis_info = NULL;
  if (OB_FAIL(create_diagnosis_info(vc_diagnosis_info))) {
    LOG_WARN("fail to allocate mem for vc diagnosis info", K(ret));
  } else {
    vc_diagnosis_info->error_code_ = error_code;
    if (error_msg.empty()) {
      error_msg = ob_strerror(error_code);
    }
    MEMCPY(vc_diagnosis_info->error_msg_, error_msg.ptr(), std::min(error_msg.length(), MAX_MSG_BUF_LEN));
    vc_diagnosis_info->vc_event_ = vc_event;
    vc_diagnosis_info->disconnect_initiator_ = initiator;
    diagnosis_info_ = vc_diagnosis_info;
  }
  return ret;
}

int ObConnectionDiagnosisTrace::record_inactivity_timeout_disconnection(ObInactivityTimeoutEvent event, ObHRTime timeout, int error_code, ObString error_msg)
{
  int ret = OB_SUCCESS;
  if (OB_NOT_NULL(diagnosis_info_)) {
    diagnosis_info_->destroy();
    diagnosis_info_ = NULL;
  }
  ObInactivityTimeoutDiagnosisInfo *timeout_diagnosis_info = NULL;
  if (OB_FAIL(create_diagnosis_info(timeout_diagnosis_info))) {
    LOG_WARN("fail to allocate mem for inactivity timeout diagnosis info", K(ret));
  } else {
    timeout_diagnosis_info->error_code_ = error_code;
    if (error_msg.empty()) {
      error_msg = ob_strerror(error_code);
    }
    MEMCPY(timeout_diagnosis_info->error_msg_, error_msg.ptr(), std::min(error_msg.length(), MAX_MSG_BUF_LEN));
    timeout_diagnosis_info->timeout_ = timeout;
    timeout_diagnosis_info->inactivity_timeout_event_ = event;
    diagnosis_info_ = timeout_diagnosis_info;
  }
  return ret;
}

int ObConnectionDiagnosisTrace::record_obproxy_internal_disconnection(const ObString &sql, obmysql::ObMySQLCmd sql_cmd, obmysql::ObMySQLCmd request_cmd, int error_code, ObString error_msg)
{
  int ret = OB_SUCCESS;
  if (OB_NOT_NULL(diagnosis_info_)) {
    diagnosis_info_->destroy();
    diagnosis_info_ = NULL;
  }
  ObProxyInternalDiagnosisInfo *internal_diagnosis_info = NULL;
  if (OB_FAIL(create_diagnosis_info(internal_diagnosis_info))) {
    LOG_WARN("fail to allocate mem for proxy internal diagnosis info", K(ret));
  } else {
    internal_diagnosis_info->error_code_ = error_code;
    if (error_msg.empty()) {
      error_msg = ob_strerror(error_code);
    }
    MEMCPY(internal_diagnosis_info->error_msg_, error_msg.ptr(), std::min(error_msg.length(), MAX_MSG_BUF_LEN));
    internal_diagnosis_info->sql_ = sql;
    internal_diagnosis_info->sql_cmd_ = sql_cmd;
    internal_diagnosis_info->request_cmd_ = request_cmd;
    diagnosis_info_ = internal_diagnosis_info;
  }
  return ret;
}

int ObConnectionDiagnosisTrace::record_login_disconnection(const ObString &internal_sql, int error_code, ObString error_msg)
{
  int ret = OB_SUCCESS;
  if (OB_NOT_NULL(diagnosis_info_)) {
    diagnosis_info_->destroy();
    diagnosis_info_ = NULL;
  }
  ObLoginDiagnosisInfo *login_info = NULL;
  if (OB_FAIL(create_diagnosis_info(login_info))) {
    LOG_WARN("fail to allocate mem for proxy internal diagnosis info", K(ret));
  } else {
    login_info->error_code_ = error_code;
    if (error_msg.empty()) {
      error_msg = ob_strerror(error_code);
    }
    MEMCPY(login_info->error_msg_, error_msg.ptr(), std::min(error_msg.length(), MAX_MSG_BUF_LEN));
    MEMCPY(login_info->internal_sql_, internal_sql.ptr(), std::min(static_cast<int>(OB_SHORT_SQL_LENGTH), internal_sql.length()));
    diagnosis_info_ = login_info;
  }
  return ret;
}

bool ObConnectionDiagnosisTrace::need_record_diagnosis_log(ObConnectionDiagnosisTraceType trace_type) const
{
  bool bret = true;
  if (!is_user_client_
      || (!is_first_packet_received_ && trace_type == OB_CLIENT_VC_TRACE)
      || is_com_quit_) {
    bret = false;
  }
  return bret;
}

void ObConnectionDiagnosisTrace::log_dianosis_info() const
{
  const char *trace_type = get_connection_diagnosis_trace_str(trace_type_);
  if (diagnosis_info_ != NULL) {
    OBPROXY_DIAGNOSIS_LOG(WARN, "[CONNECTION]", K(trace_type), "connection_diagnosis", *diagnosis_info_);
  }
}

void ObConnectionDiagnosisTrace::free()
{
  reset();
  op_free(this);
}

}
}
}
