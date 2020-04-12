#include "handleXml.h"
#include "util.h"
using namespace std;




bool CHandleXml::readXmlFile(string & strFilename)
{
	auto mydoc(UTIL_SELF::make_unique<TiXmlDocument>());
	bool loadOk = mydoc->LoadFile(strFilename.c_str());
	if (!loadOk)
	{
		LOG_ERROR("Error:%s", mydoc->ErrorDesc());
		return false;
	}

	TiXmlElement *RootElement = mydoc->RootElement();     
	TiXmlElement *pEle = NULL;

	string strNodeName = XML_NODE_NAME;
	GetNodePointerByName(RootElement, strNodeName.c_str(), pEle); 

	for (TiXmlElement *SearchModeElement = pEle->FirstChildElement(); SearchModeElement; SearchModeElement = SearchModeElement->NextSiblingElement())
	{
		vector<string> vecXml;		
		for (TiXmlElement *RegExElement = SearchModeElement->FirstChildElement(); RegExElement; RegExElement = RegExElement->NextSiblingElement())
		{
			string strValue = RegExElement->FirstChild()->Value();

			vecXml.push_back(strValue);

			string key = RegExElement->Value();
			LOG_INFO("xml: %s : %s", key.c_str(), strValue.c_str());
		}
        XML_SYSTEM_CONFIG & data = m_xml_system;
		if (serialSystemXml(data, vecXml))
		{				
			return true;
		}

		
	}
	return false;
}

bool CHandleXml::init()
{
    return readDefaultXml();
}

bool CHandleXml::readDefaultXml()
{
	if (m_strPath.empty())
		m_strPath = UTIL_SELF::getPwd();	
	string strFilename = m_strPath + XML_FILE_SYSTEM;
	

	return readXmlFile(strFilename);
}

bool CHandleXml::serialSystemXml(XML_SYSTEM_CONFIG & data, vector<string> & vecInfo)
{
	bool bResult = true;
	int total = static_cast<int>(vecInfo.size());
	int i = 0;
	do {
		VALUE_BREAK_IF(UTIL_SELF::setValue(data.run_mode, vecInfo, i, total));
        VALUE_BREAK_IF(UTIL_SELF::setValue(data.ip_v, vecInfo, i, total));
		VALUE_BREAK_IF(UTIL_SELF::setValue(data.tcp_port, vecInfo, i, total));
		VALUE_BREAK_IF(UTIL_SELF::setValue(data.http_port, vecInfo, i, total));
		VALUE_BREAK_IF(UTIL_SELF::setValue(data.https_port, vecInfo, i, total));
		VALUE_BREAK_IF(UTIL_SELF::setValue(data.thread_events, vecInfo, i, total));
        VALUE_BREAK_IF(UTIL_SELF::setValue(data.queue_capacity, vecInfo, i, total));
	} while (0);

	return bResult;
}



XML_SYSTEM_CONFIG CHandleXml::getSystemXml()
{
	std::lock_guard<std::mutex>lck(m_lock);
	return m_xml_system;
}



string CHandleXml::getPahtEx()
{	
	auto pos = m_strPath.rfind("/");
	if (string::npos != pos) {		
		return (m_strPath.substr(0, pos));
	}
	return m_strPath;
}

string CHandleXml::getPath()
{
	std::lock_guard<std::mutex>lck(m_lock);
	return m_strPath;
}


bool CHandleXml::GetNodePointerByName(TiXmlElement* pRootEle, const char* strNodeName, TiXmlElement* &destNode)
{
	if (0 == strcmp(strNodeName, pRootEle->Value()))
	{
		destNode = pRootEle;
		return true;
	}

	TiXmlElement* pEle = pRootEle;
	for (pEle = pRootEle->FirstChildElement(); pEle; pEle = pEle->NextSiblingElement())
	{	 
		if (0 != strcmp(pEle->Value(), strNodeName))
		{
			GetNodePointerByName(pEle, strNodeName, destNode);
		}
		else
		{
			destNode = pEle;
			printf("destination node name: %s\n", pEle->Value());
			return true;
		}
	}
	return false;
}

int CHandleXml::getThreadEvnetNum()
{
    std::lock_guard<std::mutex>lck(m_lock);
    return std::atoi(m_xml_system.thread_events.c_str());
}





