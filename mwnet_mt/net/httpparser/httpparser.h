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
	说明:
	只支持key1=value&key2=value&key3=value格式的http body
	调用方法:
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
		//通过key获取value的指针和长度,不做具体拷贝.若返回false代表key不存在
		bool get_value(const char* key/*key,如:cmd=aa,cmd就是key,aa就是value*/, char** value, size_t* len)
		{
			bool bRet = false;
			//统一大写进行处理
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

		//通过key获取value的内容和长度.若返回false代表key不存在
		bool get_value(const char* key/*key,如:cmd=aa,cmd就是key,aa就是value*/, std::string& value)
		{
			bool bRet = false;
			//统一大写进行处理
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
		//解析key,value格式的body,如:cmd=aa&seqid=1&msg=11
		bool parse_http_body(const char* p/*http body 如:cmd=aa&seqid=1&msg=11*/, size_t len/*http body的长度*/)
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
				//最后一个
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
					//格式错误
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
      		//最后一个
					pCur = pEnd;
      	}
				/*
				找key 和 value
				pBegin 和 pCur之间 是key=value
				如果找不到'='直接跳过
				pEq 指向的是=号, pCur 指向的是&
				*/
				//====================begin 找到key和value
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
				//====================end  找到key和value
      	pBegin = pCur + 1;
    	}
    	return bRet;
		}		
	private:
		struct item_value  
		{
			char* at;		//位置
			size_t len;		//长度
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
		//键值对
		std::map<std::string, item_value> key_value_map;
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////


class HttpParser
{
public:
	friend class HttpParserSettings;
	
	HttpParser(enum http_parser_type type/*只能填写 HTTP_REQUEST 或 HTTP_RESPONSE */);

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
	//执行解析.
	//可以执行N次,如果一个http报文,不是一次收到的,可以多次调用
	//返回值:0:本次解析OK  非0:本次解析失败 若失败请调用get_http_parse_errno_description获取失败原因
	//若over返回true,代表完成的http报文解析完成了,可以调其他函数进行http数据获取
	int		execute_http_parse(const char* data/*本次要解析的数据*/, size_t len/*本次要解析的数据长度*/, bool& over/*本次是否把整个http报解析完成*/);
	
	//根据execute_http_parse返回的失败代码,获取失败原因
	const char * get_http_parse_errno_description(int errno);

	//获取http回应的状态码,如200,202,400,403,404,500等
	//只有当初始化为HTTP_RESPONSE解析时才能获取到正确的值
	//返回值:状态码
	int		get_http_response_status_code();

	//是否需要保持连接,即connection是keep-alive还是close
	//返回值:true:keep-alive false:close
	bool	should_keep_alive();

	//是否有100 continue
	bool	expect_100_continue();

	//设置已经回应过100-continue标识
	//是否是chunk包
	//返回值:true:chunk包 false:非chunk包
	bool	is_chunk_body();

	//获取http请求的报文长度(头+体)
	//返回值:头+体的长度
	size_t  get_http_total_req_len();
	
	//获取http body的长度
	//返回值:body的长度
	size_t	get_http_body_len();

	//获取http body的数据
	//返回值:body的指针和长度
	size_t	get_http_body_data(char** data, size_t* len);

	//获取chunk包的http body的数据
	//返回值:body的长度和内容
	//注意:需配合is_chunk_body先判断是否是chunk包,然后调用get_http_body_len得到长度分配足够的大小给data
	size_t	get_http_body_chunk_data(char* data);

	//获取请求url
	//返回值:url的指针,以及长度
	//说明:如:post请求,POST /MWGate/wmgw.asmx/MongateCsSpSendSmsNew HTTP/1.1 ,/MWGate/wmgw.asmx/MongateCsSpSendSmsNew就是URL
	//     如:get请求,
	size_t	get_http_url(char** url, size_t* len);

	//获取User-Agent
	//返回值:User-Agent的指针,以及长度
	size_t	get_http_user_agent(char** useragent, size_t* len);

	//获取content-type
	//返回值:CONTENT_TYPE_XML 代表 xml,CONTENT_TYPE_JSON 代表 json,CONTENT_TYPE_URLENCODE 代表 urlencode,CONTENT_TYPE_UNKNOWN 代表 未知类型
	//说明:暂只提供这三种
	int		get_http_content_type();

	//获取host
	//返回值:host的指针,以及长度
	size_t	get_http_host(char** host, size_t* len);

	//获取x-forwarded-for
	//返回值:X-Forwarded-For的指针,以及长度
	size_t	get_http_x_forwarded_for(char** x_forwarded_for, size_t* len);

	//获取http请求的类型
	//返回值:HTTP_REQUEST_POST 代表 post请求; HTTP_REQUEST_GET 代表 get请求 ; HTTP_REQUEST_UNKNOWN 代表 未知(解析包头有错误)
	int		get_http_request_type();

	//获取SOAPAction
	//返回值:SOAPAction的指针,以及长度
	size_t	get_http_soapaction(char** soapaction, size_t* len);
	
	//获取 via
	//返回值:via的指针,以及长度
	size_t	get_http_via(char** via, size_t* len);

	//获取 X-WAP-PROFILE
	//返回值:X-WAP-PROFILE 的指针,以及长度
	size_t	get_http_x_wap_profile(char** x_wap_profile, size_t* len);

	//获取 X-BROWSER-TYPE
	//返回值:X-BROWSER-TYPE 的指针,以及长度
	size_t	get_http_x_browser_Type(char** x_browser_type, size_t* len);

	//获取 X-UP-BEAR-TYPE
	//返回值:X-UP-BEAR-TYPE 的指针,以及长度
	size_t	get_http_x_up_bear_Type(char** x_up_bear_type, size_t* len);

	//获取 整个http报文的内容(头+体)
	//返回值:http报文的指针,以及长度
	size_t	get_http_request_content(char** request_content, size_t* len);

	//获取 http头中任一个域对应的值
	//返回值:域值的指针和长度
	size_t	get_http_header_field_value(const char*field, char** value, size_t* len);
	
	//重复使用初始化
	//解析完成一次之后，如果对象不想销毁，想重复使用，必须调用该函数
	void	reset_parser();
	
//..........................附带提供一组解析urlencode的函数......................
	//通过key获取value的指针和长度,不做具体拷贝.若返回false代表key不存在
	//注意:必须先调用parse_http_body后才可以使用该函数
	bool	get_value(const char* key/*key,如:cmd=aa,cmd就是key,aa就是value*/, char** value, size_t* len);

	//通过key获取value的内容和长度.若返回false代表key不存在
	//注意:必须先调用parse_http_body后才可以使用该函数
	bool	get_value(const char* key/*key,如:cmd=aa,cmd就是key,aa就是value*/, std::string& value);

	//解析key,value格式的body,如:cmd=aa&seqid=1&msg=11
	//注意:仅当GetContentType返回CONTENT_TYPE_URLENCODE才可以使用该函数去解析
	bool	parse_http_body(const char* p/*http body 如:cmd=aa&seqid=1&msg=11*/, size_t len/*http body的长度*/);
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
	
	//释放链表
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

	//不区分大小写比较
	int http_strnicmp(const char *s1, const char *s2, size_t len)  
	{  
		/* Yes, Virginia, it had better be unsigned */  
		unsigned char c1, c2;  
  
		c1 = 0; c2 = 0;  
		if (len) {  
			do {  
				c1 = *s1; c2 = *s2;  
				s1++; s2++;  
							  //是否已到字符串的末尾或两字符串是否有空串,如果到了末尾或有空串,则比较完毕  
				if (!c1)  
					break;  
				if (!c2)  
					break;  
							  //如果没有,且字符串相等,则继续比较下个字符  
				if (c1 == c2)  
					continue;  
							  //如果不相同,则同时转换为小写字符再进行比较  
				c1 = tolower(c1);  
				c2 = tolower(c2);  
							  //如果不相同,则比较完毕,否则继续  
				if (c1 != c2)  
					break;  
			} while (--len);  
		}  
		return (int)c1 - (int)c2;  
	}  

	//不区分大小写查找
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

	//初始化类成员
	void init_class_members();
	
	enum http_parser_type getType() { return (http_parser_type)m_parser.type; }

	HttpParser(enum http_parser_type type/*只能填写 HTTP_REQUEST 或 HTTP_RESPONSE */, bool bIsFixedUrlEncodedBody/*是否已明确知道body是urlencoded格式的,如:cmd=aa&seqid=1&msg=111*/);

	//获取http头中的域 和 值 
	//返回false时表示头中没有该域
	//注意:!!!仅当使用bIsFixedUrlEncodedBody为true,并且很明确的知道没有chunk报文才能获取到想要的数据,否则请调用get_http_body获取body
	//bool get_http_body_filed_value(const char* field, char** value, size_t* len);

	//获取http头中的域 和 值
	//返回false时表示头中没有该域
	//bool get_http_header_filed_value(const char* field, char** value, size_t* len);

protected:
	struct http_parser m_parser;
	struct item_value  
	{
		char* at;		//位置
		size_t len;		//长度
		item_value()
		{
			at  = NULL;
			len = 0;
		}
	};
	//chunk 块结构
	struct chunk_block 
	{
		char* at;		//位置
		size_t len;		//长度
		chunk_block()
		{
			at  = NULL;
			len = 0;
		}
	};
	//chunk 块链表
	struct chunk_list
	{
		chunk_block block;
		chunk_list* next;
		chunk_list()
		{
			next   = NULL;
		}
	};

	bool m_bLastIsValue;												//标记上次一解析的是不是http头中域对应的value
	char* m_pCurField;													//头中当前域名称的指针
	size_t m_nCurFieldLen;												//当前域名称的长度
	enum http_parser_type m_parser_type;								//解析器工作在何种类型下(request/response)
	chunk_list* m_pHeaderChunk;											//chunk链表头
	chunk_list* m_pCurChunk;											//chunk链表当前位置													
	std::map<std::string, item_value> m_mapHeaderFieldValue;			//HTTP头中的域与值的对应关系
	item_value m_itemValue;												//当前正在解析的域对应的值
	bool m_bIsFixedUrlEncodedBody;										//是否已明确知道body是urlencoded格式的,如:cmd=aa&seqid=1&msg=111
	http_urlencoded_body_parser m_urlencode_body_parser;				//body是urlencode格式的解析器
	bool m_bChunkPack;													//Chunk包标记
	bool m_bParseOver;													//是否解析完成标志
	header_state m_curr_header_state;									//用来标记http报文头当前回调到哪个了

	char* m_pBegin;														//包起始指针
	size_t m_nTotalParseLen;											//一共解析的总长度
	
	int m_nHttpStatusCode;												//HTTP回应的状态

	bool m_bIsKeepAlive;												//连接是否需要保持

	int   m_nReqType;													//请求类型 POST,GET
	char* m_pUrl;														//请求地址的指针
	size_t m_nUrlLen;													//请求地址的内容长度

	char* m_pBody;														//body指针--非chunk时用
	size_t m_nBodyLen;													//body长度

	int   m_nContentType;												//content-type
	char* m_pContentType;												//content-type的指针
	size_t m_nContentTypeLen;											//content-type内容的长度
	
	char* m_pHost;														//host的指针
	size_t m_nHostLen;													//host内容的长度
	
	char* m_pXForwardedFor;												//x_forwarded_for的指针
	size_t m_nXForwardedForLen;											//x_forwarded_for的长度
	
	char* m_pExpect;													//except的指针
	size_t m_nExpectLen;												//except的长度
	
	char* m_pSoapAction;												//soapaction的指针
	size_t m_nSoapActionLen;											//soapaction的长度

	char* m_pUserAgent;													//UserAgent的指针
	size_t m_nUserAgentLen;												//UserAgent的长度

	char* m_pVia;														//via的指针
	size_t m_nViaLen;													//via的长度

	char* m_pXWapProfile;												//x-wap-profile的指针
	size_t m_nXWapProfileLen;											//x-wap-profile的长度

	char* m_pXBrowserType;												//x-browser-type的指针
	size_t m_nXBrowserTypeLen;											//x-browser-type的长度

	char* m_pXUpBearType;												//x-up-bear-type的指针
	size_t m_nXUpBearTypeLen;											//x-up-bear-type的长度
};   

#pragma GCC diagnostic error "-Wconversion"
#pragma GCC diagnostic error "-Wold-style-cast"
}
