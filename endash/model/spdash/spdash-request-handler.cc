/*
 * spdash-request-handler.cc
 *
 *  Created on: 08-Apr-2020
 *      Author: abhijit
 */

#include "spdash-request-handler.h"
#include <ns3/core-module.h>
#include "ns3/nlohmann_json.h"
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>

using json = nlohmann::json;

namespace ns3 {

template<typename T>
std::string
ToString (T val)
{
  std::stringstream stream;
  stream << val;
  return stream.str ();
}

template<typename T>
std::string
FromString (T val)
{
  std::stringstream stream;
  stream << val;
  return stream.str ();
}

NS_LOG_COMPONENT_DEFINE("SpDashRequestHandler");
NS_OBJECT_ENSURE_REGISTERED(SpDashRequestHandler);

TypeId SpDashRequestHandler::GetTypeId(void) {
	static TypeId tid = TypeId("ns3::SpDashRequestHandler")
			.SetParent<DashRequestHandler>()
			.SetGroupName("Applications")
			.AddConstructor<SpDashRequestHandler>();
	return tid;
}



SpDashRequestHandler::SpDashRequestHandler (): DashRequestHandler()
{
}

SpDashRequestHandler::~SpDashRequestHandler ()
{
}

std::string
SpDashRequestHandler::CreateRequestString (std::string cookie, std::string nextChunkId,
                                           std::string lastQuality, std::string buffer,
                                           std::string lastRequest,
                                           std::string rebufferTime,
                                           std::string lastChunkFinishTime,
                                           std::string lastChunkStartTime,
                                           std::string lastChunkSize)
{
  json jsonBody;
//  jsonBody["bitrateArray"] = m_videoData.m_averageBitrate;

  if (cookie == "") jsonBody["cookie"] = nullptr;
  else jsonBody["cookie"] = cookie;
  std::cout << "cookie=" << cookie << "; nextChunkId=" << nextChunkId << "; lastQuality="
            << lastQuality << "; buffer=" << buffer << "; lastRequest=" << lastRequest
            << "; rebufferTime=" << rebufferTime << "; lastChunkFinishTime="
            << lastChunkFinishTime << "; lastChunkStartTime=" << lastChunkStartTime
            << "; lastChunkSize=" << lastChunkSize << "\n";
  jsonBody["nextChunkId"] = stoul (nextChunkId);
  jsonBody["lastquality"] = stoul (lastQuality);
  jsonBody["buffer"] = stod (buffer);  //in seconds

  jsonBody["lastRequest"] = stoul (lastRequest);
  jsonBody["rebufferTime"] = stod (rebufferTime);
  jsonBody["lastChunkFinishTime"] = stoul (lastChunkFinishTime);
  jsonBody["lastChunkStartTime"] = stoul (lastChunkStartTime);
  jsonBody["lastChunkSize"] = stoul (lastChunkSize);
  std::string jsonString = jsonBody.dump (4);
  int contentLength = jsonString.length ();
  std::string requestString = "POST / HTTP/1.1\r\nUser-Agent: Mozilla/4.0 (compatible; MSIE5.01; Windows NT)\r\nHost: www.tutorialspoint.com\r\nContent-Type: text/xml; charset=utf-8\r\nContent-Length: "
      + std::to_string (contentLength)
      + "\r\nAccept-Language: en-us\r\nAccept-Encoding: gzip, deflate\r\nConnection: Keep-Alive\r\n\r\n"
      + jsonString;
  return requestString;
}

//Adds header in response
int
SpDashRequestHandler::Abr (std::string cookie, std::string segmentNum,
                           std::string lastQuality, std::string buffer,
                           std::string lastRequest, std::string rebufferTime,
                           std::string lastChunkFinishTime,
                           std::string lastChunkStartTime, std::string lastChunkSize)
{
  const int PORT = 8333;
  int sock = 0, valread;
  struct sockaddr_in serv_addr;

  std::string to_send = CreateRequestString (cookie, segmentNum, lastQuality, buffer,
                                             lastRequest, rebufferTime,
                                             lastChunkFinishTime, lastChunkStartTime,
                                             lastChunkSize);

  // std::cout<< "POST / HTTP/1.1\r\nUser-Agent: Mozilla/4.0 (compatible; MSIE5.01; Windows NT)\r\nHost: www.tutorialspoint.com\r\nContent-Type: text/xml; charset=utf-8\r\nContent-Length: 260\r\nAccept-Language: en-us\r\nAccept-Encoding: gzip, deflate\r\nConnection: Keep-Alive\r\n\r\n{\n\"bitrateArray\":[400000,600000,1000000,1500000,2500000,4000000],\n\"cookie\":null,\n\"nextChunkId\":1,\n\"lastquality\":1,\n\"buffer\":8,\n\"lastRequest\":0,\n\"rebufferTime\":0,\n\"lastChunkFinishTime\":1594907984144,\n\"lastChunkStartTime\":1594907984101,\n\"lastChunkSize\":1366834\n}"<<"\n";
  const char *AbrRequest = to_send.c_str ();

//  printf ("\n####################AbrRequest#######################\n%s\n", AbrRequest);

  char buff[4096] = {0};
  if ((sock = socket (AF_INET, SOCK_STREAM, 0)) < 0)
    {
      printf ("\n Socket creation error \n");
      return -1;
    }

  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons (PORT);

  // Convert IPv4 and IPv6 addresses from text to binary form
  if (inet_pton (AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0)
    {
      printf ("\nInvalid address/ Address not supported \n");
      return -1;
    }

  if (connect (sock, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) < 0)
    {
      printf ("\nConnection Failed \n");
      return -1;
    }
  send (sock, AbrRequest, strlen (AbrRequest), 0);
  std::string entireContent;
  while (1)
    {
      valread = read (sock, buff, sizeof(buff));
      if (valread <= 0) break;
      buff[valread] = '\0';
      entireContent += buff;
    }
  std::string delimiter = "\r\n\r\n";
  std::string responseBody = entireContent.substr (entireContent.find (delimiter) + 4,
                                                   std::string::npos);
  json jsonResponse = json::parse (responseBody);
  AddHeader ("X-LastQuality", std::to_string (int (jsonResponse["quality"])));
  AddHeader ("X-Cookie", jsonResponse["cookie"]);
  AddHeader ("X-SegmentDuration", std::to_string(int64_t(jsonResponse["segmentDuration"])));
  AddHeader ("X-LastSegment", jsonResponse["lastSeg"].get<bool>() ? "1":"0");

  m_segmentSizes = jsonResponse["sizes"].get<std::vector<uint64_t> >();
  return int (jsonResponse["quality"]);
}

//store info in server
void
SpDashRequestHandler::RequestHeaderReceived ()
{
  NS_LOG_FUNCTION(this);
  // std::string responseLen = GetHeader("X-Require-Length");
  std::string responseLen = GetHeader ("X-Require-Length");	//non zero for mpd
  std::cout << "\n";
  if (stoul (responseLen) == 0)	//if not mpd
    {
//      m_videoFilePath = GetHeader ("X-PathToVideo");
      std::string segmentNum = GetHeader ("X-Require-Segment-Num");
      std::string lastChunkFinishTime = GetHeader ("X-LastChunkFinishTime");
      std::string lastChunkStartTime = GetHeader ("X-LastChunkStartTime");
      std::string buffer = GetHeader ("X-BufferUpto");
      std::string lastChunkSize = GetHeader ("X-LastChunkSize");
      std::string cookie = GetHeader ("X-Cookie");
      std::string lastQuality = GetHeader ("X-LastQuality");
      std::string rebufferTime = GetHeader ("X-Rebuffer");
//      ReadInVideoInfo ();
      std::string lastRequest = std::to_string (stoi (segmentNum) - 1);
      int nextQualityNum = Abr (cookie, segmentNum, lastQuality, buffer, lastRequest,
                                rebufferTime, lastChunkFinishTime, lastChunkStartTime,
                                lastChunkSize);
//      std::cout << "nextQualityNum = " << nextQualityNum << "\n";
      if (nextQualityNum == -1)
        {
          std::cerr << "can't connect to Abr server\n";
          return;
        }
      //nextqua
      // std::string nextQualityNumCookie = GetQualityFromAbr(m_videoData,cookie,segmentNum,lastQuality,lastRequest,lastChunkFinishTime,lastChunkStartTime,lastChunkSize);
      auto nextSegmentLength = m_segmentSizes.at (nextQualityNum);

      responseLen = std::to_string (nextSegmentLength);

      // m_lastChunkSize = nextSegmentLength;
    }
  else
    {
      AddHeader ("X-LastQuality", "0");
      AddHeader ("X-Cookie", "");
      AddHeader ("X-LastSegment", "0");
      AddHeader ("X-SegmentDuration", "0");
    }

  AddHeader ("X-LastChunkSize", responseLen);
  // AddHeader("field", "response test");
  if (!responseLen.empty ()) m_toSent = std::stoul (responseLen);
  SetClen (m_toSent);
  SetStatus (200, "OK");
  EndHeader ();
}

} /* namespace ns3 */
