#pragma once

#include <string>
#include <map>
#include <stdlib.h>
#include <string.h>

extern "C"
{
#include "http_parser.h"    
}


namespace mw_http_parser
{

#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wold-style-cast"
	/*
	˵��:
	ֻ֧��key1=value&key2=value&key3=value��ʽ��http body
	���÷���:
	http_urlencoded_body_parser body_parser;
	body_parser.parse_http_body("cmd=aa&seqid=1&msg=11", strlen("cmd=aa&seqid=1&msg=11"));
	char* p = NULL;
	size_t len = 0;
	body_parser.get_value("cmd", &p, &len);
	body_parser.get_value("seqid", &p, &len);
	body_parser.get_value("msg", &p, &len);
	*/
	class http_urlencoded_body_parser
	{
	public:
		http_urlencoded_body_parser() {};
		virtual ~http_urlencoded_body_parser() {};
	public:
		void clear() { key_value_map.clear(); };		
	public:
		//ͨ��key��ȡvalue��ָ��ͳ���,�������忽��.������false����key������
		bool get_value(const char* key/*key,��:cmd=aa,cmd����key,aa����value*/, char** value, size_t* len)
		{
			bool bRet = false;
			//ͳһ��д���д���
			std::string strKey = key;
			to_upper_s(strKey);
			std::map<std::string, item_value>::iterator it = key_value_map.find(strKey);
			if (it != key_value_map.end())
			{
				*value = it->second.at;
				*len   = it->second.len;
				bRet   = true;
			}
			
			return bRet;
		}

		//ͨ��key��ȡvalue�����ݺͳ���.������false����key������
		bool get_value(const char* key/*key,��:cmd=aa,cmd����key,aa����value*/, std::string& value)
		{
			bool bRet = false;
			//ͳһ��д���д���
			std::string strKey = key;
			to_upper_s(strKey);
			std::map<std::string, item_value>::iterator it = key_value_map.find(strKey);
			if (it != key_value_map.end())
			{
				value.assign(it->second.at, it->second.len);
				bRet = true;
			}
			
			return bRet;
		}
		//����key,value��ʽ��body,��:cmd=aa&seqid=1&msg=11
		bool parse_http_body(const char* p/*http body ��:cmd=aa&seqid=1&msg=11*/, size_t len/*http body�ĳ���*/)
		{
			/*
			bool bRet = true;
			
			char *eq   = NULL;
			char *pand = NULL;
			const char *head = p;
			
			std::string strKey("");
			item_value item;
			
			while (p < (head + len) && *p != '\0')
			{
				if (*p == '&')
				{
					p++;
					continue;
				}
				
				if ((eq = (char*)strchr(p, '=')))
				{
					pand = (char*)strchr((const char*)(eq), '&');
				}
				
				if (eq && pand && (eq < pand))
				{
					//offset
					size_t offset = pand - p;
					
					//key
					strKey.assign(p, eq - p);
					
					//value
					item.at  = eq + 1;
					item.len = pand - eq - 1;
					
					//pointer move
					p += offset;
					
					//add to map
					to_upper_s(strKey);
					key_value_map.insert(std::make_pair(strKey, item));
					
					// 				char szBuf[32] = {0};
					// 				memcpy(szBuf, item.at, item.len);
					// 				TRACE2("%s:%s\r\n", strKey.c_str(), szBuf);
				}
				//���һ��
				else if (eq)
				{
					//offset
					size_t offset = head + len - p;
					
					//key
					strKey.assign(p, eq - p);
					
					//value
					item.at  = eq + 1;
					item.len = head + len - eq - 1;
					
					//pointer move
					p += offset;
					
					//add to map
					to_upper_s(strKey);
					key_value_map.insert(std::make_pair(strKey, item));
					
					// 				char szBuf[32] = {0};
					// 				memcpy(szBuf, item.at, item.len);
					// 				TRACE2("%s:%s\r\n", strKey.c_str(), szBuf);
				}
				else
				{
					bRet = false;
					//��ʽ����
					break;
				}	
			}
			
			return bRet;
			*/
			
			bool bRet = true;
			std::string strKey("");
			item_value  item;
		
			char *pBegin,*pEnd,*pCur,*pEq;
			
			pBegin	= pCur = (char*)p;
			pEnd	  = pBegin + len;

			pEq = NULL;

			while(pBegin < pEnd && pCur != pEnd)
    	{
				strKey = "";
      	pCur = strchr(pBegin,'&');
      	if(NULL == pCur)
      	{
      		//���һ��
					pCur = pEnd;
      	}
				/*
				��key �� value
				pBegin �� pCur֮�� ��key=value
				����Ҳ���'='ֱ������
				pEq ָ�����=��, pCur ָ�����&
				*/
				//====================begin �ҵ�key��value
				pEq = strchr(pBegin,'=');
				if(pEq)
				{
					//key
					strKey.assign(pBegin, pEq - pBegin);
				
					//value
					item.at  = pEq + 1;
					item.len = (char*)pCur - (char*)pEq - 1;
				
					to_upper_s(strKey);
					key_value_map.insert(std::make_pair(strKey, item));
				}
				//====================end  �ҵ�key��value
      	pBegin = pCur + 1;
    	}
    	return bRet;
		}		
	private:
		struct item_value  
		{
			char* at;		//λ��
			size_t len;		//����
			item_value()
			{
				at  = NULL;
				len = 0;
			}
		};

		void to_upper_s(std::string& s)
		{
			int  i = 0;
			while(s[i] != '\0')
			{
				if((s[i] >= 'a') && (s[i] <= 'z'))
				{
					s[i] -= 32;
				}
				++i;
			}
		}
		//��ֵ��
		std::map<std::string, item_value> key_value_map;
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////


class HttpParser
{
public:
	friend class HttpParserSettings;
	
	HttpParser(enum http_parser_type type/*ֻ����д HTTP_REQUEST �� HTTP_RESPONSE */);

	virtual ~HttpParser();
	
	enum Event
	{
		Parser_MessageBegin,    
        Parser_Url,
        Parser_Status,
        Parser_HeaderField,
        Parser_HeaderValue,
        Parser_HeadersComplete,
        Parser_Body,
        Parser_MessageComplete,
		Parser_ChunkHeaser,
		Parser_ChunkComplete
	};

	enum header_state
	{
		header_state_init,
		header_state_content_type,
		header_state_host,
		header_state_x_forwarded_for,
		header_state_expect,
		header_state_soap_action,
		header_state_user_agent,
		header_state_via,
		header_state_x_wap_profile,
		header_state_x_browser_type,
		header_state_x_up_bear_type
	};

public:
	//ִ�н���.
	//����ִ��N��,���һ��http����,����һ���յ���,���Զ�ε���
	//����ֵ:0:���ν���OK  ��0:���ν���ʧ�� ��ʧ�������get_http_parse_errno_description��ȡʧ��ԭ��
	//��over����true,������ɵ�http���Ľ��������,���Ե�������������http���ݻ�ȡ
	int		execute_http_parse(const char* data/*����Ҫ����������*/, size_t len/*����Ҫ���������ݳ���*/, bool& over/*�����Ƿ������http���������*/);
	
	//����execute_http_parse���ص�ʧ�ܴ���,��ȡʧ��ԭ��
	const char * get_http_parse_errno_description(int errno);

	//��ȡhttp��Ӧ��״̬��,��200,202,400,403,404,500��
	//ֻ�е���ʼ��ΪHTTP_RESPONSE����ʱ���ܻ�ȡ����ȷ��ֵ
	//����ֵ:״̬��
	int		get_http_response_status_code();

	//�Ƿ���Ҫ��������,��connection��keep-alive����close
	//����ֵ:true:keep-alive false:close
	bool	should_keep_alive();

	//�Ƿ���100 continue
	bool	expect_100_continue();

	//�����Ѿ���Ӧ��100-continue��ʶ
	//�Ƿ���chunk��
	//����ֵ:true:chunk�� false:��chunk��
	bool	is_chunk_body();

	//��ȡhttp����ı��ĳ���(ͷ+��)
	//����ֵ:ͷ+��ĳ���
	size_t  get_http_total_req_len();
	
	//��ȡhttp body�ĳ���
	//����ֵ:body�ĳ���
	size_t	get_http_body_len();

	//��ȡhttp body������
	//����ֵ:body��ָ��ͳ���
	size_t	get_http_body_data(char** data, size_t* len);

	//��ȡchunk����http body������
	//����ֵ:body�ĳ��Ⱥ�����
	//ע��:�����is_chunk_body���ж��Ƿ���chunk��,Ȼ�����get_http_body_len�õ����ȷ����㹻�Ĵ�С��data
	size_t	get_http_body_chunk_data(char* data);

	//��ȡ����url
	//����ֵ:url��ָ��,�Լ�����
	//˵��:��:post����,POST /MWGate/wmgw.asmx/MongateCsSpSendSmsNew HTTP/1.1 ,/MWGate/wmgw.asmx/MongateCsSpSendSmsNew����URL
	//     ��:get����,
	size_t	get_http_url(char** url, size_t* len);

	//��ȡUser-Agent
	//����ֵ:User-Agent��ָ��,�Լ�����
	size_t	get_http_user_agent(char** useragent, size_t* len);

	//��ȡcontent-type
	//����ֵ:CONTENT_TYPE_XML ���� xml,CONTENT_TYPE_JSON ���� json,CONTENT_TYPE_URLENCODE ���� urlencode,CONTENT_TYPE_UNKNOWN ���� δ֪����
	//˵��:��ֻ�ṩ������
	int		get_http_content_type();
	std::string		get_http_content_type_str();

	//��ȡhost
	//����ֵ:host��ָ��,�Լ�����
	size_t	get_http_host(char** host, size_t* len);

	//��ȡx-forwarded-for
	//����ֵ:X-Forwarded-For��ָ��,�Լ�����
	size_t	get_http_x_forwarded_for(char** x_forwarded_for, size_t* len);

	//��ȡhttp���������
	//����ֵ:HTTP_REQUEST_POST ���� post����; HTTP_REQUEST_GET ���� get���� ; HTTP_REQUEST_UNKNOWN ���� δ֪(������ͷ�д���)
	int		get_http_request_type();

	//��ȡSOAPAction
	//����ֵ:SOAPAction��ָ��,�Լ�����
	size_t	get_http_soapaction(char** soapaction, size_t* len);
	
	//��ȡ via
	//����ֵ:via��ָ��,�Լ�����
	size_t	get_http_via(char** via, size_t* len);

	//��ȡ X-WAP-PROFILE
	//����ֵ:X-WAP-PROFILE ��ָ��,�Լ�����
	size_t	get_http_x_wap_profile(char** x_wap_profile, size_t* len);

	//��ȡ X-BROWSER-TYPE
	//����ֵ:X-BROWSER-TYPE ��ָ��,�Լ�����
	size_t	get_http_x_browser_Type(char** x_browser_type, size_t* len);

	//��ȡ X-UP-BEAR-TYPE
	//����ֵ:X-UP-BEAR-TYPE ��ָ��,�Լ�����
	size_t	get_http_x_up_bear_Type(char** x_up_bear_type, size_t* len);

	//��ȡ ����http���ĵ�����(ͷ+��)
	//����ֵ:http���ĵ�ָ��,�Լ�����
	size_t	get_http_request_content(char** request_content, size_t* len);

	//��ȡ httpͷ����һ�����Ӧ��ֵ
	//����ֵ:��ֵ��ָ��ͳ���
	size_t	get_http_header_field_value(const char*field, char** value, size_t* len);
	
	//�ظ�ʹ�ó�ʼ��
	//�������һ��֮��������������٣����ظ�ʹ�ã�������øú���
	void	reset_parser();
	
//..........................�����ṩһ�����urlencode�ĺ���......................
	//ͨ��key��ȡvalue��ָ��ͳ���,�������忽��.������false����key������
	//ע��:�����ȵ���parse_http_body��ſ���ʹ�øú���
	bool	get_value(const char* key/*key,��:cmd=aa,cmd����key,aa����value*/, char** value, size_t* len);

	//ͨ��key��ȡvalue�����ݺͳ���.������false����key������
	//ע��:�����ȵ���parse_http_body��ſ���ʹ�øú���
	bool	get_value(const char* key/*key,��:cmd=aa,cmd����key,aa����value*/, std::string& value);

	//����key,value��ʽ��body,��:cmd=aa&seqid=1&msg=11
	//ע��:����GetContentType����CONTENT_TYPE_URLENCODE�ſ���ʹ�øú���ȥ����
	bool	parse_http_body(const char* p/*http body ��:cmd=aa&seqid=1&msg=11*/, size_t len/*http body�ĳ���*/);
//................................................................................
	
protected:
	virtual int onMessageBegin() { return 0; }
	virtual int onUrl(const char*, size_t) { return 0; }
	virtual int onHeader(const std::string& field, const std::string& value) { return 0; }
	virtual int onHeadersComplete() { return 0; }
	virtual int onBody(const char*, size_t) { return 0; }
	virtual int onStatus(const char*, size_t) { return 0; }
	virtual int onMessageComplete() { return 0; }
	virtual int onUpgrade(const char*, size_t) { return 0; }
	virtual int onChunkHeader() { return 0; }
	virtual int onChunkComplete() { return 0; }
	
private:
	int onParser(Event, const char*, size_t);
	int handleUrl(const char*, size_t);
	int handleStatus(const char*, size_t);
	int handleMessageBegin();
	int handleHeaderField(const char*, size_t);
	int handleHeaderValue(const char*, size_t);
	int handleBody(const char*, size_t);
	int handleHeadersComplete();
	int handleMessageComplete();
	int handleChunkHeader();
	int handleChunkComplete();
	int parse_content_type();
	int parse_request_type();
	
	//�ͷ�����
	void clear_chunk_list()
	{
		chunk_list* next = m_pHeaderChunk->next;
		for (;;)
		{
			if (next != NULL)
			{
				chunk_list* p = next->next;
				delete next;
				next = p;
			}
			else
			{
				break;
			}
		}
		m_pHeaderChunk->next = NULL;
		m_pCurChunk = m_pHeaderChunk;
	}

	void to_upper_s(std::string& s)
    {
        int  i = 0;
        while(s[i] != '\0')
        {
            if((s[i] >= 'a') && (s[i] <= 'z'))
            {
                s[i] -= 32;
            }
            ++i;
        }
    }

	//�����ִ�Сд�Ƚ�
	int http_strnicmp(const char *s1, const char *s2, size_t len)  
	{  
		/* Yes, Virginia, it had better be unsigned */  
		unsigned char c1, c2;  
  
		c1 = 0; c2 = 0;  
		if (len) {  
			do {  
				c1 = *s1; c2 = *s2;  
				s1++; s2++;  
							  //�Ƿ��ѵ��ַ�����ĩβ�����ַ����Ƿ��пմ�,�������ĩβ���пմ�,��Ƚ����  
				if (!c1)  
					break;  
				if (!c2)  
					break;  
							  //���û��,���ַ������,������Ƚ��¸��ַ�  
				if (c1 == c2)  
					continue;  
							  //�������ͬ,��ͬʱת��ΪСд�ַ��ٽ��бȽ�  
				c1 = tolower(c1);  
				c2 = tolower(c2);  
							  //�������ͬ,��Ƚ����,�������  
				if (c1 != c2)  
					break;  
			} while (--len);  
		}  
		return (int)c1 - (int)c2;  
	}  

	//�����ִ�Сд����
	char* http_strstri(const char* pString, const char* pFind)
	{
		char* char1 = NULL;
		char* char2 = NULL;
		if((pString == NULL) || (pFind == NULL) || (strlen(pString) < strlen(pFind)))
		{
			return NULL;
		}
		
		for(char1 = (char*)pString; (*char1) != '\0'; ++char1)
		{
			char* char3 = char1;
			for(char2 = (char*)pFind; (*char2) != '\0' && (*char1) != '\0'; ++char2, ++char1)
			{
				char c1 = (*char1) & 0xDF;
				char c2 = (*char2) & 0xDF;
				if((c1 != c2) || (((c1 > 0x5A) || (c1 < 0x41)) && (*char1 != *char2)))
					break;
			}
			
			if((*char2) == '\0')
				return char3;
			
			char1 = char3;
		}
		return NULL;
	}

	//��ʼ�����Ա
	void init_class_members();
	
	enum http_parser_type getType() { return (http_parser_type)m_parser.type; }

	HttpParser(enum http_parser_type type/*ֻ����д HTTP_REQUEST �� HTTP_RESPONSE */, bool bIsFixedUrlEncodedBody/*�Ƿ�����ȷ֪��body��urlencoded��ʽ��,��:cmd=aa&seqid=1&msg=111*/);

	//��ȡhttpͷ�е��� �� ֵ 
	//����falseʱ��ʾͷ��û�и���
	//ע��:!!!����ʹ��bIsFixedUrlEncodedBodyΪtrue,���Һ���ȷ��֪��û��chunk���Ĳ��ܻ�ȡ����Ҫ������,���������get_http_body��ȡbody
	//bool get_http_body_filed_value(const char* field, char** value, size_t* len);

	//��ȡhttpͷ�е��� �� ֵ
	//����falseʱ��ʾͷ��û�и���
	//bool get_http_header_filed_value(const char* field, char** value, size_t* len);

protected:
	struct http_parser m_parser;
	struct item_value  
	{
		char* at;		//λ��
		size_t len;		//����
		item_value()
		{
			at  = NULL;
			len = 0;
		}
	};
	//chunk ��ṹ
	struct chunk_block 
	{
		char* at;		//λ��
		size_t len;		//����
		chunk_block()
		{
			at  = NULL;
			len = 0;
		}
	};
	//chunk ������
	struct chunk_list
	{
		chunk_block block;
		chunk_list* next;
		chunk_list()
		{
			next   = NULL;
		}
	};

	bool m_bLastIsValue;												//����ϴ�һ�������ǲ���httpͷ�����Ӧ��value
	char* m_pCurField;													//ͷ�е�ǰ�����Ƶ�ָ��
	size_t m_nCurFieldLen;												//��ǰ�����Ƶĳ���
	enum http_parser_type m_parser_type;								//�����������ں���������(request/response)
	chunk_list* m_pHeaderChunk;											//chunk����ͷ
	chunk_list* m_pCurChunk;											//chunk����ǰλ��													
	std::map<std::string, item_value> m_mapHeaderFieldValue;			//HTTPͷ�е�����ֵ�Ķ�Ӧ��ϵ
	item_value m_itemValue;												//��ǰ���ڽ��������Ӧ��ֵ
	bool m_bIsFixedUrlEncodedBody;										//�Ƿ�����ȷ֪��body��urlencoded��ʽ��,��:cmd=aa&seqid=1&msg=111
	http_urlencoded_body_parser m_urlencode_body_parser;				//body��urlencode��ʽ�Ľ�����
	bool m_bChunkPack;													//Chunk�����
	bool m_bParseOver;													//�Ƿ������ɱ�־
	header_state m_curr_header_state;									//�������http����ͷ��ǰ�ص����ĸ���

	char* m_pBegin;														//����ʼָ��
	size_t m_nTotalParseLen;											//һ���������ܳ���
	
	int m_nHttpStatusCode;												//HTTP��Ӧ��״̬

	bool m_bIsKeepAlive;												//�����Ƿ���Ҫ����

	int   m_nReqType;													//�������� POST,GET
	char* m_pUrl;														//�����ַ��ָ��
	size_t m_nUrlLen;													//�����ַ�����ݳ���

	char* m_pBody;														//bodyָ��--��chunkʱ��
	size_t m_nBodyLen;													//body����

	int   m_nContentType;												//content-type
	char* m_pContentType;												//content-type��ָ��
	size_t m_nContentTypeLen;											//content-type���ݵĳ���
	
	char* m_pHost;														//host��ָ��
	size_t m_nHostLen;													//host���ݵĳ���
	
	char* m_pXForwardedFor;												//x_forwarded_for��ָ��
	size_t m_nXForwardedForLen;											//x_forwarded_for�ĳ���
	
	char* m_pExpect;													//except��ָ��
	size_t m_nExpectLen;												//except�ĳ���
	
	char* m_pSoapAction;												//soapaction��ָ��
	size_t m_nSoapActionLen;											//soapaction�ĳ���

	char* m_pUserAgent;													//UserAgent��ָ��
	size_t m_nUserAgentLen;												//UserAgent�ĳ���

	char* m_pVia;														//via��ָ��
	size_t m_nViaLen;													//via�ĳ���

	char* m_pXWapProfile;												//x-wap-profile��ָ��
	size_t m_nXWapProfileLen;											//x-wap-profile�ĳ���

	char* m_pXBrowserType;												//x-browser-type��ָ��
	size_t m_nXBrowserTypeLen;											//x-browser-type�ĳ���

	char* m_pXUpBearType;												//x-up-bear-type��ָ��
	size_t m_nXUpBearTypeLen;											//x-up-bear-type�ĳ���
};   

#pragma GCC diagnostic error "-Wconversion"
#pragma GCC diagnostic error "-Wold-style-cast"
}
