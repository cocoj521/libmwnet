// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

#include <mwnet_mt/net/http/HttpResponse.h>
#include <mwnet_mt/net/Buffer.h>

#include <stdio.h>

using namespace mwnet_mt;
using namespace mwnet_mt::net;

void HttpResponse::appendToBuffer(Buffer* output) const
{
  char buf[32];
  snprintf(buf, sizeof buf, "HTTP/1.1 %d ", statusCode_);
  output->append(buf);
  output->append(statusMessage_);
  output->append("\r\n");

  if (closeConnection_)
  {
    output->append("Connection: close\r\n");
  }
  else
  {
    snprintf(buf, sizeof buf, "Content-Length: %zd\r\n", body_.size());
    output->append(buf);
    output->append("Connection: Keep-Alive\r\n");
  }

  for (std::map<string, string>::const_iterator it = headers_.begin();
       it != headers_.end();
       ++it)
  {
    output->append(it->first);
    output->append(": ");
    output->append(it->second);
    output->append("\r\n");
  }

  output->append("\r\n");
  output->append(body_);
}
