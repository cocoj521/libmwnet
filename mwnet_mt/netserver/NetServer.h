#ifndef MWNET_MT_NETSERVER_H
#define MWNET_MT_NETSERVER_H

#include <stdint.h>
#include <stdio.h>
#include <boost/any.hpp>
#include <string>
#include <memory>

namespace MWNET_MT
{
namespace SERVER
{
//content-type
enum HTTP_CONTENT_TYPE {CONTENT_TYPE_XML=1, CONTENT_TYPE_JSON, CONTENT_TYPE_URLENCODE, CONTENT_TYPE_UNKNOWN};
//POST/GET ...
enum HTTP_REQUEST_TYPE {HTTP_REQUEST_POST=1, HTTP_REQUEST_GET, HTTP_REQUEST_UNKNOWN};

class NetServerCtrlInfo;

typedef std::shared_ptr<NetServerCtrlInfo> NetServerCtrlInfoPtr;
class HttpRequest
{
public:
	HttpRequest(const std::string& strReqIp, uint16_t uReqPort);
	~HttpRequest();
public:
	//�Ƿ���Ҫ��������,��connection��keep-alive����close
	//����ֵ:true:keep-alive false:close
	bool	IsKeepAlive() const;

	//��������http����ı��ĳ���(����ͷ+������)
	size_t  GetHttpRequestTotalLen() const;

	//��ȡhttp body������
	//����ֵ:body�����ݺͳ���
	size_t	GetBodyData(std::string& strBody) const;

	//��ȡ����url
	//����ֵ:url�������Լ�����
	//˵��:��:post����,POST /sms/std/single_send HTTP/1.1 ,/sms/std/single_send����URL
	//	   ��:get����,
	size_t	GetUrl(std::string& strUrl) const;

	//��ȡ����ͻ��˵�IP
	//����ֵ:����ͻ��˵�IP
	std::string GetClientRequestIp() const;

	//��ȡ����ͻ��˵Ķ˿�
	//����ֵ:����ͻ��˵Ķ˿�
	uint16_t GetClientRequestPort() const;

	//��ȡUser-Agent
	//����ֵ:User-Agent�������Լ�����
	size_t	GetUserAgent(std::string& strUserAgent) const;

	//��ȡcontent-type
	//����ֵ:CONTENT_TYPE_XML ���� xml,CONTENT_TYPE_JSON ���� json,CONTENT_TYPE_URLENCODE ���� urlencode,CONTENT_TYPE_UNKNOWN ���� δ֪����
	//˵��:��ֻ�ṩ������
	int		GetContentType() const;
	std::string GetContentTypeStr();

	//��ȡhost
	//����ֵ:host�������Լ�����
	size_t	GetHost(std::string& strHost) const;

	//��ȡx-forwarded-for
	//����ֵ:X-Forwarded-For�������Լ�����
	size_t	GetXforwardedFor(std::string& strXforwardedFor) const;

	//��ȡhttp���������
	//����ֵ:HTTP_REQUEST_POST ���� post����; HTTP_REQUEST_GET ���� get���� ; HTTP_REQUEST_UNKNOWN ���� δ֪(������ͷ�д���)
	int		GetRequestType() const;

	//��ȡSOAPAction(������soapʱ������)
	//����ֵ:SOAPAction�����Լ�����
	size_t	GetSoapAction(std::string& strSoapAction) const;

	//��ȡWap Via
	//����ֵ:via�����Լ�����
	size_t	GetVia(std::string& strVia) const;

	//��ȡx-wap-profile
	//����ֵ:x-wap-profile�����Լ�����
	size_t	GetXWapProfile(std::string& strXWapProfile) const;

	//��ȡx-browser-type
	//����ֵ:x-browser-type�����Լ�����
	size_t	GetXBrowserType(std::string& strXBrowserType) const;

	//��ȡx-up-bear-type
	//����ֵ:x-up-bear-type�����Լ�����
	size_t	GetXUpBearType(std::string& strXUpBearType) const;

	//��ȡ����http���ĵ�����(ͷ+��)
	//����ֵ:http���ĵ�ָ��,�Լ�����
	size_t	GetHttpRequestContent(std::string& strHttpRequestContent) const;

	//��ȡhttpͷ����һ�����Ӧ��ֵ(�����ִ�Сд)
	//����ֵ:���Ӧ��ֵ
	size_t	GetHttpHeaderFieldValue(const char* field, std::string& strValue) const;
	
	//��ȡΨһ��http����ID
	//����ֵ:����ID
	uint64_t	GetHttpRequestId() const;

	//��ȡ�յ�����http�����ʱ��
	//����ֵ:����ʱ�� 
	uint64_t	GetHttpRequestTime() const;

//..........................�����ṩһ�����urlencode�ĺ���......................
	//����key,value��ʽ��body,��:cmd=aa&seqid=1&msg=11
	//ע��:����GetContentType����CONTENT_TYPE_URLENCODE�ſ���ʹ�øú���ȥ����
	bool	ParseUrlencodedBody(const char* pbody/*http body ��:cmd=aa&seqid=1&msg=11*/, size_t len/*http body�ĳ���*/) const;

	//ͨ��key��ȡvalue��ָ��ͳ���,�������忽��.������false����key������
	//ע��:�����ȵ���ParseUrlencodedBody��ſ���ʹ�øú���
	bool	GetUrlencodedBodyValue(const char* key/*key,��:cmd=aa,cmd����key,aa����value*/, std::string& value) const;
//................................................................................
public:
//.......................................................
//!!!���º����ϲ�����ֹ����!!!

	int		ParseHttpRequest(const char* data, size_t len, size_t maxlen, bool& over, const uint64_t& request_id, const int64_t& request_time);

	//����һЩ���Ʊ�־
	void	ResetSomeCtrlFlag();

	//���Ѿ��ظ���100-continue��־
	void 	SetHasRsp100Continue();

	//�Ƿ���Ҫ�ظ�100continue;
	bool 	HasRsp100Continue();

	//����ͷ���Ƿ���100-continue;
	bool	b100ContinueInHeader();

	//���ý�����
	void	ResetParser();

	//�����ѽ��յ�������
	void	SaveIncompleteData(const char* pData, size_t nLen);
//.......................................................
private:
	void* p;
	std::string m_strReqIp;
	uint16_t m_uReqPort;
	bool m_bHasRsp100Continue;
	bool m_bIncomplete;
	std::string m_strRequest;
	uint64_t m_request_id;
	int64_t m_request_tm;
};

class HttpResponse
{
public:
	HttpResponse();
	~HttpResponse();
public:
	//����״̬�룬200��400��404 etc.
	//ע��:�ú����ڲ���У�飬ֻ�������һ��
	void SetStatusCode(int nStatusCode);

	//����conetnettype���磺text/json��application/x-www-form-urlencoded��text/xml
	//ע��:�ú����ڲ���У�飬ֻ�������һ��
	void SetContentType(const std::string& strContentType);

    //����Location���磺��״̬��Ϊ301��302����Э����תʱ����Ҫ���øò���ָ����ת��ַ
	//ע��:�ú����ڲ���У�飬ֻ�������һ��
	void SetLocation(const std::string& strLocation);

	//����һЩû����ȷ����ķǱ�׼��http��Ӧͷ
	//��ʽ��:x-token: AAA\r\nx-auth: BBB\r\n
	//ע��:�����\r\n��Ϊÿһ����Ľ���
	void SetHeader(const std::string& strFieldAndValue);
	
	//���������Ƿ񱣳֣�bKeepAlive=1����:Keep-Alive��bKeepAlive=0������:Close
	//ע��:�ú����ڲ���У�飬ֻ�������һ��
	void SetKeepAlive(bool bKeepAlive);

	//�Ƿ���Ҫ���ֳ�����
	bool IsKeepAlive() const;

	//���û�Ӧ���İ���
	//ע��:������SetStatusCode&SetContentType&SetKeepAlive������OK���Ժ�ſ��Ե���
	//ע��:�ú����ڲ���У�飬ֻ�������һ��
	void SetResponseBody(const std::string& strRspBody);

	//���ϲ�����ʽ���õ������Ļ�Ӧ���ģ�ͷ+�壩���ý���
	//ʹ�øú���ʱSetStatusCode&SetContentType&SetKeepAlive�Կ���ͬʱ���ã���SetResponseBodyһ�����ܵ���
	void SetHttpResponse(std::string& strResponse);

	//�����response��Ӧ��http�����requestId���ý���,request_idͨ��HttpRequest��GetHttpRequestId��ȡ
	void SetHttpRequestId(const uint64_t& request_id);

	//�����response��Ӧ��http�����requestTime���ý���,request_timeͨ��HttpRequest��GetHttpRequestTime��ȡ
	void SetHttpRequestTime(const int64_t& request_time);

	//�������response��Ӧ��http�����requestId
	uint64_t GetHttpRequestId() const;

	//�������response��Ӧ��http�����requestTime
	int64_t GetHttpRequestTime() const;

	//���ظ�ʽ���õ�httpresponse�ַ���
	//ע��:�����ڵ���SetResponseBody��SetHttpResponse�Ժ�ſ��Ե���
	const std::string& GetHttpResponse() const;

	//����http��Ӧ���ܳ���(ͷ+��)
	//ע��:������SetResponseBody����OK���Ժ�ſ��Ե���
	size_t GetHttpResponseTotalLen() const;
private:
	void FormatHttpResponse(const std::string& strRspBody);
private:
	std::string m_strResponse;
	std::string m_strContentType;
    std::string m_strLocation;
	std::string m_strOtherFieldValue;
	int  m_nStatusCode;
	bool m_bKeepAlive;
	bool m_bHaveSetStatusCode;
	bool m_bHaveSetContentType;
	bool m_bHaveSetLocation;
	bool m_bHaveSetKeepAlive;
	bool m_bHaveSetResponseBody;
	uint64_t m_request_id;
	int64_t m_request_tm;
};

//////////////////////////////////////////////////////�ص�����//////////////////////////////////////////////////////////////
//�ϲ���ô�����ʵ��ʱ������Ҫ���ٲ����������Լ��������������ײ㴦������Ҫע�⣬���ܼ���������ײ���߳̽���ɵ��߳�

	//��������صĻص�(����/�رյ�)
	//����ֵ0:����ɹ� >0�����������ִ���򷴿ص���Ϣ(����) <0:�ײ������ǿ�ƹر�����
	//sessioninfoΪ����settcpsessioninfo,sethttpsessioninfoʱ�����sessioninfo
	typedef int (*pfunc_on_connection)(void* pInvoker, uint64_t conn_uuid, const boost::any& conn, const boost::any& sessioninfo, bool bConnected, const char* szIpPort);

	//��������ȡ����Ϣ�Ļص�(������tcpserver)
	//����ֵ0:����ɹ� >0�����������ִ���򷴿ص���Ϣ(����) <0:�ײ������ǿ�ƹر�����
	//nReadedLen���ڿ��Ʋа�ճ��������ǲа�:������0�������ճ��:�������������Ĵ�С*�������ĸ���
	//sessioninfoΪ����settcpsessioninfo,sethttpsessioninfoʱ�����sessioninfo
	typedef int (*pfunc_on_readmsg_tcp)(void* pInvoker, uint64_t conn_uuid, const boost::any& conn, const boost::any& sessioninfo, const char* szMsg, int nMsgLen, int& nReadedLen);

	//��������ȡ����Ϣ�Ļص�(������httpserver)
	//����ֵ0:����ɹ� >0�����������ִ���򷴿ص���Ϣ(����) <0:�ײ������ǿ�ƹر�����
	//ע��:pHttpReq�뿪�ص�������ָ�뽫��ʧЧ,���Բ����Խ�����ָ��������ҵ�������ʹ��,�������뿪�ص�����ǰ������������!!!
	//sessioninfoΪ����settcpsessioninfo,sethttpsessioninfoʱ�����sessioninfo
	typedef int (*pfunc_on_readmsg_http)(void* pInvoker, uint64_t conn_uuid, const boost::any& conn, const boost::any& sessioninfo, const HttpRequest* pHttpReq);

	//����������ɵĻص�
	//����ֵ0:����ɹ� >0�����������ִ���򷴿ص���Ϣ(����) <0:�ײ������ǿ�ƹر�����
	//paramsΪ����sendhttpmsg,sendtcpmsgʱ�����params
	//sessioninfoΪ����settcpsessioninfo,sethttpsessioninfoʱ�����sessioninfo
	typedef int (*pfunc_on_sendok)(void* pInvoker, const uint64_t conn_uuid, const boost::any& conn, const boost::any& sessioninfo, const boost::any& params);

	// ��������ʧ�ܵĻص�,����������pfunc_on_sendok��ͬ
	// errCode:
	//1:�����Ѳ�����        
	//2:���ͻ�����(��ʱ���᷵��,��ʱͨ����ˮλ�ص���֪�ϲ�)       
	//3:����write����ʧ�� 
	//4:���ͳ�ʱ(δ�����õĳ�ʱ���ڷ��ͳɹ�)
	typedef int(*pfunc_on_senderr)(void* pInvoker, const uint64_t conn_uuid, const boost::any& conn, const boost::any& sessioninfo, const boost::any& params, int errCode);

	//��ˮλ�ص� ���ײ㻺��Ĵ����������������õ�ֵʱ�ᴥ���ûص�
	//����ֵ0:����ɹ� >0�����������ִ���򷴿ص���Ϣ(����) <0:�ײ������ǿ�ƹر�����
	typedef int (*pfunc_on_highwatermark)(void* pInvoker, const uint64_t conn_uuid, const boost::any& conn, const boost::any& sessioninfo, size_t highWaterMark);
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//�����Ƿ����˿ڸ���:
//�����ӽ϶������������ö˿ڸ���,����������߶����ӽ�������
//�����ӽ϶�Ĳ���������,��Ϊ���ú����ӷ�����ϵͳ���,�޷�ȷ��ÿ��IO�߳������ӵ�ƽ����

class NetServer
	{
	static const size_t kDefRecvBuf_httpsvr = 4  * 1024;
	static const size_t kMaxRecvBuf_httpsvr = 32 * 1024;
	static const size_t kMaxSendQue_httpsvr = 16;
	static const size_t kDefRecvBuf_tcpsvr  = 4  * 1024;
	static const size_t kMaxRecvBuf_tcpsvr  = 32 * 1024;
	static const size_t kMaxSendQue_tcpsvr  = 64;

	public:
		//��ʼ��������server
		//����������Ψһ����ID�ĳ�����ʹ�ñ����캯��
		NetServer(uint16_t unique_node_id/*ϵͳ��Ψһ���������ID,1~8000��ȡֵ*/);
		NetServer();
		//����nodeid(1~8000��ȡֵ)
		void SetUniqueNodeId(uint16_t unique_node_id);
		~NetServer();
	public:
		// ������رս���/������־,Ĭ����־ȫ�����Զ�������,��Ҫһ�������Զ��ر���־,��Ҫ��startserver�������º����ر�
		void 	EnableHttpServerRecvLog(bool bEnable=true);
		void 	EnableHttpServerSendLog(bool bEnable=true);
		void 	EnableTcpServerRecvLog(bool bEnable=true);
		void 	EnableTcpServerSendLog(bool bEnable=true);
	public:
		//����TCP����
		void 	StartTcpServer(void* pInvoker,		/*�ϲ���õ���ָ��*/
								uint16_t listenport,	/*�����˿�*/
								int threadnum,			/*�߳���*/
								int idleconntime,		/*�������߿�ʱ��*/
								int maxconnnum,			/*���������*/
								bool bReusePort,		/*�Ƿ�����SO_REUSEPORT*/
								pfunc_on_connection pOnConnection, 		/*��������ص��¼��ص�(connect/close/error)*/
								pfunc_on_readmsg_tcp pOnReadMsg, 	   	/*������յ����ݵ��¼��ص�*/
								pfunc_on_sendok pOnSendOk,				/*����������ɵ��¼��ص�*/
								pfunc_on_senderr pOnSendErr,			/*��������ʧ�ܵ��¼��ص�*/
								pfunc_on_highwatermark pOnHighWaterMark=NULL,/*��ˮλ�¼��ص�*/
								size_t nDefRecvBuf = kDefRecvBuf_tcpsvr,	/*ÿ�����ӵ�Ĭ�Ͻ��ջ���,��nMaxRecvBufһ��������ʵҵ�����ݰ���С��������,��ֵ����Ϊ�󲿷�ҵ������ܵĴ�С*/
								size_t nMaxRecvBuf = kMaxRecvBuf_tcpsvr,   	/*ÿ�����ӵ������ջ���,�����������ջ�����Ȼ�޷����һ��������ҵ�����ݰ�,���ӽ���ǿ�ƹر�,ȡֵʱ��ؽ��ҵ�����ݰ���С������д*/
								size_t nMaxSendQue = kMaxSendQue_tcpsvr		/*ÿ�����ӵ�����ͻ������,��������������̵�δ���ͳ�ȥ�İ�������,��������ֵ,���᷵�ظ�ˮλ�ص�,�ϲ�������pfunc_on_sendok���ƺû���*/
								);

		//ֹͣTCP����
		void 	StopTcpServer();

		//����TCP��Ϣ 
		//conn_uuid:���ӽ���ʱ���ص�����ΩһID
		//conn:���ӽ���ʱ���ص����Ӷ���
		//params:�Զ��巢�Ͳ������������/ʧ�ܻص�ʱ�᷵�أ���������ָ��
		//szMsg,nMsgLen:Ҫ�������ݵ����ݺͳ���
		//timeout:Ҫ��ײ�೤ʱ���ڱ��뷢������δ���ͳɹ�����ʧ�ܵĻص�.��λ:����,Ĭ��Ϊ0,����Ҫ��ʱ��
		//bKeepAlive:�����Ժ��Ƿ񱣳�����.������Ϊfalse����OnSendOk�ص���ɺ󣬻��Զ��Ͽ�����
		//0:�ɹ�    1:��ǰ�����Ѳ�����          2:��ǰ��������ͻ�������
		int 	SendMsgWithTcpServer(uint64_t conn_uuid, const boost::any& conn, const boost::any& params, const char* szMsg, size_t nMsgLen, int timeout=0, bool bKeepAlive=true);

		//��ͣ��ǰTCP���ӵ����ݽ���
		//conn_uuid:���ӽ���ʱ���ص�����ΩһID
		//conn:���ӽ���ʱ���ص����Ӷ���
		//delay:��ͣ���յ�ʱ��.��λ:����,Ĭ��Ϊ10ms,<=0ʱ����ʱ
		void 	SuspendTcpConnRecv(uint64_t conn_uuid, const boost::any& conn, int delay);

		//�ָ���ǰTCP���ӵ����ݽ���
		//conn_uuid:���ӽ���ʱ���ص�����ΩһID
		//conn:���ӽ���ʱ���ص����Ӷ���
		void 	ResumeTcpConnRecv(uint64_t conn_uuid, const boost::any& conn);

		//ǿ�ƹر�TCP����
		void 	ForceCloseTcpConnection(uint64_t conn_uuid, const boost::any& conn);

		//��ȡ��ǰTCP��������
		int 	GetCurrTcpConnNum();

		//��ȡ��ǰTCP��������
		int64_t GetTotalTcpReqNum();

		//��ȡ��ǰTCP�ܵĵȴ����͵����ݰ�����
		int64_t GetTotalTcpWaitSendNum();

		//����TCP�������߿�ʱ��
		void	ResetTcpIdleConnTime(int idleconntime);

		//����TCP��֧�ֵ����������
		void	ResetTcpMaxConnNum(int maxconnnum);
		
		//����tcp�Ự����Ϣ(�봫session�������ָ�룬����ֻ�������һ��&session�������п��ƶ��̻߳�������)
		void	SetTcpSessionInfo(uint64_t conn_uuid, const boost::any& conn, const boost::any& sessioninfo);

		//���ø�ˮλֵ (thread safe)
		void 	SetTcpHighWaterMark(uint64_t conn_uuid, const boost::any& conn, size_t highWaterMark);
	public:
		//����HTTP����
		void StartHttpServer(void* pInvoker,		/*�ϲ���õ���ָ��*/
								uint16_t listenport,	/*�����˿�*/
								int threadnum,			/*�߳���*/
								int idleconntime,		/*�������߿�ʱ��*/
								int maxconnnum,			/*���������*/
								bool bReusePort,		/*�Ƿ�����SO_REUSEPORT*/
								pfunc_on_connection pOnConnection, 		/*��������ص��¼��ص�(connect/close/error)*/
								pfunc_on_readmsg_http pOnReadMsg, 	   	/*������յ����ݵ��¼��ص�*/
								pfunc_on_sendok pOnSendOk,				/*����������ɵ��¼��ص�*/
								pfunc_on_senderr pOnSendErr,			/*��������ʧ�ܵ��¼��ص�*/
								pfunc_on_highwatermark pOnHighWaterMark=NULL,/*��ˮλ�¼��ص�*/
								size_t nDefRecvBuf = kDefRecvBuf_httpsvr,	/*ÿ�����ӵ�Ĭ�Ͻ��ջ���,��nMaxRecvBufһ��������ʵҵ�����ݰ���С��������,��ֵ����Ϊ�󲿷�ҵ������ܵĴ�С*/
								size_t nMaxRecvBuf = kMaxRecvBuf_httpsvr,	/*ÿ�����ӵ������ջ���,�����������ջ�����Ȼ�޷����һ��������ҵ�����ݰ�,���ӽ���ǿ�ƹر�,ȡֵʱ��ؽ��ҵ�����ݰ���С������д*/
								size_t nMaxSendQue = kMaxSendQue_httpsvr 	/*ÿ�����ӵ�����ͻ������,��������������̵�δ���ͳ�ȥ�İ�������,��������ֵ,���᷵�ظ�ˮλ�ص�,�ϲ�������pfunc_on_sendok���ƺû���*/
								);

		//ֹͣHTTP����
		void 	StopHttpServer();

		//����HTTP��Ӧ
		//conn_uuid:���ӽ���ʱ���ص�����ΩһID
		//conn:���ӽ���ʱ���ص����Ӷ���
		//params:�Զ��巢�Ͳ������������/ʧ�ܻص�ʱ�᷵�أ���������ָ��
		//rsp:Ҫ����response
		//timeout:Ҫ��ײ�೤ʱ���ڱ��뷢������δ���ͳɹ�����ʧ�ܵĻص�.��λ:����,Ĭ��Ϊ0,����Ҫ��ʱ��
		//0:�ɹ�    1:��ǰ�����Ѳ�����          2:��ǰ��������ͻ�������
		int 	SendHttpResponse(uint64_t conn_uuid, const boost::any& conn, const boost::any& params, const HttpResponse& rsp, int timeout=0);

		//ǿ�ƹر�HTTP����
		void 	ForceCloseHttpConnection(uint64_t conn_uuid, const boost::any& conn);

		//��ȡ��ǰHTTP��������
		int 	GetCurrHttpConnNum();

		//��ȡ��ǰHTTP��������
		int64_t GetTotalHttpReqNum();

		//��ȡ��ǰHTTP�ܵĵȴ����͵����ݰ�����
		int64_t GetTotalHttpWaitSendNum();

		//����HTTP�������߿�ʱ��
		void	ResetHttpIdleConnTime(int idleconntime);

		//����HTTP��֧�ֵ����������
		void	ResetHttpMaxConnNum(int maxconnnum);

		//����http�Ự����Ϣ(�봫session�������ָ�룬����ֻ�������һ��&session�������п��ƶ��̻߳�������)
		void	SetHttpSessionInfo(uint64_t conn_uuid, const boost::any& conn, const boost::any& sessioninfo);

		//���ø�ˮλֵ (thread safe)
		void	SetHttpHighWaterMark(uint64_t conn_uuid, const boost::any& conn, size_t highWaterMark);

	private:

		// ȫ�ֿ��Ʊ���
		NetServerCtrlInfoPtr m_NetServerCtrl;
	};
}
}
#endif  // MWNET_MT_NETSERVER_H

