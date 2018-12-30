#include "http_parser.h"
#include <stdio.h>
#include <string>
#include "httpparser.h"

int main(void)
{

	mw_http_parser::HttpParser httpparser1(HTTP_REQUEST);

	std::string strHeader0 = "GET /sms/v2/std/single_send?cmd=mt&userid=111&pwd=222 HTTP/1.1\r\nContent-Type: application/x-www-form-urlencoded\r\nConnection: Close\r\n\r\n";
	//std::string strHeader0 = "POST /sms/v2/std/single_send HTTP/1.1\r\nContent-Type: text/json\r\nConnection: Close\r\nContent-Length: 10\r\n\r\n1234567890";
	bool bParseOver0 = false;
	int nRet0 = httpparser1.execute_http_parse(strHeader0.c_str(), 18, bParseOver0);
		nRet0 = httpparser1.execute_http_parse(strHeader0.c_str()+18, strHeader0.length()-18, bParseOver0);
	char* pUrl = NULL; size_t nurllen = 0;
	httpparser1.get_http_url(&pUrl, &nurllen);
	int nCttype = httpparser1.get_http_content_type();
	std::string strHeader1 = "POST /MWGate/wmgw.asmx/MongateGetDeliver HTTP/1.1\r\n"
		"CONTENT-TYPE: application/x-www-form-urlencoded; charset=utf-8\r\n"
		"Cache-Control: no-cache\r\n"
		"User-Agent: Java/1.7.0_79\r\n"
		"Host: 61.145.229.29:7791\r\n"
		"Connection: close\r\n"
		"x_forwarded_for: 192.168.1.1\r\n"
		"expect: 100-continue\r\n"
		"CONTENT-LENGTH: ";
	std::string strBody1 = "userid=J10003&pwd=26dad7f364507df18f3841cc9c4ff94d&mobile=138xxxxxxxx&content=%d1%e9%d6%a4%c2%eb%a3%ba6666%a3%ac%b4%f2%cb%c0%b6%bc%b2%bb%d2%aa%b8%e6%cb%df%b1%f0%c8%cb%c5%b6%a3%a1&timestamp=0803192020&svrtype=SMS001&exno=0006&custid=b3d0a2783d31b21b8573&exdata=exdata000002";
	char szBufBodyLen1[10] = {0};
	sprintf(szBufBodyLen1, "%d", strBody1.length());
	strHeader1 += (std::string)szBufBodyLen1;
	strHeader1 += "\r\n\r\n";
	
	bool bParseOver = false;
	int nRet = httpparser1.execute_http_parse(strHeader0.c_str(), strHeader0.length(), bParseOver);
	size_t bodylen0 = httpparser1.get_http_body_len();
	//httpparser1.parse_http_body(const char * p, size_t len)
	nRet = httpparser1.execute_http_parse(strHeader1.c_str(), strHeader1.length(), bParseOver);
	nRet = httpparser1.execute_http_parse(strBody1.c_str(), strBody1.length(), bParseOver);
	size_t bodylen1 = httpparser1.get_http_body_len();

	httpparser1.reset_parser();
	nRet = httpparser1.execute_http_parse(strHeader1.c_str(), strHeader1.length(), bParseOver);

	printf("%s\n", httpparser1.get_http_parse_errno_description(nRet));
	
	char* pContentType1 = NULL;
	size_t content_type_len1 = 0;
	int contenttype = httpparser1.get_http_content_type();

	httpparser1.should_keep_alive();

	httpparser1.expect_100_continue();
	httpparser1.get_http_response_status_code();


	std::string strHeader2 = "POST /MWGate/wmgw.asmx/MongateCsSpSendSmsNew HTTP/1.1\r\n"
							"Host: 192.169.2.88:8087\r\n"
							"Content-Type: text/json; charset=utf-8\r\n"
							"Connection: close\r\n"
							"Transfer-Encoding: chunked\r\n\r\n";
	std::string strBody2 = "4\r\n"
							"user\r\n"
							"6\r\n"
							"Id=shj\r\n"
							"6d\r\n"
							"002&password=123456&pszMobis=18900008888&pszMsg=%E6%B5%8B%E8%AF%95%2B%2D%25881854&iMobiCount=1&pszSubPort=123\r\n"
							"0\r\n\r\n";

	mw_http_parser::HttpParser httpparser2(HTTP_REQUEST);

	httpparser2.execute_http_parse(strHeader2.c_str(), strHeader2.length(), bParseOver);
	httpparser2.execute_http_parse(strBody2.c_str(), strBody2.length(), bParseOver);
	
	char* pUri = NULL;size_t urilen = 0;
	httpparser2.get_http_url(&pUri, &urilen);
    int n = httpparser2.get_http_request_type();
	size_t bodylen = httpparser2.get_http_body_len();

	char* p = new char[bodylen+1];
	httpparser2.should_keep_alive();
	
	httpparser2.get_http_response_status_code();
	char* pContentType = NULL;
	size_t content_type_len = 0;
	contenttype = httpparser2.get_http_content_type();

	p[bodylen] = '\0';
	size_t len = httpparser2.get_http_body_chunk_data(p);
	
	mw_http_parser::http_urlencoded_body_parser urlencoded_body_parser;
	if (urlencoded_body_parser.parse_http_body(p, len))
	{
		char* pvalue = NULL;
		size_t valelen = 0;
		if (urlencoded_body_parser.get_value("pszmsg", &pvalue, &valelen))
		{
		}
		urlencoded_body_parser.get_value("password", &pvalue, &valelen);
		urlencoded_body_parser.get_value("iMobiCount", &pvalue, &valelen);
	}


/*
	httpparser1.execute_http_parse(strHeader2.c_str(), strHeader2.length(), &bParseOver);
	httpparser1.execute_http_parse(strBody2.c_str(), strBody2.length(), &bParseOver);
	
	//const char* perrno = httpparser1.get_http_parse_errno_description(7);
	char* pUri = NULL;size_t urilen = 0;
	httpparser1.get_http_url(&pUri, &urilen);
    int n = httpparser1.get_http_request_type();
	size_t bodylen = httpparser1.get_http_body_len();
	
	char* p = new char[bodylen+1];
	p[bodylen] = '\0';
	size_t len = httpparser1.get_http_body_chunk_data(p);
	httpparser1.should_keep_alive();
	
	httpparser1.get_http_response_status_code();
*/
}