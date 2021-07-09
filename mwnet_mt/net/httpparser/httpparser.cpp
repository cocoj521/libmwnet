#include "httpparser.h"

namespace mw_http_parser
{

#pragma GCC diagnostic ignored "-Wold-style-cast"
    struct http_parser_settings ms_settings;

    class HttpParserSettings
    {
    public:
        HttpParserSettings();

        static int onMessageBegin(struct http_parser*);
        static int onUrl(struct http_parser*, const char*, size_t);
        static int onStatus(struct http_parser*, const char*, size_t);
        static int onHeaderField(struct http_parser*, const char*, size_t);
        static int onHeaderValue(struct http_parser*, const char*, size_t);
        static int onHeadersComplete(struct http_parser*);
        static int onBody(struct http_parser*, const char*, size_t);
        static int onMessageComplete(struct http_parser*); 
		static int onChunkHeader(struct http_parser*); 
		static int onChunkComplete(struct http_parser*); 
    };

    HttpParserSettings::HttpParserSettings()
    {
        ms_settings.on_message_begin = &HttpParserSettings::onMessageBegin;
        ms_settings.on_url = &HttpParserSettings::onUrl;
        ms_settings.on_status = &HttpParserSettings::onStatus;
        ms_settings.on_header_field = &HttpParserSettings::onHeaderField;
        ms_settings.on_header_value = &HttpParserSettings::onHeaderValue;
        ms_settings.on_headers_complete = &HttpParserSettings::onHeadersComplete;
        ms_settings.on_body = &HttpParserSettings::onBody;
        ms_settings.on_message_complete = &HttpParserSettings::onMessageComplete; 
		ms_settings.on_chunk_header = &HttpParserSettings::onChunkHeader; 
		//ms_settings.on_chunk_complete = &HttpParserSettings::onChunkComplete; 
    }    

    static HttpParserSettings initObj;

    int HttpParserSettings::onMessageBegin(struct http_parser* parser)
    {
        HttpParser* p = (HttpParser*)parser->data;
        return p->onParser(HttpParser::Parser_MessageBegin, 0, 0);
    }

    int HttpParserSettings::onUrl(struct http_parser* parser, const char* at, size_t length)
    {
        HttpParser* p = (HttpParser*)parser->data;
        return p->onParser(HttpParser::Parser_Url, at, length);
    }

    int HttpParserSettings::onStatus(struct http_parser* parser, const char* at, size_t length)
    {
        HttpParser* p = (HttpParser*)parser->data;
        return p->onParser(HttpParser::Parser_Status, at, length);
    }

    int HttpParserSettings::onHeaderField(struct http_parser* parser, const char* at, size_t length)
    {
        HttpParser* p = (HttpParser*)parser->data;
        return p->onParser(HttpParser::Parser_HeaderField, at, length);
    }

    int HttpParserSettings::onHeaderValue(struct http_parser* parser, const char* at, size_t length)
    {
        HttpParser* p = (HttpParser*)parser->data;
        return p->onParser(HttpParser::Parser_HeaderValue, at, length);
    }

    int HttpParserSettings::onHeadersComplete(struct http_parser* parser)
    {
        HttpParser* p = (HttpParser*)parser->data;
        return p->onParser(HttpParser::Parser_HeadersComplete, 0, 0);
    }

    int HttpParserSettings::onBody(struct http_parser* parser, const char* at, size_t length)
    {
        HttpParser* p = (HttpParser*)parser->data;
        return p->onParser(HttpParser::Parser_Body, at, length);
    }

    int HttpParserSettings::onMessageComplete(struct http_parser* parser)
    {
        HttpParser* p = (HttpParser*)parser->data;
        return p->onParser(HttpParser::Parser_MessageComplete, 0, 0);
    }

	int HttpParserSettings::onChunkHeader(struct http_parser* parser)
    {
        HttpParser* p = (HttpParser*)parser->data;
        return p->onParser(HttpParser::Parser_ChunkHeaser, 0, 0);
    }

	int HttpParserSettings::onChunkComplete(struct http_parser* parser)
    {
        HttpParser* p = (HttpParser*)parser->data;
        return p->onParser(HttpParser::Parser_ChunkComplete, 0, 0);
    }

	HttpParser::HttpParser(enum http_parser_type type, bool bIsFixedUrlEncodedBody)
	{
	  m_parser_type = type;
	  
	  http_parser_init(&m_parser, type);

	  m_parser.data = this;

	  m_bIsFixedUrlEncodedBody = bIsFixedUrlEncodedBody;
	}

	HttpParser::HttpParser(enum http_parser_type type)
    {
    	m_parser_type = type;
		
        http_parser_init(&m_parser, type);
		
        m_parser.data = this;
		
		m_bIsFixedUrlEncodedBody = false;

		m_pHeaderChunk = new chunk_list();
    }

	void HttpParser::reset_parser()
	{
        http_parser_init(&m_parser, m_parser_type);
		
        m_parser.data = this;
		
		m_bIsFixedUrlEncodedBody = false;

		init_class_members();
	}

	//初始化类成员
	void HttpParser::init_class_members()
    {
    	m_pCurField = NULL;
		m_nCurFieldLen = 0;

		m_bLastIsValue = false;
        m_itemValue.at	= NULL;
		m_itemValue.len = 0;
		m_mapHeaderFieldValue.clear();
		m_urlencode_body_parser.clear();
		m_bChunkPack   = false;
		m_bParseOver   = false;
		m_curr_header_state = header_state_init;
			
        m_nHttpStatusCode = 0;
		
		m_bIsKeepAlive = false;
		
		m_nReqType = HTTP_REQUEST_UNKNOWN;

		m_pBegin = NULL;
		m_nTotalParseLen = 0;
			
		m_pUrl = NULL;
		m_nUrlLen = 0;

		m_pBody = NULL;
		m_nBodyLen = 0;

		m_nContentType = CONTENT_TYPE_UNKNOWN;
		m_pContentType = NULL;												//content-type的指针
		m_nContentTypeLen = 0;											//content-type内容的长度
		
		m_pHost = NULL;														//host的指针
		m_nHostLen = 0;													//host内容的长度
		
		m_pXForwardedFor = NULL;												//x_forwarded_for的指针
		m_nXForwardedForLen = 0;											//x_forwarded_for的长度
		
		m_pExpect = NULL;													//except的指针
		m_nExpectLen = 0;												//except的长度

		m_pSoapAction = NULL;												//soapaction的指针
		m_nSoapActionLen = 0;											//soapaction的长度
		
		m_pUserAgent = NULL; 												//UserAgent的指针
		m_nUserAgentLen = 0; 											//UserAgent的长度

		m_pVia = NULL;													//via的指针
		m_nViaLen = 0;												//via的长度

		m_pXWapProfile = NULL;												//x-wap-profile的指针
		m_nXWapProfileLen = 0;											//x-wap-profile的长度

		m_pXBrowserType = NULL;												//x-browser-type的指针
		m_nXBrowserTypeLen = 0;												//x-browser-type的长度

		m_pXUpBearType = NULL;												//x-up-bear-type的指针
		m_nXUpBearTypeLen = 0;												//x-up-bear-type的长度
		
		clear_chunk_list();
	}
	
    HttpParser::~HttpParser()
    {
		init_class_members();
		
		delete m_pHeaderChunk;
    }

    int HttpParser::onParser(Event event, const char* at, size_t length)
    {
        switch(event)
        {
            case Parser_MessageBegin:
                return handleMessageBegin();
            case Parser_Url:
                return handleUrl(at, length);
            case Parser_Status:
                return handleStatus(at, length);
            case Parser_HeaderField:
                return handleHeaderField(at, length);
            case Parser_HeaderValue:
                return handleHeaderValue(at, length);
            case Parser_HeadersComplete:
                return handleHeadersComplete();
            case Parser_Body:
                return handleBody(at, length);
            case Parser_MessageComplete:
                return handleMessageComplete();
			case Parser_ChunkHeaser:
                return handleChunkHeader();
			case Parser_ChunkComplete:
                return handleChunkComplete();
            default:
                break;
        }

        return 0;
    }

    int HttpParser::handleMessageBegin()
    {
        init_class_members();
        return onMessageBegin();
    }      
	
	int HttpParser::handleStatus(const char* at, size_t length)
    {	
		m_nHttpStatusCode = m_parser.status_code;
        return 0;
    }
        
	int HttpParser::handleBody(const char* at, size_t length)
    {	
		/*
		if (m_bIsFixedUrlEncodedBody)
		{
			//解析body
			m_urlencode_body_parser.parse_http_body(at, length);
		}
		*/
		if (m_bChunkPack)
		{
			m_pCurChunk->block.at	= (char*)at;
			m_pCurChunk->block.len	= length;
			m_pCurChunk->next = new chunk_list();
			m_pCurChunk = m_pCurChunk->next;
			m_nBodyLen += length;
		}
		else
		{
			if (NULL == m_pBody)
			{
				m_pBody	= (char*)at;
				m_nBodyLen = length;
			}
			else 
			{
				m_nBodyLen += length;
			}
		}
        return 0;
    }

	int HttpParser::handleUrl(const char* at, size_t length)
    {	
    	if (NULL == m_pUrl)
    	{
			m_pUrl = (char*)at;
    	}
		m_nUrlLen += length;
		//printf("handleUrl-%s,%ld\r\n", m_pUrl, length);
        return 0;
    }

    int HttpParser::handleHeaderField(const char* at, size_t length)
    {
    	if (m_bLastIsValue && NULL != m_pCurField)
    	{
			std::string strField(m_pCurField, m_nCurFieldLen);
			to_upper_s(strField);
			m_mapHeaderFieldValue.insert(std::make_pair(strField, m_itemValue));

			//当前域的名称,以及域对应的值均重置
			m_pCurField 	= NULL;
			m_nCurFieldLen 	= 0;
			m_itemValue.at	= NULL;
			m_itemValue.len = 0;

			//头状态为初始
			m_curr_header_state = header_state_init;

			m_bLastIsValue = false;
    	}

		if (NULL == m_pCurField) m_pCurField = (char*)at;
		m_nCurFieldLen += length;
			
		size_t nLen = m_nCurFieldLen;
		const char* p = (const char*)m_pCurField;
		
		switch (nLen)
		{
		case 3://via
			{
				if (0 == http_strnicmp(p, "via", nLen))
				{
					m_curr_header_state = header_state_via;
				}
			}
			break;
		case 4://host
			{
				if (0 == http_strnicmp(p, "host", nLen))
				{
					m_curr_header_state = header_state_host;
				}
			}
			break;
		case 6://expect
			{
				if (0 == http_strnicmp(p, "expect", nLen))
				{
					m_curr_header_state = header_state_expect;
				}
			}
			break;
		case 10://soapaction or User-Agent
			{
				if (0 == http_strnicmp(p, "user-agent", nLen))
				{
					m_curr_header_state = header_state_user_agent;
				}
				else if (0 == http_strnicmp(p, "soapaction", nLen))
				{
					m_curr_header_state = header_state_soap_action;
				}
			}
			break;
		case 12://content-type
			{
				if (0 == http_strnicmp(p, "content-type", nLen))
				{
					m_curr_header_state = header_state_content_type;
				}
			}
			break;
		case 13://x-wap-profile
			{
				if (0 == http_strnicmp(p, "x-wap-profile", nLen))
				{
					m_curr_header_state = header_state_x_wap_profile;
				}
			}
			break;
		case 14://x-browser-type/x_up_bear_type
			{
				if (0 == http_strnicmp(p, "x-browser-type", nLen))
				{
					m_curr_header_state = header_state_x_browser_type;
				}
				else if (0 == http_strnicmp(p, "x-up-bear-type", nLen))
				{
					m_curr_header_state = header_state_x_up_bear_type;
				}
			}
			break;
		case 15://x_forwarded_for
			{
				if (0 == http_strnicmp(p, "x-forwarded-for", nLen))
				{
					m_curr_header_state = header_state_x_forwarded_for;
				}
			}
			break;
		default:
			//其他几种暂不处理
			m_curr_header_state = header_state_init;
			break;
		}
		
        return 0;
    }
        
    int HttpParser::handleHeaderValue(const char* at, size_t length)
    {
    	if (NULL == m_itemValue.at) m_itemValue.at = (char*)at;
		m_itemValue.len += length;

		//置上一次解析的是http头中具体的域对应的值这一标志
		m_bLastIsValue = true;

		switch (m_curr_header_state)
		{
		case header_state_content_type:
			{
				if (NULL == m_pContentType)
				{
					m_pContentType    = (char*)at;
					m_nContentTypeLen = length;
				}
				else
				{
					m_nContentTypeLen += length;
				}
			}
			break;
		case header_state_x_forwarded_for:
			{
				if (NULL == m_pXForwardedFor)
				{
					m_pXForwardedFor    = (char*)at;
					m_nXForwardedForLen = length;
				}
				else
				{
					m_nXForwardedForLen += length;
				}
			}
			break;
		case header_state_host:
			{
				if (NULL == m_pHost)
				{
					m_pHost    = (char*)at;
					m_nHostLen = length;
				}
				else
				{
					m_nHostLen += length;
				}
			}
			break;
		case header_state_expect:
			{
				if (NULL == m_pExpect)
				{
					m_pExpect    = (char*)at;
					m_nExpectLen = length;
				}
				else
				{
					m_nExpectLen += length;
				}
			}
			break;
		case header_state_soap_action:
			{
				if (NULL == m_pSoapAction)
				{
					m_pSoapAction    = (char*)at;
					m_nSoapActionLen = length;
				}
				else
				{
					m_nSoapActionLen += length;
				}
			}
			break;
		case header_state_user_agent:
			{
				if (NULL == m_pUserAgent)
				{
					m_pUserAgent    = (char*)at;
					m_nUserAgentLen = length;
				}
				else
				{
					m_nUserAgentLen += length;
				}
			}
			break;
		case header_state_via:
			{
				if (NULL == m_pVia)
				{
					m_pVia    = (char*)at;
					m_nViaLen = length;
				}
				else
				{
					m_nViaLen += length;
				}
			}
			break;
		case header_state_x_wap_profile:
			{
				if (NULL == m_pXWapProfile)
				{
					m_pXWapProfile    = (char*)at;
					m_nXWapProfileLen = length;
				}
				else
				{
					m_nXWapProfileLen += length;
				}
			}
			break;
		case header_state_x_browser_type:
			{
				if (NULL == m_pXBrowserType)
				{
					m_pXBrowserType    = (char*)at;
					m_nXBrowserTypeLen = length;
				}
				else
				{
					m_nXBrowserTypeLen += length;
				}
			}
			break;
		case header_state_x_up_bear_type:
			{
				if (NULL == m_pXUpBearType)
				{
					m_pXUpBearType    = (char*)at;
					m_nXUpBearTypeLen = length;
				}
				else
				{
					m_nXUpBearTypeLen += length;
				}
			}
			break;
		default:
			break;
		}

        return 0;
    }
        
    int HttpParser::handleHeadersComplete()
    {
    	//头解完时,再进行一次判断防止最后一个域丢失
        if (m_bLastIsValue && NULL != m_pCurField)
    	{
			std::string strField(m_pCurField, m_nCurFieldLen);
			to_upper_s(strField);
			m_mapHeaderFieldValue.insert(std::make_pair(strField, m_itemValue));

			//当前域的名称,以及域对应的值均重置
			m_pCurField 	= NULL;
			m_nCurFieldLen 	= 0;
			m_itemValue.at	= NULL;
			m_itemValue.len = 0;

			//头状态为初始
			m_curr_header_state = header_state_init;

			m_bLastIsValue = false;
    	}
		
		m_bIsKeepAlive = http_should_keep_alive(&m_parser);
		m_nContentType = parse_content_type();
		m_nReqType     = parse_request_type();
		m_nHttpStatusCode = m_parser.status_code;
		
        return onHeadersComplete();
    }
	char* strnchr(const char* s, char c, int pos)
    {
    	char* p = (char*)s;
    	while (*s != '\0' && pos > 0)
    	{
    		if (*p == c) 
			{
				return p;
    		}
    		--pos;
			++p;
    	}
		return NULL;
    }
	int HttpParser::handleMessageComplete()
    {		
#pragma GCC diagnostic ignored "-Wconversion"		
		//这里要根据是否是GET请求，取出GET请求的URL和BODY
		if (HTTP_REQUEST_GET == m_nReqType)
		{
			char* pHelp = strnchr(m_pUrl, '?', 128);
			if (pHelp)
			{
				m_pBody = pHelp+1;
				int urllen = pHelp-m_pUrl;
				if (urllen >= 0)
				{
					int bodylen = m_nUrlLen-urllen-1;
					m_nBodyLen  = bodylen<0?0:bodylen;
					m_nUrlLen   = urllen;
				}
				else
				{
					m_nUrlLen  = 0;
					m_nBodyLen = 0;
				}
			}
		}
#pragma GCC diagnostic error "-Wconversion"

		m_bParseOver = true;
		//printf("handleMessageComplete-%s,%ld\r\n", m_pUrl, m_nUrlLen);
        return onMessageComplete();
    }

	int HttpParser::handleChunkHeader()
    {		
		m_bChunkPack = true;
        return onChunkHeader();
    }

	int HttpParser::handleChunkComplete()
    {		
        return onChunkComplete();
    }

//..........................附带提供一组解析urlencode的函数......................
	//通过key获取value的指针和长度,不做具体拷贝.若返回false代表key不存在
	//注意:必须先调用parse_http_body后才可以使用该函数
	bool HttpParser::get_value(const char* key/*key,如:cmd=aa,cmd就是key,aa就是value*/, char** value, size_t* len)
	{
		return m_urlencode_body_parser.get_value(key, value, len);
	}

	//通过key获取value的内容和长度.若返回false代表key不存在
	//注意:必须先调用parse_http_body后才可以使用该函数
	bool HttpParser::get_value(const char* key/*key,如:cmd=aa,cmd就是key,aa就是value*/, std::string& value)
	{
		return m_urlencode_body_parser.get_value(key, value);
	}

	//解析key,value格式的body,如:cmd=aa&seqid=1&msg=11
	//注意:仅当GetContentType返回CONTENT_TYPE_URLENCODE才可以使用该函数去解析
	bool HttpParser::parse_http_body(const char* p/*http body 如:cmd=aa&seqid=1&msg=11*/, size_t len/*http body的长度*/)
	{
		return m_urlencode_body_parser.parse_http_body(p, len);
	}
//................................................................................

    //执行解析.
	//可以执行N次,如果一个http报文,不是一次收到的,可以多次调用
	//返回值:0:本次解析OK  非0:本次解析失败 若失败请调用  获取失败原因
	//若over返回true,代表完成的http报文解析完成了,可以调其他函数进行http数据获取
	int HttpParser::execute_http_parse(const char* data/*本次要解析的数据*/, size_t len/*本次要解析的数据长度*/, bool& over/*本次是否把整个http报解析完成*/)
    {
		int nErrCode = 0;

        size_t parsed = http_parser_execute(&m_parser, &ms_settings, data, len);
        if (m_parser.upgrade)
        {
            onUpgrade(data + parsed, len - parsed); 
        }
        else if (parsed != len)
        {
			nErrCode = m_parser.http_errno;
        }     
		else
		{
			nErrCode = 0;
		}

		over = m_bParseOver;

		if (m_pBegin == NULL)
		{
			m_pBegin = (char*)data;
		}
		
		m_nTotalParseLen += len;
		
        return nErrCode;
    }

	//根据execute_http_parse返回的失败代码,获取失败原因
	const char* HttpParser::get_http_parse_errno_description(int errno)
	{
		return http_errno_description((enum http_errno)errno);
	}

	//获取http回应的状态码,如200,202,400,403,404,500等
	//只有当初始化为HTTP_RESPONSE解析时才能获取到正确的值
	//返回值:状态码
	int HttpParser::get_http_response_status_code()
    {
		return m_nHttpStatusCode;
    }

	//是否需要保持连接,即connection是keep-alive还是close
	//true:keep-alive false:close
	bool HttpParser::should_keep_alive()
	{
		return m_bIsKeepAlive;
	}

	//是否有100 continue
	bool HttpParser::expect_100_continue()
	{
		bool bRet = false;
		if (m_pExpect != NULL)
		{
			//100-continue
			if (12 == m_nExpectLen && 0 == http_strnicmp(m_pExpect, "100-continue", m_nExpectLen))
			{
				bRet = true;
			}
		}
		return bRet;
	}

	//是否是chunk包
	//返回值:true:chunk包 false:非chunk包
	bool HttpParser::is_chunk_body()
	{
		return m_bChunkPack;
	}

	/*
	//获取http头中的域 和 值
	//返回false时表示头中没有该域
	bool HttpParser::get_http_header_filed_value(const char* field, char** value, size_t* len)
	{
		bool bRet = false;
		std::string strField = field;
		to_upper_s(strField);
		std::map<std::string, item_value>::iterator it = m_mapHeaderFieldValue.find(strField);
		if (it != m_mapHeaderFieldValue.end())
		{
			*value = it->second.at;
			*len   = it->second.len;
			bRet   = true;
		}

		return bRet;
	}
	
	//获取http头中的域 和 值  !!!仅当使用bIsFixedUrlEncodedBody为true时才能获取到想要的数据
	//返回false时表示头中没有该域
	bool HttpParser::get_http_body_filed_value(const char* field, char** value, size_t* len)
	{
		bool bRet = false;
		if (m_bIsFixedUrlEncodedBody)
		{
			bRet = m_urlencode_body_parser.get_value(field, value, len);
		}
		return bRet;
	}
	*/

	//获取http body的长度
	//返回值:body的长度
	size_t HttpParser::get_http_body_len()
	{
		return m_nBodyLen;
	}

	//获取http请求的报文长度(头+体)
	//返回值:头+体的长度
	size_t  HttpParser::get_http_total_req_len()
	{
		return m_nTotalParseLen;
	}

	//获取http body的数据
	//返回值:body的指针和长度
	size_t HttpParser::get_http_body_data(char** data, size_t* len)
	{
		*data = m_pBody;
		*len  = m_nBodyLen;
		return m_nBodyLen;
	}

	//获取chunk包的http body的数据
	//返回值:body的长度
	//注意:需配合is_chunk_body先判断是否是chunk包,然后调用get_http_body_len得到长度分配足够的大小给data
	size_t HttpParser::get_http_body_chunk_data(char* data)
	{
		chunk_list* next = m_pHeaderChunk->next;
		size_t len = 0;

		if (m_pHeaderChunk->block.at != NULL)
		{
			memcpy(data + len, m_pHeaderChunk->block.at, m_pHeaderChunk->block.len);
			len += m_pHeaderChunk->block.len;
		}

		for (;;)
		{
			if (next != NULL)
			{
				chunk_list* p = next->next;
				
				if (next->block.at != NULL)
				{
					memcpy(data + len, next->block.at, next->block.len);
					len += next->block.len;
				}

				next = p;
			}
			else
			{
				break;
			}
		}
		return len;
	}

	//获取host
	//返回值:host的指针,以及长度
	size_t HttpParser::get_http_host(char** host, size_t* len)
	{
		size_t retlen = 0;
		if (m_pHost != NULL)
		{
			*host = m_pHost;
			*len  = m_nHostLen;
			retlen= *len;
		}
		return retlen;
		/*
		size_t retlen = 0;

		std::map<std::string, item_value>::iterator it = m_mapHeaderFieldValue.find("HOST");
		if (it != m_mapHeaderFieldValue.end())
		{
			*host	= it->second.at;
			*len	= it->second.len;
			retlen	= *len;	
		}
		
		return retlen;
		*/
	}

	//获取请求url
	//返回值:url的指针,以及长度
	size_t HttpParser::get_http_url(char** url, size_t* len)
	{
		size_t retlen = 0;
		if (m_pUrl != NULL)
		{
			*url  = m_pUrl;
			*len  = m_nUrlLen;
			retlen = *len;
		}
		return retlen;
	}

	//获取User-Agent
	//返回值:User-Agent的指针,以及长度
	size_t	HttpParser::get_http_user_agent(char** useragent, size_t* len)
	{
		size_t retlen = 0;
		if (m_pUserAgent != NULL)
		{
			*useragent  = m_pUserAgent;
			*len  		= m_nUserAgentLen;
			retlen = *len;
		}
		return retlen;
	}

	//获取content-type
	//返回值:CONTENT_TYPE_XML 代表 xml,CONTENT_TYPE_JSON 代表 json,CONTENT_TYPE_URLENCODE 代表 urlencode,CONTENT_TYPE_UNKNOWN 代表 未知类型
	//说明:暂只提供这三种
	int HttpParser::get_http_content_type()
	{
		return m_nContentType;
	}
	std::string	HttpParser::get_http_content_type_str()
	{
		std::string strContentType = "";
		if (m_pContentType != NULL && m_nContentTypeLen > 0)
		{
			strContentType.assign(m_pContentType, m_nContentTypeLen);
		}
		
		return strContentType;
	}
	int HttpParser::parse_content_type()
	{
		int contenttype = CONTENT_TYPE_UNKNOWN;
		if (m_pContentType != NULL)
		{
			size_t len = 0;
			char* p = m_pContentType;
			while (len < m_nContentTypeLen)
			{
				if (p[len] == '/')
				{
					if (m_nContentTypeLen - len >= 3 && 0 == http_strnicmp(p+len+1, "xml", 3))
					{
						contenttype = CONTENT_TYPE_XML;
						break;
					}
					else if (m_nContentTypeLen - len >= 4 && 0 == http_strnicmp(p+len+1, "json", 4))
					{
						contenttype = CONTENT_TYPE_JSON;
						break;
					}
					else if (m_nContentTypeLen - len >= 21 && 0 == http_strnicmp(p+len+1, "x-www-form-urlencoded", 21))
					{
						contenttype = CONTENT_TYPE_URLENCODE;
						break;
					}
					else
					{
						++len;
					}
				}
				else
				{
					++len;
				}
			}
		}
		return contenttype;
		/*
		size_t retlen = 0;
		
		std::map<std::string, item_value>::iterator it = m_mapHeaderFieldValue.find("CONTENT-TYPE");
		if (it != m_mapHeaderFieldValue.end())
		{
			*content_type	= it->second.at;
			*len			= it->second.len;
			retlen			= *len;	
		}
		
		return retlen;
		*/
	}

	//获取X-Forwarded-For
	//返回值:X-Forwarded-For的指针,以及长度
	size_t HttpParser::get_http_x_forwarded_for(char** x_forwarded_for, size_t* len)
	{
		size_t retlen = 0;
		if (m_pXForwardedFor != NULL)
		{
			*x_forwarded_for	= m_pXForwardedFor;
			*len				= m_nXForwardedForLen;
			retlen = *len;
		}
		return retlen;
		/*
		size_t retlen = 0;
		
		std::map<std::string, item_value>::iterator it = m_mapHeaderFieldValue.find("X-FORWARDED-FOR");
		if (it != m_mapHeaderFieldValue.end())
		{
			*x_forwarded_for	= it->second.at;
			*len				= it->second.len;
			retlen				= *len;	
		}
		
		return retlen;
		*/
	}

	//获取SOAPAction
	//返回值:SOAPAction的指针,以及长度
	size_t HttpParser::get_http_soapaction(char** soapaction, size_t* len)
	{
		size_t retlen = 0;
		if (m_pSoapAction != NULL)
		{
			*soapaction	= m_pSoapAction;
			*len		= m_nSoapActionLen;
			retlen = *len;
		}
		return retlen;
	}

	//获取 wap via
	//返回值:via的指针,以及长度
	size_t	HttpParser::get_http_via(char** via, size_t* len)
	{
		size_t retlen = 0;
		if (m_pVia != NULL)
		{
			*via		= m_pVia;
			*len		= m_nViaLen;
			retlen = *len;
		}
		return retlen;
	}

	//获取 X-WAP-PROFILE
	//返回值:X-WAP-PROFILE 的指针,以及长度
	size_t	HttpParser::get_http_x_wap_profile(char** x_wap_profile, size_t* len)
	{
		size_t retlen = 0;
		if (m_pXWapProfile != NULL)
		{
			*x_wap_profile	= m_pXWapProfile;
			*len			= m_nXWapProfileLen;
			retlen = *len;
		}
		return retlen;
	}

	//获取 X-BROWSER-TYPE
	//返回值:X-BROWSER-TYPE 的指针,以及长度
	size_t	HttpParser::get_http_x_browser_Type(char** x_browser_type, size_t* len)
	{
		size_t retlen = 0;
		if (m_pXBrowserType != NULL)
		{
			*x_browser_type	= m_pXBrowserType;
			*len			= m_nXBrowserTypeLen;
			retlen = *len;
		}
		return retlen;
	}

	//获取 X-UP-BEAR-TYPE
	//返回值:X-UP-BEAR-TYPE 的指针,以及长度
	size_t	HttpParser::get_http_x_up_bear_Type(char** x_up_bear_type, size_t* len)
	{
		size_t retlen = 0;
		if (m_pXUpBearType != NULL)
		{
			*x_up_bear_type	= m_pXUpBearType;
			*len			= m_nXUpBearTypeLen;
			retlen = *len;
		}
		return retlen;
	}

	//获取 http头中任一个域对应的值
	//返回值:域值的指针和长度
	size_t	HttpParser::get_http_header_field_value(const char* field, char** value, size_t* len)
	{
		size_t retlen = 0;
		std::string strField = field;
		to_upper_s(strField);
		std::map<std::string, item_value>::iterator it = m_mapHeaderFieldValue.find(strField);
		if (it != m_mapHeaderFieldValue.end())
		{
			if (it->second.at != NULL)
			{
				*value = it->second.at;
				*len   = it->second.len;
				retlen = *len;
			}
		}
		return retlen;
	}

	//获取 整个http报文的内容(头+体)
	//返回值:http报文的指针,以及长度
	size_t	HttpParser::get_http_request_content(char** request_content, size_t* len)
	{
		size_t retlen = 0;
		if (m_pBegin != NULL)
		{
			*request_content	= m_pBegin;
			*len				= m_nTotalParseLen;
			retlen = *len;
		}
		return retlen;
	}
	
	//获取http请求的类型
	//返回值:HTTP_REQUEST_POST 代表 post请求; HTTP_REQUEST_GET 代表 get请求 ; HTTP_REQUEST_UNKNOWN 代表 未知(解析包头有错误)
	int HttpParser::get_http_request_type()
	{
		return m_nReqType;
	}
	int HttpParser::parse_request_type()
	{
		char* p = m_pUrl;
		if (p != NULL 
			&& ((*(p-2) == 'T' || *(p-2) == 't')) 
			&& ((*(p-3) == 'S' || *(p-3) == 's'))
			&& ((*(p-4) == 'O' || *(p-4) == 'o'))
			&& ((*(p-5) == 'P' || *(p-5) == 'p')))
		{
			return HTTP_REQUEST_POST;	
		}
		else if (p != NULL 
			&& ((*(p-2) == 'T' || *(p-2) == 't')) 
			&& ((*(p-3) == 'E' || *(p-3) == 'e'))
			&& ((*(p-4) == 'G' || *(p-4) == 'g')))
		{
			return HTTP_REQUEST_GET;
		}
		else
		{
			return HTTP_REQUEST_UNKNOWN;
		}
	}

#pragma GCC diagnostic error "-Wold-style-cast"
}

